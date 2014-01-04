/***
 * ST LSM9DS0 9DoF driver
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
#include "sc_lsm9ds0.h"
#include "sc_i2c.h"
#include "sc_extint.h"
#include <string.h>    // memset

#ifdef SC_HAS_LSM9DS0

/* LSM9DS0 registers used in this file */
#define LSM9DS0_CTRL_REG1_G        0x20
#define LSM9DS0_CTRL_REG3_G        0x22
#define LSM9DS0_CTRL_REG5_G        0x24
#define LSM9DS0_OUT_X_L_G          0x28
#define LSM9DS0_OUT_X_H_G          0x29
#define LSM9DS0_OUT_Y_L_G          0x2A
#define LSM9DS0_OUT_Y_H_G          0x2B
#define LSM9DS0_OUT_Z_L_G          0x2C
#define LSM9DS0_OUT_Z_H_G          0x2D
#define LSM9DS0_OUT_X_L_M          0x08
#define LSM9DS0_OUT_X_H_M          0x09
#define LSM9DS0_OUT_Y_L_M          0x0A
#define LSM9DS0_OUT_Y_H_M          0x0B
#define LSM9DS0_OUT_Z_L_M          0x0C
#define LSM9DS0_OUT_Z_H_M          0x0D

#define LSM9DS0_CTRL_REG0_XM       0x1F
#define LSM9DS0_CTRL_REG1_XM       0x20
#define LSM9DS0_CTRL_REG3_XM       0x22
#define LSM9DS0_CTRL_REG4_XM       0x23
#define LSM9DS0_CTRL_REG5_XM       0x24
#define LSM9DS0_CTRL_REG6_XM       0x25
#define LSM9DS0_CTRL_REG7_XM       0x26
#define LSM9DS0_OUT_X_L_A          0x28
#define LSM9DS0_OUT_X_H_A          0x29
#define LSM9DS0_OUT_Y_L_A          0x2A
#define LSM9DS0_OUT_Y_H_A          0x2B
#define LSM9DS0_OUT_Z_L_A          0x2C
#define LSM9DS0_OUT_Z_H_A          0x2D

#define LSM9DS0_SUBADDR_AUTO_INC_BIT  0x80

static uint8_t i2cn_xm;
static uint8_t i2cn_g;
static BinarySemaphore lsm9ds0_drdy_sem;
static Mutex data_mtx;
static uint8_t sensors_ready;

#define SENSOR_RDY_ACC   0x1
#define SENSOR_RDY_MAGN  0x2
#define SENSOR_RDY_GYRO  0x4

static void lsm9ds0_drdy_cb(EXTDriver *extp, expchannel_t channel)
{
  uint8_t rdy = 0;

  (void)extp;

  switch(channel) {
  case SC_LSM9DS0_INT1_XM_PIN:
	  rdy = SENSOR_RDY_ACC;
    break;
  case SC_LSM9DS0_INT2_XM_PIN:
	  rdy = SENSOR_RDY_MAGN;
    break;
  case SC_LSM9DS0_DRDY_G_PIN:
	  rdy = SENSOR_RDY_GYRO;
    break;
  default:
    chDbgAssert(0, "Invalid channel", "#1");
    return;
  }

  chSysLockFromIsr();
  // No need to lock mutex inside chSysLockFromIsr()
  sensors_ready |= rdy;

  chBSemSignalI(&lsm9ds0_drdy_sem);

  chSysUnlockFromIsr();
}



void sc_lsm9ds0_init(void)
{
  uint8_t txbuf[2];
  int8_t i2c_n;

  sensors_ready = 0;
  chMtxInit(&data_mtx);
  chBSemInit(&lsm9ds0_drdy_sem, FALSE);

  // Pinmux
  palSetPadMode(SC_LSM9DS0_INT1_XM_PORT,
                SC_LSM9DS0_INT1_XM_PIN,
                PAL_MODE_INPUT);
  palSetPadMode(SC_LSM9DS0_INT2_XM_PORT,
                SC_LSM9DS0_INT2_XM_PIN,
                PAL_MODE_INPUT);
  palSetPadMode(SC_LSM9DS0_DRDY_G_PORT,
                SC_LSM9DS0_DRDY_G_PIN,
                PAL_MODE_INPUT);
  palSetPadMode(SC_LSM9DS0_I2CN_SDA_PORT,
                SC_LSM9DS0_I2CN_SDA_PIN,
                PAL_MODE_ALTERNATE(SC_LSM9DS0_I2CN_SDA_AF));
  palSetPadMode(SC_LSM9DS0_I2CN_SCL_PORT,
                SC_LSM9DS0_I2CN_SCL_PIN,
                PAL_MODE_ALTERNATE(SC_LSM9DS0_I2CN_SDA_AF));

  // Register data ready interrupts
  sc_extint_set_isr_cb(SC_LSM9DS0_INT1_XM_PORT,
                       SC_LSM9DS0_INT1_XM_PIN,
                       SC_EXTINT_EDGE_RISING,
                       lsm9ds0_drdy_cb);

  sc_extint_set_isr_cb(SC_LSM9DS0_INT2_XM_PORT,
                       SC_LSM9DS0_INT2_XM_PIN,
                       SC_EXTINT_EDGE_RISING,
                       lsm9ds0_drdy_cb);

  sc_extint_set_isr_cb(SC_LSM9DS0_DRDY_G_PORT,
                       SC_LSM9DS0_DRDY_G_PIN,
                       SC_EXTINT_EDGE_RISING,
                       lsm9ds0_drdy_cb);

  // Initialize two I2C instances, one for acc+magn, one for gyro
  i2c_n = sc_i2c_init(&SC_LSM9DS0_I2CN, SC_LSM9DS0_ADDR_XM);
  i2cn_xm = (uint8_t)i2c_n;

  i2c_n = sc_i2c_init(&SC_LSM9DS0_I2CN, SC_LSM9DS0_ADDR_G);
  i2cn_g = (uint8_t)i2c_n;

  // Setup and start acc+magn

  // Shutdown the chip so that we get new data ready
  // interrupts even if the chip was already running
  sc_lsm9ds0_shutdown();

  // Configure INT1_XM as acc data ready
  txbuf[0] = LSM9DS0_CTRL_REG3_XM;
  txbuf[1] = 0x04; // P1_DRDYA
  sc_i2c_write(i2cn_xm, txbuf, sizeof(txbuf));

  // Configure INT2_XM as magn data ready
  txbuf[0] = LSM9DS0_CTRL_REG4_XM;
  txbuf[1] = 0x04; // P2_DRDYM
  sc_i2c_write(i2cn_xm, txbuf, sizeof(txbuf));

  // Enable 100Hz acc
  txbuf[0] = LSM9DS0_CTRL_REG1_XM;
  txbuf[1] = 0x67; // XEN + YEN + ZEN + 100Hz
  sc_i2c_write(i2cn_xm, txbuf, sizeof(txbuf));

  // Configure 100Hz magn (and temperature)
  txbuf[0] = LSM9DS0_CTRL_REG5_XM;
  txbuf[1] = 0xF4; // High res, 100Hz, temp
  sc_i2c_write(i2cn_xm, txbuf, sizeof(txbuf));

  // Enable magn in continuous conversion mode
  txbuf[0] = LSM9DS0_CTRL_REG7_XM;
  txbuf[1] = 0x00; // Continuous conversion mode
  sc_i2c_write(i2cn_xm, txbuf, sizeof(txbuf));

  // Setup and start gyro

  // Enable gyro data ready interrupt
  txbuf[0] = LSM9DS0_CTRL_REG3_G;
  txbuf[1] = 0x08; //  I2_DRDY, gyroscope data ready
  sc_i2c_write(i2cn_g, txbuf, sizeof(txbuf));

  // Enable gyroscope (95Hz with 25 cutoff (FIXME: what?))
  txbuf[0] = LSM9DS0_CTRL_REG1_G;
  txbuf[1] = 0x3F; // XEN + YEN + ZEN + PD + BW
  sc_i2c_write(i2cn_g, txbuf, sizeof(txbuf));
}



void sc_lsm9ds0_read(int16_t *acc, int16_t *magn, int16_t *gyro)
{
  uint8_t txbuf[1];
  uint8_t rxbuf[6];
  uint8_t sensors_done = 0;
  uint8_t sensors_ready_local = 0;
  uint8_t i;
  const uint8_t all_done = SENSOR_RDY_ACC | SENSOR_RDY_MAGN | SENSOR_RDY_GYRO;

  // Read all sensors at least once
  while (sensors_done != all_done){

    // Wait for data ready signal
    chBSemWait(&lsm9ds0_drdy_sem);

    chMtxLock(&data_mtx);
    sensors_ready_local = sensors_ready;
    sensors_ready = 0;
    chMtxUnlock();

#ifdef SC_WAR_ISSUE_1
    // WAR for broken DRDY_G pin
    if (sensors_ready_local & SENSOR_RDY_ACC) {
      sensors_ready_local |= SENSOR_RDY_GYRO;
    }
#endif

    while (sensors_ready_local) {

      if (sensors_ready_local & SENSOR_RDY_ACC) {
        txbuf[0] = LSM9DS0_OUT_X_L_A | LSM9DS0_SUBADDR_AUTO_INC_BIT;
        sc_i2c_transmit(i2cn_xm, txbuf, 1, rxbuf, 6);

        for (i = 0; i < 3; ++i) {
          acc[i] = (int16_t)(rxbuf[2*i + 1] << 8 | rxbuf[2*i]);
        }

        sensors_ready_local &= ~SENSOR_RDY_ACC;
        sensors_done |= SENSOR_RDY_ACC;
      }

      if (sensors_ready_local & SENSOR_RDY_MAGN) {
        txbuf[0] = LSM9DS0_OUT_X_L_M | LSM9DS0_SUBADDR_AUTO_INC_BIT;
        sc_i2c_transmit(i2cn_xm, txbuf, 1, rxbuf, 6);

        for (i = 0; i < 3; ++i) {
          magn[i] = (int16_t)(rxbuf[2*i + 1] << 8 | rxbuf[2*i]);
        }

        sensors_ready_local &= ~SENSOR_RDY_MAGN;
        sensors_done |= SENSOR_RDY_MAGN;
      }

      if (sensors_ready_local & SENSOR_RDY_GYRO) {
        txbuf[0] = LSM9DS0_OUT_X_L_G | LSM9DS0_SUBADDR_AUTO_INC_BIT;
        sc_i2c_transmit(i2cn_g, txbuf, 1, rxbuf, 6);

        for (i = 0; i < 3; ++i) {
          gyro[i] = (int16_t)(rxbuf[2*i + 1] << 8 | rxbuf[2*i]);
        }

        sensors_ready_local &= ~SENSOR_RDY_GYRO;
        sensors_done |= SENSOR_RDY_GYRO;
      }
    }
  }
}



void sc_lsm9ds0_shutdown(void)
{
  uint8_t txbuf[2];

  // Power down gyro
  txbuf[0] = LSM9DS0_CTRL_REG1_G;
  txbuf[1] = 0x00;
  sc_i2c_write(i2cn_g, txbuf, sizeof(txbuf));

  // Disable gyro data ready interrupt
  txbuf[0] = LSM9DS0_CTRL_REG3_G;
  txbuf[1] = 0x00; //  I2_DRDY, gyroscope data ready
  sc_i2c_write(i2cn_g, txbuf, sizeof(txbuf));

  // Power down magn
  txbuf[0] = LSM9DS0_CTRL_REG7_XM;
  txbuf[1] = 0x02;
  sc_i2c_write(i2cn_xm, txbuf, sizeof(txbuf));

  // Disable magn data ready
  txbuf[0] = LSM9DS0_CTRL_REG4_XM;
  txbuf[1] = 0x00; // P2_DRDYM
  sc_i2c_write(i2cn_xm, txbuf, sizeof(txbuf));

  // Power down acc
  txbuf[0] = LSM9DS0_CTRL_REG1_XM;
  txbuf[1] = 0x00;
  sc_i2c_write(i2cn_xm, txbuf, sizeof(txbuf));

  // Disable acc data ready
  txbuf[0] = LSM9DS0_CTRL_REG3_XM;
  txbuf[1] = 0x00; // P1_DRDYA
  sc_i2c_write(i2cn_xm, txbuf, sizeof(txbuf));
}

#endif

/* Emacs indentatation information
   Local Variables:
   indent-tabs-mode:nil
   tab-width:2
   c-basic-offset:2
   End:
*/
