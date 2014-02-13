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

#ifndef UIMU_AHRS_GLUE_H
#define UIMU_AHRS_GLUE_H

#include "sc_utils.h"

#include <stdint.h>

#ifdef SC_USE_AHRS
#ifdef SC_ALLOW_GPL

#ifdef __cplusplus
extern "C" {
#endif

void uimu_ahrs_glue_init(uint32_t ts,
                         sc_float *new_acc,
                         sc_float *new_magn);

void uimu_ahrs_glue_set_beta(float beta);

void uimu_ahrs_glue_iterate(uint32_t ts,
                            sc_float *new_acc,
                            sc_float *new_magn,
                            sc_float *new_gyro);

void uimu_ahrs_glue_get_euler(sc_float *roll, sc_float *pitch, sc_float *yaw);

#ifdef __cplusplus
}
#endif
#endif // SC_ALLOW_GPL
#endif // SC_USE_AHRS
#endif // UIMU_AHRS_GLUE_H

/* Emacs indentatation information
   Local Variables:
   indent-tabs-mode:nil
   tab-width:2
   c-basic-offset:2
   End:
*/
