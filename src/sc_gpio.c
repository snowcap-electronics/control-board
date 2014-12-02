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

#if HAL_USE_PAL

struct gpio_list {
  ioportid_t port;
  uint8_t pin;
  uint8_t valid;
};

struct gpio_list gpio_list[SC_GPIO_MAX_PINS + 1] = {
  {0, 0, 0},
  {GPIO1_PORT, GPIO1_PIN, 1},
  {GPIO2_PORT, GPIO2_PIN, 1},
  {GPIO3_PORT, GPIO3_PIN, 1},
  {GPIO4_PORT, GPIO4_PIN, 1},
#ifdef GPIO5_PORT
  {GPIO5_PORT, GPIO5_PIN, 1},
#endif
#ifdef GPIO6_PORT
  {GPIO6_PORT, GPIO6_PIN, 1},
#endif
#ifdef GPIO7_PORT
  {GPIO7_PORT, GPIO7_PIN, 1},
#endif
#ifdef GPIO8_PORT
  {GPIO8_PORT, GPIO8_PIN, 1},
#endif
};


/*
 * Set pinmux
 */
void sc_gpio_init(void)
{
  uint8_t i;
  for (i = 0; i < SC_GPIO_MAX_PINS; ++i) {
    if (gpio_list[i+1].valid) {
        ioportid_t port = gpio_list[i + 1].port;
        uint8_t pin = gpio_list[i + 1].pin;
        palSetPadMode(port, pin, PAL_MODE_OUTPUT_PUSHPULL);
    }
  }
}



/*
 * Deinit
 */
void sc_gpio_deinit(void)
{
  // Nothing to do here.
}



/*
 * Turn the led on
 */
void sc_gpio_on(uint8_t gpio)
{
  chDbgAssert(gpio > 0 || gpio <= SC_GPIO_MAX_PINS, "GPIO pin outside range", "#1");
  chDbgAssert(gpio_list[gpio].valid, "GPIO pin not valid", "#1");

  palSetPad(gpio_list[gpio].port, gpio_list[gpio].pin);
}



/*
 * Turn the gpio off
 */
void sc_gpio_off(uint8_t gpio)
{
  chDbgAssert(gpio > 0 || gpio <= SC_GPIO_MAX_PINS, "GPIO pin outside range", "#2");
  chDbgAssert(gpio_list[gpio].valid, "GPIO pin not valid", "#2");

  palClearPad(gpio_list[gpio].port, gpio_list[gpio].pin);
}



/*
 * Toggle gpio
 */
void sc_gpio_toggle(uint8_t gpio)
{
  chDbgAssert(gpio > 0 || gpio <= SC_GPIO_MAX_PINS, "GPIO pin outside range", "#3");
  chDbgAssert(gpio_list[gpio].valid, "GPIO pin not valid", "#3");

  palTogglePad(gpio_list[gpio].port, gpio_list[gpio].pin);
}



/*
 * Set the state of all GPIOs
 */
void sc_gpio_set_state_all(uint8_t gpios)
{
  uint8_t i;
  for (i = 0; i < SC_GPIO_MAX_PINS; ++i) {
    if (gpio_list[i + 1].valid) {
        if (gpios & 1) {
          palSetPad(gpio_list[i + 1].port, gpio_list[i + 1].pin);
        } else {
          palClearPad(gpio_list[i + 1].port, gpio_list[i + 1].pin);
        }
    }
    gpios >>= 1;
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
