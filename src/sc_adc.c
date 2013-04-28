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

/* Max number of pins to ADC */
#define SC_ADC_MAX_CHANNELS           4

/* SC_ADC_BUFFER_DEPTH in bits for shifting instead of division */
#define SC_ADC_BUFFER_DEPTH_BITS      2
/* How many samples to read before processing */
#define SC_ADC_BUFFER_DEPTH          (1 << SC_ADC_BUFFER_DEPTH_BITS)


static msg_t tempThread(void *UNUSED(arg));
static uint8_t thread_run = 0;
static uint16_t adc_latest[SC_ADC_MAX_CHANNELS] = {0};
static uint16_t interval_ms;
static systime_t last_conversion_time;


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
static WORKING_AREA(temp_thread, 256);
static msg_t tempThread(void *UNUSED(arg))
{
  msg_t msg;

  msg = sc_event_msg_create_type(SC_EVENT_TYPE_ADC_AVAILABLE);

  //FIXME: adcAcquireBus?

  /* Start ADC driver */
  adcStart(&ADCDx, NULL);
  // FIXME: is this needed only for internal temp, bat and vref ADCs?
  adcSTM32EnableTSVREFE();

  while (thread_run) {
    msg_t retval;
    int i, ch;
    int total_adc[SC_ADC_MAX_CHANNELS] = {0};
    adcsample_t samples[SC_ADC_MAX_CHANNELS * SC_ADC_BUFFER_DEPTH];
    systime_t time_now;

    // Do a temperature conversion once per interval
    last_conversion_time += MS2ST(interval_ms);
    time_now = chTimeNow();
    chDbgAssert(last_conversion_time > time_now, "Too slow ADC for specified interval", "#1");

    chThdSleepUntil(last_conversion_time);

    retval = adcConvert(&ADCDx, &convCfg, samples, SC_ADC_BUFFER_DEPTH);
    if (retval != RDY_OK) {
      continue;
    }

    // Calculate average byt first summing the adc values ..
    for(i = 0; i < SC_ADC_BUFFER_DEPTH; i++) {
      for (ch = 0; ch < convCfg.num_channels; ++ch) {
        total_adc[ch] += samples[i*convCfg.num_channels + ch];
      }
    }

    // .. and then dividing
    for (ch = 0; ch < convCfg.num_channels; ++ch) {
      adc_latest[ch] = (uint16_t)(total_adc[ch] >> SC_ADC_BUFFER_DEPTH_BITS);
    }

    // Announce that new ADC data is available
    sc_event_msg_post(msg, SC_EVENT_MSG_POST_FROM_NORMAL);
  }

  /* Stop ADC driver */
  // FIXME: is this needed only for internal temp, bat and vref ADCs?
  adcSTM32DisableTSVREFE();
  adcStop(&ADCDx);

  //FIXME: adcReleaseBus?

  return 0;
}


/*
 * Start a temperature measuring thread
 */
void sc_temp_thread_init(void)
{
  // Start a thread dedicated to user specific things
  chThdCreateStatic(temp_thread, sizeof(temp_thread), NORMALPRIO, tempThread, NULL);
}

void sc_adc_init(void)
{
  // Nothing here
}


void sc_adc_start_conversion(uint8_t channels, uint16_t interval_in_ms, uint8_t sample_time)
{
  // FIXME: the following hardcoded pins should be somehow in sc_conf.h
  // Control Board maps PC0, PC1, PC4, and PC5 to AN1-4
  //
  // PC0: ADC123_IN10
  // PC1: ADC123_IN11
  // PC4: ADC12_IN14
  // PC5: ADC12_IN15
  //
  //

  // Set global interval time in milliseconds
  interval_ms = interval_in_ms;

  convCfg.num_channels = channels;
  convCfg.sqr1 = ADC_SQR1_NUM_CH(channels);

  if (channels < 1 || channels > 4) {
    chDbgAssert(0, "Invalid amount of channels", "#1");
    return;
  }

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

  /* Start a thread dedicated to ADC conversion */
  last_conversion_time = chTimeNow();
  thread_run = TRUE;
  chThdCreateStatic(temp_thread, sizeof(temp_thread), NORMALPRIO, tempThread, NULL);
}


void sc_adc_stop_conversion(void)
{
  thread_run = FALSE;
}

uint16_t sc_adc_channel_get(uint8_t channel)
{
  return adc_latest[channel];
}

/* Emacs indentatation information
   Local Variables:
   indent-tabs-mode:nil
   tab-width:2
   c-basic-offset:2
   End:
*/
