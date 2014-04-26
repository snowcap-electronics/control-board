/*
 * RuuviTracker C2
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
#include "nmea/nmea.h"
#include "drivers/gsm.h"

#include "chprintf.h"

static void cb_handle_byte(SC_UART uart, uint8_t byte);
static void gps_power_on(void);
//static void gps_power_off(void);

#ifndef BOARD_RUUVITRACKERC2
#error "Tracker application supports only RuuviTracker HW."
#endif

#ifndef SC_ALLOW_GPL
#error "Tracker application relies on GPL TinyNMEA"
#endif

#ifndef SC_USE_NMEA
#error "Tracker application relies on GPL TinyNMEA"
#endif

static nmeaINFO info;
static nmeaPARSER parser;

int main(void)
{
  uint8_t apn[] = "prepaid.dna.fi";
  systime_t wait_limit;
  uint8_t done = 0;

  // Set UART config for GSM
  sc_uart_set_config(SC_UART_3, 115200, 0, 0, 0);

  sc_init(SC_INIT_UART1 | SC_INIT_UART2 | SC_INIT_UART3 | SC_INIT_GPIO | SC_INIT_LED | SC_INIT_SDU);
  sc_uart_default_usb(TRUE);

  sc_log_output_uart(SC_UART_USB);

  // Start event loop. This will start a new thread and return
  sc_event_loop_start();

  // Register callbacks.
  // These will be called from Event Loop thread. It's OK to do some
  // calculations etc. time consuming there, but not to sleep or block
  // for longer periods of time.
  sc_event_register_handle_byte(cb_handle_byte);

  nmea_zero_INFO(&info);
  nmea_parser_init(&parser);

  gps_power_on();

  // DEBUG: Wait a while so that USB is ready to print debugs
  chThdSleepMilliseconds(2000);

  gsm_start();
  gsm_set_apn(apn);

  wait_limit = chTimeNow() + 20000;

  // Loop forever waiting for callbacks
  while(1) {
    uint8_t msg[] = {'d', ':', ' ', 'p','i','n','g','\r','\n'};
    chThdSleepMilliseconds(1000);
    sc_uart_send_msg(SC_UART_USB, msg, 9);
    sc_led_toggle();
    if (!done) {
      SC_LOG_PRINTF("%d: waiting for %d\r\n", chTimeNow(), wait_limit);
    }
    if (!done && chTimeNow() > wait_limit) {
      uint8_t cpin[] = "AT+CPIN?\r\n";
      done = 1;
      SC_LOG_PRINTF("%d: gsm_cmd: %s", chTimeNow(), cpin);
      gsm_cmd(cpin, sizeof(cpin));
    }
  }

  return 0;
}

static void cb_handle_byte(SC_UART uart, uint8_t byte)
{
  // NMEA from GPS
  if (uart == SC_UART_2) {
    int n;
    n = nmea_parse(&parser, (char *)&byte, 1, &info);
    if (n) {
      int len;
      uint8_t msg[64];

      len = chsnprintf((char*)msg, sizeof(msg), "smask: 0x%x, fix: %d, sig: %d, lon: %d, lat: %d\r\n",
                       info.smask, info.fix, info.sig, (int)(info.lon * 1000), (int)(info.lat * 1000));
      //sc_uart_send_msg(SC_UART_USB, msg, len);
      (void)len;
      nmea_zero_INFO(&info);
    }
    return;
  }

  // GSM
  if (uart == SC_UART_3) {
    gsm_state_parser(byte);
    return;
  }

  // Push received byte to generic command parsing
  sc_cmd_push_byte(byte);
}


static void gps_power_on(void)
{
  palSetPad(GPIOC, GPIOC_ENABLE_LDO2);
  palSetPad(GPIOC, GPIOC_ENABLE_LDO3);
}
/*
static void gps_power_off(void)
{
  palClearPad(GPIOC, GPIOC_ENABLE_LDO3);
  palClearPad(GPIOC, GPIOC_ENABLE_LDO2);
}
*/


/* Emacs indentatation information
   Local Variables:
   indent-tabs-mode:nil
   tab-width:2
   c-basic-offset:2
   End:
*/
