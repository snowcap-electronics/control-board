/***
 * ST SPIRIT1 Radio driver for the Snowcap project. Part of the implementation
 * copied from ST's spirit1_appli.[ch].
 *
 * COPYRIGHT(c) 2014 STMicroelectronics
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *   1. Redistributions of source code must retain the above copyright notice,
 *      this list of conditions and the following disclaimer.
 *   2. Redistributions in binary form must reproduce the above copyright notice,
 *      this list of conditions and the following disclaimer in the documentation
 *      and/or other materials provided with the distribution.
 *   3. Neither the name of STMicroelectronics nor the names of its contributors
 *      may be used to endorse or promote products derived from this software
 *      without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * Copyright 2015 Tuomas Kulve, <tuomas.kulve@snowcap.fi>
 */

#ifndef SC_SPIRIT1_H
#define SC_SPIRIT1_H

#include "sc_utils.h"
#include "sc_led.h"
#include "SPIRIT_Types.h"
#include "SPIRIT_Radio.h"

#define SPIRIT1_BROADCAST_ADDRESS           0xFF
#define SPIRIT1_MULTICAST_ADDRESS           0xEE

/*
 * Public Snowcap API
 */
void sc_spirit1_init(uint8_t *enc_key, uint8_t my_addr);
uint8_t sc_spirit1_read(uint8_t *addr, uint8_t *buf, uint8_t len);
uint8_t sc_spirit1_lqi(void);
uint8_t sc_spirit1_rssi(void);
void sc_spirit1_send(uint8_t addr, uint8_t *buf, uint8_t len);
void sc_spirit1_shutdown(void);
void sc_spirit1_poweroff(void);
void sc_spirit1_poweron(void);

/* Snowcap specific SPI implementations for ST's driver */
SpiritStatus sc_spirit1_reg_read(uint8_t reg, uint8_t len, uint8_t* buf);
SpiritStatus sc_spirit1_reg_write(uint8_t reg, uint8_t len, uint8_t* buf);
SpiritStatus sc_spirit1_cmd_strobe(uint8_t cmd);
SpiritStatus sc_spirit1_fifo_read(uint8_t len, uint8_t* buf);
SpiritStatus sc_spirit1_fifo_write(uint8_t len, uint8_t* buf);

/* Replace some of the ST's HAL functions with the corresponding
   functions in ChibiOS or snowcap */
#define HAL_Delay(ms)                            chThdSleepMilliseconds(ms)
#define RadioShieldLedOn(led)                    sc_led_on()
#define RadioShieldLedToggle(led)                sc_led_toggle()
#define RadioShieldLedOff(led)                   sc_led_off()
#define RadioEnterShutdown()                     sc_spirit1_poweroff()
#define RadioSpiWriteRegisters(reg, len, buf)    sc_spirit1_reg_write(reg, len, buf)
#define RadioSpiReadRegisters(reg, len, buf)     sc_spirit1_reg_read(reg, len, buf)
#define RadioSpiCommandStrobes(cmd)              sc_spirit1_cmd_strobe(cmd)
#define RadioSpiReadFifo(len, buf)               sc_spirit1_fifo_read(len, buf)
#define RadioSpiWriteFifo(len, buf)              sc_spirit1_fifo_write(len, buf)

#endif

/* Emacs indentatation information
   Local Variables:
   indent-tabs-mode:nil
   tab-width:2
   c-basic-offset:2
   End:
*/
