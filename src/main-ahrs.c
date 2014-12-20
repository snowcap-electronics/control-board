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
#define SC_LOG_MODULE_TAG SC_LOG_MODULE_UNSPECIFIED

#include "sc.h"
#include "sc_9dof.h"
#include "sc_ahrs.h"
#include "sc_filter.h"

#define AHRS_USB_SDU   1
#define AHRS_USB_HID   0

static void cb_handle_byte(SC_UART uart, uint8_t byte);
static void cb_9dof_available(void);
static void cb_ahrs_available(void);
static void init(void);
#if AHRS_USB_HID
static void update_hid(int8_t x, int8_t y);
#endif

#if defined(BOARD_ST_STM32F4_DISCOVERY)
static void cb_button_changed(void);
#endif


// Filtering
static sc_filter_brown_linear_expo_state filter_state_smooth[9];
static sc_filter_brown_linear_expo_state roll_smooth;
static sc_filter_brown_linear_expo_state pitch_smooth;
static sc_filter_brown_linear_expo_state yaw_smooth;
static sc_filter_zero_calibrate_state filter_state_calibrate[3];

// HID data
static hid_data gamepad_data;

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

  // Let's wait a second before expecting 9dof sensor to be ready
  chThdSleepMilliseconds(100);

  sc_9dof_init();
  sc_ahrs_init(0.2);

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
  uint32_t subsystems = SC_MODULE_UART1 | SC_MODULE_PWM | SC_MODULE_ADC | SC_MODULE_GPIO | SC_MODULE_LED;
  uint8_t i;

  // F1 Discovery doesn't support USB
#if defined(BOARD_ST_STM32VL_DISCOVERY)
  use_usb = 0;
#endif

  if (use_usb) {
#if AHRS_USB_SDU
    subsystems |= SC_MODULE_SDU;
#endif
#if AHRS_USB_HID
    subsystems |= SC_MODULE_HID;
#endif
  }

  halInit();
  /* Initialize ChibiOS core */
  chSysInit();

  sc_init(subsystems);

#if AHRS_USB_SDU
  if (use_usb) {
    sc_uart_default_usb(TRUE);
    sc_log_output_uart(SC_UART_USB);
  }
#endif

  // Init smoothing filter
  for (i = 0; i < sizeof(filter_state_smooth); ++i) {
    const sc_float factor = 0.2;
    sc_filter_brown_linear_expo_init(&filter_state_smooth[i], factor);
  }

  sc_filter_brown_linear_expo_init(&roll_smooth, 0.2);
  sc_filter_brown_linear_expo_init(&pitch_smooth, 0.2);
  sc_filter_brown_linear_expo_init(&yaw_smooth, 0.2);

  // Init gyroscope calibration filter
  for (i = 0; i < sizeof(filter_state_calibrate); ++i) {
    const uint8_t samples = 64;
    sc_filter_zero_calibrate_init(&filter_state_calibrate[i], samples);
  }

  // Set button state to released
  gamepad_data.button = 0;
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
  sc_float *sensors[3] = {acc, magn, gyro};
  static uint8_t warmup = 64;

  sc_9dof_get_data(&ts, acc, magn, gyro);

  // FIXME: for some reason the initial values might contain
  // garbage. Ignore them. Why aren't they valid..?
  if (warmup) {
    --warmup;
    return;
  }

  // Apply smoothing filter
  if (1) {
    uint8_t a, b;
    for (a = 0; a < 3; ++a) {
      for (b = 0; b < 3; ++b) {
        // Test: only for gyro
        if (a != 2)
          continue;
        sensors[a][b] = sc_filter_brown_linear_expo(&filter_state_smooth[a*3 + b], sensors[a][b]);
      }
    }
  }

  // Apply gyro zero level calibration
  if (1) {
    uint8_t a;
    for (a = 0; a < 3; ++a) {
        gyro[a] = sc_filter_zero_calibrate(&filter_state_calibrate[a], gyro[a]);
    }
  }

  // Let's not do AHRS on F4 disco's accelerometer only. HID only.
#if defined(BOARD_ST_STM32F4_DISCOVERY)
#if AHRS_USB_HID
  {
    // Scale +-1g to +-127
    sc_float x = acc[0];
    sc_float y = acc[1];
    x -= 0.5;
    if (x > 1) x = 1;
    if (x < -1) x = -1;
    y -= 0.5;
    if (y > 1) y = 1;
    if (y < -1) y = -1;

    x *= 127;
    y *= 127;
    update_hid((int8_t)x, (int8_t)y);
  }
#endif
#else
  sc_ahrs_push_9dof(ts, acc, magn, gyro);
#endif

#if 1
  {
    static uint8_t data_counter = 0;

    // Let's print only every Nth data
    if (++data_counter == 1) {
      data_counter = 0;
      sc_led_toggle();

      SC_LOG_PRINTF("9dof: %f, %f, %f, %f, %f, %f, %f, %f, %f\r\n",
                    acc[0], acc[1], acc[2], magn[0], magn[1], magn[2], gyro[0], gyro[1], gyro[2]);
    }
  }
#endif
}

static void cb_ahrs_available(void)
{
  uint32_t ts;
#if AHRS_USB_SDU
  static uint32_t old_ts = 0;
#endif
  sc_float roll, pitch, yaw;

  sc_ahrs_get_orientation(&ts, &roll, &pitch, &yaw);

  // Apply smoothing filter to final result
  // FIXME: broken but why?
  if (0) {
    roll  = sc_filter_brown_linear_expo(&roll_smooth,  roll);
    pitch = sc_filter_brown_linear_expo(&pitch_smooth, pitch);
    yaw   = sc_filter_brown_linear_expo(&yaw_smooth,   yaw);
  }

#if AHRS_USB_HID
  {
    static sc_float x_init;
    static sc_float y_init;
    static uint8_t init_done = 0;
    sc_float x = yaw - 180;
    sc_float y = pitch;

    if (!init_done) {
      x_init = x;
      y_init = y;
      init_done = 1;
    }

    // Shift center to initial position
    x -= x_init;
    if (x < -180) x += 360;

    y -= y_init;
    if (y < -180) y += 360;

    // Scale +-45 degrees to +-127
    if (x > 45) x = 45;
    if (x < -45) x = -45;
    x *= (127/45.0);

    if (y > 45) y = 45;
    if (y < -45) y = -45;
    y *= (127/45.0);
    // Invert y
    y *= -1;

    update_hid((int8_t)x, (int8_t)y);
  }
#endif

#if AHRS_USB_SDU
#if 1
  {
    static uint8_t data_counter = 0;

    // Let's print only every Nth data
    if (++data_counter == 1) {
      data_counter = 0;
      sc_led_toggle();

      SC_LOG_PRINTF("ahrs: %d, %f, %f, %f\r\n", ts - old_ts, roll, pitch, yaw);
    }
  }
  old_ts = ts;
#endif
#endif
}


#if defined(BOARD_ST_STM32F4_DISCOVERY)
static void cb_button_changed(void)
{
  uint8_t msg[] = "d: button state: X\r\n";
  uint8_t button_state;

  button_state = palReadPad(GPIOA, GPIOA_BUTTON);
#if AHRS_USB_HID
#if SC_HID_USE_BUTTON
  gamepad_data.button = !button_state;
  hid_transmit(&gamepad_data);
#endif
#endif

#if AHRS_USB_SDU
  msg[17] = button_state + '0';

  sc_uart_send_msg(SC_UART_LAST, msg, sizeof(msg));
#endif
}
#endif


#if AHRS_USB_HID
static void update_hid(int8_t x, int8_t y)
{
  gamepad_data.x = x;
  gamepad_data.y = y;

  hid_transmit(&gamepad_data);
}
#endif

/* Emacs indentatation information
   Local Variables:
   indent-tabs-mode:nil
   tab-width:2
   c-basic-offset:2
   End:
*/
