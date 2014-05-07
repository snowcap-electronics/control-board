/*
 *  Simcom 908/968 GSM Driver for Ruuvitracker.
 */

/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2014 Seppo Takalo
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
#ifdef SC_USE_GSM

#define SC_LOG_MODULE_TAG SC_LOG_MODULE_UNSPECIFIED

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include "gsm.h"
#include "ch.h"
#include "hal.h"

#include "sc.h"

//#define NO_DEBUG
#ifndef NO_DEBUG
/* Assume debug output to be this USB-serial port */
#define printf(...) SC_DBG_PRINTF(__VA_ARGS__)

#define D_ENTER() printf("%s:%s(): enter\r\n", __FILE__, __FUNCTION__)
#define D_EXIT() printf("%s:%s(): exit\r\n", __FILE__, __FUNCTION__)
#define _DEBUG(...) do{                                       \
	  printf(__VA_ARGS__);                                    \
  } while(0)

   //printf("%s:%s:%d: ", __FILE__, __FUNCTION__, __LINE__);

#else

#define D_ENTER()
#define D_EXIT()
#define _DEBUG(...)
#endif /* NO_DEBUG */

/* ============ DEFINE GPIO PINS HERE =========*/

#define STATUS_PIN  GPIOC_GSM_STATUS
#define STATUS_PORT GPIOC
#define DTR_PIN     GPIOC_GSM_DTR
#define DTR_PORT    GPIOC
#define POWER_PIN   GPIOC_GSM_PWRKEY
#define POWER_PORT  GPIOC
#define ENABLE_PIN  GPIOC_ENABLE_GSM_VBAT
#define ENABLE_PORT GPIOC


#define BUFF_SIZE	64

#define TIMEOUT_MS     5000        /* Default timeout 5s */
#define TIMEOUT_HTTP  35000        /* Http timeout, 35s */

/* Modem state */
enum State {
    STATE_UNKNOWN = 0,
    STATE_OFF = 1,
    STATE_BOOTING,
    STATE_ASK_PIN,
    STATE_WAIT_NETWORK,
    STATE_READY,
    STATE_ERROR,
};


enum CFUN {
    CFUN_0 = 0,
    CFUN_1 = 1,
};

enum GSM_FLAGS {
    HW_FLOW_ENABLED = 0x01,
    SIM_INSERTED    = 0x02,
    GPS_READY       = 0x04,
    GPRS_READY      = 0x08,
    CALL            = 0x10,
    INCOMING_CALL   = 0x20,
    TCP_ENABLED     = 0x40,
};

/* Modem Status */
struct gsm_modem {
    enum Power_mode power_mode;
    enum State state;
    BinarySemaphore waiting_reply;
    Mutex gsm_data_mtx;
    volatile int raw_mode;
    enum Reply reply;
    int flags;
    enum CFUN cfun;
    int last_sms_index;
    uint8_t ap_name[64];
    uint8_t buf[BUFF_SIZE+1];
    uint8_t buf_len;
    uint8_t incoming_buf[BUFF_SIZE+1];
    uint8_t incoming_i;
};

static struct gsm_modem gsm = {		/* Initial status */
    .power_mode = POWER_OFF,
    .state = STATE_OFF,
};

enum API_CALL {
    API_CALL_UNKNOWN = 0,
    API_CALL_GSM_CMD,
};

/* Struct for passing Public API functions to internal thread */
struct api {
    enum API_CALL api_call;
    BinarySemaphore new_call_sem;
    Mutex api_data_mtx;
    union {
        struct {
            const uint8_t *cmd;
            uint8_t len;
        } gsm_cmd;
    };
};

static struct api api;

static Thread *gsm_thread = NULL;
static msg_t scGsmThread(void *UNUSED(arg));

/* URC message */
typedef struct Message Message;
struct Message {
    const char *msg;        /* Message string */
    uint8_t msg_len;        /* Message length */
    enum State next_state;  /* Automatic state transition */
    void (*func)(uint8_t *line, uint8_t len);/* Function to call on message */
};

/* Send formatted string command to modem. */
static uint8_t gsm_cmd_fmt(const uint8_t *fmt, ...);
/* Send string and wait for a specific response */
static uint8_t gsm_cmd_wait(const uint8_t *cmd, uint8_t len, const uint8_t *response, int timeout);
/* Send formatted string and wait for response */
static uint8_t gsm_cmd_wait_fmt(const uint8_t *response, int timeout, uint8_t *fmt, ...);
/* wait for a pattern to appear */
static uint8_t gsm_wait(const uint8_t *pattern, int timeout);
/* Wait and copy rest of the line to a buffer */
static uint8_t gsm_wait_cpy(const uint8_t *pattern, int timeout, uint8_t *buf, unsigned int buf_size);
/* Send byte-string to GSM serial port */
static void gsm_uart_write(const uint8_t *line, uint8_t len);
/* Reset modem */
static void gsm_reset_modem(void);
/* Read bytes from serial port */
static int gsm_read_raw(uint8_t *buf, int max_len);
/* Enable GPRS data */
static int gsm_gprs_enable(void);
/* Disable GPRS data */
static int gsm_gprs_disable(void);
/* Make a GSM command */
static uint8_t gsm_cmd_internal(const uint8_t *cmd, uint8_t len);

/* Internals */
static void gsm_set_power_state(enum Power_mode mode);
static void gsm_toggle_power_pin(uint8_t wait_status);

/* Handler functions for AT replies*/
static void handle_user_defined(uint8_t *line, uint8_t len);
static void handle_ok(uint8_t *line, uint8_t len);
static void handle_fail(uint8_t *line, uint8_t len);
static void handle_error(uint8_t *line, uint8_t len);
static void handle_cfun(uint8_t *line, uint8_t len);
static void handle_call_ready(uint8_t *line, uint8_t len);
static void handle_gpsready(uint8_t *line, uint8_t len);
static void handle_sim_not_ready(uint8_t *line, uint8_t len);
static void handle_sim_inserted(uint8_t *line, uint8_t len);
static void handle_no_sim(uint8_t *line, uint8_t len);
static void handle_incoming_call(uint8_t *line, uint8_t len);
static void handle_call_ended(uint8_t *line, uint8_t len);
static void handle_network(uint8_t *line, uint8_t len);
static void handle_sapbr(uint8_t *line, uint8_t len);
static void handle_sms_in(uint8_t *line, uint8_t len);
static void handle_pdp_off(uint8_t *line, uint8_t len);

/* Other prototypes */
static void gsm_enable_hw_flow(void);
static void gsm_set_serial_flow_control(int enabled);
static Message *lookup_urc_message(const uint8_t *line, uint8_t len);

/* Messages from modem */
static Message urc_messages[] = {
    /* User defined reply, MUST BE the first */
    { "", .func = handle_user_defined },
    /* Unsolicited Result Codes (URC messages) */
    { "RDY",                  .next_state=STATE_BOOTING },
    { "NORMAL POWER DOWN",    .next_state=STATE_OFF },
    { "^+CPIN: NOT INSERTED", .next_state=STATE_ERROR,        .func = handle_no_sim },
    { "+CPIN: NOT READY",     .next_state=STATE_WAIT_NETWORK, .func = handle_sim_not_ready },
    { "+CPIN: READY",         .next_state=STATE_WAIT_NETWORK, .func = handle_sim_inserted },
    { "+CPIN: SIM PIN",       .next_state=STATE_ASK_PIN,      .func = handle_sim_inserted },
    { "+CFUN:",                                               .func = handle_cfun },
    { "Call Ready",           .next_state=STATE_READY,        .func = handle_call_ready },
    { "GPS Ready",    .func = handle_gpsready },
    { "+COPS:",       .func = handle_network },
    { "+SAPBR:",      .func = handle_sapbr },
    { "+PDP: DEACT",  .func = handle_pdp_off },
    /* Return codes */
    { "OK",           .func = handle_ok },
    { "FAIL",         .func = handle_fail },
    { "ERROR",        .func = handle_error },
    { "+CME ERROR",   .func = handle_error },
    { "+CMS ERROR",   .func = handle_error },                       /* TODO: handle */
    /* During Call */
    { "NO CARRIER",   .func = handle_call_ended }, /* This is general end-of-call */
    { "NO DIALTONE",  .func = handle_call_ended },
    { "BUSY",         .func = handle_fail },
    { "NO ANSWER",    .func = handle_fail },
    { "RING",         .func = handle_incoming_call },
    /* SMS */
    { "+CMTI:",       .func = handle_sms_in },
    { NULL } /* Table must end with NULL */
};

/**
 * GSM command and state machine handler.
 * Receives bytes from Simcom serial interfaces and parses them.
 */
void gsm_state_parser(uint8_t c)
{
    bool_t ret;
    Message *m;
    uint8_t i;

    //_DEBUG("recv byte: '%d'\r\n", c);

    if (c == '\r' || c == '\0') {
        return;
    }

    if (c != '\n') {

        gsm.incoming_buf[gsm.incoming_i++] = c;

        // No complete message and buffer not full
        if (gsm.incoming_i != BUFF_SIZE) {
            return;
        }
    }

    /* Discard data if full buffer */
    if (gsm.incoming_i == BUFF_SIZE) {
        gsm.incoming_i = 0;
        gsm.incoming_buf[BUFF_SIZE] = '\0';
        _DEBUG("buffer full: %s\r\n", gsm.incoming_buf);
        gsm.incoming_buf[0] = '\0';
        chDbgAssert(0, "GSM incoming buffer full", "#1");
        return;
    }

    /* Skip empty lines */
    if (gsm.incoming_i == 0) {
        return;
    }

    gsm.incoming_buf[gsm.incoming_i] = '\0';

    _DEBUG("recv: %s\r\n", gsm.incoming_buf);

    // Copy the data for prosessing or ignore the whole message
    // Note: Technically we can lose messages here without noticing it.
    // FIXME: Implement proper circular buffer and use 2 buffers here.
    ret = chMtxTryLock(&gsm.gsm_data_mtx);
    if (!ret) {
        chDbgAssert(0, "GSM data handling busy", "#1");
        gsm.incoming_i = 0;
        return;
    }

    gsm.buf_len = gsm.incoming_i;
    gsm.incoming_i = 0;
    for (i = 0; i < gsm.buf_len; ++i) {
        gsm.buf[i] = gsm.incoming_buf[i];
    }
    gsm.buf[i] = '\0';

    m = lookup_urc_message(gsm.buf, gsm.buf_len);
    if (m) {
        //_DEBUG("urc message: %s, next state: %d\r\n", m->msg, m->next_state);
        if (m->next_state) {
            gsm.state = m->next_state;
            // FIXME: send event (too early?)
        }
        if (m->func) {
            m->func(gsm.buf, gsm.buf_len);
        }
    }
    chMtxUnlock();
}

/* Public function to pass the actual work to a separate thread */
uint8_t gsm_cmd(const uint8_t *cmd, uint8_t len)
{
    bool_t ret;
    ret = chMtxTryLock(&api.api_data_mtx);
    if (!ret) {
        // FIXME: having call already ongoing might not be an error
        chDbgAssert(0, "GSM API call already ongoing", "#1");
        return 0;
    }

    if (api.api_call != API_CALL_UNKNOWN) {
        // FIXME: having call already ongoing might not be an error
        chDbgAssert(0, "Previous GSM API call not finished", "#1");
        return 0;
    }

    api.api_call = API_CALL_GSM_CMD;
    api.gsm_cmd.cmd = cmd;
    api.gsm_cmd.len = len;

    chBSemSignal(&api.new_call_sem);
    chMtxUnlock();

    return 1;
}


/* Find message matching current line */
static Message *lookup_urc_message(const uint8_t *line, uint8_t len)
{
    int n;
    (void)len;
    for(n=0; urc_messages[n].msg; n++) {
        if (urc_messages[n].msg[0] != '\0') {
            if (strncmp(urc_messages[n].msg, (char *)line, urc_messages[n].msg_len) == 0) {
                return &urc_messages[n];
            }
        }
    }

    return NULL;
}

static void handle_user_defined(uint8_t *line, uint8_t len)
{
    (void)line;
    (void)len;
    chBSemSignal(&gsm.waiting_reply);
}

static void handle_pdp_off(uint8_t *line, uint8_t len)
{
    (void)line;
    (void)len;
    gsm.flags &= ~TCP_ENABLED;
    // FIXME: send event
}

static void handle_incoming_call(uint8_t *line, uint8_t len)
{
    (void)line;
    (void)len;
    gsm.flags |= INCOMING_CALL;
    // FIXME: send event
}

static void handle_call_ended(uint8_t *line, uint8_t len)
{
    (void)line;
    (void)len;
    gsm.flags &= ~CALL;
    gsm.flags &= ~INCOMING_CALL;
    // FIXME: send event
}

static void handle_sms_in(uint8_t *line, uint8_t len)
{
    (void)len;
    if (1 == sscanf((char*)line, "+CMTI: \"SM\",%d", &gsm.last_sms_index)) {
        //TODO: implement this
    }
}

static void handle_network(uint8_t *line, uint8_t len)
{
    char network[64];
    (void)len;
    /* Example: +COPS: 0,0,"Saunalahti" */
    if (1 == sscanf((char*)line, "+COPS: 0,0,\"%s", network)) {
        *strchr(network,'"') = 0;
        _DEBUG("GSM: Registered to network %s\r\n", network);
        gsm.state = STATE_READY;
    }
    // FIXME: send event
}

#define STATUS_CONNECTING 0
#define STATUS_CONNECTED 1
static const char sapbr_deact[] = "+SAPBR 1: DEACT";

static void handle_sapbr(uint8_t *line, uint8_t len)
{
    int status;
    (void)len;

    /* Example: +SAPBR: 1,1,"10.172.79.111"
     * 1=Profile number
     * 1=connected, 0=connecting, 3,4 =closing/closed
     * ".." = ip addr
     */
    if (1 == sscanf((char*)line, "+SAPBR: %*d,%d", &status)) {
        switch(status) {
        case STATUS_CONNECTING:
        case STATUS_CONNECTED:
            gsm.flags |= GPRS_READY;
            break;
        default:
            // FIXME: handle all explicitly, assert on default (unknown)
            _DEBUG("%s","GPRS not active\r\n");
            gsm.flags &= ~GPRS_READY;
        }
    } else if (0 == strncmp((char*)line, sapbr_deact, strlen(sapbr_deact))) {
        gsm.flags &= ~GPRS_READY;
    }
    // FIXME: send event
}

static void handle_sim_not_ready(uint8_t *line, uint8_t len)
{
    (void)line;
    (void)len;
    _DEBUG("SIM not ready\r\n");
    // FIXME: send event
}

static void handle_sim_inserted(uint8_t *line, uint8_t len)
{
    (void)line;
    (void)len;
    gsm.flags |= SIM_INSERTED;
    // FIXME: send event
}

static void handle_no_sim(uint8_t *line, uint8_t len)
{
    (void)line;
    (void)len;
    gsm.flags &= ~SIM_INSERTED;
    // FIXME: send event
}

static void handle_gpsready(uint8_t *line, uint8_t len)
{
    (void)line;
    (void)len;
    gsm.flags |= GPS_READY;
    // FIXME: send event
}

static void handle_cfun(uint8_t *line, uint8_t len)
{
    (void)len;

    if (strchr((char*)line,'0')) {
        gsm.cfun = CFUN_0;
    } else if (strchr((char*)line,'1')) {
        gsm.cfun = CFUN_1;
    } else {
        _DEBUG("GSM: Unknown CFUN state: %s\r\n", line);
    }
}

static void handle_call_ready(uint8_t *line, uint8_t len)
{
    (void)len;
    (void)line;
    if (!(gsm.flags & SIM_INSERTED)) {
        _DEBUG("%s","GSM ready but SIM is not\r\n");
    }
}

static void handle_ok(uint8_t *line, uint8_t len)
{
    (void)line;
    (void)len;
    gsm.reply = AT_OK;
    _DEBUG("%s","GSM: AT OK\r\n");
    chBSemSignal(&gsm.waiting_reply);
}

static void handle_fail(uint8_t *line, uint8_t len)
{
    (void)line;
    (void)len;
    gsm.reply = AT_FAIL;
    _DEBUG("%s","GSM: Fail\r\n");
    chBSemSignal(&gsm.waiting_reply);
}

static void handle_error(uint8_t *line, uint8_t len)
{
    (void)line;
    (void)len;
    gsm.reply = AT_ERROR;
    _DEBUG("GSM ERROR: %s\r\n", line);
    chBSemSignal(&gsm.waiting_reply);
}

static void gsm_toggle_power_pin(uint8_t wait_status)
{
    systime_t wait_limit;

    _DEBUG("%d: Toggling power pin for status %s\r\n", chTimeNow(), wait_status ? "HIGH" : "LOW");

    palClearPad(POWER_PORT, POWER_PIN);
    chThdSleepMilliseconds(2000);
    palSetPad(POWER_PORT, POWER_PIN);

    _DEBUG("%d: Toggling done\r\n", chTimeNow());

    // Wait for status pin update
    wait_limit = chTimeNow() + 4000;
    _DEBUG("Starting waiting for status pin\r\n");
    while (chTimeNow() < wait_limit) {
        int status = palReadPad(STATUS_PORT, STATUS_PIN);
        _DEBUG("%d: Waiting for status pin to go %s\r\n", chTimeNow(), wait_status ? "HIGH" : "LOW");
        if (status == wait_status) {
            // FIXME: extra sleep, should not be needed
            chThdSleepMilliseconds(1000);
            _DEBUG("%d: Got status %s\r\n", chTimeNow());
            return;
        }
        chThdSleepMilliseconds(100);
    }
    _DEBUG("%d: Timeout waiting for status %s\r\n", chTimeNow());
}

static void sleep_enable(void)
{
    _DEBUG("sleep_enable\r\n");
    palSetPad(DTR_PORT, DTR_PIN);
}

static void sleep_disable(void)
{
    _DEBUG("sleep_disable\r\n");
    palClearPad(DTR_PORT, DTR_PIN);
    chThdSleepMilliseconds(50);
}

static void gsm_uart_write(const uint8_t *str, uint8_t len)
{
    // FIXME: no hardcoded UART (use sc_conf.h)
    sc_uart_send_msg(SC_UART_3, str, len);
}

/**
 * Send AT command to modem.
 * Waits for modem to reply with 'OK', 'FAIL' or 'ERROR'
 * return AT_OK, AT_FAIL or AT_ERROR
 */
static uint8_t gsm_cmd_internal(const uint8_t *cmd, uint8_t len)
{
    uint8_t retry;

    sleep_disable();

    /* Reset semaphore so we are sure that next time it is signalled
     * it is reply to this command */
    chBSemReset(&gsm.waiting_reply, TRUE);

    for(retry=0; retry<3; retry++) {
        _DEBUG("send (retry %d): %s", retry, cmd);
        gsm_uart_write(cmd, len);
        if(RDY_OK != chBSemWaitTimeout(&gsm.waiting_reply, TIMEOUT_MS))
            gsm.reply = AT_TIMEOUT;
        if (gsm.reply != AT_TIMEOUT)
            break;
    }

    if (gsm.reply != AT_OK) {
        _DEBUG("'%s' failed (%d)\r\n", cmd, gsm.reply);
    }

    sleep_enable();
    if (retry == 3) {             /* Modem not responding */
        _DEBUG("Modem not responding!\r\n");
        return AT_TIMEOUT;
    }
    return gsm.reply;
}

static uint8_t gsm_cmd_fmt(const uint8_t *fmt, ...)
{
    uint8_t cmd[256];
    va_list ap;
    uint8_t len;
    va_start( ap, fmt );
    len = vsnprintf((char*) cmd, sizeof(cmd), (char*)fmt, ap );
    va_end( ap );
    return gsm_cmd_internal(cmd, len);
}

/* Wait for specific pattern */
static uint8_t gsm_wait_cpy(const uint8_t *pattern, int timeout, uint8_t *line, size_t buf_size)
{
    D_ENTER();
    systime_t wait_limit;
    uint8_t found = 0;

    _DEBUG("wait=%s\r\n", pattern);

    urc_messages[0].msg = (const char*)pattern;
    urc_messages[0].msg_len = strlen((char*)pattern);

    wait_limit = chTimeNow() + timeout;

    while (chTimeNow() < wait_limit) {
        msg_t ret;

        ret = chBSemWaitTimeout(&gsm.waiting_reply, 1000);
        if (ret == RDY_OK) {
            if (strncmp((const char *)pattern, (const char *)gsm.buf, gsm.buf_len) == 0) {
                found = 1;
                break;
            }
        }
    }

    if (found && line) {
        strncpy((char *)line, (char *)gsm.buf, buf_size);
    }

    urc_messages[0].msg = "";
    urc_messages[0].msg_len = 0;

    D_EXIT();
    return found;
}

static uint8_t gsm_wait(const uint8_t *pattern, int timeout)
{
    return gsm_wait_cpy(pattern, timeout, NULL, 0);
}

static uint8_t gsm_cmd_wait(const uint8_t *cmd, uint8_t len, const uint8_t *response, int timeout)
{
    int r;
    _DEBUG("send: %s\r\n", cmd);
    gsm_uart_write(cmd, len);
    gsm_uart_write((uint8_t *)GSM_CMD_LINE_END, sizeof(GSM_CMD_LINE_END));
    r = gsm_wait(response, timeout);
    return r;
}

static uint8_t gsm_cmd_wait_fmt(const uint8_t *response, int timeout, uint8_t *fmt, ...)
{
    uint8_t cmd[256];
    uint8_t len;
    va_list ap;
    va_start( ap, fmt );
    len = vsnprintf((char*)cmd, sizeof(cmd), (char *)fmt, ap );
    va_end( ap );
    return gsm_cmd_wait(cmd, len, response, timeout);
}

#if 0
int gsm_read_raw(uint8_t *buf, int max_len)
{
    msg_t r;
    int was_locked;
    D_ENTER();
    was_locked = gsm_request_serial_port();
    r = chnReadTimeout((BaseChannel*)&SD3, (void*)buf, max_len, TIMEOUT_MS);
    _DEBUG("read %d bytes\r\n", (int)r);
    if (!was_locked)
        gsm_release_serial_port();
    D_EXIT();
    return (int)r;
}
#endif
/* =========== SIM968 Helpers ================= */
static void set_mode_to_fixed_baud(void)
{
    uint8_t at[] = "AT\r\n";
    D_ENTER();

    gsm_set_serial_flow_control(0);
    // Send 3x AT for autobaud. Works only after boot.
    _DEBUG("send: %s", at);
    gsm_uart_write(at, sizeof(at));
    _DEBUG("send: %s", at);
    gsm_uart_write(at, sizeof(at));
    if (gsm_cmd_internal(at, sizeof(at)) == AT_OK) {
        uint8_t setbaud[] = "AT+IPR=115200\r\n";  /* Switch to fixed baud rate */
        if (gsm_cmd_internal(setbaud, sizeof(setbaud)) != AT_OK) {
            _DEBUG("%s", "Failed to set baud rate to 115200, keeping autobaud\r\n");
        }
    } else {
        // FIXME: assert?
        _DEBUG("%s", "Failed to synchronize autobauding mode\r\n");
    }
    D_EXIT();
}

static void gsm_enable_hw_flow()
{
    unsigned int timeout_s = 5;
    uint8_t ifcset[] = "AT+IFC=2,2\r\n"; /* Enable HW flow ctrl */
    uint8_t echo[]   = "ATE0\r\n";       /* Disable ECHO */
    uint8_t sleep[]  = "AT+CSCLK=1\r\n"; /* Allow module to sleep */
    uint8_t verbose[] = "AT+CMEE=2\r\n"; /* Verbose errors */
    uint8_t ret = AT_OK;

    D_ENTER();
    while(gsm.state < STATE_BOOTING) {
        _DEBUG("Waiting for booting state\r\n");
        chThdSleepMilliseconds(1000);
        if (!(timeout_s--)) {
            _DEBUG("No boot messages from uart. assume autobauding\r\n");
            set_mode_to_fixed_baud();
            break;
        }
    }

    gsm_cmd_internal(verbose, sizeof(verbose));

    ret = gsm_cmd_internal(ifcset, sizeof(ifcset));
    if (ret == AT_OK) {
        gsm_set_serial_flow_control(1);
        _DEBUG("%s","HW flow enabled\r\n");
    }

    if (ret == AT_OK) {
        ret = gsm_cmd_internal(echo, sizeof(echo));
    }

    if (ret == AT_OK) {
        ret = gsm_cmd_internal(sleep, sizeof(sleep));
    }

    if (ret != AT_OK) {
        _DEBUG("Failed to enable HW flow control\r\n");
    }

    D_EXIT();
}

/* Enable GSM module */
static void gsm_set_power_state(enum Power_mode mode)
{
    int status_pin = palReadPad(STATUS_PORT, STATUS_PIN);

    switch(mode) {
    case POWER_ON:
        if (status_pin == PAL_LOW) {
            // Wake up GSM
            sleep_disable();
            gsm_toggle_power_pin(PAL_HIGH);

            gsm_enable_hw_flow();
        } else {
            /* Modem already powered */
            if (gsm.state == STATE_OFF) {
                /* Responses of these will feed the state machine */
                uint8_t cmd1[] = "AT+CPIN?\r\n";      /* Check PIN */
                uint8_t cmd2[] = "AT+CFUN?\r\n";      /* Functionality mode */
                uint8_t cmd3[] = "AT+COPS?\r\n";      /* Registered to network */
                uint8_t cmd4[] = "AT+SAPBR=2,1\r\n";  /* Query GPRS status */
                sleep_disable();
                gsm_cmd_internal(cmd1, sizeof(cmd1));
                gsm_cmd_internal(cmd2, sizeof(cmd2));
                gsm_cmd_internal(cmd3, sizeof(cmd3));
                gsm_cmd_internal(cmd4, sizeof(cmd4));
                gsm_enable_hw_flow();
                /* We should now know modem's real status */
            } else {
                chDbgAssert(0, "GSM already powered on", "#1");
            }
        }
        break;
    case POWER_OFF:
        if (1 == status_pin) {
            gsm_toggle_power_pin(PAL_LOW);
        } else {
            chDbgAssert(0, "GSM already powered off", "#1");
        }
        gsm.state = STATE_OFF;
        gsm.flags = 0;
        break;
    case CUT_OFF:
        _DEBUG("%s","CUT_OFF mode not yet handled\r\n");
        chDbgAssert(0, "CUT_OFF mode not yet handled", "#1");
        break;
    default:
        chDbgAssert(0, "Unknown GSM mode", "#1");
        break;
    }
}

static void gsm_reset_modem()
{
    gsm_set_power_state(POWER_OFF);
    chThdSleepMilliseconds(2000);
    gsm_set_power_state(POWER_ON);
}

static int gsm_gprs_enable()
{
    int rc;
    systime_t wait_limit;
    uint8_t cmd1[] = "AT+SAPBR=2,1\r\n";
    uint8_t cmd2[] = "AT+SAPBR=3,1,\"CONTYPE\",\"GPRS\"\r\n";
    uint8_t cmd3[] = "AT+SAPBR=3,1,\"APN\",\"%s\"\r\n";
    uint8_t cmd4[] = "AT+SAPBR=1,1\r\n";

    /* Wait for network */
    wait_limit = chTimeNow() + TIMEOUT_MS;
    while(gsm.state < STATE_READY && chTimeNow() < wait_limit) {
        chThdSleepMilliseconds(1000);
    }

    if (gsm.state < STATE_READY) {
        _DEBUG("Timeout waiting for RDY state before enabling GPRS\r\n");
        chDbgAssert(0, "Timeout waiting for RDY state before enabling GPRS", "#1");
        return AT_TIMEOUT;
    }

    /* Check if already enabled, response is parsed in URC handler */
    rc = gsm_cmd_internal(cmd1, sizeof(cmd1));
    if (rc != AT_OK)
        return rc;

    if (gsm.flags & GPRS_READY) {
        _DEBUG("GPRS already enabled\r\n");
        return AT_OK;
    }

    rc = gsm_cmd_internal(cmd2, sizeof(cmd2));
    if (rc != AT_OK) {
        return rc;
    }

    rc = gsm_cmd_fmt(cmd3, gsm.ap_name);
    if (rc != AT_OK) {
        return rc;
    }

    rc = gsm_cmd_internal(cmd4, sizeof(cmd4));
    if (AT_OK == rc) {
        gsm.flags |= GPRS_READY;
    }

    return rc;
}

static int gsm_gprs_disable(void)
{
    uint8_t cmd1[] = "AT+SAPBR=2,1\r\n"; // Check GPRS status
    uint8_t cmd2[] = "AT+SAPBR=0,1\r\n"; // Close Bearer
    gsm_cmd_internal(cmd1, sizeof(cmd1));
    if (!(gsm.flags & GPRS_READY))
        return AT_OK; // Was already disabled

    return gsm_cmd_internal(cmd2, sizeof(cmd2));
}

void gsm_set_apn(const uint8_t *buf)
{
    strncpy((char*)gsm.ap_name, (char*)buf, sizeof(gsm.ap_name));
}

/*
 * Setup a thread for handling GSM communication
 */
static WORKING_AREA(sc_gsm_thread, 1024);
static msg_t scGsmThread(void *arg)
{
    (void)arg;
    uint8_t i;

    // Initialise the length of the URC messages
    for (i = 0; urc_messages[i].msg != NULL; ++i) {
        urc_messages[i].msg_len = strlen(urc_messages[i].msg);
    }

    if (palReadPad(GPIOC, GPIOC_ENABLE_GSM_VBAT) == PAL_LOW) {
        // Power off GSM
        _DEBUG("GSM_VBAT already powered, toggling.\r\n");
        palSetPad(GPIOC, GPIOC_ENABLE_GSM_VBAT);
        chThdSleepMilliseconds(2000);
    }

    sleep_disable();

    // Power on GSM
    palClearPad(GPIOC, GPIOC_ENABLE_GSM_VBAT);

    gsm_set_power_state(POWER_ON);

    while(!chThdShouldTerminate()) {
        // Wait for either new command to send to GSM
        _DEBUG("scGsmThread: waiting for new cmd.\r\n");
        chBSemWait(&api.new_call_sem);

        if (chThdShouldTerminate()) {
            break;
        }

        _DEBUG("scGsmThread: waiting for api_data_mtx.\r\n");
        chMtxLock(&api.api_data_mtx);

        _DEBUG("scGsmThread: have new cmd: %d.\r\n", api.api_call);

        switch(api.api_call) {
        case API_CALL_GSM_CMD:
            gsm_cmd_internal(api.gsm_cmd.cmd, api.gsm_cmd.len);
            break;
        case API_CALL_UNKNOWN:
            chDbgAssert(0, "Unexpected API_CALL_UNKNOWN\r\n", "#1");
            break;
        default:
            chDbgAssert(0, "Unknown API CALL\r\n", "#1");
            break;
        }

        api.api_call = API_CALL_UNKNOWN;
        chMtxUnlock();
    }
    chThdExit(0);

  return 0;
}


/* ============ PLATFORM SPECIFIC ==================== */

static void gsm_set_serial_flow_control(int enabled)
{
    // FIXME: no hardcoded uart
    sc_uart_stop(SC_UART_3);
    if (enabled) {
        gsm.flags |= HW_FLOW_ENABLED;
        sc_uart_set_config(SC_UART_3, 115200, 0, USART_CR2_LINEN, USART_CR3_RTSE | USART_CR3_CTSE);
        sleep_enable();
    } else {
        gsm.flags &= ~HW_FLOW_ENABLED;
        sc_uart_set_config(SC_UART_3, 115200, 0, USART_CR2_LINEN, 0);
    }
    sc_uart_init(SC_UART_3);
}

/* ============ GENERAL ========================== */

void gsm_start(void)
{
    chBSemInit(&gsm.waiting_reply, TRUE);
    chMtxInit(&gsm.gsm_data_mtx);
    chBSemInit(&api.new_call_sem, TRUE);
	chMtxInit(&api.api_data_mtx);

    gsm.incoming_buf[0] = '\0';
    gsm.incoming_i = 0;

    if (gsm_thread) {
        chDbgAssert(0, "GSM thread already running", "#1");
    } else {
        gsm_thread = chThdCreateStatic(sc_gsm_thread, sizeof(sc_gsm_thread),
                                       NORMALPRIO, scGsmThread, NULL);
    }
}

void gsm_stop(void)
{
    // Power off GSM
    gsm_set_power_state(POWER_OFF);
    palSetPad(GPIOC, GPIOC_ENABLE_GSM_VBAT);

    if (gsm_thread) {
        chThdTerminate(gsm_thread);
        gsm_thread = NULL;
    } else {
        chDbgAssert(0, "GSM thread not running", "#1");
    }
}

#endif // SC_USE_GSM

/* Emacs indentatation information
   Local Variables:
   indent-tabs-mode:nil
   tab-width:4
   c-basic-offset:4
   End:
*/