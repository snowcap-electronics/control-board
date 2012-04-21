/***
 * TI TMP275 temperature sensor support
 *
 * Copyright 2012 Kalle Vahlman, <kalle.vahlman@snowcap.fi>
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

#include "sc_conf.h"

#ifdef SC_USE_TMP275

#include "sc_utils.h"
#include "sc_uart.h"
#include "sc_i2c.h"
#include "sc_led.h"

#include "drivers/tmp275.h"

/* ASCII output format is 't:SNNN.MM\r\n' where
 *  - 't:' is fixed
 *  - S is sign (always present)
 *  - NNN is integer part (variable lenght)
 *  - MM is fractional part (fixed lenght)
 *  - output is ended with CR+LF
 */
#define MSG_LEN 16
static uint8_t msg[MSG_LEN] = "t:";
static SC_UART uart = SC_UART_LAST;
static int running = 0;
static i2caddr_t addr = 0;

static WORKING_AREA(tmp275_thread, 192);
static msg_t tmp275Thread(void *UNUSED(arg))
{
  while (running) {
    int16_t temp;
    int16_t temp_int;
    int temp_frac;
    int len = 2;
    int sign = 1;

    temp = tmp275_read(addr);

    /* Grab the sign and convert to positive
     * Negative integer math is... interesting so we take the risk of
     * asymmetry and convert to positive. This might incur heavy error
     * for some values, but is verified to work correctly with a fixed
     * dataset in the range of 127.93...-55.00 (from datasheets).
     */
    if (temp < 0) {
      sign = -1;
      temp *= -1;
    }

    /* We always mark the sign to make parsing easier */
    msg[len++] = sign == 1 ? '+' : '-';

    /* Integer part */
    temp_int = temp >> 8;

    len += sc_itoa(temp_int, msg+len, MSG_LEN-len);

    /* Decimal part is in the low 8 bits, but only top 4 bits are used.
     * One step is roughly 0.06253 degrees Celsius, but no floats here!
     * So we multiply by 6253 and divide by 1000 to catch the first two
     * decimal places
     */
    temp_frac = (((temp & 0xff) >> 4) * 6253) / 1000;
    msg[len++] = '.';

    /* Add leading zero, if needed */
    if (temp_frac < 10)
        msg[len++] = '0';

    len += sc_itoa(temp_frac, msg+len, MSG_LEN-len);
    msg[len++] = '\r';
    msg[len++] = '\n';
    sc_uart_send_msg(uart, msg, len);

    /* Reading the value is possible at 300ms intervals when in 12 bit mode,
     * but that probably never makes sense. 1s should be ok as a default.
     * FIXME: This should be configurable... */
    chThdSleepMilliseconds(1000);
  }
  return 0;
}

static uint8_t tx_data[2];  /* Transmit buffer */
static uint8_t rx_data[2];  /* Receive buffer */

void tmp275_init(i2caddr_t paddr)
{
  /* No-op if we are already initialized */
  if (addr)
    return;

  addr = paddr;
  /* Pointer to Configuration register */
  tx_data[0] = 0x1;
  /* Configuration:
       Shutdown mode: 0
       Thermostat mode: 0
       Polarity: 0
       Fault queue: 00
       Resolution: 11 (12 bits)
       One-shot: 0
   */
  tx_data[1] = 0b01100000;
  sc_i2c_write(addr, tx_data, 2);

  /* Set pointer to temperature register here so we can read without writing */
  tx_data[0] = 0x0;
  sc_i2c_write(addr, tx_data, 1);
}

void tmp275_enable(i2caddr_t paddr)
{
  /* No-op if running */
  if (running == 1)
    return;

  running = 1;
  tmp275_init(paddr);
  chThdCreateStatic(tmp275_thread, sizeof(tmp275_thread), NORMALPRIO, tmp275Thread, NULL);
}

void tmp275_disable(i2caddr_t UNUSED(paddr))
{
  /* No-op if not running */
  if (running == 0)
    return;

  running = 0;
  /* TODO: Wait for completion? Optionally?
   * TODO: Shutdown
   */
}

int16_t tmp275_read(i2caddr_t addr)
{
  rx_data[0] = rx_data[1] = 0;

  sc_i2c_read(addr, rx_data, 2);

  /* First byte is MSB
   * Second byte has 4 LSB bits and the rest are zeroes
   * Thus it is a 8.8 fixed point representation
   */
  return (rx_data[0] << 8) | (rx_data[1]);
}

#endif /* SC_USE_TMP275 */
