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

#ifndef SC_EVENT_H
#define SC_EVENT_H

#include "sc_uart.h"
#include "sc_utils.h"

// Event types
typedef enum SC_EVENT_TYPE {
  // Application registrable events
  SC_EVENT_TYPE_PUSH_BYTE = 1,
  SC_EVENT_TYPE_NOP,
  SC_EVENT_TYPE_EXTINT,
  SC_EVENT_TYPE_GSM_STATE_CHANGED,
  SC_EVENT_TYPE_GSM_CMD_DONE,
  SC_EVENT_TYPE_PING,
  SC_EVENT_TYPE_ADC_AVAILABLE,
  SC_EVENT_TYPE_TEMP_AVAILABLE,
  SC_EVENT_TYPE_9DOF_AVAILABLE,
  SC_EVENT_TYPE_BLOB_AVAILABLE,
  SC_EVENT_TYPE_AHRS_AVAILABLE,
  SC_EVENT_TYPE_AUDIO_AVAILABLE,
  SC_EVENT_TYPE_SPIRIT1_MSG_AVAILABLE,
  SC_EVENT_TYPE_SPIRIT1_DATA_SENT,
  SC_EVENT_TYPE_SPIRIT1_DATA_LOST,
  SC_EVENT_TYPE_MS5611_AVAILABLE,
  // SC internal types
  SC_EVENT_TYPE_UART_SEND_FINISHED,
  SC_EVENT_TYPE_MAX
} SC_EVENT_TYPE;

// Whether or not sc_event_msg_post is called inside ISR
typedef enum SC_EVENT_MSG_POST_FROM {
  SC_EVENT_MSG_POST_FROM_ISR,
  SC_EVENT_MSG_POST_FROM_NORMAL,
} SC_EVENT_MSG_POST_FROM;

// Callback function definitions
#if HAL_USE_UART
typedef void (*sc_event_cb_handle_byte)(SC_UART uart, uint8_t byte);
#endif
typedef void (*sc_event_cb_extint)(void);
typedef void (*sc_event_cb_gsm_state_changed)(void);
typedef void (*sc_event_cb_gsm_cmd_done)(void);
typedef void (*sc_event_cb_ping)(void);
typedef void (*sc_event_cb_adc_available)(void);
typedef void (*sc_event_cb_temp_available)(void);
typedef void (*sc_event_cb_9dof_available)(void);
typedef void (*sc_event_cb_blob_available)(void);
typedef void (*sc_event_cb_ahrs_available)(void);
typedef void (*sc_event_cb_audio_available)(void);
typedef void (*sc_event_cb_spirit1_msg_available)(void);
typedef void (*sc_event_cb_spirit1_data_sent)(void);
typedef void (*sc_event_cb_spirit1_data_lost)(void);
typedef void (*sc_event_cb_ms5611_available)(void);

void sc_event_loop_start(void);
void sc_event_loop_stop(void);
void sc_event_msg_post(msg_t msg, SC_EVENT_MSG_POST_FROM from);
#if HAL_USE_UART
msg_t sc_event_msg_create_recv_byte(uint8_t byte, SC_UART uart);
#endif
msg_t sc_event_msg_create_extint(uint8_t pin);
msg_t sc_event_msg_create_type(SC_EVENT_TYPE type);

#if HAL_USE_UART
void sc_event_register_handle_byte(sc_event_cb_handle_byte func);
#endif
void sc_event_register_extint(uint8_t pin, sc_event_cb_extint func);
// TODO: these could be generalized? E.g.:
// void sc_event_register_data_available(SC_EVENT_TYPE, sc_event_cb_data_available func);
// void sc_event_register_cb(SC_EVENT_TYPE type, sc_event_cb func);
void sc_event_register_gsm_state_changed(sc_event_cb_gsm_state_changed func);
void sc_event_register_gsm_cmd_done(sc_event_cb_gsm_cmd_done func);
void sc_event_register_ping(sc_event_cb_ping func);
void sc_event_register_adc_available(sc_event_cb_adc_available func);
void sc_event_register_temp_available(sc_event_cb_temp_available func);
void sc_event_register_9dof_available(sc_event_cb_9dof_available func);
void sc_event_register_blob_available(sc_event_cb_blob_available func);
void sc_event_register_ahrs_available(sc_event_cb_9dof_available func);
void sc_event_register_audio_available(sc_event_cb_9dof_available func);
void sc_event_register_spirit1_msg_available(sc_event_cb_spirit1_msg_available func);
void sc_event_register_spirit1_data_sent(sc_event_cb_spirit1_data_sent func);
void sc_event_register_spirit1_data_lost(sc_event_cb_spirit1_data_lost func);
void sc_event_register_ms5611_available(sc_event_cb_ms5611_available func);

#endif



/* Emacs indentatation information
   Local Variables:
   indent-tabs-mode:nil
   tab-width:2
   c-basic-offset:2
   End:
*/
