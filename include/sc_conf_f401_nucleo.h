/***
 * ST STM32F401RE Nucleo configuration
 *
 * Copyright 2015-2016 Tuomas Kulve, <tuomas.kulve@snowcap.fi>
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

#ifndef SC_CONF_F401_NUCLEO_H
#define SC_CONF_F401_NUCLEO_H

#ifndef USER_LED
#define USER_LED           GPIOA_LED_GREEN
#endif

#ifndef USER_LED_PORT
#define USER_LED_PORT      GPIOA
#endif

// GPIO1 is PB4
#ifndef GPIO1_PIN
#define GPIO1_PIN          4
#define GPIO1_PORT         GPIOB
#endif

// GPIO2 is PC0
#ifndef GPIO2_PIN
#define GPIO2_PIN          0
#define GPIO2_PORT         GPIOC
#endif


// GPIO3 is PC1
#ifndef GPIO3_PIN
#define GPIO3_PIN          1
#define GPIO3_PORT         GPIOC
#endif

// GPIO4 is PD2
#ifndef GPIO4_PIN
#define GPIO4_PIN          2
#define GPIO4_PORT         GPIOD
#endif

// GPIO5 is PB5
#ifndef GPIO5_PIN
#define GPIO5_PIN          5
#define GPIO5_PORT         GPIOB
#endif

// GPIO6 is PC4
#ifndef GPIO6_PIN
#define GPIO6_PIN          4
#define GPIO6_PORT         GPIOC
#endif


// GPIO7 is PC5
#ifndef GPIO7_PIN
#define GPIO7_PIN          5
#define GPIO7_PORT         GPIOC
#endif

// Timer 4, ch 1-4, PB6-9, AF2
#ifndef PWMDX1
#define PWMDX1             PWMD4
#define SC_PWM1_1_PORT     GPIOB
#define SC_PWM1_1_PIN      6
#define SC_PWM1_1_AF       2
#define SC_PWM1_2_PORT     GPIOB
#define SC_PWM1_2_PIN      7
#define SC_PWM1_2_AF       2
#define SC_PWM1_3_PORT     GPIOB
#define SC_PWM1_3_PIN      8
#define SC_PWM1_3_AF       2
#define SC_PWM1_4_PORT     GPIOB
#define SC_PWM1_4_PIN      9
#define SC_PWM1_4_AF       2
#endif

// Timer 3, ch 1-4, PC6-9, AF2
#ifndef PWMDX2
#define PWMDX2             PWMD3
#define SC_PWM2_1_PORT     GPIOC
#define SC_PWM2_1_PIN      6
#define SC_PWM2_1_AF       2
#define SC_PWM2_2_PORT     GPIOC
#define SC_PWM2_2_PIN      7
#define SC_PWM2_2_AF       2
#define SC_PWM2_3_PORT     GPIOC
#define SC_PWM2_3_PIN      8
#define SC_PWM2_3_AF       2
#define SC_PWM2_4_PORT     GPIOC
#define SC_PWM2_4_PIN      9
#define SC_PWM2_4_AF       2
#endif

#ifndef ADCDX
#define ADCDX              ADCD1
#endif

#ifndef SC_UART2_TX_PORT
#define SC_UART2_TX_PORT   GPIOA
#define SC_UART2_TX_PIN    GPIOA_USART_TX
#define SC_UART2_TX_AF     7
#define SC_UART2_RX_PORT   GPIOA
#define SC_UART2_RX_PIN    GPIOA_USART_RX
#define SC_UART2_RX_AF     7
#endif


// FIXME: ADC pin macros from sc_adc_start_conversion should be here

#endif
