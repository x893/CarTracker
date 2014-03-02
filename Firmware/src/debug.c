#include <stdio.h>

#include "hardware.h"
#include "main.h"
#include "debug.h"
#include "ring_buffer.h"

#ifndef BL_VERSION
	#include "FreeRTOS.h"
	#include "task.h"
#endif

#if   ((USE_USB == 1) && (USE_USB_DEBUG == 1))

	#include "usbd_cdc_vcp.h"

	#define PUTCHAR_PROTOTYPE int fputc(int ch, FILE *f)

	/*******************************************************************************
	* Function Name	:	PUTCHAR_PROTOTYPE
	* Description	:
	* Input			:
	* Return		:
	*******************************************************************************/
	PUTCHAR_PROTOTYPE
	{
		if (ch == '\n')
			DebugPut('\r');
		DebugPut((u8)ch);
		return ch;
	}

	/*******************************************************************************
	* Function Name	:	DebugPut
	* Description	:
	* Input			:
	* Return		:
	*******************************************************************************/
	uint8_t DebugPut(char c)
	{
		VCP_Write(c);
		return 1;
	}

	/*******************************************************************************
	* Function Name	:	IsDebugAvailable
	* Description	:
	* Input			:
	* Return		:
	*******************************************************************************/
	uint8_t IsDebugAvailable(void)
	{
		return VCP_Available();
	}

	/*******************************************************************************
	* Function Name	:	DebugPutLine
	* Description	:
	* Input			:
	* Return		:
	*******************************************************************************/
	void DebugPutLine(const char *msg)
	{
		VCP_Print(msg);
	}

	/*******************************************************************************
	* Function Name	:	DebugPutLineln
	* Description	:
	* Input			:
	* Return		:
	*******************************************************************************/
	void DebugPutLineln(const char *msg)
	{
		VCP_Println(msg);
	}

	/*******************************************************************************
	* Function Name	:	DebugWaitConnect
	* Description	:
	* Input			:
	* Return		:
	*******************************************************************************/
	uint8_t DebugWaitConnect(void)
	{
		uint8_t timeout;

		if (USB_CONNECT())
		{
			timeout = 10;
			while(--timeout != 0 && VCP_Line_DTR() == 0)
				vTaskDelay(1000);
			return VCP_Line_DTR();
		}
		return 0;
	}

#endif
#if	( USE_SERIAL_DEBUG == 1)

	#ifndef BL_VERSION
		static char dbgTxBuffer[64];
		RingBuffer_t dbgTx;
		#if (DEBUG_RX_QUEUE_SIZE > 0)
			static char dbgRxBuffer[DEBUG_RX_QUEUE_SIZE];
			RingBuffer_t dbgRx;
		#endif
	#endif

	#define PUTCHAR_PROTOTYPE int fputc(int ch, FILE *f)
	/*******************************************************************************
	* Function Name	:	PUTCHAR_PROTOTYPE
	* Description	:
	* Input			:
	* Return		:
	*******************************************************************************/
	PUTCHAR_PROTOTYPE
	{
		if (ch == '\n')
			DebugPut('\r');
		DebugPut((char)ch);
		return ch;
	}

	/*******************************************************************************
	* Function Name	:	DebugInit
	* Description	:
	* Input			:
	* Return		:
	*******************************************************************************/
	void DebugInit(void)
	{
	#ifndef BL_VERSION
		rb_Init(&dbgTx, dbgTxBuffer, sizeof(dbgTxBuffer));
		#if (DEBUG_RX_QUEUE_SIZE > 0)
			rb_Init(&dbgRx, dbgRxBuffer, sizeof(dbgRxBuffer));
			USART_ITConfig(DEBUG_USART, USART_IT_RXNE, ENABLE);
		#endif
	#endif
	}

	/*******************************************************************************
	* Function Name	:	DebugPut
	* Description	:	Put data to TX queue
	* Input			:	character to send
	* Return		:	result code
	*******************************************************************************/
	 void DebugPut(char ch)
	{
		if (ch == '\n')
			DebugPut('\r');
	#ifndef BL_VERSION
		while (!rb_Put(&dbgTx, ch))
		{	// No space in TX buffer, enable interrupt and wait
			USART_ITConfig(DEBUG_USART, USART_IT_TXE, ENABLE);
			vTaskDelay(10);
		}
		USART_ITConfig(DEBUG_USART, USART_IT_TXE, ENABLE);
	#else
		while (!(DEBUG_USART->SR & USART_FLAG_TXE));
		DEBUG_USART->DR = ch;
		while (!(DEBUG_USART->SR & USART_FLAG_TC));
	#endif
	}

	/*******************************************************************************
	* Function Name	:	DebugPutLine
	* Description	:
	* Input			:
	* Return		:
	*******************************************************************************/
	void DebugPutLine(const char *msg)
	{
		char ch;
		while ((ch = *msg++) != 0)
			DebugPut(ch);
	}

	/*******************************************************************************
	* Function Name	:	DebugPutLineln
	* Description	:
	* Input			:
	* Return		:
	*******************************************************************************/
	void DebugPutLineln(const char *msg)
	{
		DebugPutLine(msg);
		DebugPutLine("\n");
	}

	/*******************************************************************************
	* Function Name	:	DebugPut
	* Description	:	Put data to TX queue
	* Input			:	character to send
	* Return		:	result code
	*******************************************************************************/
	uint8_t DebugFlush(void)
	{
	#ifndef BL_VERSION
		if (dbgTx.Head == dbgTx.Tail)
			return 0;
		return 1;
	#else
		return 0;
	#endif
	}

	/*******************************************************************************
	* Function Name	:	IsDebugAvailable
	* Description	:
	* Input			:
	* Return		:
	*******************************************************************************/
	uint8_t IsDebugAvailable(void)
	{
	#ifndef BL_VERSION
		#if (DEBUG_RX_QUEUE_SIZE > 0)
			if (dbgRx.Tail != dbgRx.Head)
				return 1;
		#endif
	#endif
		return 0;
	}

	/*******************************************************************************
	* Function Name	:	DebugWaitConnect
	* Return		:	1
	*******************************************************************************/
	uint8_t DebugWaitConnect(void)
	{
		return 1;
	}
	/*******************************************************************************
	* Function Name	:	DebugGet
	* Description	:	Get data from RX queue
	* Return		:	character or 0
	*******************************************************************************/
	char DebugGet(void)
	{
	#ifndef BL_VERSION
		#if (DEBUG_RX_QUEUE_SIZE > 0)
			char ch;
			rb_Get(&dbgRx, &ch);
			return ch;
		#else
			return 0;
		#endif
	#else
		return 0;
	#endif
	}

	#ifndef BL_VERSION
	/*******************************************************************************
	* Function Name	:	DEBUG_USART_IRQHandler
	* Description	:
	*******************************************************************************/
	void DEBUG_USART_IRQHandler(void)
	{
		char data;

		if (USART_GetITStatus(DEBUG_USART, USART_IT_TXE) != RESET)
		{
			if (rb_Get(&dbgTx, &data))
				DEBUG_USART->DR = data;
			else
				USART_ITConfig(DEBUG_USART, USART_IT_TXE, DISABLE);
		}
		if (USART_GetITStatus(DEBUG_USART, USART_IT_RXNE) != RESET)
		{
			if ( DEBUG_USART->SR & (USART_FLAG_ORE | USART_FLAG_NE | USART_FLAG_FE | USART_FLAG_PE))
				data = DEBUG_USART->DR;
			#if (DEBUG_RX_QUEUE_SIZE > 0)
			else
				rb_Put(&dbgRx, data);
			#endif
		}
	}
	#endif

#endif
