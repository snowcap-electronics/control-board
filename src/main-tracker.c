/*
 * RuuviTracker C2
 *
 * Copyright 2011,2014 Kalle Vahlman, <kalle.vahlman@snowcap.fi>
 *                     Tuomas Kulve, <tuomas.kulve@snowcap.fi>
 *                     Seppo Takalo
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

/*
 * TRACKER CONFIGURATION
 */

/*
 *  Set these to match the server information
 */
#define TRACKER_API_SECRET            "change this"
#define TRACKER_CODE                  "change this"

/*
 * Set APN of the SIM card operator
 */
#define TRACKER_SIM_APN               "change this"

/*
 * In continuous mode the MCU, GPS and GSM are fully active all the
 * time and no explicit sleep modes are used.
 *
 * High power consumption.
 *
 * Recommended for intervals up to 15 seconds.
 */
#define TRACKER_SLEEP_MODE_CONTINUOUS 1

/*
 * In stop mode, the GPS is power gated and the MCU is stopped during
 * the waiting state.
 *
 * Optimized power consumption.
 *
 * Recommended for intervals up to a minute.
 */
#define TRACKER_SLEEP_MODE_STOP       2

/*
 * In standby mode everything except alarm clock is power gated so
 * after every power on, it takes a long time for GSM to connect to
 * the network and GPS to find the fix again.
 *
 * Lowest power consumption.
 *
 * Recommended for intervals more that a minute.
 */
#define TRACKER_SLEEP_MODE_STANDBY    3

/*
 * Choose one of the sleep modes above
 */
#define TRACKER_SLEEP_MODE            TRACKER_SLEEP_MODE_STOP


/*
 * Seconds to wait for GPS fix and GSM readiness after waking
 * up. After the timeout, the device will power off for a minute and
 * wake up again.
 *
 */
#define TRACKER_TIMEOUT_READY_SEC     120

/*
 * Location update interval. This takes into account the time spent
 * finding GSM network and GPS fix. In bad conditions that can take
 * long time and in those cases the actual sleep time may decrease
 * down to one second.
 */
#define TRACKER_INTERVAL_SEC          30

/*
 * Add offset to coordinates to "anonymize" location when developing
 */
#define DBG_LOCATION_OFFSET_LON       0.0
#define DBG_LOCATION_OFFSET_LAT       0.0

#ifdef RELEASE_BUILD
#define USE_USB 0
#else
#define USE_USB 1
#endif

/*
 * TRACKER IMPLEMENTATION
 */

#define SC_LOG_MODULE_TAG SC_LOG_MODULE_UNSPECIFIED


#include "sc.h"
#include "sha1.h"
#include "nmea.h"
#include "testplatform.h"
#include "drivers/gsm.h"

#include "chprintf.h"

#include <string.h>         // strlen

// Callbacks from the event loop
static void cb_handle_byte(SC_UART uart, uint8_t byte);
static void cb_gsm_state_changed(void);
static void cb_gsm_cmd_done(void);

static void gps_power_on(void);
static void gps_power_off(void);

#ifndef BOARD_RUUVITRACKERC2
#error "Tracker application supports only RuuviTracker HW."
#endif

#ifndef SC_USE_NMEA
#error "Tracker application uses NMEA parser."
#endif

static struct nmea_data_t nmea;

static bool has_fix = false;
static bool gsm_ready = false;
static bool gsm_cmd_idle = true;
static bool generic_error = false;

// Main worker thread
static thread_t *worker_thread = NULL;

#define SHA1_STR_LEN	40
#define SHA1_LEN	20
#define BLOCKSIZE	64
#define IPAD 0x36
#define OPAD 0x5C

#define TRACKER_USER_AGENT  "RuuviTracker (SC v2)/SIM968"

static void sha1_hmac(char *response, const char *secret,
                      const char *msg, size_t msg_len)
{
	SHA1Context sha1,sha2;
	char key[BLOCKSIZE];
	char hash[SHA1_LEN];
  char *p;
	int i;
	size_t len;

	len = strlen(secret);

	//Fill with zeroes
	memset(key, 0, BLOCKSIZE);

	if (len > BLOCKSIZE) {
		//Too long key, need to cut
    len = BLOCKSIZE;
    SC_LOG_ASSERT(0, "Secret too long");
  }

  memcpy(key, secret, len);

	//XOR key with IPAD
	for (i = 0; i < BLOCKSIZE; i++) {
		key[i] ^= IPAD;
	}

	//First SHA hash
	SHA1Reset(&sha1);
	SHA1Input(&sha1, (const unsigned char *)key, BLOCKSIZE);
	SHA1Input(&sha1, (const unsigned char *)msg, msg_len);
	SHA1Result(&sha1);

  p = hash;
	for(i = 0; i < 5; i++) {
		*p++ = (sha1.Message_Digest[i] >> 24) & 0xff;
		*p++ = (sha1.Message_Digest[i] >> 16) & 0xff;
		*p++ = (sha1.Message_Digest[i] >>  8) & 0xff;
		*p++ = (sha1.Message_Digest[i] >>  0) & 0xff;
	}

	//XOR key with OPAD
	for (i=0; i<BLOCKSIZE; i++) {
		key[i] ^= IPAD ^ OPAD;
	}

	//Second hash
	SHA1Reset(&sha2);
	SHA1Input(&sha2, (const unsigned char *)key, BLOCKSIZE);
	SHA1Input(&sha2, (const unsigned char *)hash, SHA1_LEN);
	SHA1Result(&sha2);

	for(i = 0; i < 5 ; i++) {
		chsnprintf(response + i * 8, SHA1_STR_LEN - i * 8, "%.8x", sha2.Message_Digest[i]);
	}
}

struct json_t {
  char *name;
  char *value;
};

// FIXME: convert this to enums and use index instead of strcmp
#define ELEMS_IN_EVENT 6
#define MAC_INDEX 6
struct json_t js_elems[ELEMS_IN_EVENT + 1] = {
  /* Names must be in alphabethical order */
  { "latitude",      NULL },
  { "longitude",     NULL },
  { "session_code",  NULL },
  { "time",          NULL },
  { "tracker_code",  TRACKER_CODE },
  { "version",       "1" },
  /* Except "mac" that must be last element */
  { "mac",           NULL },
};

/* Replace elements value */
static void js_replace(char *name, char *value)
{
  int i;
  for (i = 0; i < ELEMS_IN_EVENT; i++) {
	  if (strcmp(name, js_elems[i].name) == 0) {
      js_elems[i].value = value;
      break;
	  }
  }
}

static void calculate_mac(char *secret)
{
  // Must not change until the next HTTP call is finished
	static char sha[SHA1_STR_LEN + 1];
  char str[512];
  int i;
  size_t n;
  size_t str_i = 0;
  str[0] = 0;
  for (i = 0; i < ELEMS_IN_EVENT; i++) {
    str_i += chsnprintf(&str[str_i], SC_MAX(sizeof(str) - str_i, 0),
                        "%s:%s|",
                        js_elems[i].name, js_elems[i].value);

    SC_LOG_ASSERT(str_i < sizeof(str), "calculate_mac buffer full");
  }

  sha1_hmac(sha, secret, str, str_i);
  sha[SHA1_STR_LEN] = '\0';
  js_elems[MAC_INDEX].value = sha;

  SC_LOG_PRINTF("Calculated MAC %s\r\nSTR: %s\r\n", js_elems[MAC_INDEX].value, str);

  // Convert to lower case
  for (n = 0; n < SHA1_STR_LEN; ++n) {
    char *v = &(js_elems[MAC_INDEX].value[n]);
    if (*v >= 'A' && *v <= 'F') {
      *v += 32;
    }
  }
}

static int js_tostr(char *str, size_t len)
{
  int i;
  size_t str_i = 0;
  size_t tmplen = len - 4; // }\r\n\0
  str[str_i++] = '{';
  for (i = 0; i < ELEMS_IN_EVENT + 1; i++) {
    str_i += chsnprintf(&str[str_i], SC_MAX(tmplen - str_i, 0),
                        "%s\"%s\": \"%s\"",
                        i ? ", " : "",
                        js_elems[i].name, js_elems[i].value);
    SC_LOG_ASSERT(str_i < tmplen, "js_tostr buffer full");
  }
  str[str_i++] = '}';
  str[str_i++] = '\r';
  str[str_i++] = '\n';
  str[str_i] = '\0';
  return str_i;
}

static void send_event(void)
{
  // json cannot be re-used until the gst_http_post has finished
  static char json[1024];
  // code must not change ever
  static char code[128];
  static const uint8_t api_url[] = "http://dev-server.ruuvitracker.fi/api/v1-dev/events";
  static const uint8_t content_type[] = "application/json";
  char buf[1024];
  size_t buf_i = 0;
  int ret;
  char *buf_p;
  size_t json_len;
  static int first_time = 1;

  buf_p = &buf[buf_i];
  ret = chsnprintf(buf_p, SC_MAX(sizeof(buf) - buf_i, 0),
                   "%f", nmea.lat + DBG_LOCATION_OFFSET_LAT);
  if (ret > 0) {
    buf_i += ret;
  }
  json[buf_i++] = '\0';
  js_replace("latitude", buf_p);

  buf_p = &buf[buf_i];
  // Not sure if chsnprintf can return -1, but using ret to make coverity happy.
  ret = chsnprintf(buf_p, SC_MAX(sizeof(buf) - buf_i, 0),
                   "%f", nmea.lon + DBG_LOCATION_OFFSET_LON);
  if (ret > 0) {
    buf_i += ret;
  }
  json[buf_i++] = '\0';
  js_replace("longitude", buf_p);

  buf_p = &buf[buf_i];
  ret = chsnprintf(buf_p, SC_MAX(sizeof(buf) - buf_i, 0),
                   "%04d-%02d-%02dT%02d:%02d:%02d.000Z",
                   nmea.dt.year, nmea.dt.month, nmea.dt.day,
                   nmea.dt.hh, nmea.dt.mm, nmea.dt.sec);
  if (ret > 0) {
    buf_i += ret;
  }
  json[buf_i++] = '\0';
  js_replace("time", buf_p);

  if (first_time) {
    strlcpy(code, buf_p, sizeof(code));
	  js_replace("session_code", code);
	  first_time = 0;
  }

  SC_LOG_ASSERT(buf_i < sizeof(buf), "Attribute buffer full");

  calculate_mac(TRACKER_API_SECRET);
  json_len = js_tostr(json, sizeof(json));

  SC_LOG_ASSERT(json_len < sizeof(json), "JSON buffer full");

  SC_LOG_PRINTF("Sending JSON event (%d bytes):\r\n", json_len);
  SC_LOG_PRINTF("%s\r\n", json);
  ret = gsm_http_post(api_url, sizeof(api_url), (const uint8_t*)json, json_len, content_type, sizeof(content_type));
  if (!ret) {
	  SC_LOG_PRINTF("HTTP POST initiation failed\r\n");
  } else {
	  SC_LOG_PRINTF("HTTP POST initiated\r\n");
  }
}

/*
 * Setup a working area with for the worker thread
 */
static THD_WORKING_AREA(main_worker_thread, 4096);
THD_FUNCTION(mainWorkerThread, arg)
{
  const uint8_t apn[] = TRACKER_SIM_APN;
  const uint8_t user_agent[] = TRACKER_USER_AGENT;
  systime_t timeout;
  systime_t next_update = 0;

  (void)arg;

  chRegSetThreadName(__func__);

  gsm_cmd_idle = true;

  nmea_start();

  SC_LOG_PRINTF("NMEA started\r\n");
  chThdSleepMilliseconds(500);

  gps_power_on();

  SC_LOG_PRINTF("GPS started\r\n");
  chThdSleepMilliseconds(500);

  gsm_set_apn(apn);
  gsm_set_user_agent(user_agent);
  gsm_start();

  SC_LOG_PRINTF("GSM started\r\n");
  chThdSleepMilliseconds(500);

  timeout = ST2MS(chVTGetSystemTime()) + TRACKER_TIMEOUT_READY_SEC * 1000;

  // In stop and standby modes, chThdTerminate() is called after
  // send_event has succeeded. In continuous mode, gsm_cmd_idle is set to true.
  while(!chThdShouldTerminateX()) {
    systime_t now;

    now = ST2MS(chVTGetSystemTime());

    // Check for initial timeout
    if (timeout && now > timeout) {
      SC_LOG_PRINTF("Initial timeout\r\n");
      generic_error = true;
      break;
    }

    // Avoid chVTGetSystemTime wrapping over by restarting after a 10 days
    // FIXME: still true with Chibios v3.0?
    if (now > (systime_t)864000000) {
      SC_LOG_PRINTF("Long run time (%d ms), restarting\r\n", now);
      generic_error = true;
      break;
    }

    if (has_fix && gsm_cmd_idle && gsm_ready && next_update < now) {
      gsm_cmd_idle = false;
      timeout = 0;

      // Update next update time
      next_update += TRACKER_INTERVAL_SEC;
      // But don't start lagging more and more behind
      if (next_update < now) {
        next_update = now;
      }
      send_event();
    } else {
      chThdSleepMilliseconds(1000);
      SC_LOG_PRINTF("Not sending location: time left: %d, has_fix: %d, "
                    "gsm_ready: %d, gsm_cmd_idle: %d\r\n",
                    next_update - now, has_fix, gsm_ready, gsm_cmd_idle);
    }

    sc_led_toggle();
  }
  SC_LOG_PRINTF("Main thread exit, should terminate: %d\r\n", chThdShouldTerminateX());

  gsm_stop(false);
  gps_power_off();
  nmea_stop();

  return;
}

int main(void)
{
  uint32_t subsystems = SC_MODULE_UART1 | SC_MODULE_UART2 | SC_MODULE_UART3 |
    SC_MODULE_GPIO | SC_MODULE_LED;
  bool standby_mode = false;
  uint32_t sleep_time_sec;
  systime_t interval_ms;

#if USE_USB
  subsystems |= SC_MODULE_SDU;
#endif

  halInit();

  while (1) {

    /* Initialize ChibiOS core */
    chSysInit();

    tp_init();

    // Set UART config for GSM
    sc_uart_set_config(SC_UART_3, 115200, 0, 0, 0);

    // Register callbacks.
    // These will be called from Event Loop thread. It's OK to do some
    // calculations etc. time consuming there, but not to sleep or block
    // for longer periods of time.
    sc_event_register_handle_byte(cb_handle_byte);
    sc_event_register_gsm_state_changed(cb_gsm_state_changed);
    sc_event_register_gsm_cmd_done(cb_gsm_cmd_done);

    // Start event loop. This will start a new thread and return
    sc_event_loop_start();

    sc_init(subsystems);

#if USE_USB
    sc_uart_default_usb(TRUE);
    sc_log_output_uart(SC_UART_USB);
#endif

    {
      int i;
      for (i = 0; i < 20; ++i) {
        chThdSleepMilliseconds(100);
        sc_led_toggle();
      }
    }

#if USE_USB
    chThdSleepMilliseconds(3000);
    SC_LOG_PRINTF("Started up, wake up flag: 0x%x, SEVONPEND: 0x%x\r\n",
                  PWR->CSR & PWR_CSR_WUF, SCB->SCR & SCB_SCR_SEVONPEND_Msk);
    chThdSleepMilliseconds(500);
#endif

    sc_led_on();

    // Start a thread for doing the bulk work
    worker_thread = chThdCreateStatic(main_worker_thread, sizeof(main_worker_thread),
                                      NORMALPRIO, mainWorkerThread, NULL);

    chThdWait(worker_thread);
    worker_thread = NULL;

    sc_led_off();

    // FIXME: assuminen here that stopping everyting doesn't take much time

    // In stop and standby modes, current time is interval time
    interval_ms = ST2MS(chVTGetSystemTime());

#if TRACKER_SLEEP_MODE == TRACKER_SLEEP_MODE_STANDBY
    standby_mode = true;
#endif

    // Sleep always one minute in case of an error
    if (generic_error) {
      sleep_time_sec = 60;
    } else {
      // Sleep at least one second even if interval based time already passed
      if (interval_ms < TRACKER_INTERVAL_SEC * 1000) {
        sleep_time_sec = TRACKER_INTERVAL_SEC - (uint32_t)(interval_ms / 1000);
      } else {
        sleep_time_sec = 1;
      }
    }

#if USE_USB
    SC_LOG_PRINTF("%d: Going to sleep %d seconds (debug bits: 0x%x, error: %d)\r\n",
                  interval_ms, sleep_time_sec, sc_pwr_get_wfi_dbg(), generic_error);
    chThdSleepMilliseconds(1000);
#endif

    sc_deinit(subsystems);
    tp_deinit();
    sc_event_loop_stop();

    palSetPad(USER_LED2_PORT, USER_LED2);
    (void)standby_mode;
    sc_pwr_rtc_sleep(sleep_time_sec);
    palClearPad(USER_LED2_PORT, USER_LED2);
  }

  return 0;
}

static void cb_handle_byte(SC_UART uart, uint8_t byte)
{
  // NMEA from GPS
  if (uart == SC_UART_2) {
    if (nmea_parse_byte(byte)) {
      nmea = nmea_get_data();
      // FIXME: could send 2d fix if otherwise timing out.
      if (nmea.fix_type == GPS_FIX_TYPE_3D) {
        has_fix = true;
      } else {
        has_fix = false;
      }

      if (has_fix) {
        SC_DBG_PRINTF("msgs: 0x%x, fix: %d, lon: %f, lat: %f, "
                      "%04d-%02d-%02dT%02d:%02d:%02d.000Z\r\n",
                      nmea.msgs_received, nmea.fix_type, nmea.lon, nmea.lat,
                      nmea.dt.year, nmea.dt.month, nmea.dt.day,
                      nmea.dt.hh, nmea.dt.mm, nmea.dt.sec);
      }
    }
    return;
  }

  // GSM
  if (uart == SC_UART_3) {
    gsm_state_parser(byte);
    return;
  }

  // Push received byte to generic command parsing
  sc_cmd_push_byte(byte);
}


static void gps_power_on(void)
{
  palSetPad(GPIOC, GPIOC_ENABLE_LDO2);
  palSetPad(GPIOC, GPIOC_ENABLE_LDO3);
}

static void gps_power_off(void)
{
  palClearPad(GPIOC, GPIOC_ENABLE_LDO3);
  palClearPad(GPIOC, GPIOC_ENABLE_LDO2);
}



static void cb_gsm_state_changed(void)
{
  enum State state;
  enum GSM_FLAGS flags;

  gsm_state_get(&state, &flags);

  if (state == STATE_READY) {
    gsm_ready = true;
  } else {
    gsm_ready = false;
  }

  SC_LOG_PRINTF("New GSM state, ready: %d, gprs: %d\r\n",
                gsm_ready, !!(flags & GPRS_READY));
}



static void cb_gsm_cmd_done(void)
{
#if TRACKER_SLEEP_MODE == TRACKER_SLEEP_MODE_CONTINUOUS
  gsm_cmd_idle = true;
#else
  chThdTerminate(worker_thread);
#endif
  // FIXME: get return value of the gsm_cmd and if failed, set
  // generic_error and terminate the thread.
}




/* Emacs indentatation information
   Local Variables:
   indent-tabs-mode:nil
   tab-width:2
   c-basic-offset:2
   End:
*/
