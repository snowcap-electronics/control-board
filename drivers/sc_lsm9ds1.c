/***
 * ST LSM9DS1 9DOF sensor driver
 *
 * Copyright 2013-2015 Tuomas Kulve, <tuomas.kulve@snowcap.fi>
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

/* Bunch of code taken from:
 * https://github.com/kriswiner/LSM9DS1/blob/master/LSM9DS1_MS5611_BasicAHRS_t3.ino
 */


#include "sc_utils.h"
#include "sc.h"
#include "sc_lsm9ds1.h"
#include "sc_i2c.h"
#include "sc_extint.h"

#ifdef SC_HAS_LSM9DS1

/* LSM9DS1 registers used in this file */
// Accelerometer and Gyroscope registers
#define LSM9DS1XG_ACT_THS	    0x04
#define LSM9DS1XG_ACT_DUR	    0x05
#define LSM9DS1XG_INT_GEN_CFG_XL    0x06
#define LSM9DS1XG_INT_GEN_THS_X_XL  0x07
#define LSM9DS1XG_INT_GEN_THS_Y_XL  0x08
#define LSM9DS1XG_INT_GEN_THS_Z_XL  0x09
#define LSM9DS1XG_INT_GEN_DUR_XL    0x0A
#define LSM9DS1XG_REFERENCE_G       0x0B
#define LSM9DS1XG_INT1_CTRL         0x0C
#define LSM9DS1XG_INT2_CTRL         0x0D
#define LSM9DS1XG_WHO_AM_I          0x0F  // should return 0x68
#define LSM9DS1XG_CTRL_REG1_G       0x10
#define LSM9DS1XG_CTRL_REG2_G       0x11
#define LSM9DS1XG_CTRL_REG3_G       0x12
#define LSM9DS1XG_ORIENT_CFG_G      0x13
#define LSM9DS1XG_INT_GEN_SRC_G     0x14
#define LSM9DS1XG_OUT_TEMP_L        0x15
#define LSM9DS1XG_OUT_TEMP_H        0x16
#define LSM9DS1XG_STATUS_REG        0x17
#define LSM9DS1XG_OUT_X_L_G         0x18
#define LSM9DS1XG_OUT_X_H_G         0x19
#define LSM9DS1XG_OUT_Y_L_G         0x1A
#define LSM9DS1XG_OUT_Y_H_G         0x1B
#define LSM9DS1XG_OUT_Z_L_G         0x1C
#define LSM9DS1XG_OUT_Z_H_G         0x1D
#define LSM9DS1XG_CTRL_REG4         0x1E
#define LSM9DS1XG_CTRL_REG5_XL      0x1F
#define LSM9DS1XG_CTRL_REG6_XL      0x20
#define LSM9DS1XG_CTRL_REG7_XL      0x21
#define LSM9DS1XG_CTRL_REG8         0x22
#define LSM9DS1XG_CTRL_REG9         0x23
#define LSM9DS1XG_CTRL_REG10        0x24
#define LSM9DS1XG_INT_GEN_SRC_XL    0x26
//#define LSM9DS1XG_STATUS_REG        0x27 // duplicate of 0x17!
#define LSM9DS1XG_OUT_X_L_XL        0x28
#define LSM9DS1XG_OUT_X_H_XL        0x29
#define LSM9DS1XG_OUT_Y_L_XL        0x2A
#define LSM9DS1XG_OUT_Y_H_XL        0x2B
#define LSM9DS1XG_OUT_Z_L_XL        0x2C
#define LSM9DS1XG_OUT_Z_H_XL        0x2D
#define LSM9DS1XG_FIFO_CTRL         0x2E
#define LSM9DS1XG_FIFO_SRC          0x2F
#define LSM9DS1XG_INT_GEN_CFG_G     0x30
#define LSM9DS1XG_INT_GEN_THS_XH_G  0x31
#define LSM9DS1XG_INT_GEN_THS_XL_G  0x32
#define LSM9DS1XG_INT_GEN_THS_YH_G  0x33
#define LSM9DS1XG_INT_GEN_THS_YL_G  0x34
#define LSM9DS1XG_INT_GEN_THS_ZH_G  0x35
#define LSM9DS1XG_INT_GEN_THS_ZL_G  0x36
#define LSM9DS1XG_INT_GEN_DUR_G     0x37
//
// Magnetometer registers
#define LSM9DS1M_OFFSET_X_REG_L_M   0x05
#define LSM9DS1M_OFFSET_X_REG_H_M   0x06
#define LSM9DS1M_OFFSET_Y_REG_L_M   0x07
#define LSM9DS1M_OFFSET_Y_REG_H_M   0x08
#define LSM9DS1M_OFFSET_Z_REG_L_M   0x09
#define LSM9DS1M_OFFSET_Z_REG_H_M   0x0A
#define LSM9DS1M_WHO_AM_I           0x0F  // should be 0x3D
#define LSM9DS1M_CTRL_REG1_M        0x20
#define LSM9DS1M_CTRL_REG2_M        0x21
#define LSM9DS1M_CTRL_REG3_M        0x22
#define LSM9DS1M_CTRL_REG4_M        0x23
#define LSM9DS1M_CTRL_REG5_M        0x24
#define LSM9DS1M_STATUS_REG_M       0x27
#define LSM9DS1M_OUT_X_L_M          0x28
#define LSM9DS1M_OUT_X_H_M          0x29
#define LSM9DS1M_OUT_Y_L_M          0x2A
#define LSM9DS1M_OUT_Y_H_M          0x2B
#define LSM9DS1M_OUT_Z_L_M          0x2C
#define LSM9DS1M_OUT_Z_H_M          0x2D
#define LSM9DS1M_INT_CFG_M          0x30
#define LSM9DS1M_INT_SRC_M          0x31
#define LSM9DS1M_INT_THS_L_M        0x32
#define LSM9DS1M_INT_THS_H_M        0x33

// Set initial input parameters
enum Ascale {  // set of allowable accel full scale settings
  AFS_2G = 0,
  AFS_16G,
  AFS_4G,
  AFS_8G
};

enum Aodr {  // set of allowable gyro sample rates
  AODR_PowerDown = 0,
  AODR_10Hz,
  AODR_50Hz,
  AODR_119Hz,
  AODR_238Hz,
  AODR_476Hz,
  AODR_952Hz
};

enum Abw {  // set of allowable accewl bandwidths
   ABW_408Hz = 0,
   ABW_211Hz,
   ABW_105Hz,
   ABW_50Hz
};

enum Gscale {  // set of allowable gyro full scale settings
  GFS_245DPS = 0,
  GFS_500DPS,
  GFS_NoOp,
  GFS_2000DPS
};

enum Godr {  // set of allowable gyro sample rates
  GODR_PowerDown = 0,
  GODR_14_9Hz,
  GODR_59_5Hz,
  GODR_119Hz,
  GODR_238Hz,
  GODR_476Hz,
  GODR_952Hz
};

enum Gbw {   // set of allowable gyro data bandwidths
  GBW_low = 0,  // 14 Hz at Godr = 238 Hz,  33 Hz at Godr = 952 Hz
  GBW_med,      // 29 Hz at Godr = 238 Hz,  40 Hz at Godr = 952 Hz
  GBW_high,     // 63 Hz at Godr = 238 Hz,  58 Hz at Godr = 952 Hz
  GBW_highest   // 78 Hz at Godr = 238 Hz, 100 Hz at Godr = 952 Hz
};

enum Mscale {  // set of allowable mag full scale settings
  MFS_4G = 0,
  MFS_8G,
  MFS_12G,
  MFS_16G
};

enum Mmode {
  MMode_LowPower = 0, 
  MMode_MedPerformance,
  MMode_HighPerformance,
  MMode_UltraHighPerformance
};

enum Modr {  // set of allowable mag sample rates
  MODR_0_625Hz = 0,
  MODR_1_25Hz,
  MODR_2_5Hz,
  MODR_5Hz,
  MODR_10Hz,
  MODR_20Hz,
  MODR_80Hz
};

#define ADC_256  0x00 // define pressure and temperature conversion rates
#define ADC_512  0x02
#define ADC_1024 0x04
#define ADC_2048 0x06
#define ADC_4096 0x08
#define ADC_D1   0x40
#define ADC_D2   0x50

#define LSM9DS1_SUBADDR_AUTO_INC_BIT  0x80

// Specify sensor full scale
static const uint8_t OSR = ADC_4096;      // set pressure amd temperature oversample rate
static const uint8_t Gscale = GFS_245DPS; // gyro full scale
static const uint8_t Godr = GODR_238Hz;   // gyro data sample rate
static const uint8_t Gbw = GBW_med;       // gyro data bandwidth
static const uint8_t Ascale = AFS_2G;     // accel full scale
static const uint8_t Aodr = AODR_238Hz;   // accel data sample rate
static const uint8_t Abw = ABW_50Hz;      // accel data bandwidth
static const uint8_t Mscale = MFS_4G;     // mag full scale
static const uint8_t Modr = MODR_10Hz;    // mag data sample rate
static const uint8_t Mmode = MMode_HighPerformance;  // magnetometer operation mode
static float aRes, gRes, mRes;            // scale resolutions per LSB for the sensors

static uint8_t i2cn_ag;
static uint8_t i2cn_m;
static binary_semaphore_t lsm9ds1_drdy_sem;
static uint8_t sensors_ready;

static void getMres(void)
{
  switch (Mscale) {
    // Possible magnetometer scales (and their register bit settings) are:
    // 4 Gauss (00), 8 Gauss (01), 12 Gauss (10) and 16 Gauss (11)
  case MFS_4G:
    mRes = (0.14/1000.0); /*  4.0/32768.0 */
    break;
  case MFS_8G:
    mRes = (0.29/1000.0); /*  8.0/32768.0 */
    break;
  case MFS_12G:
    mRes = (0.43/1000.0); /* 12.0/32768.0 */
    break;
  case MFS_16G:
    mRes = (0.58/1000.0); /* 16.0/32768.0 */
    break;
  }
}



static void getGres(void)
{
  switch (Gscale) {
      // Possible gyro scales (and their register bit settings) are:
      // 245 DPS (00), 500 DPS (01), and 2000 DPS  (11). 
  case GFS_245DPS:
    gRes = (8.75 / 1000.0);  /*  245.0/32768.0 */
    break;
  case GFS_500DPS:
    gRes = (17.50 / 1000.0); /*  500.0/32768.0 */
    break;
  case GFS_2000DPS:
    gRes = (70.00 / 1000.0); /* 2000.0/32768.0 */
    break;
  }
}



static void getAres(void)
{
  switch (Ascale) {
    // Possible accelerometer scales (and their register bit settings) are:
    // 2 Gs (00), 16 Gs (01), 4 Gs (10), and 8 Gs  (11). 
  case AFS_2G:
    aRes = (0.061 / 1000.0);  /* 2.0/32768.0 */
    break;
  case AFS_16G:
    aRes = (0.732 / 1000.0); /* 16.0/32768.0 */
    break;
  case AFS_4G:
    aRes = (0.122 / 1000.0); /*  4.0/32768.0 */
    break;
  case AFS_8G:
    aRes = (0.244 / 1000.0); /*  8.0/32768.0 */
    break;
  }
}



static void lsm9ds1_drdy_cb(EXTDriver *extp, expchannel_t channel)
{
  uint8_t rdy = 0;

  (void)extp;

  switch(channel) {
  case SC_LSM9DS1_INT1_AG_PIN:
	  rdy = SC_SENSOR_ACC;
    break;
  case SC_LSM9DS1_INT2_AG_PIN:
	  rdy = SC_SENSOR_GYRO;
    break;
  case SC_LSM9DS1_DRDY_M_PIN:
	  rdy = SC_SENSOR_MAGN;
    break;
  default:
    chDbgAssert(0, "Invalid channel");
    return;
  }
  chSysLockFromISR();
  sensors_ready |= rdy;

  chBSemSignalI(&lsm9ds1_drdy_sem);

  chSysUnlockFromISR();
}



static void writeByte(uint8_t i2c_n, uint8_t reg, uint8_t byte)
{
  uint8_t txbuf[2];
  txbuf[0] = reg;
  txbuf[1] = byte; 
  sc_i2c_write(i2c_n, txbuf, sizeof(txbuf));
}


void sc_lsm9ds1_init(void)
{
  int8_t i2c_n;

  sensors_ready = 0;
  chBSemObjectInit(&lsm9ds1_drdy_sem, TRUE);

  // Pinmux
  palSetPadMode(SC_LSM9DS1_INT1_AG_PORT,
                SC_LSM9DS1_INT1_AG_PIN,
                PAL_MODE_INPUT);
  palSetPadMode(SC_LSM9DS1_INT2_AG_PORT,
                SC_LSM9DS1_INT2_AG_PIN,
                PAL_MODE_INPUT);
  palSetPadMode(SC_LSM9DS1_DRDY_M_PORT,
                SC_LSM9DS1_DRDY_M_PIN,
                PAL_MODE_INPUT);
  palSetPadMode(SC_LSM9DS1_I2CN_SDA_PORT,
                SC_LSM9DS1_I2CN_SDA_PIN,
                PAL_MODE_ALTERNATE(SC_LSM9DS1_I2CN_SDA_AF));
  palSetPadMode(SC_LSM9DS1_I2CN_SCL_PORT,
                SC_LSM9DS1_I2CN_SCL_PIN,
                PAL_MODE_ALTERNATE(SC_LSM9DS1_I2CN_SDA_AF));

  // Register data ready interrupts
  sc_extint_set_isr_cb(SC_LSM9DS1_INT1_AG_PORT,
                       SC_LSM9DS1_INT1_AG_PIN,
                       SC_EXTINT_EDGE_RISING,
                       lsm9ds1_drdy_cb);

  sc_extint_set_isr_cb(SC_LSM9DS1_INT2_AG_PORT,
                       SC_LSM9DS1_INT2_AG_PIN,
                       SC_EXTINT_EDGE_RISING,
                       lsm9ds1_drdy_cb);

  sc_extint_set_isr_cb(SC_LSM9DS1_DRDY_M_PORT,
                       SC_LSM9DS1_DRDY_M_PIN,
                       SC_EXTINT_EDGE_RISING,
                       lsm9ds1_drdy_cb);


  // Initialize two I2C instances, one for acc+magn, one for gyro
  i2c_n = sc_i2c_init(&SC_LSM9DS1_I2CN, SC_LSM9DS1_ADDR_AG);
  i2cn_ag = (uint8_t)i2c_n;

  i2c_n = sc_i2c_init(&SC_LSM9DS1_I2CN, SC_LSM9DS1_ADDR_M);
  i2cn_m = (uint8_t)i2c_n;

  // FIXME: setXres instead of getXres?
  getAres();
  getGres();
  getMres();

  // We need to give a bit of time to the chip until we can ask it to shut down
  // i2c not ready initially? (This is inherited from the LSM9DS0 driver)
  chThdSleepMilliseconds(20);

  // Shutdown the chip so that we get new data ready
  // interrupts even if the chip was already running
  sc_lsm9ds1_shutdown(true);

  // enable the 3-axes of the gyroscope
  writeByte(i2cn_ag, LSM9DS1XG_CTRL_REG4, 0x38);
  // configure the gyroscope
  writeByte(i2cn_ag, LSM9DS1XG_CTRL_REG1_G, Godr << 5 | Gscale << 3 | Gbw);
  // enable gyroscope data ready interrupt on int2
  writeByte(i2cn_ag, LSM9DS1XG_INT2_CTRL, 0x02);

  // FIXME DS1: why delay here?
  //chThdSleepMilliseconds(200);

  // enable the three axes of the accelerometer 
  writeByte(i2cn_ag, LSM9DS1XG_CTRL_REG5_XL, 0x38);
  // configure the accelerometer-specify bandwidth selection with Abw
  writeByte(i2cn_ag, LSM9DS1XG_CTRL_REG6_XL, Aodr << 5 | Ascale << 3 | 0x04 |Abw);

  // FIXME DS1: why delay here?
  //chThdSleepMilliseconds(200);

  // enable block data update, allow auto-increment during multiple byte read
  writeByte(i2cn_ag, LSM9DS1XG_CTRL_REG8, 0x44);
  // enable accelerometer data ready interrupt on int1
  writeByte(i2cn_ag, LSM9DS1XG_INT1_CTRL, 0x01);

  // configure the magnetometer-enable temperature compensation of mag data
  writeByte(i2cn_m, LSM9DS1M_CTRL_REG2_M, 0x1 << 2 | 0x1 << 3 ); // reboot + reset
  writeByte(i2cn_m, LSM9DS1M_CTRL_REG2_M, Mscale << 5 ); // select mag full scale
  writeByte(i2cn_m, LSM9DS1M_CTRL_REG1_M, 0x80 | Mmode << 5 | Modr << 2); // select x,y-axis mode
  writeByte(i2cn_m, LSM9DS1M_CTRL_REG3_M, 0x00 ); // continuous conversion mode
  writeByte(i2cn_m, LSM9DS1M_CTRL_REG4_M, Mmode << 2 ); // select z-axis mode
  writeByte(i2cn_m, LSM9DS1M_CTRL_REG5_M, 0x40 ); // select block update mode

  // enable magnetometer interrupts
  writeByte(i2cn_m, LSM9DS1M_INT_CFG_M, 0x20 | 0x10 | 0x08 | 0x04 | 0x01 ); // XIEN,YIEN,ZIEN,IEA,IEN
}



uint8_t sc_lsm9ds1_read(sc_float *acc, sc_float *magn, sc_float *gyro)
{
  uint8_t txbuf[1];
  uint8_t rxbuf[6];
  uint8_t sensors_done = 0;
  uint8_t i;
  uint32_t now = 0;

  static uint32_t acc_read_last = 0;
  static uint32_t magn_read_last = 0;
  static uint32_t gyro_read_last = 0;

  // Wait for data ready signal
  chBSemWaitTimeout(&lsm9ds1_drdy_sem, 50);

  // Read data if there hasn't been a data ready interrupt in a while
  // just in case an interrupt was missed. May help also in the case
  // of a bad interrupt state after a soft reset
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

  if (sensors_ready & SC_SENSOR_ACC) {

    chSysLock();
    sensors_ready &= ~SC_SENSOR_ACC;
    chSysUnlock();

    txbuf[0] = LSM9DS1XG_OUT_X_L_XL;
    sc_i2c_transmit(i2cn_ag, txbuf, 1, rxbuf, 6);

    for (i = 0; i < 3; ++i) {
      acc[i] = ((int16_t)(((uint16_t)rxbuf[2*i + 1]) << 8 | rxbuf[2*i])) * aRes;
    }

    acc[0] *= -1;

    sensors_done |= SC_SENSOR_ACC;
  }

  if (sensors_ready & SC_SENSOR_GYRO) {

    chSysLock();
    sensors_ready &= ~SC_SENSOR_GYRO;
    chSysUnlock();

    txbuf[0] = LSM9DS1XG_OUT_X_L_G;
    sc_i2c_transmit(i2cn_ag, txbuf, 1, rxbuf, 6);

    for (i = 0; i < 3; ++i) {
      gyro[i] = ((int16_t)(((uint16_t)rxbuf[2*i + 1]) << 8 | rxbuf[2*i])) * gRes;
    }

    gyro[0] *= -1;

    sensors_done |= SC_SENSOR_GYRO;
  }

  if (sensors_ready & SC_SENSOR_MAGN) {

    chSysLock();
    sensors_ready &= ~SC_SENSOR_MAGN;
    chSysUnlock();

    txbuf[0] = LSM9DS1M_OUT_X_L_M;
    sc_i2c_transmit(i2cn_m, txbuf, 1, rxbuf, 6);

    for (i = 0; i < 3; ++i) {
      magn[i] = ((int16_t)(((uint16_t)rxbuf[2*i + 1]) << 8 | rxbuf[2*i])) * mRes;
    }

    // Reverse X-axis to match the AHRS algorithm
    //magn[0] *= -1;

    sensors_done |= SC_SENSOR_MAGN;
  }

  // FIXME: Ignore first n values as per datasheet
  return sensors_done;
}



void sc_lsm9ds1_shutdown(bool reset)
{
  if (!reset) {
    // Remove callbacks
    sc_extint_clear(SC_LSM9DS1_INT1_AG_PORT, SC_LSM9DS1_INT1_AG_PIN);
    sc_extint_clear(SC_LSM9DS1_INT2_AG_PORT, SC_LSM9DS1_INT2_AG_PIN);
    sc_extint_clear(SC_LSM9DS1_DRDY_M_PORT,  SC_LSM9DS1_DRDY_M_PIN);
  }

  // Disable interrupts
  writeByte(i2cn_m, LSM9DS1M_INT_CFG_M, 0);
  writeByte(i2cn_ag, LSM9DS1XG_INT1_CTRL, 0);
  writeByte(i2cn_ag, LSM9DS1XG_INT2_CTRL, 0);

  // Power down magn
  writeByte(i2cn_m, LSM9DS1M_CTRL_REG3_M, 0x03); 
  // Power down acc
  writeByte(i2cn_ag, LSM9DS1XG_CTRL_REG6_XL, 0);
  // Power down gyroscope
  writeByte(i2cn_ag, LSM9DS1XG_CTRL_REG1_G, 0);

  if (!reset) {
    sc_i2c_stop(i2cn_m);
    sc_i2c_stop(i2cn_ag);
  }
}

#endif

/* Emacs indentatation information
   Local Variables:
   indent-tabs-mode:nil
   tab-width:2
   c-basic-offset:2
   End:
*/
