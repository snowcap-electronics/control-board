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

#ifndef SC_CONF_SNOWCAP_V1_H
#define SC_CONF_SNOWCAP_V1_H

// FIXME: Should prefix all these with SC_
#ifndef USER_LED
#define USER_LED           GPIOB_LED1
#endif

#ifndef USER_LED_PORT
#define USER_LED_PORT      GPIOB
#endif

// GPIO1 is PB8
#ifndef GPIO1_PIN
#define GPIO1_PIN          8
#define GPIO1_PORT         GPIOB
#endif

// GPIO2 is PB5
#ifndef GPIO2_PIN
#define GPIO2_PIN          5
#define GPIO2_PORT         GPIOB
#endif

// GPIO3 is PD2
#ifndef GPIO3_PIN
#define GPIO3_PIN          2
#define GPIO3_PORT         GPIOD
#endif

// GPIO4 is PC12
#ifndef GPIO4_PIN
#define GPIO4_PIN          12
#define GPIO4_PORT         GPIOC
#endif

#ifndef PWMDX1
#define PWMDX1             PWMD3
#endif

#ifndef PWMDX2
#define PWMDX2             PWMD4
#endif

// PC0: ADC123_IN10
// PC1: ADC123_IN11
// PC4: ADC12_IN14
// PC5: ADC12_IN15
#ifndef ADCDX
#define ADCDX              ADCD1
#define SC_ADC_1_PORT      GPIOC
#define SC_ADC_1_PIN       0
#define SC_ADC_1_SMPR_CFG  smpr1
#define SC_ADC_1_SMPR      ADC_SMPR1_SMP_AN10
#define SC_ADC_1_AN        ADC_CHANNEL_IN10
#define SC_ADC_2_PORT      GPIOC
#define SC_ADC_2_PIN       1
#define SC_ADC_2_SMPR_CFG  smpr1
#define SC_ADC_2_SMPR      ADC_SMPR1_SMP_AN11
#define SC_ADC_2_AN        ADC_CHANNEL_IN11
#define SC_ADC_3_PORT      GPIOC
#define SC_ADC_3_PIN       4
#define SC_ADC_3_SMPR_CFG  smpr1
#define SC_ADC_3_SMPR      ADC_SMPR1_SMP_AN14
#define SC_ADC_3_AN        ADC_CHANNEL_IN14
#define SC_ADC_4_PORT      GPIOC
#define SC_ADC_4_PIN       5
#define SC_ADC_4_SMPR_CFG  smpr1
#define SC_ADC_4_SMPR      ADC_SMPR1_SMP_AN15
#define SC_ADC_4_AN        ADC_CHANNEL_IN15
#endif

#ifndef USBDX
#define USBDX              USBD2
#endif

/* Snowcap CB v1
I2C3
INT2_XM = PA12 G5
INT1_XM = PC8  G6
INT_G   = PA1  3.3V <-- FIX to XB_RTS
DRDY_G  = PA0  GND  <-- FIX to XB_CTS
SDA     = PC9  IMU_SDA
SCL     = PA8  IMU_SCL
*/

#ifdef SC_HAS_LSM9DS0
// PC9 == I2C3_SDA, PA8 == I2C3_SCL, AF 4
#define SC_LSM9DS0_I2CN            I2CD3
#define SC_LSM9DS0_I2CN_SDA_PORT   GPIOC
#define SC_LSM9DS0_I2CN_SDA_PIN    9
#define SC_LSM9DS0_I2CN_SDA_AF     4
#define SC_LSM9DS0_I2CN_SCL_PORT   GPIOA
#define SC_LSM9DS0_I2CN_SCL_PIN    8
#define SC_LSM9DS0_I2CN_SCL_AF     4
#define SC_LSM9DS0_ADDR_XM         (0x3C >> 1)
#define SC_LSM9DS0_ADDR_G          (0xD4 >> 1)
#define SC_LSM9DS0_INT1_XM_PORT    GPIOC
#define SC_LSM9DS0_INT1_XM_PIN     8
#define SC_LSM9DS0_INT2_XM_PORT    GPIOA
#define SC_LSM9DS0_INT2_XM_PIN     12
#define SC_LSM9DS0_DRDY_G_PORT     GPIOA
#define SC_LSM9DS0_DRDY_G_PIN      0
#ifndef HAL_USE_I2C
#error "lsm9ds0 needs I2C"
#endif
#endif

// FIXME: ADC pin macros from sc_adc_start_conversion should be here

#endif
