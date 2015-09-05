/*
 * Power consumption test
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
#include "testplatform.h"

//#define POWER_USE_USB_LOGGING

static void cb_handle_byte(SC_UART uart, uint8_t byte);

int main(void)
{
  uint32_t subsystems = SC_MODULE_GPIO | SC_MODULE_LED;
  bool standby = sc_pwr_get_standby_flag();
  bool wkup = sc_pwr_get_wake_up_flag();
  uint32_t *bsram;
  // Enable debugging even with WFI
  sc_pwr_set_wfi_dbg();

  // F1 Discovery and L152 Nucleo boards dont't support USB
#if !defined(BOARD_ST_STM32VL_DISCOVERY) && !defined(BOARD_ST_NUCLEO_L152RE)
  subsystems |= SC_MODULE_SDU;
#endif

  halInit();
  chSysInit();

  sc_init(subsystems);
#if !defined(BOARD_ST_STM32VL_DISCOVERY) && !defined(BOARD_ST_NUCLEO_L152RE)
  sc_log_output_uart(SC_UART_USB);
#else
  sc_log_output_uart(SC_UART_1);
#endif

  // Start event loop. This will start a new thread and return
  sc_event_loop_start();

  // Register callbacks.
  // These will be called from Event Loop thread. It's OK to do some
  // calculations etc. time consuming there, but not to sleep or block
  // for longer periods of time.
  sc_event_register_handle_byte(cb_handle_byte);

  for (uint8_t i = 0; i < 20; ++i) {
    chThdSleepMilliseconds(100);
    sc_led_toggle();
  }
  sc_led_on();

  sc_pwr_enable_backup_sram();
  bsram = sc_pwr_get_backup_sram();
  if (!standby) {
    *bsram = 0;
  } else {
    *bsram += 1;
  }
  SC_LOG_PRINTF("standby: %d, wkup: %d, bsram: %d\r\n", standby, wkup, *bsram);

  chThdSleepMilliseconds(5000);

  // This function will not return
  sc_pwr_rtc_standby(10);
}


static void cb_handle_byte(SC_UART UNUSED(uart), uint8_t byte)
{

  // Push received byte to generic command parsing
  // FIXME: per uart
  sc_cmd_push_byte(byte);
}

/* Emacs indentatation information
   Local Variables:
   indent-tabs-mode:nil
   tab-width:2
   c-basic-offset:2
   End:
*/
