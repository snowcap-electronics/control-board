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


/* Action semaphore used in event loop to signal action from other
 * threads and interrupt handlers. */
static Semaphore action_sem;

/*
 * */
static SC_EVENT_TYPE events[SC_EVENT_TYPE_MAX] = { 0 };

/*
 * Run the event loop. This must be called from the main thread.
 */
void sc_event_loop(void)
{

  // Initialize the action semaphore
  chSemInit(&action_sem, 0);

  while (TRUE) {

    // Wait for action
    chSemWait(&action_sem);

    // Go through all possible actions

    if (events[SC_EVENT_TYPE_PARSE_COMMAND]) {
      chSysLock();
      --events[SC_EVENT_TYPE_PARSE_COMMAND];
      chSysUnlock();
      sc_cmd_parse_command();
    }

    if (events[SC_EVENT_TYPE_UART_SEND_FINISHED]) {
      chSysLock();
      --events[SC_EVENT_TYPE_UART_SEND_FINISHED];
      chSysUnlock();
      sc_uart_send_finished();
    }
  }
}



/*
 * Notify the main thread to check for actions. This must be called with either
 * chSysLockFromIsr or chSysLock already hold
 */
void sc_event_action(SC_EVENT_TYPE type)
{
  ++events[type];

  chSemSignalI(&action_sem);
}



/* Emacs indentatation information
   Local Variables:
   indent-tabs-mode:nil
   tab-width:2
   c-basic-offset:2
   End:
*/
