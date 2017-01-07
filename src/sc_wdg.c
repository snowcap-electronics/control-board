/***
 * Watchdog
 *
 * Copyright 2017 Tuomas Kulve, <tuomas.kulve@snowcap.fi>
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
#include "sc_wdg.h"

#define SC_WDG_RELOAD                       0xAAAAU
#define SC_WDG_ENABLE                       0xCCCCU
#define SC_WDG_WRITE                        0x5555U

#define SC_REG_IWDG_KR                      (*((uint32_t *)(0x40003000 + 0x00)))
#define SC_REG_IWDG_PR                      (*((uint32_t *)(0x40003000 + 0x04)))
#define SC_REG_IWDG_RLR                     (*((uint32_t *)(0x40003000 + 0x08)))
#define SC_REG_IWDG_SR                      (*((uint32_t *)(0x40003000 + 0x0C)))

/*
 * Start the watchdog. It cannot be stopped or shutdown anymore
 */
void sc_wdg_init(uint32_t rlr)
{

  #if !defined(BOARD_ST_STM32VL_DISCOVERY)
  // Stop WDG on debug
  DBGMCU->APB1FZ = DBGMCU_APB1_FZ_DBG_IWDG_STOP;
  #endif

  SC_REG_IWDG_KR = SC_WDG_WRITE;

  while (SC_REG_IWDG_SR != 0)
    ;
  SC_REG_IWDG_PR = 0b111; // 256 divider
  SC_REG_IWDG_RLR = rlr;

  SC_REG_IWDG_KR = SC_WDG_RELOAD;
  SC_REG_IWDG_KR = SC_WDG_ENABLE;
}



/*
 * Reset the watchdog counter
 */
void sc_wdg_reset(void)
{
  SC_REG_IWDG_KR = SC_WDG_RELOAD;
}



/* Emacs indentatation information
   Local Variables:
   indent-tabs-mode:nil
   tab-width:4
   c-basic-offset:4
   End:
*/
