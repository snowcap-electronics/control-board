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

#define SC_LOG_MODULE_TAG SC_LOG_MODULE_UNSPECIFIED

#define PLECO_GPIO_HEAD_LIGHTS     5
#define PLECO_GPIO_REAR_LIGHTS     6
#define PLECO_PWM_GLOW             10

static void stop_engines(void);
static void cb_handle_byte(SC_UART uart, uint8_t byte);
static void cb_adc_available(void);
static void cb_ping(void);
static void pleco_parse_glow(const uint8_t *param, uint8_t param_len);

static systime_t last_ping = 0;
static uint8_t glow_enable = 0;

int main(void)
{
  uint8_t loop_counter = 0;
  uint16_t glow[20] = {0,287,1114,2388,3960,5653,7270,8627,9568,9985,9830,9121,7939,6420,4738,3087,1654,606,62,0};

  halInit();
  /* Initialize ChibiOS core */
  chSysInit();

  // Init SC framework, with USB if not F1 Discovery
#if defined(BOARD_ST_STM32VL_DISCOVERY)
  sc_init(SC_MODULE_UART1 | SC_MODULE_PWM | SC_MODULE_ADC | SC_MODULE_GPIO | SC_MODULE_LED);
#else
  sc_init(SC_MODULE_UART1 | SC_MODULE_PWM | SC_MODULE_SDU | SC_MODULE_ADC | SC_MODULE_GPIO | SC_MODULE_LED);
  sc_log_output_uart(SC_UART_USB);
#endif

  sc_cmd_register("glow", SC_CMD_HELP("Enable glow (0/1)"), pleco_parse_glow);

  // Start event loop. This will start a new thread and return
  sc_event_loop_start();

  // Register callbacks.
  // These will be called from Event Loop thread. It's OK to do some
  // calculations etc. time consuming there, but not to sleep or block.
  sc_event_register_handle_byte(cb_handle_byte);
  sc_event_register_adc_available(cb_adc_available);
  sc_event_register_ping(cb_ping);

  // Start periodic ADC readings
#if defined(BOARD_ST_STM32VL_DISCOVERY)
  // FIXME: Not tested
  sc_adc_start_conversion(3, 1000, ADC_SAMPLE_55P5);
#else
  sc_adc_start_conversion(3, 1000, ADC_SAMPLE_56);
#endif

  // Loop forever waiting for callbacks
  while(1) {
    systime_t now;
    chThdSleepMilliseconds(100);

    if (loop_counter == 9 || loop_counter == 19) {
      SC_LOG_PRINTF("d: ping\r\n");
    }

    // Stop engines and start blinking lights for an error if no ping
    // from host for 500 ms
    now = ST2MS(chVTGetSystemTime());
    if (last_ping > 0 && now > last_ping && now - last_ping > 500) {
      stop_engines();
      if (loop_counter % 2 == 0) {
        sc_led_toggle();
        sc_gpio_toggle(PLECO_GPIO_HEAD_LIGHTS);
        sc_gpio_toggle(PLECO_GPIO_REAR_LIGHTS);
      }
    }

    if (glow_enable) {
      sc_pwm_set_duty(PLECO_PWM_GLOW, glow[loop_counter]);
    }

    if (++loop_counter == 20) {
      loop_counter = 0;
    }
  }

  return 0;
}



static void stop_engines(void)
{
#ifdef SC_PWM1_1_PIN
  sc_pwm_stop(1);
#endif
#ifdef SC_PWM1_2_PIN
  sc_pwm_stop(2);
#endif
#ifdef SC_PWM1_3_PIN
  sc_pwm_stop(3);
#endif
#ifdef SC_PWM1_4_PIN
  sc_pwm_stop(4);
#endif
#ifdef SC_PWM2_1_PIN
  sc_pwm_stop(5);
#endif
#ifdef SC_PWM2_2_PIN
  sc_pwm_stop(6);
#endif
#ifdef SC_PWM2_3_PIN
  sc_pwm_stop(7);
#endif
#ifdef SC_PWM2_4_PIN
  sc_pwm_stop(8);
#endif
}



static void cb_handle_byte(SC_UART UNUSED(uart), uint8_t byte)
{

  // Push received byte to generic command parsing
  // FIXME: per uart
  sc_cmd_push_byte(byte);
}



static void cb_adc_available(void)
{
  uint32_t timestamp_ms;
  uint16_t adc, adc_values[3], converted;
  uint8_t msg[16];
  int len;

  // Get ADC reading for the sonar pin
  // Vcc/512 per inch, 1 bit == 0.3175 cm
  sc_adc_channel_get(adc_values, &timestamp_ms);
  adc = adc_values[0];
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

  // FIXME: use SC_LOG_PRINTF
  sc_uart_send_msg(SC_UART_USB, msg, len);

  // Get ADC reading for the battery voltage
  // Max 51.8V, 1bit == 0.012646484375 volts
  adc = adc_values[1];
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

  // FIXME: use SC_LOG_PRINTF
  sc_uart_send_msg(SC_UART_USB, msg, len);

  // Get ADC reading for the current consumption in milliamps
  // Max 89.4A, 1bit == 0.021826171875 amps
  adc = adc_values[2];
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

  // FIXME: use SC_LOG_PRINTF
  sc_uart_send_msg(SC_UART_USB, msg, len);

}



static void cb_ping(void)
{
  last_ping = ST2MS(chVTGetSystemTime());
}

/*
 * Parse glow command
 */
static void pleco_parse_glow(const uint8_t *param, uint8_t param_len)
{
  if (param_len < 1) {
    // Invalid message
    return;
  }

  switch (param[0]) {
  case '0':
    glow_enable = 0;
    break;
  case '1':
    glow_enable = 1;
    break;
  default:
    // Invalid value, ignoring command
	return;
  }
}

/* Emacs indentatation information
   Local Variables:
   indent-tabs-mode:nil
   tab-width:2
   c-basic-offset:2
   End:
*/
