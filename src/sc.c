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
    sc_uart_init(SC_UART_1);
  }

  if (subsystems & SC_INIT_UART2) {
    sc_uart_init(SC_UART_2);
  }

  
  if (subsystems & SC_INIT_PWM) {
    sc_pwm_init();
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
    sc_adc_init();
  }

  if (subsystems & SC_INIT_GPIO) {
    sc_gpio_init();
  }

}

/* Emacs indentatation information
   Local Variables:
   indent-tabs-mode:nil
   tab-width:2
   c-basic-offset:2
   End:
*/
