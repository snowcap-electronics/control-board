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

#ifndef USBDX
#define USBDX              USBD1
#endif

#define SC_UART2_TX_PORT   GPIOA
#define SC_UART2_TX_PIN    2
#define SC_UART2_TX_AF     7
#define SC_UART2_RX_PORT   GPIOA
#define SC_UART2_RX_PIN    3
#define SC_UART2_RX_AF     7

#define SC_UART3_TX_PORT   GPIOC
#define SC_UART3_TX_PIN    GPIOC_PIN10
#define SC_UART3_TX_AF     7
#define SC_UART3_RX_PORT   GPIOC
#define SC_UART3_RX_PIN    GPIOC_PIN11
#define SC_UART3_RX_AF     7

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

#ifdef SC_HAS_LSM9DS1
// PB11 == I2C2_SDA, AF 4
// PB10 == I2C2_SCL, AF 4
// PC0 == base board GPIO2 == DRM = DRDYM
// PC4 == GPIO6 == base board GPIO9 == NT1 == INT1AG
// PC5 == GPIO7 == base board GPIO10 == NT2 == INT2AG
#define SC_LSM9DS1_I2CN            I2CD2
#define SC_LSM9DS1_I2CN_SDA_PORT   GPIOB
#define SC_LSM9DS1_I2CN_SDA_PIN    GPIOB_PIN11
#define SC_LSM9DS1_I2CN_SDA_AF     4
#define SC_LSM9DS1_I2CN_SCL_PORT   GPIOB
#define SC_LSM9DS1_I2CN_SCL_PIN    GPIOB_PIN10
#define SC_LSM9DS1_I2CN_SCL_AF     4
// Teensy LSM9DS1 shield has AD0 pulled up
#define SC_LSM9DS1_ADDR_AG         0x6B
#define SC_LSM9DS1_ADDR_M          0x1E
#define SC_LSM9DS1_INT1_AG_PORT    GPIOC
#define SC_LSM9DS1_INT1_AG_PIN     GPIOC_PIN4
#define SC_LSM9DS1_INT2_AG_PORT    GPIOC
#define SC_LSM9DS1_INT2_AG_PIN     GPIOC_PIN5
#define SC_LSM9DS1_DRDY_M_PORT     GPIOC
#define SC_LSM9DS1_DRDY_M_PIN      GPIOC_PIN0
#ifndef HAL_USE_I2C
#error "lsm9ds1 needs I2C"
#endif
#endif

#ifdef SC_HAS_SPIRIT1
#define SC_SPIRIT1_SPIN           SPID2
#define SC_SPIRIT1_SPI_SCK_PORT   GPIOB
#define SC_SPIRIT1_SPI_SCK_PIN    GPIOB_PIN13
#define SC_SPIRIT1_SPI_PORT       GPIOB
#define SC_SPIRIT1_SPI_MISO_PIN   GPIOB_PIN14
#define SC_SPIRIT1_SPI_MOSI_PIN   GPIOB_PIN15
#define SC_SPIRIT1_SPI_CS_PORT    GPIOB
#define SC_SPIRIT1_SPI_CS_PIN     GPIOB_PIN12
#define SC_SPIRIT1_SPI_AF         5
#define SC_SPIRIT1_SDN_PORT       GPIOC
#define SC_SPIRIT1_SDN_PIN        GPIOC_PIN2
#define SC_SPIRIT1_INT_PORT       GPIOC
#define SC_SPIRIT1_INT_PIN        GPIOC_PIN3  // Same pin as the user led
#ifndef HAL_USE_SPI
#error "SPIRIT1 needs SPI"
#endif
#endif

#ifdef SC_HAS_RBV2
// PC2 == SBWTDIO/nRST, PC3 == SBWTCK/TEST
#define SC_RADIO_NRST_PORT        GPIOC
#define SC_RADIO_NRST_PIN         GPIOC_PIN2
#define SC_RADIO_TEST_PORT        GPIOC
#define SC_RADIO_TEST_PIN         GPIOC_PIN3
#endif

// FIXME: ADC pin macros from sc_adc_start_conversion should be here

#endif
