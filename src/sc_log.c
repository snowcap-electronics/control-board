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

#include "sc_utils.h"
#include "sc_log.h"

#include <limits.h>

#define SC_LOG_OUTPUT_NONE   0
#define SC_LOG_OUTPUT_UART   1

static uint8_t log_output = SC_LOG_OUTPUT_NONE;
static SC_UART output_uart;
static SC_LOG_LVL output_lvl = SC_LOG_LVL_DEBUG;

void sc_log(SC_LOG_LVL lvl, SC_LOG_MODULE module, uint8_t *msg)
{
  uint16_t len = 0;

  // FIXME: implement module based filtering
  (void)module;

  if (lvl > output_lvl) {
	// Too high log level, ignore the message
	return;
  }

  if (log_output != SC_LOG_OUTPUT_NONE) {
	while (len < USHRT_MAX) {
	  if (msg[len] == '\0') {
		break;
	  }
	  ++len;
	}
  }

  if (log_output == SC_LOG_OUTPUT_UART) {
	sc_uart_send_msg(output_uart, msg, len);
  }
}


void sc_log_output_uart(SC_UART uart)
{
  log_output = SC_LOG_OUTPUT_UART;
  output_uart = uart;
}


void sc_log_level(SC_LOG_LVL lvl)
{
  output_lvl = lvl;
}
