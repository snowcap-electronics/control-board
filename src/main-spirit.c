/*
 * SPIRIT1 receiver app
 *
 * Copyright 2011-2017 Kalle Vahlman, <kalle.vahlman@snowcap.fi>
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

#ifndef SC_HAS_SPIRIT1
#error "This app requires SPIRIT1"
#endif

#if !defined(BOARD_ST_NUCLEO_L152RE)
#error "Only the following boards are supported: Nucleo L152 and Snowcap STM32F4_v1"
#endif

#include "sc.h"
#include "sc_spirit1.h"
#include "spirit1_key.h"

static void cb_handle_byte(SC_UART uart, uint8_t byte);
static void cb_spirit1_msg(void);
static void cb_spirit1_sent(void);
static void cb_spirit1_lost(void);
static void init(void);

systime_t last_radio_msg;

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

  sc_event_register_spirit1_msg_available(cb_spirit1_msg);
  sc_event_register_spirit1_data_sent(cb_spirit1_sent);
  sc_event_register_spirit1_data_lost(cb_spirit1_lost);
  sc_spirit1_init(TOP_SECRET_KEY, MY_ADDRESS);

  chThdSleepMilliseconds(1000);
  // Loop forever waiting for callbacks
  while(1) {
    systime_t now = chVTGetSystemTime();

    if (0) {
      uint8_t msg[] = {'t', 'e', 's', 't', '\r', '\n', '\0'};
      SC_LOG_PRINTF("sending: test\r\n");
      sc_spirit1_send(SPIRIT1_BROADCAST_ADDRESS, msg, sizeof(msg) - 1);
      chThdSleepMilliseconds(1000);
    } else {
      // FIXME: Ugly WAR: reset if nothing heard from radio in 5 minutes
      if (ST2MS(now - last_radio_msg) > 5*60*1000) {
        sc_wdg_init(1);
      }
      SC_LOG_PRINTF("d: ping (%d)\r\n", ST2MS(now));
    }
    chThdSleepMilliseconds(1000);
  }

  return 0;
}

static void init(void)
{
  uint32_t subsystems = SC_MODULE_UART2 | SC_MODULE_SPI;


  // F1 Discovery and L152 Nucleo boards dont't support USB
  #if defined(BOARD_SNOWCAP_STM32F4_V1)
  subsystems |= SC_MODULE_SDU;
  #else
  subsystems |= SC_MODULE_UART2;
  #endif

  // Init ChibiOS
  halInit();
  chSysInit();

  sc_init(subsystems);

  #if defined(BOARD_ST_NUCLEO_L152RE)
  sc_log_output_uart(SC_UART_2);
  #else
  sc_log_output_uart(SC_UART_USB);
  #endif
}



static void cb_handle_byte(SC_UART uart, uint8_t byte)
{
  sc_cmd_push_byte(byte);
  (void)uart;
}



static void cb_spirit1_msg(void)
{
  uint8_t msg[32];
  uint8_t len;
  uint8_t addr;
  uint8_t lqi;
  uint8_t rssi;

  last_radio_msg = chVTGetSystemTime();

  len = sc_spirit1_read(&addr, msg, sizeof(msg));
  if (len) {
    lqi = sc_spirit1_lqi();
    rssi = sc_spirit1_rssi();
    SC_LOG_PRINTF("GOT MSG (LQI: %u, RSSI: %u): %s", lqi, rssi, msg);
  } else {
    SC_LOG_PRINTF("SPIRIT1 READ FAILED");
  }
}



static void cb_spirit1_sent(void)
{
  SC_LOG_PRINTF("d: spirit1 msg sent\r\n");
}



static void cb_spirit1_lost(void)
{
  SC_LOG_PRINTF("d: spirit1 msg lost\r\n");
}


/* Emacs indentatation information
   Local Variables:
   indent-tabs-mode:nil
   tab-width:2
   c-basic-offset:2
   End:
*/
