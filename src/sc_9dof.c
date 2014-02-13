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
#include "sc_9dof.h"
#include "sc_event.h"
#include "drivers/sc_lis302dl.h"
#include "drivers/sc_lsm9ds0.h"

#if SC_USE_9DOF

#include <string.h>

static int running = 0;
static sc_float acc[3];
static sc_float magn[3];
static sc_float gyro[3];
static uint32_t ts;
static uint16_t calibration_total;
static uint16_t calibration_i;

static Mutex data_mtx;
static Thread * sc_9dof_thread_ptr;

static WORKING_AREA(sc_9dof_thread, 512);
static msg_t sc9dofThread(void *UNUSED(arg))
{
  sc_float tmp_acc[3]  = {0, 0, 0};
  sc_float tmp_magn[3] = {0, 0, 0};
  sc_float tmp_gyro[3] = {0, 0, 0};
  sc_float gyro_calib[3] = {0, 0, 0};
  msg_t drdy;
  uint8_t warmup = 64;

  // Init sensor(s)
#ifdef SC_HAS_LIS302DL
  sc_lis302dl_init();
#elif defined(SC_HAS_LSM9DS0)
  sc_lsm9ds0_init();
#else
  chDbgAssert(0, "No 9dof drivers included in the build", "#1");
#endif

  while (running) {

    // Read data from the sensor. Assume it handles data ready signals
    // (or the appropriate sleeps) internally.
#ifdef SC_HAS_LIS302DL
    sc_lis302dl_read(tmp_acc);
    (void)tmp_magn;
#elif defined(SC_HAS_LSM9DS0)
    sc_lsm9ds0_read(tmp_acc, tmp_magn, tmp_gyro);
#else
    chDbgAssert(0, "No 9dof drivers included in the build", "#3");
#endif

    // FIXME: for some reason the initial values might contain
    // garbage. Ignore them. Why aren't they valid..?
    if (warmup) {
      --warmup;
      continue;
    }

    // Calculate average gyro offset
    if (calibration_i < calibration_total) {
      gyro_calib[0] += tmp_gyro[0];
      gyro_calib[1] += tmp_gyro[1];
      gyro_calib[2] += tmp_gyro[2];

      if (++calibration_i < calibration_total) {
        continue;
      } else {
        gyro_calib[0] /= calibration_total;
        gyro_calib[1] /= calibration_total;
        gyro_calib[2] /= calibration_total;
      }
    }

    // Adjust gyro reading based on the average offset
    tmp_gyro[0] -= gyro_calib[0];
    tmp_gyro[1] -= gyro_calib[1];
    tmp_gyro[2] -= gyro_calib[2];

    chMtxLock(&data_mtx);
    memcpy(acc,  tmp_acc,  sizeof(tmp_acc));
    memcpy(magn, tmp_magn, sizeof(tmp_magn));
    memcpy(gyro, tmp_gyro, sizeof(tmp_gyro));
    chDbgAssert(CH_FREQUENCY == 1000, "Assuming ChTimeNow() returns milliseconds", "#1");
    ts   = chTimeNow();
    chMtxUnlock();

    // Create and send data ready notification
    drdy = sc_event_msg_create_type(SC_EVENT_TYPE_9DOF_AVAILABLE);
    sc_event_msg_post(drdy, SC_EVENT_MSG_POST_FROM_NORMAL);
  }

#ifdef SC_HAS_LIS302DL
  sc_lis302dl_shutdown();
#elif defined(SC_HAS_LSM9DS0)
  sc_lsm9ds0_shutdown();
#else
  chDbgAssert(0, "No 9dof drivers included in the build", "#2");
#endif

  return 0;
}


void sc_9dof_init(uint16_t calibration_samples)
{
  if (running == 1) {
    chDbgAssert(0, "9DoF already running", "#1");
    return;
  }

  chMtxInit(&data_mtx);
  calibration_total = calibration_samples;
  calibration_i = 0;
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
    chDbgAssert(0, "9DoF not running", "#1");
    return;
  }

  running = 0;

  chThdWait(sc_9dof_thread_ptr);
  sc_9dof_thread_ptr = NULL;
}



void sc_9dof_get_data(uint32_t *ret_ts,
                      sc_float *ret_acc,
                      sc_float *ret_magn,
                      sc_float *ret_gyro)
{
  if (running == 0) {
    chDbgAssert(0, "9DoF not running", "#2");
    return;
  }

  chMtxLock(&data_mtx);
  *ret_ts = ts;
  memcpy(ret_acc,  acc, sizeof(acc));
  memcpy(ret_magn, magn, sizeof(magn));
  memcpy(ret_gyro, gyro, sizeof(gyro));
  chMtxUnlock();
}

#endif // SC_USE_9DOF

/* Emacs indentatation information
   Local Variables:
   indent-tabs-mode:nil
   tab-width:2
   c-basic-offset:2
   End:
*/
