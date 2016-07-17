/***
 * External interrupts functions
 *
 * Copyright 2013 Tuomas Kulve, <tuomas.kulve@snowcap.fi>
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
#include "sc_extint.h"
#include "sc_event.h"

#if HAL_USE_EXT

MUTEX_DECL(cfg_mtx);

static EXTConfig extcfg;
static virtual_timer_t vt;

typedef struct __sc_timer_t {
  uint8_t active;
  systime_t delay;
  ioportid_t port;
  expchannel_t channel;
  SC_EXTINT_EDGE mode;
} _sc_timer_t;
static _sc_timer_t debouncetimers[EXT_MAX_CHANNELS] = { {0, }, };

static void _extint_cb(EXTDriver *extp, expchannel_t channel)
{
  (void)extp;
  msg_t msg;

  chDbgAssert(channel < EXT_MAX_CHANNELS, "Channel number too large");

  msg = sc_event_msg_create_extint(channel);
  sc_event_msg_post(msg, SC_EVENT_MSG_POST_FROM_ISR);
}

void delay_handler(void *arg)
{
  msg_t msg;
  _sc_timer_t *t = (_sc_timer_t *)arg;

  chDbgAssert(t != NULL, "Delay triggered with NULL timer");

  /* If the interrupt is requested in rising/falling edge, only send event
   * if the channel settled in state that matches requested mode.
   */
  if (t->mode == SC_EXTINT_EDGE_RISING) {
    if (palReadPad(t->port, t->channel) != PAL_HIGH) {
      t->active = 0;
      return;
    }
  } else if (t->mode == SC_EXTINT_EDGE_FALLING) {
    if (palReadPad(t->port, t->channel) != PAL_LOW) {
      t->active = 0;
      return;
    }
  }

  msg = sc_event_msg_create_extint(t->channel);
  sc_event_msg_post(msg, SC_EVENT_MSG_POST_FROM_ISR);
  t->active = 0;
}

static void _extint_debounce_cb(EXTDriver *extp, expchannel_t channel)
{
  (void)extp;

  chDbgAssert(channel < EXT_MAX_CHANNELS, "Channel number too large");

  chSysLockFromISR();
  if (debouncetimers[channel].active == 0) {
    debouncetimers[channel].active = 1;
    debouncetimers[channel].channel = channel;
    chVTSetI(&vt, MS2ST(debouncetimers[channel].delay), delay_handler, &debouncetimers[channel]);
  }
  chSysUnlockFromISR();
}


uint32_t _get_ext_from_port(ioportid_t port)
{
#define GPIO2EXT(a)                             \
  if (port == a) return EXT_MODE_##a

  GPIO2EXT(GPIOA);
  GPIO2EXT(GPIOB);
  GPIO2EXT(GPIOC);
  GPIO2EXT(GPIOD);
  GPIO2EXT(GPIOE);
#if defined GPIOF && defined GPIOF_BASE
  GPIO2EXT(GPIOF);
#endif
#if defined GPIOG && defined GPIOG_BASE
  GPIO2EXT(GPIOG);
#endif
#if defined GPIOH && defined GPIOH_BASE
  GPIO2EXT(GPIOH);
#endif
#if defined GPIOI && defined GPIOI_BASE
  GPIO2EXT(GPIOI);
#endif
#undef GPIO2EXT

  chDbgAssert(0, "Invalid port");

  return 0;
}


void sc_extint_set_event(ioportid_t port, uint8_t pin, SC_EXTINT_EDGE mode)
{
  sc_extint_set_isr_cb(port, pin, mode, _extint_cb);
}

void sc_extint_set_debounced_event(ioportid_t port, uint8_t pin, SC_EXTINT_EDGE mode, systime_t delay)
{
  debouncetimers[pin].delay = delay;
  debouncetimers[pin].mode = mode;
  debouncetimers[pin].port = port;
  sc_extint_set_isr_cb(port, pin, mode, _extint_debounce_cb);
}


void sc_extint_set_isr_cb(ioportid_t port,
                          uint8_t pin,
                          SC_EXTINT_EDGE mode,
                          extcallback_t cb)

{
  uint8_t need_start = 1;
  uint8_t i;
  uint32_t ext_mode;
  EXTChannelConfig cfg;

  chDbgAssert(pin < EXT_MAX_CHANNELS , "EXT pin number outside range");
  chDbgAssert(mode == EXT_CH_MODE_RISING_EDGE ||
              mode == EXT_CH_MODE_FALLING_EDGE ||
              mode == EXT_CH_MODE_BOTH_EDGES, "Invalid edge mode");

  chMtxLock(&cfg_mtx);

  chDbgAssert(extcfg.channels[pin].cb == NULL,
              "EXT pin already registered");

  for (i = 0; i < EXT_MAX_CHANNELS; ++i) {
    if (extcfg.channels[pin].cb != NULL) {
      need_start = 0;
      break;
    }
  }

  ext_mode = (mode | EXT_CH_MODE_AUTOSTART | _get_ext_from_port(port));

  cfg.cb = cb;
  cfg.mode = ext_mode;

  if (need_start) {
    extStart(&EXTD1, &extcfg);
  }

  extSetChannelMode(&EXTD1, pin, &cfg);

  chMtxUnlock(&cfg_mtx);
}



void sc_extint_clear(ioportid_t port, uint8_t pin)
{
  uint8_t need_stop = 1;
  uint8_t i;
  EXTChannelConfig cfg;

  (void)port;

  chMtxLock(&cfg_mtx);

  chDbgAssert(pin < EXT_MAX_CHANNELS , "EXT pin number outside range");
  chDbgAssert(extcfg.channels[pin].cb != NULL,
              "EXT pin cb not registered");

  // FIXME: should check that port matches as well?

  cfg.cb = NULL;
  cfg.mode = EXT_CH_MODE_DISABLED;

  extSetChannelMode(&EXTD1, pin, &cfg);

  for (i = 0; i < EXT_MAX_CHANNELS; ++i) {
    if (extcfg.channels[pin].cb != NULL) {
      need_stop = 0;
      break;
    }
  }

  if (need_stop) {
    extStop(&EXTD1);
  }

  chMtxUnlock(&cfg_mtx);
}


#endif /* HAL_USE_EXT */

/* Emacs indentatation information
   Local Variables:
   indent-tabs-mode:nil
   tab-width:2
   c-basic-offset:2
   End:
*/
