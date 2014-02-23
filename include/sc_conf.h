/***
 * Snowcap configurations
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

#ifndef SC_CONF_H
#define SC_CONF_H

#include "hal.h"

#ifndef SC_USE_9DOF
#define SC_USE_9DOF FALSE
#endif

#ifndef SC_USE_AHRS
#define SC_USE_AHRS FALSE
#endif

#ifndef sc_float
#define sc_float float
#endif

#if defined(BOARD_SNOWCAP_V1)
#include "sc_conf_snowcap_v1.h"
#elif defined(BOARD_SNOWCAP_STM32F4_V1)
#include "sc_conf_snowcap_stm32f4_v1.h"
#elif defined(BOARD_ST_STM32F4_DISCOVERY)
#include "sc_conf_f4_discovery.h"
#elif defined(BOARD_ST_STM32VL_DISCOVERY)
#include "sc_conf_f1_discovery.h"
#else
#error "Unknown board or board not defined."
#endif

#endif
