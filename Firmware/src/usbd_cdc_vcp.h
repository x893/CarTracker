#ifndef __USBD_CDC_VCP_H
#define __USBD_CDC_VCP_H

#include "stm32fxxx.h"

#include "usbd_cdc_core.h"
#include "usbd_conf.h"

#define CONTROL_LINE_DTR		(0x01)
#define CONTROL_LINE_RTS		(0x02)
#define USB_INITIALIZED			(0x80)

/* The following structures groups all needed parameters to be configured for the 
   ComPort. These parameters can modified on the fly by the host through CDC class
   command class requests. */
typedef struct
{
	uint32_t bitrate;
	uint8_t  format;
	uint8_t  paritytype;
	uint8_t  datatype;
} LINE_CODING;

uint8_t VCP_Initialize(void);
uint8_t VCP_Line_RTS(void);
uint8_t VCP_Line_DTR(void);
uint8_t VCP_Available(void);
uint8_t VCP_Read(void);

USBD_Status VCP_Write(uint8_t data);
USBD_Status VCP_Print(const char* text);
USBD_Status VCP_Println(const char* text);

#endif /* __USBD_CDC_VCP_H */
