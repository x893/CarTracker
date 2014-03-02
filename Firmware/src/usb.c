#include "hardware.h"

#ifdef USB_IRQHandler

#include "usb_dcd_int.h"
extern USB_OTG_CORE_HANDLE	USB_OTG_dev;

void USB_IRQHandler(void)
{
	USBD_OTG_ISR_Handler(&USB_OTG_dev);
}
#endif
