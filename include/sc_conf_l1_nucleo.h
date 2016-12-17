/***
 * ST STM32L152 Nucleo configuration
 *
 * Copyright 2015 Tuomas Kulve, <tuomas.kulve@snowcap.fi>
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

#ifndef SC_CONF_L1_NUCLEO_H
#define SC_CONF_L1_NUCLEO_H

#ifndef USER_LED
#define USER_LED           GPIOA_LED_GREEN
#endif

#ifndef USER_LED_PORT
#define USER_LED_PORT      GPIOA
#endif

// GPIO1 is PB3
#ifndef GPIO1_PIN
#define GPIO1_PIN          3
#define GPIO1_PORT         GPIOB
#endif

// GPIO2 is PB4
#ifndef GPIO2_PIN
#define GPIO2_PIN          4
#define GPIO2_PORT         GPIOB
#endif


// GPIO3 is PB5
#ifndef GPIO3_PIN
#define GPIO3_PIN          5
#define GPIO3_PORT         GPIOB
#endif

// GPIO4 is PB6
#ifndef GPIO4_PIN
#define GPIO4_PIN          6
#define GPIO4_PORT         GPIOB
#endif

#ifndef SC_UART2_TX_PORT
#define SC_UART2_TX_PORT   GPIOA
#define SC_UART2_TX_PIN    GPIOA_USART_TX
#define SC_UART2_TX_AF     7
#define SC_UART2_RX_PORT   GPIOA
#define SC_UART2_RX_PIN    GPIOA_USART_RX
#define SC_UART2_RX_AF     7
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
// Teensy LSM9DS1 + MS5611 shield has AD0 pulled up
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

#ifdef SC_HAS_MS5611
// PB10 == I2C2_SDA, AF 4
// PB11 == I2C2_SCL, AF 4
#define SC_MS5611_I2CN            I2CD2
#define SC_MS5611_I2CN_SDA_PORT   GPIOB
#define SC_MS5611_I2CN_SDA_PIN    GPIOB_PIN10
#define SC_MS5611_I2CN_SDA_AF     4
#define SC_MS5611_I2CN_SCL_PORT   GPIOB
#define SC_MS5611_I2CN_SCL_PIN    GPIOB_PIN11
#define SC_MS5611_I2CN_SCL_AF     4
// Teensy LSM9DS1 + MS5611 shield has AD0 pulled up
#define SC_MS5611_ADDR            0x77
#ifndef HAL_USE_I2C
#error "ms5611 needs I2C"
#endif
#endif


#ifdef SC_HAS_SPIRIT1
#define SC_SPIRIT1_SPIN           SPID1
#define SC_SPIRIT1_SPI_SCK_PORT   GPIOB
#define SC_SPIRIT1_SPI_SCK_PIN    3
#define SC_SPIRIT1_SPI_PORT       GPIOA
#define SC_SPIRIT1_SPI_MISO_PIN   GPIOA_PIN6
#define SC_SPIRIT1_SPI_MOSI_PIN   GPIOA_PIN7
#define SC_SPIRIT1_SPI_CS_PORT    GPIOB
#define SC_SPIRIT1_SPI_CS_PIN     GPIOB_PIN6
#define SC_SPIRIT1_SPI_AF         5
#define SC_SPIRIT1_SDN_PORT       GPIOA
#define SC_SPIRIT1_SDN_PIN        GPIOA_PIN10
#define SC_SPIRIT1_INT_PORT       GPIOC
#define SC_SPIRIT1_INT_PIN        GPIOC_PIN7
#ifndef HAL_USE_SPI
#error "SPIRIT1 needs SPI"
#endif
#endif

// FIXME: ADC pin macros from sc_adc_start_conversion should be here

#endif
