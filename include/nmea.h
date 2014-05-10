/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2014 Tomi Hautakoski
 *               2014 Seppo Takalo
 *               2014 Tuomas Kulve
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
#include "sc_utils.h"
#ifdef SC_USE_NMEA

#ifndef _NMEA_H_
#define _NMEA_H_

/* GPS Fix type */
typedef enum {
    GPS_FIX_TYPE_NONE = 1,
    GPS_FIX_TYPE_2D   = 2,
    GPS_FIX_TYPE_3D   = 3
} fix_t;

/* Date and time data */
typedef struct _nmea_datetime {
    uint8_t  hh;
    uint8_t  mm;
    uint8_t  sec;
    uint16_t msec;
    uint8_t  day;
    uint8_t  month;
    uint16_t year;
} nmea_datetime;

/* Location data */
struct nmea_data_t {
    fix_t   fix_type;         /* Satellite fix type */
    uint8_t n_satellites;     /* Number of satellites in view */
    double   lat;              /* Latitude */
    double   lon;
    double   speed;
    double   heading;
    double   altitude;
    double   pdop;
    double   hdop;
    double   vdop;
    nmea_datetime dt;
    systime_t last_update;
    uint8_t msgs_received;
};


/******** API ***************/

/**
 * Start GPS module and processing
 */
void nmea_start(void);

/**
 * Stop GPS processing and shuts down the module
 */
void nmea_stop(void);

/**
 * Parse byte of NMEA data.
 * Returns true when a new data is available.
 */
bool nmea_parse_byte(uint8_t c);

/**
 * Request GPS data.
 * \return nmea_data_t structure.
 */
struct nmea_data_t nmea_get_data(void);

#endif

#endif // SC_USE_NMEA

/* Emacs indentatation information
   Local Variables:
   indent-tabs-mode:nil
   tab-width:4
   c-basic-offset:4
   End:
*/
