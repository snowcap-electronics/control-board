/***
 * ADC functions
 *
 * Copyright 2012 Tuomas Kulve, <tuomas.kulve@snowcap.fi>
 *                Kalle Vahlman, <kalle.vahlman@snowcap.fi>
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following
 * conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#include "sc_conf.h"
#include "sc_utils.h"
#include "sc_adc.h"
#include "sc_event.h"

#if HAL_USE_ADC

/* Max number of pins to ADC */
#define SC_ADC_MAX_CHANNELS           4

/* SC_ADC_BUFFER_DEPTH in bits for shifting instead of division */
#define SC_ADC_BUFFER_DEPTH_BITS      2
/* How many samples to read before processing */
#define SC_ADC_BUFFER_DEPTH          (1 << SC_ADC_BUFFER_DEPTH_BITS)

static uint16_t adc_latest[SC_ADC_MAX_CHANNELS] = {0};
static systime_t adc_latest_ts = 0;
static uint16_t interval_ms;
static mutex_t adc_mtx;
static thread_t *adc_thread = NULL;

static ADCConversionGroup convCfg = {
  /* Circular buffer mode */
  FALSE,
  /* Sampled channels, will be overwritten */
  0,
  /* Conversion completed callback */
  NULL,
  /* Error callback */
  NULL,
  /* CR1 register setup */
  0,
  /* CR2 register setup */
  ADC_CR2_SWSTART,
  /* Sample time setup for channels 10..17, will be overwritten */
  0,
  /* Sample time setup for channels 0..9,   will be overwritten  */
  0,
  /* Conversion sequence setup register 1
	 The sequence length is encoded here,     will be overwritten */
  0,
  /* Conversion sequence setup register 2,  will be overwritten */
  0,
  /* Conversion sequence setup register 3,  will be overwritten */
  0,
};


/*
 * Setup a adc conversion thread
 */
static THD_WORKING_AREA(adc_loop_thread, 1024);
THD_FUNCTION(adcLoopThread, arg)
{
  msg_t msg;
  systime_t last_conversion_time;

  (void)arg;

  chRegSetThreadName(__func__);

  msg = sc_event_msg_create_type(SC_EVENT_TYPE_ADC_AVAILABLE);

  //FIXME: adcAcquireBus?

  /* Start ADC driver */
  adcStart(&ADCDX, NULL);

#ifndef BOARD_ST_STM32VL_DISCOVERY
  // FIXME: not defined on F1?
  // FIXME: is this needed only for internal temp, bat and vref ADCs?
  adcSTM32EnableTSVREFE();
#endif

  last_conversion_time = ST2MS(chVTGetSystemTime());

  while (!chThdShouldTerminateX()) {
    msg_t retval;
    int i, ch;
    int total_adc[SC_ADC_MAX_CHANNELS] = {0};
    adcsample_t samples[SC_ADC_MAX_CHANNELS * SC_ADC_BUFFER_DEPTH];
    systime_t time_now;

    // Do ADC conversion(s) once per interval
    if (interval_ms) {
      last_conversion_time += MS2ST(interval_ms);
      time_now = ST2MS(chVTGetSystemTime());

      if (last_conversion_time < time_now) {
        chDbgAssert(0, "Too slow ADC for specified interval");
        last_conversion_time = time_now + MS2ST(1);
      }

      chThdSleepUntil(last_conversion_time);
      if (chThdShouldTerminateX()) {
        break;
      }
    }

    retval = adcConvert(&ADCDX, &convCfg, samples, SC_ADC_BUFFER_DEPTH);
    if (retval != MSG_OK) {
      continue;
    }

    // Calculate average byt first summing the adc values ..
    for(i = 0; i < SC_ADC_BUFFER_DEPTH; i++) {
      for (ch = 0; ch < convCfg.num_channels; ++ch) {
        total_adc[ch] += samples[i*convCfg.num_channels + ch];
      }
    }

    chMtxLock(&adc_mtx);
    // .. and then dividing
    for (ch = 0; ch < convCfg.num_channels; ++ch) {
      adc_latest[ch] = (uint16_t)(total_adc[ch] >> SC_ADC_BUFFER_DEPTH_BITS);
    }

    adc_latest_ts = ST2MS(last_conversion_time);
    chMtxUnlock(&adc_mtx);

    // Announce that new ADC data is available
    sc_event_msg_post(msg, SC_EVENT_MSG_POST_FROM_NORMAL);

    if (!interval_ms) {
      break;
    }
  }

  /* Stop ADC driver */
#ifndef BOARD_ST_STM32VL_DISCOVERY
  // FIXME: not defined on F1?
  // FIXME: is this needed only for internal temp, bat and vref ADCs?
  adcSTM32DisableTSVREFE();
#endif
  adcStop(&ADCDX);

  //FIXME: adcReleaseBus?
}


void sc_adc_init(void)
{
  chMtxObjectInit(&adc_mtx);
}



void sc_adc_deinit(void)
{
  if (adc_thread) {
    chThdTerminate(adc_thread);
    adc_thread = NULL;
    // FIXME: should wait for thread exit
  }
}



void sc_adc_start_conversion(uint8_t channels, uint16_t interval_in_ms, uint8_t sample_time)
{
  chDbgAssert(adc_thread == NULL, "ADC thread already running");

  // Set global interval time in milliseconds
  interval_ms = interval_in_ms;

  convCfg.num_channels = channels;
  convCfg.sqr1 = ADC_SQR1_NUM_CH(channels);

  switch(channels) {
  #ifdef SC_ADC_4_PIN
  case 4:
    palSetPadMode(SC_ADC_4_PORT, SC_ADC_4_PIN, PAL_MODE_INPUT_ANALOG);
    convCfg.SC_ADC_4_SMPR_CFG |= SC_ADC_4_SMPR(sample_time);
    convCfg.sqr3 |= ADC_SQR3_SQ4_N(SC_ADC_4_AN);
    /* fallthrough */
  #endif
  #ifdef SC_ADC_3_PIN
  case 3:
    palSetPadMode(SC_ADC_3_PORT, SC_ADC_3_PIN, PAL_MODE_INPUT_ANALOG);
    convCfg.SC_ADC_3_SMPR_CFG |= SC_ADC_3_SMPR(sample_time);
    convCfg.sqr3 |= ADC_SQR3_SQ3_N(SC_ADC_3_AN);
    /* fallthrough */
  #endif
  #ifdef SC_ADC_2_PIN
  case 2:
    palSetPadMode(SC_ADC_2_PORT, SC_ADC_2_PIN, PAL_MODE_INPUT_ANALOG);
    convCfg.SC_ADC_2_SMPR_CFG |= SC_ADC_2_SMPR(sample_time);
    convCfg.sqr3 |= ADC_SQR3_SQ2_N(SC_ADC_2_AN);
    /* fallthrough */
  #endif
  #ifdef SC_ADC_1_PIN
  case 1:
    palSetPadMode(SC_ADC_1_PORT, SC_ADC_1_PIN, PAL_MODE_INPUT_ANALOG);
    convCfg.SC_ADC_1_SMPR_CFG |= SC_ADC_1_SMPR(sample_time);
    convCfg.sqr3 |= ADC_SQR3_SQ1_N(SC_ADC_1_AN);
    break;
  #endif
  default:
    chDbgAssert(0, "Unsupported amount of channels");
    break;
  }

  /* Start a thread dedicated to ADC conversion */
  adc_thread = chThdCreateStatic(adc_loop_thread, sizeof(adc_loop_thread), NORMALPRIO, adcLoopThread, NULL);
}



void sc_adc_stop_conversion(void)
{
  chThdTerminate(adc_thread);
  adc_thread = NULL;
  // FIXME: should wait for thread exit
}



void sc_adc_channel_get(uint16_t *channels, systime_t *ts)
{
  uint8_t ch;

  chMtxLock(&adc_mtx);
  for (ch = 0; ch < convCfg.num_channels; ++ch) {
    channels[ch] = adc_latest[ch];
  }
  *ts = adc_latest_ts;
  chMtxUnlock(&adc_mtx);
}

#endif // HAL_USE_ADC

/* Emacs indentatation information
   Local Variables:
   indent-tabs-mode:nil
   tab-width:2
   c-basic-offset:2
   End:
*/
