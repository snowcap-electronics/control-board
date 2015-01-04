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

#if defined(SC_ENABLE_WFI_DBG)
  sc_pwr_set_wfi_dbg();
#endif

  chRegSetThreadName("main");

  /* Initialize command parsing */
  sc_cmd_init();

#if HAL_USE_UART
  if (subsystems & (SC_MODULE_UART1 | SC_MODULE_UART2 | SC_MODULE_UART3) ) {
    sc_uart_init();
  }
#endif

  if (subsystems & SC_MODULE_UART1) {
#if STM32_UART_USE_USART1 && HAL_USE_UART
    sc_uart_start(SC_UART_1);
#else
    chDbgAssert(0, "HAL_USE_UART undefined", "#1");
#endif
  }

  if (subsystems & SC_MODULE_UART2) {
#if STM32_UART_USE_USART2 && HAL_USE_UART
    sc_uart_start(SC_UART_2);
#else
    chDbgAssert(0, "HAL_USE_UART undefined", "#1");
#endif
  }

  if (subsystems & SC_MODULE_UART3) {
#if STM32_UART_USE_USART3 && HAL_USE_UART
    sc_uart_start(SC_UART_3);
#else
    chDbgAssert(0, "HAL_USE_UART undefined", "#1");
#endif
  }

  if (subsystems & SC_MODULE_PWM) {
#if HAL_USE_PWM
    sc_pwm_init();
#else
    chDbgAssert(0, "HAL_USE_PWM undefined", "#1");
#endif
  }

  if (subsystems & SC_MODULE_ICU) {
#if HAL_USE_ICU
    sc_icu_init(1);
#else
    chDbgAssert(0, "HAL_USE_ICU undefined", "#1");
#endif
  }

  if (subsystems & SC_MODULE_SDU) {
#if HAL_USE_SERIAL_USB
    sc_sdu_init();
#else
    chDbgAssert(0, "HAL_USE_SERIAL_USB undefined", "#1");
#endif
  }

  if (subsystems & SC_MODULE_ADC) {
#if HAL_USE_ADC
    sc_adc_init();
#else
    chDbgAssert(0, "HAL_USE_ADC undefined", "#1");
#endif
  }

  if (subsystems & SC_MODULE_GPIO) {
#if HAL_USE_PAL
    sc_gpio_init();
#else
    chDbgAssert(0, "HAL_USE_PAL undefined", "#1");
#endif
  }

  if (subsystems & SC_MODULE_LED) {
#if HAL_USE_PAL
    sc_led_init();
#else
    chDbgAssert(0, "HAL_USE_PAL undefined", "#1");
#endif
  }

  if (subsystems & SC_MODULE_RADIO) {
#ifdef SC_HAS_RBV2
    sc_radio_init();
#else
    chDbgAssert(0, "SC_HAS_RBV2 undefined", "#1");
#endif
  }

  if (subsystems & SC_MODULE_HID) {
#if HAL_USE_USB
    sc_hid_init();
#else
    chDbgAssert(0, "HAL_USE_USB undefined", "#1");
#endif
  }
}


void sc_deinit(uint32_t subsystems)
{

  if (subsystems & SC_MODULE_HID) {
#if HAL_USE_USB
    sc_hid_deinit();
#else
    chDbgAssert(0, "HAL_USE_USB undefined", "#1");
#endif
  }

  if (subsystems & SC_MODULE_RADIO) {
#ifdef SC_HAS_RBV2
    sc_radio_deinit();
#else
    chDbgAssert(0, "SC_HAS_RBV2 undefined", "#1");
#endif
  }

  if (subsystems & SC_MODULE_LED) {
#if HAL_USE_PAL
    sc_led_deinit();
#else
    chDbgAssert(0, "HAL_USE_PAL undefined", "#1");
#endif
  }

  if (subsystems & SC_MODULE_GPIO) {
#if HAL_USE_PAL
    sc_gpio_deinit();
#else
    chDbgAssert(0, "HAL_USE_PAL undefined", "#1");
#endif
  }

  if (subsystems & SC_MODULE_ADC) {
#if HAL_USE_ADC
    sc_adc_deinit();
#else
    chDbgAssert(0, "HAL_USE_ADC undefined", "#1");
#endif
  }

  if (subsystems & SC_MODULE_SDU) {
#if HAL_USE_SERIAL_USB
    sc_sdu_deinit();
#else
    chDbgAssert(0, "HAL_USE_SERIAL_USB undefined", "#1");
#endif
  }

  if (subsystems & SC_MODULE_ICU) {
#if HAL_USE_ICU
    sc_icu_deinit(1);
#else
    chDbgAssert(0, "HAL_USE_ICU undefined", "#1");
#endif
  }

  if (subsystems & SC_MODULE_PWM) {
#if HAL_USE_PWM
    sc_pwm_deinit();
#else
    chDbgAssert(0, "HAL_USE_PWM undefined", "#1");
#endif
  }

  if (subsystems & SC_MODULE_UART3) {
#if STM32_UART_USE_USART3 && HAL_USE_UART
    sc_uart_stop(SC_UART_3);
#else
    chDbgAssert(0, "HAL_USE_UART undefined", "#1");
#endif
  }

  if (subsystems & SC_MODULE_UART2) {
#if STM32_UART_USE_USART2 && HAL_USE_UART
    sc_uart_stop(SC_UART_2);
#else
    chDbgAssert(0, "HAL_USE_UART undefined", "#1");
#endif
  }

  if (subsystems & SC_MODULE_UART1) {
#if STM32_UART_USE_USART1 && HAL_USE_UART
    sc_uart_stop(SC_UART_1);
#else
    chDbgAssert(0, "HAL_USE_UART undefined", "#1");
#endif
  }

#if HAL_USE_UART
  if (subsystems & (SC_MODULE_UART1 | SC_MODULE_UART2 | SC_MODULE_UART3) ) {
    sc_uart_deinit();
  }
#endif

  /* Deinitialize command parsing */
  sc_cmd_deinit();

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
