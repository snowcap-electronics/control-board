/***
 * I²C functions
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

#ifndef SC_I2C_H
#define SC_I2C_H

#include "sc_utils.h"

#if HAL_USE_I2C

void sc_i2c_init(uint32_t clock_speed);
void sc_i2c_read(i2caddr_t addr, uint8_t *data, size_t bytes);
void sc_i2c_transmit(i2caddr_t addr,
                     uint8_t *tx, size_t tx_bytes,
                     uint8_t *rx, size_t rx_bytes);
#define sc_i2c_write(addr, data, bytes) sc_i2c_transmit(addr, data, bytes, NULL, 0);

#endif /* HAL_USE_I2C */

#endif
