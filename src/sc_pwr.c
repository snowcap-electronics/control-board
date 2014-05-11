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
#include "sc_extint.h"


/*
 * Put the MCU to StandBy mode (all off except RTC)
 */
void sc_pwr_mode_standby(bool wake_on_pa0, bool wake_on_rtc)
{
    SCB->SCR |= SCB_SCR_SLEEPDEEP_Msk;
    PWR->CR |= (PWR_CR_PDDS | PWR_CR_CSBF | PWR_CR_CWUF);
    PWR->CSR |= PWR_CSR_EWUP;

    if (wake_on_pa0) {
        PWR->CSR |= PWR_CSR_EWUP;
    }

    if (wake_on_rtc) {
#if HAL_USE_RTC
        RTC->ISR &= ~(RTC_ISR_ALRBF | RTC_ISR_ALRAF | RTC_ISR_WUTF | RTC_ISR_TAMP1F | RTC_ISR_TSOVF | RTC_ISR_TSF);
#else
        chDbgAssert(0, "RTC not enabled", "#1");
#endif
    }
    __WFI();
}



void sc_pwr_mode_stop(bool wake_on_rtc)
{
    SCB->SCR |= SCB_SCR_SLEEPDEEP_Msk;
    PWR->CR |= (PWR_CR_LPDS/* | PWR_CR_FPDS*/ | PWR_CR_CSBF | PWR_CR_CWUF);
    PWR->CR &= ~PWR_CR_PDDS;

    if (wake_on_rtc) {
#if HAL_USE_RTC
        // RTC wake from stop mode needs EXTI line 22 configured to rising edge
        sc_extint_set_event(GPIOA, 22, SC_EXTINT_EDGE_RISING);

        RTC->ISR &= ~(RTC_ISR_ALRBF | RTC_ISR_ALRAF | RTC_ISR_WUTF | RTC_ISR_TAMP1F | RTC_ISR_TSOVF | RTC_ISR_TSF);
#else
        chDbgAssert(0, "RTC not enabled", "#1");
#endif
    }

    __WFI();
}


void sc_pwr_mode_clear(void)
{
    SCB->SCR &= ~SCB_SCR_SLEEPDEEP_Msk;
    PWR->CSR &= ~PWR_CSR_EWUP;
    PWR->CR &= ~(PWR_CR_PDDS | PWR_CR_LPDS/* | PWR_CR_FPDS*/);
#if HAL_USE_RTC
    sc_extint_clear(GPIOA, 22);
#endif
}


void sc_pwr_chibios_stop(void)
{
    // http://www.chibios.org/dokuwiki/doku.php?id=chibios:howtos:stop_os
    // FIXME: What's a system timer? How to stop it?

    chSysEnable();
}



void sc_pwr_chibios_start(void)
{
    stm32_clock_init();
    chSysDisable();
}



#if HAL_USE_RTC
/*
 * Set periodic wakeup.
 * Note that this seems to be cleared on reset (i.e. also in wake up from
 * standby).
 */
void sc_pwr_wakeup_set(uint32_t sec, uint32_t ms)
{
    RTCWakeup wakeupspec;
    (void)ms;
    /* select 1 Hz clock source */
    wakeupspec.wakeup = ((uint32_t)4) << 16;
    wakeupspec.wakeup |= sec;

    // FIXME: should not use hard coded RTCD1?
    rtcSetPeriodicWakeup_v2(&RTCD1, &wakeupspec);
}



/*
 * Clear periodic wakeup
 */
void sc_pwr_wakeup_clear(void)
{
    // FIXME: should not use hard coded RTCD1?
    rtcSetPeriodicWakeup_v2(&RTCD1, NULL);
}
#endif


/* Emacs indentatation information
   Local Variables:
   indent-tabs-mode:nil
   tab-width:4
   c-basic-offset:4
   End:
*/
