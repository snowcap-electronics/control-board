/***
 * Filter algorithms
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

#ifndef SC_FILTER_H
#define SC_FILTER_H

#include <sc_utils.h>

typedef struct {
  sc_float offset;
  double sum;
  uint16_t samples;
  uint16_t samples_total;
  uint8_t initialised;
} sc_filter_zero_calibrate_state;

typedef struct {
  sc_float s1;
  sc_float s2;
  sc_float a;
  uint8_t initialised;
} sc_filter_brown_linear_expo_state;


void sc_filter_zero_calibrate_init(sc_filter_zero_calibrate_state *state, uint16_t samples);
sc_float sc_filter_zero_calibrate(sc_filter_zero_calibrate_state *state, sc_float x);

// From http://en.wikipedia.org/wiki/Exponential_smoothing
void sc_filter_brown_linear_expo_init(sc_filter_brown_linear_expo_state *state, sc_float factor);
sc_float sc_filter_brown_linear_expo(sc_filter_brown_linear_expo_state *state, sc_float x);

#endif



/* Emacs indentatation information
   Local Variables:
   indent-tabs-mode:nil
   tab-width:2
   c-basic-offset:2
   End:
*/
