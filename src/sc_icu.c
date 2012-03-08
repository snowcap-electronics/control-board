/***
 * Input Capture Unit (ICU) functions
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

#include "sc_utils.h"

#if HAL_USE_ICU

#include "sc_icu.h"

static int last_width = -1;
static int last_period = -1;

static void icu_width_cb(ICUDriver *icup);
static void icu_period_cb(ICUDriver *icup);

static ICUConfig icucfg = {
  ICU_INPUT_ACTIVE_HIGH,
  10000,                                    /* 10 kHz ICU clock frequency */
  icu_width_cb,
  icu_period_cb,
  ICU_CHANNEL_1
};



/*
 * Initialize the ICU unit with given channel
 */
void sc_icu_init(int channel)
{
  // FIXME: ChibiOS supports only channel 1
  if (channel == 1) {
	icuStart(&ICUD4, &icucfg);
	icuEnable(&ICUD4);
  }
}



/*
 * Get last width (will be set to -1 on every read)
 */
int sc_icu_get_width(int channel)
{
  // FIXME: ChibiOS supports only channel 1
  int width = -1;

  if (channel == 1) {
	width = last_width;
	last_width = -1;
  }

  return width;
}



/*
 * Get last period (will be set to -1 on every read)
 */
int sc_icu_get_period(int channel)
{
  // FIXME: ChibiOS supports only channel 1
  int period = -1;

  if (channel == 1) {
	period = last_period;
	last_period = -1;
  }

  return period;
}



/*
 * Callback for width.
 */
static void icu_width_cb(ICUDriver *icup)
{
  last_width = icuGetWidth(icup);
}



/*
 * Callback for period.
 */
static void icu_period_cb(ICUDriver *icup)
{
  last_period = icuGetPeriod(icup);
}

#endif /* HAL_USE_ICU */
