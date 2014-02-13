/***
 * C glue code for C++ code from:
 * Inertial Measurement Unit Maths Library
 * Copyright (C) 2013-2014  Samuel Cowen
 * www.camelsoftware.com
 *
 * Copyright 2014 Tuomas Kulve <tuomas.kulve@snowcap.fi>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "sc_utils.h"

#include <math.h>

#ifdef SC_USE_AHRS
#ifdef SC_ALLOW_GPL

#include "uimu_ahrs_glue.h"
#include "src/imumaths/imumaths.h" // Includes bunch of c++ class implementations
#include "src/imumaths/ahrs.cpp"

void uimu_ahrs_glue_init(uint32_t ts,
                         sc_float *new_acc,
                         sc_float *new_magn)
{
  imu::Vector<3> acc;
  imu::Vector<3> mag;

  acc.x() = new_acc[0];
  acc.y() = new_acc[1];
  acc.z() = new_acc[2];

  mag.x() = new_magn[0];
  mag.y() = new_magn[1];
  mag.z() = new_magn[2];

  uimu_ahrs_init(ts, acc, mag);
}

void uimu_ahrs_glue_set_beta(float beta)
{
  uimu_ahrs_set_beta(beta);
}

void uimu_ahrs_glue_iterate(uint32_t ts,
                            sc_float *new_acc,
                            sc_float *new_magn,
                            sc_float *new_gyro)
{
  imu::Vector<3> acc;
  imu::Vector<3> ang_vel;
  imu::Vector<3> mag;

  acc.x() = new_acc[0];
  acc.y() = new_acc[1];
  acc.z() = new_acc[2];

  ang_vel.x() = new_gyro[0];
  ang_vel.y() = new_gyro[1];
  ang_vel.z() = new_gyro[2];
  ang_vel.toRadians();

  mag.x() = new_magn[0];
  mag.y() = new_magn[1];
  mag.z() = new_magn[2];

  uimu_ahrs_iterate(ts, acc, ang_vel, mag);
}

void uimu_ahrs_glue_get_euler(sc_float *roll, sc_float *pitch, sc_float *yaw)
{
  imu::Vector<3> euler = uimu_ahrs_get_euler();

  *yaw = euler.x();
  *pitch = euler.y();
  *roll = euler.z();
}

#endif // SC_ALLOW_GPL
#endif //SC_ALLOW_GPL


/* Emacs indentatation information
   Local Variables:
   indent-tabs-mode:nil
   tab-width:2
   c-basic-offset:2
   End:
*/
