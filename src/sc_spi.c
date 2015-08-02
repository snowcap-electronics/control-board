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

static mutex_t data_mtx;

typedef struct sc_spi_conf {
  SPIDriver *spip;
  SPIConfig cfg;
  bool selected;
} sc_spi_conf;

static sc_spi_conf spi_conf[SC_SPI_MAX_CLIENTS];

void sc_spi_init(void)
{
  chMtxObjectInit(&data_mtx);

}

void sc_spi_deinit(void)
{
  chMtxLock(&data_mtx);
  for (uint8_t i = 0; i < SC_SPI_MAX_CLIENTS; ++i) {
    spi_conf[i].spip = NULL;
    spi_conf[i].selected = false;
  }
  chMtxUnlock(&data_mtx);
}


int8_t sc_spi_register(SPIDriver *spip, ioportid_t cs_port, uint8_t cs_pin, uint32_t cr1)
{
  int8_t i;
  chDbgAssert(spip != NULL, "SPI driver null");

  chMtxLock(&data_mtx);

  /* Find empty slot for new SPI client */
  for (i = 0; i < SC_SPI_MAX_CLIENTS; ++i) {
    if (spi_conf[i].spip == NULL) {
      spi_conf[i].spip = spip;
      spi_conf[i].cfg.end_cb = NULL;
      spi_conf[i].cfg.ssport = cs_port;
      spi_conf[i].cfg.sspad = cs_pin;
      spi_conf[i].cfg.cr1 = cr1;
      spi_conf[i].selected = false;
      chMtxUnlock(&data_mtx);
      return i;
    }
  }

  chMtxUnlock(&data_mtx);

  chDbgAssert(0, "Too many SPI clients");

  return -1;
}

void sc_spi_unregister(uint8_t spin)
{
  chDbgAssert(spin < SC_SPI_MAX_CLIENTS, "SPI n outside range");
  chDbgAssert(spi_conf[spin].spip != NULL, "SPI n not initialized");
  chDbgAssert(spi_conf[spin].selected, "SPI n still selected");

  chMtxLock(&data_mtx);
  spi_conf[spin].spip = NULL;
  chMtxUnlock(&data_mtx);

}

void sc_spi_select(uint8_t spin)
{
  chDbgAssert(spin < SC_SPI_MAX_CLIENTS, "SPI n outside range");
  chDbgAssert(spi_conf[spin].spip != NULL, "SPI n not initialized");

  spiAcquireBus(spi_conf[spin].spip);

  chDbgAssert(!spi_conf[spin].selected, "SPI n already selected");

  spiStart(spi_conf[spin].spip, &spi_conf[spin].cfg);
  spiSelect(spi_conf[spin].spip);

  spi_conf[spin].selected = true;
}

void sc_spi_deselect(uint8_t spin)
{
  chDbgAssert(spin < SC_SPI_MAX_CLIENTS, "SPI n outside range");
  chDbgAssert(spi_conf[spin].spip != NULL, "SPI n not initialized");

  chDbgAssert(spi_conf[spin].selected, "SPI n not selected");
  spi_conf[spin].selected = false;

  spiUnselect(spi_conf[spin].spip);
  spiStop(spi_conf[spin].spip);

  spiReleaseBus(spi_conf[spin].spip);
}

void sc_spi_exchange(uint8_t spin, uint8_t *tx, uint8_t *rx, size_t bytes)
{
  chDbgAssert(spin < SC_SPI_MAX_CLIENTS, "SPI n outside range");
  chDbgAssert(spi_conf[spin].spip != NULL, "SPI n not initialized");
  chDbgAssert(spi_conf[spin].selected, "SPI n not selected");

  spiExchange(spi_conf[spin].spip, bytes, tx, rx);
}

void sc_spi_send(uint8_t spin, uint8_t *data, size_t bytes)
{
  chDbgAssert(spin < SC_SPI_MAX_CLIENTS, "SPI n outside range");
  chDbgAssert(spi_conf[spin].spip != NULL, "SPI n not initialized");
  chDbgAssert(spi_conf[spin].selected, "SPI n not selected");

  spiSend(spi_conf[spin].spip, bytes, data);
}

void sc_spi_receive(uint8_t spin, uint8_t *data, size_t bytes)
{
  chDbgAssert(spin < SC_SPI_MAX_CLIENTS, "SPI n outside range");
  chDbgAssert(spi_conf[spin].spip != NULL, "SPI n not initialized");
  chDbgAssert(spi_conf[spin].selected, "SPI n not selected");

  spiReceive(spi_conf[spin].spip, bytes, data);
}

#endif /* HAL_USE_SPI */

/* Emacs indentatation information
   Local Variables:
   indent-tabs-mode:nil
   tab-width:2
   c-basic-offset:2
   End:
*/
