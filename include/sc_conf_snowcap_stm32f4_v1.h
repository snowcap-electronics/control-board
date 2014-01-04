/***
 * Snowcap Controller Board v1 configuration
 *
 * Copyright 2013 Tuomas Kulve, <tuomas.kulve@snowcap.fi>
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

#ifndef SC_CONF_SNOWCAP_STM32F4_V1_H
#define SC_CONF_SNOWCAP_STM32F4_V1_H

#ifndef USER_LED
#define USER_LED           GPIOC_PIN3
#endif

#ifndef USER_LED_PORT
#define USER_LED_PORT      GPIOC
#endif

// GPIO1 is PA15
#ifndef GPIO1_PIN
#define GPIO1_PIN          15
#define GPIO1_PORT         GPIOA
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

// GPIO4 (GPIO7 in Base Board v1) is PD2
#ifndef GPIO4_PIN
#define GPIO4_PIN          2
#define GPIO4_PORT         GPIOD
#endif

// GPIO5 (GPIO8 in Base Board v1) is PA8
#ifndef GPIO5_PIN
#define GPIO5_PIN          8
#define GPIO5_PORT         GPIOA
#endif

// GPIO6 (GPIO9 in Base Board v1) is PC4
#ifndef GPIO6_PIN
#define GPIO6_PIN          4
#define GPIO6_PORT         GPIOC
#endif

// GPIO7 (GPIO10 in Base Board v1) is PC5
#ifndef GPIO7_PIN
#define GPIO7_PIN          5
#define GPIO7_PORT         GPIOC
#endif

// Timer 4, ch 1-4
#ifndef PWMDX1
#define PWMDX1             PWMD4
#endif

// Timer 8, ch 1-4
#ifndef PWMDX2
#define PWMDX2             PWMD8
#endif

#ifndef ADCDX
#define ADCDX              ADCD1
#endif

#ifndef USBDX
#define USBDX              USBD1
#endif

/* LSM9DS0 minicap with Snowcap STM32F4 MCU Board v1
I2C2
INT2_XM = PC1
INT1_XM = PC0
INT_G   = PC4
DRDY_G  = PC5
SDA     = PB11
SCL     = PB10
*/

#ifdef SC_HAS_LSM9DS0
// PB11 == I2C2_SDA, PB10 == I2C2_SCL, AF 4
#define SC_LSM9DS0_I2CN            I2CD2
#define SC_LSM9DS0_I2CN_SDA_PORT   GPIOB
#define SC_LSM9DS0_I2CN_SDA_PIN    GPIOB_PIN11
#define SC_LSM9DS0_I2CN_SDA_AF     4
#define SC_LSM9DS0_I2CN_SCL_PORT   GPIOB
#define SC_LSM9DS0_I2CN_SCL_PIN    GPIOB_PIN10
#define SC_LSM9DS0_I2CN_SCL_AF     4
#define SC_LSM9DS0_ADDR_XM         (0x3C >> 1)
#define SC_LSM9DS0_ADDR_G          (0xD4 >> 1)
#define SC_LSM9DS0_INT1_XM_PORT    GPIOC
#define SC_LSM9DS0_INT1_XM_PIN     GPIOC_PIN0
#define SC_LSM9DS0_INT2_XM_PORT    GPIOC
#define SC_LSM9DS0_INT2_XM_PIN     GPIOC_PIN1
#define SC_LSM9DS0_DRDY_G_PORT     GPIOC
#define SC_LSM9DS0_DRDY_G_PIN      GPIOC_PIN5
#ifndef HAL_USE_I2C
#error "lsm9ds0 needs I2C"
#endif
#endif

// FIXME: ADC pin macros from sc_adc_start_conversion should be here

#endif
