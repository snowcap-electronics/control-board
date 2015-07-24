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

#if HAL_USE_UART

#ifndef UART_MAX_SEND_BUF_LEN
#define UART_MAX_SEND_BUF_LEN      128
#endif

#ifndef UART_MAX_CIRCULAR_BUFS
#define UART_MAX_CIRCULAR_BUFS     8
#endif

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
static bool circular_add_buffer(UARTDriver *uartdrv, const uint8_t *msg, int len);
static void uart_set_enable(SC_UART uart, uint8_t enable);
static uint8_t uart_is_enabled(UARTDriver *drv);

/*
 * Circular buffer for sending.
 * TODO: Could be one per UART but for simplicity and saving memory there's
 * only one for now.
 */
static uint8_t circular_init_done = 0;
static uint8_t circular_buf[UART_MAX_CIRCULAR_BUFS][UART_MAX_SEND_BUF_LEN];
static uint8_t circular_len[UART_MAX_CIRCULAR_BUFS];
static UARTDriver *circular_uart[UART_MAX_CIRCULAR_BUFS];
static int8_t circular_sending;
static int8_t circular_free;
static mutex_t circular_mtx;

/* Boolean to tell if the last character was received from USB */
/* FIXME: Quite a hack */
static uint8_t uart_use_usb;

static uint8_t uarts_enabled;

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


void sc_uart_set_config(SC_UART uart, uint32_t speed, uint32_t cr1, uint32_t cr2, uint32_t cr3)
{
  UARTConfig *cfg;

  switch (uart) {
#if STM32_UART_USE_USART1
  case SC_UART_1:
    cfg = &uart_cfg_1;
    break;
#endif
#if STM32_UART_USE_USART2
  case SC_UART_2:
    cfg = &uart_cfg_2;
    break;
#endif
#if STM32_UART_USE_USART3
  case SC_UART_3:
    cfg = &uart_cfg_3;
    break;
#endif
  default:
    // Invalid uart
    chDbgAssert(0, "Invalid UART specified");
    return;
  }

  cfg->speed = speed;
  cfg->cr1 = cr1;
  cfg->cr2 = cr2;
  cfg->cr3 = cr3;
}

/**
 * @brief   Initialize an UART
 */
void sc_uart_init(void)
{
  int i;

  chDbgAssert(!circular_init_done, "uart already initialized");

  // Initialize sending side circular buffer state
  circular_init_done = 1;
  circular_sending = -1;
  circular_free = 0;
  for (i = 0; i < UART_MAX_CIRCULAR_BUFS; ++i) {
    circular_len[i] = 0;
  }
  chMtxObjectInit(&circular_mtx);
}



void sc_uart_deinit(void)
{
#if STM32_UART_USE_USART1
  chDbgAssert(uart_is_enabled(&UARTD1) == 0, "Uart 1 still enabled in deinit");
#endif
#if STM32_UART_USE_USART2
  chDbgAssert(uart_is_enabled(&UARTD2) == 0, "Uart 2 still enabled in deinit");
#endif
#if STM32_UART_USE_USART3
  chDbgAssert(uart_is_enabled(&UARTD3) == 0, "Uart 3 still enabled in deinit");
#endif

  circular_init_done = 0;
}



/**
 * @brief   Start an UART
 *
 * @param[in] uart      UART to start
 */
void sc_uart_start(SC_UART uart)
{
  switch (uart) {
#if STM32_UART_USE_USART1
  case SC_UART_1:
#ifdef SC_UART1_TX_PORT
    palSetPadMode(SC_UART1_TX_PORT, SC_UART1_TX_PIN, PAL_MODE_ALTERNATE(SC_UART1_TX_AF));
    palSetPadMode(SC_UART1_RX_PORT, SC_UART1_RX_PIN, PAL_MODE_ALTERNATE(SC_UART1_RX_AF));
#endif
    uartStart(&UARTD1, &uart_cfg_1);
    if (last_uart == NULL) {
      last_uart = &UARTD1;
    }
    uart_set_enable(SC_UART_1, 1);
    break;
#endif
#if STM32_UART_USE_USART2
  case SC_UART_2:
#ifdef SC_UART2_TX_PORT
    palSetPadMode(SC_UART2_TX_PORT, SC_UART2_TX_PIN, PAL_MODE_ALTERNATE(SC_UART2_TX_AF));
    palSetPadMode(SC_UART2_RX_PORT, SC_UART2_RX_PIN, PAL_MODE_ALTERNATE(SC_UART2_RX_AF));
#endif
    uartStart(&UARTD2, &uart_cfg_2);
    if (last_uart == NULL) {
      last_uart = &UARTD2;
    }
    uart_set_enable(SC_UART_2, 1);
    break;
#endif
#if STM32_UART_USE_USART3
  case SC_UART_3:
    uartStart(&UARTD3, &uart_cfg_3);
    if (last_uart == NULL) {
      last_uart = &UARTD3;
    }
    uart_set_enable(SC_UART_3, 1);
    break;
#endif
  default:
    // Invalid uart, do nothing
    chDbgAssert(0, "Invalid UART specified");
    return;
  }
}

void sc_uart_stop(SC_UART uart)
{
  switch (uart) {
#if STM32_UART_USE_USART1
  case SC_UART_1:
    uartStop(&UARTD1);
    uart_set_enable(SC_UART_1, 0);
    break;
#endif
#if STM32_UART_USE_USART2
  case SC_UART_2:
    uartStop(&UARTD2);
    uart_set_enable(SC_UART_2, 0);
    break;
#endif
#if STM32_UART_USE_USART3
  case SC_UART_3:
    uartStop(&UARTD3);
    uart_set_enable(SC_UART_3, 0);
    break;
#endif
  default:
    // Invalid uart
    chDbgAssert(0, "Invalid UART specified");
    return;
  }

  // FIXME: should empty the queue messages for stopped UART
}



void sc_uart_default_usb(uint8_t enable)
{
  uart_use_usb = enable;
}

/**
 * @brief   Send a message over UART.
 *
 * @param[in] uart      UART to use.
 * @param[in] msg       Message to send.
 * @param[in] len       Length of the message in bytes. 
 */
void sc_uart_send_msg(SC_UART uart, const uint8_t *msg, int len)
{
  UARTDriver *uartdrv;
  int bytes_done = 0;
  int bytes_left = len;

#if HAL_USE_SERIAL_USB
  // If last byte received from Serial USB, send message using Serial USB
  if (uart == SC_UART_USB || (uart == SC_UART_LAST && uart_use_usb)) {
    sc_sdu_send_msg(msg, len);
    return;
  }
#endif

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
    chDbgAssert(0, "Invalid UART specified");
    return;
  }

  while (bytes_left) {
    int tmplen = bytes_left;
    if (bytes_left > UART_MAX_SEND_BUF_LEN) {
      tmplen = UART_MAX_SEND_BUF_LEN;
    }

    if (!circular_add_buffer(uartdrv, &msg[bytes_done], tmplen)) {
      // FIXME: overflow: add to some statistics?
      return;
    }
    bytes_done += tmplen;
    bytes_left -= tmplen;
  }
}



/**
 * @brief   Receive a character from Serial USB (sc_sdu)
 *
 * @param[in] c         Received character
 */
#if !HAL_USE_SERIAL_USB
void sc_uart_revc_usb_byte(UNUSED(uint8_t c))
{
  chDbgAssert(0, "HAL_USE_SERIAL_USB not defined, cannot call sc_uart_revc_usb_byte()");
}
#else
void sc_uart_revc_usb_byte(uint8_t c)
{
  msg_t msg;

  // Mark that the last character is received from Serial USB
  uart_use_usb = TRUE;

  // Pass data to to command parsing through the main thread
  msg = sc_event_msg_create_recv_byte(c, SC_UART_USB);
  sc_event_msg_post(msg, SC_EVENT_MSG_POST_FROM_NORMAL);
}
#endif

/**
 * @brief   Send a string over UART.
 *
 * @param[in] uart      UART to use.
 * @param[in] str       String to send. Must be \0 terminated.
 */
void sc_uart_send_str(SC_UART uart, const char *msg)
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
  chMtxLock(&circular_mtx);

  while (1) {
    // Mark buffer as empty
    circular_len[circular_sending] = 0;

    // If no free buffers, mark this one as free
    if (circular_free == -1) {
      circular_free = circular_sending;
    }

    // Advance the currently sending index
    if (++circular_sending == UART_MAX_CIRCULAR_BUFS) {
      circular_sending = 0;
    }

    // Check for buffer to send
    if (circular_len[circular_sending] > 0) {
      if (!uart_is_enabled(circular_uart[circular_sending])) {
        // UART not enabled, skip to next message in queue
        continue;
      }
      uartStartSend(circular_uart[circular_sending],
                    circular_len[circular_sending],
                    circular_buf[circular_sending]);
      break;
    } else {
      // Nothing to send
      circular_sending = -1;
      break;
    }
  }

  chMtxUnlock(&circular_mtx);
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
static bool circular_add_buffer(UARTDriver *uartdrv, const uint8_t *msg, int len)
{
  int circular_current;
  int i;

  chMtxLock(&circular_mtx);

  // Lose message, if no space
  if (circular_free == -1) {
    chMtxUnlock(&circular_mtx);
    return false;
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
  if (++circular_free == UART_MAX_CIRCULAR_BUFS) {
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

  chMtxUnlock(&circular_mtx);

  return true;
}



/*
 * Set UART's enable status for circular buffers
 */
static void uart_set_enable(SC_UART uart, uint8_t enable)
{

  chMtxLock(&circular_mtx);

  if (enable) {
    uarts_enabled |= (1 << uart);
  } else {
    uarts_enabled &= ~(1 << uart);
  }

  chMtxUnlock(&circular_mtx);
}



/*
 * Get UART's enable status
 */
static uint8_t uart_is_enabled(UARTDriver *drv)
{
  SC_UART uart = SC_UART_LAST;
#if STM32_UART_USE_USART1
  if (drv == &UARTD1) {
    uart = SC_UART_1;
  }
#endif
#if STM32_UART_USE_USART2
  if (drv == &UARTD2) {
    uart = SC_UART_2;
  }
#endif
#if STM32_UART_USE_USART3
  if (drv == &UARTD3) {
    uart = SC_UART_3;
  }
#endif

  if (uart == SC_UART_LAST) {
    chDbgAssert(0, "Invalid UART driver");
    return 0;
  }

  return uarts_enabled & (1 << uart);
}
#endif // HAL_USE_UART



/* Emacs indentatation information
   Local Variables:
   indent-tabs-mode:nil
   tab-width:2
   c-basic-offset:2
   End:
*/
