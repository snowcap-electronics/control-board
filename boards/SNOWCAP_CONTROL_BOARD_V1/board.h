/*
    ChibiOS/RT - Copyright (C) 2006-2013 Giovanni Di Sirio
                               2013 Tuomas Kulve <tuomas.kulve@snowcap.fi>

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.
*/

#ifndef _BOARD_H_
#define _BOARD_H_

/*
 * Setup for Snowcap Control Board v1 board.
 */

/*
 * Board identifier.
 */
#define BOARD_SNOWCAP_V1
#define BOARD_NAME              "Snowcap Control Board v1"

/*
 * Board frequencies.
 */
#define STM32_LSECLK            32768
#define STM32_HSECLK            25000000

/*
 * Board voltages.
 * Required for performance limits calculation.
 */
#define STM32_VDD               300

/*
 * MCU type as defined in the ST header.
 */
#define STM32F405xx

/*
 * IO pins assignments.
 */

#define GPIOB_LED1              8

/*
 * I/O ports initial setup, this configuration is established soon after reset
 * in the initialization code.
 * Please refer to the STM32 Reference Manual for details.
 */
#define PIN_MODE_INPUT(n)           (0 << ((n) * 2))
#define PIN_MODE_OUTPUT(n)          (1 << ((n) * 2))
#define PIN_MODE_ALTERNATE(n)       (2 << ((n) * 2))
#define PIN_MODE_ANALOG(n)          (3 << ((n) * 2))
#define PIN_OTYPE_PUSHPULL(n)       (0 << (n))
#define PIN_OTYPE_OPENDRAIN(n)      (1 << (n))
#define PIN_OSPEED_2M(n)            (0 << ((n) * 2))
#define PIN_OSPEED_25M(n)           (1 << ((n) * 2))
#define PIN_OSPEED_50M(n)           (2 << ((n) * 2))
#define PIN_OSPEED_100M(n)          (3 << ((n) * 2))
#define PIN_PUDR_FLOATING(n)        (0 << ((n) * 2))
#define PIN_PUDR_PULLUP(n)          (1 << ((n) * 2))
#define PIN_PUDR_PULLDOWN(n)        (2 << ((n) * 2))
#define PIN_AFIO_AF0(n)             (0 << ((n % 8) * 4))
#define PIN_AFIO_AF1(n)             (1 << ((n % 8) * 4))
#define PIN_AFIO_AF2(n)             (2 << ((n % 8) * 4))
#define PIN_AFIO_AF3(n)             (3 << ((n % 8) * 4))
#define PIN_AFIO_AF4(n)             (4 << ((n % 8) * 4))
#define PIN_AFIO_AF5(n)             (5 << ((n % 8) * 4))
#define PIN_AFIO_AF6(n)             (6 << ((n % 8) * 4))
#define PIN_AFIO_AF7(n)             (7 << ((n % 8) * 4))
#define PIN_AFIO_AF8(n)             (8 << ((n % 8) * 4))
#define PIN_AFIO_AF9(n)             (9 << ((n % 8) * 4))
#define PIN_AFIO_AF10(n)            (10 << ((n % 8) * 4))
#define PIN_AFIO_AF11(n)            (11 << ((n % 8) * 4))
#define PIN_AFIO_AF12(n)            (12 << ((n % 8) * 4))
#define PIN_AFIO_AF13(n)            (13 << ((n % 8) * 4))
#define PIN_AFIO_AF14(n)            (14 << ((n % 8) * 4))
#define PIN_AFIO_AF15(n)            (15 << ((n % 8) * 4))

/*
 * Port A setup.
 * PA0  - USART2 CTS (xBee), AF7, input floating / input pull-up
 * PA1  - USART2 RTS (xBee), AF7, push-pull
 * PA2  - USART2 TX (xBee), AF7, push-pull
 * PA3  - USART2 RX (xBee), AF7, input floating / input pull-up
 * PA4  - USB Power SW, GPIO, push-pull
 * PA5  - DAC1, GPIO (FIXME: use DAC), push-pull
 * PA6  - PWM1, AF2, push-pull
 * PA7  - PWM2, AF2, push-pull
 * PA8  - I2C3 SCL (For IMU), AF4, open-drain
 * PA9  - USART1 TX (Bootloader / GPS), AF7, push-pull
 * PA10 - USART1 RX (Bootloader / GPS), AF7, input floating / input pull-up
 * PA11 - xBee RSSI pwm input, AF1, input floating / input pull-down
 * PA12 - IMU GPIO2, GPIO, push-pull
 * PA13 - JTMS, AF0, internal pull-up
 * PA14 - JTCK, AF0, internal pull-down
 * PA15 - JTDI, AF0, internal pull-up
 */
#define VAL_GPIOA_MODER             (PIN_MODE_ALTERNATE(0)    | \
                                     PIN_MODE_ALTERNATE(1)   | \
                                     PIN_MODE_ALTERNATE(2)   | \
                                     PIN_MODE_ALTERNATE(3)   | \
                                     PIN_MODE_OUTPUT(4) | \
                                     PIN_MODE_OUTPUT(5) | \
                                     PIN_MODE_ALTERNATE(6) | \
                                     PIN_MODE_ALTERNATE(7) | \
                                     PIN_MODE_ALTERNATE(8)   | \
                                     PIN_MODE_ALTERNATE(9)   | \
                                     PIN_MODE_ALTERNATE(10)   | \
                                     PIN_MODE_ALTERNATE(11) | \
                                     PIN_MODE_OUTPUT(12) | \
                                     PIN_MODE_ALTERNATE(13)   | \
                                     PIN_MODE_ALTERNATE(14)   | \
                                     PIN_MODE_ALTERNATE(15))

#define VAL_GPIOA_OTYPER            (PIN_OTYPE_OPENDRAIN(0) | \
                                     PIN_OTYPE_PUSHPULL(1) | \
                                     PIN_OTYPE_PUSHPULL(2) | \
                                     PIN_OTYPE_OPENDRAIN(3) | \
                                     PIN_OTYPE_PUSHPULL(4) | \
                                     PIN_OTYPE_PUSHPULL(5) | \
                                     PIN_OTYPE_PUSHPULL(6) | \
                                     PIN_OTYPE_PUSHPULL(7) | \
                                     PIN_OTYPE_OPENDRAIN(8) | \
                                     PIN_OTYPE_PUSHPULL(9) | \
                                     PIN_OTYPE_OPENDRAIN(10) | \
                                     PIN_OTYPE_OPENDRAIN(11) | \
                                     PIN_OTYPE_PUSHPULL(12) | \
                                     0 /* FIXME: JTMS */ | \
                                     0 /* FIXME: JTCK */ | \
                                     0 /* FIXME: JTDI */)

#define VAL_GPIOA_OSPEEDR           0xFFFFFFFF /* All pins 100MHz */

#define VAL_GPIOA_PUPDR             (PIN_PUDR_PULLUP(0)    | \
                                     PIN_PUDR_FLOATING(1)    | \
                                     PIN_PUDR_FLOATING(2)    | \
                                     PIN_PUDR_PULLUP(3)    |	\
                                     PIN_PUDR_FLOATING(4)    | \
                                     PIN_PUDR_FLOATING(5)    | \
                                     PIN_PUDR_PULLDOWN(6)    | \
                                     PIN_PUDR_PULLDOWN(7)    | \
                                     PIN_PUDR_FLOATING(9)    | \
                                     PIN_PUDR_PULLUP(10)    |	\
                                     PIN_PUDR_PULLDOWN(11)    | \
                                     PIN_PUDR_FLOATING(12)    | \
                                     0 /* FIXME: JTMS */ | \
                                     0 /* FIXME: JTCK */ | \
                                     0 /* FIXME: JTDI */)

#define VAL_GPIOA_ODR               0x00000000 /* All pins low by default */

#define VAL_GPIOA_AFRL              (PIN_AFIO_AF7(0) | \
                                     PIN_AFIO_AF7(1) | \
                                     PIN_AFIO_AF7(2) | \
                                     PIN_AFIO_AF7(3) | \
                                     PIN_AFIO_AF2(6) | \
                                     PIN_AFIO_AF2(7))
#define VAL_GPIOA_AFRH              (PIN_AFIO_AF4(8) | \
                                     PIN_AFIO_AF7(9) | \
                                     PIN_AFIO_AF7(10) | \
                                     PIN_AFIO_AF1(11) | \
                                     PIN_AFIO_AF0(13) | \
                                     PIN_AFIO_AF0(14) | \
                                     PIN_AFIO_AF0(15))


/*
 * Port B setup.
 * PB0  - PWM3, AF2, push-pull
 * PB1  - PWM4, AF2, push-pull
 * PB2  - BOOT1 (GND)
 * PB3  - JTDO, AF0, input floating
 * PB4  - JNRESET, AF0, input pull-up
 * PB5  - GPIO2, GPIO, push-pull
 * PB6  - PWM7, AF2, push-pull (same for PWM and ICU)
 * PB7  - PWM8, AF2 push-pull
 * PB8  - GPIO1, GPIO, push-pull
 * PB9  - SPI2 NSS, AF5, push-pull
 * PB10 - SPI2 SCK, AF5, push-pull
 * PB11 - GPIO (xBee not-reset), pull-up
 * PB12 - USB OTG HS ID, AF12
 * PB13 - USB OTG HS VBUS, "Other function"??
 * PB14 - USB OTG HS DM, AF12
 * PB15 - USB OTG HS DP, AF12
 */
#define VAL_GPIOB_MODER             (PIN_MODE_ALTERNATE(0)    | \
                                     PIN_MODE_ALTERNATE(1)   | \
                                     0 /* BOOT1 */   | \
                                     PIN_MODE_ALTERNATE(3)   | \
                                     PIN_MODE_ALTERNATE(4) | \
                                     PIN_MODE_OUTPUT(5) | \
                                     PIN_MODE_ALTERNATE(6) | \
                                     PIN_MODE_ALTERNATE(7) | \
                                     PIN_MODE_OUTPUT(8)   | \
                                     PIN_MODE_ALTERNATE(9)   | \
                                     PIN_MODE_ALTERNATE(10)   | \
                                     PIN_MODE_OUTPUT(11) | \
                                     PIN_MODE_ALTERNATE(12) | \
                                     PIN_MODE_INPUT(13)  /* CHECKME: vbus detection */  | \
                                     PIN_MODE_ALTERNATE(14)   | \
                                     PIN_MODE_ALTERNATE(15))

#define VAL_GPIOB_OTYPER            (PIN_OTYPE_PUSHPULL(0) | \
                                     PIN_OTYPE_PUSHPULL(1) | \
                                     0 /* BOOT1 */   | \
                                     PIN_OTYPE_OPENDRAIN(3) | \
                                     0 /* FIXME: JNRESET */ | \
                                     PIN_OTYPE_PUSHPULL(5) | \
                                     PIN_OTYPE_PUSHPULL(6) | \
                                     PIN_OTYPE_PUSHPULL(7) | \
                                     PIN_OTYPE_PUSHPULL(8) | \
                                     PIN_OTYPE_PUSHPULL(9) | \
                                     PIN_OTYPE_PUSHPULL(10) | \
                                     PIN_OTYPE_PUSHPULL(11) | \
                                     0 /* FIXME: OTG ID */ | \
                                     0 /* FIXME: OTG VBUS */ | \
                                     0 /* FIXME: OTG DM */ | \
                                     0 /* FIXME: OTG DP */)

#define VAL_GPIOB_OSPEEDR           0xFFFFFFFF /* All pins 100MHz */

#define VAL_GPIOB_PUPDR             (PIN_PUDR_PULLDOWN(0)    | \
                                     PIN_PUDR_PULLDOWN(1)    | \
                                     0 /* BOOT1 */ | \
                                     PIN_PUDR_FLOATING(3)    |	\
                                     PIN_PUDR_PULLUP(4)    | \
                                     PIN_PUDR_FLOATING(5)    | \
                                     PIN_PUDR_PULLDOWN(6)    | \
                                     PIN_PUDR_PULLDOWN(7)    | \
                                     PIN_PUDR_FLOATING(9)    | \
                                     PIN_PUDR_FLOATING(10)    |	\
                                     PIN_PUDR_FLOATING(11) | \
                                     0 /* FIXME: OTG ID */ | \
                                     0 /* FIXME: OTG VBUS */ | \
                                     0 /* FIXME: OTG DM */ | \
                                     0 /* FIXME: OTG DP */)

#define VAL_GPIOB_ODR               0x00000800 /* All pins low by default, except PB11 (xBee RST) */

#define VAL_GPIOB_AFRL              (PIN_AFIO_AF2(0) | \
                                     PIN_AFIO_AF2(1) | \
                                     PIN_AFIO_AF0(3) | \
                                     PIN_AFIO_AF0(4) | \
                                     PIN_AFIO_AF2(6) | \
                                     PIN_AFIO_AF2(7))
#define VAL_GPIOB_AFRH              (PIN_AFIO_AF5(9) | \
                                     PIN_AFIO_AF5(10) | \
                                     PIN_AFIO_AF12(12) | \
                                     PIN_AFIO_AF12(14) | \
                                     PIN_AFIO_AF12(15))


/*
 * Port C setup.
 * PC0  - AN1, open drain + pull-up
 * PC1  - AN2, open drain + pull-up
 * PC2  - SPI2 MISO, AF5, push-pull
 * PC3  - SPI2 MOSI, AF5, input floating / input pull-up
 * PC4  - AN3, open drain + pull-up
 * PC5  - AN4, open drain + pull-up
 * PC6  - PWM5, AF3, push-pull
 * PC7  - PWM6, AF3, push-pull
 * PC8  - IMU GPIO1, GPIO, push-pull
 * PC9  - I2C3 SDA (IMU), AF4, open-drain
 * PC10 - USART3 TX, AF7, push-pull
 * PC11 - USART3 RX, AF7, input floating / input pull-up
 * PC12 - GPIO4, GPIO, push-pull
 * PC13 - NC
 * PC14 - NC
 * PC15 - NC
 */
#define VAL_GPIOC_MODER             (PIN_MODE_ANALOG(0)    | \
                                     PIN_MODE_ANALOG(1)   | \
                                     PIN_MODE_ALTERNATE(2)   | \
                                     PIN_MODE_ALTERNATE(3) | \
                                     PIN_MODE_ANALOG(4) | \
                                     PIN_MODE_ANALOG(5) | \
                                     PIN_MODE_ALTERNATE(6) | \
                                     PIN_MODE_ALTERNATE(7) | \
                                     PIN_MODE_OUTPUT(8)   | \
                                     PIN_MODE_ALTERNATE(9)   | \
                                     PIN_MODE_ALTERNATE(10)   | \
                                     PIN_MODE_ALTERNATE(11)   | \
                                     PIN_MODE_OUTPUT(12) | \
                                     0 /* NC */ | \
                                     0 /* NC */ | \
                                     0 /* NC */)

#define VAL_GPIOC_OTYPER            (PIN_OTYPE_OPENDRAIN(0) | \
                                     PIN_OTYPE_OPENDRAIN(1) | \
                                     PIN_OTYPE_PUSHPULL(2) | \
                                     PIN_OTYPE_OPENDRAIN(3) | \
                                     PIN_OTYPE_OPENDRAIN(4) | \
                                     PIN_OTYPE_OPENDRAIN(5) | \
                                     PIN_OTYPE_PUSHPULL(6) | \
                                     PIN_OTYPE_PUSHPULL(7) | \
                                     PIN_OTYPE_PUSHPULL(8) | \
                                     PIN_OTYPE_OPENDRAIN(9) | \
                                     PIN_OTYPE_PUSHPULL(10) | \
                                     PIN_OTYPE_OPENDRAIN(11) | \
                                     PIN_OTYPE_PUSHPULL(12) | \
                                     0 /* NC */ | \
                                     0 /* NC */ | \
                                     0 /* NC */)

#define VAL_GPIOC_OSPEEDR           0xFFFFFFFF /* All pins 100MHz */

#define VAL_GPIOC_PUPDR             (PIN_PUDR_FLOATING(0)    | \
                                     PIN_PUDR_FLOATING(1)    | \
                                     PIN_PUDR_FLOATING(2)    |	\
                                     PIN_PUDR_FLOATING(3)    |	\
                                     PIN_PUDR_FLOATING(4)    | \
                                     PIN_PUDR_FLOATING(5)    | \
                                     PIN_PUDR_PULLDOWN(6)    | \
                                     PIN_PUDR_PULLDOWN(7)    | \
                                     PIN_PUDR_FLOATING(9)    | \
                                     PIN_PUDR_FLOATING(10)    |	\
                                     PIN_PUDR_PULLUP(11)    | \
                                     PIN_PUDR_FLOATING(12)    |	\
                                     0 /* NC */ | \
                                     0 /* NC */ | \
                                     0 /* NC */)


#define VAL_GPIOC_ODR               0x00000000 /* All pins low by default */

#define VAL_GPIOC_AFRL              (PIN_AFIO_AF5(2) | \
                                     PIN_AFIO_AF5(3) | \
                                     PIN_AFIO_AF3(6) | \
                                     PIN_AFIO_AF3(7))
#define VAL_GPIOC_AFRH              (PIN_AFIO_AF4(9) | \
                                     PIN_AFIO_AF7(10) | \
                                     PIN_AFIO_AF7(11))


/*
 * Port D setup.
 * PD2  - GPIO3, GPIO, push-pull
 */
#define VAL_GPIOD_MODER             (PIN_MODE_OUTPUT(2))
#define VAL_GPIOD_OTYPER            (PIN_OTYPE_PUSHPULL(2))
#define VAL_GPIOD_OSPEEDR           0xFFFFFFFF /* All pins 100MHz */
#define VAL_GPIOD_PUPDR             (PIN_PUDR_FLOATING(2))
#define VAL_GPIOD_ODR               0x00000000 /* All pins low by default */
#define VAL_GPIOD_AFRL              0x0
#define VAL_GPIOD_AFRH              0x0

/*
 * Port E setup.
 * No pins in F205 nor F405 for port E
 */
#define VAL_GPIOE_MODER             0x0
#define VAL_GPIOE_OTYPER            0x0
#define VAL_GPIOE_OSPEEDR           0x0
#define VAL_GPIOE_PUPDR             0x0
#define VAL_GPIOE_ODR               0x0
#define VAL_GPIOE_AFRL              0x0
#define VAL_GPIOE_AFRH              0x0

/*
 * Port F setup.
 * No pins in F205 nor F405 for port F
 */
#define VAL_GPIOF_MODER             0x0
#define VAL_GPIOF_OTYPER            0x0
#define VAL_GPIOF_OSPEEDR           0x0
#define VAL_GPIOF_PUPDR             0x0
#define VAL_GPIOF_ODR               0x0
#define VAL_GPIOF_AFRL              0x0
#define VAL_GPIOF_AFRH              0x0

/*
 * Port G setup.
 * No pins in F205 nor F405 for port G
 */
#define VAL_GPIOG_MODER             0x0
#define VAL_GPIOG_OTYPER            0x0
#define VAL_GPIOG_OSPEEDR           0x0
#define VAL_GPIOG_PUPDR             0x0
#define VAL_GPIOG_ODR               0x0
#define VAL_GPIOG_AFRL              0x0
#define VAL_GPIOG_AFRH              0x0

/*
 * Port H setup.
 * PH0  - OSC IN, CHECKME: input floating
 * PH1  - OSC OUT, CHECKME: input floating
 */
#define VAL_GPIOH_MODER             (PIN_MODE_INPUT(0) | \
                                     PIN_MODE_INPUT(1))
#define VAL_GPIOH_OTYPER            (PIN_OTYPE_OPENDRAIN(0) | \
                                     PIN_OTYPE_OPENDRAIN(1))
#define VAL_GPIOH_OSPEEDR           0xFFFFFFFF /* All pins 100MHz */
#define VAL_GPIOH_PUPDR             (PIN_PUDR_FLOATING(0) | \
                                     PIN_PUDR_FLOATING(1))
#define VAL_GPIOH_ODR               0x00000000 /* All pins low by default */
#define VAL_GPIOH_AFRL              0x0
#define VAL_GPIOH_AFRH              0x0

/*
 * Port I setup.
 * No pins in F205 nor F405 for port I
 */
#define VAL_GPIOI_MODER             0x0
#define VAL_GPIOI_OTYPER            0x0
#define VAL_GPIOI_OSPEEDR           0x0
#define VAL_GPIOI_PUPDR             0x0
#define VAL_GPIOI_ODR               0x0
#define VAL_GPIOI_AFRL              0x0
#define VAL_GPIOI_AFRH              0x0



#if !defined(_FROM_ASM_)
#ifdef __cplusplus
extern "C" {
#endif
  void boardInit(void);
#ifdef __cplusplus
}
#endif
#endif /* _FROM_ASM_ */

#endif /* _BOARD_H_ */
