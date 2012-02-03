/***
 * User thread (empty thread for easy addition of functionality)
 *
 * Copyright 2011 Tuomas Kulve, <tuomas.kulve@snowcap.fi>
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

#include "sc_utils.h"
#include "sc_user_thread.h"
#include "sc_uart.h"


static msg_t userThread(void *UNUSED(arg));

static uint8_t msg[] = {'t','e','s','t','i','n','g','\r','\n'};

/*
 * Setup a working area with a 32 byte stack for user thread
 */
static WORKING_AREA(user_thread, 32);
static msg_t userThread(void *UNUSED(arg))
{

  // Threads are active until they return, we don't want to
  while (TRUE) {
	// Do nothing with 1 sec interval
	chThdSleepMilliseconds(1000);
	sc_uart_send_msg(SC_UART_LAST, msg, 9);
  }
  
  return 0;
}


/*
 * Start a user thread
 */
void sc_user_thread_init(void)
{
  // Start a thread dedicated to user specific things
  chThdCreateStatic(user_thread, sizeof(user_thread), NORMALPRIO, userThread, NULL);
}
