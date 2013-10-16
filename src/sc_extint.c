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

static EXTConfig extcfg = {
  {
    {EXT_CH_MODE_DISABLED, NULL},
    {EXT_CH_MODE_DISABLED, NULL},
    {EXT_CH_MODE_DISABLED, NULL},
    {EXT_CH_MODE_DISABLED, NULL},
    {EXT_CH_MODE_DISABLED, NULL},
    {EXT_CH_MODE_DISABLED, NULL},
    {EXT_CH_MODE_DISABLED, NULL},
    {EXT_CH_MODE_DISABLED, NULL},
    {EXT_CH_MODE_DISABLED, NULL},
    {EXT_CH_MODE_DISABLED, NULL},
    {EXT_CH_MODE_DISABLED, NULL},
    {EXT_CH_MODE_DISABLED, NULL},
    {EXT_CH_MODE_DISABLED, NULL},
    {EXT_CH_MODE_DISABLED, NULL},
    {EXT_CH_MODE_DISABLED, NULL},
    {EXT_CH_MODE_DISABLED, NULL},
    {EXT_CH_MODE_DISABLED, NULL},
    {EXT_CH_MODE_DISABLED, NULL},
    {EXT_CH_MODE_DISABLED, NULL},
    {EXT_CH_MODE_DISABLED, NULL},
  }
};

static void _extint_cb(EXTDriver *extp, expchannel_t channel)
{
  (void)extp;
  msg_t msg;

  chDbgAssert(channel < EXT_MAX_CHANNELS, "Channel number too large", "#1");

  msg = sc_event_msg_create_extint(channel);
  sc_event_msg_post(msg, SC_EVENT_MSG_POST_FROM_ISR);
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
  GPIO2EXT(GPIOF);
  GPIO2EXT(GPIOG);
  GPIO2EXT(GPIOH);
  GPIO2EXT(GPIOI);
#undef GPIO2EXT

  chDbgAssert(0, "Invalid port", "#1");

  return 0;
}


void sc_extint_set_event(ioportid_t port, uint8_t pin, SC_EXTINT_EDGE mode)
{
  sc_extint_set_isr_cb(port, pin, mode, _extint_cb);
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

  chMtxLock(&cfg_mtx);

  chDbgAssert(pin < EXT_MAX_CHANNELS , "EXT pin number outside range", "#2");
  chDbgAssert(extcfg.channels[pin].cb == NULL,
              "EXT pin already registered", "#2");
  chDbgAssert(mode == EXT_CH_MODE_RISING_EDGE ||
              mode == EXT_CH_MODE_FALLING_EDGE ||
              mode == EXT_CH_MODE_BOTH_EDGES, "Invalid edge mode", "#1");

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

  chMtxUnlock();
}



void sc_extint_clear(ioportid_t port, uint8_t pin)
{
  uint8_t need_stop = 1;
  uint8_t i;
  EXTChannelConfig cfg;

  (void)port;

  chMtxLock(&cfg_mtx);

  chDbgAssert(pin < EXT_MAX_CHANNELS , "EXT pin number outside range", "#3");
  chDbgAssert(extcfg.channels[pin].cb != NULL,
              "EXT pin cb not registered", "#3");

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

  chMtxUnlock();
}


#endif /* HAL_USE_EXT */

/* Emacs indentatation information
   Local Variables:
   indent-tabs-mode:nil
   tab-width:2
   c-basic-offset:2
   End:
*/