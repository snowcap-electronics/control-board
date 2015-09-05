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

#ifndef SC_PWR_H
#define SC_PWR_H

void sc_pwr_set_wfi_dbg(void);
uint32_t sc_pwr_get_wfi_dbg(void);

#if HAL_USE_RTC
void sc_pwr_rtc_standby(int timeout_sec);
bool sc_pwr_get_wake_up_flag(void);
bool sc_pwr_get_standby_flag(void);
void sc_pwr_enable_backup_sram(void);
uint32_t *sc_pwr_get_backup_sram(void);


#endif

#endif
