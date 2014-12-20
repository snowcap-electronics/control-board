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

#if HAL_USE_PWM

// Use 1MHz clock
#define SC_PWM_CLOCK    1000000

static PWMConfig pwmcfg1 = {
  SC_PWM_CLOCK,      // 1 MHz PWM clock frequency
  SC_PWM_CLOCK / 50, // 50 Hz (20.0 ms) initial frequency
  NULL,              // No periodic callback.
  {
#ifdef SC_PWM1_1_PIN
    {PWM_OUTPUT_ACTIVE_HIGH, NULL},
#else
    {PWM_OUTPUT_DISABLED, NULL},
#endif
#ifdef SC_PWM1_2_PIN
    {PWM_OUTPUT_ACTIVE_HIGH, NULL},
#else
    {PWM_OUTPUT_DISABLED, NULL},
#endif
#ifdef SC_PWM1_3_PIN
    {PWM_OUTPUT_ACTIVE_HIGH, NULL},
#else
    {PWM_OUTPUT_DISABLED, NULL},
#endif
#ifdef SC_PWM1_4_PIN
    {PWM_OUTPUT_ACTIVE_HIGH, NULL}
#else
    {PWM_OUTPUT_DISABLED, NULL}
#endif
  },
  // HW dependent part
  0, // TIM CR2 register initialization data
  0, // TIM BDTR (break & dead-time) register initialization data.
};

static PWMConfig pwmcfg2 = {
  SC_PWM_CLOCK,      // 1 MHz PWM clock frequency
  SC_PWM_CLOCK / 50, // 50 Hz (20.0 ms) initial frequency
  NULL,              // No periodic callback.
  {
#ifdef SC_PWM2_1_PIN
    {PWM_OUTPUT_ACTIVE_HIGH, NULL},
#else
    {PWM_OUTPUT_DISABLED, NULL},
#endif
#ifdef SC_PWM2_2_PIN
    {PWM_OUTPUT_ACTIVE_HIGH, NULL},
#else
    {PWM_OUTPUT_DISABLED, NULL},
#endif
#ifdef SC_PWM2_3_PIN
    {PWM_OUTPUT_ACTIVE_HIGH, NULL},
#else
    {PWM_OUTPUT_DISABLED, NULL},
#endif
#ifdef SC_PWM2_4_PIN
    {PWM_OUTPUT_ACTIVE_HIGH, NULL}
#else
    {PWM_OUTPUT_DISABLED, NULL}
#endif
  },
  // HW dependent part
  0, // TIM CR2 register initialization data
  0, // TIM BDTR (break & dead-time) register initialization data.
};


/*
 * Initialize PWM
 */
void sc_pwm_init(void)
{

#ifdef SC_PWM1_1_PIN
  palSetPadMode(SC_PWM1_1_PORT, SC_PWM1_1_PIN, PAL_MODE_ALTERNATE(SC_PWM1_1_AF));
#endif

#ifdef SC_PWM1_2_PIN
  palSetPadMode(SC_PWM1_2_PORT, SC_PWM1_2_PIN, PAL_MODE_ALTERNATE(SC_PWM1_2_AF));
#endif

#ifdef SC_PWM1_3_PIN
  palSetPadMode(SC_PWM1_3_PORT, SC_PWM1_3_PIN, PAL_MODE_ALTERNATE(SC_PWM1_3_AF));
#endif

#ifdef SC_PWM1_4_PIN
  palSetPadMode(SC_PWM1_4_PORT, SC_PWM1_4_PIN, PAL_MODE_ALTERNATE(SC_PWM1_4_AF));
#endif

#ifdef SC_PWM2_1_PIN
  palSetPadMode(SC_PWM2_1_PORT, SC_PWM2_1_PIN, PAL_MODE_ALTERNATE(SC_PWM2_1_AF));
#endif

#ifdef SC_PWM2_2_PIN
  palSetPadMode(SC_PWM2_2_PORT, SC_PWM2_2_PIN, PAL_MODE_ALTERNATE(SC_PWM2_2_AF));
#endif

#ifdef SC_PWM2_3_PIN
  palSetPadMode(SC_PWM2_3_PORT, SC_PWM2_3_PIN, PAL_MODE_ALTERNATE(SC_PWM2_3_AF));
#endif

#ifdef SC_PWM2_4_PIN
  palSetPadMode(SC_PWM2_4_PORT, SC_PWM2_4_PIN, PAL_MODE_ALTERNATE(SC_PWM2_4_AF));
#endif

#ifdef PWMDX1
  pwmStart(&PWMDX1, &pwmcfg1);
#endif

#ifdef PWMDX2
  pwmStart(&PWMDX2, &pwmcfg2);
#endif
}


/*
 * Deinitialize PWM
 */
void sc_pwm_deinit(void)
{
  pwmStop(&PWMDX1);

#ifdef PWMDX2
  pwmStop(&PWMDX2);
#endif
}



/*
 * Set frequency in Hz (e.g. 50 for standard servo/ESC, 400 for fast servo/ESC)
 */
void sc_pwm_set_freq(uint16_t freq)
{
  pwmChangePeriod(&PWMDX1, SC_PWM_CLOCK / freq);

#ifdef PWMDX2
  pwmChangePeriod(&PWMDX2, SC_PWM_CLOCK / freq);
#endif
}



/*
 * Stop PWM signal
 */
void sc_pwm_stop(int pwm)
{
  if (pwm >= 1 && pwm <= 4) {
	pwmDisableChannel(&PWMDX1, pwm - 1);
  }

  if (pwm >= 5 && pwm <= 6) {
	// Not enabled, do nothing
	return;
  }

  if (pwm >= 7 && pwm <= 8) {
#ifdef PWMDX2
	pwmDisableChannel(&PWMDX2, pwm - 7);
#else
	chDbgAssert(0, "PWMDX2 not defined", "#1");
#endif
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
#ifdef PWMDX1
	pwmEnableChannel(&PWMDX1, pwm - 1, PWM_PERCENTAGE_TO_WIDTH(&PWMDX1, duty));
#else
	chDbgAssert(0, "PWMDX1 not defined", "#2");
#endif
  }

  if (pwm >= 5 && pwm <= 8) {
#ifdef PWMDX2
	pwmEnableChannel(&PWMDX2, pwm - 5, PWM_PERCENTAGE_TO_WIDTH(&PWMDX2, duty));
#else
	chDbgAssert(0, "PWMDX2 not defined", "#2");
#endif
  }
}

#endif // HAL_USE_PWM == TRUE
