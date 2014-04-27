/*
 * Snowcap Control Board v1 firmware
 *
 * Copyright 2011,2014 Kalle Vahlman, <kalle.vahlman@snowcap.fi>
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
static void init(void);

#if defined(BOARD_ST_STM32F4_DISCOVERY)
static void cb_button_changed(void);
#endif

int main(void)
{
  init();

  // Start event loop. This will start a new thread and return
  sc_event_loop_start();

  // Register callbacks.
  // These will be called from Event Loop thread. It's OK to do some
  // calculations etc. time consuming there, but not to sleep or block
  // for longer periods of time.
  sc_event_register_handle_byte(cb_handle_byte);

#if defined(BOARD_ST_STM32F4_DISCOVERY)
  // Register user button on F4 discovery
  sc_extint_set_event(GPIOA, GPIOA_BUTTON, SC_EXTINT_EDGE_BOTH);
  sc_event_register_extint(GPIOA_BUTTON, cb_button_changed);
#endif

  // Loop forever waiting for callbacks
  while(1) {
    uint8_t msg[] = {'d', ':', ' ', 'p','i','n','g','\r','\n'};
    chThdSleepMilliseconds(1000);
    sc_uart_send_msg(SC_UART_LAST, msg, 9);
  }

  return 0;
}

static void init(void)
{
  uint8_t use_usb = 1;
  uint32_t subsystems = SC_MODULE_UART1 | SC_MODULE_PWM | SC_MODULE_ADC | SC_MODULE_GPIO | SC_MODULE_LED;

  // F1 Discovery doesn't support USB
#if defined(BOARD_ST_STM32VL_DISCOVERY)
  use_usb = 0;
#endif

  if (use_usb) {
    subsystems |= SC_MODULE_SDU;
  }

  sc_init(subsystems);

  if (use_usb) {
    sc_uart_default_usb(TRUE);
  }
}


static void cb_handle_byte(SC_UART UNUSED(uart), uint8_t byte)
{

  // Push received byte to generic command parsing
  // FIXME: per uart
  sc_cmd_push_byte(byte);
}



#if defined(BOARD_ST_STM32F4_DISCOVERY)
static void cb_button_changed(void)
{
  uint8_t msg[] = "d: button state: X\r\n";
  uint8_t button_state;

  button_state = palReadPad(GPIOA, GPIOA_BUTTON);

  msg[17] = button_state + '0';

  sc_uart_send_msg(SC_UART_LAST, msg, sizeof(msg));
}
#endif


/* Emacs indentatation information
   Local Variables:
   indent-tabs-mode:nil
   tab-width:2
   c-basic-offset:2
   End:
*/
