/***
 * 9DoF wrapper for orientation sensors
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
#include "drivers/sc_lis302dl.h"
#include "drivers/sc_lsm9ds0.h"

#ifdef SC_USE_9DOF

#include <string.h>

static uint8_t running = 0;
static uint8_t sensors_valid = 0;
static sc_float acc[3];
static sc_float magn[3];
static sc_float gyro[3];
static uint32_t ts;

static mutex_t data_mtx;
static thread_t * sc_9dof_thread_ptr;

static THD_WORKING_AREA(sc_9dof_thread, 512);
THD_FUNCTION(sc9dofThread, arg)
{
  sc_float tmp_acc[3]  = {0, 0, 0};
  sc_float tmp_magn[3] = {0, 0, 0};
  sc_float tmp_gyro[3] = {0, 0, 0};
  msg_t drdy;
  uint8_t sensors_read;
  (void)arg;

  chRegSetThreadName(__func__);

  drdy = sc_event_msg_create_type(SC_EVENT_TYPE_9DOF_AVAILABLE);

  // Init sensor(s)
#ifdef SC_HAS_LIS302DL
  sc_lis302dl_init();
#elif defined(SC_HAS_LSM9DS0)
  sc_lsm9ds0_init();
#else
  chDbgAssert(0, "No 9dof drivers included in the build");
#endif

  while (running) {

    // Read data from the sensor. Assume it handles data ready signals
    // (or the appropriate sleeps) internally.
#ifdef SC_HAS_LIS302DL
    sensors_read = sc_lis302dl_read(tmp_acc);
    (void)tmp_magn;
#elif defined(SC_HAS_LSM9DS0)
    sensors_read = sc_lsm9ds0_read(tmp_acc, tmp_magn, tmp_gyro);
#else
    chDbgAssert(0, "No 9dof drivers included in the build");
#endif

    if (!sensors_read) {
      continue;
    }

    chMtxLock(&data_mtx);
    sensors_valid = sensors_read;
    if (sensors_valid & SC_SENSOR_ACC) {
      memcpy(acc,  tmp_acc,  sizeof(tmp_acc));
    }
    if (sensors_valid & SC_SENSOR_MAGN) {
      memcpy(magn, tmp_magn, sizeof(tmp_magn));
    }
    if (sensors_valid & SC_SENSOR_GYRO) {
      memcpy(gyro, tmp_gyro, sizeof(tmp_gyro));
    }
    ts = ST2MS(chVTGetSystemTime());
    chMtxUnlock(&data_mtx);

    // Send data ready notification
    sc_event_msg_post(drdy, SC_EVENT_MSG_POST_FROM_NORMAL);
  }

#ifdef SC_HAS_LIS302DL
  sc_lis302dl_shutdown();
#elif defined(SC_HAS_LSM9DS0)
  sc_lsm9ds0_shutdown();
#else
  chDbgAssert(0, "No 9dof drivers included in the build");
#endif
}


void sc_9dof_init(void)
{
  if (running == 1) {
    chDbgAssert(0, "9DoF already running");
    return;
  }

  chMtxObjectInit(&data_mtx);
  running = 1;

  sc_9dof_thread_ptr = chThdCreateStatic(sc_9dof_thread,
                                         sizeof(sc_9dof_thread),
                                         NORMALPRIO,
                                         sc9dofThread,
                                         NULL);
}



void sc_9dof_shutdown(void)
{
  if (running == 0) {
    chDbgAssert(0, "9DoF not running");
    return;
  }

  running = 0;

  chThdWait(sc_9dof_thread_ptr);
  sc_9dof_thread_ptr = NULL;
}



uint8_t sc_9dof_get_data(uint32_t *ret_ts,
                         sc_float *ret_acc,
                         sc_float *ret_magn,
                         sc_float *ret_gyro)
{
  uint8_t sensors_ready;

  if (running == 0) {
    chDbgAssert(0, "9DoF not running");
    return 0;
  }

  chMtxLock(&data_mtx);
  sensors_ready = sensors_valid;
  *ret_ts = ts;
  if (sensors_valid & SC_SENSOR_ACC && ret_acc) {
    memcpy(ret_acc,  acc, sizeof(acc));
  }
  if (sensors_valid & SC_SENSOR_MAGN && ret_magn) {
    memcpy(ret_magn, magn, sizeof(magn));
  }
  if (sensors_valid & SC_SENSOR_GYRO && ret_gyro) {
    memcpy(ret_gyro, gyro, sizeof(gyro));
  }
  sensors_valid = 0;
  chMtxUnlock(&data_mtx);

  return sensors_ready;
}

#endif // SC_USE_9DOF

/* Emacs indentatation information
   Local Variables:
   indent-tabs-mode:nil
   tab-width:2
   c-basic-offset:2
   End:
*/
