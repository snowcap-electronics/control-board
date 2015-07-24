/***
 * Led functions
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
#include "sc_led.h"
#include "sc_cmd.h"

#if HAL_USE_PAL

static void parse_command_led(const uint8_t *param, uint8_t param_len);

static const struct sc_cmd cmds[] = {
  {"led",           SC_CMD_HELP("Set LED off/on/toggle (0/1/t)"), parse_command_led},
};

void sc_led_init(void)
{
  uint8_t i;
  for (i = 0; i < SC_ARRAY_SIZE(cmds); ++i) {
    sc_cmd_register(cmds[i].cmd, cmds[i].help, cmds[i].cmd_cb);
  }
  palSetPadMode(USER_LED_PORT,
                USER_LED,
                PAL_MODE_OUTPUT_PUSHPULL);
}



void sc_led_deinit(void)
{
  // Nothing to do here.
}



/*
 * Turn the led on
 */
void sc_led_on(void)
{
  palSetPad(USER_LED_PORT, USER_LED);
}



/*
 * Turn the led off
 */
void sc_led_off(void)
{
  palClearPad(USER_LED_PORT, USER_LED);
}



/*
 * Toggle led
 */
void sc_led_toggle(void)
{
  palTogglePad(USER_LED_PORT, USER_LED);
}



/*
 * Parse led command
 */
static void parse_command_led(const uint8_t *param, uint8_t param_len)
{
  (void)param_len;

  switch (param[0]) {
  case '0':
    sc_led_off();
    break;
  case '1':
    sc_led_on();
    break;
  case 't':
    sc_led_toggle();
    break;
  default:
    // Invalid value, ignoring command
	return;
  }
}

#endif // HAL_USE_PAL



/* Emacs indentatation information
   Local Variables:
   indent-tabs-mode:nil
   tab-width:2
   c-basic-offset:2
   End:
*/

