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

#ifdef SC_USE_FILTER_ZERO_CALIBRATE
typedef struct {
  sc_float offset;
  double sum;
  uint16_t samples;
  uint16_t samples_total;
  uint8_t initialised;
} sc_filter_zero_calibrate_state;
#endif



#ifdef SC_USE_FILTER_BROWN_LINEAR_EXPO
typedef struct {
  sc_float s1;
  sc_float s2;
  sc_float a;
  uint8_t initialised;
} sc_filter_brown_linear_expo_state;
#endif



#ifdef SC_USE_FILTER_PDM_FIR
#define PDM_FTL_TAPS 16
typedef struct sc_filter_pdm_fir {
  uint16_t buffer[PDM_FTL_TAPS];
  int next_tap;
} sc_filter_pdm_fir_state;
#endif



#ifdef SC_USE_FILTER_ZERO_CALIBRATE
/*
 * Count average offset during first samples and then remove if from subsequent data
 */
void sc_filter_zero_calibrate_init(sc_filter_zero_calibrate_state *state, uint16_t samples);
sc_float sc_filter_zero_calibrate(sc_filter_zero_calibrate_state *state, sc_float x);
#endif



#ifdef SC_USE_FILTER_BROWN_LINEAR_EXPO
/*
 * Brown's linear exponential smoothing
 *
 * From http://en.wikipedia.org/wiki/Exponential_smoothing
 *
 */
void sc_filter_brown_linear_expo_init(sc_filter_brown_linear_expo_state *state, sc_float factor);
sc_float sc_filter_brown_linear_expo(sc_filter_brown_linear_expo_state *state, sc_float x);
#endif



#ifdef SC_USE_FILTER_PDM_FIR
/* PDM FIR low pass filter.
 * The source frequency is expected to be 1024kHz so we are receiving 16 bit words at 64kHz rate MSB first.
 * The filter cutoff frequency is 3.6kHz. Filter parameters may be easily customized by modifying
 * the pdm_fir.py script and regenerating tables in pdm_fir_.h header.
 *
 * From https://github.com/piratfm/codec2_m4f/blob/636dccfaae9d2ef6171e41160bdd6dbe6a53afa6/lib/PDM_filter/pdm_fir.h
 * Originally from st.com's forum's "Open source PDM filter for MP45DT02 mic" thread
 *
 */
void sc_filter_pdm_fir_init(sc_filter_pdm_fir_state *state);
/* Put 16 bits of input PDM signal (MSB first) */
void sc_filter_pdm_fir_put(sc_filter_pdm_fir_state *state, uint16_t bits);
/* Retrieve output value. May be called at any rate since it does not change the filter state.
* The output ranges from -(2**(out_bits-1)) to +(2**(out_bits-1)). Those values correspond to
* all 0 or all 1 input signal. Note that the output value may still exceed this range so caller
* should truncate return value on its own if necessary.
*/
int sc_filter_pdm_fir_get(sc_filter_pdm_fir_state *state, uint8_t out_bits);
#endif


#endif



/* Emacs indentatation information
   Local Variables:
   indent-tabs-mode:nil
   tab-width:2
   c-basic-offset:2
   End:
*/
