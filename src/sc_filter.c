/***
 * Filter algorithms
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

#include "sc_utils.h"
#include "sc_filter.h"
#if SC_USE_FILTER_PDM_FIR
#include "sc_filter_pdm_fir_coeff.h"
#endif



#if SC_USE_FILTER_ZERO_CALIBRATE
void sc_filter_zero_calibrate_init(sc_filter_zero_calibrate_state *state, uint16_t samples)
{
  if (!state) {
    chDbgAssert(0, "Null state");
    return;
  }

  state->offset        = 0;
  state->sum           = 0;
  state->samples       = 0;
  state->samples_total = samples;
  state->initialised   = 0;
}



sc_float sc_filter_zero_calibrate(sc_filter_zero_calibrate_state *state, sc_float x)
{
  if (!state) {
    chDbgAssert(0, "Null state");
    return 0;
  }

  if (!state->initialised) {
    state->sum += x;

    if (++state->samples == state->samples_total) {
      // CHECKME: assuming here that sum fits float
      state->offset = (sc_float)(state->sum / (sc_float)(state->samples_total));
      state->initialised = 1;
    }
  }

  return x - state->offset;
}
#endif



#if SC_USE_FILTER_BROWN_LINEAR_EXPO
void sc_filter_brown_linear_expo_init(sc_filter_brown_linear_expo_state *state, sc_float factor)
{
  if (!state) {
    chDbgAssert(0, "Null state");
    return;
  }

  state->s1          = 0;
  state->s2          = 0;
  state->a           = factor;
  state->initialised = 0;
}



sc_float sc_filter_brown_linear_expo(sc_filter_brown_linear_expo_state *state, sc_float x)
{
  sc_float a;
  sc_float at, bt;

  if (!state) {
    chDbgAssert(0, "Null state");
    return 0;
  }

  if (!state->initialised) {
    state->s1 = x;
    state->s2 = x;
    state->initialised = 1;
    return x;
  }

  a = state->a;

  // From http://en.wikipedia.org/wiki/Exponential_smoothing
  state->s1 = (a *         x) + (1 - a) * state->s1;
  state->s2 = (a * state->s1) + (1 - a) * state->s2;

  at = 2 * state->s1 - state->s2;
  bt = (a / (1 - a)) * (state->s1 - state->s2);

  // FIXME: should we consider time delta here, in case we have missed a measurement?
  return at + bt;
}
#endif



#if SC_USE_FILTER_PDM_FIR
void sc_filter_pdm_fir_init(sc_filter_pdm_fir_state *state)
{
	uint8_t t;

  state->next_tap = 0;

  for (t = 0; t < PDM_FTL_TAPS; ++t) {
    state->buffer[t] = 0x5555;
  }
}



void sc_filter_pdm_fir_put(sc_filter_pdm_fir_state *state, uint16_t bits)
{
	state->buffer[state->next_tap] = bits;

  if (++state->next_tap >= PDM_FTL_TAPS) {
    state->next_tap = 0;
  }
}



int sc_filter_pdm_fir_get(sc_filter_pdm_fir_state *state, uint8_t out_bits)
{
  uint8_t t = state->next_tap;
  uint16_t i = 0;
	int tot = 0;

  while (1) {

    uint16_t v = state->buffer[t];

    tot += byte_coeff[i++][(uint8_t)(v >> 8)];
    tot += byte_coeff[i++][(uint8_t)(v)];

    if (++t >= PDM_FTL_TAPS) {
      t = 0;
    }
    if (t == state->next_tap) {
      break;
    }
  }

  /* Rescale to output range */
  return tot >> (PDM_FTL_SCALE_BITS - out_bits + 1);
}
#endif


/* Emacs indentatation information
   Local Variables:
   indent-tabs-mode:nil
   tab-width:2
   c-basic-offset:2
   End:
*/

