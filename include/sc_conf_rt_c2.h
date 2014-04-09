/***
 * RuuviTracker C2 configuration
 *
 * Copyright 2014 Tuomas Kulve, <tuomas.kulve@snowcap.fi>
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

#ifndef SC_CONF_RT_C2_H
#define SC_CONF_RT_C2_H

#ifndef USER_LED
#define USER_LED           GPIOB_LED1
#endif

#ifndef USER_LED_PORT
#define USER_LED_PORT      GPIOB
#endif

#ifndef USER_LED2
#define USER_LED2          GPIOB_LED2
#endif

#ifndef USER_LED2_PORT
#define USER_LED2_PORT     GPIOB
#endif

// Pick some generic GPIO pins
#ifndef GPIO1_PIN
#define GPIO1_PORT         GPIOB
#define GPIO1_PIN          0
#endif

#ifndef GPIO2_PIN
#define GPIO2_PORT         GPIOB
#define GPIO2_PIN          1
#endif

#ifndef GPIO3_PIN
#define GPIO3_PORT         GPIOB
#define GPIO3_PIN          2
#endif

#ifndef GPIO4_PIN
#define GPIO4_PORT         GPIOB
#define GPIO4_PIN          12
#endif

#ifndef USBDX
#define USBDX              USBD1
#endif

#define SC_UART1_TX_PORT   GPIOA
#define SC_UART1_TX_PIN    GPIOA_USART1_TXD
#define SC_UART1_TX_AF     7
#define SC_UART1_RX_PORT   GPIOA
#define SC_UART1_RX_PIN    GPIOA_USART1_RXD
#define SC_UART1_RX_AF     7

#define SC_UART2_TX_PORT   GPIOA
#define SC_UART2_TX_PIN    GPIOA_GPS_TXD
#define SC_UART2_TX_AF     7
#define SC_UART2_RX_PORT   GPIOA
#define SC_UART2_RX_PIN    GPIOA_GPS_RXD
#define SC_UART2_RX_AF     7

#define SC_UART3_TX_PORT   GPIOB
#define SC_UART3_TX_PIN    GPIOB_GSM_TXD
#define SC_UART3_TX_AF     7
#define SC_UART3_RX_PORT   GPIOB
#define SC_UART3_RX_PIN    GPIOB_GSM_RXD
#define SC_UART3_RX_AF     7

#endif
