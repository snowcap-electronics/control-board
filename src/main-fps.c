/*
 * Light sensor based FPS counter
 *
 * Copyright 2011,2015 Kalle Vahlman, <kalle.vahlman@snowcap.fi>
 *                     Tuomas Kulve, <tuomas.kulve@snowcap.fi>
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

/*

The wiring for this test is as follows:

 3.3V - LDR----
              |
 A3 ----------+
              |
 GND - 10kR --

The relative size of the LDR and normal resistor calibrates the level A1 reads.

LDR ohms on my 22" IPS monitor:
- white xterm: 7k
- black xterm: 45k
*/

/*
 * Include support for F4 discovery microphone to detect A/V sync:
 * https://www.youtube.com/watch?v=1MdDIS7Z0y8
 */


#define SC_LOG_MODULE_TAG SC_LOG_MODULE_UNSPECIFIED

#include "sc.h"
#include "sc_led.h"

static void cb_handle_byte(SC_UART uart, uint8_t byte);
static void cb_adc_available(void);
#if defined(BOARD_ST_STM32F4_DISCOVERY)
static void cb_audio_available(void);
static sc_filter_pdm_fir_state pdm_state;
#endif
static uint32_t loudness = -1;

int main(void)
{
  halInit();
  /* Initialize ChibiOS core */
  chSysInit();
  uint32_t subsystems =
    SC_MODULE_UART1 |
    SC_MODULE_PWM |
    SC_MODULE_ADC |
    SC_MODULE_GPIO |
    SC_MODULE_LED;

  // Init SC framework, with USB if not F1 Discovery
#if !defined(BOARD_ST_STM32VL_DISCOVERY)
  subsystems |= SC_MODULE_SDU;
#endif

#if defined(BOARD_ST_STM32F4_DISCOVERY)
  subsystems |= SC_MODULE_I2S;
  sc_filter_pdm_fir_init(&pdm_state);
#endif

  sc_init(subsystems);

#if !defined(BOARD_ST_STM32VL_DISCOVERY)
  sc_log_output_uart(SC_UART_USB);
#endif

  // Start event loop. This will start a new thread and return
  sc_event_loop_start();

  // Register callbacks.
  // These will be called from Event Loop thread. It's OK to do some
  // calculations etc. time consuming there, but not to sleep or block.
  sc_event_register_handle_byte(cb_handle_byte);
  sc_event_register_adc_available(cb_adc_available);
#if defined(BOARD_ST_STM32F4_DISCOVERY)
  sc_event_register_audio_available(cb_audio_available);
  // Start reading PDM from F4 microphone
  sc_i2s_start();
#endif

  // Start periodic ADC readings
#if defined(BOARD_ST_STM32VL_DISCOVERY)
  // FIXME: Not tested
  sc_adc_start_conversion(2, 1, ADC_SAMPLE_7P5);
#else
  sc_adc_start_conversion(2, 1, ADC_SAMPLE_3);
#endif

  // Loop forever waiting for callbacks
  while(1) {
    chThdSleepMilliseconds(1000);
  }

  return 0;
}



static void cb_handle_byte(SC_UART UNUSED(uart), uint8_t byte)
{

  // Push received byte to generic command parsing
  // FIXME: per uart
  sc_cmd_push_byte(byte);
}



static void cb_adc_available(void)
{
  uint16_t adc_value[2];
  uint32_t timestamp_ms;

  // Get ADC reading for light sensors
  sc_adc_channel_get(adc_value, &timestamp_ms);

  // Light values are read @ 1000Hz, audio detection runs @ 500Hz
  SC_LOG_PRINTF("adc: %d, %d, %d, %d\r\n",
                timestamp_ms, loudness, adc_value[0], adc_value[1]);
}


#if defined(BOARD_ST_STM32F4_DISCOVERY)
static void cb_audio_available(void)
{
  uint16_t *data;
  size_t len;
  size_t i;
  uint8_t pdm_i = 0;
  uint32_t sum = 0;

  data = sc_i2s_get_data(&len);

  for (i = 0; i < len; ++i) {
    sc_filter_pdm_fir_put(&pdm_state, data[i]);

    // len == 128 x 16bits every 2ms with 1MHz PDM
    // Convert after every 8th sample to PCM == 16x16 bits every 2ms == 8KHz
    if (++pdm_i == 8) {
      const uint8_t pcm_bits = 16;
      int pcm_data_tmp;

      pdm_i = 0;

      pcm_data_tmp = sc_filter_pdm_fir_get(&pdm_state, pcm_bits);
      if (pcm_data_tmp < 0) {
        sum -= pcm_data_tmp;
      } else {
        sum += pcm_data_tmp;
      }
    }
  }

  loudness = sum >> 4;
}
#endif

/* Emacs indentatation information
   Local Variables:
   indent-tabs-mode:nil
   tab-width:2
   c-basic-offset:2
   End:
*/
