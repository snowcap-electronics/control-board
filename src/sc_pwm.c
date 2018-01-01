/***
 * PWM functions
 *
 * Copyright 2011-2017 Tuomas Kulve, <tuomas.kulve@snowcap.fi>
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

#if HAL_USE_PWM

// Use 1 MHz clock
#ifndef SC_PWM_CLOCK_PWMD1
#define SC_PWM_CLOCK_PWMD1    1000000
#endif

#ifndef SC_PWM_CLOCK_PWMD2
#define SC_PWM_CLOCK_PWMD2    1000000
#endif

#ifndef SC_PWM_CLOCK_PWMD3
#define SC_PWM_CLOCK_PWMD3    1000000
#endif


// Default to 50 Hz (20 ms)
#ifndef SC_PWM_DEFAULT_PERIOD_PWMD1
#define SC_PWM_DEFAULT_PERIOD_PWMD1  (SC_PWM_CLOCK_PWMD1 / 50)
#endif

#ifndef SC_PWM_DEFAULT_PERIOD_PWMD2
#define SC_PWM_DEFAULT_PERIOD_PWMD2  (SC_PWM_CLOCK_PWMD2 / 50)
#endif

#ifndef SC_PWM_DEFAULT_PERIOD_PWMD3
#define SC_PWM_DEFAULT_PERIOD_PWMD3  (SC_PWM_CLOCK_PWMD3 / 50)
#endif

static bool is_valid_pwm(uint8_t pwm);
static PWMDriver *get_pwmd(uint8_t pwm);
static PWMConfig *get_pwmc(uint8_t pwm);
static void parse_command_pwm_frequency(const uint8_t *param, uint8_t param_len);
static void parse_command_pwm_duty(const uint8_t *param, uint8_t param_len);
static void parse_command_pwm_stop(const uint8_t *param, uint8_t param_len);

static const struct sc_cmd cmds[] = {
  {"pwm_frequency", SC_CMD_HELP("Set PWM frequency (Hz) for given pwm (pwm freq))"), parse_command_pwm_frequency},
  {"pwm_duty",      SC_CMD_HELP("Set PWM (1-12) duty cycle (0-10000)"), parse_command_pwm_duty},
  {"pwm_stop",      SC_CMD_HELP("Stop PWM (1-12)"), parse_command_pwm_stop},
};


static PWMConfig pwmcfg1 = {
  SC_PWM_CLOCK_PWMD1,            // PWM clock frequency
  SC_PWM_DEFAULT_PERIOD_PWMD1,   // Default period
  NULL,                          // No periodic callback.
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
  SC_PWM_CLOCK_PWMD2,            // PWM clock frequency
  SC_PWM_DEFAULT_PERIOD_PWMD2,   // Default period
  NULL,                          // No periodic callback.
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
  SC_PWM_CLOCK_PWMD3,            // PWM clock frequency
  SC_PWM_DEFAULT_PERIOD_PWMD3,   // Default period
  NULL,                          // No periodic callback.
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

static bool is_valid_pwm(uint8_t pwm)
{

  if (pwm == 0) {
    return false;
  }

#ifdef PWMDX1
  if (pwm >= 1 && pwm <= 4) {
    return true;
  }
#endif

#ifdef PWMDX2
  if (pwm >= 5 && pwm <= 8) {
    return true;
  }
#endif

#ifdef PWMDX3
  if (pwm >= 9 && pwm <= 12) {
    return true;
  }
#endif

  return false;
}


static PWMDriver *get_pwmd(uint8_t pwm)
{
  if (pwm >= 1 && pwm <= 4) {
#ifdef PWMDX1
    return &PWMDX1;
#else
    chDbgAssert(0, "PWMDX1 not defined");
#endif
  }

  if (pwm >= 5 && pwm <= 8) {
#ifdef PWMDX2
    return &PWMDX2;
#else
    chDbgAssert(0, "PWMDX2 not defined");
#endif
  }

  if (pwm >= 9 && pwm <= 12) {
#ifdef PWMDX3
    return &PWMDX3;
#else
    chDbgAssert(0, "PWMDX3 not defined");
#endif
  }

  chDbgAssert(0, "Invalid pwm");

  return NULL;
}

static PWMConfig *get_pwmc(uint8_t pwm)
{
  if (pwm >= 1 && pwm <= 4) {
#ifdef PWMDX1
    return &pwmcfg1;
#else
    chDbgAssert(0, "PWMDX1 not defined");
#endif
  }

  if (pwm >= 5 && pwm <= 8) {
#ifdef PWMDX2
    return &pwmcfg2;
#else
    chDbgAssert(0, "PWMDX2 not defined");
#endif
  }

  if (pwm >= 9 && pwm <= 12) {
#ifdef PWMDX3
    return &pwmcfg3;
#else
    chDbgAssert(0, "PWMDX3 not defined");
#endif
  }

  chDbgAssert(0, "Invalid pwm");

  return NULL;
}

/*
 * Initialize PWM
 */
void sc_pwm_init(void)
{
  uint8_t i;

  for (i = 0; i < SC_ARRAY_SIZE(cmds); ++i) {
    sc_cmd_register(cmds[i].cmd, cmds[i].help, cmds[i].cmd_cb);
  }

#ifdef PWMDX1
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

  pwmStart(&PWMDX1, &pwmcfg1);
#endif

#ifdef PWMDX2
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

  pwmStart(&PWMDX2, &pwmcfg2);
#endif

#ifdef PWMDX3
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
 * Set frequency (i.e. PWM period) in Hz (e.g. 50 for standard
 * servo/ESC, 400 for fast servo/ESC)
 */

void sc_pwm_set_freq(uint8_t pwm, uint32_t freq)
{
  PWMDriver *pwmd = get_pwmd(pwm);
  if (!pwmd) {
    return;
  }

  PWMConfig *pwmc = get_pwmc(pwm);
  if (!pwmc) {
    return;
  }

  if (freq == 0) {
    // freq 0 is invalid, do nothing
    return;
  }

  pwmChangePeriod(pwmd, pwmc->frequency / freq);
}



/*
 * Stop PWM signal
 */
void sc_pwm_stop(uint8_t pwm)
{
  PWMDriver *pwmd = get_pwmd(pwm);
  if (!pwmd) {
    return;
  }

  uint8_t channel_pwm = (pwm - 1) % 4;
  pwmDisableChannel(pwmd, channel_pwm);
}



/*
 * Set duty cycle. 0 for 0%, 10000 for 100%
 */
void sc_pwm_set_duty(uint8_t pwm, uint16_t duty)
{
  PWMDriver *pwmd = get_pwmd(pwm);
  if (!pwmd) {
    return;
  }

  if (duty > 10000) {
    duty = 10000;
  }

  uint8_t channel_pwm = (pwm - 1) % 4;
  pwmEnableChannel(pwmd, channel_pwm, PWM_PERCENTAGE_TO_WIDTH(pwmd, duty));
}



/*
 * Parse PWM frequency command
 */
static void parse_command_pwm_frequency(const uint8_t *param, uint8_t param_len)
{
  uint8_t pwm;
  uint16_t freq;

  if (!param || param[0] < '0' || param[0] > '9') {
    // Invalid message
    return;
  }

  // Parse two parameters, pwm and freq
  uint8_t value_offset = 2;
  pwm  = sc_atoi(param, param_len);
  if (pwm > 9)  value_offset++;
  if (pwm > 99) value_offset++;
  freq = sc_atoi(&param[value_offset], param_len - value_offset);

  if (!is_valid_pwm(pwm)) {
    return;
  }

  sc_pwm_set_freq(pwm, freq);
}



/*
 * Parse PWM duty cycle command
 */
static void parse_command_pwm_duty(const uint8_t *param, uint8_t param_len)
{
  uint8_t pwm;
  uint16_t value;

  if (!param || param[0] < '0' || param[0] > '9') {
    // Invalid message
    return;
  }

  // Parse two parameters, pwm and value
  uint8_t value_offset = 2;
  pwm  = sc_atoi(param, param_len);
  if (pwm > 9)  value_offset++;
  if (pwm > 99) value_offset++;
  value = sc_atoi(&param[value_offset], param_len - value_offset);

  if (!is_valid_pwm(pwm)) {
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

  if (!param || param[0] < '0' || param[0] > '9') {
    // Invalid message
    return;
  }

  pwm = sc_atoi(param, param_len);

  if (!is_valid_pwm(pwm)) {
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
