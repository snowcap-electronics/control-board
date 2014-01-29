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
#include "ch.h"
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

#define SC_INIT_UART1       0x0001
#define SC_INIT_UART2       0x0002
#define SC_INIT_UART3       0x0004
#define SC_INIT_UART4       0x0008
#define SC_INIT_PWM         0x0010
#define SC_INIT_ICU         0x0020
#define SC_INIT_I2C         0x0040
#define SC_INIT_SDU         0x0080
#define SC_INIT_ADC         0x0100
#define SC_INIT_GPIO        0x0200
#define SC_INIT_LED         0x0400
#define SC_INIT_RADIO       0x0800

void sc_init(uint32_t subsystems);

#endif
