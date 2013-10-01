/***
 * Run mode / power consumption functions
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
#include "sc_pwr.h"


/*
 * Put the MCU to StandBy mode (all off except RTC)
 */
#ifdef BOARD_ST_STM32VL_DISCOVERY
void sc_pwr_standby(void)
{
  chDbgAssert(0, "sc_pwr_standby() not implemented for F1 Discovery", "#1");
}
#else
void sc_pwr_standby(void)
{
  chSysLock();

  SCB->SCR |= SCB_SCR_SLEEPDEEP_Msk;
  PWR->CR |= (PWR_CR_PDDS | PWR_CR_CSBF | PWR_CR_CWUF);
  RTC->ISR &= ~(RTC_ISR_ALRBF | RTC_ISR_ALRAF | RTC_ISR_WUTF | RTC_ISR_TAMP1F | RTC_ISR_TSOVF | RTC_ISR_TSF);

  __WFI();
}
#endif



/*
 * Set periodic wakeup.
 * Note that this seems to be cleared on reset (i.e. also in wake up from
 * stand by).
 */
#ifdef BOARD_ST_STM32VL_DISCOVERY
void sc_pwr_wakeup_set(UNUSED(uint32_t sec), UNUSED(uint32_t ms))
{
  chDbgAssert(0, "sc_pwr_wakeup_set() not implemented for F1 Discovery", "#1");
}
#else
void sc_pwr_wakeup_set(uint32_t sec, UNUSED(uint32_t ms))
{
  RTCWakeup wakeupspec;

  /* FIXME: faster clock source for millisecond sleeps? */
  wakeupspec.wakeup = ((uint32_t)4) << 16; /* select 1 Hz clock source */

  /* FIXME: No idea why this doesn't match to 1Hz */
  wakeupspec.wakeup |= 30 * sec;

  rtcSetPeriodicWakeup_v2(&RTCD1, &wakeupspec);
}
#endif



/*
 * Clear periodic wakeup
 */
#ifdef BOARD_ST_STM32VL_DISCOVERY
void sc_pwr_wakeup_clear(void)
{
  chDbgAssert(0, "sc_pwr_wakeup_clear() not implemented for F1 Discovery", "#1");
}
#else
void sc_pwr_wakeup_clear(void)
{
  rtcSetPeriodicWakeup_v2(&RTCD1, NULL);
}
#endif
