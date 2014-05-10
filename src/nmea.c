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

#define SC_LOG_MODULE_TAG SC_LOG_MODULE_UNSPECIFIED

#include "ch.h"
#include "hal.h"
#include "slre.h"
#include "nmea.h"
#include "sc_log.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

//#define NO_DEBUG
#ifndef NO_DEBUG
#define printf(...) SC_DBG_PRINTF(__VA_ARGS__)

#define D_ENTER() printf("%s:%s(): enter\r\n", __FILE__, __FUNCTION__)
#define D_EXIT() printf("%s:%s(): exit\r\n", __FILE__, __FUNCTION__)
#define _DEBUG(...) do{                                       \
	  printf(__VA_ARGS__);                                    \
  } while(0)
#else
#define printf(...)
#define D_ENTER()
#define D_ENTER()
#define D_EXIT()
#endif

#define NMEA_BUFF_SIZE  128
#define GPRMC (1<<0)
#define GPGSA (1<<1)
#define GPGGA (1<<2)

/* Prototypes */
static void nmea_parse_line(const char *line, size_t len);
static bool calculate_nmea_checksum(const char *data, size_t len);
static bool parse_gpgga(const char *line, size_t len);
static bool parse_gpgsa(const char *line, size_t len);
static bool parse_gprmc(const char *line, size_t len);
static void parse_nmea_time_str(char *str, nmea_datetime *dt);
static void parse_nmea_date_str(char *str, nmea_datetime *dt);
static double nmeadeg2degree(double val);

/* Location data */
static struct nmea_data_t nmea_data;
static struct nmea_data_t nmea_data_latest;

static Mutex nmea_data_mtx;

/* ===================Internal functions begin=============================== */


/**
 * Receives lines from serial interfaces and parses them.
 */
static char buf[NMEA_BUFF_SIZE];
bool nmea_parse_byte(uint8_t c)
{
    static size_t i = 0;

    if (c == '\r' || c == '\0') {
        return false;
    }
    if ('\n' == c) { /* End of line */
        bool new_data = false;
        buf[i] = 0; /* Mark the end */
        //SC_DBG_PRINTF("nmea: new data: %s\r\n", buf);
        chMtxLock(&nmea_data_mtx);
        nmea_parse_line(buf, i);
        // FIXME: How to check that these are from the same block?
        if (nmea_data.msgs_received & (GPRMC | GPGGA | GPGSA)) {
            nmea_data_latest = nmea_data;
            nmea_data_latest.last_update = chTimeNow();
            nmea_data.msgs_received = 0;
            new_data = true;
        }
        i = 0;
        chMtxUnlock();
        return new_data;
    }

    buf[i++] = (char)c;
    if (i == NMEA_BUFF_SIZE) {
        i = 0;
        chDbgAssert(0, "NMEA incoming buffer full", "#1");
        return false;
    }

    return false;
}

/**
 * Parse received line.
 */
static void nmea_parse_line(const char *line, size_t len)
{
    if (len == 0) {
        return;
    }

    if(calculate_nmea_checksum(line, len)) {
        // FIXME: these could be optimized not to read whole lines always
        if(strstr(line, "RMC")) {         // Required minimum data
            parse_gprmc(line, len);
            nmea_data.msgs_received |= GPRMC;
        } else if(strstr(line, "GGA")) {  // Global Positioning System Fix Data
            parse_gpgga(line, len);
            nmea_data.msgs_received |= GPGGA;
        } else if(strstr(line, "GSA")) {  // GPS DOP and active satellites
            parse_gpgsa(line, len);
            nmea_data.msgs_received |= GPGSA;
        }
    }
}

static bool calculate_nmea_checksum(const char *data, size_t len)
{
    char checksum = 0;
    char received_checksum = 0;
    char *checksum_index;

    (void)len;

    if((checksum_index = strstr(data, "*")) == NULL) { // Find the beginning of checksum
        printf("GPS: error, cannot find the beginning of checksum from input line '%s'\r\n", data);
        return false;
    }
    sscanf(checksum_index + 1, "%02hhx", &received_checksum);
    // FIXME: check return value
    /*	checksum_index++;
    	*(checksum_index + 2) = 0;
    	received_checksum = strtol(checksum_index, NULL, 16);*/

    /* Loop through data, XORing each character to the next */
    data = strstr(data, "$");
    data++;
    while(*data != '*') {
        checksum ^= *data;
        data++;
    }

    if(checksum == received_checksum) {
        return true;
    } else {
        return false;
    }
}

static bool parse_gpgga(const char *line, size_t len)
{
    int n_sat = 0;
    double altitude = 0.0;
    const char *error;

    error = slre_match(0, "^\\$GPGGA,[^,]*,[^,]*,[^,]*,[^,]*,[^,]*,[^,]*,([^,]*),[^,]*,([^,]*),[^,]*",
                       line, len,
                       SLRE_INT, sizeof(n_sat), &n_sat,
                       SLRE_FLOAT, sizeof(altitude), &altitude);
    if(error != NULL) {
        if (nmea_data.fix_type == GPS_FIX_TYPE_3D) {
            printf("GPS: Error parsing GPGGA string '%s': %s\r\n", line, error);
        }
        return false;
    } else {
        if(nmea_data.n_satellites != n_sat) {
            printf("GPS: Number of satellites in view: %d\r\n", n_sat);
        }
        nmea_data.n_satellites = n_sat;
        nmea_data.altitude = altitude;
        return true;
    }
}

static bool parse_gpgsa(const char *line, size_t len)
{
    int nmea_fix_type;
    double pdop, hdop, vdop;
    const char *error;

    error = slre_match(0, "^\\$G[PN]GSA,[^,]*,([^,]*),[^,]*,[^,]*,[^,]*,[^,]*,[^,]*,[^,]*,[^,]*,[^,]*,[^,]*,[^,]*,[^,]*,[^,]*,([^,]*),([^,]*),([^,]*)",
                       line, len,
                       SLRE_INT, sizeof(nmea_fix_type), &nmea_fix_type,
                       SLRE_FLOAT, sizeof(pdop), &pdop,
                       SLRE_FLOAT, sizeof(hdop), &hdop,
                       SLRE_FLOAT, sizeof(vdop), &vdop);
    if(error != NULL) {
        // TODO: add a check for incomplete sentence
        if (nmea_data.fix_type == GPS_FIX_TYPE_3D) {
            printf("GPS: Error parsing GPGSA string '%s': %s\r\n", line, error);
        }
        return false;
    } else {
        switch(nmea_fix_type) {
        case GPS_FIX_TYPE_NONE:
            if(nmea_data.fix_type != GPS_FIX_TYPE_NONE)
                printf("GPS: fix lost\r\n");
            nmea_data.fix_type = GPS_FIX_TYPE_NONE;
            nmea_data.lat = 0.0;
            nmea_data.lon = 0.0;
            return false;
            break;
        case GPS_FIX_TYPE_2D:
            if(nmea_data.fix_type != GPS_FIX_TYPE_2D)
                printf("GPS: fix type 2D\r\n");
            nmea_data.fix_type = GPS_FIX_TYPE_2D;
            break;
        case GPS_FIX_TYPE_3D:
            if(nmea_data.fix_type != GPS_FIX_TYPE_3D)
                printf("GPS: fix type 3D\r\n");
            nmea_data.fix_type = GPS_FIX_TYPE_3D;
            break;
        default:
            chDbgAssert(0, "GPS: Error, unknown GPS fix type!", "#1");
            return false;
            break;
        }
        //postpone this until GPRMC: nmea_data.fix_type = nmea_fix_type;
        nmea_data.pdop = pdop;
        nmea_data.hdop = hdop;
        nmea_data.vdop = vdop;
        /*
        printf("GPS: pdop: %f\n", nmea_data.pdop);
        printf("GPS: hdop: %f\n", nmea_data.hdop);
        printf("GPS: vdop: %f\n", nmea_data.vdop);
        */
        return true;
    }
}

static bool parse_gprmc(const char *line, size_t len)
{
    char time[15], status[2], ns[2], ew[2], date[7];
    double lat, lon, speed_ms, heading;

    const char *error;
    error = slre_match(0, "^\\$G[PN]RMC,([^,]*),([^,]*),([^,]*),([^,]*),([^,]*),([^,]*),([^,]*),([^,]*),([^,]*),[^,]*,[^,]*,[^,]*",
                       line, len,
                       SLRE_STRING, sizeof(time), time,
                       SLRE_STRING, sizeof(status), status,
                       SLRE_FLOAT, sizeof(lat), &lat,
                       SLRE_STRING, sizeof(ns), ns,
                       SLRE_FLOAT, sizeof(lon), &lon,
                       SLRE_STRING, sizeof(ew), ew,
                       SLRE_FLOAT, sizeof(speed_ms), &speed_ms,
                       SLRE_FLOAT, sizeof(heading), &heading,
                       SLRE_STRING, sizeof(date), date);
    if(error != NULL) {
        printf("GPS: Error parsing GPRMC string '%s': %s\r\n", line, error);
        return false;
    } else {
        if(strcmp(status, "A") != 0) {
            return false;
        }
        parse_nmea_time_str(time, &nmea_data.dt);
        parse_nmea_date_str(date, &nmea_data.dt);
        nmea_data.lat = nmeadeg2degree(lat);
        if(strstr(ns, "S")) {
            nmea_data.lat = -1 * nmea_data.lat;
        }
        nmea_data.lon = nmeadeg2degree(lon);
        if(strstr(ew, "W")) {
            nmea_data.lon = -1 * nmea_data.lon;
        }
        /* Assume that we have received all required information
         * So let user API to know that we have fix */
        // FIXME: what was the point of this?
        #if 0
        switch(gps.state) {
        case STATE_HAS_2D_FIX:
            nmea_data.fix_type = GPS_FIX_TYPE_2D;
            break;
        case STATE_HAS_3D_FIX:
            nmea_data.fix_type = GPS_FIX_TYPE_3D;
            break;
        default:
            nmea_data.fix_type = GPS_FIX_TYPE_NONE;
        }
        #endif
        return true;
    }
}

// Time string should be the following format: '093222.000'
static void parse_nmea_time_str(char *str, nmea_datetime *dt)
{
    char tmp[4];

    memcpy(tmp, str, 2);
    tmp[2] = 0;
    dt->hh = strtol(tmp, NULL, 10);
    memcpy(tmp, str+2, 2);
    dt->mm = strtol(tmp, NULL, 10);
    memcpy(tmp, str+4, 2);
    dt->sec = strtol(tmp, NULL, 10);
    memcpy(tmp, str+7, 3);
    dt->msec = strtol(tmp, NULL, 10);
    tmp[3] = 0;
}

// Date string should be the following format: '140413'
static void parse_nmea_date_str(char *str, nmea_datetime *dt)
{
    char tmp[6];

    memcpy(tmp, str, 2);
    tmp[2] = 0;
    dt->day = strtol(tmp, NULL, 10);
    memcpy(tmp, str+2, 2);
    dt->month = strtol(tmp, NULL, 10);
    memcpy(tmp, str+4, 2);
    dt->year = 2000 + strtol(tmp, NULL, 10); // Its past Y2k now
}

static double nmeadeg2degree(double val)
{
    double deg = ((int)(val / 100));
    val = deg + (val - deg * 100) / 60;
    return val;
}

void nmea_start()
{
    chMtxInit(&nmea_data_mtx);
    memset(&nmea_data_latest, 0, sizeof(nmea_data_latest));
    nmea_data_latest.fix_type = GPS_FIX_TYPE_NONE;
    memset(&nmea_data, 0, sizeof(nmea_data));
    nmea_data.fix_type = GPS_FIX_TYPE_NONE;
}

void nmea_stop()
{
    // Nothing to do here
}

struct nmea_data_t nmea_get_data(void)
{
    struct nmea_data_t d;
    chMtxLock(&nmea_data_mtx);
    d = nmea_data_latest;
    chMtxUnlock();
    return d;
}

#endif // SC_USE_NMEA


/* Emacs indentatation information
   Local Variables:
   indent-tabs-mode:nil
   tab-width:4
   c-basic-offset:4
   End:
*/
