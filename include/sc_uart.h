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
#if STM32_UART_USE_USART1
  SC_UART_1    = 1,
#endif
#if STM32_UART_USE_USART2
  SC_UART_2    = 2,
#endif
#if STM32_UART_USE_USART3
  SC_UART_3    = 3,
#endif
#if STM32_UART_USE_USART4
  SC_UART_4    = 4,
#endif
  /**
   * @brief Use the UART that has latest activity.
   */
  SC_UART_LAST = 64,
} SC_UART;

void sc_uart_init(SC_UART uart);
void sc_uart_send_msg(SC_UART uart, uint8_t *msg, int len);

#endif
