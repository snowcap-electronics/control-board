/*
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

#ifndef SC_H
#define SC_H

/* ChibiOS includes */
#include "hal.h"

/* Project includes */
#include "sc_utils.h"
#include "sc_event.h"
#include "sc_uart.h"
#include "sc_pwm.h"
#include "sc_icu.h"
#include "sc_i2c.h"
#include "sc_sdu.h"
#include "sc_cmd.h"
#include "sc_adc.h"
#include "sc_gpio.h"
#include "sc_spi.h"
#include "sc_9dof.h"
#include "sc_extint.h"
#include "sc_led.h"
#include "sc_log.h"
#include "sc_radio.h"
#include "sc_pwr.h"
#include "sc_hid.h"

#define SC_MODULE_UART1       (0x1 <<  1)
#define SC_MODULE_UART2       (0x1 <<  2)
#define SC_MODULE_UART3       (0x1 <<  3)
#define SC_MODULE_UART4       (0x1 <<  4)
#define SC_MODULE_PWM         (0x1 <<  5)
#define SC_MODULE_ICU         (0x1 <<  6)
#define SC_MODULE_I2C         (0x1 <<  7)
#define SC_MODULE_SDU         (0x1 <<  8)
#define SC_MODULE_ADC         (0x1 <<  9)
#define SC_MODULE_GPIO        (0x1 << 10)
#define SC_MODULE_LED         (0x1 << 11)
#define SC_MODULE_RADIO       (0x1 << 12)
#define SC_MODULE_HID         (0x1 << 13)

void sc_init(uint32_t subsystems);
void sc_deinit(uint32_t subsystems);

#endif
