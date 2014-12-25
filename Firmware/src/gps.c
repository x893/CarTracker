#include <stdio.h>
#include <string.h>
#include <math.h>

#include "hardware.h"
#include "FreeRTOS.h"
#include "queue.h"
#include "semphr.h"
#include "task.h"
#include "main.h"
#include "gps.h"
#include "gsm.h"
#include "debug.h"
#include "kalman.h"
#include "gpsOne.h"
#include "nmea/nmea.h"
#include "nmea/tok.h"
#include "can.h"
#include "ring_buffer.h"

#define GPS_SEND_INTERVAL	30

static char gpsTxBuffer[32];
static char gpsRxBuffer[128];
GpsContext_t GpsContext;

const char HEX_CHARS[]		= "0123456789ABCDEF";

const char GPS_SET_MTK0[]		= "PMTK314,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0";

const char GPS_STM_SBASONOFF[]	= "$PSTMSBASONOFF";
const char GPS_STM_GETSWVER[]	= "$PSTMGETSWVER";
const char GPS_STM_REPLAY[]		= "$PSTMVER,";
const char GPS_SET_STM1[]		= "PSTMSETPAR,1201,40";
const char GPS_SET_STM2[]		= "PSTMSAVEPAR";
const char GPS_SET_STM3[]		= "PSTMSETCONSTMASK,3";

uint8_t	GpsGeneral_Init(GpsContext_t * gc);
uint8_t	GpsSiRF_Init(GpsContext_t * gc);
uint8_t	GpsMTK_Init(GpsContext_t * gc);
uint8_t	GpsSTM_Init(GpsContext_t * gc);
uint8_t	GpsML8088_Init(GpsContext_t * gc);

const GpsCommands_t	GpsCmdGeneral	= { GpsGeneral_Init };
const GpsCommands_t	GpsCmdSiRF		= { GpsSiRF_Init };
const GpsCommands_t	GpsCmdMTK		= { GpsMTK_Init };
const GpsCommands_t	GpsCmdSTM		= { GpsSTM_Init };
const GpsCommands_t	GpsCmdML8088	= { GpsML8088_Init };

const char GPS_SET_SIRF0[]		= "PSRF105,0";			// Debug Off
const char GPS_SET_SIRF1[]		= "PSRF103,0,0,0,1";	// Disable GGA
const char GPS_SET_SIRF2[]		= "PSRF103,1,0,0,1";	// Disable GLL
const char GPS_SET_SIRF3[]		= "PSRF103,2,0,0,1";	// Disable GSA
const char GPS_SET_SIRF4[]		= "PSRF103,3,0,0,1";	// Disable GSV
const char GPS_SET_SIRF5[]		= "PSRF103,4,0,1,1";	// Enable  RMC
const char GPS_SET_SIRF6[]		= "PSRF103,5,0,0,1";	// Disable VTG
/*
const char GPS_SET_SIRF7[]		= "PSRF100,0,4800,8,1,0";	// Switch to OSP protocol
															// Enable SBAS
const uint8_t GPS_SET_SIRF8[]		= {	0x85,		// MID: 133
									0x01,		// Uses SBAS Satellite
									0x00, 0x00, 0x00, 0x00,
									0x00
								};
									// Switch to NMEA protocol, Only RMC enable
const uint8_t GPS_SET_SIRF9[]		= {	0x81,		// MID: 129
									0x02,		// Do not change last-set value for NMEA debug messages
									0x00, 0x01,	// GGA
									0x00, 0x01, // GLL
									0x00, 0x01, // GSA
									0x00, 0x01, // GSV
									0x01, 0x01, // RMC
									0x00, 0x01,	// VTG
									0x00, 0x01, // MSS
									0x00, 0x00, // EPE
									0x00, 0x01, // ZDA
									0x00, 0x00,	// Unused
									0x12, 0xC0	// 4800
								};
const uint8_t GPS_SET_SIRF10[]		= {	0x80,	// Factory Reset
									0x00, 0x00, 0x00, 0x00,
									0x00, 0x00, 0x00, 0x00,
									0x00, 0x00, 0x00, 0x00,
									0x00, 0x00, 0x00, 0x00,
									0x00, 0x00, 0x00, 0x00,
									0x00, 0x00,
									0x0C,
									0x88,
									0x01,
									0x14 };
*/

typedef const char * pChar;

const pChar GPS_SET_STM[] = {
	GPS_SET_STM1,
	GPS_SET_STM1,
	GPS_SET_STM2,
	GPS_SET_STM2,
	GPS_SET_STM3,
	NULL
};

const pChar GPS_SET_SIRFS[] = {
	GPS_SET_SIRF0,
	GPS_SET_SIRF1,
	GPS_SET_SIRF2,
	GPS_SET_SIRF3,
	GPS_SET_SIRF4,
	GPS_SET_SIRF5,
	GPS_SET_SIRF6,
	NULL
};
#define GPS_MODE_IGNORE 0
#define GPS_MODE_START	1
#define GPS_MODE_END	2
#define GPS_MODE_READY	3
#define GPS_MODE_CHECK	4
#define GPS_MODE_CHECK2	5
#define GPS_MODE_END2	6

/*******************************************************************************
* Function Name	:	gpsTxGet
* Description	:	Get data from GPS TX queue
* Input			:	character from queue
* Return		:	result code
*******************************************************************************/
void GpsPut(uint8_t data)
{
	while(rb_Put(&GpsContext.RB_Tx, data) == 0)
	{
		USART_ITConfig(GPS_USART, USART_IT_TXE, ENABLE);
		vTaskDelay(5);
	}
}

/*******************************************************************************
* Function Name	:	GpsPutLine
* Description	:	Send line to GPS
* Input			:	Add $ at begin and *XX\r\n checksum at end
* Return		:	result code
*******************************************************************************/
void GpsPutLineCS(const char * text)
{
	uint8_t data;
	uint8_t cs = 0;

	GpsPut('$');
	while((data = *text++) != 0)
	{
		cs ^= data;
		GpsPut(data);
	}

	GpsPut('*');
	GpsPut(HEX_CHARS[cs >> 4]);
	GpsPut(HEX_CHARS[cs & 0x0F]);
	GpsPut('\r');
	GpsPut('\n');
	USART_ITConfig(GPS_USART, USART_IT_TXE, ENABLE);
}

/*******************************************************************************
* Function Name	:	GpsPutLineLn
* Description	:	Send line to GPS
* Input			:	Add <cr><lf> at end
* Return		:	result code
*******************************************************************************/
void GpsPutLine(const char * text)
{
	uint8_t data;
	while((data = *text++) != 0)
		GpsPut(data);
	USART_ITConfig(GPS_USART, USART_IT_TXE, ENABLE);
}

void GpsPutLineLn(const char * text)
{
	GpsPutLine(text);
	GpsPut('\r');
	GpsPut('\n');
}

void GpsPutBinary(const char * text, int len)
{
	while(len-- != 0)
		GpsPut(*text++);
	USART_ITConfig(GPS_USART, USART_IT_TXE, ENABLE);
}

/*******************************************************************************
* Function Name :	GpsSendSiRFHeader
* Description   :	
*******************************************************************************/
void GpsSendSiRF(const uint8_t * cmd, int len)
{
	uint8_t data;
	uint16_t cs = 0;

	GpsPut(0xA0);
	GpsPut(0xA2);
	GpsPut(len >> 8);
	GpsPut(len & 0xFF);
	while (len-- != 0)
	{
		data = *cmd++;
		while(rb_Put(&GpsContext.RB_Tx, data) == 0)
		{
			USART_ITConfig(GPS_USART, USART_IT_TXE, ENABLE);
			vTaskDelay(5);
		}
		cs += (uint16_t)data;
	}
	cs &= 0x7FFF;
	GpsPut(cs >> 8);
	GpsPut(cs & 0xFF);
	GpsPut(0xB0);
	GpsPut(0xB3);
	USART_ITConfig(GPS_USART, USART_IT_TXE, ENABLE);
}

/*******************************************************************************
* Function Name	:	GpsInit
* Description	:	Initialize GPS
*******************************************************************************/
void GpsInit(GpsContext_t * gc, uint32_t speed)
{
	USART_InitTypeDef USART_InitStructure;

	rb_Init(&gc->RB_Tx, gpsTxBuffer, sizeof(gpsTxBuffer));

	GPS_RX_PREPARE();

	gc->WaitAliveSize = 0;
	gc->RxMode = GPS_MODE_START;

	GPS_PWR_ON();

	GPS_TX_INIT();
	GPS_RX_INIT();
	GPS_USART_CLK(ENABLE);

	USART_StructInit(&USART_InitStructure);
	USART_InitStructure.USART_BaudRate = speed;
	USART_Init(GPS_USART, &USART_InitStructure);
	USART_Cmd(GPS_USART, ENABLE);
	USART_ITConfig(GPS_USART, USART_IT_RXNE, ENABLE);
}

/*******************************************************************************
* Function Name	:	GpsDeInit
* Description	:	DeInitialize GPS
*******************************************************************************/
void GpsDeInit(GpsContext_t * gc)
{
	gc->RxMode = GPS_MODE_IGNORE;
	GPS_DEINIT();
	vTaskDelay(1000);
}

bool gpsWaitComplete(GpsContext_t * gc, uint16_t ms)
{
	while (gc->RxMode != GPS_MODE_READY && ms != 0)
	{
		if (ms >= 10)
		{
			vTaskDelay(10);
			ms -= 10;
		}
		else
		{
			--ms;
			vTaskDelay(1);
		}
	}
	return (gc->RxMode == GPS_MODE_READY ? true : false);
}

bool gpsWaitReady(GpsContext_t * gc, uint16_t ms)
{
	gc->RxMode = GPS_MODE_START;
	return gpsWaitComplete(gc, ms);
}

/*******************************************************************************
* Function Name	:	gpsCheckType
* Description	:
*******************************************************************************/
bool gpsCheckTypeInit(GpsContext_t * gc, uint32_t speed)
{
	GpsInit(gc, speed);
	if (gpsWaitComplete(gc, 5000))
	{	// Receive first message from GPS
		if (strcmp(gpsRxBuffer, "$PSRF150,1*3E\r\n") == 0)
		{	// SiRF chipset
			DebugPutLine(" SiRF");
			gc->GpsCommand = &GpsCmdSiRF;
		}
		else if (strcmp(gpsRxBuffer, "$PMTK011,MTKGPS*08\r\n") == 0)
		{	// MTK chipset
			DebugPutLine(" MTK");
			gc->GpsCommand = &GpsCmdMTK;
		}
		else if (strncmp(gpsRxBuffer, "$PSTMVER,GNSSLIB_7.2.4.40_ARM", sizeof("$PSTMVER,GNSSLIB_7.2.4.40_ARM") - 1) == 0)
		{	// ML8088s
			DebugPutLine(" ML8088");
			gc->GpsCommand = &GpsCmdML8088;
		}
		else if (
				strncmp(gpsRxBuffer, "$PSTMVER,", 9) == 0
			||	strncmp(gpsRxBuffer, "$GPTXT,TELIT SW Version: SL869", sizeof("$GPTXT,TELIT SW Version: SL869") - 1) == 0
				)
		{	// STM chipset
			DebugPutLine(" STM");
			gc->GpsCommand = &GpsCmdSTM;
		}
		return true;
	}
	GpsDeInit(gc);
	return false;
}

/*******************************************************************************
* Function Name	:	vGpsTask
* Description	:	GPS Task
*******************************************************************************/
void vGpsTask(void *pvArg)
{
	register GpsContext_t * gc = &GpsContext;
	uint8_t msg_count;

#ifdef DISPLAY_STACK
	DisplayStackHigh("*** GPS Stack 0:");
#endif

restart_gps:
	GpsDeInit(gc);

	while (GlobalStatus & GPS_STOP)
		vTaskDelay(5000);

	gc->RxMode = GPS_MODE_IGNORE;
	gc->GpsCommand = &GpsCmdGeneral;

	while(1)
	{
		DebugPutLine("GPS");

		if (gpsCheckTypeInit(gc, 9600))
		{
			DebugPutLineln(" 9600");
			break;
		}
		if (gpsCheckTypeInit(gc, 4800))
		{
			DebugPutLineln(" 4800");
			break;
		}
		if (gpsCheckTypeInit(gc, 115200))
		{
			DebugPutLineln(" 115200");
			break;
		}

		DebugPutLineln(" not found");
		ShowError(ERROR_GPS_INIT_FAIL);
	}

	(*gc->GpsCommand->Init)(gc);

	GlobalStatus |= GPS_COMPLETE;
	gc->SendTimer = 5;
	gc->NoDataTimer = 0;
	gc->SendInterval = GPS_SEND_INTERVAL;

	gc->RxMode = GPS_MODE_START;

#if ( USE_KALMAN == 1 )
	FilterKalmanInit();
#endif
#if ( USE_AVERAGE == 1 )
	FilterAverageInit(gc);
#endif

	msg_count = 15;
	for(;;)
	{
		if (GlobalStatus & GPS_STOP)
			goto restart_gps;

		gc->NoDataTimer++;
		if (gc->NoDataTimer >= 5 * 60)
		{	// No fix within 5 minutes, restart GPS module
			vTaskDelay(5000);
			goto restart_gps;
		}

		if (!gpsWaitComplete(gc, 10000))		// Wait for packet within 5 seconds
		{	// No data from GPS within 5 seconds
			gc->RxMode = GPS_MODE_START;
			ShowError(ERROR_GPS_NO_DATA);
			gc->NoDataTimer += 10;
			continue;
		}

		if (nmea_parse_GPRMC(gpsRxBuffer, gc->RxIndex, &gc->RMC))
		{
			if (msg_count++ >= 15)
			{
				msg_count = 0;
				DebugPutLine(gpsRxBuffer);
			}
			// DebugPutLine(gpsRxBuffer);
			gc->RxMode = GPS_MODE_START;

			if (gc->RMC.status != 'A')
			{
				ShowError(1);	// Position not fixed
			}
			else
			{
#if ( USE_AVERAGE == 1 || USE_KALMAN == 1 )
				{
					nmeaGPRMC * rmc = &(gc->RMC);
					bool use_kalman = true;
					uint32_t time_delta = 0;
					uint32_t time;
					
					double lat= rmc->lat;
					double lon = rmc->lon;

					lat = nmea_ndeg2degree((rmc->ns == 'S') ? -lat : lat);
					lon = nmea_ndeg2degree((rmc->ew == 'W') ? -lon : lon);

					time = xTaskGetTickCount() / configTICK_RATE_HZ;
					if (gc->LastFilterTime != 0)
					{
						if (time == gc->LastFilterTime)
						{
							time_delta = 1;
						}
						else if (time > gc->LastFilterTime)
						{
							time_delta = time - gc->LastFilterTime;
						}
						else
						{
							time_delta = gc->LastFilterTime - time;
						}
					}
					gc->LastFilterTime = time;

	#if ( USE_AVERAGE == 1 )
					FilterAverageUpdate(gc, lat, lon, time_delta);

					if (rmc->speed < AVERAGE_SPEED_5)			// Adaptate average sum with speed
					{
						gc->AverageMaxCount = AVERAGE_SUM_5;
						use_kalman = false;
					}
					else if (rmc->speed < AVERAGE_SPEED_10)
					{
						gc->AverageMaxCount = AVERAGE_SUM_5;
						use_kalman = false;
					}
					else if (rmc->speed < AVERAGE_SPEED_20)
					{
						gc->AverageMaxCount = AVERAGE_SUM_20;
					}
					else
					{
						gc->AverageMaxCount = AVERAGE_SUM_MORE;
					}

					FilterAverageGet(gc, &lat, &lon);	// Get avarage position
					gc->RMC.lat = lat;					// and save it
					gc->RMC.lon = lon;
	#endif
	#if ( USE_KALMAN == 1 )
					if (time_delta == 0)
						time_delta++;
					FilterKalmanUpdate(lat, lon, time_delta);
					if (use_kalman)
					{
						FilterKalmanGet(&(rmc->lat), &(rmc->lon));
					}
	#endif
#endif
				}
				if (GlobalStatus & GSM_COMPLETE)
				{
					if (gc->SendTimer == 0)
					{
						gc->SendTimer = gc->SendInterval;
						gc->NoDataTimer = 0;
						memcpy(&gc->GsmTask.GPRMC, &gc->RMC, sizeof(nmeaGPRMC));

						gc->GsmTask.SenseD1 = (SENS_D1_READ() ? 1 : 0);
						gc->GsmTask.SenseA1 = 0;

#if (USE_CAN_SUPPORT == 1)
						{
							uint8_t idx;
							CanEnable = 0;
							while (!CanDisabled)
								vTaskDelay(1);

							for (idx = 0; idx < CAN_VALUES_SIZE; idx ++)
								gc->GsmTask.CanData[idx] = CanSumGet(idx);

							CanSumInit();
							CanEnable = 1;
						}
#endif
						gc->GsmTask.Command = GSM_TASK_CMD_GPS;

						if (GsmAddTask(&gc->GsmTask) == 0)
						{	// GSM task queue full, show error
							ShowError(ERROR_GPS_TASK_NOT_ADD);
						}
#ifdef DISPLAY_STACK
						DisplayStackHigh("*** GPS Stack 1:");
#endif
					}
					gc->SendTimer--;
				}
			}
		}
		else
			gc->RxMode = GPS_MODE_START;
	}
}

/*******************************************************************************
* Function Name	:	GPS_USART_IRQHandler
* Description	:	GPS USART interrupt handler
*******************************************************************************/
void GPS_USART_IRQHandler(void)
{
	USART_TypeDef * port = GPS_USART;
	register GpsContext_t *gc = &GpsContext;

	if (USART_GetITStatus(port, USART_IT_RXNE) != RESET)
	{
		gc->Data = port->DR;
		if ( port->SR & (USART_FLAG_ORE | USART_FLAG_NE | USART_FLAG_FE | USART_FLAG_PE))
		{
			gc->Data = port->DR;	// Receive error, ignore line
			gc->RxMode = (gc->RxMode == GPS_MODE_END2) ? GPS_MODE_CHECK : GPS_MODE_START;
		}
		else if (gc->RxMode == GPS_MODE_IGNORE)
		{
			return;
		}
		else if (gc->RxMode == GPS_MODE_END
			||	gc->RxMode == GPS_MODE_END2)
		{
			if (gc->RxIndex > (sizeof(gpsRxBuffer) - 2))
			{	// String too long, ignore it and set wait a $
				gc->RxMode = (gc->RxMode == GPS_MODE_END2) ? GPS_MODE_CHECK : GPS_MODE_START;
			}
			else
			{
				if (gc->Data == '$')
					gc->RxIndex = 0;
					
				gpsRxBuffer[gc->RxIndex++] = gc->Data;
				if (gc->WaitAliveSize != 0)
				{
					if (gc->WaitAliveSize == gc->RxIndex)
					{
						if (strncmp(gpsRxBuffer, gc->WaitAliveText, gc->WaitAliveSize) == 0)
							gc->WaitAliveSize = 0;
						else
							gc->RxMode = (gc->RxMode == GPS_MODE_END2) ? GPS_MODE_CHECK : GPS_MODE_START;
					}
					else if (gc->Data == '\n')
					{
						gc->RxMode = (gc->RxMode == GPS_MODE_END2) ? GPS_MODE_CHECK : GPS_MODE_START;
					}
				}
				else if (gc->Data == '\n')
				{
					if (GPS_MODE_END2 || (gc->RxIndex > 6 && strncmp(gpsRxBuffer, "$GPRMC", 6) == 0))
					{
						gpsRxBuffer[gc->RxIndex] = 0;
						gc->RxMode = GPS_MODE_READY;
					}
					else
					{
						gc->RxMode = (gc->RxMode == GPS_MODE_END2) ? GPS_MODE_CHECK : GPS_MODE_START;
					}
				}
			}
		}
		else if (gc->RxMode == GPS_MODE_START || gc->RxMode == GPS_MODE_CHECK)
		{
			if (gc->Data == '$')
			{
				gc->RxIndex = 0;
				gpsRxBuffer[gc->RxIndex++] = gc->Data;
				gc->RxMode = (gc->RxMode == GPS_MODE_CHECK) ? GPS_MODE_CHECK2 : GPS_MODE_END;
			}
		}
		else if (gc->RxMode == GPS_MODE_CHECK2)
		{
			gpsRxBuffer[gc->RxIndex++] = gc->Data;
			if (gc->RxIndex == 3)
			{
				if (gpsRxBuffer[0] == '$'
				&&	gpsRxBuffer[1] == 'G'
				&&	gpsRxBuffer[2] == 'P'
					)
					gc->RxMode = GPS_MODE_END2;
				else
					gc->RxMode = GPS_MODE_CHECK;
			}
		}
	}
	if (USART_GetITStatus(port, USART_IT_TXE) != RESET)
	{
		if (rb_Get(&gc->RB_Tx, &gc->Data))
			port->DR = gc->Data;
		else
			USART_ITConfig(port, USART_IT_TXE, DISABLE);
	}
}

uint8_t GpsGeneral_Init(GpsContext_t * gc)
{
	return E_OK;
}

uint8_t GpsMTK_Init(GpsContext_t * gc)
{
	GpsPutLineCS(GPS_SET_MTK0);
	vTaskDelay(10);
	return E_OK;
}

uint8_t gpsSendBatch(const pChar * cmds)
{
	while (*cmds++ != NULL)
	{
		vTaskDelay(500);
		GpsPutLineCS(*cmds);
	}
	vTaskDelay(10);
	return E_OK;
}

uint8_t GpsSiRF_Init(GpsContext_t * gc)
{
	return gpsSendBatch(GPS_SET_SIRFS);
}

uint8_t GpsSTM_Init(GpsContext_t * gc)
{
	return gpsSendBatch(GPS_SET_STM);
}

uint8_t GpsML8088_Init(GpsContext_t * gc)
{
	return gpsSendBatch(GPS_SET_STM);
}
