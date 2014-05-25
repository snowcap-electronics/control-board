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
#include "testplatform.h"

#include <strings.h> // bzero

#ifdef TP_PB_PORT
#define DEBUG_TOGGLE(a,b) tp_sync(a,b)
#else
#define DEBUG_TOGGLE(a,b)
#endif

#ifndef EXTI_EMR_MR22
#define EXTI_EMR_MR22                       ((uint32_t)0x00400000)
#endif

#ifndef EXTI_RTSR_TR22
#define EXTI_RTSR_TR22                      ((uint32_t)0x00400000)
#endif

#ifndef EXTI_PR_PR22
#define EXTI_PR_PR22                        ((uint32_t)0x00400000)
#endif

volatile uint32_t *DBGMCU_CR = (uint32_t *)0xe0042004;

/*
 * Enable clocks to use a debugger (GDB) with WFI.
 */
void sc_pwr_set_wfi_dbg(void)
{
    uint32_t cr;
    cr = *DBGMCU_CR;
    cr |= 0x7; /* DBG_STANDBY | DBG_STOP | DBG_SLEEP */
    *DBGMCU_CR = cr;
}



/*
 * Get WFI debug bits
 */
uint32_t sc_pwr_get_wfi_dbg(void)
{
    return *DBGMCU_CR;
}

#if HAL_USE_RTC

#define SC_PWR_USE_WFI
#ifdef SC_PWR_USE_WFI
static void _empty_cb(EXTDriver *extp, expchannel_t channel)
{
    (void)extp;
    (void)channel;
}

void sc_pwr_rtc_sleep(int timeout_sec)
{
    EXTChannelConfig cfg;
    EXTConfig extcfg;

    DEBUG_TOGGLE(1, true);

    // Allow RTC write
    RTC->WPR = 0xCA;
    RTC->WPR = 0x53;

    while (!(RTC->ISR & RTC_ISR_WUTWF)){
        RTC->CR &= ~RTC_CR_WUTE;
    }

    DEBUG_TOGGLE(2, true);

    RTC->CR &= (~7); // Clear WUCKSEL
    RTC->CR |= 4;    // Wakeup clock = ck_spre (1Hz)
    RTC->WUTR = timeout_sec;

    DEBUG_TOGGLE(3, true);
    
    // Start the wakeup timer
    PWR->CR |= PWR_CR_CWUF;
    RTC->ISR &= ~RTC_ISR_WUTF;
    RTC->CR |= RTC_CR_WUTE | RTC_CR_WUTIE;

    DEBUG_TOGGLE(4, true);

    bzero(&cfg, sizeof(cfg));
    bzero(&extcfg, sizeof(extcfg));

    cfg.mode = EXT_CH_MODE_RISING_EDGE | EXT_CH_MODE_AUTOSTART;
    cfg.cb = _empty_cb;
    
    extStart(&EXTD1, &extcfg);
    extSetChannelMode(&EXTD1, 22, &cfg);

    DEBUG_TOGGLE(5, true);

    SCB->SCR |= SCB_SCR_SLEEPDEEP_Msk;
    PWR->CR |= (PWR_CR_LPDS/* | PWR_CR_FPDS*/ | PWR_CR_CSBF | PWR_CR_CWUF);
    PWR->CR &= ~PWR_CR_PDDS;

    __WFI();

    SCB->SCR &= ~SCB_SCR_SLEEPDEEP_Msk;

    stm32_clock_init();

    DEBUG_TOGGLE(6, true);

    extChannelDisable(&EXTD1, 22);
    extStop(&EXTD1);

    RTC->CR &= ~RTC_CR_WUTE;
    RTC->ISR &= ~RTC_ISR_WUTF;
    RTC->WPR = 0; // Relock registers
}
#else
void sc_pwr_rtc_sleep(int timeout_sec)
{
    chSysLock();

    // Allow RTC write
    RTC->WPR = 0xCA;
    RTC->WPR = 0x53;

    DEBUG_TOGGLE(1, true);

    while (!(RTC->ISR & RTC_ISR_WUTWF))
        RTC->CR &= ~RTC_CR_WUTE;

    DEBUG_TOGGLE(2, true);

    RTC->CR &= (~7); // Clear WUCKSEL
    RTC->CR |= 4;    // Wakeup clock = ck_spre (1Hz)
    RTC->WUTR = timeout_sec;

    // Start the wakeup timer
    PWR->CR |= PWR_CR_CWUF;
    RTC->ISR &= ~RTC_ISR_WUTF;
    RTC->CR |= RTC_CR_WUTE | RTC_CR_WUTIE;

    DEBUG_TOGGLE(3, true);

    // Enable EXTI0 and EXTI22 rising edge events
    EXTI->EMR = EXTI->IMR = EXTI->RTSR = 0;
    EXTI->PR = 0x7FFFFF;

    DEBUG_TOGGLE(4, true);

    // Event mask register
    EXTI->EMR |= EXTI_EMR_MR22;
    // Rising trigger selection register
    EXTI->RTSR |= EXTI_RTSR_TR22;
    // Pending register
    EXTI->PR |= EXTI_PR_PR22;

    DEBUG_TOGGLE(5, true);

    // Go to sleep
    PWR->CR |= PWR_CR_LPDS;
    PWR->CR &= ~PWR_CR_PDDS;
    SCB->SCR |= SCB_SCR_SLEEPDEEP_Msk;

    asm("dsb");
    asm("wfe");

    SCB->SCR &= ~SCB_SCR_SLEEPDEEP_Msk;

    DEBUG_TOGGLE(6, true);

    // Switch back to HSI oscillator
    stm32_clock_init();

    DEBUG_TOGGLE(7, true);

    // Re-enable access to backup domain
    PWR->CR |= PWR_CR_DBP;
    
    RTC->CR &= ~RTC_CR_WUTE;
    RTC->ISR &= ~RTC_ISR_WUTF;
    RTC->WPR = 0; // Relock registers

    chSysUnlock();

    DEBUG_TOGGLE(8, true);
}
#endif // !SC_PWR_USE_WFI

#endif // HAL_USE_RTC

/* Emacs indentatation information
   Local Variables:
   indent-tabs-mode:nil
   tab-width:4
   c-basic-offset:4
   End:
*/
