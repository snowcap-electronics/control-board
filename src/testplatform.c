/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2014 Eero af Heurlin
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
#ifdef TP_PB_PORT

#define SC_LOG_MODULE_TAG SC_LOG_MODULE_UNSPECIFIED

#include "testplatform.h"
#include "sc_log.h"

static Thread *tp_thread = NULL;
static msg_t scTpThread(void *arg);
static void block_tp_sync(uint8_t pulses);
static void tp_sleep_ms(uint8_t ms);

#ifndef TP_MB_SIZE
#define TP_MB_SIZE 8
#endif
static msg_t tp_mb_buffer[TP_MB_SIZE];
MAILBOX_DECL(tp_mb, tp_mb_buffer, TP_MB_SIZE);



static WORKING_AREA(sc_tp_thread, 256);
static msg_t scTpThread(void *arg)
{
  (void)arg;

  // Make sure the pad is output
  palSetPadMode(TP_PB_PORT, TP_PB_PIN, PAL_MODE_OUTPUT_PUSHPULL);
  // Make sure the pad is cleared
  palClearPad(TP_PB_PORT, TP_PB_PIN);

  while(!chThdShouldTerminate()) {
    msg_t ret;
    msg_t msg;

    chRegSetThreadName(__func__);

    ret = chMBFetch(&tp_mb, &msg, TIME_INFINITE);

    if (chThdShouldTerminate()) {
      break;
    }

    SC_LOG_ASSERT(ret == RDY_OK, "chMBFetch failed");
    if (ret != RDY_OK) {
      continue;
    }

    if (msg) {
      block_tp_sync((uint8_t)msg);
    }

	}

  // Make sure the pad is cleared
  palClearPad(TP_PB_PORT, TP_PB_PIN);

  return RDY_OK;
}



/**
 * initializes the pads used for testplatform syncs
 */
void tp_init(void)
{
  // Make sure the pad is output
  palSetPadMode(TP_PB_PORT, TP_PB_PIN, PAL_MODE_OUTPUT_PUSHPULL);
  // Make sure the pad is cleared
  palClearPad(TP_PB_PORT, TP_PB_PIN);

  if (tp_thread) {
    SC_LOG_ASSERT(0, "Testplatform thread already running");
  } else {
    tp_thread = chThdCreateStatic(sc_tp_thread, sizeof(sc_tp_thread),
                                  NORMALPRIO, scTpThread, NULL);
  }
}



/**
 * Deinitializes the testplatform 
 */
void tp_deinit(void)
{
  if (tp_thread) {
    chThdTerminate(tp_thread);
    chThdWait(tp_thread);
    tp_thread = NULL;
  } else {
    SC_LOG_ASSERT(0, "Testplatform thread not running");
  }
}



/**
 * Signals the thread to send the pulses asynchronously
 */
void tp_sync(uint8_t pulses, bool block)
{
  msg_t ret;
  if (block) {
    block_tp_sync(pulses);
  } else {
    ret = chMBPost(&tp_mb, pulses, TIME_IMMEDIATE);
    SC_LOG_ASSERT(ret == RDY_OK, "chMBPost failed");
  }
}



/**
 * Sends a pulse train of N 1ms (approx) sync pulses, starting and ending with 4ms (approx) pulses
 */
static void block_tp_sync(uint8_t pulses)
{
  uint8_t i;
  // Start pulsetrain
  palSetPad(TP_PB_PORT, TP_PB_PIN);
  tp_sleep_ms(4);
  palClearPad(TP_PB_PORT, TP_PB_PIN);
  tp_sleep_ms(1);
  for (i=0; i<pulses; i++) {
    palSetPad(TP_PB_PORT, TP_PB_PIN);
    tp_sleep_ms(1);
    palClearPad(TP_PB_PORT, TP_PB_PIN);
    tp_sleep_ms(1);
  }
  // Stop pulsetrain
  palSetPad(TP_PB_PORT, TP_PB_PIN);
  tp_sleep_ms(4);
  palClearPad(TP_PB_PORT, TP_PB_PIN);
  tp_sleep_ms(4);
}


static void tp_sleep_ms(uint8_t ms)
{
  uint32_t i, n;
  for (i = 0; i < ms; ++i) {
    for (n = 0; n < 16800; ++n) {
      // Do nothing
    }
  }
}


#endif // TP_PB_PORT


/* Emacs indentatation information
   Local Variables:
   indent-tabs-mode:nil
   tab-width:2
   c-basic-offset:2
   End:
*/
