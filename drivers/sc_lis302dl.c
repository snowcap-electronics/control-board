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
#include "sc.h"
#include "sc_lis302dl.h"
#include "sc_spi.h"
#include "sc_extint.h"
#include <string.h>    // memset

#ifdef SC_HAS_LIS302DL

// Speed 5.25MHz, CPHA=1, CPOL=1, 8bits frames, MSb transmitted first.
#define LIS302DL_SPI_CR1       (SPI_CR1_BR_0 | SPI_CR1_BR_1 | SPI_CR1_CPOL | SPI_CR1_CPHA)

/* LIS302DL registers used in this file */
#define LIS302DL_CTRL_REG1   0x20
#define LIS302DL_CTRL_REG2   0x21
#define LIS302DL_CTRL_REG3   0x22
#define LIS302DL_STATUS_REG  0x27
#define LIS302DL_OUTX        0x29
#define LIS302DL_OUTY        0x2B
#define LIS302DL_OUTZ        0x2D

#define LIS302DL_READ_BIT    0x80

// By default lis302dl sensitivity is 18 mg/bit
#define LIS302DL_SENSIVITY  0.018

static uint8_t spin;
static binary_semaphore_t lis302dl_drdy_sem;

static void lis302dl_drdy_cb(EXTDriver *extp, expchannel_t channel)
{
  (void)extp;
  (void)channel;

  chSysLockFromISR();
  chBSemSignalI(&lis302dl_drdy_sem);
  chSysUnlockFromISR();
}

// FIXME: add a flag parameter for interrupts, double click recognition, etc?
void sc_lis302dl_init(void)
{
  uint8_t txbuf[2];
  int8_t spi_n;

  chBSemObjectInit(&lis302dl_drdy_sem, TRUE);

  // Register data ready interrupt
  sc_extint_set_isr_cb(SC_LIS302DL_INT_DRDY_PORT,
                       SC_LIS302DL_INT_DRDY_PIN,
                       SC_EXTINT_EDGE_RISING,
                       lis302dl_drdy_cb);

  spi_n = sc_spi_register(&SC_LIS302DL_SPIN,
                          SC_LIS302DL_CS_PORT,
                          SC_LIS302DL_CS_PIN,
                          LIS302DL_SPI_CR1);

  spin = (uint8_t)spi_n;

  sc_spi_select(spin);

  // Enable data ready interrupt
  txbuf[0] = LIS302DL_CTRL_REG3;
  txbuf[1] = 0x20; // INT2 on data ready
  sc_spi_send(spin, txbuf, sizeof(txbuf));

  // Enable measurements
  txbuf[0] = LIS302DL_CTRL_REG1;
  txbuf[1] = 0x47; // XEN + YEN + ZEN + PD (1 == enable active mode)
  sc_spi_send(spin, txbuf, sizeof(txbuf));

  sc_spi_deselect(spin);
}



uint8_t sc_lis302dl_read(sc_float *acc)
{
  uint8_t txbuf[2];
  uint8_t rxbuf[2];

  // Wait for data ready signal
  chBSemWait(&lis302dl_drdy_sem);

  sc_spi_select(spin);

  txbuf[0] = LIS302DL_READ_BIT | LIS302DL_OUTX;
  txbuf[1] = 0xff;
  sc_spi_exchange(spin, txbuf, rxbuf, sizeof(rxbuf));
  acc[0] = rxbuf[1] * LIS302DL_SENSIVITY;

  txbuf[0] = LIS302DL_READ_BIT | LIS302DL_OUTY;
  txbuf[1] = 0xff;
  sc_spi_exchange(spin, txbuf, rxbuf, sizeof(rxbuf));
  acc[1] = rxbuf[1] * LIS302DL_SENSIVITY;

  txbuf[0] = LIS302DL_READ_BIT | LIS302DL_OUTZ;
  txbuf[1] = 0xff;
  sc_spi_exchange(spin, txbuf, rxbuf, sizeof(rxbuf));
  acc[2] = rxbuf[1] * LIS302DL_SENSIVITY;

  sc_spi_deselect(spin);

  return SC_SENSOR_ACC;
}



void sc_lis302dl_shutdown(void)
{
  uint8_t txbuf[2];

  sc_spi_select(spin);

  // Disable interrupts
  txbuf[0] = LIS302DL_CTRL_REG3;
  txbuf[1] = 0x0; // No interrupts
  sc_spi_send(spin, txbuf, sizeof(txbuf));

  txbuf[0] = LIS302DL_CTRL_REG1;
  txbuf[1] = 0x0; // PD (0 == power down mode)
  sc_spi_send(spin, txbuf, sizeof(txbuf));

  sc_spi_deselect(spin);

  sc_spi_unregister(spin);
}

#endif

/* Emacs indentatation information
   Local Variables:
   indent-tabs-mode:nil
   tab-width:2
   c-basic-offset:2
   End:
*/
