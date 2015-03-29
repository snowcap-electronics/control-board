/*
  Modified for Snowcap projects
  Copyright (C) 2014 Tuomas Kulve <tuomas.kulve@snowcap.fi>
*/
/*
  USB-HID Gamepad for ChibiOS/RT
  Copyright (C) 2014, +inf Wenzheng Xu.

  EMAIL: wx330@nyu.edu

  This piece of code is FREE SOFTWARE and is released under the terms
  of the GNU General Public License <http://www.gnu.org/licenses/>
*/

/*
  ChibiOS/RT - Copyright (C) 2006,2007,2008,2009,2010,
  2011,2012 Giovanni Di Sirio.

  This file is part of ChibiOS/RT.

  ChibiOS/RT is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 3 of the License, or
  (at your option) any later version.

  ChibiOS/RT is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#define SC_LOG_MODULE_TAG SC_LOG_MODULE_UNSPECIFIED

/* Snowcap includes */
#include "sc_hid.h"
#include "sc_utils.h"

#if HAL_USE_USB

//#define HID_CONTROL_EP_ADDRESS	0	/* Implicit */
#define HID_IN_EP_ADDRESS 		1	/* Interrupt. Mandatory */
#define HID_OUT_EP_ADDRESS		2	/* Interrupt. Optional */

/* HID Class Specific Requests */
#define HID_GET_REPORT_REQUEST		0x01
#define HID_GET_IDLE_REQUEST		0x02
#define HID_GET_PROTOCOL_REQUEST	0x03
#define HID_SET_REPORT_REQUEST		0x09
#define HID_SET_IDLE_REQUEST		0x0A
#define HID_SET_PROTOCOL_REQUEST	0x0B
/* --------------------------- */

#define USB_DESCRIPTOR_HID 		0x21

#define HID_USAGE_PAGE(up) 		(uint8_t)(0x05 & 255),(uint8_t)(((up)) & 255)
#define HID_USAGE(u) 	   		(uint8_t)(0x09 & 255),(uint8_t)(((u)) & 255)
#define HID_COLLECTION(c)  		(uint8_t)(0xA1 & 255),(uint8_t)(((c)) & 255)
#define HID_END_COLLECTION		0xC0

#define HID_USAGE_MINIMUM(x)  		(uint8_t)(0x19 & 255),(uint8_t)(((x)) & 255)
#define HID_USAGE_MAXIMUM(x)  		(uint8_t)(0x29 & 255),(uint8_t)(((x)) & 255)
#define HID_LOGICAL_MINIMUM(x)  	(uint8_t)(0x15 & 255),(uint8_t)(((x)) & 255)
#define HID_LOGICAL_MAXIMUM(x)  	(uint8_t)(0x25 & 255),(uint8_t)(((x)) & 255)
#define HID_REPORT_COUNT(x)  		(uint8_t)(0x95 & 255),(uint8_t)(((x)) & 255)
#define HID_REPORT_SIZE(x)  		(uint8_t)(0x75 & 255),(uint8_t)(((x)) & 255)
#define HID_INPUT(x)  			(uint8_t)(0x81 & 255),(uint8_t)(((x)) & 255)

#define HID_COLLECTION_PHYSICAL		0x00
#define HID_COLLECTION_APPLICATION	0x01
#define HID_COLLECTION_LOGICAL		0x02
#define HID_COLLECTION_REPORT		0x03
#define	HID_COLLECTION_NAMED_ARRAY	0x04
#define HID_COLLECTION_USAGE_SWITCH	0x05
#define HID_COLLECTION_USAGE_MODIFIER	0x06

#define HID_USAGE_PAGE_GENERIC_DESKTOP  0x01
#define HID_USAGE_PAGE_SIMULATION	0x02
#define HID_USAGE_PAGE_VR		0x03
#define HID_USAGE_PAGE_SPORT		0x04
#define HID_USAGE_PAGE_GAME		0x05
#define HID_USAGE_PAGE_GENERIC_DEVICE	0x06
#define HID_USAGE_PAGE_KEYBOARD_KEYPAD	0x07
#define HID_USAGE_PAGE_LEDS		0x08
#define HID_USAGE_PAGE_BUTTON		0x09
#define HID_USAGE_PAGE_ORDINAL		0x0A
#define HID_USAGE_PAGE_TELEPHONY	0x0B
#define HID_USAGE_PAGE_CONSUMER		0x0C
#define HID_USAGE_PAGE_DIGITIZER	0x0D
#define HID_USAGE_PAGE_PID		0x0F
#define HID_USAGE_PAGE_UNICODE		0x10
#define HID_USAGE_ALPHANUMERIC_DISPLAY	0x14
#define HID_USAGE_MEDICAL_INSTRUMENTS	0x40
#define HID_USAGE_MONITOR_PAGE1		0x80
#define HID_USAGE_MONITOR_PAGE2		0x81
#define HID_USAGE_MONITOR_PAGE3		0x82
#define HID_USAGE_MONITOR_PAGE4		0x83
#define HID_USAGE_POWER_PAGE1		0x84
#define HID_USAGE_POWER_PAGE2		0x85
#define HID_USAGE_POWER_PAGE3		0x86
#define HID_USAGE_POWER_PAGE4		0x87
#define HID_USAGE_BAR_CODE_SCANNER_PAGE	0x8C
#define HID_USAGE_SCALE_PAGE		0x8D
#define HID_USAGE_MSR_PAGE		0x8E
#define HID_USAGE_CAMERA_PAGE		0x90
#define HID_USAGE_ARCADE_PAGE		0x91

#define HID_USAGE_POINTER		0x01
#define HID_USAGE_MOUSE			0x02
#define HID_USAGE_JOYSTICK		0x04
#define HID_USAGE_GAMEPAD		0x05
#define HID_USAGE_KEYBOARD		0x06
#define HID_USAGE_KEYPAD		0x07
#define HID_USAGE_MULTIAXIS_CONTROLLER  0x08

#define HID_USAGE_BUTTON1         0x01
#define HID_USAGE_BUTTON8         0x08

#define HID_USAGE_X			0x30
#define HID_USAGE_Y			0x31
#define HID_USAGE_Z			0x32
#define HID_USAGE_RX			0x33
#define HID_USAGE_RY			0x34
#define HID_USAGE_RZ			0x35
#define HID_USAGE_VX			0x40
#define HID_USAGE_VY			0x41
#define HID_USAGE_VZ			0x42
#define HID_USAGE_VBRX			0x43
#define HID_USAGE_VBRY			0x44
#define HID_USAGE_VBRZ			0x45
#define HID_USAGE_VNO			0x46

#define HID_INPUT_DATA_VAR_ABS	0x02
#define HID_INPUT_CNST_VAR_ABS	0x03
#define HID_INPUT_DATA_VAR_REL	0x06


#define USB_DESC_HID(bcdHID, bCountryCode, bNumDescriptors, bDescriptorType, wDescriptorLength) \
  USB_DESC_BYTE(9),                                                     \
    USB_DESC_BYTE(USB_DESCRIPTOR_HID),                                  \
    USB_DESC_BCD(bcdHID),                                               \
    USB_DESC_BYTE(bCountryCode),                                        \
    USB_DESC_BYTE(bNumDescriptors),                                     \
    USB_DESC_BYTE(bDescriptorType),                                     \
    USB_DESC_WORD(wDescriptorLength)

static uint8_t usbInitState;

static hid_data hid_in_data;

static void hidDataReceived(USBDriver *usbp, usbep_t ep);
static void hidDataTransmitted(USBDriver *usbp, usbep_t ep);


#define USB_DESCRIPTOR_REPORT 0x22
/*===========================================================================*/
/* USB related stuff.                                                        */
/*===========================================================================*/

/* For a better understanding look at http://www.beyondlogic.org/usbnutshell/usb1.shtml */
/* Reference: [DCDHID] USB - Device Class Definition for Human Interface Devices (HID)  */
/*
 * USB Device Descriptor.
 */

static const uint8_t hid_device_descriptor_data[] = {
  USB_DESC_DEVICE       (0x0110,        /* bcdUSB (1.1).                    						*/
                         0x00,          /* bDeviceClass (in interface).     						*/
                         0x00,          /* bDeviceSubClass.                 						*/
                         0x00,          /* bDeviceProtocol.                 						*/
                         0x40,          /* bMaxPacketSize. Maximum Packet Size for Zero Endpoint (8,16,32,64) 		*/
                         0x0483,        /* idVendor (ST). Assigned by USB.org 						*/
                         0x5740,        /* idProduct. Assigned by the manufacture 					*/
                         0x0100,        /* bcdDevice. Device Version Number assigned by the developer 			*/
                         0x00,             /* iManufacturer.                   						*/
                         0x00,             /* iProduct.                        						*/
                         0x00,             /* iSerialNumber.                   						*/
                         0x01)             /* bNumConfigurations. The system has only one configuration             	*/
};

/*
 * Device Descriptor wrapper.
 */

static const USBDescriptor hid_device_descriptor = {
  sizeof hid_device_descriptor_data,
  hid_device_descriptor_data
};


/*
 * USB Configuration Descriptor.
 */
static const uint8_t hid_generic_joystick_reporter_data[] ={
  HID_USAGE_PAGE	(HID_USAGE_PAGE_GENERIC_DESKTOP),
  HID_USAGE		(HID_USAGE_GAMEPAD),
  HID_COLLECTION		(HID_COLLECTION_APPLICATION),
  HID_COLLECTION		(HID_COLLECTION_PHYSICAL),
  HID_USAGE_PAGE	(HID_USAGE_PAGE_GENERIC_DESKTOP),
  HID_USAGE		(HID_USAGE_X),
  HID_USAGE		(HID_USAGE_Y),
  HID_LOGICAL_MINIMUM     (-127),
  HID_LOGICAL_MAXIMUM	    (127),
  HID_REPORT_SIZE		(8),
  HID_REPORT_COUNT	(2),
  HID_INPUT	(HID_INPUT_DATA_VAR_ABS),
#if SC_HID_USE_BUTTON
  HID_USAGE_PAGE(HID_USAGE_PAGE_BUTTON),
  HID_USAGE_MINIMUM(HID_USAGE_BUTTON1),
  HID_USAGE_MAXIMUM(HID_USAGE_BUTTON8),
  HID_LOGICAL_MINIMUM     (0),
  HID_LOGICAL_MAXIMUM	    (1),
  HID_REPORT_SIZE		(1),
  HID_REPORT_COUNT	(8),
  HID_INPUT	(HID_INPUT_DATA_VAR_ABS),
#endif
  HID_END_COLLECTION ,
  HID_END_COLLECTION,
};
static const USBDescriptor hid_generic_joystick_reporter = {
  sizeof hid_generic_joystick_reporter_data,
  hid_generic_joystick_reporter_data
};

static const uint8_t hid_configuration_descriptor_data[] = {
  /* Configuration Descriptor.*/
  USB_DESC_CONFIGURATION(0x29,          /* wTotalLength.                    */
                         0x01,          /* bNumInterfaces.                  */
                         0x01,          /* bConfigurationValue.             */
                         0x00,          /* iConfiguration.                  */
                         0xC0,          /* bmAttributes (self powered).     */
                         0x19),         /* bMaxPower (50mA).                */
  /* Interface Descriptor.*/
  USB_DESC_INTERFACE    (0x00,          /* bInterfaceNumber.                */
                         0x00,          /* bAlternateSetting.               */
                         0x02,          /* bNumEndpoints.                   */
                         0x03,          /* bInterfaceClass (Human Device Interface).   */
                         0x00,          /* bInterfaceSubClass  (DCDHID page 8)         */
                         0x00,          /* bInterfaceProtocol  (DCDHID page 9)         */
                         0x00),            /* iInterface.                                 */

  USB_DESC_HID		    (0x0111,	/* bcdHID 		*/
                       0x00,		/* bCountryCode 	*/
                       0x01,		/* bNumDescriptor 	*/
                       0x22,		/* bDescriptorType	*/
                       sizeof(hid_generic_joystick_reporter_data)),		/* Report Descriptor Lenght. Compute it! */
  /* Endpoint 1 Descriptor INTERRUPT IN  */
  USB_DESC_ENDPOINT     (0x81,   	/* bEndpointAddress.   (0x80 = Direction IN) + (0x01 = Address 1)     */
                         0x03,          /* bmAttributes (Interrupt).             		 	      */
                         0x04,          /* wMaxPacketSize.     0x40 = 64 BYTES                               */
                         0x04),         /* bInterval (Polling every 50ms)                                     */
  /* Endpoint 1 Descriptor INTERRUPT OUT */
  USB_DESC_ENDPOINT     (0x02,       	/* bEndpointAddress.   (0x00 = Direction OUT) + (0x0q = Address 1)    */
                         0x03,          /* bmAttributes (Interrupt).             */
                         0x04,          /* wMaxPacketSize.     0x40 = 64 BYTES  */
                         0x08),         /* bInterval (Polling every 50ms)        */
  /* HID Report Descriptor */
  /* Specific Class HID Descriptor */
};

/*
 * Configuration Descriptor wrapper.
 */
static const USBDescriptor hid_configuration_descriptor = {
  sizeof hid_configuration_descriptor_data,
  hid_configuration_descriptor_data
};



/*
 * U.S. English language identifier.
 */
static const uint8_t hid_string0[] = {
  USB_DESC_BYTE(4),                     /* bLength.                         */
  USB_DESC_BYTE(USB_DESCRIPTOR_STRING), /* bDescriptorType.                 */
  USB_DESC_WORD(0x0409)                 /* wLANGID (U.S. English).          */
};

/*
 * Vendor string.
 */
static const uint8_t hid_string1[] = {
  USB_DESC_BYTE(38),                    /* bLength.                         */
  USB_DESC_BYTE(USB_DESCRIPTOR_STRING), /* bDescriptorType.                 */
  'S', 0, 'T', 0, 'M', 0, 'i', 0, 'c', 0, 'r', 0, 'o', 0, 'e', 0,
  'l', 0, 'e', 0, 'c', 0, 't', 0, 'r', 0, 'o', 0, 'n', 0, 'i', 0,
  'c', 0, 's', 0
};

/*
 * Device Description string.
 */
static const uint8_t hid_string2[] = {
  USB_DESC_BYTE(38),                    /* bLength.                         */
  USB_DESC_BYTE(USB_DESCRIPTOR_STRING), /* bDescriptorType.                 */
  'C', 0, 'h', 0, 'i', 0, 'b', 0, 'i', 0, 'O', 0, 'S', 0, '/', 0,
  'R', 0, 'T', 0, ' ', 0, 'U', 0, 'S', 0, 'B', 0, '-', 0, 'H', 0,
  'I', 0, 'D', 0
};

/*
 * Serial Number string.
 */
static const uint8_t hid_string3[] = {
  USB_DESC_BYTE(8),                     /* bLength.                         */
  USB_DESC_BYTE(USB_DESCRIPTOR_STRING), /* bDescriptorType.                 */
  '0' + CH_KERNEL_MAJOR, 0,
  '0' + CH_KERNEL_MINOR, 0,
  '0' + CH_KERNEL_PATCH, 0
};

/*
 * Interface string.
 */


static const uint8_t hid_string4[] = {
  USB_DESC_BYTE(16),                    /* bLength.                             */
  USB_DESC_BYTE(USB_DESCRIPTOR_STRING), /* bDescriptorType.                     */
  'U', 0, 'S', 0, 'B', 0, ' ', 0, 'H', 0, 'I', 0, 'D', 0
};

/*
 * Strings wrappers array.
 */
static const USBDescriptor hid_strings[] = {
  {sizeof hid_string0, hid_string0},
  {sizeof hid_string1, hid_string1},
  {sizeof hid_string2, hid_string2},
  {sizeof hid_string3, hid_string3},
  {sizeof hid_string4, hid_string4}
};



static uint16_t get_hword(uint8_t *p)
{
  uint16_t hw;

  hw  = (uint16_t)*p++;
  hw |= (uint16_t)*p << 8U;
  return hw;
}

static bool hidRequestsHook(USBDriver *usbp)
{
  const USBDescriptor *dp;
  if ((usbp->setup[0] & (USB_RTYPE_TYPE_MASK | USB_RTYPE_RECIPIENT_MASK)) ==
      (USB_RTYPE_TYPE_STD | USB_RTYPE_RECIPIENT_INTERFACE)) {
    switch (usbp->setup[1]) {
    case USB_REQ_GET_DESCRIPTOR:
      dp = usbp->config->get_descriptor_cb(
                                           usbp, usbp->setup[3], usbp->setup[2],
                                           get_hword(&usbp->setup[4]));
      if (dp == NULL)
        return FALSE;
      usbSetupTransfer(usbp, (uint8_t *)dp->ud_string, dp->ud_size, NULL);
      return TRUE;
    default:
      return FALSE;
    }
  }

  if ((usbp->setup[0] & (USB_RTYPE_TYPE_MASK | USB_RTYPE_RECIPIENT_MASK)) ==
      (USB_RTYPE_TYPE_CLASS | USB_RTYPE_RECIPIENT_INTERFACE)) {
    switch (usbp->setup[1]) {
    case HID_GET_REPORT_REQUEST:
      //usbSetupTransfer(usbp, (uint8_t *)&linecoding, sizeof(linecoding), NULL);
      //palSetPadMode(GPIOD, 12, PAL_MODE_OUTPUT_PUSHPULL);
      //palSetPad(GPIOD, 12);
      hid_in_data.x=0;
      hid_in_data.y=0;
#if SC_HID_USE_BUTTON
      hid_in_data.button=0;
#endif
      usbSetupTransfer(usbp,(uint8_t *)&hid_in_data,sizeof(hid_in_data), NULL);
      usbInitState = 1;
      return TRUE;
    case HID_GET_IDLE_REQUEST:
      usbSetupTransfer(usbp,NULL,0, NULL);
      return TRUE;
    case HID_GET_PROTOCOL_REQUEST:
      return TRUE;
    case HID_SET_REPORT_REQUEST:
      usbSetupTransfer(usbp,NULL,0, NULL);
      return TRUE;
    case HID_SET_IDLE_REQUEST:
      usbSetupTransfer(usbp,NULL,0, NULL);
      return TRUE;
    case HID_SET_PROTOCOL_REQUEST:
      return TRUE;
    default:
      return FALSE;
    }
  }
  return FALSE;
}



static const USBDescriptor *get_descriptor(USBDriver *usbp, uint8_t dtype, uint8_t dindex, uint16_t lang) {

  (void)usbp;
  (void)lang;
  switch (dtype) {
  case USB_DESCRIPTOR_DEVICE:
    return &hid_device_descriptor;
  case USB_DESCRIPTOR_CONFIGURATION:
    return &hid_configuration_descriptor;
  case USB_DESCRIPTOR_REPORT:
    //palSetPadMode(GPIOD, 12, PAL_MODE_OUTPUT_PUSHPULL);
    //palSetPad(GPIOD, 12);
    return &hid_generic_joystick_reporter;
  case USB_DESCRIPTOR_STRING:
    if (dindex < 5)
      return &hid_strings[dindex];
  }
  return NULL;
}

static USBInEndpointState ep1instate;
//static USBOutEndpointState ep1outstate;
//	EndPoint Initialization. INTERRUPT IN. Device -> Host
static const USBEndpointConfig ep1config = {
  USB_EP_MODE_TYPE_INTR,
  NULL,
  hidDataTransmitted,
  NULL,
  0x0004,
  0x0000,
  &ep1instate,
  NULL,
  1,
  NULL
};

static USBOutEndpointState ep2outstate;
//static USBOutEndpointState ep1outstate;
//	EndPoint Initialization. INTERRUPT IN. Device -> Host
static const USBEndpointConfig ep2config = {
  USB_EP_MODE_TYPE_INTR,
  NULL,
  NULL,
  hidDataReceived,
  0x0000,
  0x0004,
  NULL,
  &ep2outstate,
  1,
  NULL
};

static void usb_event(USBDriver *usbp, usbevent_t event) {
  switch(event) {
  case USB_EVENT_RESET:
    return;
  case USB_EVENT_ADDRESS:
    return;
  case USB_EVENT_CONFIGURED:
    chSysLockFromISR();
    usbInitEndpointI(usbp, HID_IN_EP_ADDRESS, &ep1config);
    usbInitEndpointI(usbp, HID_OUT_EP_ADDRESS, &ep2config);
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


const USBConfig usbcfg = {
  usb_event,
  get_descriptor,
  hidRequestsHook,
  NULL
};

//-----------------------------


void hid_transmit(hid_data *data)
{
  if (!usbInitState) {
    return;
  }
  usbPrepareTransmit(&USBDX, HID_IN_EP_ADDRESS, (uint8_t *)data, sizeof(hid_data));
  chSysLock();
  usbStartTransmitI(&USBDX, HID_IN_EP_ADDRESS);
  //palSetPadMode(GPIOD, 14, PAL_MODE_OUTPUT_PUSHPULL);
  //palClearPad(GPIOD, 14);
  chSysUnlock();
}

// Callback for THE IN ENDPOINT (INTERRUPT). Device -> HOST
static void hidDataTransmitted(USBDriver *usbp, usbep_t ep){
  (void)usbp;
  (void)ep;
  //palSetPadMode(GPIOD, 14, PAL_MODE_OUTPUT_PUSHPULL);
  //palSetPad(GPIOD, 14);
  //hid_transmit(usbp,&hid_in_data);
}


// Callback for THE OUT ENDPOINT (INTERRUPT)
static void hidDataReceived(USBDriver *usbp, usbep_t ep){
  (void)usbp;
  (void)ep;
  //palSetPadMode(GPIOD, 15, PAL_MODE_OUTPUT_PUSHPULL);
  //palSetPad(GPIOD, 15);
}

void sc_hid_init(void)
{
  usbInitState = 0;
  // FIXME: use USBDX
  usbDisconnectBus(&USBDX);
  // FIXME: can't block here for this long
  chThdSleepMilliseconds(1000);
  usbStart(&USBDX, &usbcfg);
  usbConnectBus(&USBDX);
}

void sc_hid_deinit(void)
{
  usbInitState = 0;
}
#endif


/* Emacs indentatation information
   Local Variables:
   indent-tabs-mode:nil
   tab-width:2
   c-basic-offset:2
   End:
*/
