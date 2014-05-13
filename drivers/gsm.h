/*
 *  Simcom 908 GSM Driver for Ruuvitracker.
 *
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

#ifndef _GSM_H
#define _GSM_H

#include <stdint.h>

#define GSM_CMD_LINE_END "\r\n"

enum Power_mode {
  POWER_OFF = 0,
  POWER_ON,
  CUT_OFF
};

enum Reply {
  AT_OK = 0,
  AT_FAIL,
  AT_ERROR,
  AT_TIMEOUT
};

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

typedef struct HTTP_Response_t {
    int code;
    unsigned int content_len;
    uint8_t content[0];
} HTTP_Response;

/* Start the modem */
void gsm_start(void);
/* Stop the modem optionally powering it down */
void gsm_stop(bool power_off);
/* Set GPRS access point name. Example "internet". Can be set at any time before using http */
void gsm_set_apn(const uint8_t *apn);
/* Set HTTP user Agent name. Example "RuuviTracker/SIM968". Can be set at any time before using http */
void gsm_set_user_agent(const uint8_t *agent);
/* Feed one byte from the GSM to the GSM parser */
void gsm_state_parser(uint8_t c);
/* Get GSM state */
void gsm_state_get(enum State *state, enum GSM_FLAGS *flags);

/* Send AT command to modem. Returns 1 if the function call is queued for the GSM, 0 otherwise */
uint8_t gsm_cmd(const uint8_t *cmd, uint8_t len);

uint8_t gsm_http_get(const uint8_t *url, uint8_t len);
uint8_t gsm_http_post(const uint8_t *url, uint8_t url_len,
					  const uint8_t *data, int data_len,
					  const uint8_t *content_type, uint8_t type_len);

#endif /* GSM_H */
#endif /* SC_USE_GSM */
