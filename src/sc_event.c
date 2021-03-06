/*
 * Event loop
 *
 * Copyright 2012 Tuomas Kulve, <tuomas.kulve@snowcap.fi>
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

#include "sc_cmd.h"
#include "sc_event.h"
#include "sc_uart.h"

/*
 * Reserve bits for different kind of messages, msg_t == int32_t
 */
// FIXME: come up with a more sane logic
#define EVENT_MSG_TYPE_MASK    0x000000ff // common
#define EVENT_MSG_UART_MASK    0x00000f00 // handle_byte
#define EVENT_MSG_BYTE_MASK    0x00ff0000 // handle_byte
#define EVENT_MSG_PIN_MASK     0x0000ff00 // extint

#define EVENT_MSG_TYPE_SHIFT            0
#define EVENT_MSG_UART_SHIFT            8
#define EVENT_MSG_BYTE_SHIFT           16
#define EVENT_MSG_PIN_SHIFT             8

// FIXME: convert to cleaner macros
#define EVENT_MSG_GET_TYPE(m)                                           \
  ((SC_EVENT_TYPE)((m & EVENT_MSG_TYPE_MASK) >> EVENT_MSG_TYPE_SHIFT))

#define EVENT_MSG_GET_BYTE(m)                                     \
  ((uint8_t)((m & EVENT_MSG_BYTE_MASK) >> EVENT_MSG_BYTE_SHIFT))

#define EVENT_MSG_GET_UART(m)                                     \
  ((SC_UART)((m & EVENT_MSG_UART_MASK) >> EVENT_MSG_UART_SHIFT))

#define EVENT_MSG_GET_PIN(m)                                          \
  ((uint8_t)((m & EVENT_MSG_PIN_MASK) >> EVENT_MSG_PIN_SHIFT))

/* Action event mailbox used in event loop to pass data and action
 * from other threads and interrupt handlers to the main thread. */
#ifndef EVENT_MB_SIZE
#define EVENT_MB_SIZE 128
#endif
static msg_t event_mb_buffer[EVENT_MB_SIZE];
MAILBOX_DECL(event_mb, event_mb_buffer, EVENT_MB_SIZE);

/*
 * Callbacks
 */
#if HAL_USE_UART
static sc_event_cb_handle_byte    cb_handle_byte = NULL;
#endif
#if HAL_USE_EXT
static sc_event_cb_extint         cb_extint[EXT_MAX_CHANNELS] = {NULL};
#endif

static sc_event_cb_gsm_state_changed cb_gsm_state_changed = NULL;
static sc_event_cb_gsm_cmd_done   cb_gsm_cmd_done = NULL;
static sc_event_cb_ping           cb_ping = NULL;

#if HAL_USE_ADC
static sc_event_cb_adc_available  cb_adc_available = NULL;
#endif
static sc_event_cb_temp_available cb_temp_available = NULL;
static sc_event_cb_9dof_available cb_9dof_available = NULL;
static sc_event_cb_blob_available cb_blob_available = NULL;
static sc_event_cb_ahrs_available cb_ahrs_available = NULL;
static sc_event_cb_audio_available cb_audio_available = NULL;
static sc_event_cb_spirit1_msg_available cb_spirit1_msg_available = NULL;
static sc_event_cb_spirit1_data_sent cb_spirit1_data_sent = NULL;
static sc_event_cb_spirit1_data_lost cb_spirit1_data_lost = NULL;
static sc_event_cb_spirit1_error cb_spirit1_error = NULL;
static sc_event_cb_ms5611_available cb_ms5611_available = NULL;

static thread_t *event_thread = NULL;

/*
 * Setup a working area with for the event loop thread
 */
#ifndef SC_EVENT_LOOP_THREAD_WA
#define SC_EVENT_LOOP_THREAD_WA 2048
#endif
static THD_WORKING_AREA(event_loop_thread, SC_EVENT_LOOP_THREAD_WA);
static THD_FUNCTION(eventLoopThread, arg)
{
  (void) arg;

  chRegSetThreadName(__func__);

  while (!chThdShouldTerminateX()) {
    msg_t msg;
    msg_t ret;
    SC_EVENT_TYPE type;

    // Wait for action
    ret = chMBFetch(&event_mb, &msg, TIME_INFINITE);

    if (chThdShouldTerminateX()) {
      break;
    }

    chDbgAssert(ret == MSG_OK, "chMBFetch failed");
    if (ret != MSG_OK) {
      continue;
    }

    // Get event type from the message;
    type = EVENT_MSG_GET_TYPE(msg);

    switch(type) {

      // Application registrable events
#if HAL_USE_UART
    case SC_EVENT_TYPE_PUSH_BYTE:
      if (cb_handle_byte != NULL) {
        SC_UART uart = EVENT_MSG_GET_UART(msg);
        uint8_t byte = EVENT_MSG_GET_BYTE(msg);
        cb_handle_byte(uart, byte);
      }
      break;
#endif
#if HAL_USE_EXT
    case SC_EVENT_TYPE_EXTINT:
      {
        uint8_t pin = EVENT_MSG_GET_PIN(msg);
        if (cb_extint[pin] != NULL) {
          cb_extint[pin]();
        }
      }
      break;
#endif
    case SC_EVENT_TYPE_GSM_STATE_CHANGED:
      if (cb_gsm_state_changed != NULL) {
        cb_gsm_state_changed();
      }
      break;
    case SC_EVENT_TYPE_GSM_CMD_DONE:
      if (cb_gsm_cmd_done != NULL) {
        cb_gsm_cmd_done();
      }
      break;
    case SC_EVENT_TYPE_PING:
      if (cb_ping != NULL) {
        cb_ping();
      }
      break;
#if HAL_USE_ADC
    case SC_EVENT_TYPE_ADC_AVAILABLE:
      if (cb_adc_available != NULL) {
        cb_adc_available();
      }
      break;
#endif
    case SC_EVENT_TYPE_TEMP_AVAILABLE:
      if (cb_temp_available != NULL) {
        cb_temp_available();
      }
      break;
    case SC_EVENT_TYPE_9DOF_AVAILABLE:
      if (cb_9dof_available != NULL) {
        cb_9dof_available();
      }
      break;
    case SC_EVENT_TYPE_BLOB_AVAILABLE:
      if (cb_blob_available != NULL) {
        cb_blob_available();
      }
      break;
    case SC_EVENT_TYPE_AHRS_AVAILABLE:
      if (cb_ahrs_available != NULL) {
        cb_ahrs_available();
      }
      break;
    case SC_EVENT_TYPE_AUDIO_AVAILABLE:
      if (cb_audio_available != NULL) {
        cb_audio_available();
      }
      break;
    case SC_EVENT_TYPE_SPIRIT1_MSG_AVAILABLE:
      if (cb_spirit1_msg_available != NULL) {
        cb_spirit1_msg_available();
      }
      break;
    case SC_EVENT_TYPE_SPIRIT1_DATA_SENT:
      if (cb_spirit1_data_sent != NULL) {
        cb_spirit1_data_sent();
      }
      break;
    case SC_EVENT_TYPE_SPIRIT1_ERROR:
      if (cb_spirit1_error != NULL) {
        cb_spirit1_error();
      }
      break;
    case SC_EVENT_TYPE_MS5611_AVAILABLE:
      if (cb_ms5611_available != NULL) {
        cb_ms5611_available();
      }
      break;

      // SC internal types
#if HAL_USE_UART
    case SC_EVENT_TYPE_UART_SEND_FINISHED:
      sc_uart_send_finished();
      break;
#endif
    case SC_EVENT_TYPE_NOP:
      // Do nothing
      break;
    default:
      chDbgAssert(0, "Unhandled event");
      break;
    }
  }
}



/*
 * Start a event loop thread
 */
void sc_event_loop_start(void)
{

  chDbgAssert(event_thread == NULL, "Event thread already running");

  // Start a thread dedicated to event loop
  event_thread = chThdCreateStatic(event_loop_thread, sizeof(event_loop_thread),
                                   NORMALPRIO, eventLoopThread, NULL);
}


/*
 * Stop a event loop thread
 */
void sc_event_loop_stop(void)
{
  msg_t nop;

  chDbgAssert(event_thread != NULL, "Event thread not running");

  chThdTerminate(event_thread);

  // Send an event to wake up the thread
  nop = sc_event_msg_create_type(SC_EVENT_TYPE_NOP);
  sc_event_msg_post(nop, SC_EVENT_MSG_POST_FROM_NORMAL);

  chThdWait(event_thread);
  event_thread = NULL;

  // Clear all handlers
#if HAL_USE_UART
  cb_handle_byte = NULL;
#endif
#if HAL_USE_EXT
  {
    uint8_t i;
    for (i = 0; i < EXT_MAX_CHANNELS; ++i) {
      cb_extint[i] = NULL;
    }
  }
#endif

  cb_gsm_state_changed = NULL;
  cb_gsm_cmd_done = NULL;
  cb_ping = NULL;

#if HAL_USE_ADC
  cb_adc_available = NULL;
#endif
  cb_temp_available = NULL;
  cb_9dof_available = NULL;
  cb_blob_available = NULL;
  cb_ahrs_available = NULL;
}




/*
 * Post a mailbox message to main thread. Use helper functions to
 * create the message.
 *
 */
void sc_event_msg_post(msg_t msg, SC_EVENT_MSG_POST_FROM from)
{
  msg_t ret;

  if (from == SC_EVENT_MSG_POST_FROM_ISR) {
    chSysLockFromISR();

    ret = chMBPostI(&event_mb, msg);
    chDbgAssert(ret == MSG_OK, "chMBPostI failed");

    chSysUnlockFromISR();
  } else {

    ret = chMBPost(&event_mb, msg, TIME_IMMEDIATE);
    chDbgAssert(ret == MSG_OK, "chMBPost failed");
  }
}



#if HAL_USE_UART
/*
 * Create a mailbox message from received byte
 */
msg_t sc_event_msg_create_recv_byte(uint8_t byte, SC_UART uart)
{
  msg_t msg = 0;
  SC_EVENT_TYPE type = SC_EVENT_TYPE_PUSH_BYTE;

  // FIXME: assert if uart > 16?
  // FIXME: assert if any of the values too large
  msg =
    (type << EVENT_MSG_TYPE_SHIFT) |
    (uart << EVENT_MSG_UART_SHIFT) |
    (byte << EVENT_MSG_BYTE_SHIFT);

  return msg;
}
#endif



#if HAL_USE_EXT
/*
 * Create a mailbox message from external interrupt
 */
msg_t sc_event_msg_create_extint(uint8_t pin)
{
  msg_t msg = 0;
  SC_EVENT_TYPE type = SC_EVENT_TYPE_EXTINT;

  chDbgAssert(pin < EXT_MAX_CHANNELS, "EXT int pin number too large");

  msg =
    (type << EVENT_MSG_TYPE_SHIFT) |
    (pin  << EVENT_MSG_PIN_SHIFT);

  return msg;
}
#endif



/*
 * Create a mailbox message for event
 */
msg_t sc_event_msg_create_type(SC_EVENT_TYPE type)
{
  msg_t msg = 0;

  // FIXME: asssert if type too large for the reserved bits
  msg = (type << EVENT_MSG_TYPE_SHIFT);

  return msg;
}



#if HAL_USE_UART
/*
 * Register callback for new byte
 */
void sc_event_register_handle_byte(sc_event_cb_handle_byte func)
{
  cb_handle_byte = func;
}
#endif



#if HAL_USE_EXT
/*
 * Register callback for external interrupt
 */
void sc_event_register_extint(uint8_t pin, sc_event_cb_extint func)
{
  chDbgAssert(pin < EXT_MAX_CHANNELS,
              "External interrupt pin number too large");
  cb_extint[pin] = func;
}
#endif

/*
 * Register callback for GSM state changed
 */
void sc_event_register_gsm_state_changed(sc_event_cb_gsm_state_changed func)
{
  cb_gsm_state_changed = func;
}

/*
 * Register callback for GSM command done
 */
void sc_event_register_gsm_cmd_done(sc_event_cb_gsm_cmd_done func)
{
  cb_gsm_cmd_done = func;
}

/*
 * Register callback for external ping message
 */
void sc_event_register_ping(sc_event_cb_ping func)
{
  cb_ping = func;
}

#if HAL_USE_ADC
/*
 * Register callback for new ADC data available
 */
void sc_event_register_adc_available(sc_event_cb_adc_available func)
{
  cb_adc_available = func;
}
#endif



/*
 * Register callback for new temperature data available
 */
void sc_event_register_temp_available(sc_event_cb_temp_available func)
{
  cb_temp_available = func;
}



/*
 * Register callback for new 9dof data available
 */
void sc_event_register_9dof_available(sc_event_cb_9dof_available func)
{
  cb_9dof_available = func;
}



/*
 * Register callback for new blob data available
 */
void sc_event_register_blob_available(sc_event_cb_blob_available func)
{
  cb_blob_available = func;
}



/*
 * Register callback for new AHRS data available
 */
void sc_event_register_ahrs_available(sc_event_cb_ahrs_available func)
{
  cb_ahrs_available = func;
}



/*
 * Register callback for new audio data available
 */
void sc_event_register_audio_available(sc_event_cb_audio_available func)
{
  cb_audio_available = func;
}



/*
 * Register callback for new spirit1 message available
 */
void sc_event_register_spirit1_msg_available(sc_event_cb_spirit1_msg_available func)
{
  cb_spirit1_msg_available = func;
}



/*
 * Register callback for spirit1 data sent notification
 */
void sc_event_register_spirit1_data_sent(sc_event_cb_spirit1_data_sent func)
{
  cb_spirit1_data_sent = func;
}



/*
 * Register callback for spirit1 data lost notification
 */
void sc_event_register_spirit1_data_lost(sc_event_cb_spirit1_data_lost func)
{
  cb_spirit1_data_lost = func;
}



/*
 * Register callback for spirit1 error notification
 */
void sc_event_register_spirit1_error(sc_event_cb_spirit1_error func)
{
  cb_spirit1_error = func;
}



/*
 * Register callback for new MS5611 data available
 */
void sc_event_register_ms5611_available(sc_event_cb_ms5611_available func)
{
  cb_ms5611_available = func;
}




/* Emacs indentatation information
   Local Variables:
   indent-tabs-mode:nil
   tab-width:2
   c-basic-offset:2
   End:
*/
