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

#define API_SECRET                  "ruuvitracker"
#define TRACKER_CODE                "RuuviTracker"

/*
 * Seconds to sleep between sending GPS positions to the server
 */
#define TRACKER_SLEEP_SEC   10

// FIXME: why "== TRUE" is needed?
#if RELEASE_BUILD == TRUE
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

// Main worker thread
static Thread *worker_thread = NULL;

#define SHA1_STR_LEN	40
#define SHA1_LEN	20
#define BLOCKSIZE	64
#define IPAD 0x36
#define OPAD 0x5C

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
    chDbgAssert(0, "Secret too long", "#1");
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

    chDbgAssert(str_i < sizeof(str), "calculate_mac buffer full", "#1");
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
    chDbgAssert(str_i < tmplen, "js_tostr buffer full", "#1");
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
  char *buf_p;
  size_t json_len;
  uint8_t ret;
  static int first_time = 1;

  buf_p = &buf[buf_i];
  buf_i += chsnprintf(buf_p, SC_MAX(sizeof(buf) - buf_i, 0),
                      "%f,N", (float)6446.8000/*info.lat*/);
  json[buf_i++] = '\0';
  js_replace("latitude", buf_p);

  buf_p = &buf[buf_i];
  buf_i += chsnprintf(buf_p, SC_MAX(sizeof(buf) - buf_i, 0),
                      "%f,E", (float)2521.0000/*info.lon*/);
  json[buf_i++] = '\0';
  js_replace("longitude", buf_p);

  buf_p = &buf[buf_i];
  buf_i += chsnprintf(buf_p, SC_MAX(sizeof(buf) - buf_i, 0),
                      "%04d-%02d-%02dT%02d:%02d:%02d.000Z",
                      nmea.dt.year, nmea.dt.month, nmea.dt.day,
                      nmea.dt.hh, nmea.dt.mm, nmea.dt.sec);
  json[buf_i++] = '\0';
  js_replace("time", buf_p);

  if (first_time) {
    strncpy(code, buf_p, sizeof(code));
	  js_replace("session_code", code);
	  first_time = 0;
  }

  chDbgAssert(buf_i < sizeof(buf), "Attribute buffer full", "#1");

  calculate_mac(API_SECRET);
  json_len = js_tostr(json, sizeof(json));

  chDbgAssert(json_len < sizeof(json), "JSON buffer full", "#1");

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
static WORKING_AREA(main_worker_thread, 4096);
static msg_t mainWorkerThread(void *UNUSED(arg))
{
  uint8_t apn[] = "prepaid.dna.fi";
  systime_t wait_limit;

  gsm_cmd_idle = true;

  if (!chThdShouldTerminate()) {

    gps_power_on();

    // DEBUG: Wait a while so that USB is ready to print debugs
    //chThdSleepMilliseconds(3000);

    SC_LOG_PRINTF("Started up\r\n");

    gsm_set_apn(apn);
    gsm_start();

#if 1
    // Normal operation until timeout
    wait_limit = chTimeNow() + 60000;

    // Loop sometime before sleeping
    while(!chThdShouldTerminate() && chTimeNow() < wait_limit) {

      chThdSleepMilliseconds(2000);
      if (has_fix && gsm_cmd_idle && gsm_ready) {
        gsm_cmd_idle = false;
        send_event();
      } else {
        SC_LOG_PRINTF("Not sending location: has_fix: %d, "
                      "gsm_ready: %d, gsm_cmd_idle: %d\r\n",
                      has_fix, gsm_ready, gsm_cmd_idle);
        SC_LOG_PRINTF("DBGMCU_CR 0x%08x\n", *(uint32_t*)0xe0042004);
      }

      sc_led_toggle();
    }
#else
    // GPS power testing
    while /*if*/ (1) {
      SC_LOG_PRINTF("%d, Waiting for fix\r\n", chTimeNow());
      wait_limit = chTimeNow() + 5000;
      while (chTimeNow() < wait_limit) {
        chThdSleepMilliseconds(1000);
      }

      if (has_fix) {
        uint8_t standby_on[] = "$PMTK161,1*29\r\n";
        uint8_t standby_off[] = "$PMTK161,0*28\r\n";
        SC_LOG_PRINTF("Got fix, running for 10 secs\r\n");
        wait_limit = chTimeNow() + 10000;
        while (chTimeNow() < wait_limit) {
          chThdSleepMilliseconds(1000);
        }
        SC_LOG_PRINTF("Setting GPS to Standby\r\n");
        sc_uart_send_msg(SC_UART_2, standby_on, sizeof(standby_on));
        palClearPad(GPIOC, GPIOC_ENABLE_LDO2);

        nmea_stop();

        has_fix = false;
        wait_limit = chTimeNow() + 10000;
        while (chTimeNow() < wait_limit) {
          chThdSleepMilliseconds(1000);
        }

        nmea_start();

        SC_LOG_PRINTF("%d: Waking up GPS\r\n", chTimeNow());
        palSetPad(GPIOC, GPIOC_ENABLE_LDO2);
        sc_uart_send_msg(SC_UART_2, standby_off, sizeof(standby_off));

      } else {
        SC_LOG_PRINTF("No fix\r\n");
      }
    }
#endif
  }

  gsm_stop(false);
  gps_power_off();

  return RDY_OK;
}

int main(void)
{
  uint32_t subsystems = SC_MODULE_UART1 | SC_MODULE_UART2 | SC_MODULE_UART3 |
    SC_MODULE_GPIO | SC_MODULE_LED;

#if USE_USB
  subsystems |= SC_MODULE_SDU;
#endif

#if RELEASE_BUILD == FALSE
  {
    uint32_t cr;
    // Enable GDB with WFI
    cr = *(uint32_t*)0xe0042004; /* DBGMCU_CR */
    cr |= 0x7; /* DBG_STANDBY | DBG_STOP | DBG_SLEEP */
    *(uint32_t*)0xe0042004 = cr;
  }
#endif

  halInit();
  while (1) {

    // Set UART config for GSM
    sc_uart_set_config(SC_UART_3, 115200, 0, 0, 0);

    sc_init(subsystems);

#if USE_USB
    sc_uart_default_usb(TRUE);
    sc_log_output_uart(SC_UART_USB);
#endif

    nmea_start();

    // Register callbacks.
    // These will be called from Event Loop thread. It's OK to do some
    // calculations etc. time consuming there, but not to sleep or block
    // for longer periods of time.
    sc_event_register_handle_byte(cb_handle_byte);
    sc_event_register_gsm_state_changed(cb_gsm_state_changed);
    sc_event_register_gsm_cmd_done(cb_gsm_cmd_done);

    // Start event loop. This will start a new thread and return
    sc_event_loop_start();

    sc_led_on();

    // Start a thread for doing the bulk work
    worker_thread = chThdCreateStatic(main_worker_thread, sizeof(main_worker_thread),
                                      NORMALPRIO, mainWorkerThread, NULL);

    chThdWait(worker_thread);
    worker_thread = NULL;

    nmea_stop();

    sc_led_off();
    palSetPad(USER_LED2_PORT, USER_LED2);

    SC_LOG_PRINTF("Going to sleep\r\n");
    //chThdSleepMilliseconds(1000);

    sc_event_loop_stop();
    sc_deinit(subsystems);
    palClearPad(USER_LED2_PORT, USER_LED2);

    sc_pwr_chibios_stop();
    sc_pwr_wakeup_set(TRACKER_SLEEP_SEC, 0);
#if 0
    sc_pwr_mode_standby(false, true);
#else
    sc_pwr_mode_stop(true);
#endif


#if RELEASE_BUILD == FALSE
    if (1) {
      int i, j;
      int a;

      // WFI does not cause sleep in debug mode so fake a short sleep
      for (i = 0; i < 10000; ++i) {
        for (j = 0; j < 1000; ++j) {
          a = j;
        }
      }
    }
#endif
    // STM32 should now be in full stop mode. Should wake on RTC (no interrupts should be occurring)

    sc_pwr_wakeup_clear();
    sc_pwr_mode_clear();
    sc_pwr_chibios_start();
  }

  return 0;
}

static void cb_handle_byte(SC_UART uart, uint8_t byte)
{
  // NMEA from GPS
  if (uart == SC_UART_2) {
    if (nmea_parse_byte(byte)) {
      nmea = nmea_get_data();
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
  //gsm_cmd_idle = true;
  chThdTerminate(worker_thread);
}




/* Emacs indentatation information
   Local Variables:
   indent-tabs-mode:nil
   tab-width:2
   c-basic-offset:2
   End:
*/
