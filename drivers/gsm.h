/*
 *  Simcom 908 GSM Driver for Ruuvitracker.
 *
 * @author: Seppo Takalo
 */

/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2014 Seppo Takalo
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

/* Start the modem */
void gsm_start(void);
/* Set GPRS access point name. Example "internet". Can be set at any time before using http */
void gsm_set_apn(const uint8_t *apn);
/* Feed one byte from the GSM to the GSM parser */
void gsm_state_parser(uint8_t c);
/* Send AT command to modem. Returns 1 if the function call is queued for the GSM, 0 otherwise */
uint8_t gsm_cmd(const uint8_t *cmd, uint8_t len);

#endif	/* GSM_H */
