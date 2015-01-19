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

static msg_t adcThread(void *UNUSED(arg));
static uint8_t thread_run = 0;
static uint16_t adc_latest[SC_ADC_MAX_CHANNELS] = {0};
static systime_t adc_latest_ts = 0;
static uint16_t interval_ms;
static mutex_t adc_mtx;

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
static THD_WORKING_AREA(adc_thread, 1024);
THD_FUNCTION(adcThread, arg)
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

  while (thread_run) {
    msg_t retval;
    int i, ch;
    int total_adc[SC_ADC_MAX_CHANNELS] = {0};
    adcsample_t samples[SC_ADC_MAX_CHANNELS * SC_ADC_BUFFER_DEPTH];
    systime_t time_now;

    // Do ADC conversion(s) once per interval
    last_conversion_time += MS2ST(interval_ms);
    time_now = ST2MS(chVTGetSystemTime());

    if (last_conversion_time < time_now) {
      chDbgAssert(0, "Too slow ADC for specified interval");
      last_conversion_time = time_now + MS2ST(1);
    }

    chThdSleepUntil(last_conversion_time);

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
  }

  /* Stop ADC driver */
#ifndef BOARD_ST_STM32VL_DISCOVERY
  // FIXME: not defined on F1?
  // FIXME: is this needed only for internal temp, bat and vref ADCs?
  adcSTM32DisableTSVREFE();
#endif
  adcStop(&ADCDX);

  //FIXME: adcReleaseBus?

  return 0;
}


void sc_adc_init(void)
{
  chMtxObjectInit(&adc_mtx);
}



void sc_adc_deinit(void)
{
  // Nothing to do here.
}



void sc_adc_start_conversion(uint8_t channels, uint16_t interval_in_ms, uint8_t sample_time)
{
#if defined(BOARD_SNOWCAP_V1)
#elif defined(BOARD_SNOWCAP_STM32F4_V1) || defined (BOARD_ST_STM32F4_DISCOVERY)
  ioportmask_t mask = 0;
  uint8_t c;

  for (c = 0; c < channels; ++c) {
    mask |= PAL_PORT_BIT(c);
  }
  // FIXME: the pins should be in sc_conf.h
  palSetGroupMode(GPIOA, mask, 0, PAL_MODE_INPUT_ANALOG);
#endif

  // Set global interval time in milliseconds
  interval_ms = interval_in_ms;

  convCfg.num_channels = channels;
  convCfg.sqr1 = ADC_SQR1_NUM_CH(channels);

  if (channels < 1 || channels > 4) {
    chDbgAssert(0, "Invalid amount of channels");
    return;
  }

#if defined(BOARD_SNOWCAP_V1)
  // FIXME: the following hardcoded pins should be somehow in sc_conf.h
  // SC Control Board v1 maps PC0, PC1, PC4, and PC5 to AN1-4
  //
  // PC0: ADC123_IN10
  // PC1: ADC123_IN11
  // PC4: ADC12_IN14
  // PC5: ADC12_IN15

  switch(channels) {
  case 4: // Sampling time and channel for 4th pin
    convCfg.smpr1 |= ADC_SMPR1_SMP_AN15(sample_time);
    convCfg.sqr3  |= ADC_SQR3_SQ4_N(ADC_CHANNEL_IN15);
  case 3: // Sampling time and channel for 3rd pin
    convCfg.smpr1 |= ADC_SMPR1_SMP_AN14(sample_time);
    convCfg.sqr3  |= ADC_SQR3_SQ3_N(ADC_CHANNEL_IN14);
  case 2: // Sampling time and channel for 2nd pin
    convCfg.smpr1 |= ADC_SMPR1_SMP_AN11(sample_time);
    convCfg.sqr3  |= ADC_SQR3_SQ2_N(ADC_CHANNEL_IN11);
  case 1: // Sampling time and channel for 1st pin
    convCfg.smpr1 |= ADC_SMPR1_SMP_AN10(sample_time);
    convCfg.sqr3  |= ADC_SQR3_SQ1_N(ADC_CHANNEL_IN10);
    break;
  }
#elif defined(BOARD_SNOWCAP_STM32F4_V1) || defined (BOARD_ST_STM32F4_DISCOVERY)
  //
  // FIXME: the following hardcoded pins should be somehow in sc_conf.h
  // SC STM32F4 MCU Board v1 maps PA0, PA1, PA2, and PA3 to AN1-4
  //
  // PA0: ADC123_IN0
  // PA1: ADC123_IN1
  // PA2: ADC123_IN2
  // PA3: ADC123_IN3
  //

  switch(channels) {
  case 4: // Sampling time and channel for 4th pin
    convCfg.smpr1 |= ADC_SMPR2_SMP_AN3(sample_time);
    convCfg.sqr3  |= ADC_SQR3_SQ4_N(ADC_CHANNEL_IN3);
  case 3: // Sampling time and channel for 3rd pin
    convCfg.smpr1 |= ADC_SMPR2_SMP_AN2(sample_time);
    convCfg.sqr3  |= ADC_SQR3_SQ3_N(ADC_CHANNEL_IN2);
  case 2: // Sampling time and channel for 2nd pin
    convCfg.smpr1 |= ADC_SMPR2_SMP_AN1(sample_time);
    convCfg.sqr3  |= ADC_SQR3_SQ2_N(ADC_CHANNEL_IN1);
  case 1: // Sampling time and channel for 1st pin
    convCfg.smpr1 |= ADC_SMPR2_SMP_AN0(sample_time);
    convCfg.sqr3  |= ADC_SQR3_SQ1_N(ADC_CHANNEL_IN0);
    break;
  }
#else
  (void)sample_time;
  chDbgAssert(0, BOARD_NAME " is not supported in ADC yet. Please fix this logic. ");
#endif
  /* Start a thread dedicated to ADC conversion */
  thread_run = TRUE;
  chThdCreateStatic(adc_thread, sizeof(adc_thread), NORMALPRIO, adcThread, NULL);
}


void sc_adc_stop_conversion(void)
{
  thread_run = FALSE;
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
