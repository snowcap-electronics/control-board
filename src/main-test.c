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
#define SC_LOG_MODULE_TAG SC_LOG_MODULE_UNSPECIFIED

#include "sc.h"
#ifdef SC_HAS_SPIRIT1
#include "sc_spirit1.h"
// This is a local file containing local secrets
#include "spirit1_key.h"
// Something like this:
/*
#define TOP_SECRET_KEY          ((uint8_t*)"_TOP_SECRET_KEY_")
#define MY_ADDRESS              0x34
*/
#endif

static void cb_handle_byte(SC_UART uart, uint8_t byte);
#ifdef SC_HAS_SPIRIT1
static void cb_spirit1_msg(void);
#endif
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

#ifdef SC_HAS_SPIRIT1
  sc_event_register_spirit1_available(cb_spirit1_msg);
  sc_spirit1_init(TOP_SECRET_KEY, MY_ADDRESS);
#endif

  chThdSleepMilliseconds(1000);
  // Loop forever waiting for callbacks
  while(1) {
#ifndef SC_HAS_SPIRIT1
    SC_LOG_PRINTF("d: ping\r\n");
    chThdSleepMilliseconds(1000);
#else
    {
      uint8_t msg[] = {'t', 'e', 's', 't', '\r', '\n', '\0'};
      //SC_LOG_PRINTF("sending: test\r\n");
      sc_spirit1_send(SPIRIT1_BROADCAST_ADDRESS, msg, sizeof(msg) - 1);
      chThdSleepMilliseconds(10000);
    }
#endif
  }

  return 0;
}

static void init(void)
{
  uint32_t subsystems = SC_MODULE_UART2 | SC_MODULE_GPIO | SC_MODULE_LED | SC_MODULE_SPI;

  // UART1, PWM nor ADC pins are defined for L152 Nucleo board
#if !defined(BOARD_ST_NUCLEO_L152RE)
  subsystems |= SC_MODULE_UART1 | SC_MODULE_PWM | SC_MODULE_ADC;
#endif

  // F1 Discovery and L152 Nucleo boards dont't support USB
#if !defined(BOARD_ST_STM32VL_DISCOVERY) && !defined(BOARD_ST_NUCLEO_L152RE)
  subsystems |= SC_MODULE_SDU;
#endif

  // Init ChibiOS
  halInit();
  chSysInit();

  sc_init(subsystems);

#if defined(BOARD_ST_NUCLEO_L152RE)
  sc_log_output_uart(SC_UART_2);
#else
# if !defined(BOARD_ST_STM32VL_DISCOVERY)
  sc_uart_default_usb(TRUE);
  sc_log_output_uart(SC_UART_USB);
#else
  sc_log_output_uart(SC_UART_1);
#endif
#endif
}


static void cb_handle_byte(SC_UART uart, uint8_t byte)
{
  // F1 Discovery doesn't support USB (Nucleo has UART1 routed through ST Link's USB)
#if defined(BOARD_ST_STM32VL_DISCOVERY) || defined(BOARD_ST_NUCLEO_L152RE)
  sc_cmd_push_byte(byte);
  (void)uart;
#else
  if (uart != SC_UART_USB) {
    // Log bytes coming from real UARTs to USB
    SC_LOG_PRINTF("%c", byte);
  } else {
    // Push bytes received from USB to command parsing
    sc_cmd_push_byte(byte);
  }
#endif
}



#ifdef SC_HAS_SPIRIT1
static void cb_spirit1_msg(void)
{
  uint8_t msg[32];
  uint8_t len;
  uint8_t addr;
  uint8_t lqi;
  uint8_t rssi;

  len = sc_spirit1_read(&addr, msg, sizeof(msg));
  if (len) {
    lqi = sc_spirit1_lqi();
    rssi = sc_spirit1_rssi();
    SC_LOG_PRINTF("GOT MSG (LQI: %u, RSSI: %u): %s", lqi, rssi, msg);
  } else {
    SC_LOG_PRINTF("SPIRIT1 READ FAILED");
  }
}
#endif



#if defined(BOARD_ST_STM32F4_DISCOVERY)
static void cb_button_changed(void)
{
  uint8_t button_state;

  button_state = palReadPad(GPIOA, GPIOA_BUTTON);

  SC_LOG_PRINTF("d: button state: %d\r\n", button_state);
}
#endif


/* Emacs indentatation information
   Local Variables:
   indent-tabs-mode:nil
   tab-width:2
   c-basic-offset:2
   End:
*/
