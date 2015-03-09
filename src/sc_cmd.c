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
static void parse_command_ping(uint8_t *cmd, uint8_t cmd_len);
static void parse_command_blob(uint8_t *cmd, uint8_t cmd_len);
static void parse_command_radio(uint8_t *cmd, uint8_t cmd_len);
static void parse_command_power(uint8_t *cmd, uint8_t cmd_len);
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
static mutex_t blob_mtx;

/*
 * Command storage
 */
#ifndef SC_CMD_MAX_COMMANDS
#define SC_CMD_MAX_COMMANDS   16
#endif
struct sc_cmd {
    uint8_t cmd;
    sc_cmd_cb cmd_cb;
};
static struct sc_cmd commands[SC_CMD_MAX_COMMANDS] = { {0, NULL}, };

/*
 * Initialize command module
 */
void sc_cmd_init(void)
{
  chMtxObjectInit(&blob_mtx);

  sc_cmd_register('b', parse_command_blob);
  sc_cmd_register('c', parse_command_power);
#if HAL_USE_PAL
  sc_cmd_register('g', parse_command_gpio);
  sc_cmd_register('G', parse_command_gpio_all);
  sc_cmd_register('l', parse_command_led);
#endif
  sc_cmd_register('P', parse_command_ping);
#if HAL_USE_PWM
  sc_cmd_register('p', parse_command_pwm);
#endif
  sc_cmd_register('r', parse_command_radio);

}




/*
 * Deinitialize command module
 */
void sc_cmd_deinit(void)
{
  // Nothing to do here.
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
      chMtxUnlock(&blob_mtx);
      return;
    }

    if (blob_i+1 >= SC_BLOB_MAX_SIZE) {
      chDbgAssert(0, "blob_i out of bounds");
      chMtxUnlock(&blob_mtx);
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
  chMtxUnlock(&blob_mtx);

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
 * Register a command and handler
 */
void sc_cmd_register(uint8_t cmd, sc_cmd_cb cb)
{
  int i = 0;

  do {
    chDbgAssert(i < SC_CMD_MAX_COMMANDS, "Too many commands registered!");
    chDbgAssert(commands[i].cmd != cmd, "Command registered twice!");
    i++;
  } while (i < SC_CMD_MAX_COMMANDS && commands[i].cmd != 0);

  if (i < SC_CMD_MAX_COMMANDS && commands[i].cmd == 0) {
    commands[i].cmd = cmd;
    commands[i].cmd_cb = cb;
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

  if (command_buf[0] == 0)
    return;

  for (i = 0; i < SC_CMD_MAX_COMMANDS; i++) {
    if (commands[i].cmd == command_buf[0] && commands[i].cmd_cb != NULL) {
      commands[i].cmd_cb(command_buf, found);
      return;
    }
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
  case '9':
  case 'a':
  case 'b':
  case 'c':
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

  // Parse the PWM number as hex
  if (cmd[1] >= '0' && cmd[1] <= '9') {
    pwm = cmd[1] - '0';
  } else {
    pwm = cmd[1] - 'a' + 10;
  }

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



#if HAL_USE_PAL
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
  (void)cmd_len;
  uint8_t gpio, gpios = 0;

  for (gpio = 0; gpio < SC_GPIO_MAX_PINS; ++gpio) {
    if (cmd[1 + gpio] == '1') {
      gpios |= (1 << gpio);
    } else if (cmd[1 + gpio] != '0') {
      /* If it's neither 1 nor 0, the command has
       * ended and we leave the rest bits as zero.
       * NOTE: this might have side-effects
       */
      break;
    }
  }

  sc_gpio_set_state_all(gpios);
}
#endif



/*
 * Parse ping command
 */
static void parse_command_ping(uint8_t *cmd, uint8_t cmd_len)
{
  (void)cmd;
  (void)cmd_len;

  msg_t pingmsg;

  pingmsg = sc_event_msg_create_type(SC_EVENT_TYPE_PING);
  sc_event_msg_post(pingmsg, SC_EVENT_MSG_POST_FROM_NORMAL);
}



/*
 * Parse blob command (bXXXXX)
 */
static void parse_command_blob(uint8_t *cmd, uint8_t cmd_len)
{
  uint16_t bytes;

  bytes = sc_atoi(&cmd[1], cmd_len - 1);

  if (bytes == 0) {
    // Something went wrong, ignore
    chDbgAssert(0, "Failed to parse blob size");
  }

  if (bytes >= SC_BLOB_MAX_SIZE) {
    chDbgAssert(0, "Requested blob size too large");
    return;
  }

  chMtxLock(&blob_mtx);
  blob_i = 0;
  blob_len = bytes;
  chMtxUnlock(&blob_mtx);
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
  chMtxUnlock(&blob_mtx);

  // There might still be a small change that the blob gets restarted between the check above and returning below.
  if (len) {
    *blob = blob_buf;
  } else {
    chDbgAssert(0, "No binary blob available");
    *blob = NULL;
  }
  return len;
}



/*
 * Parse power command
 */
static void parse_command_power(uint8_t *cmd, uint8_t cmd_len)
{

  // Unused currently
  (void)cmd_len;

  switch (cmd[1]) {
  case 'a':
    //sc_pwr_mode_standby(true, true);
    break;
  case 'o':
    //sc_pwr_mode_stop(true);
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
