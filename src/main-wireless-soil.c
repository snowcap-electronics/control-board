/*
 * Spirit1 based wireless soil moisture sensor
 *
 * Copyright 2015-2017 Tuomas Kulve, <tuomas.kulve@snowcap.fi>
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

/*
 * PA8 == GPIO8 == PWM
 * PA7 == SPI1_MOSI == V_MEASURE2 == super caps
 * PA6 == SPI1_MISO == V_MEASURE1 == solar panel
 * PA5 == SPI1_SCK == moisture
 */

#define SC_LOG_MODULE_TAG SC_LOG_MODULE_UNSPECIFIED

#include "sc.h"

#ifndef SC_HAS_SPIRIT1
#error "This application supports only Spirit1"
#endif

#include "sc_spirit1.h"
#include "spirit1_key.h"
#include "chprintf.h"

#include <inttypes.h>

// Sleep 30min between every measurement
#define WIRELESS_SENSOR_SLEEP_SEC      1800
#define NO_SLEEP 0

typedef enum {
  LDR_PWR_SLEEP       = 0xf0f0,
  LDR_PWR_RESET       = 0x0f0f,
} LDR_PWR;


static void cb_handle_byte(SC_UART uart, uint8_t byte);
static void cb_spirit1_msg(void);
static void cb_spirit1_sent(void);
static void cb_spirit1_lost(void);
static void cb_spirit1_error(void);
static void cb_adc_available(void);
#if NO_SLEEP == 0
static void enable_shutdown(int sleep);
#endif

static bool standby = false, wkup = false;

// Disable unneeded PWMs
#undef SC_PWM1_2_PIN
#undef SC_PWM1_3_PIN
#undef SC_PWM1_4_PIN
#undef SC_PWM2_1_PIN
#undef SC_PWM2_2_PIN
#undef SC_PWM2_3_PIN
#undef SC_PWM2_4_PIN
#undef PWMDX2

int main(void)
{
  uint32_t *bsram = sc_pwr_get_backup_sram();

  standby = sc_pwr_get_standby_flag();
  wkup = sc_pwr_get_wake_up_flag();

  // Enable debugging even with WFI
  // This increases the power consumption by 1.5mA
  //sc_pwr_set_wfi_dbg();

  halInit();
  chSysInit();

  // Reset after WDG, goto sleep without WDG
  if (bsram[1] == LDR_PWR_RESET) {
    bsram[1] = LDR_PWR_SLEEP;
    #if NO_SLEEP == 0
    enable_shutdown(WIRELESS_SENSOR_SLEEP_SEC);
    #endif
  }

  bsram[1] = LDR_PWR_RESET;

  // Init WDG, 2 secs
  sc_wdg_init(256);

  // UART3 DMA stream conflicts with SPI2 RX DMA stream used by spirit1
  sc_init(SC_MODULE_SPI | SC_MODULE_ADC | SC_MODULE_PWM /*| SC_MODULE_UART3*/);
  //sc_log_output_uart(SC_UART_3);

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
  sc_event_register_spirit1_error(cb_spirit1_error);
  sc_event_register_adc_available(cb_adc_available);

  sc_spirit1_init(TOP_SECRET_KEY, MY_ADDRESS);
  SC_LOG_PRINTF("spirit1 init done\r\n");

  sc_pwm_set_freq(1, 1500000);

  sc_pwm_set_duty(1, 5000);

  // 500ms to give ADC time to start up and we'll ignore the other readings
  sc_adc_start_conversion(3, 500, ADC_SAMPLE_480);

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
  uint16_t adc_value[3];
  uint32_t timestamp_ms;
  uint8_t msg[64] = {'\0'};
  uint32_t *bsram;
  uint8_t len;

  bsram = sc_pwr_get_backup_sram();
  if (!standby) {
    bsram[0] = 0;
  } else {
    bsram[0] += 1;
  }

  sc_adc_channel_get(adc_value, &timestamp_ms);

  len = snprintf((char *)msg, sizeof(msg),
                 "soil[0]: %" PRIu32 ", %" PRIu16 ", %" PRIu16 ", %" PRIu16 "\r\n",
                 bsram[0], adc_value[0], adc_value[1], adc_value[2]);

  SC_LOG_PRINTF((char *)msg);
  
  sc_spirit1_send(SPIRIT1_BROADCAST_ADDRESS, msg, len);
}



static void cb_spirit1_msg(void)
{
  uint8_t msg[128];
  uint8_t len;
  uint8_t addr;
  uint8_t lqi;
  uint8_t rssi;

  len = sc_spirit1_read(&addr, msg, sizeof(msg));
  if (len > 1) {
    lqi = sc_spirit1_lqi();
    rssi = sc_spirit1_rssi();
    // Remove \r\n
    msg[len - 2] = '\0';
    SC_LOG_PRINTF("m: %s, %u, %u\r\n", msg, lqi, rssi);
  } else {
    SC_LOG_PRINTF("e: spirit1 read failed\r\n");
  }
}



/*
 * Message has been sent and received
 */
static void cb_spirit1_sent(void)
{
#if NO_SLEEP
  SC_LOG_PRINTF("d: spirit1 msg sent\r\n");

  chThdSleepMilliseconds(1000);
#else
  enable_shutdown(WIRELESS_SENSOR_SLEEP_SEC);
#endif
}



/*
 * Message has been sent but no ACK was received
 */
static void cb_spirit1_lost(void)
{
#if NO_SLEEP
  SC_LOG_PRINTF("d: spirit1 msg lost1\r\n");

  chThdSleepMilliseconds(1000);
#else
  enable_shutdown(WIRELESS_SENSOR_SLEEP_SEC);
#endif
}



/*
 * Error event received from spirit driver
 */
static void cb_spirit1_error(void)
{
#if NO_SLEEP
  SC_LOG_PRINTF("d: spirit1 error\r\n");

  chThdSleepMilliseconds(1000);
#else
  enable_shutdown(WIRELESS_SENSOR_SLEEP_SEC);
#endif

}


#if NO_SLEEP == 0
static void enable_shutdown(int sleep)
{

  // This function will not return
  sc_pwr_rtc_standby(sleep);
}
#endif
/* Emacs indentatation information
   Local Variables:
   indent-tabs-mode:nil
   tab-width:2
   c-basic-offset:2
   End:
*/
