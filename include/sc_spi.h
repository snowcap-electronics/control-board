/***
 * SPI functions
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

#ifndef SC_SPI_H
#define SC_SPI_H

#include "sc_utils.h"

#if HAL_USE_SPI

void sc_spi_init(void);
void sc_spi_deinit(void);
int8_t sc_spi_register(SPIDriver *spip,
                       ioportid_t cs_port,
                       uint8_t cs_pin,
                       uint32_t cr1);
void sc_spi_unregister(uint8_t spin);
void sc_spi_select(uint8_t spin);
void sc_spi_deselect(uint8_t spin);
void sc_spi_exchange(uint8_t spin, uint8_t *tx, uint8_t *rx, size_t bytes);
void sc_spi_send(uint8_t spin, uint8_t *data, size_t bytes);
void sc_spi_receive(uint8_t spin, uint8_t *data, size_t bytes);

#endif /* HAL_USE_SPI */

#endif

/* Emacs indentatation information
   Local Variables:
   indent-tabs-mode:nil
   tab-width:2
   c-basic-offset:2
   End:
*/
