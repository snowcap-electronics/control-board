/*
 * Light sensor based FPS counter
 *
 * Copyright 2011,2012 Kalle Vahlman, <kalle.vahlman@snowcap.fi>
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

#define SC_LOG_MODULE_TAG SC_LOG_MODULE_UNSPECIFIED

#include "sc.h"
#include "sc_led.h"

static void cb_handle_byte(SC_UART uart, uint8_t byte);
static void cb_adc_available(void);

int main(void)
{
  halInit();
  /* Initialize ChibiOS core */
  chSysInit();

  // Init SC framework, with USB if not F1 Discovery
#if defined(BOARD_ST_STM32VL_DISCOVERY)
  sc_init(SC_MODULE_UART1 | SC_MODULE_PWM | SC_MODULE_ADC | SC_MODULE_GPIO);
#else
  sc_init(SC_MODULE_UART1 | SC_MODULE_PWM | SC_MODULE_SDU | SC_MODULE_ADC | SC_MODULE_GPIO);
  sc_uart_default_usb(TRUE);
#endif

  sc_log_output_uart(SC_UART_USB);

  // Start event loop. This will start a new thread and return
  sc_event_loop_start();

  // Register callbacks.
  // These will be called from Event Loop thread. It's OK to do some
  // calculations etc. time consuming there, but not to sleep or block.
  sc_event_register_handle_byte(cb_handle_byte);
  sc_event_register_adc_available(cb_adc_available);

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

  SC_LOG_PRINTF("adc: %d, %d, %d\r\n", timestamp_ms, adc_value[0], adc_value[1]);
}




/* Emacs indentatation information
   Local Variables:
   indent-tabs-mode:nil
   tab-width:2
   c-basic-offset:2
   End:
*/
