/***
 * AHRS functions
 *
 * Copyright 2013-2014 Tuomas Kulve, <tuomas.kulve@snowcap.fi>
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
#include "sc_event.h"
#include "sc_9dof.h"
#include "sc_ahrs.h"
#ifdef SC_ALLOW_GPL
#include "uimu_ahrs_glue.h"
#endif

#include <string.h>       // memcpy

static Mutex ahrs_mtx;
static BinarySemaphore ahrs_new_data_sem;

static uint8_t running = 0;

static uint32_t new_ts;
static sc_float new_acc[3];
static sc_float new_magn[3];
static sc_float new_gyro[3];
static uint32_t latest_ts;
static sc_float latest_roll;
static sc_float latest_pitch;
static sc_float latest_yaw;

static Thread * sc_ahrs_thread_ptr;

static WORKING_AREA(sc_ahrs_thread, 1024);
static msg_t scAhrsThread(void *UNUSED(arg))
{
  msg_t drdy;
  uint8_t first = 1;

  // Create data ready notification
  drdy = sc_event_msg_create_type(SC_EVENT_TYPE_AHRS_AVAILABLE);

  while (running) {
    uint32_t ts;
    sc_float acc[3];
    sc_float magn[3];
    sc_float gyro[3];
    sc_float roll = 0, pitch = 0, yaw = 0;

    chBSemWait(&ahrs_new_data_sem);

    if (!running) {
      return 0;
    }

    // Copy global data for local handling.
    chMtxLock(&ahrs_mtx);
    ts = new_ts;
    memcpy(acc, new_acc, sizeof(new_acc));
    memcpy(magn, new_magn, sizeof(new_magn));
    memcpy(gyro, new_gyro, sizeof(new_gyro));
    chMtxUnlock();

#ifdef SC_ALLOW_GPL
    if (first) {
      uimu_ahrs_glue_init(ts, acc, magn);
      uimu_ahrs_glue_set_beta(0.2);
      first = 0;
      continue;
    }

    uimu_ahrs_glue_iterate(ts, acc, magn, gyro);
    uimu_ahrs_glue_get_euler(&roll, &pitch, &yaw);
#else
    (void)first;
    chDbgAssert(0, "Only GPL licensed AHRS supported currently", "#1");
#endif

    // Store latest values for later use
    chMtxLock(&ahrs_mtx);
    latest_ts = ts;
    latest_roll = roll;
    latest_pitch = pitch;
    latest_yaw = yaw;
    chMtxUnlock();

    // Send data ready notification
    sc_event_msg_post(drdy, SC_EVENT_MSG_POST_FROM_NORMAL);
  }
  return 0;
}


void sc_ahrs_init(void)
{
  if (running == 1) {
    chDbgAssert(0, "AHRS already running", "#1");
    return;
  }

  chMtxInit(&ahrs_mtx);
  chBSemInit(&ahrs_new_data_sem, TRUE);

  running = 1;

  // Heavy math, setting low priority
  sc_ahrs_thread_ptr = chThdCreateStatic(sc_ahrs_thread,
                                         sizeof(sc_ahrs_thread),
                                         LOWPRIO,
                                         scAhrsThread,
                                         NULL);
}



void sc_ahrs_shutdown(void)
{

  if (running == 0) {
    chDbgAssert(0, "AHRS not running", "#1");
    return;
  }

  running = 0;
  chBSemSignalI(&ahrs_new_data_sem);

  chThdWait(sc_ahrs_thread_ptr);
  sc_ahrs_thread_ptr = NULL;
}



void sc_ahrs_push_9dof(uint32_t ts,
                       sc_float *acc,
                       sc_float *magn,
                       sc_float *gyro)
{
  if (running == 0) {
    chDbgAssert(0, "AHRS not running", "#2");
    return;
  }

  chMtxLock(&ahrs_mtx);
  new_ts = ts;
  memcpy(new_acc, acc, sizeof(new_acc));
  memcpy(new_magn, magn, sizeof(new_magn));
  memcpy(new_gyro, gyro, sizeof(new_gyro));
  chMtxUnlock();

  chBSemSignal(&ahrs_new_data_sem);
}




void sc_ahrs_get_orientation(uint32_t *ts,
                             sc_float *roll,
                             sc_float *pitch,
                             sc_float *yaw)
{
  if (running == 0) {
    chDbgAssert(0, "AHRS not running", "#3");
    return;
  }

  chMtxLock(&ahrs_mtx);
  *ts = latest_ts;
  *roll = latest_roll;
  *pitch = latest_pitch;
  *yaw = latest_yaw;
  chMtxUnlock();
}


/* Emacs indentatation information
   Local Variables:
   indent-tabs-mode:nil
   tab-width:2
   c-basic-offset:2
   End:
*/
