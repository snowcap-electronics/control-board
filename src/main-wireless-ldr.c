/*
 * Spirit1 based wireless LDR (alarm light) sensor
 *
 * Copyright 2015-2016      Tuomas Kulve, <tuomas.kulve@snowcap.fi>
 * Copyright 2011-2014 Kalle Vahlman, <kalle.vahlman@snowcap.fi>
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
#error "This application supports only Spirit1"
#endif

#include "sc_spirit1.h"
#include "spirit1_key.h"
#include "chprintf.h"

// Sleep 180 seconds between every measurement
#define WIRELESS_LDR_SLEEP_SEC      180

static void cb_handle_byte(SC_UART uart, uint8_t byte);
static void cb_spirit1_msg(void);
static void cb_spirit1_sent(void);
static void cb_spirit1_lost(void);
static void cb_adc_available(void);
static void enable_shutdown(void);

static bool standby = false, wkup = false;

int main(void)
{
  standby = sc_pwr_get_standby_flag();
  wkup = sc_pwr_get_wake_up_flag();

  // Enable debugging even with WFI
  // This increases the power consumption by 1.5mA
  //sc_pwr_set_wfi_dbg();

  halInit();
  chSysInit();

  sc_init(SC_MODULE_GPIO | /*SC_MODULE_LED |*/ SC_MODULE_SPI/* | SC_MODULE_SDU*/ | SC_MODULE_ADC);
  //sc_log_output_uart(SC_UART_USB);

  // Start event loop. This will start a new thread and return
  sc_event_loop_start();
  SC_LOG_PRINTF("started\r\n");

  // Register callbacks.
  // These will be called from Event Loop thread. It's OK to do some
  // calculations etc. time consuming there, but not to sleep or block
  // for longer periods of time.
  sc_event_register_handle_byte(cb_handle_byte);
  sc_event_register_spirit1_msg_available(cb_spirit1_msg);
  sc_event_register_spirit1_data_sent(cb_spirit1_sent);
  sc_event_register_spirit1_data_lost(cb_spirit1_lost);
  sc_event_register_adc_available(cb_adc_available);

  sc_spirit1_init(TOP_SECRET_KEY, MY_ADDRESS);
  SC_LOG_PRINTF("spirit1 init done\r\n");
  
  // Get ADC readings once
#if defined(BOARD_ST_STM32VL_DISCOVERY)
  // FIXME: Not tested
  sc_adc_start_conversion(2, 0, ADC_SAMPLE_7P5);
#else
  sc_adc_start_conversion(2, 0, ADC_SAMPLE_3);
#endif

  while(1) {
    //sc_led_toggle(); // Conflicts with Spirit's interrupt
    chThdSleepMilliseconds(1000);
  }
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
  uint8_t msg[32] = {'p', 'v', 'p', ':', ' ', '\n'};
  uint32_t *bsram;
  bsram = sc_pwr_get_backup_sram();
  if (!standby) {
    bsram[0] = 0;
  } else {
    bsram[0] += 1;
  }
  
  sc_adc_channel_get(adc_value, &timestamp_ms);

  /* ADC 2 is not used for anything in this particular HW setup */
  chsnprintf((char*)msg, sizeof(msg), "pvp: %d, %d, %d\r\n",
             adc_value[0], adc_value[1], bsram[0]);
	
  sc_spirit1_send(SPIRIT1_BROADCAST_ADDRESS, msg, sizeof(msg) - 1);
}



/*
 * Received a message over Spirit1
 */
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



/*
 * Message has been sent and received
 */
static void cb_spirit1_sent(void)
{
#if 0
  SC_LOG_PRINTF("d: spirit1 msg sent\r\n");

  chThdSleepMilliseconds(1000);
#endif

  enable_shutdown();
}



/*
 * Message has been sent but no ACK was received
 */
static void cb_spirit1_lost(void)
{
#if 0
  SC_LOG_PRINTF("d: spirit1 msg lost1\r\n");
  
  chThdSleepMilliseconds(1000);
#endif

  enable_shutdown();
}


static void enable_shutdown(void)
{
#if 0
  uint32_t *bsram;

  bsram = sc_pwr_get_backup_sram();
  if (!standby) {
    *bsram = 0;
  } else {
    *bsram += 1;
  }

  SC_LOG_PRINTF("standby: %d, wkup: %d, bsram: %d\r\n", standby, wkup, *bsram);
#endif
  //chThdSleepMilliseconds(5000);

  // This function will not return
  sc_pwr_rtc_standby(WIRELESS_LDR_SLEEP_SEC);
}

/* Emacs indentatation information
   Local Variables:
   indent-tabs-mode:nil
   tab-width:2
   c-basic-offset:2
   End:
*/
