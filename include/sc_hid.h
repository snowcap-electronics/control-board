/*
  Modified for Snowcap projects
  Copyright (C) 2014 Tuomas Kulve <tuomas.kulve@snowcap.fi>
*/
/*
  USB-HID Gamepad for ChibiOS/RT
  Copyright (C) 2014, +inf Wenzheng Xu.

  EMAIL: wx330@nyu.edu

  This piece of code is FREE SOFTWARE and is released under the terms
  of the GNU General Public License <http://www.gnu.org/licenses/>
*/

/*
  ChibiOS/RT - Copyright (C) 2006,2007,2008,2009,2010,
  2011,2012 Giovanni Di Sirio.

  This file is part of ChibiOS/RT.

  ChibiOS/RT is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 3 of the License, or
  (at your option) any later version.

  ChibiOS/RT is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef SC_HID_H
#define SC_HID_H

#include "sc_utils.h"

#if HAL_USE_USB

typedef struct {
  int8_t x;
  int8_t y;
  uint8_t button;
} hid_data;

void hid_transmit(hid_data *data);
void sc_hid_init(void);
void sc_hid_deinit(void);

#endif /* HAL_USE_USB */

#endif



/* Emacs indentatation information
   Local Variables:
   indent-tabs-mode:nil
   tab-width:2
   c-basic-offset:2
   End:
*/
