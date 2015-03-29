/***
 * Copyright 2012-2014 Tuomas Kulve, <tuomas.kulve@snowcap.fi>
 *                     Seppo Takalo
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

/* Mostly copied from ChibiOS/testhal/STM32F4xx/USB_CDC/main.c */

/* Snowcap includes */
#include "sc_sdu.h"
#include "sc_utils.h"
#include "sc_uart.h"
#include "sc_cmd.h"
#include "sc_led.h"
#include "sc_log.h"

#if HAL_USE_SERIAL_USB

/* ChibiOS includes */
//#include "usb_cdc.h"
#include "chprintf.h"

/* Max message size, should match the sc_uart.c's UART_MAX_SEND_BUF_LEN */
#ifndef SDU_MAX_SEND_BUF_LEN
#define SDU_MAX_SEND_BUF_LEN      (128 + 1)
#endif
#ifndef SDU_MAX_SEND_BUFFERS
#define SDU_MAX_SEND_BUFFERS      4
#endif

/*
 * Buffer for blocking send. First byte is the message length.
 */
static uint8_t send_buf[SDU_MAX_SEND_BUFFERS][SDU_MAX_SEND_BUF_LEN];
static mutex_t buf_mtx;
static semaphore_t send_sem;
static uint8_t first_free = 0;
static uint8_t previous_full = SDU_MAX_SEND_BUFFERS - 1;

static thread_t *sdu_send_thread = NULL;
static thread_t *sdu_read_thread = NULL;
static void scSduReadThread(void *UNUSED(arg));
static void scSduSendThread(void *UNUSED(arg));

/*
 * Endpoints to be used for USBDX.
 */
#define USBDX_DATA_REQUEST_EP           1
#define USBDX_DATA_AVAILABLE_EP         1
#define USBDX_INTERRUPT_REQUEST_EP      2

/*
 * Serial over USB Driver structure.
 */
static SerialUSBDriver SDUX;

/*
 * USB Device Descriptor.
 */
static const uint8_t vcom_device_descriptor_data[18] = {
  USB_DESC_DEVICE       (0x0110,        /* bcdUSB (1.1).                    */
                         0x02,          /* bDeviceClass (CDC).              */
                         0x00,          /* bDeviceSubClass.                 */
                         0x00,          /* bDeviceProtocol.                 */
                         0x40,          /* bMaxPacketSize.                  */
                         0x0483,        /* idVendor (ST).                   */
                         0x5740,        /* idProduct.                       */
                         0x0200,        /* bcdDevice.                       */
                         1,             /* iManufacturer.                   */
                         2,             /* iProduct.                        */
                         3,             /* iSerialNumber.                   */
                         1)             /* bNumConfigurations.              */
};

/*
 * Device Descriptor wrapper.
 */
static const USBDescriptor vcom_device_descriptor = {
  sizeof vcom_device_descriptor_data,
  vcom_device_descriptor_data
};

/* Configuration Descriptor tree for a CDC.*/
static const uint8_t vcom_configuration_descriptor_data[67] = {
  /* Configuration Descriptor.*/
  USB_DESC_CONFIGURATION(67,            /* wTotalLength.                    */
                         0x02,          /* bNumInterfaces.                  */
                         0x01,          /* bConfigurationValue.             */
                         0,             /* iConfiguration.                  */
                         0xC0,          /* bmAttributes (self powered).     */
                         50),           /* bMaxPower (100mA).               */
  /* Interface Descriptor.*/
  USB_DESC_INTERFACE    (0x00,          /* bInterfaceNumber.                */
                         0x00,          /* bAlternateSetting.               */
                         0x01,          /* bNumEndpoints.                   */
                         0x02,          /* bInterfaceClass (Communications
                                           Interface Class, CDC section
                                           4.2).                            */
                         0x02,          /* bInterfaceSubClass (Abstract
                                           Control Model, CDC section 4.3).   */
                         0x01,          /* bInterfaceProtocol (AT commands,
                                           CDC section 4.4).                */
                         0),            /* iInterface.                      */
  /* Header Functional Descriptor (CDC section 5.2.3).*/
  USB_DESC_BYTE         (5),            /* bLength.                         */
  USB_DESC_BYTE         (0x24),         /* bDescriptorType (CS_INTERFACE).  */
  USB_DESC_BYTE         (0x00),         /* bDescriptorSubtype (Header
                                           Functional Descriptor.           */
  USB_DESC_BCD          (0x0110),       /* bcdCDC.                          */
  /* Call Management Functional Descriptor. */
  USB_DESC_BYTE         (5),            /* bFunctionLength.                 */
  USB_DESC_BYTE         (0x24),         /* bDescriptorType (CS_INTERFACE).  */
  USB_DESC_BYTE         (0x01),         /* bDescriptorSubtype (Call Management
                                           Functional Descriptor).          */
  USB_DESC_BYTE         (0x00),         /* bmCapabilities (D0+D1).          */
  USB_DESC_BYTE         (0x01),         /* bDataInterface.                  */
  /* ACM Functional Descriptor.*/
  USB_DESC_BYTE         (4),            /* bFunctionLength.                 */
  USB_DESC_BYTE         (0x24),         /* bDescriptorType (CS_INTERFACE).  */
  USB_DESC_BYTE         (0x02),         /* bDescriptorSubtype (Abstract
                                           Control Management Descriptor).  */
  USB_DESC_BYTE         (0x02),         /* bmCapabilities.                  */
  /* Union Functional Descriptor.*/
  USB_DESC_BYTE         (5),            /* bFunctionLength.                 */
  USB_DESC_BYTE         (0x24),         /* bDescriptorType (CS_INTERFACE).  */
  USB_DESC_BYTE         (0x06),         /* bDescriptorSubtype (Union
                                           Functional Descriptor).          */
  USB_DESC_BYTE         (0x00),         /* bMasterInterface (Communication
                                           Class Interface).                */
  USB_DESC_BYTE         (0x01),         /* bSlaveInterface0 (Data Class
                                           Interface).                      */
  /* Endpoint 2 Descriptor.*/
  USB_DESC_ENDPOINT     (USBDX_INTERRUPT_REQUEST_EP|0x80,
                         0x03,          /* bmAttributes (Interrupt).        */
                         0x0008,        /* wMaxPacketSize.                  */
                         0xFF),         /* bInterval.                       */
  /* Interface Descriptor.*/
  USB_DESC_INTERFACE    (0x01,          /* bInterfaceNumber.                */
                         0x00,          /* bAlternateSetting.               */
                         0x02,          /* bNumEndpoints.                   */
                         0x0A,          /* bInterfaceClass (Data Class
                                           Interface, CDC section 4.5).     */
                         0x00,          /* bInterfaceSubClass (CDC section
                                           4.6).                            */
                         0x00,          /* bInterfaceProtocol (CDC section
                                           4.7).                            */
                         0x00),         /* iInterface.                      */
  /* Endpoint 3 Descriptor.*/
  USB_DESC_ENDPOINT     (USBDX_DATA_AVAILABLE_EP,       /* bEndpointAddress.*/
                         0x02,          /* bmAttributes (Bulk).             */
                         0x0040,        /* wMaxPacketSize.                  */
                         0x00),         /* bInterval.                       */
  /* Endpoint 1 Descriptor.*/
  USB_DESC_ENDPOINT     (USBDX_DATA_REQUEST_EP|0x80,    /* bEndpointAddress.*/
                         0x02,          /* bmAttributes (Bulk).             */
                         0x0040,        /* wMaxPacketSize.                  */
                         0x00)          /* bInterval.                       */
};

/*
 * Configuration Descriptor wrapper.
 */
static const USBDescriptor vcom_configuration_descriptor = {
  sizeof vcom_configuration_descriptor_data,
  vcom_configuration_descriptor_data
};

/*
 * U.S. English language identifier.
 */
static const uint8_t vcom_string0[] = {
  USB_DESC_BYTE(4),                     /* bLength.                         */
  USB_DESC_BYTE(USB_DESCRIPTOR_STRING), /* bDescriptorType.                 */
  USB_DESC_WORD(0x0409)                 /* wLANGID (U.S. English).          */
};

/*
 * Vendor string.
 */
static const uint8_t vcom_string1[] = {
  USB_DESC_BYTE(38),                    /* bLength.                         */
  USB_DESC_BYTE(USB_DESCRIPTOR_STRING), /* bDescriptorType.                 */
  'S', 0, 'T', 0, 'M', 0, 'i', 0, 'c', 0, 'r', 0, 'o', 0, 'e', 0,
  'l', 0, 'e', 0, 'c', 0, 't', 0, 'r', 0, 'o', 0, 'n', 0, 'i', 0,
  'c', 0, 's', 0
};

/*
 * Device Description string.
 */
static const uint8_t vcom_string2[] = {
  USB_DESC_BYTE(56),                    /* bLength.                         */
  USB_DESC_BYTE(USB_DESCRIPTOR_STRING), /* bDescriptorType.                 */
  'C', 0, 'h', 0, 'i', 0, 'b', 0, 'i', 0, 'O', 0, 'S', 0, '/', 0,
  'R', 0, 'T', 0, ' ', 0, 'V', 0, 'i', 0, 'r', 0, 't', 0, 'u', 0,
  'a', 0, 'l', 0, ' ', 0, 'C', 0, 'O', 0, 'M', 0, ' ', 0, 'P', 0,
  'o', 0, 'r', 0, 't', 0
};

/*
 * Serial Number string.
 */
static const uint8_t vcom_string3[] = {
  USB_DESC_BYTE(8),                     /* bLength.                         */
  USB_DESC_BYTE(USB_DESCRIPTOR_STRING), /* bDescriptorType.                 */
  '0' + CH_KERNEL_MAJOR, 0,
  '0' + CH_KERNEL_MINOR, 0,
  '0' + CH_KERNEL_PATCH, 0
};

/*
 * Strings wrappers array.
 */
static const USBDescriptor vcom_strings[] = {
  {sizeof vcom_string0, vcom_string0},
  {sizeof vcom_string1, vcom_string1},
  {sizeof vcom_string2, vcom_string2},
  {sizeof vcom_string3, vcom_string3}
};

/*
 * Handles the GET_DESCRIPTOR callback. All required descriptors must be
 * handled here.
 */
static const USBDescriptor *get_descriptor(USBDriver *usbp,
                                           uint8_t dtype,
                                           uint8_t dindex,
                                           uint16_t lang) {

  (void)usbp;
  (void)lang;
  switch (dtype) {
  case USB_DESCRIPTOR_DEVICE:
    return &vcom_device_descriptor;
  case USB_DESCRIPTOR_CONFIGURATION:
    return &vcom_configuration_descriptor;
  case USB_DESCRIPTOR_STRING:
    if (dindex < 4)
      return &vcom_strings[dindex];
  }
  return NULL;
}

/**
 * @brief   IN EP1 state.
 */
static USBInEndpointState ep1instate;

/**
 * @brief   OUT EP1 state.
 */
static USBOutEndpointState ep1outstate;

/**
 * @brief   EP1 initialization structure (both IN and OUT).
 */
static const USBEndpointConfig ep1config = {
  USB_EP_MODE_TYPE_BULK,
  NULL,
  sduDataTransmitted,
  sduDataReceived,
  0x0040,
  0x0040,
  &ep1instate,
  &ep1outstate,
  2,
  NULL
};

/**
 * @brief   IN EP2 state.
 */
static USBInEndpointState ep2instate;

/**
 * @brief   EP2 initialization structure (IN only).
 */
static const USBEndpointConfig ep2config = {
  USB_EP_MODE_TYPE_INTR,
  NULL,
  sduInterruptTransmitted,
  NULL,
  0x0010,
  0x0000,
  &ep2instate,
  NULL,
  1,
  NULL
};

/*
 * Handles the USB driver global events.
 */
static void usb_event(USBDriver *usbp, usbevent_t event) {

  switch (event) {
  case USB_EVENT_RESET:
    return;
  case USB_EVENT_ADDRESS:
    return;
  case USB_EVENT_CONFIGURED:
    chSysLockFromISR();

    /* Enables the endpoints specified into the configuration.
       Note, this callback is invoked from an ISR so I-Class functions
       must be used.*/
    usbInitEndpointI(usbp, USBDX_DATA_REQUEST_EP, &ep1config);
    usbInitEndpointI(usbp, USBDX_INTERRUPT_REQUEST_EP, &ep2config);

    /* Resetting the state of the CDC subsystem.*/
    sduConfigureHookI(&SDUX);

    chSysUnlockFromISR();
    return;
  case USB_EVENT_SUSPEND:
    return;
  case USB_EVENT_WAKEUP:
    return;
  case USB_EVENT_STALLED:
    return;
  }
  return;
}

/*
 * USB driver configuration.
 */
static const USBConfig usbcfg = {
  usb_event,
  get_descriptor,
  sduRequestsHook,
  NULL
};

/*
 * Serial over USB driver configuration.
 */
static const SerialUSBConfig serusbcfg = {
  &USBDX,
  USBDX_DATA_REQUEST_EP,
  USBDX_DATA_AVAILABLE_EP,
  USBDX_INTERRUPT_REQUEST_EP
};

/*
 * Setup a working area with a stack for reading SDU messages
 */
static THD_WORKING_AREA(sc_sdu_read_thread, 256);
THD_FUNCTION(scSduReadThread, arg)
{
  int retval;
  (void) arg;

  chRegSetThreadName(__func__);

  usbDisconnectBus(serusbcfg.usbp);

#ifdef SC_FORCE_USB_REDETECT
  // Don't force on wake-up to avoid confusing the PC on frequent wake-ups
  if (!(PWR->CSR & PWR_CSR_WUF)) {
    /*
     * Force HOST re-detect device after e.g. DFU flashing by
     * pulling D+ low so host assumes that device is disconnected.
     */
    palSetPadMode(SC_USB_DP_PORT, SC_USB_DP_PIN, PAL_MODE_OUTPUT_PUSHPULL);
    palClearPad(SC_USB_DP_PORT, SC_USB_DP_PIN);
    chThdSleepMilliseconds(1000);
    palSetPadMode(SC_USB_DP_PORT, SC_USB_DP_PIN, PAL_MODE_ALTERNATE(SC_USB_DP_AF));
  }
#endif

  /*
   * Activates the USB driver and then the USB bus pull-up on D+.
   * Note, a delay is inserted in order to not have to disconnect the cable
   * after a reset.
   */
  // FIXME: would less than 1000ms be enough? While loop?
  chThdSleepMilliseconds(1000);
  usbStart(serusbcfg.usbp, &usbcfg);
  usbConnectBus(serusbcfg.usbp);

  // Wait for USB active
  while (TRUE) {
    if (SDUX.config->usbp->state == USB_ACTIVE)
      break;
    chThdSleepMilliseconds(10);
  }

  // Loop forever reading characters
  while (!chThdShouldTerminateX()) {

    retval = chnGetTimeout((BaseChannel *)&SDUX, 100/*TIME_IMMEDIATE*/);
    if (retval >= 0) {
      sc_uart_revc_usb_byte((uint8_t)retval);
    }
  }
}

/*
 * Setup a working area with a stack for sending message
 */
static THD_WORKING_AREA(sc_sdu_send_thread, 512);
THD_FUNCTION(scSduSendThread, arg)
{
  chRegSetThreadName(__func__);

  (void)arg;

  while (!chThdShouldTerminateX()) {

    // Wait for data to be send
    chSemWait(&send_sem);

    if (chThdShouldTerminateX()) {
      break;
    }

    // Lock send buffer
    chMtxLock(&buf_mtx);

    ++previous_full;
    if (previous_full == SDU_MAX_SEND_BUFFERS) {
      previous_full = 0;
    }

    // Unlock send buffer
    chMtxUnlock(&buf_mtx);

    // First byte of the buffer indicates the buffer length
    chSequentialStreamWrite((BaseSequentialStream *)&SDUX,
                            &send_buf[previous_full][1],
                            send_buf[previous_full][0]);
  }
}




int sc_sdu_send_msg(const uint8_t *msg, int len)
{
  int i;
  int bytes_done = 0;
  uint8_t msgs_sent = 0;

  // Lock send buffer
  chMtxLock(&buf_mtx);

  while (bytes_done < len) {
    int bytes_to_send;

    if ((len - bytes_done) < (SDU_MAX_SEND_BUF_LEN - 1)) {
      bytes_to_send = len - bytes_done;
    } else {
      bytes_to_send = SDU_MAX_SEND_BUF_LEN - 1;
    }

    // Check if there's space in the buffer
    if (first_free == previous_full) {
      // No space, lose data
      chMtxUnlock(&buf_mtx);

      // XXX: There's a multisecond delay in starting the USB. Also the
      // buffers seem to get full if nobody is reading the data.
      // So not asserting here.
      // chDbgAssert(0, "SDU send buffer full");
      return 1;
    }

    // Store data
    for (i = 0; i < bytes_to_send; ++i) {
      send_buf[first_free][i + 1] = msg[i + bytes_done];
    }

    // First byte indicates the buffer length
    send_buf[first_free][0] = bytes_to_send;

    ++first_free;
    if (first_free == SDU_MAX_SEND_BUFFERS) {
      first_free = 0;
    }
    bytes_done += bytes_to_send;

    msgs_sent++;
  }

  // Unlock send buffer
  chMtxUnlock(&buf_mtx);

  for (i = 0; i < msgs_sent; ++i) {
    // Inform the sending thread that there is data to send
    chSemSignal(&send_sem);
  }

  return 0;
}


void sc_sdu_init(void)
{
  // Initialize sending related semaphores and variables
  chMtxObjectInit(&buf_mtx);
  chSemObjectInit(&send_sem, 0);

  // Initialize USB
  sduObjectInit(&SDUX);
  sduStart(&SDUX, &serusbcfg);

  chDbgAssert(sdu_read_thread == NULL, "SDU read thread already running");
  // Start a thread dedicated USB activating and reading messages
  sdu_read_thread = chThdCreateStatic(sc_sdu_read_thread,
                                      sizeof(sc_sdu_read_thread),
                                      NORMALPRIO, scSduReadThread, NULL);

  chDbgAssert(sdu_send_thread == NULL, "SDU send thread already running");
  // Start a thread dedicated to sending messages
  sdu_send_thread = chThdCreateStatic(sc_sdu_send_thread,
                                      sizeof(sc_sdu_send_thread),
                                      NORMALPRIO, scSduSendThread, NULL);
}


void sc_sdu_deinit(void)
{

  chDbgAssert(sdu_send_thread != NULL, "SDU send thread not running");
  chThdTerminate(sdu_send_thread);
  chSemSignal(&send_sem);
  chThdWait(sdu_send_thread);
  sdu_send_thread = NULL;

  chDbgAssert(sdu_read_thread != NULL, "SDU read thread not running");
  chThdTerminate(sdu_read_thread);
  chThdWait(sdu_read_thread);
  sdu_read_thread = NULL;

  sduStop(&SDUX);
  usbDisconnectBus(serusbcfg.usbp);

  // Deinitialize USB
  usbStop(serusbcfg.usbp);
}


#endif // HAL_USE_SERIAL_USB

/* Emacs indentatation information
   Local Variables:
   indent-tabs-mode:nil
   tab-width:2
   c-basic-offset:2
   End:
*/
