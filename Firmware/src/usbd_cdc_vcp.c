#include "usbd_cdc_vcp.h"
#include "usb_bsp.h"
#include "usb_conf.h"

LINE_CODING linecoding =
{
	115200, /* baud rate*/
	0x00,   /* stop bits-1*/
	0x00,   /* parity - none*/
	0x08    /* nb. of bits 8*/
};

extern uint8_t  APP_Rx_Buffer []; /* Write CDC received data in this buffer.
                                     These data will be sent over USB IN endpoint
                                     in the CDC core functions. */
extern uint32_t APP_Rx_ptr_out;
extern uint32_t APP_Rx_ptr_in;	/*	Increment this pointer or roll it back to
									start address when writing received data
									in the buffer APP_Rx_Buffer.
								*/
uint8_t APP_Tx_Buffer   [APP_TX_DATA_SIZE];

uint16_t APP_Tx_ptr_in  = 0;
uint16_t APP_Tx_ptr_out = 0;

uint8_t  cdcStatus = 0;

uint16_t VCP_Init     (void);
uint16_t VCP_DeInit   (void);
uint16_t VCP_Ctrl     (uint32_t Cmd, uint8_t* Buf, uint32_t Len);
uint16_t VCP_DataRx   (uint8_t* Buf, uint32_t Len);

const CDC_IF_Prop_TypeDef VCP_fops = 
{
	VCP_Init,
	VCP_DeInit,
	VCP_Ctrl,
	NULL,
	VCP_DataRx
};

/* Private functions ---------------------------------------------------------*/
/**
  * @brief  VCP_Init
  *         Initializes the Media on the STM32
  * @param  None
  * @retval Result of the opeartion (USBD_OK in all cases)
  */
uint16_t VCP_Init(void)
{
	return USBD_OK;
}

/**
  * @brief  VCP_DeInit
  *         DeInitializes the Media on the STM32
  * @param  None
  * @retval Result of the opeartion (USBD_OK in all cases)
  */
uint16_t VCP_DeInit(void)
{
	return USBD_OK;
}

/**
  * @brief  VCP_Ctrl
  *         Manage the CDC class requests
  * @param  Cmd: Command code            
  * @param  Buf: Buffer containing command data (request parameters)
  * @param  Len: Number of data to be sent (in bytes)
  * @retval Result of the opeartion (USBD_OK in all cases)
  */
uint16_t VCP_Ctrl (uint32_t Cmd, uint8_t* Buf, uint32_t Len)
{ 
	switch (Cmd)
	{
	case SEND_ENCAPSULATED_COMMAND:
	case GET_ENCAPSULATED_RESPONSE:
	case SET_COMM_FEATURE:
	case GET_COMM_FEATURE:
	case CLEAR_COMM_FEATURE:
	case SEND_BREAK:
		/* Not  needed for this driver */
		break;

	case SET_LINE_CODING:
		linecoding.bitrate = (uint32_t)(Buf[0] | (Buf[1] << 8) | (Buf[2] << 16) | (Buf[3] << 24));
		linecoding.format = Buf[4];
		linecoding.paritytype = Buf[5];
		linecoding.datatype = Buf[6];
		break;

	case GET_LINE_CODING:
		Buf[0] = (uint8_t)(linecoding.bitrate);
		Buf[1] = (uint8_t)(linecoding.bitrate >> 8);
		Buf[2] = (uint8_t)(linecoding.bitrate >> 16);
		Buf[3] = (uint8_t)(linecoding.bitrate >> 24);
		Buf[4] = linecoding.format;
		Buf[5] = linecoding.paritytype;
		Buf[6] = linecoding.datatype; 
		break;

	case SET_CONTROL_LINE_STATE:
		cdcStatus &= ~(CONTROL_LINE_RTS | CONTROL_LINE_DTR);
		cdcStatus |= (Buf[0] & (CONTROL_LINE_RTS | CONTROL_LINE_DTR));
		break;
	}
	return USBD_OK;
}

/**
  * @brief  VCP_DataRx
  *         Data received over USB OUT endpoint are sent over CDC interface 
  *         through this function.
  *           
  *         @note
  *         This function will block any OUT packet reception on USB endpoint 
  *         untill exiting this function. If you exit this function before transfer
  *         is complete on CDC interface (ie. using DMA controller) it will result 
  *         in receiving more data while previous ones are still not sent.
  *                 
  * @param  Buf: Buffer of data to be received
  * @param  Len: Number of data received (in bytes)
  * @retval Result of the opeartion: USBD_OK if all operations are OK else VCP_FAIL
  */
uint16_t VCP_DataRx (uint8_t * buffer, uint32_t length)
{
	uint16_t next;
	uint8_t data;

	while (length != 0)
	{
		data = *buffer++;

		next = APP_Tx_ptr_in + 1;
		if (next == APP_TX_DATA_SIZE)
			next = 0;
		if (next != APP_Tx_ptr_out)
		{
			APP_Tx_Buffer[APP_Tx_ptr_in] = data;
			APP_Tx_ptr_in = next;
		}
		--length;
	}
	return USBD_OK;
}

/**
  * @brief  VCP_Initialize
  *         Return status
  * @retval status
  */
uint8_t VCP_Initialize(void)
{
	return (cdcStatus & USB_INITIALIZED ? 1 : 0);
}

/**
  * @brief  VCP_Line_DTR
  *         Return status
  * @retval status
  */
uint8_t VCP_Line_DTR(void)
{
	return (cdcStatus & CONTROL_LINE_DTR ? 1 : 0);
}

/**
  * @brief  VCP_Line_RTS
  *         Return status
  * @retval status
  */
uint8_t VCP_Line_RTS(void)
{
	return (cdcStatus & CONTROL_LINE_RTS ? 1 : 0);
}

/**
 * @brief	VCP_Available
 *			Return data available flag
 * @retval	= 1, available
 *			= 0, no data
 */
uint8_t VCP_Available(void)
{
	if (APP_Tx_ptr_in != APP_Tx_ptr_out)
		return 1;
	return 0;
}

/**
  * @brief	VCP_Flush
  *			Flush Tx buffer
  * @retval	None
  */
void VCP_Flush(void)
{
	__disable_irq();
	APP_Tx_ptr_out = APP_Tx_ptr_in = 0;
	__enable_irq();
}

/**
 * @brief	VCP_Read
 *			Return data from USB OUT endpoint
 * @retval	data
 */
uint8_t VCP_Read(void)
{
	uint8_t data = 0;
	if (APP_Tx_ptr_in != APP_Tx_ptr_out)
	{
		__disable_irq();
		data = APP_Tx_Buffer[APP_Tx_ptr_out++];
		if (APP_Tx_ptr_out == APP_TX_DATA_SIZE)
			APP_Tx_ptr_out = 0;
		__enable_irq();
	}
	return data;
}

/**
 * @brief	VCP_Write
 *			Data to be send over USB IN endpoint
 * @param	data:
 * @retval	Result of the opeartion: USBD_OK if all operations are OK else VCP_FAIL
 */
USBD_Status VCP_Write(uint8_t data)
{
	uint16_t retry = 10;
	uint16_t next = APP_Rx_ptr_in + 1;
	if (next == APP_RX_DATA_SIZE)
		next = 0;

	// Check for buffer available
	while(next == APP_Rx_ptr_out)
	{
		if (--retry == 0)
			return USBD_FAIL;
		USB_OTG_BSP_mDelay(50);
	}

	// Store to buffer
	__disable_irq();
	APP_Rx_Buffer[APP_Rx_ptr_in] = data;
	APP_Rx_ptr_in = next;
	__enable_irq();

	return USBD_OK;
}

/**
 * @brief	VCP_Print
 *			Send string to USB IN endpoint
 * @param	text:
 * @retval	Result of the opeartion: USBD_OK if all operations are OK else VCP_FAIL
 */
USBD_Status VCP_Print(const char* text)
{
	char c;
	while((c = *text++) != 0)
	{
		if (VCP_Write(c) != USBD_OK)
			return USBD_FAIL;
	}
	return USBD_OK;
}

/**
 * @brief	VCP_Println
 *			Send string to USB IN endpoint
 * @param	text:
 * @retval	Result of the opeartion: USBD_OK if all operations are OK else VCP_FAIL
 */
USBD_Status VCP_Println(const char* text)
{
	if (VCP_Print(text) == USBD_OK)
		return VCP_Print("\r\n");
	return USBD_FAIL;
}
