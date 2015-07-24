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
#include "sc_cmd.h"

#include <stdio.h>        // sscanf

#if HAL_USE_PWM

// Use 1MHz clock
#define SC_PWM_CLOCK    1000000

static void parse_command_pwm_frequency(const uint8_t *param, uint8_t param_len);
static void parse_command_pwm_duty(const uint8_t *param, uint8_t param_len);
static void parse_command_pwm_stop(const uint8_t *param, uint8_t param_len);

static const struct sc_cmd cmds[] = {
  {"pwm_frequency", SC_CMD_HELP("Set PWM frequency (Hz) for all PWMs"), parse_command_pwm_frequency},
  {"pwm_duty",      SC_CMD_HELP("Set PWM (0-16) duty cycle (0-10000)"), parse_command_pwm_duty},
  {"pwm_stop",      SC_CMD_HELP("Stop PWM (0-16)"), parse_command_pwm_stop},
};


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

#ifdef PWMDX2
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
#endif

#ifdef PWMDX3
static PWMConfig pwmcfg3 = {
  SC_PWM_CLOCK,      // 1 MHz PWM clock frequency
  SC_PWM_CLOCK / 10000, // 10 kHz initial frequency
  NULL,              // No periodic callback.
  {
#ifdef SC_PWM3_1_PIN
    {PWM_OUTPUT_ACTIVE_HIGH, NULL},
#else
    {PWM_OUTPUT_DISABLED, NULL},
#endif
#ifdef SC_PWM3_2_PIN
    {PWM_OUTPUT_ACTIVE_HIGH, NULL},
#else
    {PWM_OUTPUT_DISABLED, NULL},
#endif
#ifdef SC_PWM3_3_PIN
    {PWM_OUTPUT_ACTIVE_HIGH, NULL},
#else
    {PWM_OUTPUT_DISABLED, NULL},
#endif
#ifdef SC_PWM3_4_PIN
    {PWM_OUTPUT_ACTIVE_HIGH, NULL}
#else
    {PWM_OUTPUT_DISABLED, NULL}
#endif
  },
  // HW dependent part
  0, // TIM CR2 register initialization data
  0, // TIM BDTR (break & dead-time) register initialization data.
};
#endif

/*
 * Initialize PWM
 */
void sc_pwm_init(void)
{
  uint8_t i;

  for (i = 0; i < SC_ARRAY_SIZE(cmds); ++i) {
    sc_cmd_register(cmds[i].cmd, cmds[i].help, cmds[i].cmd_cb);
  }

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

#ifdef PWMDX1
  pwmStart(&PWMDX1, &pwmcfg1);
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

#ifdef PWMDX2
  pwmStart(&PWMDX2, &pwmcfg2);
#endif

#ifdef SC_PWM3_1_PIN
  palSetPadMode(SC_PWM3_1_PORT, SC_PWM3_1_PIN, PAL_MODE_ALTERNATE(SC_PWM3_1_AF));
#endif

#ifdef SC_PWM3_2_PIN
  palSetPadMode(SC_PWM3_2_PORT, SC_PWM3_2_PIN, PAL_MODE_ALTERNATE(SC_PWM3_2_AF));
#endif

#ifdef SC_PWM3_3_PIN
  palSetPadMode(SC_PWM3_3_PORT, SC_PWM3_3_PIN, PAL_MODE_ALTERNATE(SC_PWM3_3_AF));
#endif

#ifdef SC_PWM3_4_PIN
  palSetPadMode(SC_PWM3_4_PORT, SC_PWM3_4_PIN, PAL_MODE_ALTERNATE(SC_PWM3_4_AF));
#endif

#ifdef PWMDX3
  pwmStart(&PWMDX3, &pwmcfg3);
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

#ifdef PWMDX3
  pwmStop(&PWMDX3);
#endif
}



/*
 * Set frequency in Hz (e.g. 50 for standard servo/ESC, 400 for fast servo/ESC)
 */
// FIXME: Need to be able to control this per PWMDX
void sc_pwm_set_freq(uint16_t freq)
{
  if (freq == 0) {
	// freq 0 is invalid, do nothing
	return;
  }
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

  if (pwm >= 5 && pwm <= 8) {
#ifdef PWMDX2
	pwmDisableChannel(&PWMDX2, pwm - 5);
#else
	chDbgAssert(0, "PWMDX2 not defined");
#endif
  }

  if (pwm >= 9 && pwm <= 12) {
#ifdef PWMDX3
	pwmDisableChannel(&PWMDX3, pwm - 9);
#else
	chDbgAssert(0, "PWMDX3 not defined");
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
	chDbgAssert(0, "PWMDX1 not defined");
#endif
  }

  if (pwm >= 5 && pwm <= 8) {
#ifdef PWMDX2
	pwmEnableChannel(&PWMDX2, pwm - 5, PWM_PERCENTAGE_TO_WIDTH(&PWMDX2, duty));
#else
	chDbgAssert(0, "PWMDX2 not defined");
#endif
  }

  if (pwm >= 9 && pwm <= 12) {
#ifdef PWMDX3
	pwmEnableChannel(&PWMDX3, pwm - 9, PWM_PERCENTAGE_TO_WIDTH(&PWMDX3, duty));
#else
	chDbgAssert(0, "PWMDX2 not defined");
#endif
  }
}



/*
 * Parse PWM frequency command
 */
static void parse_command_pwm_frequency(const uint8_t *param, uint8_t param_len)
{
  uint16_t freq;

  (void)param_len;

  if (!param) {
    // Invalid message
    return;
  }

  // Parse the frequency value
  if (sscanf((char*)param, "%hu", &freq) < 1) {
    return;
  }

  sc_pwm_set_freq(freq);
}



/*
 * Parse PWM duty cycle command
 */
static void parse_command_pwm_duty(const uint8_t *param, uint8_t param_len)
{
  uint8_t pwm;
  uint16_t value;

  (void)param_len;

  if (!param) {
    // Invalid message
    return;
  }

  if (sscanf((char*)param, "%hhu %hu", &pwm, &value) < 2) {
    return;
  }

  sc_pwm_set_duty(pwm, value);
}



/*
 * Parse PWM stop command
 */
static void parse_command_pwm_stop(const uint8_t *param, uint8_t param_len)
{
  uint8_t pwm;

  (void)param_len;

  if (!param) {
    // Invalid message
    return;
  }

  if (sscanf((char*)param, "%hhu", &pwm) < 1) {
    return;
  }

  sc_pwm_stop(pwm);
}
#endif // HAL_USE_PWM == TRUE



/* Emacs indentatation information
   Local Variables:
   indent-tabs-mode:nil
   tab-width:2
   c-basic-offset:2
   End:
*/
