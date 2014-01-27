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
static int16_t acc[3];
static int16_t magn[3];
static int16_t gyro[3];
static uint32_t ts;

static Mutex data_mtx;

static WORKING_AREA(sc_9dof_thread, 512);
static msg_t sc9dofThread(void *UNUSED(arg))
{
  int16_t tmp_acc[3]  = {0, 0, 0};
  int16_t tmp_magn[3] = {0, 0, 0};
  int16_t tmp_gyro[3] = {0, 0, 0};
  msg_t drdy;

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
    (void)tmp_gyro;
#elif defined(SC_HAS_LSM9DS0)
    sc_lsm9ds0_read(tmp_acc, tmp_magn, tmp_gyro);
#else
    chDbgAssert(0, "No 9dof drivers included in the build", "#3");
#endif

    chMtxLock(&data_mtx);
    memcpy(acc,  tmp_acc,  sizeof(tmp_acc));
    memcpy(magn, tmp_magn, sizeof(tmp_magn));
    memcpy(gyro, tmp_gyro, sizeof(tmp_gyro));
    ts   = 0; // FIXME
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


void sc_9dof_init(void)
{
  if (running == 1) {
    chDbgAssert(0, "9DoF already running", "#1");
    return;
  }

  chMtxInit(&data_mtx);

  running = 1;

  chThdCreateStatic(sc_9dof_thread, sizeof(sc_9dof_thread), NORMALPRIO, sc9dofThread, NULL);
}



void sc_9dof_shutdown(void)
{
  if (running == 0) {
    chDbgAssert(0, "9DoF not running", "#1");
    return;
  }

  running = 0;
  // FIXME: it will take a while for the sensor thread to actually
  // exit due to shutting down sensors etc. Wait?
}



void sc_9dof_get_data(uint32_t *ret_ts,
                      int16_t *ret_acc,
                      int16_t *ret_magn,
                      int16_t *ret_gyro)
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
