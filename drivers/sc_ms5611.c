/***
 * MS5611-01BA03  sensor driver
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

#define SC_LOG_MODULE_TAG SC_LOG_MODULE_UNSPECIFIED

#include "sc_utils.h"
#include "sc.h"
#include "sc_ms5611.h"
#include "sc_i2c.h"

#ifdef SC_HAS_MS5611

#define MS5611_RESET        0x1E
#define MS5611_CONVERT_D1   0x40
#define MS5611_CONVERT_D2   0x50
#define MS5611_ADC_READ     0x00

static uint8_t i2cn;
static uint16_t ms5611_interval_ms;
static uint16_t ms5611_prom[8];
static thread_t *ms5611_thread_ptr;
static binary_semaphore_t ms5611_wait_sem;
static mutex_t data_mtx;
static uint32_t ms5611_temp;
static uint32_t ms5611_pres;
static uint32_t ms5611_time_st;

static void ms5611_reset(void);
static void ms5611_read_prom(void);
static uint8_t ms5611_calc_crc(void);
static uint32_t ms5611_read(bool temp);

static THD_WORKING_AREA(sc_ms5611_thread, 512);
THD_FUNCTION(scMS5611Thread, arg)
{
  systime_t sleep_until;
  systime_t interval_st = MS2ST(ms5611_interval_ms);
  
  (void)arg;
  chRegSetThreadName(__func__);

  SC_LOG_PRINTF("d: ms5611 init\r\n");
  
  // We need to give a bit of time to the chip until we can ask it to shut down
  // i2c not ready initially? (This is inherited from the LSM9DS0 driver)
  chThdSleepMilliseconds(20);

  ms5611_reset();
  chThdSleepMilliseconds(100);
  ms5611_reset();
  chThdSleepMilliseconds(100);
  ms5611_reset();
  chThdSleepMilliseconds(100);
  ms5611_read_prom();
  uint8_t ref_crc = ms5611_prom[7] & 0x0F;
  uint8_t calc_crc = ms5611_calc_crc();
  if (ref_crc != calc_crc) {
    SC_LOG_PRINTF("e: ms5611 crc failure: 0x%x vs 0x%x\r\n", ref_crc, calc_crc);
  }

  sleep_until = chVTGetSystemTime() + interval_st;
  
  // Loop initiating measurements and waiting for results
  while (!chThdShouldTerminateX()) {
    systime_t now = chVTGetSystemTime();
    systime_t wait_time = sleep_until - now;
    uint32_t temp;
    uint32_t pres;
    msg_t msg;

    // Using a semaphore allows to cancel the sleep outside the thread
    if (chBSemWaitTimeout(&ms5611_wait_sem, wait_time) != MSG_TIMEOUT) {
      continue;
    }

    sleep_until += interval_st;
    now = chVTGetSystemTime();
    
    temp = ms5611_read(true);
    
    if (chThdShouldTerminateX()) {
      break;
    }
    
    pres = ms5611_read(false);

    chMtxLock(&data_mtx);
    ms5611_temp = temp;
    ms5611_pres = pres;
    ms5611_time_st = now;
    chMtxUnlock(&data_mtx);

    // Notify main app about new measurements being available
    msg = sc_event_msg_create_type(SC_EVENT_TYPE_MS5611_AVAILABLE);
    sc_event_msg_post(msg, SC_EVENT_MSG_POST_FROM_NORMAL);
  }
}


static void ms5611_reset(void)
{
  uint8_t txbuf[1];
  txbuf[0] = MS5611_RESET;
  sc_i2c_write(i2cn, txbuf, sizeof(txbuf));
}

static void ms5611_read_prom(void)
{
  uint16_t prom[8];

  for (uint8_t i = 0; i < sizeof(prom); ++i) {
    uint8_t txbuf[1] = {0xA0 | i << 1}; // prom register
    uint8_t rxbuf[2];

    sc_i2c_transmit(i2cn, txbuf, sizeof(txbuf), rxbuf, sizeof(rxbuf));
    ms5611_prom[i] = (uint16_t) (((uint16_t)rxbuf[0] << 8) | rxbuf[1]);
  }
}

static uint8_t ms5611_calc_crc(void)
{
  unsigned int n_rem;    // crc reminder
  unsigned int crc_read; // original value of the crc

  n_rem = 0x00;
  crc_read = ms5611_prom[7];    //save read CRC
  ms5611_prom[7]=(0xFF00 & (ms5611_prom[7])); // CRC byte is replaced by 0

  for (uint8_t cnt = 0; cnt < 16; cnt++) {
    // choose LSB or MSB
    if (cnt % 2 == 1) {
      n_rem ^= (uint16_t) ((ms5611_prom[cnt >> 1]) & 0x00FF);
    } else {
      n_rem ^= (uint16_t) (ms5611_prom[cnt >> 1] >> 8);
    }

    for (uint8_t n_bit = 8; n_bit > 0; n_bit--) {
      if (n_rem & (0x8000)) {
        n_rem = (n_rem << 1) ^ 0x3000;
      } else {
        n_rem = (n_rem << 1);
      }
    }
  }
  n_rem = (0x000F & (n_rem >> 12)); // final 4-bit reminder is CRC code
  ms5611_prom[7] = crc_read; // restore the crc_read to its original place
  return (n_rem ^ 0x0);
}

static uint32_t ms5611_read(bool temp)
{
  uint8_t reg_pres = MS5611_CONVERT_D1;
  uint8_t reg_temp = MS5611_CONVERT_D2;
  uint8_t txbuf[1] = {temp ? reg_temp : reg_pres | 0x08}; // temp or pressure reg with 4096 sampling rate
  uint8_t rxbuf[3];

  sc_i2c_transmit(i2cn, txbuf, sizeof(txbuf), NULL, 0);

  chThdSleepMilliseconds(10); // 10 ms for 4096 sampling rate

  txbuf[0] = 0; // ADC read
  sc_i2c_transmit(i2cn, txbuf, sizeof(txbuf), rxbuf, sizeof(rxbuf));

  return (uint32_t) (((uint32_t)rxbuf[0] << 16) |
                     (uint32_t)rxbuf[1] << 8 |
                     rxbuf[2]);
}

void sc_ms5611_init(uint16_t interval_ms)
{
  int8_t i2c_n;

  ms5611_interval_ms = interval_ms;

  chBSemObjectInit(&ms5611_wait_sem, TRUE);
  chMtxObjectInit(&data_mtx);

  // Pinmux
  // FIXME: add a parameter to init function to skip pinmux if it's already done for LSM9DS1?
  // FIXME: then again, it's the same for both, so does't it matter to do it twice?
  palSetPadMode(SC_MS5611_I2CN_SDA_PORT,
                SC_MS5611_I2CN_SDA_PIN,
                PAL_MODE_ALTERNATE(SC_MS5611_I2CN_SDA_AF));
  palSetPadMode(SC_MS5611_I2CN_SCL_PORT,
                SC_MS5611_I2CN_SCL_PIN,
                PAL_MODE_ALTERNATE(SC_MS5611_I2CN_SDA_AF));

  i2c_n = sc_i2c_init(&SC_MS5611_I2CN, SC_MS5611_ADDR);
  i2cn = (uint8_t)i2c_n;

  ms5611_thread_ptr =
    chThdCreateStatic(sc_ms5611_thread,
                      sizeof(sc_ms5611_thread),
                      NORMALPRIO,
                      scMS5611Thread,
                      NULL);

}



void sc_ms5611_read(uint32_t *temp, uint32_t *pres, systime_t *time_st)
{
    chMtxLock(&data_mtx);
    *temp = ms5611_temp;
    *pres = ms5611_pres;
    *time_st = ms5611_time_st;
    chMtxUnlock(&data_mtx);
}



void sc_ms5611_shutdown(bool reset)
{
  uint8_t i = 25;

  chThdTerminate(ms5611_thread_ptr);

  // Reset the semaphore so that the thread can exit
  chBSemReset(&ms5611_wait_sem, TRUE);

  // Wait up to 25 ms for the thread to exit
  while (i-- > 0 && !chThdTerminatedX(ms5611_thread_ptr)) {
    chThdSleepMilliseconds(1);
  }

  chDbgAssert(chThdTerminatedX(ms5611_thread_ptr), "MS5611 thread not yet terminated");

  ms5611_thread_ptr = NULL;

  // FIXME: shutdown ms5611

  if (!reset) {
    sc_i2c_stop(i2cn);
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
