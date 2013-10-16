/***
 * ST LIS302DL 3D gyroscope driver
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
#include "sc_lis302dl.h"
#include "sc_spi.h"
#include "sc_extint.h"
#include <string.h>    // memset

#ifdef SC_USE_LIS302DL

/* LIS302DL registers used in this file */
#define LIS302DL_CTRL_REG1   0x20
#define LIS302DL_CTRL_REG2   0x21
#define LIS302DL_CTRL_REG3   0x22
#define LIS302DL_STATUS_REG  0x27
#define LIS302DL_OUTX        0x29
#define LIS302DL_OUTY        0x2B
#define LIS302DL_OUTZ        0x2D

#define LIS302DL_READ_BIT    0x80

static uint8_t spin;
static BinarySemaphore lis302dl_drdy_sem;

static void lis302dl_drdy_cb(EXTDriver *extp, expchannel_t channel)
{
  (void)extp;
  (void)channel;

  chSysLockFromIsr();
  chBSemSignalI(&lis302dl_drdy_sem);
  chSysUnlockFromIsr();
}

// FIXME: add a flag parameter for interrupts, double click recognition, etc?
void sc_lis302dl_init(void)
{
  uint8_t txbuf[2];
  int8_t spi_n;

  chBSemInit(&lis302dl_drdy_sem, FALSE);

  // Register data ready interrupt
  sc_extint_set_isr_cb(SC_LIS302DL_INT_DRDY_PORT,
                       SC_LIS302DL_INT_DRDY_PIN,
                       SC_EXTINT_EDGE_RISING,
                       lis302dl_drdy_cb);

  spi_n = sc_spi_init(&SC_LIS302DL_SPIN,
                      SC_LIS302DL_CS_PORT,
                      SC_LIS302DL_CS_PIN);

  spin = (uint8_t)spi_n;

  // Enable data ready interrupt
  txbuf[0] = LIS302DL_CTRL_REG3;
  txbuf[1] = 0x20; // INT2 on data ready
  sc_spi_send(spin, txbuf, sizeof(txbuf));

  // Enable measurements
  txbuf[0] = LIS302DL_CTRL_REG1;
  txbuf[1] = 0x47; // XEN + YEN + ZEN + PD (1 == enable active mode)
  sc_spi_send(spin, txbuf, sizeof(txbuf));
}



void sc_lis302dl_read(int16_t *acc)
{
  uint8_t txbuf[2];
  uint8_t rxbuf[2];

  // Wait for data ready signal
  chBSemWait(&lis302dl_drdy_sem);
  //chThdSleepMilliseconds(1000);

  // FIXME: why 16 bits instead of just 8?

  txbuf[0] = LIS302DL_READ_BIT | LIS302DL_OUTX;
  txbuf[1] = 0xff;
  sc_spi_exchange(spin, txbuf, rxbuf, sizeof(rxbuf));
  acc[0] = rxbuf[1];

  txbuf[0] = LIS302DL_READ_BIT | LIS302DL_OUTY;
  txbuf[1] = 0xff;
  sc_spi_exchange(spin, txbuf, rxbuf, sizeof(rxbuf));
  acc[1] = rxbuf[1];

  txbuf[0] = LIS302DL_READ_BIT | LIS302DL_OUTZ;
  txbuf[1] = 0xff;
  sc_spi_exchange(spin, txbuf, rxbuf, sizeof(rxbuf));
  acc[2] = rxbuf[1];
}



void sc_lis302dl_shutdown(void)
{
  uint8_t txbuf[2];

  // Disable interrupts
  txbuf[0] = LIS302DL_CTRL_REG3;
  txbuf[1] = 0x0; // No interrupts
  sc_spi_send(spin, txbuf, sizeof(txbuf));

  txbuf[0] = LIS302DL_CTRL_REG1;
  txbuf[1] = 0x0; // PD (0 == power down mode)
  sc_spi_send(spin, txbuf, sizeof(txbuf));

  sc_spi_stop(spin);
}

#endif

/* Emacs indentatation information
   Local Variables:
   indent-tabs-mode:nil
   tab-width:2
   c-basic-offset:2
   End:
*/
