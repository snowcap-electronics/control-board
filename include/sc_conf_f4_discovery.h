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

#ifndef SC_CONF_F4_DISCOVERY_H
#define SC_CONF_F4_DISCOVERY_H

#ifndef USER_LED
#define USER_LED            GPIOD_LED4
#endif

#ifndef USER_LED_PORT
#define USER_LED_PORT      GPIOD
#endif

// GPIO1 is PE7
#ifndef GPIO1_PIN
#define GPIO1_PIN          7
#define GPIO1_PORT         GPIOE
#endif

// GPIO2 is PE8
#ifndef GPIO2_PIN
#define GPIO2_PIN          8
#define GPIO2_PORT         GPIOE
#endif


// GPIO3 is PE9
#ifndef GPIO3_PIN
#define GPIO3_PIN          9
#define GPIO3_PORT         GPIOE
#endif

// GPIO4 is PE10
#ifndef GPIO4_PIN
#define GPIO4_PIN          10
#define GPIO4_PORT         GPIOE
#endif

// Test pin for debug state output
#ifndef TP_PB_PORT
#define TP_PB_PORT         GPIOA
#define TP_PB_PIN          GPIOA_PIN2
#endif

#ifndef PWMDX1
#define PWMDX1             PWMD3
#endif

#ifndef PWMDX2
#define PWMDX2             PWMD4
#endif

#ifndef ADCDX
#define ADCDX              ADCD1
#endif

#ifndef USBDX
#define USBDX              USBD1
#endif

#ifndef SC_UART1_TX_PORT
#define SC_UART1_TX_PORT   GPIOB
#define SC_UART1_TX_PIN    6
#define SC_UART1_TX_AF     7
#define SC_UART1_RX_PORT   GPIOB
#define SC_UART1_RX_PIN    7
#define SC_UART1_RX_AF     7
#endif

#ifdef SC_HAS_LIS302DL
#define SC_LIS302DL_SPIN           SPID1
#define SC_LIS302DL_CS_PORT        GPIOE
#define SC_LIS302DL_CS_PIN         GPIOE_CS_SPI
#define SC_LIS302DL_INT_DRDY_PORT  GPIOE
#define SC_LIS302DL_INT_DRDY_PIN   GPIOE_INT2
#ifndef HAL_USE_SPI
#error "lis302dl needs SPI"
#endif
#endif

/* F4 Discovery
I2C3
INT2_XM = PA3
INT1_XM = PA2
INT_G   = PA1
DRDY_G  = PC4
SDA     = PC9
SCL     = PA8
*/

#ifdef SC_HAS_LSM9DS0
// PC9 == I2C3_SDA, PA8 == I2C3_SCL, AF 4
#define SC_LSM9DS0_I2CN            I2CD3
#define SC_LSM9DS0_I2CN_SDA_PORT   GPIOC
#define SC_LSM9DS0_I2CN_SDA_PIN    GPIOC_PIN9
#define SC_LSM9DS0_I2CN_SDA_AF     4
#define SC_LSM9DS0_I2CN_SCL_PORT   GPIOA
#define SC_LSM9DS0_I2CN_SCL_PIN    GPIOA_PIN8
#define SC_LSM9DS0_I2CN_SCL_AF     4
#define SC_LSM9DS0_ADDR_XM         (0x3C >> 1)
#define SC_LSM9DS0_ADDR_G          (0xD4 >> 1)
#define SC_LSM9DS0_INT1_XM_PORT    GPIOA
#define SC_LSM9DS0_INT1_XM_PIN     GPIOA_PIN2
#define SC_LSM9DS0_INT2_XM_PORT    GPIOA
#define SC_LSM9DS0_INT2_XM_PIN     GPIOA_PIN3
#define SC_LSM9DS0_DRDY_G_PORT     GPIOC
#define SC_LSM9DS0_DRDY_G_PIN      GPIOC_PIN4
#ifndef HAL_USE_I2C
#error "lsm9ds0 needs I2C"
#endif
#endif

// FIXME: ADC pin macros from sc_adc_start_conversion should be here

#endif
