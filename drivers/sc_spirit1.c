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

#define SC_LOG_MODULE_TAG SC_LOG_MODULE_UNSPECIFIED

#ifdef SC_HAS_SPIRIT1

#include "sc.h"
#include "sc_spirit1.h"
#include "sc_spi.h"
#include "sc_extint.h"
#include "sc_uart.h"
#include "SPIRIT_Types.h"
#include "SPIRIT_Gpio.h"
#include "SPIRIT_PktBasic.h"
#include "SPIRIT_Irq.h"
#include "SPIRIT_Timer.h"
#include "SPIRIT_Commands.h"
#include "SPIRIT_Qi.h"
#include "MCU_Interface.h"

#ifndef SC_ALLOW_ST_LIBERTY_V2
#error "This driver uses Spirit1 code from ST licensed under MCD-ST Liberty SW License Agreement V2"
#endif

#ifdef SC_ALLOW_GPL
#error "GPL is probably not compatible with MCD-ST Liberty SW License Agreement V2"
#endif

// Integrity check characters
#define SPIRIT1_INTEGRITY_ID1         'S'
#define SPIRIT1_INTEGRITY_ID2         'C'

// Max number of retransmissions
#define SPIRIT1_RETX_COUNT            4

#define LINEAR_FIFO_ADDRESS           0xFF
#define XTAL_FREQUENCY                50000000
#define SPIRIT_VERSION                SPIRIT_VERSION_3_0
#define RANGE_TYPE                    RANGE_EXT_NONE
#define RSSI_THRESHOLD                -120
// Timeout for one retry
#define SPIRIT1_RETX_TIMEOUT_MS     100

// Spirit1's FIFO size is 96 bytes
#ifndef SC_SPIRIT1_TX_BUF_MAX_LEN
#define SC_SPIRIT1_TX_BUF_MAX_LEN      96
#endif

static uint8_t spin;
static binary_semaphore_t spirit1_act_sem;
static binary_semaphore_t spirit1_aes_sem;
static binary_semaphore_t spirit1_irq_sem;
static thread_t *sc_spirit1_act_thread_ptr;
static thread_t *sc_spirit1_irq_thread_ptr;

static uint8_t *spirit1_enc_key;

typedef enum {
  SPIRIT1_MSG_IDX_ID1          = 0,
  SPIRIT1_MSG_IDX_ID2          = 1,
  SPIRIT1_MSG_IDX_LEN          = 2,
  SPIRIT1_MSG_IDX_FLAGS        = 3,
  SPIRIT1_MSG_IDX_SEQ          = 4,
  SPIRIT1_MSG_IDX_PAYLOAD      = 5,
} SPIRIT1_MSG_IDX;

typedef enum {
  SPIRIT1_FLAG_TX_DATA_SENT    = (0x01 << 0),
  SPIRIT1_FLAG_RX_DATA_READY   = (0x01 << 1),
  SPIRIT1_FLAG_RX_TIMEOUT      = (0x01 << 2),
} SPIRIT1_FLAG;

static volatile SPIRIT1_FLAG flags;

typedef enum {
  SPIRIT1_MSG_FLAG_ACK         = (0x1 << 0),
  SPIRIT1_MSG_FLAG_RETX        = (0x1 << 1),
} SPIRIT1_MSG_FLAG;

typedef enum {
  SPIRIT1_BUF_STATE_ENC        = (0x1 << 0),
  SPIRIT1_BUF_STATE_WAIT_ACK   = (0x1 << 1),
  SPIRIT1_BUF_STATE_DEC        = (0x1 << 2),
  SPIRIT1_BUF_STATE_PARSED     = (0x1 << 3),
} SPIRIT1_BUF_STATE;

static struct SPIRIT1_BUF {
  bool busy; // FIXME: theoretically should be one SPIRIT1_BUF_STATE
  uint8_t seq;
  uint8_t flags;
  uint8_t addr;
  uint8_t buf[SC_SPIRIT1_TX_BUF_MAX_LEN];
  uint8_t len;
  uint8_t retx;
  uint8_t lqi;
  uint8_t rssi;
  SPIRIT1_BUF_STATE state;
  mutex_t mtx;
} tx_buf, rx_buf;

// Spirit1 radio configuration
#define XTAL_OFFSET_PPM             0
#define INFINITE_TIMEOUT            0.0
#define BASE_FREQUENCY              868.0e6
#define CHANNEL_SPACE               20e3
#define CHANNEL_NUMBER              0
#define MODULATION_SELECT           FSK
#define DATARATE                    38400
#define FREQ_DEVIATION              20e3
#define BANDWIDTH                   100e3

#define POWER_DBM                   11.6
#define POWER_INDEX                 7

/* Spirit1 basic packet configuration */
#define PREAMBLE_LENGTH             PKT_PREAMBLE_LENGTH_04BYTES
#define SYNC_LENGTH                 PKT_SYNC_LENGTH_4BYTES
#define SYNC_WORD                   0x1A2635A8
#define LENGTH_TYPE                 PKT_LENGTH_VAR
#define LENGTH_WIDTH                7
#define CRC_MODE                    PKT_CRC_MODE_16BITS_1
#define CONTROL_LENGTH              PKT_CONTROL_LENGTH_0BYTES
#define EN_ADDRESS                  S_ENABLE
// FEC needs even payload len, but AES already needs multiple of 16
#define EN_FEC                      S_ENABLE
#define EN_WHITENING                S_ENABLE

/* Spirit1 packet address configuration */
#define EN_FILT_MY_ADDRESS          S_DISABLE
#define EN_FILT_MULTICAST_ADDRESS   S_DISABLE
#define MULTICAST_ADDRESS           SPIRIT1_MULTICAST_ADDRESS
#define EN_FILT_BROADCAST_ADDRESS   S_DISABLE
#define BROADCAST_ADDRESS           SPIRIT1_BROADCAST_ADDRESS
#define EN_FILT_SOURCE_ADDRESS      S_DISABLE

static inline SpiritStatus uint2status(uint8_t *data)
{
  SpiritStatus stat;

  uint16_t data16 = (((uint16_t)data[0]) << 8) | data[1];

  stat = *(SpiritStatus*)(void*)&data16;

  return stat;
}

static void spirit1_aes(bool encrypt, struct SPIRIT1_BUF *buf);
static void spirit1_goto_ready(void);
static void spirit1_irq_cb(EXTDriver *extp, expchannel_t channel);
static void spirit1_start_tx(void);
static void spirit1_retransmit(void);
static void spirit1_start_rx(uint16_t timeout_ms);
static void spirit1_send_ack(uint8_t addr, uint8_t seq);
static void spirit1_rx_get_packet(void);
static void spirit1_rx_parse(void);
static void spirit1_send_flag(uint8_t addr,
                              uint8_t *buf,
                              uint8_t len,
                              SPIRIT1_MSG_FLAG flags);



/*
 * Encrypt or decrypt the buf with the given key.
 * NOTE: This must be called with the mutex taken.
 */
static void spirit1_aes(bool encrypt, struct SPIRIT1_BUF *buf)
{
  uint8_t bytes_left;
  uint8_t bytes_to_aes;

  chDbgAssert(buf->busy && buf->state == 0,
              "AES called, but buf not in correct state");

  SpiritAesMode(S_ENABLE);
  SpiritAesWriteKey(spirit1_enc_key);

  // Start AES operation
  bytes_left = buf->len;
  while(bytes_left) {
    bytes_to_aes = bytes_left;
    if (bytes_to_aes > 16) {
      bytes_to_aes = 16;
    }
    SpiritAesWriteDataIn(&buf->buf[buf->len - bytes_left], bytes_to_aes);

    if (encrypt) {
      SpiritAesExecuteEncryption();
    } else {
      SpiritAesDeriveDecKeyExecuteDec();
    }

    // Wait for AES to complete
    if (chBSemWaitTimeout(&spirit1_aes_sem, MS2ST(100)) != MSG_OK) {
      chDbgAssert(0, "AES not finishing in 100ms");
    }

    SpiritAesReadDataOut(&buf->buf[buf->len - bytes_left], 16);
    bytes_left -= bytes_to_aes;
  }

  SpiritAesMode(S_DISABLE);

  // Mark buffer as encrypted or decrypted
  if (encrypt) {
    // Add the possible AES padding to the buf len
    buf->len += (16 - bytes_to_aes);
    buf->state |= SPIRIT1_BUF_STATE_ENC;
  } else {
    buf->state |= SPIRIT1_BUF_STATE_DEC;
  }
}



static THD_WORKING_AREA(sc_spirit1_irq_thread, 1024);
THD_FUNCTION(scSpirit1IrqThread, arg)
{
  (void)arg;
  chRegSetThreadName(__func__);

  // Loop waiting for interrupts
  while (!chThdShouldTerminateX()) {
    SpiritIrqs irqs;
    SPIRIT1_FLAG local_flags = 0;

    if (chBSemWait(&spirit1_irq_sem) != MSG_OK) {
      continue;
    }

    SpiritIrqGetStatus(&irqs);

    if(irqs.IRQ_AES_END) {
      chBSemSignal(&spirit1_aes_sem);
    }

    if(irqs.IRQ_TX_DATA_SENT) {
      local_flags |= SPIRIT1_FLAG_TX_DATA_SENT;
    }

    if(irqs.IRQ_RX_DATA_READY) {
      local_flags |= SPIRIT1_FLAG_RX_DATA_READY;
    }

    if(irqs.IRQ_RX_TIMEOUT) {
      local_flags |= SPIRIT1_FLAG_RX_TIMEOUT;
    }

    if (local_flags) {
      chSysLock();
      flags = local_flags;
      chSysUnlock();
      chBSemSignal(&spirit1_act_sem);
    }
  }
}



static THD_WORKING_AREA(sc_spirit1_act_thread, 2048);
THD_FUNCTION(scSpirit1ActThread, arg)
{
  SPIRIT1_FLAG local_flags;
  (void)arg;

  chRegSetThreadName(__func__);

  while (!chThdShouldTerminateX()) {

    if (chBSemWait(&spirit1_act_sem) != MSG_OK) {
      continue;
    }

    // Atomically store and clear the global flags
    chSysLock();
    local_flags = flags;
    flags = 0;
    chSysUnlock();

    // If data sent, start listen (either for ACK or in general)
    if (local_flags & SPIRIT1_FLAG_TX_DATA_SENT) {
      uint16_t timeout = SPIRIT1_RETX_TIMEOUT_MS;

      chMtxLock(&tx_buf.mtx);

      chDbgAssert(tx_buf.busy, "TX done but TX buffer is not busy");

      if (tx_buf.state & SPIRIT1_BUF_STATE_WAIT_ACK) {
        timeout = SPIRIT1_RETX_TIMEOUT_MS;
      } else {
        // Not waiting for ACK, release the buffer
        tx_buf.state = 0;
        tx_buf.busy = 0;
        timeout = 0;
      }
      chMtxUnlock(&tx_buf.mtx);

      spirit1_start_rx(timeout);
    }

    // New incoming packet ready in radio
    if (local_flags & SPIRIT1_FLAG_RX_DATA_READY) {
      chMtxLock(&rx_buf.mtx);
      // Must SABORT to be ablet get RSSI in get_packet
      spirit1_goto_ready();
      spirit1_rx_get_packet();
      if (rx_buf.busy && rx_buf.state == 0) {
        //debug_print_packet("rx before aes", &rx_buf);
        spirit1_aes(false, &rx_buf);
        //debug_print_packet("rx after  aes", &rx_buf);
        spirit1_rx_parse();
      }
      chMtxUnlock(&rx_buf.mtx);
    }

    // Timeout waiting for ACK
    if (local_flags & SPIRIT1_FLAG_RX_TIMEOUT) {
      SpiritCmdStrobeFlushRxFifo();
      spirit1_retransmit();
    }

    // Check if the TX buf is ready for encryption and sending
    chMtxLock(&tx_buf.mtx);
    if (tx_buf.busy && tx_buf.state == 0) {
      //debug_print_packet("tx before aes", &tx_buf);
      spirit1_aes(true, &tx_buf);
      //debug_print_packet("tx after  aes", &tx_buf);
      //spirit1_aes(false, &tx_buf);
      //debug_print_packet("tx after aes2", &tx_buf);
      spirit1_start_tx();
    }
    chMtxUnlock(&tx_buf.mtx);
  }
}


/*
 * GPIO interrupt from the Spirit1 radio chip
 */
static void spirit1_irq_cb(EXTDriver *extp, expchannel_t channel)
{
  (void)extp;
  (void)channel;

  chSysLockFromISR();
  chBSemSignalI(&spirit1_irq_sem);
  chSysUnlockFromISR();
}



/*
 * Set radio state to READY
 */
static void spirit1_goto_ready(void)
{
  SpiritRefreshStatus();

  switch (g_xStatus.MC_STATE) {
  case MC_STATE_RX:
  case MC_STATE_TX:
    SpiritCmdStrobeSabort();
    break;
  case MC_STATE_READY:
    // Already in the right state
    return;
    break;
  default:
    chDbgAssert(0, "Unexpected radio state");
    break;
  }

  SpiritRefreshStatus();

  chDbgAssert(g_xStatus.MC_STATE == MC_STATE_READY,
              "Failed to set ready state");
}



/*
 * Copy local buffer to radio and initiate send
 * NOTE: This must be called with the TX mutex taken
 */
static void spirit1_start_tx(void)
{
  spirit1_goto_ready();

  chDbgAssert(tx_buf.busy, "Starting send but nothing to send");

  if (tx_buf.retx) {
    // Mark buffer as waiting for ACK
    tx_buf.state |= SPIRIT1_BUF_STATE_WAIT_ACK;
  }

  SpiritPktBasicSetPayloadLength(tx_buf.len);
  SpiritPktBasicSetDestinationAddress(tx_buf.addr);
  SpiritCmdStrobeFlushTxFifo();

  SpiritSpiWriteLinearFifo(tx_buf.len, tx_buf.buf);

  SpiritCmdStrobeTx();
}



/*
 * Retransmit the message that is already in the Spirit1's FIFO
 */
static void spirit1_retransmit(void)
{
  chMtxLock(&tx_buf.mtx);

  if (!tx_buf.busy) {
    chDbgAssert(0, "Starting retransmit but tx buf not busy anymore");
    chMtxUnlock(&tx_buf.mtx);
    return;
  }

  if (tx_buf.retx == 0) {
    // No more retransmissions, discard the TX buffer
    tx_buf.state = 0;
    tx_buf.busy = false;
    chMtxUnlock(&tx_buf.mtx);
    spirit1_start_rx(0);
    return;
  }

  tx_buf.retx--;

  spirit1_start_tx();

  chMtxUnlock(&tx_buf.mtx);
}



/*
 * Send ACK
 */
static void spirit1_send_ack(uint8_t addr, uint8_t seq)
{
  uint8_t ack_buf[1] = {seq};

  // send will skip sending, if TX buf already busy. That's OK as it
  // will cause the other end to resend.
  spirit1_send_flag(addr, ack_buf, sizeof(ack_buf), SPIRIT1_MSG_FLAG_ACK);
}



/*
 * Enable radio to listen
 */
static void spirit1_start_rx(uint16_t timeout_ms)
{
  spirit1_goto_ready();

  if (!timeout_ms) {
    SET_INFINITE_RX_TIMEOUT();
    SpiritTimerSetRxTimeoutStopCondition(ANY_ABOVE_THRESHOLD);
  } else {
    SpiritTimerSetRxTimeoutMs(timeout_ms);
    SpiritQiSetSqiThreshold(SQI_TH_0);
    SpiritQiSqiCheck(S_ENABLE);
    SpiritTimerSetRxTimeoutStopCondition(RSSI_AND_SQI_ABOVE_THRESHOLD);
  }

  SpiritCmdStrobeRx();
}



/*
 * Copy received packet from radio to driver buffer
 * NOTE: This must be called with the mutex taken.
 */
static void spirit1_rx_get_packet(void)
{
  if (rx_buf.busy) {
    // Overwriting the old buffer
    // FIXME: increase some global overflow counter?
  }

  rx_buf.len = SpiritLinearFifoReadNumElementsRxFifo();
  rx_buf.addr = SpiritPktCommonGetReceivedSourceAddress();

  if (rx_buf.len >= SC_SPIRIT1_TX_BUF_MAX_LEN) {
    rx_buf.len = SC_SPIRIT1_TX_BUF_MAX_LEN - 1;
  }

  if (rx_buf.len == 0) {
    chDbgAssert(tx_buf.busy, "Recv 0 len msg");
    rx_buf.busy = false;
    rx_buf.state = 0;
    SpiritCmdStrobeFlushRxFifo();
    return;
  }

  SpiritSpiReadLinearFifo(rx_buf.len, rx_buf.buf);

  rx_buf.buf[rx_buf.len] = '\0';
  rx_buf.busy = true;
  rx_buf.state = 0;
  rx_buf.lqi = SpiritQiGetLqi();
  // RX must be aborted before querying RSSI
  rx_buf.rssi = SpiritQiGetRssi();
  SpiritCmdStrobeFlushRxFifo();
}



/*
 * Send with flags
 */
static void spirit1_send_flag(uint8_t addr, uint8_t *buf, uint8_t len, SPIRIT1_MSG_FLAG send_flags)
{
  // 1 byte for length, 1 byte for seq, 1 byte to flags, 2 bytes to validity check "SC"
  if (len + SPIRIT1_MSG_IDX_PAYLOAD > SC_SPIRIT1_TX_BUF_MAX_LEN) {
    // FIXME: divide into multiple buffers
    chDbgAssert(0, "Buffer too big to be sent");
    return;
  }

  chMtxLock(&tx_buf.mtx);
  if (tx_buf.busy) {
    // FIXME: increase some global overflow counter?
    chMtxUnlock(&tx_buf.mtx);
    return;
  }

  tx_buf.busy = true;
  tx_buf.addr = addr;
  tx_buf.len = len + SPIRIT1_MSG_IDX_PAYLOAD;
  tx_buf.flags = send_flags;
  tx_buf.state = 0;
  tx_buf.seq++;

  if (send_flags) {
    tx_buf.retx = 0;
  } else {
    tx_buf.retx = SPIRIT1_RETX_COUNT;
  }

  // Store 'S', 'C' as the first two bytes for validity check.
  // Store length as the third byte so that the receiver knows the
  // actual length even with padding.
  // Store seq number as the byte 4.
  tx_buf.buf[SPIRIT1_MSG_IDX_ID1]   = SPIRIT1_INTEGRITY_ID1;
  tx_buf.buf[SPIRIT1_MSG_IDX_ID2]   = SPIRIT1_INTEGRITY_ID2;
  tx_buf.buf[SPIRIT1_MSG_IDX_LEN]   = tx_buf.len;
  tx_buf.buf[SPIRIT1_MSG_IDX_FLAGS] = tx_buf.flags;
  tx_buf.buf[SPIRIT1_MSG_IDX_SEQ]   = tx_buf.seq;
  for (uint8_t i = 0; i < len; ++i) {
    tx_buf.buf[i + SPIRIT1_MSG_IDX_PAYLOAD] = buf[i];
  }

  chMtxUnlock(&tx_buf.mtx);

  chBSemSignal(&spirit1_act_sem);
}



/*
 * Parse the received packet
 * NOTE: This must be called with the RX mutex taken
 */
static void spirit1_rx_parse(void)
{

  chDbgAssert(rx_buf.busy, "Trying to parse but rx buffer empty");

  // Discard package if integrity characters not found
  if (rx_buf.buf[SPIRIT1_MSG_IDX_ID1] != SPIRIT1_INTEGRITY_ID1 ||
      rx_buf.buf[SPIRIT1_MSG_IDX_ID2] != SPIRIT1_INTEGRITY_ID2) {

    uint16_t timeout = 0;

    rx_buf.state = 0;
    rx_buf.busy = false;

    // Check if returning to wait for ACK or normal listen mode
    chMtxLock(&tx_buf.mtx);
    if (tx_buf.busy && tx_buf.state & SPIRIT1_BUF_STATE_WAIT_ACK) {
      // FIXME: in theory the timeout should be decreased as we
      // probably already waited a bit
      timeout = SPIRIT1_RETX_TIMEOUT_MS;
    }
    chMtxUnlock(&tx_buf.mtx);

    spirit1_start_rx(timeout);
    return;
  }

  rx_buf.len   = rx_buf.buf[SPIRIT1_MSG_IDX_LEN];
  rx_buf.seq   = rx_buf.buf[SPIRIT1_MSG_IDX_SEQ];
  rx_buf.flags = rx_buf.buf[SPIRIT1_MSG_IDX_FLAGS];
  rx_buf.state |= SPIRIT1_BUF_STATE_PARSED;
  rx_buf.buf[rx_buf.len + 1] = '\0';

  // Check if ACK received
  if (rx_buf.flags & SPIRIT1_MSG_FLAG_ACK) {
    uint16_t timeout = 0;

    // Free the buffer
    rx_buf.busy = false;
    rx_buf.state = 0;

    chMtxLock(&tx_buf.mtx);
    if (!tx_buf.busy) {
      chMtxUnlock(&tx_buf.mtx);
      spirit1_start_rx(0);
      return;
    }

    if (tx_buf.seq == rx_buf.buf[SPIRIT1_MSG_IDX_PAYLOAD]) {
      // TX buffer ack'ed, so let's free it
      tx_buf.retx = 0;
      tx_buf.state = 0;
      tx_buf.busy = false;
    } else {
      // Still waiting for ACK
      timeout = SPIRIT1_RETX_TIMEOUT_MS;
    }
    chMtxUnlock(&tx_buf.mtx);

    spirit1_start_rx(timeout);
  } else  {
    uint8_t seq = rx_buf.buf[SPIRIT1_MSG_IDX_SEQ];
    uint8_t len = rx_buf.buf[SPIRIT1_MSG_IDX_LEN];
    uint8_t addr = rx_buf.addr;

    if (len) {
      msg_t drdy;

      // Notify main app about new data
      drdy = sc_event_msg_create_type(SC_EVENT_TYPE_SPIRIT1_AVAILABLE);
      sc_event_msg_post(drdy, SC_EVENT_MSG_POST_FROM_NORMAL);

      // Send ack back to the receiver
      spirit1_send_ack(addr, seq);
    }
  }
}



// FIXME: add a flag parameter for frequency?
/*
 * Initialise the Spirit1
 */
void sc_spirit1_init(uint8_t *enc_key, uint8_t my_addr)
{
  int8_t spi_n;
  SRadioInit radio_conf = {
    XTAL_OFFSET_PPM,
    BASE_FREQUENCY,
    CHANNEL_SPACE,
    CHANNEL_NUMBER,
    MODULATION_SELECT,
    DATARATE,
    FREQ_DEVIATION,
    BANDWIDTH
  };

  SGpioInit gpio_conf = {
    SPIRIT_GPIO_3,
    SPIRIT_GPIO_MODE_DIGITAL_OUTPUT_LP,
    SPIRIT_GPIO_DIG_OUT_IRQ
  };

  PktBasicInit packet_conf = {
    PREAMBLE_LENGTH,
    SYNC_LENGTH,
    SYNC_WORD,
    LENGTH_TYPE,
    LENGTH_WIDTH,
    CRC_MODE,
    CONTROL_LENGTH,
    EN_ADDRESS,
    EN_FEC,
    EN_WHITENING
  };

  PktBasicAddressesInit addr_conf = {
    EN_FILT_MY_ADDRESS,
    my_addr,
    EN_FILT_MULTICAST_ADDRESS,
    MULTICAST_ADDRESS,
    EN_FILT_BROADCAST_ADDRESS,
    BROADCAST_ADDRESS
  };

  flags = 0;

  // Store pointer to the encryption key
  spirit1_enc_key = enc_key;

  chBSemObjectInit(&spirit1_act_sem, TRUE);
  chBSemObjectInit(&spirit1_irq_sem, TRUE);
  chBSemObjectInit(&spirit1_aes_sem, TRUE);
  chMtxObjectInit(&tx_buf.mtx);
  chMtxObjectInit(&rx_buf.mtx);

  // Pin mux
  // FIXME: Should these be in the SPI module?
  palSetPadMode(SC_SPIRIT1_SPI_SCK_PORT,
                SC_SPIRIT1_SPI_SCK_PIN,
                PAL_MODE_ALTERNATE(SC_SPIRIT1_SPI_AF) |
                PAL_STM32_OSPEED_HIGHEST);
  palSetPadMode(SC_SPIRIT1_SPI_PORT,
                SC_SPIRIT1_SPI_MISO_PIN,
                PAL_MODE_ALTERNATE(SC_SPIRIT1_SPI_AF) |
                PAL_STM32_OSPEED_HIGHEST);
  palSetPadMode(SC_SPIRIT1_SPI_PORT,
                SC_SPIRIT1_SPI_MOSI_PIN,
                PAL_MODE_ALTERNATE(SC_SPIRIT1_SPI_AF) |
                PAL_STM32_OSPEED_HIGHEST);
  palSetPadMode(SC_SPIRIT1_SPI_CS_PORT,
                SC_SPIRIT1_SPI_CS_PIN,
                PAL_MODE_OUTPUT_PUSHPULL |
                PAL_STM32_OSPEED_HIGHEST);

  palSetPadMode(SC_SPIRIT1_SDN_PORT,
                SC_SPIRIT1_SDN_PIN,
                PAL_MODE_OUTPUT_PUSHPULL |
                PAL_STM32_OSPEED_HIGHEST);

  palSetPadMode(SC_SPIRIT1_INT_PORT,
                SC_SPIRIT1_INT_PIN,
                PAL_MODE_INPUT);

  palSetPad(SC_SPIRIT1_SPI_CS_PORT, SC_SPIRIT1_SPI_CS_PIN);

  // Speed 5.25MHz, CPHA=0, CPOL=0, 8bits frames, MSb transmitted first.
  // FIXME: is it really 5.25Mhz with all STM32s?
  spi_n = sc_spi_register(&SC_SPIRIT1_SPIN,
                          SC_SPIRIT1_SPI_CS_PORT,
                          SC_SPIRIT1_SPI_CS_PIN,
                          (SPI_CR1_BR_0 | SPI_CR1_BR_1));

  spin = (uint8_t)spi_n;

  sc_extint_set_isr_cb(SC_SPIRIT1_INT_PORT,
                       SC_SPIRIT1_INT_PIN,
                       SC_EXTINT_EDGE_FALLING,
                       spirit1_irq_cb);

  // Reset Spirit1
  sc_spirit1_poweroff();
  chThdSleepMilliseconds(1);
  sc_spirit1_poweron();
  chThdSleepMilliseconds(1);

  SpiritRadioSetXtalFrequency(XTAL_FREQUENCY);
  SpiritGeneralSetSpiritVersion(SPIRIT_VERSION);

  SpiritGpioInit(&gpio_conf);

  SpiritRadioInit(&radio_conf);

  SpiritRadioSetPALeveldBm(POWER_INDEX, POWER_DBM);
  SpiritRadioSetPALevelMaxIndex(POWER_INDEX);

  /* Spirit1 packet config */
  SpiritPktBasicInit(&packet_conf);
  SpiritPktBasicAddressesInit(&addr_conf);

  SpiritQiSetSqiThreshold(SQI_TH_0);
  SpiritQiSqiCheck(S_ENABLE);

  SpiritQiSetRssiThresholddBm(RSSI_THRESHOLD);

  /* Spirit IRQs enable */
  SpiritIrq(RX_DATA_READY, S_ENABLE);
  SpiritIrq(RX_TIMEOUT, S_ENABLE);
  SpiritIrq(TX_DATA_SENT, S_ENABLE);
  SpiritIrq(AES_END, S_ENABLE);

  spirit1_start_rx(0);

  sc_spirit1_irq_thread_ptr =
    chThdCreateStatic(sc_spirit1_irq_thread,
                      sizeof(sc_spirit1_irq_thread),
                      HIGHPRIO,
                      scSpirit1IrqThread,
                      NULL);

  sc_spirit1_act_thread_ptr =
    chThdCreateStatic(sc_spirit1_act_thread,
                      sizeof(sc_spirit1_act_thread),
                      NORMALPRIO,
                      scSpirit1ActThread,
                      NULL);
}



/*
 * Read a buffer from the radio to a local buffer
 */
uint8_t sc_spirit1_read(uint8_t *addr, uint8_t *buf, uint8_t len)
{
  (void)addr;
  (void)buf;
  (void)len;

  chMtxLock(&rx_buf.mtx);

  if (!rx_buf.busy || rx_buf.len == 0 ||
      !(rx_buf.state & SPIRIT1_BUF_STATE_PARSED)) {
    chMtxUnlock(&rx_buf.mtx);
    return 0;
  }

  // FIXME: now just silently ignoring the bytes not fitting to the buf
  if (len > rx_buf.len - SPIRIT1_MSG_IDX_PAYLOAD) {
    len = rx_buf.len - SPIRIT1_MSG_IDX_PAYLOAD;
  }

  for (uint8_t i = 0; i < len; ++i) {
    buf[i] = rx_buf.buf[SPIRIT1_MSG_IDX_PAYLOAD + i];
  }
  buf[len] = '\0';
  *addr = rx_buf.addr;

  rx_buf.busy  = 0;
  rx_buf.len   = 0;
  rx_buf.state = 0;

  chMtxUnlock(&rx_buf.mtx);

  return len;
}



/*
 * Return the latest link quality indicator.
 * The link quality indicator is a 4-bit value. Its value depends on
 * the noise power on the demodulated signal. The lower the value, the
 * noisier the signal.
 */
uint8_t sc_spirit1_lqi(void)
{
  return rx_buf.lqi;
}



/*
 * Return the latest received signal strength indicator.
 * The received signal strength indicator (RSSI) is a measurement of
 * the received signal power at the antenna measured in the channel
 * filter bandwidth.
 */
uint8_t sc_spirit1_rssi(void)
{
  return rx_buf.rssi;
}



/*
 * Copy buffer from application to a local buffer to be sent asynchronously
 */
void sc_spirit1_send(uint8_t addr, uint8_t *buf, uint8_t len)
{
  spirit1_send_flag(addr, buf, len, 0);
}



/*
 * Disable the driver and power off the radio
 */
void sc_spirit1_shutdown(void)
{
  uint8_t i = 20;

  chThdTerminate(sc_spirit1_act_thread_ptr);
  chThdTerminate(sc_spirit1_irq_thread_ptr);

  // Reset semaphores so that the threads can exit
  chBSemReset(&spirit1_irq_sem, TRUE);
  chBSemReset(&spirit1_act_sem, TRUE);

  // Wait up to 200 ms for the thread to exit
  while (i-- > 0 &&
         !chThdTerminatedX(sc_spirit1_act_thread_ptr) &&
         !chThdTerminatedX(sc_spirit1_irq_thread_ptr)) {
    chThdSleepMilliseconds(10);
  }

  chDbgAssert(chThdTerminatedX(sc_spirit1_act_thread_ptr), "Spirit1 act thread not yet terminated");
  chDbgAssert(chThdTerminatedX(sc_spirit1_irq_thread_ptr), "Spirit1 AES thread not yet terminated");

  sc_spirit1_act_thread_ptr = NULL;
  sc_spirit1_irq_thread_ptr = NULL;

  sc_spirit1_poweroff();

  sc_spi_unregister(spin);
}



/*
 * Power off the radio
 */
void sc_spirit1_poweroff(void)
{
  palSetPad(SC_SPIRIT1_SDN_PORT, SC_SPIRIT1_SDN_PIN);
}



/*
 * Power on the radio
 */
void sc_spirit1_poweron(void)
{
  palClearPad(SC_SPIRIT1_SDN_PORT, SC_SPIRIT1_SDN_PIN);
}



/*
 * Read radio registers over SPI. Used by the ST's Spirit1 code.
 */
SpiritStatus sc_spirit1_reg_read(uint8_t reg, uint8_t len, uint8_t* buf)
{
  uint8_t status[2];
  uint8_t header[2];

  /* Create READ header */
  header[0] = 0x01;
  header[1] = reg;

  sc_spi_select(spin);

  /* Write the header and read the SPIRIT1 status bytes */
  sc_spi_exchange(spin, header, status, sizeof(header));

  /* Read register values */
  sc_spi_receive(spin, buf, len);

  sc_spi_deselect(spin);

  return uint2status(status);
}



/*
 * Write radio registers over SPI. Used by the ST's Spirit1 code.
 */
SpiritStatus sc_spirit1_reg_write(uint8_t reg, uint8_t len, uint8_t* buf)
{
  uint8_t status[2];
  uint8_t header[2];

  /* Create WRITE header */
  header[0] = 0x00;
  header[1] = reg;

  sc_spi_select(spin);

  /* Write the header and read the SPIRIT1 status bytes */
  sc_spi_exchange(spin, header, status, sizeof(header));

  /* Write register values */
  sc_spi_send(spin, buf, len);

  sc_spi_deselect(spin);

  return uint2status(status);
}



/*
 * Send a radio command over SPI. Used by the ST's Spirit1 code.
 */
SpiritStatus sc_spirit1_cmd_strobe(uint8_t cmd)
{
  uint8_t status[2];
  uint8_t header[2];

  /* Create COMMAND header */
  header[0] = 0x80;
  header[1] = cmd;

  sc_spi_select(spin);

  /* Write the header and read the SPIRIT1 status bytes */
  sc_spi_exchange(spin, header, status, sizeof(header));

  sc_spi_deselect(spin);

  return uint2status(status);
}



/*
 * Read fifo from radio command over SPI. Used by the ST's Spirit1 code.
 */
SpiritStatus sc_spirit1_fifo_read(uint8_t len, uint8_t* buf)
{
  return sc_spirit1_reg_read(LINEAR_FIFO_ADDRESS, len, buf);
}



/*
 * Write fifo from radio command over SPI. Used by the ST's Spirit1 code.
 */
SpiritStatus sc_spirit1_fifo_write(uint8_t len, uint8_t* buf)
{
  return sc_spirit1_reg_write(LINEAR_FIFO_ADDRESS, len, buf);
}



#endif

/* Emacs indentatation information
   Local Variables:
   indent-tabs-mode:nil
   tab-width:2
   c-basic-offset:2
   End:
*/
