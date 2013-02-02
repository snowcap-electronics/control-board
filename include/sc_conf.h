/***
 * Snowcap project specific configurations
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

#ifndef SC_CONF_H
#define SC_CONF_H

// Snowcap Control Board V1 defines
#ifndef USER_LED
#define USER_LED           GPIOB_LED1
#endif

#ifndef USER_LED_PORT
#define USER_LED_PORT      GPIOB
#endif

#ifndef PWMDX1
#define PWMDX1             PWMD3
#endif

#ifndef PWMDX2
#define PWMDX2             PWMD4
#endif

#ifndef I2CDX
#define I2CDX              I2CD3
#endif

#ifndef ADCDx
#define ADCDx              ADCD1
#endif

// FIXME: ADC pin macros from sc_adc_start_conversion should be here

#endif
