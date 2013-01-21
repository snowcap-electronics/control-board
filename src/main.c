/*
 * Snowcap Control Board v1 firmware
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

#include "sc.h"

static void cb_handle_byte(SC_UART uart, uint8_t byte);
static void cb_adc_available(void);

int main(void)
{
  // Init SC framework
  // FIXME: define what subsystems to initialize?
  sc_init();

  // Start event loop. This will start a new thread and return
  sc_event_loop_start();

  // Register callbacks.
  // These will be called from Event Loop thread. It's OK to do some
  // calculations etc. time consuming there, but not to sleep or block.
  sc_event_register_handle_byte(cb_handle_byte);
  sc_event_register_adc_available(cb_adc_available);

  // Start periodic ADC readings
  sc_adc_start_conversion(3);

  // Loop forever waiting for callbacks
  while(1) {
    uint8_t msg[] = {'d', ':', ' ', 'p','i','n','g','\r','\n'};
    chThdSleepMilliseconds(1000);
    sc_uart_send_msg(SC_UART_LAST, msg, 9);
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
  uint16_t adc, converted;
  uint8_t msg[16];
  int len;

  // Get ADC reading for the sonar pin
  // Vcc/512 per inch, 1 bit == 0.3175 cm
  adc = sc_adc_channel_get(0);
  converted = (uint16_t)(adc * 0.3175);

  len = 0;
  msg[len++] = 'd';
  msg[len++] = 's';
  msg[len++] = 't';
  msg[len++] = ':';
  msg[len++] = ' ';

  len += sc_itoa(converted, msg + len, 16 - (len + 2));

  msg[len++] = '\r';
  msg[len++] = '\n';

  sc_uart_send_msg(SC_UART_LAST, msg, len);

  // Get ADC reading for the battery voltage
  // Max 51.8V, 1bit == 0.012646484375 volts
  adc = sc_adc_channel_get(1);
  converted = (uint16_t)(adc * 12.646484375);

  len = 0;
  msg[len++] = 'v';
  msg[len++] = 'l';
  msg[len++] = 't';
  msg[len++] = ':';
  msg[len++] = ' ';

  len += sc_itoa(converted, msg + len, 16 - (len + 2));

  msg[len++] = '\r';
  msg[len++] = '\n';

  sc_uart_send_msg(SC_UART_LAST, msg, len);

  // Get ADC reading for the current consumption in milliamps
  // Max 89.4A, 1bit == 0.021826171875 amps
  adc = sc_adc_channel_get(2);
  converted = (uint16_t)(adc * 21.826171875);

  len = 0;
  msg[len++] = 'a';
  msg[len++] = 'm';
  msg[len++] = 'p';
  msg[len++] = ':';
  msg[len++] = ' ';

  len += sc_itoa(converted, msg + len, 16 - (len + 2));

  msg[len++] = '\r';
  msg[len++] = '\n';

  sc_uart_send_msg(SC_UART_LAST, msg, len);

}


/* Emacs indentatation information
   Local Variables:
   indent-tabs-mode:nil
   tab-width:2
   c-basic-offset:2
   End:
*/
