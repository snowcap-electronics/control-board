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
#include <sc_utils.h>

typedef enum SC_EVENT_TYPE {
  SC_EVENT_TYPE_PUSH_BYTE = 1,
  SC_EVENT_TYPE_UART_SEND_FINISHED,
  SC_EVENT_TYPE_MAX
} SC_EVENT_TYPE;

typedef enum SC_EVENT_MSG_POST_FROM {
  SC_EVENT_MSG_POST_FROM_ISR,
  SC_EVENT_MSG_POST_FROM_NORMAL,
} SC_EVENT_MSG_POST_FROM;

void sc_event_loop(void);
void sc_event_action(SC_EVENT_TYPE type);
void sc_event_msg_post(msg_t msg, SC_EVENT_MSG_POST_FROM from);
msg_t sc_event_msg_create_recv_byte(uint8_t byte, SC_UART uart);
msg_t sc_event_msg_create_type(SC_EVENT_TYPE type);

#endif



/* Emacs indentatation information
   Local Variables:
   indent-tabs-mode:nil
   tab-width:2
   c-basic-offset:2
   End:
*/
