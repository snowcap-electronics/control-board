/*
 * Snowcap firmware for Base Boards with Radio Boards.
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
#include "sc_radio.h"

static void cb_handle_byte(SC_UART uart, uint8_t byte);
static void cb_blob_available(void);
static void cb_pa0_changed(void);

int main(void)
{
  halInit();
  /* Initialize ChibiOS core */
  chSysInit();

  sc_init(SC_MODULE_UART2 | SC_MODULE_GPIO | SC_MODULE_LED | SC_MODULE_SDU | SC_MODULE_RADIO);
  sc_uart_default_usb(TRUE);

  sc_log_output_uart(SC_UART_USB);

  // Start event loop. This will start a new thread and return
  sc_event_loop_start();

  // Register callbacks.
  // These will be called from Event Loop thread. It's OK to do some
  // calculations etc. time consuming there, but not to sleep or block
  // for longer periods of time.
  sc_event_register_handle_byte(cb_handle_byte);
  sc_event_register_blob_available(cb_blob_available);
  sc_event_register_extint(0, cb_pa0_changed);
  sc_extint_set_event(GPIOA, 0, SC_EXTINT_EDGE_BOTH);

  // Loop forever waiting for callbacks
  while(1) {
    uint8_t msg[] = {'d', ':', ' ', 'p','i','n','g','\r','\n'};
    chThdSleepMilliseconds(1000);
    sc_uart_send_msg(SC_UART_USB, msg, sizeof(msg));
  }

  return 0;
}


static void cb_handle_byte(SC_UART uart, uint8_t byte)
{
  switch (uart) {
  case SC_UART_2:
    // Separate handling for all data coming from radio board
    sc_radio_process_byte(byte);
    break;
  case SC_UART_USB:
    // Push received byte to generic command parsing
    sc_cmd_push_byte(byte);
    break;
  default:
    chDbgAssert(0, "Unsupported UART");
  }
}


static void cb_blob_available(void)
{
  uint8_t *blob;
  uint16_t blob_len;

  blob_len = sc_cmd_blob_get(&blob);

  if (blob_len) {
		uint8_t msg[] = "Blob received bytes:            \r\n";
		uint8_t len = 21;

		len += sc_itoa(blob_len, &msg[len], 6);
		msg[len++] = '\r';
		msg[len++] = '\n';
    sc_uart_send_msg(SC_UART_USB, msg, len);
  }
}


static void cb_pa0_changed(void)
{
  uint8_t msg[] = "pa0: state: X\r\n";
  uint8_t button_state;

  button_state = palReadPad(GPIOA, 0);

  msg[12] = button_state + '0';

  sc_uart_send_msg(SC_UART_USB, msg, sizeof(msg));
}
/* Emacs indentatation information
   Local Variables:
   indent-tabs-mode:nil
   tab-width:2
   c-basic-offset:2
   End:
*/
