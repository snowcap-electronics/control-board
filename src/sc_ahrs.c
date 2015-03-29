/***
 * AHRS functions
 *
 * The AHRS math is GPL, Copyright Sebastian Madgwick and x-io.co.uk.
 *
 * For the rest:
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
#define SC_LOG_MODULE_TAG SC_LOG_MODULE_UNSPECIFIED

#include "sc_utils.h"
#include "sc_event.h"
#include "sc_9dof.h"
#include "sc_ahrs.h"
#include "sc_log.h"

#include <string.h>       // memcpy
#include <math.h>         // atan2 etc.

#ifdef SC_USE_AHRS

static mutex_t ahrs_mtx;
static binary_semaphore_t ahrs_new_data_sem;

static uint8_t running = 0;

static uint32_t new_ts;
static sc_float new_acc[3];
static sc_float new_magn[3];
static sc_float new_gyro[3];
static uint32_t latest_ts = 0;
static sc_float latest_roll = 0;
static sc_float latest_pitch = 0;
static sc_float latest_yaw = 0;

static sc_float beta;

#ifdef SC_ALLOW_GPL
// From MadgwickAHRS.[ch]
static sc_float q0 = 1, q1 = 0, q2 = 0, q3 = 0; // quaternion of sensor frame relative to auxiliary frame
static sc_float invSqrt(sc_float x);
static void q_init(sc_float *acc, sc_float *magn);
static void MadgwickAHRSupdate(sc_float dt,
                               sc_float gx, sc_float gy, sc_float gz,
                               sc_float ax, sc_float ay, sc_float az,
                               sc_float mx, sc_float my, sc_float mz);
#endif

static thread_t * sc_ahrs_thread_ptr;

static THD_WORKING_AREA(sc_ahrs_thread, 2048);
THD_FUNCTION(scAhrsThread, arg)
{
  msg_t drdy;
  uint8_t initialised = 0;

  (void)arg;

  chRegSetThreadName(__func__);

  // Create data ready notification
  drdy = sc_event_msg_create_type(SC_EVENT_TYPE_AHRS_AVAILABLE);

  while (running) {
    uint32_t ts;
    sc_float acc[3];
    sc_float magn[3];
    sc_float gyro[3];
    sc_float roll = 0, pitch = 0, yaw = 0;
    sc_float dt;

    chBSemWait(&ahrs_new_data_sem);

    if (!running) {
      return 0;
    }

    if (latest_ts == 0) {
      latest_ts = new_ts;
      continue;
    }

    // Copy global data for local handling.
    chMtxLock(&ahrs_mtx);
    ts = new_ts;
    memcpy(acc, new_acc, sizeof(new_acc));
    memcpy(magn, new_magn, sizeof(new_magn));
    memcpy(gyro, new_gyro, sizeof(new_gyro));
    dt = (new_ts - latest_ts) / 1000.0;
    chMtxUnlock(&ahrs_mtx);

#ifdef SC_ALLOW_GPL
    // Should be rare cases, so just ignore values that would lead to NaN
    if ((magn[0] == 0.0f && magn[1] == 0.0f && magn[2] == 0.0f) ||
        (acc[0] == 0.0f && acc[1] == 0.0f && acc[2] == 0.0f)) {
      continue;
    }

    if (!initialised) {
      q_init(acc, magn);
      initialised = 1;
    } else {
      // Degrees to radians
      gyro[0] *= M_PI / 180.0;
      gyro[1] *= M_PI / 180.0;
      gyro[2] *= M_PI / 180.0;

      // http://www.x-io.co.uk/open-source-imu-and-ahrs-algorithms/
      MadgwickAHRSupdate(dt,
                         gyro[0], gyro[1], gyro[2],
                         acc[0], acc[1], acc[2],
                         magn[0], magn[1], magn[2]);
    }

    // http://stackoverflow.com/questions/11492299/quaternion-to-euler-angles-algorithm-how-to-convert-to-y-up-and-between-ha
    // http://en.wikipedia.org/wiki/Conversion_between_quaternions_and_Euler_angles
    // Creative Commons Attribution-ShareAlike License
    roll = (sc_float)atan2(2.0f * q1 * q0 + 2.0f * q2 * q3, 1 - 2.0f * (q3*q3  + q0*q0));
    pitch = (sc_float)asin(2.0f * ( q1 * q3 - q0 * q2 ) );
    yaw = (sc_float)atan2(2.0f * q1 * q2 + 2.0f * q3 * q0, 1 - 2.0f * (q2*q2 + q3*q3));

    // Radians to degrees
    yaw   *= 180 / M_PI;
    pitch *= 180 / M_PI;
    roll  *= 180 / M_PI;
#else
    (void)dt;
    chDbgAssert(0, "Only GPL licensed AHRS supported currently");
#endif

    // Store latest values for later use
    chMtxLock(&ahrs_mtx);
    latest_ts = ts;
    latest_roll = roll;
    latest_pitch = pitch;
    latest_yaw = yaw;
    chMtxUnlock(&ahrs_mtx);

    // Send data ready notification
    sc_event_msg_post(drdy, SC_EVENT_MSG_POST_FROM_NORMAL);
  }
}


void sc_ahrs_init(sc_float user_beta)
{
  if (running == 1) {
    chDbgAssert(0, "AHRS already running");
    return;
  }

  chMtxObjectInit(&ahrs_mtx);
  chBSemObjectInit(&ahrs_new_data_sem, TRUE);

  running = 1;
  beta = user_beta;

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
    chDbgAssert(0, "AHRS not running");
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
    chDbgAssert(0, "AHRS not running");
    return;
  }

  chMtxLock(&ahrs_mtx);
  new_ts = ts;
  memcpy(new_acc, acc, sizeof(new_acc));
  memcpy(new_magn, magn, sizeof(new_magn));
  memcpy(new_gyro, gyro, sizeof(new_gyro));
  chMtxUnlock(&ahrs_mtx);

  chBSemSignal(&ahrs_new_data_sem);
}




void sc_ahrs_get_orientation(uint32_t *ts,
                             sc_float *roll,
                             sc_float *pitch,
                             sc_float *yaw)
{
  if (running == 0) {
    chDbgAssert(0, "AHRS not running");
    return;
  }

  chMtxLock(&ahrs_mtx);
  *ts = latest_ts;
  *roll = latest_roll;
  *pitch = latest_pitch;
  *yaw = latest_yaw;
  chMtxUnlock(&ahrs_mtx);
}



#ifdef SC_ALLOW_GPL

static void vector_normalise(sc_float *a)
{
  sc_float recipNorm;

  recipNorm = invSqrt(a[0] * a[0] + a[1] * a[1] + a[2] * a[2]);
  a[0] *= recipNorm;
  a[1] *= recipNorm;
  a[2] *= recipNorm;
}

// From http://sourceforge.net/p/ce10dofahrs/code/ci/b2f7428b98ba09953f4a2cfadd6e64960070b2ad/tree/vector.h#l125
static void vector_cross(sc_float a[3], sc_float b[3], sc_float ret[3])
{
  ret[0] = (a[1] * b[2]) - (a[2] * b[1]);
  ret[1] = (a[2] * b[0]) - (a[0] * b[2]);
  ret[2] = (a[0] * b[1]) - (a[1] * b[0]);
}

// From http://sourceforge.net/p/ce10dofahrs/code/ci/b2f7428b98ba09953f4a2cfadd6e64960070b2ad/tree/ahrs.cpp#l13
static void q_init(sc_float acc[3], sc_float magn[3])
{
  sc_float down[3];
  sc_float east[3];
  sc_float north[3];

  // TODO: no need to invert? (Was inverted in the zip file)
  down[0] = acc[0];
  down[1] = acc[1];
  down[2] = acc[2];
  vector_cross(down, magn, east);
  vector_cross(east, down, north);

  vector_normalise(down);
  vector_normalise(east);
  vector_normalise(north);

  // (down, east, north) => (q0, q1, q2, q3)
  // From http://sourceforge.net/p/ce10dofahrs/code/ci/b2f7428b98ba09953f4a2cfadd6e64960070b2ad/tree/quaternion.h#l112
  {

    float tr = north[0] + east[1] + down[2];
    float S = 0.0;
    if (tr > 0) {
      S = sqrt(tr+1.0) * 2;
      q0 = 0.25 * S;
      q1 = (down[1] - east[2]) / S;
      q2 = (north[2] - down[0]) / S;
      q3 = (east[0] - north[1]) / S;
    } else if ((north[0] < east[1]) && (north[0] < down[2])) {
      S = sqrt(1.0 + north[0] - east[1] - down[2]) * 2;
      q0 = (down[1] - east[2]) / S;
      q1 = 0.25 * S;
      q2 = (north[1] + east[0]) / S;
      q3 = (north[2] + down[0]) / S;
    } else if (east[1] < down[2]) {
      S = sqrt(1.0 + east[1] - north[0] - down[2]) * 2;
      q0 = (north[2] - down[0]) / S;
      q1 = (north[1] + east[0]) / S;
      q2 = 0.25 * S;
      q3 = (east[2] + down[1]) / S;
    } else {
      S = sqrt(1.0 + down[2] - north[0] - east[1]) * 2;
      q0 = (east[0] - north[1]) / S;
      q1 = (north[2] + down[0]) / S;
      q2 = (east[2] + down[1]) / S;
      q3 = 0.25 * S;
    }
  }
}

// From http://www.x-io.co.uk/open-source-imu-and-ahrs-algorithms/ (MadgwickAHRS.c)
// Modified to use time stamps
static void MadgwickAHRSupdate(sc_float dt,
                               sc_float gx, sc_float gy, sc_float gz,
                               sc_float ax, sc_float ay, sc_float az,
                               sc_float mx, sc_float my, sc_float mz)
{
  sc_float recipNorm;
  sc_float s0, s1, s2, s3;
  sc_float qDot1, qDot2, qDot3, qDot4;
  sc_float hx, hy;
  sc_float _2q0mx, _2q0my, _2q0mz, _2q1mx, _2bx, _2bz, _4bx, _4bz, _8bx, _8bz;
  sc_float _2q0, _2q1, _2q2, _2q3, q0q0, q0q1, q0q2, q0q3;
  sc_float q1q1, q1q2, q1q3, q2q2, q2q3, q3q3;

  // Rate of change of quaternion from gyroscope
  qDot1 = 0.5f * (-q1 * gx - q2 * gy - q3 * gz);
  qDot2 = 0.5f * (q0 * gx + q2 * gz - q3 * gy);
  qDot3 = 0.5f * (q0 * gy - q1 * gz + q3 * gx);
  qDot4 = 0.5f * (q0 * gz + q1 * gy - q2 * gx);

  if(1) {

    // Normalise accelerometer measurement
    recipNorm = invSqrt(ax * ax + ay * ay + az * az);
    ax *= recipNorm;
    ay *= recipNorm;
    az *= recipNorm;

    // Normalise magnetometer measurement
    recipNorm = invSqrt(mx * mx + my * my + mz * mz);
    mx *= recipNorm;
    my *= recipNorm;
    mz *= recipNorm;

    // Auxiliary variables to avoid repeated arithmetic
    _2q0mx = 2.0f * q0 * mx;
    _2q0my = 2.0f * q0 * my;
    _2q0mz = 2.0f * q0 * mz;
    _2q1mx = 2.0f * q1 * mx;
    _2q0 = 2.0f * q0;
    _2q1 = 2.0f * q1;
    _2q2 = 2.0f * q2;
    _2q3 = 2.0f * q3;
    q0q0 = q0 * q0;
    q0q1 = q0 * q1;
    q0q2 = q0 * q2;
    q0q3 = q0 * q3;
    q1q1 = q1 * q1;
    q1q2 = q1 * q2;
    q1q3 = q1 * q3;
    q2q2 = q2 * q2;
    q2q3 = q2 * q3;
    q3q3 = q3 * q3;

    // Updated from http://diydrones.com/xn/detail/705844:Comment:1755084
		// Reference direction of Earth's magnetic field
    hx = mx * q0q0 - _2q0my * q3 + _2q0mz * q2 + mx * q1q1 + _2q1 * my * q2 + _2q1 * mz * q3 - mx * q2q2 - mx * q3q3;
    hy = _2q0mx * q3 + my * q0q0 - _2q0mz * q1 + _2q1mx * q2 - my * q1q1 + my * q2q2 + _2q2 * mz * q3 - my * q3q3;
    _2bx = sqrt(hx * hx + hy * hy);
    _2bz = -_2q0mx * q2 + _2q0my * q1 + mz * q0q0 + _2q1mx * q3 - mz * q1q1 + _2q2 * my * q3 - mz * q2q2 + mz * q3q3;
    _4bx = 2.0f * _2bx;
    _4bz = 2.0f * _2bz;
    _8bx = 2.0f * _4bx;
    _8bz = 2.0f * _4bz;

    // Gradient decent algorithm corrective step
    s0= -_2q2*(2.0f*(q1q3 - q0q2) - ax)    +   _2q1*(2.0f*(q0q1 + q2q3) - ay)   +  -_4bz*q2*(_4bx*(0.5 - q2q2 - q3q3) + _4bz*(q1q3 - q0q2) - mx)   +   (-_4bx*q3+_4bz*q1)*(_4bx*(q1q2 - q0q3) + _4bz*(q0q1 + q2q3) - my)    +   _4bx*q2*(_4bx*(q0q2 + q1q3) + _4bz*(0.5 - q1q1 - q2q2) - mz);
    s1= _2q3*(2.0f*(q1q3 - q0q2) - ax) +   _2q0*(2.0f*(q0q1 + q2q3) - ay) +   -4.0f*q1*(2.0f*(0.5 - q1q1 - q2q2) - az)    +   _4bz*q3*(_4bx*(0.5 - q2q2 - q3q3) + _4bz*(q1q3 - q0q2) - mx)   + (_4bx*q2+_4bz*q0)*(_4bx*(q1q2 - q0q3) + _4bz*(q0q1 + q2q3) - my)   +   (_4bx*q3-_8bz*q1)*(_4bx*(q0q2 + q1q3) + _4bz*(0.5 - q1q1 - q2q2) - mz);             
    s2= -_2q0*(2.0f*(q1q3 - q0q2) - ax)    +     _2q3*(2.0f*(q0q1 + q2q3) - ay)   +   (-4.0f*q2)*(2.0f*(0.5 - q1q1 - q2q2) - az) +   (-_8bx*q2-_4bz*q0)*(_4bx*(0.5 - q2q2 - q3q3) + _4bz*(q1q3 - q0q2) - mx)+(_4bx*q1+_4bz*q3)*(_4bx*(q1q2 - q0q3) + _4bz*(q0q1 + q2q3) - my)+(_4bx*q0-_8bz*q2)*(_4bx*(q0q2 + q1q3) + _4bz*(0.5 - q1q1 - q2q2) - mz);
    s3= _2q1*(2.0f*(q1q3 - q0q2) - ax) +   _2q2*(2.0f*(q0q1 + q2q3) - ay)+(-_8bx*q3+_4bz*q1)*(_4bx*(0.5 - q2q2 - q3q3) + _4bz*(q1q3 - q0q2) - mx)+(-_4bx*q0+_4bz*q2)*(_4bx*(q1q2 - q0q3) + _4bz*(q0q1 + q2q3) - my)+(_4bx*q1)*(_4bx*(q0q2 + q1q3) + _4bz*(0.5 - q1q1 - q2q2) - mz);

    recipNorm = invSqrt(s0 * s0 + s1 * s1 + s2 * s2 + s3 * s3); // normalise step magnitude
    s0 *= recipNorm;
    s1 *= recipNorm;
    s2 *= recipNorm;
    s3 *= recipNorm;

    // Apply feedback step
    qDot1 -= beta * s0;
    qDot2 -= beta * s1;
    qDot3 -= beta * s2;
    qDot4 -= beta * s3;
	}

  // Integrate rate of change of quaternion to yield quaternion
  q0 += qDot1 * dt;
  q1 += qDot2 * dt;
  q2 += qDot3 * dt;
  q3 += qDot4 * dt;

  // Normalise quaternion
  recipNorm = invSqrt(q0 * q0 + q1 * q1 + q2 * q2 + q3 * q3);
  q0 *= recipNorm;
  q1 *= recipNorm;
  q2 *= recipNorm;
  q3 *= recipNorm;
}



// From https://github.com/TobiasSimon/MadgwickTests/blob/4f76ef1475219bedbdba7afab297e3468d9e7c44/MadgwickAHRS.c
static sc_float invSqrt(sc_float x)
{

  const int instability_fix = 1;

  if (instability_fix == 0) {
    /* original code */
    sc_float halfx = 0.5f * x;
    sc_float y = x;
    long i = *(long*)&y;
    i = 0x5f3759df - (i>>1);
    y = *(sc_float*)&i;
    y = y * (1.5f - (halfx * y * y));
    return y;
  } else if (instability_fix == 1) {
    /* close-to-optimal method with low cost from http://pizer.wordpress.com/2008/10/12/fast-inverse-square-root */
    unsigned int i = 0x5F1F1412 - (*(unsigned int*)&x >> 1);
    sc_float tmp = *(sc_float*)&i;
    return tmp * (1.69000231f - 0.714158168f * x * tmp * tmp);
  }
  else {
    /* optimal but expensive method: */
    return 1.0f / sqrtf(x);
  }
}
#endif // SC_ALLOW_GPL
#endif // SC_USE_AHRS



/* Emacs indentatation information
   Local Variables:
   indent-tabs-mode:nil
   tab-width:2
   c-basic-offset:2
   End:
*/
