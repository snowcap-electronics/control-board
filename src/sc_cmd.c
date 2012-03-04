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

static void parse_command(void);
static void parse_command_pwm(void);
static void parse_command_pwm_frequency(void);
static void parse_command_pwm_duty(void);
static void parse_command_led(void);

/*
 * Buffer for incoming commands.
 * FIXME: Separate handling for SPI/UART1-3?
 */
#define MAX_RECV_BUF_LEN      16
#define MIN_CMD_LEN           3        // 8 bit command, >=8 bit value, \n
static uint8_t receive_buffer[MAX_RECV_BUF_LEN];
static uint8_t recv_i = 0;

/*
 * Receive byte
 */
void sc_cmd_push_byte(uint8_t byte)
{  
  // Store the received character
  receive_buffer[recv_i++] = byte;

  // Ignore zero length commands
  if (recv_i == 1 && (byte == '\n' || byte == '\r')) {
	recv_i = 0;
	return;
  }


  // Check for full command in the buffer
  if (byte == '\n' || byte == '\r') {
	// FIXME: return and handle in separate thread?
	parse_command();
	recv_i = 0;
	return;
  }

  // Discard all data in buffer if buffer is full and no \n received
  if (recv_i == MAX_RECV_BUF_LEN) {
	recv_i = 0;
  }
}



/*
 * Parse a received command
 */
static void parse_command(void)
{
  if (recv_i < MIN_CMD_LEN) {
	// Invalid command, ignoring
	return;
  }

  switch (receive_buffer[0]) {
  case 'p':
	parse_command_pwm();
	break;
  case 'l':
	parse_command_led();
	break;
  default:
	// Invalid command, ignoring
	break;
  }	
}



/*
 * Parse PWM command
 */
static void parse_command_pwm(void)
{

  switch (receive_buffer[1]) {
  case 'f':
	parse_command_pwm_frequency();
	break;
  case '1':
  case '2':
  case '3':
  case '4':
  case '5':
  case '6':
  case '7':
  case '8':
	parse_command_pwm_duty();
	break;
  default:
	// Invalid PWM command, ignoring
	return;
  }
}



/*
 * Parse PWM frequency command
 */
static void parse_command_pwm_frequency(void)
{
  uint16_t freq;

  // Parse the frequency value
  freq = (uint16_t)(sc_atoi(&receive_buffer[2], recv_i - 2));

  // Set the frequency
  sc_pwm_set_freq(freq);
}


static uint8_t str_duty[10];

/*
 * Parse PWM duty cycle command
 */
static void parse_command_pwm_duty(void)
{
  uint8_t pwm;
  uint16_t value;
  int str_len;

  // Parse the PWM number as integer
  pwm = receive_buffer[1] - '0';

  // Check for stop command
  if (receive_buffer[2] == 's') {
	sc_pwm_stop(pwm);
	return;
  }

  // Parse the PWM value
  value = (uint16_t)(sc_atoi(&receive_buffer[2], recv_i - 2));

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
static void parse_command_led(void)
{

  switch (receive_buffer[1]) {
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

