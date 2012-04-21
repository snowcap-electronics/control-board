/***
 * Temperature API
 *
 * Copyright 2011 Kalle Vahlman, <kalle.vahlman@snowcap.fi>
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


// ChibiOS includes
#include "ch.h"
#include "hal.h"

#include "sc_conf.h"
#include "sc_temperature.h"

#ifdef SC_USE_TMP275
#include "drivers/tmp275.h"
#endif

static uint16_t interval = 1000;

void temperature_enable(uint16_t interval_ms)
{
  interval = interval_ms;
#ifdef SC_USE_TMP275
  tmp275_enable(TMP275_ADDRESS);
#endif
}

void temperature_disable(void)
{
#ifdef SC_USE_TMP275
  tmp275_disable(TMP275_ADDRESS);
#endif
}

uint16_t temperature_interval(void)
{
  return interval;
}
