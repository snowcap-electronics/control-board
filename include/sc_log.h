/***
 * Logging functions
 *
 * Copyright 2014 Tuomas Kulve, <tuomas.kulve@snowcap.fi>
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

#ifndef SC_LOG_H
#define SC_LOG_H

#include "sc_uart.h"

typedef enum SC_LOG_LVL {
  SC_LOG_LVL_VERBOSE   = 1,
  SC_LOG_LVL_DEBUG     = 2
} SC_LOG_LVL;

typedef enum SC_LOG_MODULE {
  SC_LOG_MODULE_UNSPECIFIED  = 0,
  SC_LOG_MODULE_ANY          = 1,
  SC_LOG_MODULE_RADIO        = 2
} SC_LOG_MODULE;

// Conveniency macros
#define SC_LOG(msg) sc_log(SC_LOG_LVL_VERBOSE, SC_LOG_MODULE_TAG, (uint8_t *)(msg))
#define SC_DBG(msg) sc_log(SC_LOG_LVL_DEBUG, SC_LOG_MODULE_TAG, (uint8_t *)(msg))

void sc_log(SC_LOG_LVL lvl, SC_LOG_MODULE module, uint8_t *msg);
#if HAL_USE_UART
void sc_log_output_uart(SC_UART uart);
#endif
void sc_log_level(SC_LOG_LVL lvl);


#endif
