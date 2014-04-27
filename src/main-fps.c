/*
 * Light sensor based FPS counter
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

/*

The wiring for this test is as follows:

 3.3V - LDR----
              |
 A3 ----------+
              |
 GND - 10kR --

The relative size of the LDR and normal resistor calibrates the level A1 reads.

LDR ohms on my 22" IPS monitor:
- white xterm: 7k
- black xterm: 45k
*/

#include "sc.h"
#include "sc_led.h"

#define FPS_DERIVATIVE_MS     6
#define FPS_NEG_DIFF_LIMIT -300
#define FPS_POS_DIFF_LIMIT  300
#define FPS_STATE_INIT        0
#define FPS_STATE_BLACK       1
#define FPS_STATE_WHITE       2

// Debug: and adc reading only
#define FPS_SEND_ADC_ONLY     1

static void cb_handle_byte(SC_UART uart, uint8_t byte);
static void cb_adc_available(void);

static uint8_t handle_adc(uint16_t adc, uint32_t timestamp_ms, uint16_t *dxdt);
static uint8_t create_message(uint8_t *buf,
                              uint8_t max_len,
                              uint32_t timestamp_ms,
                              uint32_t interval,
                              uint16_t missed_adc,
                              uint16_t low_adc,
                              uint16_t high_adc,
                              uint16_t dxdt);
static uint8_t create_adc_msg(uint8_t *buf,
                              uint8_t max_len,
                              uint16_t adc,
                              uint32_t timestamp_ms);


int main(void)
{
  // Init SC framework, with USB if not F1 Discovery
#if defined(BOARD_ST_STM32VL_DISCOVERY)
  sc_init(SC_MODULE_UART1 | SC_MODULE_PWM | SC_MODULE_ADC | SC_MODULE_GPIO);
#else
  sc_init(SC_MODULE_UART1 | SC_MODULE_PWM | SC_MODULE_SDU | SC_MODULE_ADC | SC_MODULE_GPIO);
  sc_uart_default_usb(TRUE);
#endif

  // Start event loop. This will start a new thread and return
  sc_event_loop_start();

  // Register callbacks.
  // These will be called from Event Loop thread. It's OK to do some
  // calculations etc. time consuming there, but not to sleep or block.
  sc_event_register_handle_byte(cb_handle_byte);
  sc_event_register_adc_available(cb_adc_available);

  // Start periodic ADC readings
#if defined(BOARD_ST_STM32VL_DISCOVERY)
  // FIXME: Not tested
  sc_adc_start_conversion(1, 1, ADC_SAMPLE_7P5);
#else
  sc_adc_start_conversion(1, 1, ADC_SAMPLE_3);
#endif

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
  static uint8_t led_count = 0;
  static uint32_t timestamp_counter = 0;
  static uint32_t timestamp_last_frame = 0;
  static uint16_t missed_adc = 0;
  static uint16_t high_adc = 0;
  static uint16_t low_adc = 0xffff;

  uint16_t missed;
  uint16_t adc_value;
  uint16_t dxdt;
  uint32_t timestamp_ms;
  uint8_t send_adc_only = 0;

#if FPS_SEND_ADC_ONLY
  send_adc_only = 1;
#endif

  // Get ADC reading for light sensor
  sc_adc_channel_get(&adc_value, &timestamp_ms);

  // Check if we missed an ADC reading
  ++timestamp_counter;
  missed = timestamp_ms - timestamp_counter;
  timestamp_counter = timestamp_ms;

  missed_adc += missed;

  led_count += missed;
  if (led_count++ > 100) {
    led_count = 0;
    sc_led_toggle();
  }

  // Store lowest and highest values for debugging
  if (adc_value > high_adc) {
    high_adc = adc_value;
  }
  if (adc_value < low_adc) {
    low_adc = adc_value;
  }



  if (handle_adc(adc_value, timestamp_ms, &dxdt) || send_adc_only) {
    uint8_t buf[64];
    uint8_t len;

    if (send_adc_only) {
      len = create_adc_msg(buf, 64, adc_value, timestamp_ms);
    } else {
      len = create_message(buf, 64,
                           timestamp_ms,
                           timestamp_ms - timestamp_last_frame,
                           missed_adc,
                           low_adc,
                           high_adc,
                           dxdt);
    }
    sc_uart_send_msg(SC_UART_LAST, buf, len);
    high_adc = 0;
    low_adc = 0xffff;
    timestamp_last_frame = timestamp_ms;
    missed_adc = 0;
  }
}




/*
 * Takes brightness and timestamp as parameters and returns true for a new frame
 */
static uint8_t handle_adc(uint16_t adc,
                          uint32_t timestamp_ms,
                          uint16_t *dxdt)
{
  static uint8_t state = FPS_STATE_INIT;
  static uint16_t prev_adc[FPS_DERIVATIVE_MS] = { 0 };
  static uint8_t adc_i = 0;

  uint8_t current_adc;
  int8_t comp_adc;
  int16_t diff;

  // Current index
  current_adc = adc_i++;

  // Store a few latest FPS_DERIVATIVE_MS values
  prev_adc[current_adc] = adc;
  if (adc_i == FPS_DERIVATIVE_MS) {
    adc_i = 0;
  }

  // Gather initial statistics
  if (state == FPS_STATE_INIT) {
    if (timestamp_ms < FPS_DERIVATIVE_MS) {
      return 0;
    } else {
      // Guess black, doesn't need to be correct
      state = FPS_STATE_BLACK;
    }
  }

  // Compare against the adc value from FPS_DERIVATIVE_MS milliseconds
  // ago
  comp_adc = current_adc - (FPS_DERIVATIVE_MS - 1);
  if (comp_adc < 0) {
    comp_adc += FPS_DERIVATIVE_MS;
  }

  // Calculate derivative so that it's always positive
  diff = prev_adc[current_adc] - prev_adc[comp_adc];

  // We have a new frame, if the values are decreasing rapidly from a
  // white state
  if (state == FPS_STATE_WHITE && diff < FPS_NEG_DIFF_LIMIT) {
    state = FPS_STATE_BLACK;
    *dxdt = -1 * diff;
    return 1;
  }

  // We have a new frame, if the values are increasing rapidly from
  // black state
  if (state == FPS_STATE_BLACK && diff > FPS_POS_DIFF_LIMIT) {
    state = FPS_STATE_WHITE;
    *dxdt = diff;
    return 1;
  }

  return 0;
}



/*
 * Construct a message
 */
static uint8_t create_message(uint8_t *buf,
                              uint8_t max_len,
                              uint32_t timestamp_ms,
                              uint32_t interval,
                              uint16_t missed_adc,
                              uint16_t low_adc,
                              uint16_t high_adc,
                              uint16_t dxdt)
{
  uint8_t len = 0;
  len += sc_itoa(timestamp_ms, &buf[len], max_len - len);
  buf[len++] = ',';

  len += sc_itoa(interval, &buf[len], max_len - len);
  buf[len++] = ',';

  len += sc_itoa(missed_adc, &buf[len], max_len - len);
  buf[len++] = ',';

  len += sc_itoa(low_adc, &buf[len], max_len - len);
  buf[len++] = ',';

  len += sc_itoa(high_adc, &buf[len], max_len - len);
  buf[len++] = ',';

  len += sc_itoa(dxdt, &buf[len], max_len - len);

  buf[len++] = '\r';
  buf[len++] = '\n';

  return len;
}



/*
 * Construct a debug message with adc value only
 */
static uint8_t create_adc_msg(uint8_t *buf,
                              uint8_t max_len,
                              uint16_t adc,
                              uint32_t timestamp_ms)
{
  uint8_t len = 0;

  buf[len++] = 'a';
  buf[len++] = 'd';
  buf[len++] = 'c';
  buf[len++] = ':';
  buf[len++] = ' ';

  len += sc_itoa(timestamp_ms, &buf[len], max_len - len);
  buf[len++] = ',';

  len += sc_itoa(adc, &buf[len], max_len - len);
  buf[len++] = '\r';
  buf[len++] = '\n';

  return len;
}

/* Emacs indentatation information
   Local Variables:
   indent-tabs-mode:nil
   tab-width:2
   c-basic-offset:2
   End:
*/
