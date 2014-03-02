#include "hardware.h"

#ifdef USB_IRQHandler

#include "usbd_usr.h"
#include "usbd_ioreq.h"
#include "usbd_cdc_vcp.h"

const USBD_Usr_cb_TypeDef USR_cb =
{
	USBD_USR_Init,
	USBD_USR_DeviceReset,
	USBD_USR_DeviceConfigured,
	USBD_USR_DeviceSuspended,
	USBD_USR_DeviceResumed,

	USBD_USR_DeviceConnected,
	USBD_USR_DeviceDisconnected
};

extern USB_OTG_CORE_HANDLE USB_OTG_dev;
extern uint8_t cdcStatus;

/**
* @brief  USBD_USR_Init 
*         Displays the message on LCD for host lib initialization
* @param  None
* @retval None
*/
void USBD_USR_Init(void)
{
#if defined( STM32F2XX ) || defined( STM32F4XX )
	DCD_DevConnect(&USB_OTG_dev);
#else
	USB_DISC_OFF();
#endif

	cdcStatus &= ~(CONTROL_LINE_DTR | CONTROL_LINE_RTS);
	cdcStatus |= USB_INITIALIZED;
}

void USBD_USR_DeInit(USB_OTG_CORE_HANDLE *pdev)
{
#if defined( STM32F2XX ) || defined( STM32F4XX )
	DCD_DevDisconnect(&USB_OTG_dev);
#else
	USB_DISC_ON();
#endif
	cdcStatus &= ~(CONTROL_LINE_DTR | CONTROL_LINE_RTS | USB_INITIALIZED);
}

/**
* @brief  USBD_USR_DeviceReset 
*         Displays the message on LCD on device Reset Event
* @param  speed : device speed
* @retval None
*/
void USBD_USR_DeviceReset(uint8_t speed )
{
}

/**
* @brief  USBD_USR_DeviceConfigured
*         Displays the message on LCD on device configuration Event
* @param  None
* @retval Staus
*/
void USBD_USR_DeviceConfigured (void)
{
}

/**
* @brief  USBD_USR_DeviceSuspended 
*         Displays the message on LCD on device suspend Event
* @param  None
* @retval None
*/
void USBD_USR_DeviceSuspended(void)
{
}


/**
* @brief  USBD_USR_DeviceResumed 
*         Displays the message on LCD on device resume Event
* @param  None
* @retval None
*/
void USBD_USR_DeviceResumed(void)
{
}


/**
* @brief  USBD_USR_DeviceConnected
*         Displays the message on LCD on device connection Event
* @param  None
* @retval Staus
*/
void USBD_USR_DeviceConnected (void)
{
}


/**
* @brief  USBD_USR_DeviceDisonnected
*         Displays the message on LCD on device disconnection Event
* @param  None
* @retval Staus
*/
void USBD_USR_DeviceDisconnected (void)
{
}

#endif
