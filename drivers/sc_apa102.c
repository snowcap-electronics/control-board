/***
 * APA 102 LED strip driver
 * https://www.pololu.com/product/2552
 *
 * Copyright 2016 Tuomas Kulve, <tuomas.kulve@snowcap.fi>
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

#define SC_LOG_MODULE_TAG SC_LOG_MODULE_UNSPECIFIED

#include "sc_utils.h"
#include "sc.h"
#include "sc_spi.h"
#include "sc_apa102.h"

#ifdef SC_HAS_APA102

#ifndef SC_APA102_MAX_LEDS
#define SC_APA102_MAX_LEDS 30
#endif

static uint8_t spin;
// 4 byte header, 4 bytes per led, 4 byte end frame
static uint8_t data[4 + SC_APA102_MAX_LEDS * 4 + 4];
static enum sc_apa102_effect effect;

static thread_t *sc_apa102_spi_thread_ptr;

static THD_WORKING_AREA(sc_apa102_spi_thread, 256);
THD_FUNCTION(scApa102SpiThread, arg)
{
  (void)arg;
  chRegSetThreadName(__func__);
  uint8_t led = 0;
  uint8_t round = 0;

  // Loop waiting for action
  while (!chThdShouldTerminateX()) {

    // Wait up to 100ms. After a timeout, update effect leds if ongoing
    chThdSleepMilliseconds(100);
    if (effect == SC_APA102_EFFECT_NONE) {
      continue;
    }

    switch(effect) {
      // Light all leds one at a time
    case SC_APA102_EFFECT_CIRCLE:
      if (led - 3 >= 0) {
        sc_apa102_set(led - 3, 0, 0, 0, 0, false);
      }
      if (led - 2 >= 0) {
        sc_apa102_set(led - 2, 31,
                      round == 0 ? 255 : 0,
                      round == 1 ? 255 : 0,
                      round == 2 ? 255 : 0, false);
      }
      if (led - 1 >= 0) {
        sc_apa102_set(led - 1, 31,
                      round == 1 ? 255 : 0,
                      round == 2 ? 255 : 0,
                      round == 0 ? 255 : 0, false);
      }
      if (led - 0 >= 0) {
        sc_apa102_set(led - 0, 31,
                      round == 2 ? 255 : 0,
                      round == 0 ? 255 : 0,
                      round == 1 ? 255 : 0, true);
      }

      if (++round > 2) {
        round = 0;
      }
      
      if (++led >= SC_APA102_MAX_LEDS) {
        led = 0;
      }
      break;
    default:
      // Do nothing
      break;
    }
  }
}


void sc_apa102_init(void)
{
  int8_t spi_n;

  // Initialise end frame bits to 1
  for (uint16_t i = 0; i < 4; ++i) {
    data[(sizeof(data) - 1) - i] = 0xff;
  }
    
  // Pin mux
  // FIXME: Should these be in the SPI module?
  palSetPadMode(SC_APA102_SPI_SCK_PORT,
                SC_APA102_SPI_SCK_PIN,
                PAL_MODE_ALTERNATE(SC_APA102_SPI_AF) |
                PAL_STM32_OSPEED_HIGHEST);
  palSetPadMode(SC_APA102_SPI_PORT,
                SC_APA102_SPI_MISO_PIN,
                PAL_MODE_ALTERNATE(SC_APA102_SPI_AF) |
                PAL_STM32_OSPEED_HIGHEST);
  palSetPadMode(SC_APA102_SPI_PORT,
                SC_APA102_SPI_MOSI_PIN,
                PAL_MODE_ALTERNATE(SC_APA102_SPI_AF) |
                PAL_STM32_OSPEED_HIGHEST);
  palSetPadMode(SC_APA102_SPI_CS_PORT,
                SC_APA102_SPI_CS_PIN,
                PAL_MODE_OUTPUT_PUSHPULL |
                PAL_STM32_OSPEED_HIGHEST);

  palSetPad(SC_APA102_SPI_CS_PORT, SC_APA102_SPI_CS_PIN);

  // CPHA=0, CPOL=0, 8bits frames, MSb transmitted first.
  spi_n = sc_spi_register(&SC_APA102_SPIN,
                          SC_APA102_SPI_CS_PORT,
                          SC_APA102_SPI_CS_PIN,
                          // FIXME: Driver shouldn't need to know about the clocks
#if defined(SC_MCU_LOW_SPEED) && SC_MCU_LOW_SPEED
                          SPI_CR1_BR_0); // div4, good for 25 Mhz AHB1.
#else
                          SPI_CR1_BR_0 | SPI_CR1_BR_1);
#endif

  spin = (uint8_t)spi_n;

  sc_apa102_spi_thread_ptr =
    chThdCreateStatic(sc_apa102_spi_thread,
                      sizeof(sc_apa102_spi_thread),
                      HIGHPRIO,
                      scApa102SpiThread,
                      NULL);

}



void sc_apa102_effect(enum sc_apa102_effect ef)
{
  effect = ef;
  if (effect == SC_APA102_EFFECT_NONE) {
    for (uint8_t led = 1; led < SC_APA102_MAX_LEDS; ++led) {
      sc_apa102_set(led, 0, 0, 0, 0, false);
    }
    sc_apa102_set(0, 0, 0, 0, 0, true);
  }
}



void sc_apa102_set(uint16_t led, uint8_t brightness, uint8_t r, uint8_t g, uint8_t b, bool set)
{
  uint32_t li = 4 + led * 4;

  if (led >= SC_APA102_MAX_LEDS) {
    SC_LOG_ASSERT(0, "led index too large");
    return;
  }

  data[li + 0] = brightness & 0xe0; 
  data[li + 1] = r;
  data[li + 2] = g;
  data[li + 3] = b;

  if (set) {
    sc_spi_select(spin);
    sc_spi_send(spin, data, sizeof(data));
    sc_spi_deselect(spin);
  }
}



void sc_apa102_shutdown(void)
{
  uint8_t i = 20;

  sc_apa102_effect(SC_APA102_EFFECT_NONE);

  // Wait up to 200 ms for the thread to exit
  while (i-- > 0 &&
         !chThdTerminatedX(sc_apa102_spi_thread_ptr)) {
    chThdSleepMilliseconds(10);
  }

}


#endif

/* Emacs indentatation information
   Local Variables:
   indent-tabs-mode:nil
   tab-width:2
   c-basic-offset:2
   End:
*/
