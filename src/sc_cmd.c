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

#include "sc_cmd.h"
#include "sc_uart.h"
#include "sc_pwm.h"
#include "sc_led.h"
#include "sc_event.h"

static void parse_command_pwm(uint8_t *cmd, uint8_t cmd_len);
static void parse_command_pwm_frequency(uint8_t *cmd, uint8_t cmd_len);
static void parse_command_pwm_duty(uint8_t *cmd, uint8_t cmd_len);
static void parse_command_led(uint8_t *cmd, uint8_t cmd_len);

/*
 * Buffer for incoming commands.
 * FIXME: Separate handling for SPI/UART1-3?
 */
#define MAX_RECV_BUF_LEN      16
#define MIN_CMD_LEN           3        // 8 bit command, >=8 bit value, \n
static uint8_t receive_buffer[MAX_RECV_BUF_LEN];
static uint8_t recv_i = 0;

/*
 * Initialize command module
 */
void sc_cmd_init(void)
{
  // Nothing
}




/*
 * Receive byte. This can be called only from an interrupt handler.
 * from_isr must be set to 1 when called from ISR, 0 otherwise.
 */
void sc_cmd_push_byte(uint8_t byte, uint8_t from_isr)
{
  #define LOCK() if (from_isr) chSysLockFromIsr() else chSysLock();
  #define UNLOCK() if (from_isr) chSysUnlockFromIsr() else chSysUnlock();

  // Lock receive_buffer
  LOCK();

  // Convert all different types of newlines to single \n
  if (byte == '\r') {
    byte = '\n';
  }

  // Do nothing on \n if there's no previous command in the buffer
  if ((byte == '\n') && (recv_i == 0 || receive_buffer[recv_i - 1] == '\n')) {
    UNLOCK();
    return;
  }

  // Store the received character
  receive_buffer[recv_i++] = byte;

  // Check for full command in the buffer
  if (byte == '\n') {

    // Signal about new data
    sc_event_action(SC_EVENT_TYPE_PARSE_COMMAND);

    UNLOCK();
    return;
  }

  // Discard all data in buffer if buffer is full.
  // This also discards commands in buffer that has not been handled yet.
  if (recv_i == MAX_RECV_BUF_LEN) {
    recv_i = 0;
  }

  UNLOCK();

  #undef UNLOCK
  #undef LOCK

  return;
}



/*
 * Parse a received command.
 */
void sc_cmd_parse_command(void)
{
  int i;
  int found = 0;
  uint8_t command_buf[MAX_RECV_BUF_LEN];

  // Lock receive_buffer
  chSysLock();

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
    chSysUnlock();
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

  chSysUnlock();

  switch (command_buf[0]) {
  case 'p':
    parse_command_pwm(command_buf, found);
    break;
  case 'l':
    parse_command_led(command_buf, found);
    break;
  default:
    // Invalid command, ignoring
    break;
  }

}



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

/* Emacs indentatation information
   Local Variables:
   indent-tabs-mode:nil
   tab-width:2
   c-basic-offset:2
   End:
*/
