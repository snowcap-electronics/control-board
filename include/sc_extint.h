/***
 * External interrupt functions
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

#ifndef SC_EXTINT_H
#define SC_EXTINT_H

#include "sc_utils.h"

#if HAL_USE_EXT

typedef enum {
  SC_EXTINT_EDGE_RISING  = 0x1,
  SC_EXTINT_EDGE_FALLING = 0x2,
  SC_EXTINT_EDGE_BOTH    = 0x3
} SC_EXTINT_EDGE;



void sc_extint_set_event(ioportid_t port,
                         uint8_t pin,
                         SC_EXTINT_EDGE mode);
void sc_extint_set_debounced_event(ioportid_t port,
                                 uint8_t pin,
                                 SC_EXTINT_EDGE mode,
                                 systime_t delay);
void sc_extint_set_isr_cb(ioportid_t port,
                          uint8_t pin,
                          SC_EXTINT_EDGE mode,
                          extcallback_t cb);
void sc_extint_clear(ioportid_t port, uint8_t pin);

#endif /* HAL_USE_EXT */

#endif

/* Emacs indentatation information
   Local Variables:
   indent-tabs-mode:nil
   tab-width:2
   c-basic-offset:2
   End:
*/

