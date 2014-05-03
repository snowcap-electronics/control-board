/*
 * Small test firmware
 *
 * Copyright 2014 Tuomas Kulve <tuomas.kulve@snowcap.fi>
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

#include "sc.h"

int main(void)
{
  halInit();

  sc_init(SC_MODULE_LED);

  // Start event loop. This will start a new thread and return
  sc_event_loop_start();

#if CH_NO_IDLE_THREAD
  chThdSetPriority(IDLEPRIO);
#endif

  // Loop forever waiting for callbacks
  while(1) {
    sc_led_toggle();

#if CH_NO_IDLE_THREAD
    chDbgAssert(0, "Can't call chThdSleepMilliseconds() without idle thread", "#1");
#endif
    chThdSleepMilliseconds(1000);
  }

  return 0;
}

/* Emacs indentatation information
   Local Variables:
   indent-tabs-mode:nil
   tab-width:2
   c-basic-offset:2
   End:
*/
