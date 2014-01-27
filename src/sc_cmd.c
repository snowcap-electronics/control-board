/*
 * Command parsing
 *
 * Copyright 2011 Tuomas Kulve, <tuomas.kulve@snowcap.fi>
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

#if HAL_USE_PWM
static void parse_command_pwm(uint8_t *cmd, uint8_t cmd_len);
static void parse_command_pwm_frequency(uint8_t *cmd, uint8_t cmd_len);
static void parse_command_pwm_duty(uint8_t *cmd, uint8_t cmd_len);
#endif
#if HAL_USE_PAL
static void parse_command_led(uint8_t *cmd, uint8_t cmd_len);
static void parse_command_gpio(uint8_t *cmd, uint8_t cmd_len);
static void parse_command_gpio_all(uint8_t *cmd, uint8_t cmd_len);
#endif
static void parse_command_blob(uint8_t *cmd, uint8_t cmd_len);
static void parse_command_radio(uint8_t *cmd, uint8_t cmd_len);
static void parse_command(void);

/*
 * Buffer for incoming commands.
 * FIXME: Separate handling for SPI/UART1-3?
 */
#ifndef SC_CMD_MAX_RECV_BUF_LEN
#define SC_CMD_MAX_RECV_BUF_LEN      16
#endif
#define MIN_CMD_LEN           3        // 8 bit command, >=8 bit value, \n
static uint8_t receive_buffer[SC_CMD_MAX_RECV_BUF_LEN];
static uint8_t recv_i = 0;

/*
 * Blob transfers
 */
#ifndef SC_BLOB_MAX_SIZE
#define SC_BLOB_MAX_SIZE      256
#endif
static uint8_t blob_buf[SC_BLOB_MAX_SIZE];
static uint16_t blob_len = 0;
static uint16_t blob_i = 0;
static Mutex blob_mtx;


/*
 * Initialize command module
 */
void sc_cmd_init(void)
{
  chMtxInit(&blob_mtx);
}




/*
 * Receive byte. This can be called only from the main thread.
 * FIXME: per uart buffers. Now we happily mix all uarts together.
 */
void sc_cmd_push_byte(uint8_t byte)
{

  // If in a middle of blob transfer, store raw data without interpretation
  chMtxLock(&blob_mtx);
  if (blob_i < blob_len) {

    // FIXME: HACK: Assume \n is from the previous command and not
    // part of the blob
    if (blob_i == 0 && byte == '\n') {
      return;
    }

    blob_buf[blob_i++] = byte;

    if (blob_i % 1024 == 0) {
      SC_DBG("1K received\r\n");
    }

    if (blob_i == blob_len) {
      msg_t drdy;
      // Create and send blob ready notification
      drdy = sc_event_msg_create_type(SC_EVENT_TYPE_BLOB_AVAILABLE);
      sc_event_msg_post(drdy, SC_EVENT_MSG_POST_FROM_NORMAL);
    }
  }
  chMtxUnlock();

  // Convert all different types of newlines to single \n
  if (byte == '\r') {
    byte = '\n';
  }

  // Do nothing on \n if there's no previous command in the buffer
  if ((byte == '\n') && (recv_i == 0 || receive_buffer[recv_i - 1] == '\n')) {
    return;
  }

  // Store the received character
  receive_buffer[recv_i++] = byte;

  // Check for full command in the buffer
  if (byte == '\n') {

    // Parse the received command
    parse_command();
    return;
  }

  // Discard all data in buffer if buffer is full.
  // This also discards commands in buffer that has not been handled yet.
  if (recv_i == SC_CMD_MAX_RECV_BUF_LEN) {
    recv_i = 0;
  }
}



/*
 * Parse a received command.
 */
static void parse_command(void)
{
  int i;
  int found = 0;
  uint8_t command_buf[SC_CMD_MAX_RECV_BUF_LEN];

  // FIXME: there can be only one command in buffer, so this is useless
  // Check for full command in the buffer
  for(i = 0; i < recv_i; ++i) {
    if (receive_buffer[i] == '\n') {
      // Mark the position of the next possible command
      found = i + 1;
      break;
    }
  }

  if (!found ) {
    // No full command in buffer, do nothing
    return;
  }

  // Copy command to handling buffer
  for (i = 0; i < found; ++i) {
    command_buf[i] = receive_buffer[i];
  }

  // Clear the newline character as that command has been handled
  receive_buffer[found - 1] = 0;

  // Pop the command away from the recv buffer
  for (i = 0; i < recv_i - found; ++i) {
    receive_buffer[i] = receive_buffer[found];
  }
  recv_i -= found;

  switch (command_buf[0]) {
  case 'b':
    parse_command_blob(command_buf, found);
    break;
#if HAL_USE_PAL
  case 'g':
    parse_command_gpio(command_buf, found);
    break;
  case 'G':
    parse_command_gpio_all(command_buf, found);
    break;
#endif
  case 'l':
    parse_command_led(command_buf, found);
    break;
#if HAL_USE_PWM
  case 'p':
    parse_command_pwm(command_buf, found);
    break;
#endif
  case 'r':
    parse_command_radio(command_buf, found);
    break;
  default:
    // Invalid command, ignoring
    break;
  }

}



#if HAL_USE_PWM
/*
 * Parse PWM command
 */
static void parse_command_pwm(uint8_t *cmd, uint8_t len)
{

  switch (cmd[1]) {
  case 'f':
    parse_command_pwm_frequency(cmd, len);
	break;
  case '1':
  case '2':
  case '3':
  case '4':
  case '5':
  case '6':
  case '7':
  case '8':
    parse_command_pwm_duty(cmd, len);
	break;
  default:
	// Invalid PWM command, ignoring
	return;
  }
}



/*
 * Parse PWM frequency command
 */
static void parse_command_pwm_frequency(uint8_t *cmd, uint8_t cmd_len)
{
  uint16_t freq;

  // Parse the frequency value
  freq = (uint16_t)(sc_atoi(&cmd[2], cmd_len - 2));

  // Set the frequency
  sc_pwm_set_freq(freq);
}


/*
 * Parse PWM duty cycle command
 */
static void parse_command_pwm_duty(uint8_t *cmd, uint8_t cmd_len)
{
  uint8_t pwm;
  uint16_t value;
  int str_len;
  uint8_t str_duty[10];

  // Parse the PWM number as integer
  pwm = cmd[1] - '0';

  // Check for stop command
  if (cmd[2] == 's') {
    sc_pwm_stop(pwm);
    return;
  }

  // Parse the PWM value
  value = (uint16_t)(sc_atoi(&cmd[2], cmd_len - 2));

  // Set the duty cycle
  sc_pwm_set_duty(pwm, value);

  str_len = sc_itoa(value, str_duty, 8);

  // In error send 'e'
  if (str_len == 0) {
    str_duty[0] = 'e';
    str_len = 1;
  } else {
    str_duty[str_len++] = '\r';
    str_duty[str_len++] = '\n';
  }

  sc_uart_send_msg(SC_UART_LAST, str_duty, str_len);
}
#endif



/*
 * Parse led command
 */
static void parse_command_led(uint8_t *cmd, uint8_t cmd_len)
{

  // Unused currently
  (void)cmd_len;

  switch (cmd[1]) {
  case '0':
    sc_led_off();
    break;
  case '1':
    sc_led_on();
    break;
  case 't':
    sc_led_toggle();
    break;
  default:
    // Invalid value, ignoring command
	return;
  }
}



#if HAL_USE_PAL
/*
 * Parse gpio command
 */
static void parse_command_gpio(uint8_t *cmd, uint8_t cmd_len)
{
  uint8_t gpio;

  // Unused currently
  (void)cmd_len;

  gpio = cmd[1] - '0';

  switch (cmd[2]) {
  case '0':
    sc_gpio_off(gpio);
    break;
  case '1':
    sc_gpio_on(gpio);
    break;
  case 't':
    sc_gpio_toggle(gpio);
    break;
  default:
    // Invalid value, ignoring command
    break;
  }
}



/*
 * Parse gpio_all command (GXXXX)
 */
static void parse_command_gpio_all(uint8_t *cmd, uint8_t cmd_len)
{
  uint8_t gpio, gpios = 0;

  // Command must have states of all GPIO pins, ignore otherwise
  if (cmd_len < SC_GPIO_MAX_PINS + 1) {
    return;
  }

  for (gpio = 0; gpio < SC_GPIO_MAX_PINS; ++gpio) {
    if (cmd[1 + gpio] == '1') {
      gpios |= (1 << gpio);
    }
  }

  sc_gpio_set_state_all(gpios);
}
#endif



/*
 * Parse blob command (bXXXXX)
 */
static void parse_command_blob(uint8_t *cmd, uint8_t cmd_len)
{
  uint16_t bytes;

  bytes = sc_atoi(&cmd[1], cmd_len - 1);

  if (bytes == 0) {
    // Something went wrong, ignore
    chDbgAssert(0, "Failed to parse blob size", "#1");
  }

  if (bytes >= SC_BLOB_MAX_SIZE) {
    chDbgAssert(0, "Requested blob size too large", "#1");
    return;
  }

  chMtxLock(&blob_mtx);
  blob_i = 0;
  blob_len = bytes;
  chMtxUnlock();
}



/*
 * Parse radio related commands
 */
static void parse_command_radio(uint8_t *cmd, uint8_t cmd_len)
{
  // Unused currently
  (void)cmd_len;
#ifndef SC_HAS_RBV2
  (void)cmd;
#else
  switch (cmd[1]) {
  case 'b':
    sc_radio_reset_bsl();
    break;
  case 'f':
    sc_radio_flash();
    break;
  case 'r':
    if (cmd[2] == '1') {
      sc_radio_set_reset(1);
    } else {
      sc_radio_set_reset(0);
    }
    break;
  case 't':
    if (cmd[2] == '1') {
      sc_radio_set_test(1);
    } else {
      sc_radio_set_test(0);
    }
    break;
  case 'n':
    sc_radio_reset_normal();
    break;
  default:
    // Invalid value, ignoring command
    break;
  }
#endif
}


/*
 * Return pointer to newly received binary blob
 */
uint16_t sc_cmd_blob_get(uint8_t **blob)
{
  uint16_t len = 0;

  chMtxLock(&blob_mtx);
  if (blob_len > 0 && blob_i == blob_len) {
    len = blob_len;
  }
  chMtxUnlock();

  // There might still be a small change that the blob gets restarted between the check above and returning below.
  if (len) {
    *blob = blob_buf;
  } else {
    chDbgAssert(0, "No binary blob available", "#1");
    *blob = NULL;
  }
  return len;
}



/* Emacs indentatation information
   Local Variables:
   indent-tabs-mode:nil
   tab-width:2
   c-basic-offset:2
   End:
*/
