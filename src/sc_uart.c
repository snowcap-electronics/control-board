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
#include "sc_cmd.h"

#define MAX_SEND_BUF_LEN      16
#define MAX_CIRCULAR_BUFS     2

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

  // Pass data to command module
  sc_cmd_push_byte((uint8_t)c);
}



/*
 * Receive error callback.
 */
static void rxerr_cb(UNUSED(UARTDriver *uartp), UNUSED(uartflags_t e))
{
  
}
