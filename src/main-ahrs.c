/*
 * AHRS project
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
#include "sc_9dof.h"
#include "sc_ahrs.h"

static void cb_handle_byte(SC_UART uart, uint8_t byte);
static void cb_9dof_available(void);
static void cb_ahrs_available(void);
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
  sc_event_register_9dof_available(cb_9dof_available);
  sc_event_register_ahrs_available(cb_ahrs_available);

#if defined(BOARD_ST_STM32F4_DISCOVERY)
  // Register user button on F4 discovery
  sc_extint_set_event(GPIOA, GPIOA_BUTTON, SC_EXTINT_EDGE_BOTH);
  sc_event_register_extint(GPIOA_BUTTON, cb_button_changed);
#endif

  sc_9dof_init(128);
  sc_ahrs_init();

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
  uint32_t subsystems = SC_INIT_UART1 | SC_INIT_PWM | SC_INIT_ADC | SC_INIT_GPIO | SC_INIT_LED;

  // F1 Discovery doesn't support USB
#if defined(BOARD_ST_STM32VL_DISCOVERY)
  use_usb = 0;
#endif

  if (use_usb) {
    subsystems |= SC_INIT_SDU;
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



static void cb_9dof_available(void)
{
  uint32_t ts;
  sc_float acc[3];
  sc_float magn[3];
  sc_float gyro[3];
#if 0
  uint8_t msg[128] = {'9', 'd', 'o', 'f', ':', ' ', '\0'};
  int len = 6;
  uint8_t i, s;
  int16_t *sensors[3] = {&acc[0], &gyro[0], &magn[0]};
  static uint8_t data_counter = 0;
#endif

  sc_9dof_get_data(&ts, acc, magn, gyro);
  sc_ahrs_push_9dof(ts, acc, magn, gyro);
#if 0
  // FIXME: printing is now broken as the values are floats
  // Let's print only every Nth data
  if (++data_counter == 10) {
    data_counter = 0;

    for (s = 0; s < 3; s++) {
      for (i = 0; i < 3; i++) {
        uint8_t l, m;
        l = sc_itoa(sensors[s][i], &msg[len], sizeof(msg) - len);
        len += l;

        // Move right to align to 6 character columns
        for (m = 0; m < 6; ++m) {
          if (m < l) {
            // Move
            msg[len - 1 + (6 - l) - m] = msg[len - 1 - m];
          } else {
            // Clear
            msg[len - 1 + (6 - l) - m] = ' ';
          }
        }

        len += (6 - l);
        msg[len++] = ',';
        msg[len++] = ' ';
      }
    }

    // Overwrite the last ", "
    msg[len - 2] = '\r';
    msg[len - 1] = '\n';
    msg[len++]   = '\0';
    sc_uart_send_msg(SC_UART_LAST, msg, len);
  }
#endif
}

static void cb_ahrs_available(void)
{
  uint32_t ts;
  uint8_t msg[128] = {'a', 'h', 'r', 's', ':', ' ', '\0'};
  sc_float roll, pitch, yaw;
  static uint8_t data_counter = 0;;
  int16_t roll_i, pitch_i, yaw_i;
  int16_t *euler[3] = {&roll_i, &pitch_i, &yaw_i};
  uint8_t i;
  int len = 6;

  sc_ahrs_get_orientation(&ts, &roll, &pitch, &yaw);

  roll_i = (int16_t)(roll + 0.5);
  pitch_i = (int16_t)(pitch + 0.5);
  yaw_i = (int16_t)(yaw + 0.5);

  // Let's print only every Nth data
  if (++data_counter == 1) {
    data_counter = 0;
    sc_led_toggle();

    for (i = 0; i < 3; i++) {
      uint8_t l, m;
      l = sc_itoa(*euler[i], &msg[len], sizeof(msg) - len);
      len += l;

      // Move right to align to 6 character columns
      for (m = 0; m < 6; ++m) {
        if (m < l) {
          // Move
          msg[len - 1 + (6 - l) - m] = msg[len - 1 - m];
        } else {
          // Clear
          msg[len - 1 + (6 - l) - m] = ' ';
        }
      }

      len += (6 - l);
      msg[len++] = ',';
      msg[len++] = ' ';
    }

    // Overwrite the last ", "
    msg[len - 2] = '\r';
    msg[len - 1] = '\n';
    msg[len++]   = '\0';
    sc_uart_send_msg(SC_UART_LAST, msg, len);
  }
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
