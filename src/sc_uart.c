/***
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

/**
 * @file    src/sc_uart.c
 * @brief   UART spesific functions
 *
 * @addtogroup UART
 * @{
 */

#include "sc_utils.h"
#include "sc_uart.h"
#include "sc_pwm.h"
#include "sc_led.h"
#include "sc_icu.h"

#define MAX_RECV_BUF_LEN      16
#define MAX_SEND_BUF_LEN      16
#define MAX_CIRCULAR_BUFS     2
#define MIN_CMD_LEN           3        // 8 bit command, >=8 bit value, \n

static uint8_t recv_i = 0;
static uint8_t receive_buffer[MAX_RECV_BUF_LEN];
static UARTDriver *last_uart = NULL;

static void txend1_cb(UARTDriver *uartp);
static void txend2_cb(UARTDriver *uartp);
static void rxend_cb(UARTDriver *uartp);
#if STM32_UART_USE_USART1
static void rx1char_cb(UARTDriver *uartp, uint16_t c);
#endif
#if STM32_UART_USE_USART2
static void rx2char_cb(UARTDriver *uartp, uint16_t c);
#endif
#if STM32_UART_USE_USART3
static void rx3char_cb(UARTDriver *uartp, uint16_t c);
#endif
static void rxerr_cb(UARTDriver *uartp, uartflags_t e);
static void rxchar(uint16_t c);

static void parse_command(void);
static void parse_command_pwm(void);
static void parse_command_pwm_frequency(void);
static void parse_command_pwm_duty(void);
static void parse_command_led(void);

/*
 * Circular buffer for sending.
 * TODO: Could be one per UART but for simplicity and saving memory there's
 * only one for now.
 */
static uint8_t circular_buf[MAX_CIRCULAR_BUFS][MAX_SEND_BUF_LEN];
static uint8_t circular_len[MAX_CIRCULAR_BUFS];
static UARTDriver *circular_uart[MAX_CIRCULAR_BUFS];
static int8_t circular_sending;
static int8_t circular_free;
static BinarySemaphore circular_sem;

/*
 * UART driver configuration structure for UART1
 */
#if STM32_UART_USE_USART1
static UARTConfig uart_cfg_1 = {
  txend1_cb,  // End of transmission buffer callback.
  txend2_cb,  // Physical end of transmission callback.
  rxend_cb,   // Receive buffer filled callback.
  rx1char_cb, // Character received while out of the UART_RECEIVE state.
  rxerr_cb,   // Receive error callback.
  115200,
  0,
  USART_CR2_LINEN,
  0
};
#endif



/*
 * UART driver configuration structure for UART2 (xBee)
 */
#if STM32_UART_USE_USART2
static UARTConfig uart_cfg_2 = {
  txend1_cb,  // End of transmission buffer callback.
  txend2_cb,  // Physical end of transmission callback.
  rxend_cb,   // Receive buffer filled callback.
  rx2char_cb, // Character received while out of the UART_RECEIVE state.
  rxerr_cb,   // Receive error callback.
  115200,
  0,
  USART_CR2_LINEN,
  0
};
#endif



/*
 * UART driver configuration structure for UART3
 */
#if STM32_UART_USE_USART3
static UARTConfig uart_cfg_3 = {
  txend1_cb,  // End of transmission buffer callback.
  txend2_cb,  // Physical end of transmission callback.
  rxend_cb,   // Receive buffer filled callback.
  rx3char_cb, // Character received while out of the UART_RECEIVE state.
  rxerr_cb,   // Receive error callback.
  115200,
  0,
  USART_CR2_LINEN,
  0
};
#endif


/**
 * @brief   Initialize an UART
 * @note    Several UARTs can be initialized with several calls. The lastly
 *          initialized UART will be used when sending data using the
 *          SC_UART_LAST.
 *
 * @param[in] uart      UART to initialize
 */
void sc_uart_init(SC_UART uart)
{
  int i;

  switch (uart) {
#if STM32_UART_USE_USART1
  case SC_UART_1:
	uartStart(&UARTD1, &uart_cfg_1);
	last_uart = &UARTD1;
	break;
#endif
#if STM32_UART_USE_USART2
  case SC_UART_2:
	uartStart(&UARTD2, &uart_cfg_2);
	last_uart = &UARTD2;
	break;
#endif
#if STM32_UART_USE_USART3
  case SC_UART_3:
	uartStart(&UARTD3, &uart_cfg_3);
	last_uart = &UARTD3;
	break;
#endif
  default:
	// Invalid uart, do nothing
	// CHECKME: assert?
	return;
  }

  // Initialize sending side circular buffer state
  circular_sending = -1;
  circular_free = 0;
  for (i = 0; i < MAX_CIRCULAR_BUFS; ++i) {
	circular_len[i] = 0;
  }
  chBSemInit(&circular_sem, FALSE);
}



/**
 * @brief   Send a message over UART.
 *
 * @param[in] uart      UART to use.
 * @param[in] msg       Message to send.
 * @param[in] len       Lenght of the message in bytes.
 */
void sc_uart_send_msg(SC_UART uart, uint8_t *msg, int len)
{
  UARTDriver *uartdrv;
  int i;
  int circular_current;

  switch (uart) {
#if STM32_UART_USE_USART1
  case SC_UART_1:
	uartdrv = &UARTD1;
	break;
#endif
#if STM32_UART_USE_USART2
  case SC_UART_2:
	uartdrv = &UARTD2;
	break;
#endif
#if STM32_UART_USE_USART3
  case SC_UART_3:
	uartdrv = &UARTD3;
	break;
#endif
  case SC_UART_LAST:
	uartdrv = last_uart;
	break;
  default:
	// Invalid uart, do nothing
	// CHECKME: assert?
	return;
  }

  // Check for oversized message
  if (len > MAX_SEND_BUF_LEN) {
	// CHECKME: assert?
	return;
  }

  // Check for free buffers
  if (circular_free == -1) {
	// CHECKME: assert?
	return;
  }

  chBSemWaitS(&circular_sem);

  // Copy the message to static circular buffer
  for (i = 0; i < len; ++i) {
	circular_buf[circular_free][i] = msg[i];
  }
  circular_len[circular_free] = len;
  circular_uart[circular_free] = uartdrv;

  // Store temporarily the current index
  circular_current = circular_free;

  // Check for buffer wrapping
  if (++circular_free == MAX_CIRCULAR_BUFS) {
	circular_free = 0;
  }

  // Check for full buffer
  if (circular_sending != -1 && circular_free == circular_sending) {
	circular_free = -1;
  }

  // Check for idle UART
  if (circular_sending == -1) {
	circular_sending = circular_current;
	uartStartSend(circular_uart[circular_current],
				  circular_len[circular_current],
				  circular_buf[circular_current]);
  }

  chBSemSignal(&circular_sem);
}



/*
 * End of transmission buffer callback.
 */
static void txend1_cb(UNUSED(UARTDriver *uartp))
{
  chBSemWaitS(&circular_sem);

  // Mark buffer as empty
  circular_len[circular_sending] = 0;

  // If no free buffers, mark this one as free
  if (circular_free == -1) {
	circular_free = circular_sending;
  }

  // Advance the currently sending index
  if (++circular_sending == MAX_CIRCULAR_BUFS) {
	circular_sending = 0;
  }

  // Check for buffer to send
  if (circular_len[circular_sending] > 0) {

	uartStartSendI(circular_uart[circular_sending],
				   circular_len[circular_sending],
				   circular_buf[circular_sending]);

  } else {
	// Nothing to send
	circular_sending = -1;
  }

  chBSemSignal(&circular_sem);
}



/*
 * Physical end of transmission callback.
 */
static void txend2_cb(UNUSED(UARTDriver *uartp))
{
  
}



/*
 * Receive buffer filled callback.
 */
static void rxend_cb(UNUSED(UARTDriver *uartp))
{
  
}



/*
 * Character received while out of the UART_RECEIVE state (UART1).
 */
#if STM32_UART_USE_USART1
static void rx1char_cb(UARTDriver *uartp, uint16_t c)
{
  last_uart = uartp;
  rxchar(c);
}
#endif

static uint8_t str_rssi[16];
/*
 * Character received while out of the UART_RECEIVE state (UART2, xBee).
 */
#if STM32_UART_USE_USART2
static void rx2char_cb(UARTDriver *uartp, uint16_t c)
{
  int period;

  last_uart = uartp;

  // Send xBee signal strength after every full command
  if (1/*c == '\n'*/) {

	period = sc_icu_get_period(1 /* xBee RSSI with jumper wire */);

	// Send only if have valid strength
	if (1/*period != -1*/) {
	  int str_len;

	  str_rssi[0] = 's';
	  str_rssi[1] = ':';
	  if (period == -1) {
		str_rssi[2] = '-';
		str_len = 1;
	  } else {
		str_len = sc_itoa(period, &str_rssi[2], 12);
	  }
	  
	  str_rssi[str_len + 2] = '\r';
	  str_rssi[str_len + 3] = '\n';
	  sc_uart_send_msg(SC_UART_LAST, str_rssi, str_len + 4);
	}
  }

  rxchar(c);
}
#endif



/*
 * Character received while out of the UART_RECEIVE state (UART3).
 */
#if STM32_UART_USE_USART3
static void rx3char_cb(UARTDriver *uartp, uint16_t c)
{
  last_uart = uartp;
  rxchar(c);
}
#endif



/*
 * Character received while out of the UART_RECEIVE state.
 */
static void rxchar(uint16_t c)
{
  // TODO: blink a led (optionally)?

  // Store the received character
  receive_buffer[recv_i++] = c;

  // Ignore zero length commands
  if (recv_i == 1 && (c == '\n' || c == '\r')) {
	recv_i = 0;
	return;
  }
  
  // Check for full command in the buffer
  if (c == '\n' || c == '\r') {
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
 * Receive error callback.
 */
static void rxerr_cb(UNUSED(UARTDriver *uartp), UNUSED(uartflags_t e))
{
  
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

