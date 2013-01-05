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
#include "sc_sdu.h"
#include "sc_event.h"

#define MAX_SEND_BUF_LEN      32
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
static void rxchar(uint16_t c, SC_UART uart);
static void circular_add_buffer(UARTDriver *uartdrv, uint8_t *msg, int len);

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

/* Boolean to tell if the last character was received from USB */
/* FIXME: Quite a hack */
static uint8_t uart_use_usb;

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

  // Don't use Serial over USB by default
  uart_use_usb = TRUE;

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
 * @param[in] len       Length of the message in bytes. 
 */
void sc_uart_send_msg(SC_UART uart, uint8_t *msg, int len)
{
  UARTDriver *uartdrv;

  // If last byte received from Serial USB, send message using Serial USB
  if (uart == SC_UART_USB || (uart == SC_UART_LAST && uart_use_usb)) {
	sc_sdu_send_msg(msg, len);
	return;
  }

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

  circular_add_buffer(uartdrv, msg, len);
}



/**
 * @brief   Receive a character from Serial USB (sc_sdu)
 *
 * @param[in] c         Received character
 */
void sc_uart_revc_usb_byte(uint8_t c)
{
  msg_t msg;

  // Mark that the last character is received from Serial USB
  uart_use_usb = TRUE;

  // Pass data to to command parsing through the main thread
  msg = sc_event_msg_create_recv_byte(c, SC_UART_USB);
  sc_event_msg_post(msg, SC_EVENT_MSG_POST_FROM_NORMAL);
}

/**
 * @brief   Send a string over UART.
 *
 * @param[in] uart      UART to use.
 * @param[in] str       String to send. Must be \0 terminated.
 */
void sc_uart_send_str(SC_UART uart, char *msg)
{
  int len = 0;

  if (msg == NULL) {
	return;
  }

  while (msg[len] != '\0') {
	++len;
  }

  sc_uart_send_msg(uart, (uint8_t *)msg, len);
}



/*
 * Delete message after sent (and start new send, if something to send).
 * This is called from the main event loop as we can't call this directly from ISR.
 */
void sc_uart_send_finished(void)
{
  chBSemWait(&circular_sem);

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

	uartStartSend(circular_uart[circular_sending],
				  circular_len[circular_sending],
				  circular_buf[circular_sending]);

  } else {
	// Nothing to send
	circular_sending = -1;
  }

  chBSemSignal(&circular_sem);
}



/*
 * End of transmission buffer callback.
 */
static void txend1_cb(UNUSED(UARTDriver *uartp))
{
  msg_t msg;

  msg = sc_event_msg_create_type(SC_EVENT_TYPE_UART_SEND_FINISHED);
  sc_event_msg_post(msg, SC_EVENT_MSG_POST_FROM_ISR);
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
  rxchar(c, SC_UART_1);
}
#endif

/*
 * Character received while out of the UART_RECEIVE state (UART2, xBee).
 */
#if STM32_UART_USE_USART2
static void rx2char_cb(UARTDriver *uartp, uint16_t c)
{
  last_uart = uartp;
  rxchar(c, SC_UART_2);
}
#endif



/*
 * Character received while out of the UART_RECEIVE state (UART3).
 */
#if STM32_UART_USE_USART3
static void rx3char_cb(UARTDriver *uartp, uint16_t c)
{
  last_uart = uartp;
  rxchar(c, SC_UART_3);
}
#endif



/*
 * Character received while out of the UART_RECEIVE state.
 */
static void rxchar(uint16_t c, SC_UART uart)
{
  msg_t msg;

  // Mark that the last character was received over UART
  uart_use_usb = FALSE;

  // Pass data to to command parsing through the main thread
  msg = sc_event_msg_create_recv_byte((uint8_t)c, uart);
  sc_event_msg_post(msg, SC_EVENT_MSG_POST_FROM_ISR);
}



/*
 * Receive error callback.
 */
static void rxerr_cb(UNUSED(UARTDriver *uartp), UNUSED(uartflags_t e))
{
  
}



/*
 * Queue message for sending (and start send, if currently idle)
 */
static void circular_add_buffer(UARTDriver *uartdrv, uint8_t *msg, int len)
{
  int circular_current;
  int i;

  chBSemWait(&circular_sem);

  // Lose message, if no space
  if (circular_free == -1) {
	chBSemSignal(&circular_sem);
	return;
  }

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
