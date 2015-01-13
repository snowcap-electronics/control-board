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


#include "sc_utils.h"
#include "sc_spi.h"

#if HAL_USE_SPI

#define SC_SPI_MAX_CLIENTS       8
// Speed 5.25MHz, CPHA=1, CPOL=1, 8bits frames, MSb transmitted first.
#define SC_SPI_DEFAULT_CR1       SPI_CR1_BR_0 | SPI_CR1_BR_1 | SPI_CR1_CPOL | SPI_CR1_CPHA;

typedef struct sc_spi_conf {
  SPIDriver *spip;
  ioportid_t cs_port;
  uint16_t cs_pin;
  uint16_t cr1;
} sc_spi_conf;

static sc_spi_conf spi_conf[SC_SPI_MAX_CLIENTS];


int8_t sc_spi_init(SPIDriver *spip, ioportid_t cs_port, uint8_t cs_pin)
{
  int8_t i;
  chDbgAssert(spip != NULL, "SPI driver null");

  /* Find empty slot for new SPI client */
  for (i = 0; i < SC_SPI_MAX_CLIENTS; ++i) {
    if (spi_conf[i].spip == NULL) {
      spi_conf[i].spip = spip;
      spi_conf[i].cs_port = cs_port;
      spi_conf[i].cs_pin = cs_pin;
      spi_conf[i].cr1 = SC_SPI_DEFAULT_CR1;
      return i;
    }
  }

  chDbgAssert(0, "Too many SPI clients");

  return -1;
}

void sc_spi_stop(uint8_t spin)
{
  chDbgAssert(spin < SC_SPI_MAX_CLIENTS, "SPI n outside range");
  chDbgAssert(spi_conf[spin].spip != NULL, "SPI n not initialized");

  spiStop(spi_conf[spin].spip);
  spi_conf[spin].spip = NULL;
}

void sc_spi_exchange(uint8_t spin, uint8_t *tx, uint8_t *rx, size_t bytes)
{
  SPIConfig spi_cfg;

  chDbgAssert(spin < SC_SPI_MAX_CLIENTS, "SPI n outside range");
  chDbgAssert(spi_conf[spin].spip != NULL, "SPI n not initialized");

  /* spiStart always to set CS for every call according to SPI client. */
  spi_cfg.end_cb = NULL;
  spi_cfg.ssport = spi_conf[spin].cs_port;
  spi_cfg.sspad = spi_conf[spin].cs_pin;
  spi_cfg.cr1 = spi_conf[spin].cr1;

  spiStart(spi_conf[spin].spip, &spi_cfg);
  spiAcquireBus(spi_conf[spin].spip);
  spiSelect(spi_conf[spin].spip);

  spiExchange(spi_conf[spin].spip, bytes, tx, rx);

  spiUnselect(spi_conf[spin].spip);
  spiReleaseBus(spi_conf[spin].spip);
  spiStop(spi_conf[spin].spip);
}

void sc_spi_send(uint8_t spin, uint8_t *data, size_t bytes)
{
  SPIConfig spi_cfg;

  chDbgAssert(spin < SC_SPI_MAX_CLIENTS, "SPI n outside range");
  chDbgAssert(spi_conf[spin].spip != NULL, "SPI n not initialized");

  /* spiStart always to set CS per SPI client. */
  spi_cfg.end_cb = NULL;
  spi_cfg.ssport = spi_conf[spin].cs_port;
  spi_cfg.sspad = spi_conf[spin].cs_pin;
  spi_cfg.cr1 = spi_conf[spin].cr1;

  spiStart(spi_conf[spin].spip, &spi_cfg);
  spiAcquireBus(spi_conf[spin].spip);
  spiSelect(spi_conf[spin].spip);

  spiSend(spi_conf[spin].spip, bytes, data);

  spiUnselect(spi_conf[spin].spip);
  spiReleaseBus(spi_conf[spin].spip);
  spiStop(spi_conf[spin].spip);
}

#endif /* HAL_USE_SPI */

/* Emacs indentatation information
   Local Variables:
   indent-tabs-mode:nil
   tab-width:2
   c-basic-offset:2
   End:
*/
