#include "usbd_core.h"
#include "usbd_cdc_core.h"
#include "usbd_desc.h"
#include "usbd_req.h"
#include "usbd_conf.h"
#include "usb_regs.h"

#define USBD_VID						0x0480
#define USBD_PID						0x5740

#define USBD_LANGID_STRING				0x409
#define USBD_MANUFACTURER_STRING		"FleetWeb"

#define USBD_PRODUCT_FS_STRING			"FWTracker"
#define USBD_SERIALNUMBER_FS_STRING		"00000000050C"

#define USBD_CONFIGURATION_FS_STRING	"VCP Config"
#define USBD_INTERFACE_FS_STRING		"VCP Interface"

const USBD_DEVICE USR_desc =
{
	USBD_USR_DeviceDescriptor,
	USBD_USR_LangIDStrDescriptor, 
	USBD_USR_ManufacturerStrDescriptor,
	USBD_USR_ProductStrDescriptor,
	USBD_USR_SerialStrDescriptor,
	USBD_USR_ConfigStrDescriptor,
	USBD_USR_InterfaceStrDescriptor,
};

/* USB Standard Device Descriptor */
uint8_t USBD_DeviceDesc[USB_SIZ_DEVICE_DESC] =
{
	0x12,						/*bLength */
	USB_DEVICE_DESCRIPTOR_TYPE,	/*bDescriptorType*/
	0x00,						/*bcdUSB */
	0x02,
	DEVICE_CLASS_CDC,			/*bDeviceClass*/
	DEVICE_SUBCLASS_CDC,		/*bDeviceSubClass*/
	0x00,						/*bDeviceProtocol*/
	USB_OTG_MAX_EP0_SIZE,		/*bMaxPacketSize*/
	LOBYTE(USBD_VID),           /*idVendor*/
	HIBYTE(USBD_VID),           /*idVendor*/
	LOBYTE(USBD_PID),           /*idVendor*/
	HIBYTE(USBD_PID),           /*idVendor*/
	0x00,                       /*bcdDevice rel. 2.00*/
	0x02,
	USBD_IDX_MFC_STR,           /*Index of manufacturer  string*/
	USBD_IDX_PRODUCT_STR,       /*Index of product string*/
	USBD_IDX_SERIAL_STR,        /*Index of serial number string*/
	USBD_CFG_MAX_NUM            /*bNumConfigurations*/
};

/* USB Standard Device Descriptor */
const uint8_t USBD_DeviceQualifierDesc[USB_LEN_DEV_QUALIFIER_DESC] =
{
	USB_LEN_DEV_QUALIFIER_DESC,
	USB_DESC_TYPE_DEVICE_QUALIFIER,
	0x00,
	0x02,
	0x00,
	0x00,
	0x00,
	0x40,
	0x01,
	0x00,
};

/* USB Standard Device Descriptor */
uint8_t USBD_LangIDDesc[USB_SIZ_STRING_LANGID] =
{
	USB_SIZ_STRING_LANGID,         
	USB_DESC_TYPE_STRING,       
	LOBYTE(USBD_LANGID_STRING),
	HIBYTE(USBD_LANGID_STRING), 
};

/**
* @brief  USBD_USR_DeviceDescriptor 
*         return the device descriptor
* @param  speed : current device speed
* @param  length : pointer to data length variable
* @retval pointer to descriptor buffer
*/
uint8_t * USBD_USR_DeviceDescriptor(uint8_t speed, uint16_t *length)
{
	*length = sizeof(USBD_DeviceDesc);
	return (uint8_t *)USBD_DeviceDesc;
}

/**
* @brief  USBD_USR_LangIDStrDescriptor 
*         return the LangID string descriptor
* @param  speed : current device speed
* @param  length : pointer to data length variable
* @retval pointer to descriptor buffer
*/
uint8_t * USBD_USR_LangIDStrDescriptor( uint8_t speed, uint16_t *length )
{
	*length =  sizeof(USBD_LangIDDesc);
	return USBD_LangIDDesc;
}

/**
* @brief  USBD_USR_ProductStrDescriptor 
*         return the product string descriptor
* @param  speed : current device speed
* @param  length : pointer to data length variable
* @retval pointer to descriptor buffer
*/
uint8_t *  USBD_USR_ProductStrDescriptor( uint8_t speed , uint16_t *length )
{
	USBD_GetString(USBD_PRODUCT_FS_STRING, USBD_StrDesc, length);
	return USBD_StrDesc;
}

/**
* @brief  USBD_USR_ManufacturerStrDescriptor 
*         return the manufacturer string descriptor
* @param  speed : current device speed
* @param  length : pointer to data length variable
* @retval pointer to descriptor buffer
*/
uint8_t * USBD_USR_ManufacturerStrDescriptor( uint8_t speed , uint16_t *length )
{
	USBD_GetString (USBD_MANUFACTURER_STRING, USBD_StrDesc, length);
	return USBD_StrDesc;
}

/**
* @brief  USBD_USR_SerialStrDescriptor 
*         return the serial number string descriptor
* @param  speed : current device speed
* @param  length : pointer to data length variable
* @retval pointer to descriptor buffer
*/
uint8_t * USBD_USR_SerialStrDescriptor( uint8_t speed , uint16_t *length )
{
	USBD_GetString (USBD_SERIALNUMBER_FS_STRING, USBD_StrDesc, length);    
	return USBD_StrDesc;
}

/**
* @brief  USBD_USR_ConfigStrDescriptor 
*         return the configuration string descriptor
* @param  speed : current device speed
* @param  length : pointer to data length variable
* @retval pointer to descriptor buffer
*/
uint8_t * USBD_USR_ConfigStrDescriptor( uint8_t speed , uint16_t *length )
{
	USBD_GetString (USBD_CONFIGURATION_FS_STRING, USBD_StrDesc, length); 
	return USBD_StrDesc;
}


/**
* @brief  USBD_USR_InterfaceStrDescriptor 
*         return the interface string descriptor
* @param  speed : current device speed
* @param  length : pointer to data length variable
* @retval pointer to descriptor buffer
*/
uint8_t * USBD_USR_InterfaceStrDescriptor( uint8_t speed , uint16_t *length )
{
	USBD_GetString (USBD_INTERFACE_FS_STRING, USBD_StrDesc, length);
	return USBD_StrDesc;  
}
