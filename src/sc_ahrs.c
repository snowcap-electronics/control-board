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
#include "sc.h"

#include <string.h>       // memcpy
#include <math.h>         // atan2 etc.

#ifdef SC_USE_AHRS

// From: https://github.com/kriswiner/LSM9DS0/blob/master/Teensy3.1/LSM9DS0-MS5637/LSM9DS0_MS5637_Mini_Add_On.ino
// global constants for 9 DoF fusion and AHRS (Attitude and Heading Reference System)
#define GYRO_ERROR_DEG   40.0f                          // gyroscope measurement error in degs/s (start at 40 deg/s)
#define GYRO_MEAS_ERROR  (M_PI * (GYRO_ERROR_DEG / 180.0f)) // gyroscope measurement error in rads/s
// There is a tradeoff in the beta parameter between accuracy and response speed.
// In the original Madgwick study, beta of 0.041 (corresponding to GyroMeasError of 2.7 degrees/s) was found to give optimal accuracy.
// However, with this value, the LSM9SD0 response time is about 10 seconds to a stable initial quaternion.
// Subsequent changes also require a longish lag time to a stable output, not fast enough for a quadcopter or robot car!
// By increasing beta (GyroMeasError) by about a factor of fifteen, the response time constant is reduced to ~2 sec
// I haven't noticed any reduction in solution accuracy. This is essentially the I coefficient in a PID control sense;
// the bigger the feedback coefficient, the faster the solution converges, usually at the expense of accuracy.
// In any case, this is the free parameter in the Madgwick filtering and fusion scheme.
// Actual beta calculated in sc_ahrs_init()

#define AHRS_LOOP_INTERVAL_ST    (MS2ST(3))

static mutex_t ahrs_mtx;

static uint8_t running = 0;
static uint8_t initialised = 0;
static sc_float new_acc[3] = {0, 0, 0};
static sc_float new_magn[3] = {0, 0, 0};
static sc_float new_gyro[3] = {0, 0, 0};
static systime_t latest_ts = 0;
static sc_float latest_roll = 0;
static sc_float latest_pitch = 0;
static sc_float latest_yaw = 0;

static sc_float beta;

#ifdef SC_ALLOW_GPL
// From MadgwickAHRS.[ch]
static sc_float q[4] = {1, 0, 0, 0}; // quaternion of sensor frame relative to auxiliary frame
static sc_float invSqrt(sc_float x);
static void q_init(sc_float *acc, sc_float *magn);

static bool MadgwickAHRSupdate(sc_float dt,
                               sc_float *gyro,
                               sc_float *acc,
                               sc_float *magn);
static void reset_state(void);

#endif

static thread_t * sc_ahrs_thread_ptr;

static THD_WORKING_AREA(sc_ahrs_thread, 2048);
THD_FUNCTION(scAhrsThread, arg)
{
  msg_t drdy;
  systime_t last_ts = 0;

  (void)arg;

  chRegSetThreadName(__func__);

  // Create data ready notification
  drdy = sc_event_msg_create_type(SC_EVENT_TYPE_AHRS_AVAILABLE);

  // FIXME: Use shouldexit() etc.
  while (running) {
    systime_t ts;
    sc_float acc[3];
    sc_float magn[3];
    sc_float gyro[3];
    sc_float roll = 0, pitch = 0, yaw = 0;
    sc_float dt;

    // We don't need to run exactly at certain interval, so sleeping
    // the same amount of ticks on every run is ok.
    chThdSleep(AHRS_LOOP_INTERVAL_ST);

    // Copy global data for local handling. Not all of them contain necessarily new data
    chMtxLock(&ahrs_mtx);
    memcpy(acc, new_acc, sizeof(new_acc));
    memcpy(magn, new_magn, sizeof(new_magn));
    memcpy(gyro, new_gyro, sizeof(new_gyro));
    chMtxUnlock(&ahrs_mtx);

    // These should really rarely be exactly zero, so ignore cases where any of the sensors are just zeros
    if ((acc[0]  == 0 && acc[1]  == 0 && acc[2]  == 0) ||
        (magn[0] == 0 && magn[1] == 0 && magn[2] == 0) ||
        (gyro[0] == 0 && gyro[1] == 0 && gyro[2] == 0)) {
      continue;
    }

    ts = chVTGetSystemTime();

#ifdef SC_ALLOW_GPL

    if (!initialised) {
      if (0) q_init(acc, magn);
      last_ts = ts;
      initialised = 1;
      continue;
    } else {
      // Degrees to radians
      gyro[0] *= M_PI / 180.0f;
      gyro[1] *= M_PI / 180.0f;
      gyro[2] *= M_PI / 180.0f;

      dt = ST2US(chVTTimeElapsedSinceX(last_ts)) / 1000000.0f;
      last_ts = ts;
      if (!MadgwickAHRSupdate(dt, gyro, acc, magn)) {
        SC_LOG_PRINTF("error: %s\r\n", __func__);
        reset_state();
        continue;
      }
    }
#else
    (void)dt;
    chDbgAssert(0, "Only GPL licensed AHRS supported currently");
#endif

    // https://github.com/kriswiner/LSM9DS0/blob/master/Teensy3.1/LSM9DS0-MS5637/LSM9DS0_MS5637_Mini_Add_On.ino#L537
    yaw   = atan2(2.0f * (q[1] * q[2] + q[0] * q[3]), q[0] * q[0] + q[1] * q[1] - q[2] * q[2] - q[3] * q[3]);
    pitch = -asin(2.0f * (q[1] * q[3] - q[0] * q[2]));
    roll  = atan2(2.0f * (q[0] * q[1] + q[2] * q[3]), q[0] * q[0] - q[1] * q[1] - q[2] * q[2] + q[3] * q[3]);
    if (!isnormal(yaw)) yaw = 0;
    if (!isnormal(pitch)) pitch = 0;
    if (!isnormal(roll)) roll = 0;
    // Radians to degrees
    yaw   *= 180 / M_PI;
    pitch *= 180 / M_PI;
    roll  *= 180 / M_PI;

    chMtxLock(&ahrs_mtx);
    // Store latest values for later use
    latest_ts = ts;
    latest_yaw = yaw;
    latest_pitch = pitch;
    latest_roll = roll;
    chMtxUnlock(&ahrs_mtx);

    sc_event_msg_post(drdy, SC_EVENT_MSG_POST_FROM_NORMAL);
  }
}


void sc_ahrs_init(void)
{
  if (running == 1) {
    chDbgAssert(0, "AHRS already running");
    return;
  }

  chMtxObjectInit(&ahrs_mtx);

  running = 1;
  beta = sqrt(3.0f / 4.0f) * GYRO_MEAS_ERROR;

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

  chThdWait(sc_ahrs_thread_ptr);
  sc_ahrs_thread_ptr = NULL;
  reset_state();
}



void sc_ahrs_push_9dof(sc_float *acc,
                       sc_float *gyro,
                       sc_float *magn)
{
  if (running == 0) {
    chDbgAssert(0, "AHRS not running");
    return;
  }

  chMtxLock(&ahrs_mtx);
  if (acc) {
    memcpy(new_acc, acc, sizeof(new_acc));
  }
  if (magn) {
    memcpy(new_magn, magn, sizeof(new_magn));
  }
  if (gyro) {
    memcpy(new_gyro, gyro, sizeof(new_gyro));
  }
  chMtxUnlock(&ahrs_mtx);
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
  *ts = ST2MS(latest_ts);
  *roll = latest_roll;
  *pitch = latest_pitch;
  *yaw = latest_yaw;
  chMtxUnlock(&ahrs_mtx);
}



#ifdef SC_ALLOW_GPL

static bool vector_normalise(uint8_t n, sc_float *a)
{
  sc_float recipNorm;
  sc_float x;

  x = a[0] * a[0] + a[1] * a[1] + a[2] * a[2];
  if (n == 4) {
    x += (a[3] * a[3]);
  }
  recipNorm = invSqrt(x);
  if (!isnormal(recipNorm) || recipNorm == 0.0f) {
    return false;
  }

  a[0] *= recipNorm;
  a[1] *= recipNorm;
  a[2] *= recipNorm;
  if (n == 4) {
    a[3] *= recipNorm;
  }
  return true;
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

  if (!vector_normalise(3, down)) return;
  if (!vector_normalise(3, east)) return;
  if (!vector_normalise(3, north)) return;

  // (down, east, north) => (q0, q1, q2, q3)
  // From http://sourceforge.net/p/ce10dofahrs/code/ci/b2f7428b98ba09953f4a2cfadd6e64960070b2ad/tree/quaternion.h#l112
  {

    float tr = north[0] + east[1] + down[2];
    float S = 0.0;
    if (tr > 0) {
      S = sqrt(tr+1.0) * 2;
      q[0] = 0.25 * S;
      q[1] = (down[1] - east[2]) / S;
      q[2] = (north[2] - down[0]) / S;
      q[3] = (east[0] - north[1]) / S;
    } else if ((north[0] < east[1]) && (north[0] < down[2])) {
      S = sqrt(1.0 + north[0] - east[1] - down[2]) * 2;
      q[0] = (down[1] - east[2]) / S;
      q[1] = 0.25 * S;
      q[2] = (north[1] + east[0]) / S;
      q[3] = (north[2] + down[0]) / S;
    } else if (east[1] < down[2]) {
      S = sqrt(1.0 + east[1] - north[0] - down[2]) * 2;
      q[0] = (north[2] - down[0]) / S;
      q[1] = (north[1] + east[0]) / S;
      q[2] = 0.25 * S;
      q[3] = (east[2] + down[1]) / S;
    } else {
      S = sqrt(1.0 + down[2] - north[0] - east[1]) * 2;
      q[0] = (east[0] - north[1]) / S;
      q[1] = (north[2] + down[0]) / S;
      q[2] = (east[2] + down[1]) / S;
      q[3] = 0.25 * S;
    }
  }
}

// http://www.x-io.co.uk/open-source-imu-and-ahrs-algorithms/
// https://raw.githubusercontent.com/kriswiner/LSM9DS0/master/Teensy3.1/LSM9DS0-MS5637/quaternionFilters.ino
static bool MadgwickAHRSupdate(sc_float dt,
                               sc_float *gyro,
                               sc_float *acc,
                               sc_float *magn)
{
  sc_float q1 = q[0], q2 = q[1], q3 = q[2], q4 = q[3];   // short name local variable for readability
  sc_float hx, hy, _2bx, _2bz;
  sc_float qDot1, qDot2, qDot3, qDot4;
  sc_float ax, ay, az;
  sc_float mx, my, mz;
  sc_float gx, gy, gz;
  sc_float s[4];

  // Auxiliary variables to avoid repeated arithmetic
  sc_float _2q1mx;
  sc_float _2q1my;
  sc_float _2q1mz;
  sc_float _2q2mx;
  sc_float _4bx;
  sc_float _4bz;
  sc_float _2q1 = 2.0f * q1;
  sc_float _2q2 = 2.0f * q2;
  sc_float _2q3 = 2.0f * q3;
  sc_float _2q4 = 2.0f * q4;
  sc_float _2q1q3 = 2.0f * q1 * q3;
  sc_float _2q3q4 = 2.0f * q3 * q4;
  sc_float q1q1 = q1 * q1;
  sc_float q1q2 = q1 * q2;
  sc_float q1q3 = q1 * q3;
  sc_float q1q4 = q1 * q4;
  sc_float q2q2 = q2 * q2;
  sc_float q2q3 = q2 * q3;
  sc_float q2q4 = q2 * q4;
  sc_float q3q3 = q3 * q3;
  sc_float q3q4 = q3 * q4;
  sc_float q4q4 = q4 * q4;

  if (!vector_normalise(3, acc)) return false;
  if (!vector_normalise(3, magn)) return false;

  ax = acc[0];
  ay = acc[1];
  az = acc[2];
  mx = magn[0];
  my = magn[1];
  mz = magn[2];
  gx = gyro[0];
  gy = gyro[1];
  gz = gyro[2];

  // Reference direction of Earth's magnetic field
  _2q1mx = 2.0f * q1 * mx;
  _2q1my = 2.0f * q1 * my;
  _2q1mz = 2.0f * q1 * mz;
  _2q2mx = 2.0f * q2 * mx;
  hx = mx * q1q1 - _2q1my * q4 + _2q1mz * q3 + mx * q2q2 + _2q2 * my * q3 + _2q2 * mz * q4 - mx * q3q3 - mx * q4q4;
  hy = _2q1mx * q4 + my * q1q1 - _2q1mz * q2 + _2q2mx * q3 - my * q2q2 + my * q3q3 + _2q3 * mz * q4 - my * q4q4;
  _2bx = sqrt(hx * hx + hy * hy);
  _2bz = -_2q1mx * q3 + _2q1my * q2 + mz * q1q1 + _2q2mx * q4 - mz * q2q2 + _2q3 * my * q4 - mz * q3q3 + mz * q4q4;
  _4bx = 2.0f * _2bx;
  _4bz = 2.0f * _2bz;

  // Gradient decent algorithm corrective step
  s[0] = -_2q3 * (2.0f * q2q4 - _2q1q3 - ax) + _2q2 * (2.0f * q1q2 + _2q3q4 - ay) - _2bz * q3 * (_2bx * (0.5f - q3q3 - q4q4) + _2bz * (q2q4 - q1q3) - mx) + (-_2bx * q4 + _2bz * q2) * (_2bx * (q2q3 - q1q4) + _2bz * (q1q2 + q3q4) - my) + _2bx * q3 * (_2bx * (q1q3 + q2q4) + _2bz * (0.5f - q2q2 - q3q3) - mz);
  s[1] = _2q4 * (2.0f * q2q4 - _2q1q3 - ax) + _2q1 * (2.0f * q1q2 + _2q3q4 - ay) - 4.0f * q2 * (1.0f - 2.0f * q2q2 - 2.0f * q3q3 - az) + _2bz * q4 * (_2bx * (0.5f - q3q3 - q4q4) + _2bz * (q2q4 - q1q3) - mx) + (_2bx * q3 + _2bz * q1) * (_2bx * (q2q3 - q1q4) + _2bz * (q1q2 + q3q4) - my) + (_2bx * q4 - _4bz * q2) * (_2bx * (q1q3 + q2q4) + _2bz * (0.5f - q2q2 - q3q3) - mz);
  s[2] = -_2q1 * (2.0f * q2q4 - _2q1q3 - ax) + _2q4 * (2.0f * q1q2 + _2q3q4 - ay) - 4.0f * q3 * (1.0f - 2.0f * q2q2 - 2.0f * q3q3 - az) + (-_4bx * q3 - _2bz * q1) * (_2bx * (0.5f - q3q3 - q4q4) + _2bz * (q2q4 - q1q3) - mx) + (_2bx * q2 + _2bz * q4) * (_2bx * (q2q3 - q1q4) + _2bz * (q1q2 + q3q4) - my) + (_2bx * q1 - _4bz * q3) * (_2bx * (q1q3 + q2q4) + _2bz * (0.5f - q2q2 - q3q3) - mz);
  s[3] = _2q2 * (2.0f * q2q4 - _2q1q3 - ax) + _2q3 * (2.0f * q1q2 + _2q3q4 - ay) + (-_4bx * q4 + _2bz * q2) * (_2bx * (0.5f - q3q3 - q4q4) + _2bz * (q2q4 - q1q3) - mx) + (-_2bx * q1 + _2bz * q3) * (_2bx * (q2q3 - q1q4) + _2bz * (q1q2 + q3q4) - my) + _2bx * q2 * (_2bx * (q1q3 + q2q4) + _2bz * (0.5f - q2q2 - q3q3) - mz);

  if (!vector_normalise(4, s)) return false;

  // Compute rate of change of quaternion
  #define GYRO_AFFECT 0.5f
  qDot1 = GYRO_AFFECT * (-q2 * gx - q3 * gy - q4 * gz) - beta * s[0];
  qDot2 = GYRO_AFFECT * (q1 * gx + q3 * gz - q4 * gy) - beta * s[1];
  qDot3 = GYRO_AFFECT * (q1 * gy - q2 * gz + q4 * gx) - beta * s[2];
  qDot4 = GYRO_AFFECT * (q1 * gz + q2 * gy - q3 * gx) - beta * s[3];
  #undef GYRO_AFFECT

  // Integrate to yield quaternion
  q1 += qDot1 * dt;
  q2 += qDot2 * dt;
  q3 += qDot3 * dt;
  q4 += qDot4 * dt;

  q[0] = q1;
  q[1] = q2;
  q[2] = q3;
  q[3] = q4;

  if (!vector_normalise(4, q)) {
#if 0
    q[0] = 1;
    q[1] = 0;
    q[2] = 0;
    q[3] = 0;
#endif
    return false;
  }

  return true;
}

// From https://github.com/TobiasSimon/MadgwickTests/blob/4f76ef1475219bedbdba7afab297e3468d9e7c44/MadgwickAHRS.c
static sc_float invSqrt(sc_float x)
{

  const int instability_fix = 2;

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
    if (sizeof(sc_float) == sizeof(float)) {
      return 1.0f / sqrtf(x);
    } else {
      return 1.0f / sqrt(x);
    }
  }
}



void sc_ahrs_set_beta(sc_float user_beta)
{
  beta = user_beta;
}



void reset_state(void)
{
  initialised = 0;
  q[0] = 1;
  q[1] = 0;
  q[2] = 0;
  q[3] = 0;
  latest_ts = 0;
  latest_roll = 0;
  latest_pitch = 0;
  latest_yaw = 0;
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
