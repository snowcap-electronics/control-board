/***
 * PWM functions
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
#include "sc_pwm.h"

// Use 1MHz clock
#define SC_PWM_CLOCK    1000000

static PWMConfig pwmcfg = {
  SC_PWM_CLOCK,      // 1 MHz PWM clock frequency
  SC_PWM_CLOCK / 50, // 50 Hz (20.0 ms) initial frequency
  NULL,              // No periodic callback.
  {
    // Enable channels 1 through 4 (all)
    {PWM_OUTPUT_ACTIVE_HIGH, NULL},
    {PWM_OUTPUT_ACTIVE_HIGH, NULL},
    {PWM_OUTPUT_DISABLED, NULL},
    {PWM_OUTPUT_DISABLED, NULL}
  },
  // HW dependent part
  0, // TIM CR2 register initialization data
#if STM32_PWM_USE_ADVANCED
  0, // TIM BDTR (break & dead-time) register initialization data.
#endif
};


/*
 * Initialize PWM
 */
void sc_pwm_init(void)
{
  // Start the second PWM driver driven by TIM2 timer
  pwmStart(&PWMDX, &pwmcfg);
}



/*
 * Set frequency in Hz (e.g. 50 for standard servo/ESC, 400 for fast servo/ESC)
 */
void sc_pwm_set_freq(uint16_t freq)
{
  // pwmChangePeriod(&PWMDX, SC_PWM_CLOCK / freq);
}



/*
 * Stop PWM signal
 */
void sc_pwm_stop(int pwm)
{
  if (pwm >= 1 && pwm <= 4) {
	pwmDisableChannel(&PWMDX, pwm - 1);
  }
}



/*
 * Set duty cycle. 0 for 0%, 10000 for 100%
 */
void sc_pwm_set_duty(int pwm, uint16_t duty)
{
  if (duty > 10000) {
	duty = 10000;
  }
  if (pwm >= 1 && pwm <= 4) {
	pwmEnableChannel(&PWMDX, pwm - 1, PWM_PERCENTAGE_TO_WIDTH(&PWMDX, duty));
  }
}
