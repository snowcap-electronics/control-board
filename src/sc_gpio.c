/***
 * GPIO functions
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
#include "sc_gpio.h"

struct gpio_list {
  ioportid_t port;
  uint8_t pin;
};

struct gpio_list gpio_list[SC_GPIO_MAX_PINS + 1] = {
  {0, 0},
  {GPIO1_PORT, GPIO1_PIN},
  {GPIO2_PORT, GPIO2_PIN},
  {GPIO3_PORT, GPIO3_PIN},
  {GPIO4_PORT, GPIO4_PIN},
};


/*
 * Set pinmux
 */
void sc_gpio_init(void)
{
  uint8_t i;
  for (i = 0; i < 4; ++i) {
	palSetPadMode(gpio_list[i].port, gpio_list[i].pin, PAL_MODE_OUTPUT_PUSHPULL);
  }
}

/*
 * Turn the led on
 */
void sc_gpio_on(uint8_t gpio)
{
  palSetPad(gpio_list[gpio].port, gpio_list[gpio].pin);
}



/*
 * Turn the gpio off
 */
void sc_gpio_off(uint8_t gpio)
{
  palClearPad(gpio_list[gpio].port, gpio_list[gpio].pin);
}



/*
 * Toggle gpio
 */
void sc_gpio_toggle(uint8_t gpio)
{
  palTogglePad(gpio_list[gpio].port, gpio_list[gpio].pin);
}



/*
 * Set the state of all GPIOs
 */
void sc_gpio_set_state_all(uint8_t gpios)
{
  uint8_t i;
  for (i = 0; i < SC_GPIO_MAX_PINS; ++i) {
	if (gpios & 1) {
	  palSetPad(gpio_list[i].port, gpio_list[i].pin);
	} else {
	  palClearPad(gpio_list[i].port, gpio_list[i].pin);
	}

	gpios >>= 1;
  }
}
