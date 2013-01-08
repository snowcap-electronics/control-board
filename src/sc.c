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

void sc_init(void)
{
  /* Initialize ChibiOS HAL and core */
  halInit();
  chSysInit();

  /* Init uarts 1 and 2 (radio) */
  sc_uart_init(SC_UART_1);
  //sc_uart_init(SC_UART_2);

  /* Init PWM */
  sc_pwm_init();

  /* Initialize command parsing */
  sc_cmd_init();

#if 0 /*HAL_USE_ICU*/
  /* Init ICU for reading xBee signal strength */
  sc_icu_init(1);
#endif

#if HAL_USE_I2C
  /* Init I2C bus */
  sc_i2c_init(0);
#endif

#if HAL_USE_SERIAL_USB
  /* Initializes a serial-over-USB CDC driver */
  sc_sdu_init();
#endif

  sc_adc_init();
}

/* Emacs indentatation information
   Local Variables:
   indent-tabs-mode:nil
   tab-width:2
   c-basic-offset:2
   End:
*/
