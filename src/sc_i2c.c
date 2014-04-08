/***
 * I²C functions
 *
 * Copyright 2013 Kalle Vahlman, <kalle.vahlman@snowcap.fi>,
 *                Tuomas Kulve, <tuomas.kulve@snowcap.fi>
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
#include "sc_i2c.h"

#if HAL_USE_I2C

#define SC_I2C_MAX_CLIENTS       8

MUTEX_DECL(i2ccfg_mtx);

typedef struct sc_i2c_conf {
  I2CDriver *i2cp;
  i2caddr_t addr;
} sc_i2c_conf;

static sc_i2c_conf i2c_conf[SC_I2C_MAX_CLIENTS];

int8_t sc_i2c_init(I2CDriver *i2cp, i2caddr_t addr)
{
  int8_t i;
  I2CConfig i2c_cfg = {
    OPMODE_I2C,
    400000,
    FAST_DUTY_CYCLE_2
  };

  chDbgAssert(i2cp != NULL, "I2C driver null", "#1");

  chMtxLock(&i2ccfg_mtx);

  // Check if I2CDx already activated for existing client
  for (i = 0; i < SC_I2C_MAX_CLIENTS; ++i) {
    if (i2c_conf[i].i2cp == i2cp) {
      break;
    }
  }

  if (i == SC_I2C_MAX_CLIENTS) {
    i2cStart(i2cp, &i2c_cfg);
  }

  // Find empty slot for new I2C client
  for (i = 0; i < SC_I2C_MAX_CLIENTS; ++i) {
    if (i2c_conf[i].i2cp == NULL) {
      i2c_conf[i].i2cp = i2cp;
      i2c_conf[i].addr = addr;
      break;
    }
  }

  if (i == SC_I2C_MAX_CLIENTS) {
    chDbgAssert(0, "Too many I2C clients", "#1");
  }
  chMtxUnlock();

  return i;
}



void sc_i2c_stop(uint8_t i2cn)
{
  I2CDriver *i2cp;
  int i;

  chMtxLock(&i2ccfg_mtx);

  chDbgAssert(i2cn < SC_I2C_MAX_CLIENTS, "I2C n outside range", "#3");
  chDbgAssert(i2c_conf[i2cn].i2cp != NULL, "I2C n not initialized", "#2");

  i2cp = i2c_conf[i2cn].i2cp;

  i2c_conf[i2cn].i2cp = NULL;
  i2c_conf[i2cn].addr = 0;

  // Check if no clients are using I2CDx anymore
  for (i = 0; i < SC_I2C_MAX_CLIENTS; ++i) {
    if (i2c_conf[i].i2cp == i2cp) {
      break;
    }
  }

  if (i == SC_I2C_MAX_CLIENTS) {
    i2cStop(i2cp);
  }

  chMtxUnlock();
}


void sc_i2c_read(uint8_t i2cn, uint8_t *data, size_t bytes)
{
  msg_t ret;

  chDbgAssert(i2cn < SC_I2C_MAX_CLIENTS, "I2C n outside range", "#4");
  chDbgAssert(i2c_conf[i2cn].i2cp != NULL, "I2C n not initialized", "#3");

  i2cAcquireBus(i2c_conf[i2cn].i2cp);

  ret = i2cMasterReceive(i2c_conf[i2cn].i2cp, i2c_conf[i2cn].addr, data, bytes);
  chDbgAssert(ret == RDY_OK, "i2cMasterReceive error", "#1");

  i2cReleaseBus(i2c_conf[i2cn].i2cp);
}

void sc_i2c_transmit(uint8_t i2cn,
                     uint8_t *tx, size_t tx_bytes,
                     uint8_t *rx, size_t rx_bytes)
{
  msg_t ret;

  chDbgAssert(i2cn < SC_I2C_MAX_CLIENTS, "I2C n outside range", "#5");
  chDbgAssert(i2c_conf[i2cn].i2cp != NULL, "I2C n not initialized", "#4");

  i2cAcquireBus(i2c_conf[i2cn].i2cp);

  ret = i2cMasterTransmit(i2c_conf[i2cn].i2cp, i2c_conf[i2cn].addr, tx, tx_bytes, rx, rx_bytes);
  chDbgAssert(ret == RDY_OK, "i2cMasterTransmit error", "#1");

  i2cReleaseBus(i2c_conf[i2cn].i2cp);
}

#endif /* HAL_USE_I2C */

/* Emacs indentatation information
   Local Variables:
   indent-tabs-mode:nil
   tab-width:2
   c-basic-offset:2
   End:
*/
