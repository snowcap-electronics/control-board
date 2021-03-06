/***
 * GPIO functions
 *
 * Copyright 2013 Tuomas Kulve, <tuomas.kulve@snowcap.fi>
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

#if HAL_USE_PAL
#ifndef SC_GPIO_H
#define SC_GPIO_H

#define SC_GPIO_MAX_PINS 8

void sc_gpio_init(void);
void sc_gpio_deinit(void);
void sc_gpio_on(uint8_t gpio);
void sc_gpio_off(uint8_t gpio);
void sc_gpio_toggle(uint8_t gpio);
void sc_gpio_set_state_all(uint8_t gpios);
uint8_t sc_gpio_state(uint8_t gpio);
uint8_t sc_gpio_get_state_all(void);

#endif
#endif
