/***
 * TI TMP275 temperature sensor support
 *
 * Copyright 2012 Kalle Vahlman, <kalle.vahlman@snowcap.fi>
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

#include "sc_conf.h"

#ifndef SC_TMP275_H
#define SC_TMP275_H

/* Enable this driver by defining SC_USE_TMP275 in sc_conf.h */
#ifdef SC_USE_TMP275

#ifndef TMP275_ADDRESS
#define TMP275_ADDRESS 0b1001111
#endif

void tmp275_enable(i2caddr_t addr);
void tmp275_disable(i2caddr_t addr);
int16_t tmp275_read(i2caddr_t addr);

#endif /* SC_USE_TMP275 */

#endif /* SC_TMP275_H */
