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

#include "sc_event.h"
#include "sc_cmd.h"


// Action semaphore used in event loop to signal action from other
// threads and interrupt handlers.
static Semaphore action_sem;

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
	// Fixme: Use flags to do only the flagged action?
	sc_cmd_parse_command();
  }
}



/*
 * Notify the main thread to check for actions. This can be called
 * from other threads and interrupt handlers.
 */
void sc_event_action(void)
{
  chSemSignalI(&action_sem);
}



/* Emacs indentatation information
   Local Variables:
   indent-tabs-mode:nil
   tab-width:2
   c-basic-offset:2
   End:
*/
