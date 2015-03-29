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
#include "sc.h"
#include "sc_lsm9ds0.h"
#include "sc_i2c.h"
#include "sc_extint.h"

#ifdef SC_HAS_LSM9DS0

/* LSM9DS0 registers used in this file */
#define LSM9DS0_INT_CTRL_REG_M     0x12
#define LSM9DS0_CTRL_REG1_G        0x20
#define LSM9DS0_CTRL_REG3_G        0x22
#define LSM9DS0_CTRL_REG4_G        0x23
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
#define LSM9DS0_CTRL_REG2_XM       0x21
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

// +-2 g: 0.061 mg/bit
#define LSM9DS0_ACC_SENSITIVITY    (0.061 / 1000.0)
// +-4 gauss: 0.16 mgauss/bit
#define LSM9DS0_MAGN_SENSITIVITY   (0.16 / 1000.0)
// +-12 gauss: 0.48 mgauss/bit
//#define LSM9DS0_MAGN_SENSITIVITY   (0.48 / 1000.0)
// +-245 mdps: 8.75 mdps/bit
#define LSM9DS0_GYRO_SENSITIVITY   (8.75 / 1000.0)
// +-500 mdps: 17.50 mdps/bit
//#define LSM9DS0_GYRO_SENSITIVITY   (17.50 / 1000.0)


enum Ascale { // set of allowable accel full scale settings
  AFS_2G = 0,
  AFS_4G,
  AFS_6G,
  AFS_8G,
  AFS_16G
};

enum Aodr { // set of allowable gyro sample rates
  AODR_PowerDown = 0,
  AODR_3_125Hz,
  AODR_6_25Hz,
  AODR_12_5Hz,
  AODR_25Hz,
  AODR_50Hz,
  AODR_100Hz,
  AODR_200Hz,
  AODR_400Hz,
  AODR_800Hz,
  AODR_1600Hz
};

enum Abw { // set of allowable accewl bandwidths
  ABW_773Hz = 0,
  ABW_194Hz,
  ABW_362Hz,
  ABW_50Hz
};

enum Gscale { // set of allowable gyro full scale settings
  GFS_245DPS = 0,
  GFS_500DPS,
  GFS_NoOp,
  GFS_2000DPS
};

enum Godr { // set of allowable gyro sample rates
  GODR_95Hz = 0,
  GODR_190Hz,
  GODR_380Hz,
  GODR_760Hz
};

enum Gbw { // set of allowable gyro data bandwidths
  GBW_low = 0, // 12.5 Hz at Godr = 95 Hz, 12.5 Hz at Godr = 190 Hz, 30 Hz  at Godr = 760 Hz
  GBW_med,     // 25 Hz   at Godr = 95 Hz, 25 Hz   at Godr = 190 Hz, 35 Hz  at Godr = 760 Hz
  GBW_high,    // 25 Hz   at Godr = 95 Hz, 50 Hz   at Godr = 190 Hz, 50 Hz  at Godr = 760 Hz
  GBW_highest  // 25 Hz   at Godr = 95 Hz, 70 Hz   at Godr = 190 Hz, 100 Hz at Godr = 760 Hz
};

enum Mscale { // set of allowable mag full scale settings
  MFS_2G = 0,
  MFS_4G,
  MFS_8G,
  MFS_12G
};

enum Mres {
  MRES_LowResolution = 0,
  MRES_NoOp,
  MRES_HighResolution
};

enum Modr { // set of allowable mag sample rates
  MODR_3_125Hz = 0,
  MODR_6_25Hz,
  MODR_12_5Hz,
  MODR_25Hz,
  MODR_50Hz,
  MODR_100Hz
};

enum Temp {
  TEMP_Disable = 0,
  TEMP_Enable
};

static uint8_t i2cn_xm;
static uint8_t i2cn_g;
static binary_semaphore_t lsm9ds0_drdy_sem;
static uint8_t sensors_ready;

static void lsm9ds0_drdy_cb(EXTDriver *extp, expchannel_t channel)
{
  uint8_t rdy = 0;

  (void)extp;

  switch(channel) {
  case SC_LSM9DS0_INT1_XM_PIN:
	  rdy = SC_SENSOR_ACC;
    break;
  case SC_LSM9DS0_INT2_XM_PIN:
	  rdy = SC_SENSOR_MAGN;
    break;
  case SC_LSM9DS0_DRDY_G_PIN:
	  rdy = SC_SENSOR_GYRO;
    break;
  default:
    chDbgAssert(0, "Invalid channel");
    return;
  }
  chSysLockFromISR();
  sensors_ready |= rdy;

  chBSemSignalI(&lsm9ds0_drdy_sem);

  chSysUnlockFromISR();
}



void sc_lsm9ds0_init(void)
{
  uint8_t txbuf[2];
  int8_t i2c_n;

  sensors_ready = 0;
  chBSemObjectInit(&lsm9ds0_drdy_sem, TRUE);

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

  // We need to give a bit of time to the chip until we can ask it to shut down
  // i2c not ready initially?
  chThdSleepMilliseconds(20);

  // Shutdown the chip so that we get new data ready
  // interrupts even if the chip was already running
  sc_lsm9ds0_shutdown();

  // Setup and start gyro

  // Enable gyro data ready interrupt
  txbuf[0] = LSM9DS0_CTRL_REG3_G;
  txbuf[1] = 0x08; //  I2_DRDY, gyroscope data ready
  sc_i2c_write(i2cn_g, txbuf, sizeof(txbuf));

  // Set gyro sensitivy
  txbuf[0] = LSM9DS0_CTRL_REG4_G;
  txbuf[1] = GFS_245DPS << 4;
  sc_i2c_write(i2cn_g, txbuf, sizeof(txbuf));

  // Enable gyroscope X,Y,Z with powerdown mode disabled
  txbuf[0] = LSM9DS0_CTRL_REG1_G;
  txbuf[1] = GODR_190Hz << 6 | GBW_low << 4 | 0x0F;
  sc_i2c_write(i2cn_g, txbuf, sizeof(txbuf));

  // Configure INT1_XM as acc data ready
  txbuf[0] = LSM9DS0_CTRL_REG3_XM;
  txbuf[1] = 0x04; // P1_DRDYA
  sc_i2c_write(i2cn_xm, txbuf, sizeof(txbuf));

  // Configure INT2_XM as magn data ready
  txbuf[0] = LSM9DS0_CTRL_REG4_XM;
  txbuf[1] = 0x04; // P2_DRDYM
  sc_i2c_write(i2cn_xm, txbuf, sizeof(txbuf));

  // Enable acc
  txbuf[0] = LSM9DS0_CTRL_REG1_XM;
  txbuf[1] = AODR_200Hz << 4 | 0x07; // XEN + YEN + ZEN
  sc_i2c_write(i2cn_xm, txbuf, sizeof(txbuf));

  txbuf[0] = LSM9DS0_CTRL_REG2_XM;
  txbuf[1] = ABW_50Hz << 6 | AFS_2G << 3;
  sc_i2c_write(i2cn_xm, txbuf, sizeof(txbuf));

  // Configure magn and temperature
  txbuf[0] = LSM9DS0_CTRL_REG5_XM;
  txbuf[1] = TEMP_Enable << 7 | MRES_HighResolution << 5 | MODR_50Hz << 2;
  sc_i2c_write(i2cn_xm, txbuf, sizeof(txbuf));

  txbuf[0] = LSM9DS0_CTRL_REG6_XM;
  txbuf[1] = MFS_4G << 5;
  sc_i2c_write(i2cn_xm, txbuf, sizeof(txbuf));

  // Enable magn in continuous conversion mode
  txbuf[0] = LSM9DS0_CTRL_REG7_XM;
  txbuf[1] = 0x00; // Continuous conversion mode
  sc_i2c_write(i2cn_xm, txbuf, sizeof(txbuf));

  // Enable interrupts
  txbuf[0] = LSM9DS0_INT_CTRL_REG_M;
  txbuf[1] = 0xE9; // XEN + YEN + ZEN + int + active high
  sc_i2c_write(i2cn_xm, txbuf, sizeof(txbuf));
}



uint8_t sc_lsm9ds0_read(sc_float *acc, sc_float *magn, sc_float *gyro)
{
  uint8_t txbuf[1];
  uint8_t rxbuf[6];
  uint8_t sensors_done = 0;
  uint8_t i;
  uint32_t now = 0;

  static uint32_t acc_read_last = 0;
  static uint32_t magn_read_last = 0;
  static uint32_t gyro_read_last = 0;

  if (!sensors_ready) {
    // Wait for data ready signal
    chBSemWaitTimeout(&lsm9ds0_drdy_sem, 50);
  }

#ifdef SC_WAR_ISSUE_1
  // WAR: read data if there hasn't been a data ready interrupt in a while
  {
    // FIXME: assuming certain rates here
    now = ST2MS(chVTGetSystemTime());
    chSysLock();
    if (now - acc_read_last > 100) {
      sensors_ready |= SC_SENSOR_ACC;
    }
    if (now - magn_read_last > 100) {
      sensors_ready |= SC_SENSOR_MAGN;
    }
    if (now - gyro_read_last > 100) {
      sensors_ready |= SC_SENSOR_GYRO;
    }
    chSysUnlock();
  }
#else
  (void)acc_read_last;
  (void)magn_read_last;
  (void)gyro_read_last;
#endif

  if (sensors_ready & SC_SENSOR_ACC) {

    chSysLock();
    sensors_ready &= ~SC_SENSOR_ACC;
    chSysUnlock();

    txbuf[0] = LSM9DS0_OUT_X_L_A | LSM9DS0_SUBADDR_AUTO_INC_BIT;
    sc_i2c_transmit(i2cn_xm, txbuf, 1, rxbuf, 6);

    for (i = 0; i < 3; ++i) {
      acc[i] = ((int16_t)(((uint16_t)rxbuf[2*i + 1]) << 8 | rxbuf[2*i])) *
        LSM9DS0_ACC_SENSITIVITY;
    }

    sensors_done |= SC_SENSOR_ACC;
    acc_read_last = now;
  }

  if (sensors_ready & SC_SENSOR_MAGN) {

    chSysLock();
    sensors_ready &= ~SC_SENSOR_MAGN;
    chSysUnlock();

    txbuf[0] = LSM9DS0_OUT_X_L_M | LSM9DS0_SUBADDR_AUTO_INC_BIT;
    sc_i2c_transmit(i2cn_xm, txbuf, 1, rxbuf, 6);

    for (i = 0; i < 3; ++i) {
      magn[i] = ((int16_t)(((uint16_t)rxbuf[2*i + 1]) << 8 | rxbuf[2*i])) *
        LSM9DS0_MAGN_SENSITIVITY;
    }

    // AHRS algorithm seems to assume that magnetometer's Z-axis is reversed
    magn[2] *= -1;

    sensors_done |= SC_SENSOR_MAGN;
    magn_read_last = now;
  }

  if (sensors_ready & SC_SENSOR_GYRO) {

    chSysLock();
    sensors_ready &= ~SC_SENSOR_GYRO;
    chSysUnlock();

    txbuf[0] = LSM9DS0_OUT_X_L_G | LSM9DS0_SUBADDR_AUTO_INC_BIT;
    sc_i2c_transmit(i2cn_g, txbuf, 1, rxbuf, 6);

    for (i = 0; i < 3; ++i) {
      gyro[i] = ((int16_t)(((uint16_t)rxbuf[2*i + 1]) << 8 | rxbuf[2*i])) *
        LSM9DS0_GYRO_SENSITIVITY;
    }

    sensors_done |= SC_SENSOR_GYRO;
    gyro_read_last = now;
  }

  return sensors_done;
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
