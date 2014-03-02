#ifndef __USB_H__
#define __USB_H__

#include "hardware.h"

#if ( USE_USB )

#include "usbd_core.h"
#include "usbd_desc.h"
#include "usbd_cdc_core.h"
#include "usbd_usr.h"
#include "usbd_cdc_vcp.h"
void USBD_USR_DeInit(USB_OTG_CORE_HANDLE *pdev);

#endif

#endif
