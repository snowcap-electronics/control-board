/***
 * I2S functions
 *
 * Copyright 2011-2015 Tuomas Kulve, <tuomas.kulve@snowcap.fi>
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
#include "sc_i2s.h"
#include "sc_event.h"

#if HAL_USE_I2S

#ifndef SC_I2S_BUF_SIZE
#define SC_I2S_BUF_SIZE            256
#endif

static uint16_t i2s_rx_buf[SC_I2S_BUF_SIZE];
static size_t last_offset = 0;
static size_t last_len = 0;

static void i2scallback(I2SDriver *i2sp, size_t offset, size_t len);

/*
 * Clocks from TRM 6.4.23:
 * f(VCO clock) = f(PLLI2S clock input) × (PLLI2SN / PLLM)
 * f(PLL I2S clock output) = f(VCO clock) / PLLI2SR
 *
 * From mcuconf:
 * PLLM    = 8
 * PLLI2SN = 192
 * PLLI2SR = 2
 *
 */

static const I2SConfig i2scfg = {
  NULL,
  i2s_rx_buf,
  SC_I2S_BUF_SIZE,
  i2scallback,
  // SPI_I2SCFGR configuration register
  // * I2SSTD (bits 4-5): 00: Philips standard
  // * CKPOL  (bits   3):  1: clock steady state is high level
  // * DATLEN (bits 1-2): 00: 16 bit data length
  // * CHLEN  (bits   0):  0: 16 bit wide
  SPI_I2SCFGR_CKPOL,
  // SPI_I2SPR I2S prescaler register
  // * ODD    (bits   8): Odd factor for the prescaler
  // * I2SDIV (bits 0-7): I2S Linear prescaler
  // FS = I2SxCLK / [16*((2*I2SDIV)+ODD)*8)] when the channel frame is 16-bit wide
  SPI_I2SPR_ODD | 48
};

static void i2scallback(I2SDriver *i2sp, size_t offset, size_t len)
{
  (void)i2sp;

  msg_t drdy;

  chSysLockFromISR();
  last_offset = offset;
  last_len = len;
  chSysUnlockFromISR();

  drdy = sc_event_msg_create_type(SC_EVENT_TYPE_AUDIO_AVAILABLE);
  sc_event_msg_post(drdy, SC_EVENT_MSG_POST_FROM_ISR);
}



void sc_i2s_init(void)
{
  // Nothing to do here.
}



void sc_i2s_deinit(void)
{
  // Nothing to do here.
}



void sc_i2s_start(void)
{
  i2sStart(&I2SDX, &i2scfg);

  palSetPadMode(SC_I2S_CLK_PORT,
                SC_I2S_CLK_PIN,
                PAL_MODE_ALTERNATE(SC_I2S_CLK_AF));
  palSetPadMode(SC_I2S_DATA_PORT,
                SC_I2S_DATA_PIN,
                PAL_MODE_ALTERNATE(SC_I2S_DATA_AF));

  i2sStartExchange(&I2SDX);
}



void sc_i2s_stop(void)
{
  i2sStopExchange(&I2SDX);
}



uint16_t *sc_i2s_get_data(size_t *len)
{
  uint16_t *data;

  // This data gets overwritten quickly
  chSysLock();
  *len = last_len;
  data = &i2s_rx_buf[last_offset];
  chSysUnlock();

  return data;
}

#endif // HAL_USE_I2S



/* Emacs indentatation information
   Local Variables:
   indent-tabs-mode:nil
   tab-width:2
   c-basic-offset:2
   End:
*/
