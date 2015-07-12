/***
 * Radio Board related functions
 *
 * Copyright 2014 Tuomas Kulve, <tuomas.kulve@snowcap.fi>
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

#ifdef SC_HAS_RBV2

#define SC_LOG_MODULE_TAG SC_LOG_MODULE_RADIO

#include "sc_utils.h"
#include "sc_radio.h"
#include "sc_uart.h"
#include "sc.h"

#define DEBUG_FLOOD                         0

#define SC_RADIO_BSL_RETRIES                3
#define SC_RADIO_COMM_WRAPPER_LEN           6
#define SC_RADIO_COMM_BUF_LEN              64
#define SC_RADIO_COMM_BUF_MSG_LEN_IL        2
#define SC_RADIO_COMM_BUF_MSG_LEN_IH        3
#define SC_RADIO_COMM_CORE_RES_I            4
#define SC_RADIO_COMM_CORE_RES_CMD       0x3A
#define SC_RADIO_COMM_CORE_RES_MSG       0x3B
#define SC_RADIO_COMM_CORE_RES_MSG_OK     0x0

typedef enum SC_RADIO_STATE {
  SC_RADIO_STATE_NORMAL,
  SC_RADIO_STATE_FLASH_SYNC,
  SC_RADIO_STATE_FLASH_SYNC_UART_NOT_READY,
  SC_RADIO_STATE_FLASH_SYNC_UART_READY
} SC_RADIO_STATE;

static uint8_t radio_state = SC_RADIO_STATE_NORMAL;

static uint8_t radio_comm_buf[SC_RADIO_COMM_BUF_LEN];
static uint8_t radio_comm_buf_i = 0;
static uint16_t radio_comm_buf_expected = 0;
static uint8_t radio_comm_bsl_version[4];
static uint8_t radio_comm_data_crc[2];
static binary_semaphore_t radio_comm_sem;
static mutex_t radio_comm_mtx;

static uint16_t crc;
static void crc_ccitt_init(void);
static void crc_ccitt_update(uint8_t x);
static void crc_ccitt_set(uint8_t *buf, uint16_t len);
static uint8_t radio_comm_validate_reply(void);
static uint8_t radio_comm_validate_reply_successful(void);
static uint8_t radio_comm_validate_reply_version(void);
static uint8_t radio_comm_validate_reply_crc(void);
static uint8_t radio_comm_mass_erase(void);
static uint8_t radio_comm_send_password(void);
static uint8_t radio_comm_get_version(void);
static uint8_t radio_comm_send_program(uint8_t dry_run);
static uint8_t ascii2bin(uint8_t h, uint8_t l);

static THD_WORKING_AREA(sc_radio_flash_thread, 512);
THD_FUNCTION(scRadioFlashThread, arg)
{
  uint8_t retries = SC_RADIO_BSL_RETRIES;
  uint8_t msg_retries;

  (void)arg;

  chRegSetThreadName(__func__);

  chMtxObjectInit(&radio_comm_mtx);
  chBSemObjectInit(&radio_comm_sem, TRUE);

  // Do a dry run to validate SREC blob
  if (radio_comm_send_program(1) == 0) {
    SC_LOG("radio flash: failed: invalid srec\r\n");
    return;
  }

  do {

    // Reset radio to BSL mode
    if (sc_radio_reset_bsl() == 0) {
      continue;
    }

    // Try the first message for a few times
    msg_retries = SC_RADIO_BSL_RETRIES;
    do {
      // FIXME: validate new blob before mass erase
      if (radio_comm_mass_erase()) {
        break;
      }
      chThdSleepMilliseconds(100);
    } while (--msg_retries);

    if (msg_retries == 0) {
      chThdSleepMilliseconds(500);
      continue;
    }

    chThdSleepMilliseconds(5);

    // Send password
    if (radio_comm_send_password() == 0) {
      chThdSleepMilliseconds(100);
      continue;
    }

    chThdSleepMilliseconds(5);

    // Get BSL version
    if (radio_comm_get_version() == 0) {
      chThdSleepMilliseconds(100);
      continue;
    }

    chThdSleepMilliseconds(5);

    // Send new program to radio
    if (radio_comm_send_program(0) == 0) {
      chThdSleepMilliseconds(100);
      continue;
    }

    break;
  } while (--retries);

  sc_radio_reset_normal();

  if (retries == 0) {
    SC_LOG("radio flash: failed\r\n");
  } else {
    SC_LOG("radio flash: ok\r\n");
  }

  return;
}



/*
 * Check that the reply is a valid reply message
 */
static uint8_t radio_comm_validate_reply(void)
{
  if (radio_comm_buf_i == SC_RADIO_COMM_BUF_LEN) {
    SC_DBG("RADIO DEBUG: full buffer\r\n");
    return 0;
  }

  if (radio_comm_buf_i < radio_comm_buf_expected) {
    SC_DBG("RADIO DEBUG: invalid message\r\n");
    return 0;
  }

  return 1;
}



/*
 * Check that the reply is a valid reply message with an "operation succesfull"
 * return code.
 */
static uint8_t radio_comm_validate_reply_successful(void)
{
  chMtxLock(&radio_comm_mtx);

  if (radio_comm_validate_reply() == 0) {
    chMtxUnlock(&radio_comm_mtx);
    return 0;
  }

  if (radio_comm_buf[SC_RADIO_COMM_CORE_RES_I + 0] == SC_RADIO_COMM_CORE_RES_MSG &&
      radio_comm_buf[SC_RADIO_COMM_CORE_RES_I + 1] == SC_RADIO_COMM_CORE_RES_MSG_OK) {
    chMtxUnlock(&radio_comm_mtx);

    return 1;
  }

  chMtxUnlock(&radio_comm_mtx);

  SC_DBG("RADIO DEBUG: reply message not operation successful\r\n");
  return 0;
}



/*
 * Check that the reply is a valid reply message with a version number
 */
static uint8_t radio_comm_validate_reply_version(void)
{
  chMtxLock(&radio_comm_mtx);

  if (radio_comm_validate_reply() == 0) {
    chMtxUnlock(&radio_comm_mtx);
    return 0;
  }

  if (radio_comm_buf[SC_RADIO_COMM_CORE_RES_I] == SC_RADIO_COMM_CORE_RES_CMD) {
    uint8_t i;

    chDbgAssert(radio_comm_buf_i > SC_RADIO_COMM_CORE_RES_I + 4,
                "Invalid message for BSL version");

    for (i = 0; i < 4; ++i) {
      radio_comm_bsl_version[i] = radio_comm_buf[SC_RADIO_COMM_CORE_RES_I + 1 + i];
    }
    chMtxUnlock(&radio_comm_mtx);
    return 1;
  }

  chMtxUnlock(&radio_comm_mtx);

  SC_DBG("RADIO DEBUG: invalid version message\r\n");
  return 0;
}



/*
 * Check that the reply is a valid reply message with a CRC
 */
static uint8_t radio_comm_validate_reply_crc(void)
{
  chMtxLock(&radio_comm_mtx);

  if (radio_comm_validate_reply() == 0) {
    chMtxUnlock(&radio_comm_mtx);
    return 0;
  }

  if (radio_comm_buf[SC_RADIO_COMM_CORE_RES_I] == SC_RADIO_COMM_CORE_RES_CMD) {
    uint8_t i;

    chDbgAssert(radio_comm_buf_i > SC_RADIO_COMM_CORE_RES_I + 4,
                "Invalid message for CRC reply");

    for (i = 0; i < 2; ++i) {
      radio_comm_data_crc[i] = radio_comm_buf[SC_RADIO_COMM_CORE_RES_I + 1 + i];
    }
    chMtxUnlock(&radio_comm_mtx);
    return 1;
  }

  chMtxUnlock(&radio_comm_mtx);

  SC_DBG("RADIO DEBUG: invalid crc message\r\n");
  return 0;
}



/*
 * Send mass erase message to radio
 */
static uint8_t radio_comm_mass_erase(void)
{
  uint8_t mass_erase[] = {0x80, 0x01, 0x00, 0x15, 'X', 'X'};

  crc_ccitt_set(mass_erase, sizeof(mass_erase));

  chMtxLock(&radio_comm_mtx);
  radio_comm_buf_i = 0;
  radio_comm_buf_expected = 0;
  chMtxUnlock(&radio_comm_mtx);

  // Send password
  sc_uart_send_msg(SC_UART_2, mass_erase, sizeof(mass_erase));
  SC_DBG("RADIO DEBUG: sent mass erase\r\n");

  // Wait for reply
  if (chBSemWaitTimeout(&radio_comm_sem, 1000) != MSG_OK) {
    SC_DBG("RADIO DEBUG: time out\r\n");
    return 0;
  }

  if (radio_comm_validate_reply_successful() == 1) {
    SC_DBG("RADIO DEBUG: mass erase OK\r\n");
    return 1;
  } else {
    SC_DBG("RADIO DEBUG: mass erase failed\r\n");
    return 0;
  }
}



/*
 * Send empty password to radio
 */
static uint8_t radio_comm_send_password(void)
{
  uint8_t bsl_password[] = {0x80, 0x21, 0x00, 0x11,
                            0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
                            0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
                            'X', 'X'};

  crc_ccitt_set(bsl_password, sizeof(bsl_password));

  chMtxLock(&radio_comm_mtx);
  radio_comm_buf_i = 0;
  radio_comm_buf_expected = 0;
  chMtxUnlock(&radio_comm_mtx);

  // Send password
  sc_uart_send_msg(SC_UART_2, bsl_password, sizeof(bsl_password));
  SC_DBG("RADIO DEBUG: sent password\r\n");

  // Wait for reply
  if (chBSemWaitTimeout(&radio_comm_sem, 1000) != MSG_OK) {
    SC_DBG("RADIO DEBUG: time out\r\n");
    return 0;
  }

  if (radio_comm_validate_reply_successful() == 1) {
    SC_DBG("RADIO DEBUG: send password OK\r\n");
    return 1;
  } else {
    SC_DBG("RADIO DEBUG: send password failed\r\n");
    return 0;
  }
}



/*
 * Request BSL version number from radio
 */
static uint8_t radio_comm_get_version(void)
{
  uint8_t get_bsl_version[] = {0x80, 0x01, 0x00, 0x19, 'X', 'X'};

  crc_ccitt_set(get_bsl_version, sizeof(get_bsl_version));

  chMtxLock(&radio_comm_mtx);
  radio_comm_buf_i = 0;
  radio_comm_buf_expected = 0;
  chMtxUnlock(&radio_comm_mtx);

  // Get radio BSL version
  sc_uart_send_msg(SC_UART_2, get_bsl_version, sizeof(get_bsl_version));

  // Wait for reply
  if (chBSemWaitTimeout(&radio_comm_sem, 1000) != MSG_OK) {
    SC_DBG("RADIO DEBUG: time out\r\n");
    return 0;
  }

  if (radio_comm_validate_reply_version() == 0) {
    SC_DBG("RADIO DEBUG: get version failed\r\n");
    return 0;
  }

  {
    uint8_t msg[] = "RADIO DEBUG: Got BSL version:                  \r\n";
    uint8_t len = sizeof(msg) - 20;
    uint8_t i;

    chMtxLock(&radio_comm_mtx);
    for (i = 0; i < 4; ++i) {
      msg[len++] = ' ';
      len += sc_itoa(radio_comm_bsl_version[i], &msg[len], sizeof(msg) - len);
    }
    msg[len++] = '\r';
    msg[len++] = '\n';
    msg[len++] = '\0';
    chMtxUnlock(&radio_comm_mtx);
    SC_LOG(msg);
  }

  return 1;
}



/*
 * Convert two written ascii hex numbers to binary numbers
 */
static uint8_t ascii2bin(uint8_t h, uint8_t l)
{
  uint8_t ret;

  if (l < 'A') {
    ret = l - '0';
  } else {
    ret = l - 'A' + 10;
  }

  if (h < 'A') {
    ret |= ((h - '0') << 4);
  } else {
    ret |= ((h - 'A' + 10) << 4);
  }
  return ret;
}



/*
 * Parse SREC blob and flash it to radio
 */
static uint8_t radio_comm_send_program(uint8_t dry_run)
{
  uint8_t *blob;
  uint16_t blob_len;
  uint16_t i = 0;

  blob_len = sc_cmd_blob_get(&blob);
  if (blob_len == 0 || blob == NULL || blob[0] != 'S' || blob[1] != '0') {
    SC_DBG("RADIO DEBUG: No valid SREC blob\r\n");
    return 0;
  }

  // Skip initial SREC header record
  do {
    ++i;
  }	while(blob[i] != 'S' && i < blob_len);

  // Parse SREC line at a time
  while (i < blob_len) {

    if (blob[i++] != 'S') {
      SC_DBG("RADIO DEBUG: SREC: expected new rec start\r\n");
      return 0;
    }

    // SREC data record
    if (blob[i++] == '1') {
      uint8_t buf[64];
      uint8_t buf_len = 0;
      uint8_t count  = ascii2bin(blob[i + 0], blob[i + 1]);
      uint8_t addr_h = ascii2bin(blob[i + 2], blob[i + 3]);
      uint8_t addr_l = ascii2bin(blob[i + 4], blob[i + 5]);
      uint8_t srec_crc;
      uint8_t j;
      uint8_t retries = SC_RADIO_BSL_RETRIES + 1;
      uint8_t data_crc[2];
      i += 6;

      // BSL RX data block contains 3 byte address, while SREC address
      // is 2 bytes, hence count + 1
      buf[buf_len++] = 0x80;
      buf[buf_len++] = count + 1; // Length low byte
      buf[buf_len++] = 0x00;      // Length high byte
      buf[buf_len++] = 0x10;      // RX Data block
      buf[buf_len++] = addr_l;    // Addr low byte
      buf[buf_len++] = addr_h;    // Addr middle byte
      buf[buf_len++] = 0x00;      // Addr upper byte

      // Parse data from SREC to radio comm buffer and calculate CRC
      // for the data itself for verification
      crc_ccitt_init();
      for (j = 0; j < count - 3; ++j) {
        uint8_t data = ascii2bin(blob[i], blob[i + 1]);
        crc_ccitt_update(data);
        buf[buf_len++] = data;
        i += 2;
      }
      data_crc[0] = (uint8_t)(crc & 0x00ff);
      data_crc[1] = (uint8_t)((crc & 0xff00) >> 8);

      // FIXME: Use CRC byte from the SREC to validate the SREC data record.
      srec_crc = ascii2bin(blob[i], blob[i + 1]);
      i += 2;
      (void)srec_crc;

      if (blob[i++] == '\r') {
        // Allow SREC without \n
        if (blob[i + 1] == '\n') {
          ++i;
        }
      } else {
        SC_DBG("RADIO DEBUG: SREC: expected EOL\r\n");
        return 0;
      }

      // CCITT CRC bytes
      buf_len += 2;

      crc_ccitt_set(buf, buf_len);

      while (!dry_run && --retries) {
        uint8_t check_crc[] = {0x80, 0x06, 0x00, 0x16, addr_l, addr_h, 0x0, count - 3, 0x0, 'X', 'X'};

        crc_ccitt_set(check_crc, sizeof(check_crc));

        if (DEBUG_FLOOD) {
          uint8_t msg[] = "RADIO DEBUG: SREC len:                                   \r\n";
          uint8_t len = 23;
          len += sc_itoa(count, &msg[len], 5);
          msg[len++] = ' ';
          len += sc_itoa(addr_h, &msg[len], 5);
          msg[len++] = ' ';
          len += sc_itoa(addr_l, &msg[len], 5);
          msg[len++] = '\r';
          msg[len++] = '\n';
          msg[len++] = '\0';
          SC_DBG(msg);
        } else {
          static uint16_t bytes_sent = 1024;

          if (i > bytes_sent) {
            bytes_sent += 1024;
            SC_DBG("RADIO DEBUG: SREC 1K sent\r\n");
          }
        }

        chMtxLock(&radio_comm_mtx);
        radio_comm_buf_i = 0;
        radio_comm_buf_expected = 0;
        chMtxUnlock(&radio_comm_mtx);

        // Send data block
        sc_uart_send_msg(SC_UART_2, buf, buf_len);

        // Wait for reply
        if (chBSemWaitTimeout(&radio_comm_sem, 1000) != MSG_OK) {
          SC_DBG("RADIO DEBUG: time out\r\n");
          continue;
        }

        if (radio_comm_validate_reply_successful() == 0) {
          SC_DBG("RADIO DEBUG: data block rx failed\r\n");
          continue;
        }

        chMtxLock(&radio_comm_mtx);
        radio_comm_buf_i = 0;
        radio_comm_buf_expected = 0;
        chMtxUnlock(&radio_comm_mtx);

        // Verify CRC
        sc_uart_send_msg(SC_UART_2, check_crc, sizeof(check_crc));

        // Wait for reply
        if (chBSemWaitTimeout(&radio_comm_sem, 1000) != MSG_OK) {
          SC_DBG("RADIO DEBUG: time out\r\n");
          continue;
        }

        if (radio_comm_validate_reply_crc() == 0) {
          SC_DBG("RADIO DEBUG: crc reply failed\r\n");
          continue;
        }

        // Verify CRC
        if (radio_comm_data_crc[0] != data_crc[0] || radio_comm_data_crc[1] != data_crc[1]) {
          SC_DBG("RADIO DEBUG: crc mismatch\r\n");
          continue;
        }

        break;
      }
      if (retries == 0) {
        return 0;
      }
    } else {
      // S9 termination record
      break;
    }
  }
  if (blob[i - 1] != '9') {
    SC_DBG("RADIO DEBUG: SREC: expected termination record\r\n");
    return 0;
  }

  // Count bytes to verify valid length
  // FIXME: Hackish, allowing missing \n and allow \0 instead of \r\n
  while((blob[i] != '\n' && blob[i] != '\r' && blob[i] != '\0') && i < blob_len) {
    ++i;
  }

  // FIXME: hackish, allowing 2 bytes of mismatch due to different
  // line endings (\r\n\0)
  if (blob_len - i > 2) {
    SC_DBG("RADIO DEBUG: SREC: invalid length\r\n");
    return 0;
  }

  return 1;
}


/*
 * Calculate CRC and update it to the buf
 */
static void crc_ccitt_set(uint8_t *buf, uint16_t len)
{
  uint16_t i;
  crc_ccitt_init();

  for (i = 3; i < len - 2; ++i) {
    crc_ccitt_update(buf[i]);
  }

  // CRC, low byte
  buf[len - 2] = (uint8_t)(crc & 0x00ff);
  // CRC, high byte
  buf[len - 1] = (uint8_t)((crc & 0xff00) >> 8);
}



/*
 * Initialise CRC
 */
static void crc_ccitt_init(void)
{
  crc = 0xffff;
}



/*
 * Update CRC for the next byte
 */
static void crc_ccitt_update(uint8_t x)
{
  uint16_t crc_new = (uint8_t)(crc >> 8) | (crc << 8);
  crc_new ^= x;
  crc_new ^= (uint8_t)(crc_new & 0xff) >> 4;
  crc_new ^= crc_new << 12;
  crc_new ^= (crc_new & 0xff) << 5;
  crc = crc_new;
}



/*
 * Init radio
 */
void sc_radio_init(void)
{
  radio_state = SC_RADIO_STATE_NORMAL;

  palSetPadMode(SC_RADIO_NRST_PORT,
                SC_RADIO_NRST_PIN,
                PAL_MODE_OUTPUT_PUSHPULL);
  palSetPadMode(SC_RADIO_TEST_PORT,
                SC_RADIO_TEST_PIN,
                PAL_MODE_OUTPUT_PUSHPULL);

  // Release reset, test pin to low.
  sc_radio_set_reset(0);
  sc_radio_set_test(0);
}



/*
 * Deinit radio
 */
void sc_radio_deinit(void)
{
  // Nothing to do here.
}



void sc_radio_set_reset(uint8_t enable)
{
  uint8_t msg[] = "R: R_\r\n";

  if (enable) {
    msg[4] = '1';
    palClearPad(SC_RADIO_NRST_PORT, SC_RADIO_NRST_PIN);
  } else {
    msg[4] = '0';
    palSetPad(SC_RADIO_NRST_PORT, SC_RADIO_NRST_PIN);
  }
  SC_DBG(msg);
}



void sc_radio_set_test(uint8_t enable)
{
  uint8_t msg[] = "R: T_\r\n";


  if (enable) {
    msg[4] = '1';
    palSetPad(SC_RADIO_TEST_PORT, SC_RADIO_TEST_PIN);
  } else {
    msg[4] = '0';
    palClearPad(SC_RADIO_TEST_PORT, SC_RADIO_TEST_PIN);
  }
  SC_DBG(msg);
}


/*
 * Reset radio to BSL mode, return 0 on failure, 1 otherwise.
 */
uint8_t sc_radio_reset_bsl(void)
{
  uint16_t i;
  uint8_t retries = SC_RADIO_BSL_RETRIES;

  radio_state = SC_RADIO_STATE_FLASH_SYNC_UART_NOT_READY;

  sc_uart_stop(SC_UART_2);
  palSetPadMode(SC_UART2_TX_PORT, SC_UART2_TX_PIN, PAL_MODE_INPUT_PULLDOWN);
  palSetPadMode(SC_UART2_RX_PORT, SC_UART2_RX_PIN, PAL_MODE_INPUT_PULLDOWN);

  while (retries--) {
    // Put the radio to reset state
    sc_radio_set_reset(1);
    sc_radio_set_test(0);

    chThdSleepMilliseconds(1);

    // Put the radio to BSL state
    sc_radio_set_test(1);
    chThdSleepMilliseconds(1);
    sc_radio_set_test(0);
    chThdSleepMilliseconds(1);
    sc_radio_set_test(1);
    chThdSleepMilliseconds(1);
    sc_radio_set_reset(0);
    chThdSleepMilliseconds(1);
    sc_radio_set_test(0);

    // Wait up to 1 second for the radio's TX to get high
    for (i = 0; i < 1000; ++i) {
      if (palReadPad(SC_UART2_RX_PORT, SC_UART2_RX_PIN) == 1) {
        break;
      } else {
        chThdSleepMilliseconds(1);
      }
    }

    // Wait awhile and verify that TX is still high
    chThdSleepMilliseconds(100);

    if (palReadPad(SC_UART2_RX_PORT, SC_UART2_RX_PIN) == 1) {

      palSetPadMode(SC_UART2_RX_PORT, SC_UART2_RX_PIN, PAL_MODE_ALTERNATE(SC_UART2_RX_AF));
      palSetPadMode(SC_UART2_TX_PORT, SC_UART2_TX_PIN, PAL_MODE_ALTERNATE(SC_UART2_TX_AF));
      // Set 9600, 8 data bits, 9th bit as even parity bit
      sc_uart_set_config(SC_UART_2, 9600, USART_CR1_PCE | USART_CR1_M, 0/*USART_CR2_LINEN*/, 0);
      sc_uart_start(SC_UART_2);

      radio_state = SC_RADIO_STATE_FLASH_SYNC_UART_READY;
      return 1;
    }
  }

  return 0;
}



/*
 * Reset radio to normal mode
 */
void sc_radio_reset_normal(void)
{
  radio_state = SC_RADIO_STATE_FLASH_SYNC_UART_NOT_READY;

  sc_uart_stop(SC_UART_2);

  sc_radio_set_test(0);
  chThdSleepMilliseconds(1);
  sc_radio_set_test(1);
  chThdSleepMilliseconds(1);
  sc_radio_set_test(0);
  chThdSleepMilliseconds(1);
  sc_radio_set_test(1);
  chThdSleepMilliseconds(1);
  sc_radio_set_test(0);
  sc_radio_set_reset(1);
  chThdSleepMilliseconds(1);
  sc_radio_set_reset(0);
  chThdSleepMilliseconds(1);

  // Set 115200, 8 data bits, no parity bit
  sc_uart_set_config(SC_UART_2, 115200, 0, USART_CR2_LINEN, 0);
  sc_uart_start(SC_UART_2);

  radio_state = SC_RADIO_STATE_NORMAL;
}


void sc_radio_process_byte(uint8_t byte)
{

  switch (radio_state) {
  case SC_RADIO_STATE_NORMAL:
    // In normal operation send the byte to USB
    // FIXME: sent to main app to decide what to do (although it was
    // just sent from main ap to here..)
    sc_uart_send_msg(SC_UART_USB, &byte, 1);
    break;
  case SC_RADIO_STATE_FLASH_SYNC_UART_NOT_READY:
    // Ignore any line toggling when UART is not ready
    break;
  case SC_RADIO_STATE_FLASH_SYNC_UART_READY:
    // Flashing radio

    chDbgAssert(radio_comm_buf_i < SC_RADIO_COMM_BUF_LEN, "Radio comm buffer full");

    if (DEBUG_FLOOD) {
      uint8_t len;
      uint8_t msg[] = "RADIO DEBUG: recv:    \r\n";
      len = sizeof(msg) - 5;
      len += sc_itoa(byte, &msg[len], 5);
      msg[len++] = '\r';
      msg[len++] = '\n';
      msg[len++] = '\0';
      SC_DBG(msg);
    }
    chMtxLock(&radio_comm_mtx);

    if (radio_comm_buf_i == 0 && byte != 0x0) {
      // One byte error message from radio
      chBSemSignal(&radio_comm_sem);
      chMtxUnlock(&radio_comm_mtx);
      break;
    }

    radio_comm_buf[radio_comm_buf_i++] = byte;

    if (radio_comm_buf_i - 1 == SC_RADIO_COMM_BUF_MSG_LEN_IH) {
      // Should have message length now
      if (radio_comm_buf[0] == 0 && radio_comm_buf[1] == 0x80) {
        radio_comm_buf_expected =
          ((((uint16_t)radio_comm_buf[SC_RADIO_COMM_BUF_MSG_LEN_IH]) << 8) |
           radio_comm_buf[SC_RADIO_COMM_BUF_MSG_LEN_IL]) +
          SC_RADIO_COMM_WRAPPER_LEN;

        if (DEBUG_FLOOD) {
          uint8_t len;
          uint8_t msg[] = "RADIO DEBUG: got expected count:       \r\n";
          len = sizeof(msg) - 9;
          len += sc_itoa(radio_comm_buf_expected, &msg[len], 5);
          msg[len++] = '\r';
          msg[len++] = '\n';
          msg[len++] = '\0';
          SC_DBG(msg);
        }
      } else {
        // Have enough bytes but not a valid message.
        chBSemSignal(&radio_comm_sem);
      }
    }

    if (radio_comm_buf_i == radio_comm_buf_expected) {
      // Has full message
      chBSemSignal(&radio_comm_sem);
    }

    if (radio_comm_buf_i == SC_RADIO_COMM_BUF_LEN) {
      // Buffer full
      chBSemSignal(&radio_comm_sem);
    }
    chMtxUnlock(&radio_comm_mtx);
    break;
  default:
    // Unknown state
    chDbgAssert(0, "Unknown Radio state");
    break;
  }
}



/*
 * Launch a separate thread for driving the flash process
 */
void sc_radio_flash(void)
{
  chThdCreateStatic(sc_radio_flash_thread, sizeof(sc_radio_flash_thread), NORMALPRIO, scRadioFlashThread, NULL);

  SC_DBG("R: F\r\n");
}

#endif // SC_HAS_RBV2



/* Emacs indentatation information
   Local Variables:
   indent-tabs-mode:nil
   tab-width:2
   c-basic-offset:2
   End:
*/
