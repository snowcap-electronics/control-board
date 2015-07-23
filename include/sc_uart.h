/*
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
#if HAL_USE_UART
/**
 * @file    include/sc_uart.h
 * @brief   UART spesific functions
 *
 * @addtogroup UART
 * @{
 */

#ifndef SC_UART_H
#define SC_UART_H

#include <sc_utils.h>

/**
 * @brief   UART definitions.
 */
typedef enum SC_UART {
#if defined STM32_UART_USE_USART1 && STM32_UART_USE_USART1
  SC_UART_1    = 1,
#endif
#if defined STM32_UART_USE_USART2 && STM32_UART_USE_USART2
  SC_UART_2    = 2,
#endif
#if defined STM32_UART_USE_USART3 && STM32_UART_USE_USART3
  SC_UART_3    = 3,
#endif
#if defined STM32_UART_USE_USART4 && STM32_UART_USE_USART4
  SC_UART_4    = 4,
#endif
#if defined HAL_USE_SERIAL_USB && HAL_USE_SERIAL_USB
  SC_UART_USB  = 5,
#endif
  /**
   * @brief Use the UART that has latest activity.
   */
  SC_UART_LAST = 64,
} SC_UART;

void sc_uart_set_config(SC_UART uart, uint32_t speed, uint32_t cr1, uint32_t cr2, uint32_t cr3);
void sc_uart_init(void);
void sc_uart_start(SC_UART uart);
void sc_uart_stop(SC_UART uart);
void sc_uart_deinit(void);
void sc_uart_default_usb(uint8_t enable);
void sc_uart_send_msg(SC_UART uart, const uint8_t *msg, int len);
void sc_uart_send_str(SC_UART uart, const char *msg);
void sc_uart_send_finished(void);
/* Hackish way to receive characters from Serial USB */
void sc_uart_revc_usb_byte(uint8_t byte);

#endif
#endif // HAL_USE_UART



/* Emacs indentatation information
   Local Variables:
   indent-tabs-mode:nil
   tab-width:2
   c-basic-offset:2
   End:
*/
