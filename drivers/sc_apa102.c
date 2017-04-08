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
static enum sc_apa102_effect effect = SC_APA102_EFFECT_NONE;
static bool run_update;

static thread_t *sc_apa102_spi_thread_ptr;

static THD_WORKING_AREA(sc_apa102_spi_thread, 256);
THD_FUNCTION(scApa102SpiThread, arg)
{
  (void)arg;
  chRegSetThreadName(__func__);
  uint8_t led = 0;
  bool inc = true;

  sc_spi_select(spin);
  // Loop waiting for action
  while (!chThdShouldTerminateX()) {

    chThdSleepMilliseconds(100);
    if (chThdShouldTerminateX()) {
      break;
    }

    switch(effect) {
    case SC_APA102_EFFECT_NONE:
      // Nothing to update;
      break;
      // Light all leds one at a time
    case SC_APA102_EFFECT_CIRCLE:
      sc_apa102_set(led, 0, 0, 0, 0);
      if (++led >= SC_APA102_MAX_LEDS) {
        led = 0;
      }
      sc_apa102_set(led, 31, 255, 0, 0);
      break;
    case SC_APA102_EFFECT_KITT:
      if (led < SC_APA102_MAX_LEDS / 3) {
        led = SC_APA102_MAX_LEDS / 3;
      }
      for (uint8_t i = SC_APA102_MAX_LEDS / 3; i < 2 * (SC_APA102_MAX_LEDS / 3); ++i) {
        uint16_t red = 4 + i * 4 + 3;
        // Dim a led by 1/6th
        if (data[red] > 255/6) {
          data[red] -= 255/6;
        } else {
          data[red] = 0;
        }
      }
      if (inc) {
        ++led;
        if (led == 2 * (SC_APA102_MAX_LEDS / 3)) {
          inc = false;
          led--;
        }
      } else {
        --led;
        if (led == SC_APA102_MAX_LEDS / 3 - 1) {
          inc = true;
          led++;
        }
      }
      sc_apa102_set(led, 31, 255, 0, 0);
      break;
    case SC_APA102_EFFECT_KITT_FULL:
      for (uint8_t i = 0; i < SC_APA102_MAX_LEDS - 1; ++i) {
        uint16_t blue = 4 + i * 4 + 1;
        if (data[blue] > 255/8) {
          data[blue] -= 255/8;
        } else {
          data[blue] = 0;
        }
      }
      if (inc) {
        ++led;
        if (led == SC_APA102_MAX_LEDS - 1) {
          inc = false;
          led--;
        }
      } else {
        --led;
        if (led == 0) {
          inc = true;
          led++;
        }
      }
      sc_apa102_set(led, 31, 0, 0, 255);
      break;
    default:
      // Do nothing
      break;
    }

    // Update the led strip always, just in case the data has change
    // Can't select and select the SPI as there's no slave select and
    // the APA assumes the clock is low on idle.
    //sc_spi_select(spin);
    sc_spi_send(spin, data, sizeof(data));
    //sc_spi_deselect(spin);
  }
}


void sc_apa102_init(void)
{
  int8_t spi_n;

  run_update = false;
  effect = SC_APA102_EFFECT_NONE;

  // Initialise start frame bits to 0
  for (uint16_t i = 0; i < 4; ++i) {
    data[i] = 0;
  }

  // Number of end frame bytes
  // FIXME: make a define, use also for the size of the array
  uint8_t end_frame_len = (uint8_t)(((SC_APA102_MAX_LEDS - 1) / 16.0) + 1);

  // Initialise end frame bits to 0
  for (uint16_t i = 0; i < end_frame_len; ++i) {
    data[(sizeof(data) - 1) - i] = 0x0;
  }

  // Initialise leds to off
  for (uint8_t i = 0; i < SC_APA102_MAX_LEDS; ++i) {
    sc_apa102_set(i, 0, 0, 0, 0);
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
                          SPI_CR1_BR_0); // APA isn't picky about the speed

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
  // Clear all LEDs when changing the effect
  effect = SC_APA102_EFFECT_NONE;
  for (uint8_t led = 1; led < SC_APA102_MAX_LEDS; ++led) {
    sc_apa102_set(led, 0, 0, 0, 0);
  }
  effect = ef;
}



void sc_apa102_set(uint16_t led, uint8_t brightness, uint8_t r, uint8_t g, uint8_t b)
{
  uint32_t li = 4 + led * 4;

  if (led >= SC_APA102_MAX_LEDS) {
    SC_LOG_ASSERT(0, "led index too large");
    return;
  }

  // No data locking to avoid blocking the call for too long
  // May cause wrong colors briefly
  data[li + 0] = brightness | 0b11100000;
  data[li + 1] = b;
  data[li + 2] = g;
  data[li + 3] = r;
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
