/*
 * SC init
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

#include "sc.h"

void sc_init(uint32_t subsystems)
{
  /* Initialize ChibiOS HAL and core */
  halInit();
  chSysInit();

  /* Initialize command parsing */
  sc_cmd_init();

  if (subsystems & SC_INIT_UART1) {
#if STM32_UART_USE_USART1 && HAL_USE_UART
    sc_uart_init(SC_UART_1);
#else
    chDbgAssert(0, "HAL_USE_UART undefined", "#1");
#endif
  }

  if (subsystems & SC_INIT_UART2) {
#if STM32_UART_USE_USART2 && HAL_USE_UART
    sc_uart_init(SC_UART_2);
#else
    chDbgAssert(0, "HAL_USE_UART undefined", "#1");
#endif
  }

  if (subsystems & SC_INIT_UART3) {
#if STM32_UART_USE_USART3 && HAL_USE_UART
    sc_uart_init(SC_UART_3);
#else
    chDbgAssert(0, "HAL_USE_UART undefined", "#1");
#endif
  }

  if (subsystems & SC_INIT_PWM) {
#if HAL_USE_PWM
    sc_pwm_init();
#else
    chDbgAssert(0, "HAL_USE_PWM undefined", "#1");
#endif
  }

  /* Init ICU for reading xBee signal strength */
  if (subsystems & SC_INIT_ICU) {
#if HAL_USE_ICU
    sc_icu_init(1);
#else
    chDbgAssert(0, "HAL_USE_ICU undefined", "#1");
#endif
  }

  /* Initializes a serial-over-USB CDC driver */
  if (subsystems & SC_INIT_SDU) {
#if HAL_USE_SERIAL_USB
    sc_sdu_init();
#else
    chDbgAssert(0, "HAL_USE_SERIAL_USB undefined", "#1");
#endif
  }

  if (subsystems & SC_INIT_ADC) {
#if HAL_USE_ADC
    sc_adc_init();
#else
    chDbgAssert(0, "HAL_USE_ADC undefined", "#1");
#endif
  }

  if (subsystems & SC_INIT_GPIO) {
#if HAL_USE_PAL
    sc_gpio_init();
#else
    chDbgAssert(0, "HAL_USE_PAL undefined", "#1");
#endif
  }

  if (subsystems & SC_INIT_LED) {
#if HAL_USE_PAL
    sc_led_init();
#else
    chDbgAssert(0, "HAL_USE_PAL undefined", "#1");
#endif
  }

  if (subsystems & SC_INIT_RADIO) {
#ifdef SC_HAS_RBV2
    sc_radio_init();
#else
    chDbgAssert(0, "SC_HAS_RBV2 undefined", "#1");
#endif
  }
}

/* libc stub */
int _getpid(void) {return 1;}
/* libc stub */
void _exit(int i) {(void)i;for(;;){}}
/* libc stub */
#include <errno.h>
#undef errno
extern int errno;
int _kill(int pid, int sig) {
  (void)pid;
  (void)sig;
  errno = EINVAL;
  return -1;
}
/* libc stub*/
int _gettimeofday(void *tv, void *tz)
{
  (void)tv;
  (void)tz;
  errno = EINVAL;
  return -1;
}


/* Emacs indentatation information
   Local Variables:
   indent-tabs-mode:nil
   tab-width:2
   c-basic-offset:2
   End:
*/
