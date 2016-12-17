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
#include "sc_cmd.h"
#include "sc_gpio.h"

#include <stdio.h>        // sscanf

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

static void parse_command_gpio(const uint8_t *param, uint8_t param_len);
static void parse_command_gpio_all(const uint8_t *param, uint8_t param_len);

static const struct sc_cmd cmds[] = {
  {"gpio",          SC_CMD_HELP("Set GPIO (0-7) to low/high/toggle (0/1/t)"), parse_command_gpio},
  {"gpio_all",      SC_CMD_HELP("Set all GPIOs (XXXXXXXX, where X = 0/1)"), parse_command_gpio_all},
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
  for (i = 0; i < SC_ARRAY_SIZE(cmds); ++i) {
    sc_cmd_register(cmds[i].cmd, cmds[i].help, cmds[i].cmd_cb);
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
 * Turn the gpio on
 */
void sc_gpio_on(uint8_t gpio)
{
  if (gpio == 0 || gpio > SC_GPIO_MAX_PINS) {
    chDbgAssert(0, "GPIO pin outside range");
    return;
  }
  if (!gpio_list[gpio].valid) {
    chDbgAssert(0, "GPIO pin not valid");
    return;
  }

  palSetPad(gpio_list[gpio].port, gpio_list[gpio].pin);
}



/*
 * Turn the gpio off
 */
void sc_gpio_off(uint8_t gpio)
{
  if (gpio == 0 || gpio > SC_GPIO_MAX_PINS) {
    chDbgAssert(0, "GPIO pin outside range");
    return;
  }
  if (!gpio_list[gpio].valid) {
    chDbgAssert(0, "GPIO pin not valid");
    return;
  }

  palClearPad(gpio_list[gpio].port, gpio_list[gpio].pin);
}



/*
 * Toggle gpio
 */
void sc_gpio_toggle(uint8_t gpio)
{
  if (gpio == 0 || gpio > SC_GPIO_MAX_PINS) {
    chDbgAssert(0, "GPIO pin outside range");
    return;
  }
  if (!gpio_list[gpio].valid) {
    chDbgAssert(0, "GPIO pin not valid");
    return;
  }

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

/*
 * Return the state of the gpio
 */
uint8_t sc_gpio_state(uint8_t gpio)
{
  if (gpio == 0 || gpio > SC_GPIO_MAX_PINS) {
    chDbgAssert(0, "GPIO pin outside range");
    return 0;
  }
  if (!gpio_list[gpio].valid) {
    chDbgAssert(0, "GPIO pin not valid");
    return 0;
  }

  return palReadPad(gpio_list[gpio].port, gpio_list[gpio].pin);
}

/*
 * Return the state of all the gpios as a bitmask
 * NOTE: Only valid pins are accurate, rest are zeroed
 */
uint8_t sc_gpio_get_state_all(void)
{
  uint8_t i;
  uint8_t gpios = 0;

  for (i = 0; i < SC_GPIO_MAX_PINS; ++i) {
    if (gpio_list[i + 1].valid
     && palReadPad(gpio_list[i].port, gpio_list[i].pin)) {
        gpios |= (1 << i);
    }
  }
  return gpios;
}



/*
 * Parse gpio command
 */
static void parse_command_gpio(const uint8_t *param, uint8_t param_len)
{
  uint8_t gpio;
  uint8_t value;

  (void)param_len;

  if (!param) {
    // Invalid message
    return;
  }

  if (sscanf((char*)param, "%hhu %c", &gpio, &value) < 2) {
    return;
  }

  switch (value) {
  case '0':
    sc_gpio_off(gpio);
    break;
  case '1':
    sc_gpio_on(gpio);
    break;
  case 't':
    sc_gpio_toggle(gpio);
    break;
  default:
    // Invalid value, ignoring command
    break;
  }
}



/*
 * Parse gpio_all command (GXXXX)
 */
static void parse_command_gpio_all(const uint8_t *param, uint8_t param_len)
{
  (void)param_len;

  uint8_t gpio, gpios = 0;

  for (gpio = 0; gpio < SC_GPIO_MAX_PINS; ++gpio) {
    if (param[gpio] == '1') {
      gpios |= (1 << gpio);
    } else if (param[gpio] != '0') {
      /* If it's neither 1 nor 0, the command has
       * ended and we leave the rest bits as zero.
       * NOTE: this might have side-effects
       */
      break;
    }
  }

  sc_gpio_set_state_all(gpios);
}

#endif // HAL_USE_PAL

/* Emacs indentatation information
   Local Variables:
   indent-tabs-mode:nil
   tab-width:2
   c-basic-offset:2
   End:
*/
