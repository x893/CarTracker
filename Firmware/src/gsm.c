#include <stdio.h>
#include <stdarg.h>
#include <stdbool.h>
#include <string.h>
#include <math.h>

#include "hardware.h"
#include "FreeRTOS.h"
#include "queue.h"
#include "semphr.h"
#include "task.h"

#include "main.h"
#include "fw_update.h"
#include "gsm.h"
#include "debug.h"
#include "utility.h"
#include "gpsOne.h"
#include "ring_buffer.h"

#include <nmea/tok.h>

#define GSM_MODE_IGNORE			0
#define GSM_MODE_AT				1
#define GSM_MODE_STREAM			2

#define GSM_PING_MS				90

#define GSM_GPRS_TIMEOUT		30000
#define GSM_TCP_TIMEOUT			30000
#define GSM_MAX_REPLAY_TIMEOUT	15000
#define GSM_INITIAL_WAIT		30000

#define GSM_STATUS_TCP_ON		0x01
#define GSM_STATUS_AUTH_ERR		0x02
#define GSM_STATUS_TCP_DISABLE	0x04
#define GSM_STATUS_REGISTER		0x08

typedef struct {
		uint8_t	(*GprsStop)(void);
	const char	* StartIP;
		uint8_t	(*GprsSet)(void);
		uint8_t	(*GprsStart)(void);
		uint8_t	(*TcpOpen)(void);	// 4
	const char	* TcpData;
	const char	* TcpReady;
	const char	* TcpClose;			// 7
	const char	* PSSSTK;
	const char	* GetIMEI;
} GsmCommands_t;

typedef struct GsmContext_s
{
	const char *	RxWaitFor;
	const char *	WaitAlive;

	int				WaitAliveLen;
	uint16_t		HostIndex;
	uint8_t			SkipHostLine;
	uint8_t			RxMode;
	char			Data;
	uint8_t			Status;
	uint8_t			AuthErrorCount;
	uint8_t			Error;

volatile portTickType 	LastReplyTime;
volatile portTickType	PingInterval;
volatile portTickType 	PingTick;

	RingBuffer_t	RB_Rx;
	RingBuffer_t	RB_Tx;

	const GsmCommands_t * Commands;
	xSemaphoreHandle	ReadySemaphore;
	xQueueHandle		TaskQueue;
	char				IMEI[24];

} GsmContext_t;

/*	Private functions */
void	gsmSend		(const char *text);
void	gsmSends	(const char * cmd, ...);
uint8_t	gsmCmd0		(const char* text);
uint8_t	gsmCmd1		(const char* text, u16 timeout);
uint8_t	gsmCmd2		(const char* text, u16 timeout, const char* waitFor, const char* waitAlive);
uint8_t	gsmCmdX0	(const char* str1, char* str2, const char* str3);
uint8_t	gsmCmdX2	(const char* str1, char* str2, const char* str3, char* str4, const char* str5, u16 timeout, const char* wait);

uint8_t	gsmSaveSMS		(const char *da, const char *msg);
uint8_t	gsmSendSMS		(const char *da, const char *msg);
void	gsmScanSMS		(GsmContext_t * gc);
uint8_t	gsmCheckPIN		(GsmContext_t *);
uint8_t	gsmIsRegister	(GsmContext_t *);
uint8_t	gsmWaitRegister	(GsmContext_t *);
void	gsmWaitNetwork	(GsmContext_t * gc);
char *	gsmToken		(char *);
void	gsmRxFlush		(GsmContext_t *);

GsmContext_t GsmContext;
GsmTask_t	GsmTask;
GsmTask_t	GsmTaskSms;

typedef struct SMS_s {
			int	Index;
		uint8_t	Type;
	uint32_t	DateTime;
		char	DA[16];
} SMS_t;

char gsmSenderDA[16];		// Buffer for data to GSM
char gsmTxBuffer[64];		// Buffer for data to GSM
#ifdef STM32F100CB
	char gsmRxBuffer[1024+128];	// Buffer for data from GSM
	char gsmBuffer  [512+128];	// Buffer for host response
#else
	char gsmRxBuffer[2048+128];	// Buffer for data from GSM
	char gsmBuffer  [1024+128];	// Buffer for host response
#endif

const char GSM_AT[]				= "AT\r\n";
const char GSM_E0[]				= "ATE0\r\n";
const char GSM_CFUN1[]			= "AT+CFUN=1\r\n";
const char GSM_CMEE[]			= "AT+CMEE=1\r\n";
const char GSM_GET_CPIN[]		= "AT+CPIN?\r\n";
const char GSM_CPIN_RDY[]		= "+CPIN: READY";
const char GSM_NO_SIM[]			= "+CME ERROR: 10";

const char GSM_CMGF[]			= "AT+CMGF=1\r\n";
const char GSM_CNMI[]			= "AT+CNMI=0\r\n";
const char GSM_CMGL[]			= "AT+CMGL=\"ALL\"\r\n";
const char GSM_CSDH[]			= "AT+CSDH=0\r\n";

const char GSM_CREG[]			= "AT+CREG?\r\n";
const char GSM_CREG0[]			= "AT+CREG=0\r\n";
const char GSM_COPS2[]			= "AT+COPS=2\r\n";
const char GSM_COPS0[]			= "AT+COPS=0\r\n";
const char GSM_CREG01[]			= "+CREG: 0,1";
const char GSM_CREG03[]			= "+CREG: 0,3";
const char GSM_CREG05[]			= "+CREG: 0,5";

const char GSM_CGATT[]			= "AT+CGATT?\r\n";
const char GSM_CGATT1[]			= "+CGATT: 1";
const char GSM_LINE_END[]		= "\"\r\n";
const char GSM_CRLF[]			= "\r\n";

const char GSM_OK[]				= "OK";
const char GSM_ERROR[]			= "ERROR";
const char GSM_CME_ERROR[]		= "+CME ERROR:";
const char GSM_CMS_ERROR[]		= "+CMS ERROR:";
const char GSM_SHUTDOWN[]		= "SHUTDOWN";
const char GSM_CONNECT[]		= "CONNECT";
const char GSM_NO_CARRIER[]		= "NO CARRIER";

const char GSM_K0[]				= "AT+IFC=0,0\r\n";
const char GSM_PSSTKI[]			= "AT*PSSTKI=0\r\n";
const char GSM_GSN[]			= "AT+GSN\r\n";
const char GSM_CGSN[]			= "AT+CGSN\r\n";
const char GSM_ATI3[]			= "ATI3\r\n";
const char GSM_WIPPEERCLOSE[]	= "+WIPPEERCLOSE:";
const char GSM_WIPCFG0[]		= "AT+WIPCFG=0\r\n";
const char GSM_IP_STOP[]		= "AT+WIPCFG=0\r\n";
const char GSM_IP_START[]		= "AT+WIPCFG=1\r\n";
const char GSM_GPRS_SET[]		= "AT+WIPBR=1,6\r\n";
const char GSM_GPRS_APN[]		= "AT+WIPBR=2,6,11,\"";
const char GSM_GPRS_USER[]		= "AT+WIPBR=2,6,0,\"";
const char GSM_GPRS_PASS[]		= "AT+WIPBR=2,6,1,\"";
const char GSM_GPRS_START[]		= "AT+WIPBR=4,6,0\r\n";
const char GSM_TCP_OPEN[]		= "AT+WIPCREATE=2,1,\"";
const char GSM_TCP_DATA[]		= "AT+WIPDATA=2,1,1\r\n";
const char GSM_TCP_READY[]		= "+WIPREADY: 2,1";
const char GSM_TCP_CLOSE[]		= "AT+WIPCLOSE=2,1\r\n";

uint8_t GPRS_SET_WS228(void)
{
	if (E_OK == gsmCmd0("AT+WIPBR=1,6\r\n")
	&&	E_OK == gsmCmdX0("AT+WIPBR=2,6,11,\"", SET_GRPS_APN_NAME, GSM_LINE_END)
	&&	E_OK == gsmCmdX0("AT+WIPBR=2,6,0,\"", SET_GPRS_USERNAME, GSM_LINE_END)
		)
		return gsmCmdX0("AT+WIPBR=2,6,1,\"", SET_GPRS_PASSWORD, GSM_LINE_END);
	return GsmContext.Error;
}

uint8_t TCP_OPEN_WS228(void)
{
	return gsmCmdX2(
		"AT+WIPCREATE=2,1,\"",
		SET_HOST_ADDR,
		"\",",
		SET_HOST_PORT,
		GSM_CRLF,
		GSM_TCP_TIMEOUT,
		"+WIPREADY: 2,1");
}

uint8_t GPRS_STOP_W228(void)
{
	return gsmCmd0("AT+WIPCFG=0\r\n");
}
uint8_t GPRS_START_W228(void)
{
	return gsmCmd1("AT+WIPBR=4,6,0\r\n", GSM_GPRS_TIMEOUT);
}

uint8_t GPRS_SET_GL865(void)
{
	if (E_OK == gsmCmdX0("AT+CGDCONT=1,\"IP\",\"", SET_GRPS_APN_NAME, GSM_LINE_END)
	&&	E_OK == gsmCmdX0("AT#USERID=\"", SET_GPRS_USERNAME, GSM_LINE_END)
		)
		return gsmCmdX0("AT#PASSW=\"", SET_GPRS_PASSWORD, GSM_LINE_END);
	return GsmContext.Error;
}

uint8_t GPRS_STOP_GL865(void)
{
	uint8_t result = gsmCmd0("AT#GPRS=0\r\n");
	vTaskDelay(500);
	return result;
}
uint8_t GPRS_START_GL865(void)
{
	uint8_t result;
	vTaskDelay(500);
	result = gsmCmd1("AT#GPRS=1\r\n", GSM_GPRS_TIMEOUT);
	if (result == E_ERROR)
	{	// Try deactivate GPRS abd activate again
		result = GPRS_STOP_GL865();
		if (result == E_OK)
		{
			vTaskDelay(500);
			result = gsmCmd1("AT#GPRS=1\r\n", GSM_GPRS_TIMEOUT);
		}
	}
	return result;
}
uint8_t TCP_OPEN_GL865(void)
{
	gsmSends("AT#SKTD=0,", SET_HOST_PORT, ",\"", SET_HOST_ADDR, NULL);
	return gsmCmd2(GSM_LINE_END, GSM_TCP_TIMEOUT, NULL, GSM_CONNECT);
}

const GsmCommands_t Commands_WS228 =
{
	GPRS_STOP_W228,
	"AT+WIPCFG=1\r\n",
	GPRS_SET_WS228,
	GPRS_START_W228,
	TCP_OPEN_WS228,
	"AT+WIPDATA=2,1,1\r\n",
	"+WIPREADY: 2,1",
	"AT+WIPCLOSE=2,1\r\n",
	"AT*PSSTKI=0\r\n",
	"AT+GSN\r\n"
};

const GsmCommands_t Commands_GL865 =
{
	GPRS_STOP_GL865,
	NULL,
	GPRS_SET_GL865,
	GPRS_START_GL865,
	TCP_OPEN_GL865,
	NULL,
	NULL,
	NULL,
	NULL,
	"AT+CGSN\r\n"
};

uint8_t GsmIsRegister(void)
{
	return (GsmContext.Status & GSM_STATUS_REGISTER);
}

/*******************************************************************************
* Function Name :	GsmAddTask
* Description   :	Add task to queue to back
*******************************************************************************/
uint8_t GsmAddTask(GsmTask_t * task)
{
	if (GsmContext.TaskQueue != NULL)
	{
		if (xQueueSend(GsmContext.TaskQueue, task, 5000) == pdPASS)
			return 1;
	}
	return 0;
}

/*******************************************************************************
* Function Name :	GsmAddTaskTop
* Description   :	Add task to queue to top
*******************************************************************************/
uint8_t GsmAddTaskAlways(GsmTask_t * task)
{
	if (GsmContext.TaskQueue != NULL)
	{
		while (xQueueSend(GsmContext.TaskQueue, task, 500) != pdPASS)
			xQueueReceive(GsmContext.TaskQueue, NULL, 500);
		return 1;
	}
	return 0;
}

/*******************************************************************************
* Function Name :	GsmIMEI
* Description   :	Return pointer to IMEI string
*******************************************************************************/
char * GsmIMEI(void)
{
	return &GsmContext.IMEI[0];
}

/*******************************************************************************
* Function Name	:	GsmDeInit
* Description	:	Deinitialize GSM
*******************************************************************************/
void GsmDeInit(void)
{
	USART_ITConfig(GSM_USART, USART_IT_RXNE, DISABLE);
	USART_Cmd(GSM_USART, DISABLE);
	GSM_USART_CLK(DISABLE);

	GSM_PIN_DEINIT();
	GSM_PWR_OFF();
}

/*******************************************************************************
* Function Name :	GsmInit
* Description   :	Initialize GSM hardware,
*					check communication,
*					read IMEI code
*******************************************************************************/
uint8_t GsmInit(void)
{
	USART_InitTypeDef usart_init;
	uint8_t result = E_GSM_NOT_READY;
	uint8_t retry;
	char *item;

	GsmContext.Commands = &Commands_WS228;

	GSM_PWR_ON();

	vTaskDelay(500);
#ifdef GSM_ON_PIN
	GSM_ON_LOW();
	vTaskDelay(1000);
	GSM_ON_HIGH();
	vTaskDelay(100);
#endif

#ifdef GSM_READY_PIN
	GSM_READY_PREPARE();
	retry = 8;
	while(retry-- != 0)		// Check ready signal from module
	{
		if (GSM_READY())	// Signal set, break
			break;
		vTaskDelay(500);	// Wait delay
	}
	if (!GSM_READY())
	{
		DebugPutLineln("No GSM READY\n");
		GSM_RX_DEINIT();
		GSM_PWR_OFF();
		return result;		// Ready signal not set after timeout
	}
#endif

	// Initialize GSM
	GSM_USART_CLK(ENABLE);

	rb_Init(&GsmContext.RB_Tx, gsmTxBuffer, sizeof(gsmTxBuffer));

	USART_StructInit(&usart_init);
	usart_init.USART_BaudRate = 115200;
	USART_Init(GSM_USART, &usart_init);
	USART_Cmd(GSM_USART, ENABLE);

	GSM_PIN_INIT();

	GsmContext.RxMode = GSM_MODE_IGNORE;
	USART_ITConfig(GSM_USART, USART_IT_RXNE, ENABLE);

	GSM_DTR_LOW();
	GSM_RTS_LOW();

	retry = 8;
	while(retry-- != 0)
	{
		vTaskDelay(500);
		if (E_OK == gsmCmd1(GSM_AT, 2000) &&
			E_OK == gsmCmd0(GSM_E0) &&
			E_OK == gsmCmd0(GSM_K0)
			)
		{
			if (E_OK == gsmCmd0(GSM_ATI3))
			{
				item = TokenBefore(gsmToken(NULL), "OK");
				if (item != NULL && strcmp(item, "Telit") == 0)
					GsmContext.Commands = &Commands_GL865;
			}
			
			if (E_OK == gsmCmd0(GSM_CMEE)
			&&	E_OK == gsmCmd0(GSM_CREG0)
			&&	E_OK == gsmCmd0(GSM_CFUN1)
				)
			{
				gsmCmd0(GSM_COPS0);
				if (GsmContext.Commands->PSSSTK != NULL)
					gsmCmd1(GsmContext.Commands->PSSSTK, 5000);

				if (E_OK == gsmCmd0(GsmContext.Commands->GetIMEI))
				{
					item = TokenBefore(gsmToken(NULL), "OK");
					if (item != NULL && (strlen(item) < (sizeof(GsmContext.IMEI) - 1)))
					{
						strcpy(&GsmContext.IMEI[0], item);
						result = E_OK;
						break;
					}
				}
			}
		}
	}
	return result;
}

/*******************************************************************************
* Function Name	:	gsmCmdX0
* Description	:
*******************************************************************************/
uint8_t gsmCmdX0(const char* str1, char* str2, const char* str3)
{
	gsmSends(str1, str2, NULL);
	return gsmCmd0(str3);
}

/*******************************************************************************
* Function Name	:	gsmCmdX2
* Description	:
*******************************************************************************/
uint8_t gsmCmdX2(const char* str1,
			char* str2,
			const char* str3,
			char* str4,
			const char* str5,
			u16 timeout, const char* wait)
{
	gsmSends(str1, str2, str3, str4, NULL);
	return gsmCmd2(str5, timeout, wait, NULL);
}

/*******************************************************************************
* Function Name	:	gsmWaitNetwork
* Description	:	Wait Network Registration
*******************************************************************************/
void gsmWaitNetwork(GsmContext_t * gc)
{
	int retry;
	GsmDeInit();
	while(1)
	{
		retry = 10;
		while (--retry != 0)
		{
			if (GsmInit() != E_OK)
			{
				ShowError(ERROR_GSM_NOT_READY);
				GsmDeInit();
				vTaskDelay(5000);
				continue;
			}

			if (gsmWaitRegister(gc) == E_OK)
			{
				gsmScanSMS(gc);
				return;
			}

			if (gc->Error == E_SIM_NOT_READY)
				ShowError(ERROR_GSM_SIM_NOT_READY);
			else if (gc->Error == E_NOT_REGISTER)
			{
				ShowError(ERROR_GSM_NOT_REGISTER);
			}
			else if (gc->Error == E_REG_DENIED)
			{
				gsmCmd0("AT#MONI\r\n");
				vTaskDelay(1000);
				ShowError(ERROR_GSM_REG_DENIED);
			}
			else if (gc->Error == E_NO_SIM)
				ShowError(ERROR_GSM_NO_SIM);
			else
				ShowError(ERROR_GSM_OTHER);
		}

		GsmDeInit();
		vTaskDelay(5000);
	}
}

/*******************************************************************************
* Function Name	:	gsmDisconnectTCP
* Description	:	Disconnect TCP
*******************************************************************************/
void gsmDisconnectTCP(GsmContext_t * gc)
{
	gc->Status &= ~GSM_STATUS_TCP_ON;
	vTaskDelay(1000);
	if (gsmCmd2("+++", 10000, GSM_NO_CARRIER, NULL) == E_OK
	||	gsmCmd0(GSM_AT) == E_OK
		)
	{
		gsmCmd0(gc->Commands->TcpClose);
		(*gc->Commands->GprsStop)();
		vTaskDelay(500);
		if (gsmIsRegister(gc) != E_OK)
		{
			GsmDeInit();
			gsmWaitNetwork(gc);
		}
	}
	else
	{
		GsmDeInit();
		gsmWaitNetwork(gc);
	}
}

uint8_t gsmHostUndefine(void)
{
	return (SET_HOST_ADDR[0] == 0 ? 1 : 0);
}

/*******************************************************************************
* Function Name	:	gsmConnectTCP
* Description	:	Connect TCP
*******************************************************************************/
uint8_t gsmConnectTCP(GsmContext_t * gc)
{
	uint8_t last_error;
	if (gc->Status & GSM_STATUS_TCP_ON)
	{
		gc->Error = E_OK;
	}
	else
	{
		if ((*gc->Commands->GprsStop)() == E_TIMEOUT)
			gsmDisconnectTCP(gc);

		gsmScanSMS(gc);

		if (gsmHostUndefine())
			return (gc->Error = E_GSM_TCP_NOT_OPEN);

		if (	E_OK == gsmWaitRegister(gc)
			&&	E_OK == gsmCmd0(gc->Commands->StartIP)
			&&	E_OK == (*gc->Commands->GprsSet)()
			&&	E_OK == (*gc->Commands->GprsStart)()
			&&	E_OK == (*gc->Commands->TcpOpen)()
			&&	E_OK == gsmCmd2(gc->Commands->TcpData, GSM_TCP_TIMEOUT, NULL, GSM_CONNECT)
			)
		{
			gc->RxMode = GSM_MODE_IGNORE;
			gsmRxFlush(gc);
			gc->RxMode = GSM_MODE_STREAM;
			gc->Error = E_OK;
			gc->Status |= GSM_STATUS_TCP_ON;
		}
		else
		{
			last_error = GsmContext.Error;
			(*gc->Commands->GprsStop)();
			gc->Error = last_error;
		}
	}
	return gc->Error;
}

/*******************************************************************************
* Function Name	:	gsmDeleteSMS
* Description	:
*******************************************************************************/
uint8_t gsmDeleteSMS(int index)
{
	sprintf(gsmBuffer, "AT+CMGD=%d\r\n", index);
	return gsmCmd0(gsmBuffer);
}

/*******************************************************************************
* Function Name	:	gsmGetHostLine
* Description	:
*******************************************************************************/
uint8_t gsmGetHostLine(GsmContext_t *gc, int ms_delay)
{
	char data;
	int retry;

	gc->HostIndex = 0;
	gc->SkipHostLine = 0;
	gc->RxMode = GSM_MODE_STREAM;
	retry = ms_delay;
	
	while (retry != 0)
	{
		while (rb_Get(&gc->RB_Rx, &data) != 0)
		{
			retry = ms_delay;
			if (gc->SkipHostLine == 1)
			{
				if (data == 0)	// Check for End Of Line
				{
					gc->SkipHostLine = 0;
					gc->HostIndex = 0;
				}
				continue;
			}
			if (gc->HostIndex >= (sizeof(gsmBuffer) - 2))
			{	// Line too long, ignore it
				gc->SkipHostLine = 1;
				continue;
			}
			gsmBuffer[gc->HostIndex++] = data;
			if (data == 0)
				return E_OK;
		}
		vTaskDelay(5);
		retry -= 5;
	}
	return E_TIMEOUT;
}

/*******************************************************************************
* Function Name	:	gsmSendTaskCommand
* Description	:	Send message to host
*******************************************************************************/
int32_t D2I_Int, D2I_Frac;
const double D2I_CONST[] = {
	0.5,		1.0,
	0.05,		10.0,
	0.005,		100.0,
	0.0005,		1000.0,
	0.00005,	10000.0,
	0.000005,	100000.0,
	0.0000005,	1000000.0
};
void D2I_6D(double val, uint8_t digits)
{
	double  dbl2int;
	digits *= 2;
	D2I_Frac = (uint32_t)((modf(val, &dbl2int) + D2I_CONST[digits]) * D2I_CONST[digits + 1]);
	D2I_Int = (uint32_t)dbl2int;
}

/*******************************************************************************
* Function Name	:	gsmSendTaskCommand
* Description	:	Send message to host
*******************************************************************************/
char * D2IToString(char *dst, double value, uint8_t digits)
{
	D2I_6D(value, digits);
	dst = itoa(dst, D2I_Int, 0);
	*dst++ = '.';
	dst = itoa(dst, D2I_Frac, digits);
	*dst++ = ',';
	return dst;
}

/*******************************************************************************
* Function Name	:	gsmSendTaskCommand
* Description	:	Send message to host
*******************************************************************************/
uint8_t gsmSendTaskCommand(GsmContext_t *gc)
{
	const GgCommand_t *cmd;
	int send_size = 0;
	uint8_t result = E_ERROR;
	GsmTask_t * task = &GsmTask;
	char *dst = gsmBuffer;

#if ( USE_CAN_SUPPORT == 1 )
	uint8_t idx;
#endif

	if (gc->Status & GSM_STATUS_TCP_DISABLE)
	{
		ShowError(ERROR_GSM_TCP_DISABLE);
		return E_GSM_TCP_DISABLE;
	}

	if (task->Command == GSM_TASK_CMD_SMS)
	{
		if (task->SmsIndex != 0)
		{
			gsmDisconnectTCP(gc);
			cmd = task->GG_Command;
			if (cmd == NULL
			||	cmd->Response == NULL
			||	((cmd->Response)(gsmBuffer, sizeof(gsmBuffer), true, cmd) == E_OK)
				)
			{
				gsmDeleteSMS(task->SmsIndex);
			}
		}
	}
	else if (gsmConnectTCP(gc) == E_OK)
	{
		if (task->Command == GSM_TASK_CMD_GPS)
		{
			nmeaGPRMC * rmc = &(task->GPRMC);

			dst = strcpyEx( strcpyEx( strcpyEx( gsmBuffer, "$FRCMD,"), GsmIMEI()), ",_SendMessage,,");

#if ( USE_AVERAGE == 1 || USE_KALMAN == 1 )
				rmc->ns = ((rmc->lat > 0) ? 'N' : 'S');
				rmc->ew = ((rmc->lon > 0) ? 'E' : 'W');
				rmc->lat = nmea_degree2ndeg(fabs(rmc->lat));
				rmc->lon = nmea_degree2ndeg(fabs(rmc->lon));
#endif
			dst = D2IToString(dst, rmc->lat, 6);	*dst++ = rmc->ns;	*dst++ = ',';
			dst = D2IToString(dst, rmc->lon, 6);	*dst++ = rmc->ew;	*dst++ = ',';
			*dst++ = ',';
			
			dst = D2IToString(dst, rmc->speed, 1);
			dst = D2IToString(dst, rmc->direction, 1);

			dst = itoa(dst, rmc->utc.day, 2);
			dst = itoa(dst, rmc->utc.mon + 1, 2);
			dst = itoa(dst, ((rmc->utc.year > 2000) ? rmc->utc.year - 2000 : rmc->utc.year), 2);
			*dst++ = ',';

			dst = itoa(dst, rmc->utc.hour, 2);
			dst = itoa(dst, rmc->utc.min, 2);
			dst = itoa(dst, rmc->utc.sec, 2);
			*dst++ = '.';
			dst = itoa(dst, rmc->utc.hsec, 3);
			dst = strcpyEx( itoa( strcpyEx(dst, ",1,Switch1="), task->SenseD1, 1), VERSION);
			
#if (USE_CAN_SUPPORT)
			// Format: obd=N,value[,value]
			//	N		number of values
			//	value	CAN parameter value
			dst = strcpyEx(dst, ",obd=");
			// dst = itoa(dst, CAN_VALUES_SIZE, 0);

			for (idx = 0; idx < CAN_VALUES_SIZE; idx ++)
				dst = D2IToString(dst, task->CanData[idx], 3);

			--dst;
#endif
			// Add NMEA checksum
			send_size = NmeaAddChecksum(dst, gsmBuffer);
			result = E_OK;
		}
		else if (task->Command == GSM_TASK_CMD_PING)
		{
			dst = strcpyEx(strcpyEx(strcpyEx(gsmBuffer, "$FRCMD,"), GsmIMEI()), ",_SendMessage,,,,,,,,,,,0,Switch1=");
			dst = itoa(dst, task->SenseD1, 1);
			dst = strcpyEx(dst, VERSION);

#if (USE_CAN_SUPPORT)
			// Format: obd=N,value[,value]
			//	N		number of values
			//	value	CAN parameter value
			dst = strcpyEx(dst, ",obd=");
			// dst = itoa(dst, CAN_VALUES_SIZE, 0);

			for (idx = 0; idx < CAN_VALUES_SIZE; idx ++)
				dst = D2IToString(dst, task->CanData[idx], 3);

			--dst;
#endif
			// Add NMEA checksum
			send_size = NmeaAddChecksum(dst, gsmBuffer);
			result = E_OK;
		}
		else
		{
			result = E_NO_RESPONSE;
			if ((cmd = task->GG_Command) != NULL)
			{
				if (task->Command == GSM_TASK_CMD_RET)
				{
					if (cmd->Response != NULL
					&&	(cmd->Response)(gsmBuffer, sizeof(gsmBuffer), false, cmd) == E_OK
						)
						send_size = 1;
					if (task->SmsIndex != 0)
					{
						if (cmd->Counts == NULL
						||	(cmd->Response != NULL && (cmd->Response)(NULL, task->SmsIndex, true, cmd) == E_OK)
							)
						{	// Add post processing SMS
							task->Command = GSM_TASK_CMD_SMS;
							GsmAddTask(task);
						}
					}
				}
				else if (task->Command == GSM_TASK_CMD_ERR)
				{
					dst = strcpyEx( strcpyEx( gsmBuffer, DGG_FRERR ), GsmIMEI() );
					*dst++ = ',';
					send_size = NmeaAddChecksum(strcpyEx(dst, cmd->Command), gsmBuffer);

					if (task->SmsIndex != 0)
					{	// Add post processing SMS
						task->Command = GSM_TASK_CMD_SMS;
						GsmAddTask(task);
					}
				}
			}
		}

		if (send_size != 0)
		{
			DebugPutLine("> ");
			gsmSend(gsmBuffer);
		}
		else
			result = E_NO_RESPONSE;
	}
	else
	{
		GsmAddTask(task);
		ShowError(ERROR_GSM_TCP_NOT_OPEN);
		result = E_GSM_TCP_NOT_OPEN;
	}

	return result;
}

/*******************************************************************************
* Function Name	:	gsmProcessHostResponse
* Description	:	Process command from host
*******************************************************************************/
void gsmProcessHostResponse(GsmContext_t * gc, bool disconnect)
{
	bool reply = true;
	const GgCommand_t *cmd;

	while(reply)
	{
		if (gsmGetHostLine(gc, (disconnect ? 15000 : 1000)) != E_OK)
		{
			if (disconnect)
				gsmDisconnectTCP(gc);
			break;
		}
		// Ignore empty response string
		if (strlen(gsmBuffer) == 0)
			continue;

		DebugPutLine("< ");
		DebugPutLineln(gsmBuffer);
		
		//	gsmLastReplyTime = xTaskGetTickCount();
		//	Process host line
		if (strncmp(gsmBuffer, GG_FRRET, GG_FRRET_S) == 0)
		{
			reply = false;
		}
		else if (strncmp(gsmBuffer, GG_FRCMD, GG_FRCMD_S) == 0)
		{
			cmd = ProcessGpsGateCommand(gsmBuffer, true);
			GsmTask.Command = GSM_TASK_CMD_ERR;
			GsmTask.GG_Command = cmd;
			GsmTask.SmsIndex = 0;
			if (cmd != NULL && cmd->Response != NULL)
			{
				if ((cmd->Response)(gsmBuffer, sizeof(gsmBuffer), true, cmd) == E_OK)
				{
					gsmDisconnectTCP(gc);
					gsmSaveSMS(SMS_CENTER_1, gsmBuffer);
				}
				GsmTask.Command = GSM_TASK_CMD_RET;
			}
			GsmAddTask(&GsmTask);
			disconnect = false;
		}
		else if (strncmp(gsmBuffer, GG_FRERR_AUTH, GG_FRERR_AUTH_S) == 0)
		{	// Authentication error
			gc->AuthErrorCount++;
			if (gc->AuthErrorCount >= 2)
			{
				gc->Status |= GSM_STATUS_TCP_DISABLE;
				gsmDisconnectTCP(gc);
				break;
			}
		}
		else if (
				strcmp(gsmBuffer, GSM_SHUTDOWN) == 0
			||	strncmp(gsmBuffer, GSM_NO_CARRIER, (sizeof(GSM_NO_CARRIER)-1)) == 0
			||	strncmp(gsmBuffer, GSM_WIPPEERCLOSE, (sizeof(GSM_WIPPEERCLOSE)-1)) == 0
				)
		{
			gsmDisconnectTCP(gc);
			break;
		}
	}
}

/*******************************************************************************
* Function Name	:	gsmSendSMS
* Description	:
*******************************************************************************/
uint8_t gsmSendSMS(const char *da, const char *msg)
{
	gsmSends("AT+CMGS=", da, NULL);
	if (gsmCmd2(GSM_CRLF, 2000, NULL, "> ")  == E_OK)
	{
		gsmSend(msg);
		if (gsmCmd1("\x1A", 20000) != E_OK)
			gsmSend("\x1B");
	}
	return GsmContext.Error;
}

/*******************************************************************************
* Function Name	:	gsmSaveSMS
* Description	:
*******************************************************************************/
uint8_t gsmSaveSMS(const char *da, const char *msg)
{
	gsmSends("AT+CMGW=", da, ",145,\"STO SENT\"", NULL);
	if (gsmCmd2("\r\n", 2000, NULL, "> ")  == E_OK)
	{
		gsmSend(msg);
		return gsmCmd1("\x1A", 20000);
	}
	return GsmContext.Error;
}

/*******************************************************************************
* Function Name	:	gsmGetNextIMEI
* Description	:
*******************************************************************************/
char * gsmGetNextIMEI(char * cmd)
{
	char * token = TokenNextComma(cmd);
	if (token == NULL || strncmp(token, GsmContext.IMEI, strlen(GsmContext.IMEI)) != 0)
		return NULL;
	return token;
}

/*******************************************************************************
* Function Name	:	gsmGetNextIMEI
* Description	:
*******************************************************************************/
uint8_t gsmScanSMSInt(GsmContext_t * gc, bool not_delete)
{
	bool is_text;
	int smsIndex;
	int nlen;
	char * token;
	uint32_t smsDate;
	int smsDelete;
	uint8_t ntoken;
	const GgCommand_t *cmds;

sms_rescan:
	smsIndex = 0;
	smsDelete = 0;
	smsDate = 0;
	is_text = false;
	
	gsmRxFlush(gc);
	gc->RxMode = GSM_MODE_STREAM;
	gsmSend(GSM_CMGL);

	SET_HOST_ADDR[0] = 0;
	GG_ClearAllCounts();
	GlobalStatus &= ~SMS_INIT_OK;

	while (gsmGetHostLine(gc, 2000) == E_OK)
	{
		DebugPutLineln(gsmBuffer);

		nlen = strlen(gsmBuffer);
		if (is_text)
		{
			is_text = false;
			if (smsDelete != 0)
				continue;

			if (nlen == 0)
			{
sms_delete:		if (!not_delete && smsIndex != 0)
					smsDelete = smsIndex;
				continue;
			}

			cmds = ProcessGpsGateCommand(gsmBuffer, not_delete);
			if (cmds == NULL)
			{	// Bad IMEI or unknown command
				if (not_delete)
				{
				}
				else
				{	// Scan pass - delete this sms
					goto sms_delete;
				}
			}
			else if (not_delete)
			{
				if (cmds->Response != NULL)
					(cmds->Response)(NULL, smsIndex, true, cmds);
			}
			else if (cmds->Counts != NULL)
			{	// Scan pass - check for duplicated sms
				if (smsDate != 0)
				{
					if (cmds->Counts->Count == 0)
					{
						cmds->Counts->Count++;
						cmds->Counts->DateTime = smsDate;
						cmds->Counts->Index = smsIndex;
					}
					else if (smsDate > cmds->Counts->DateTime)
					{
						nlen = cmds->Counts->Index;
						cmds->Counts->Count++;
						cmds->Counts->DateTime = smsDate;
						cmds->Counts->Index = smsIndex;
						smsIndex = nlen;
						goto sms_delete;
					}
					else
						goto sms_delete;
				}
			}
			continue;
		}
		if (nlen == 0)
			continue;
		if (strcmp(gsmBuffer, "OK") == 0)
		{
			if (smsDelete != 0)
			{
				if (gsmDeleteSMS(smsDelete) != E_OK)
				{	// Delete failed, so delete all
					gsmCmd1("AT+CMGD=1,4\r\n", 2000);
					return E_ERROR;
				}
				goto sms_rescan;
			}
			return E_OK;
		}
		if (strncmp(gsmBuffer, "+CMGL:", 6) == 0)
		{
			is_text = true;
			if (smsDelete != 0)
				continue;

			//!!! memset(&gsmSMS, 0, sizeof(SMS_t));
			smsDate = 0;
			ntoken = 0;
			token = gsmBuffer;
			do
			{
				nlen = TokenSizeComma(token);
				if (ntoken == 0)
				{
					/*
					int nsize = TokenSizeQuote(token);
					if (nlen > nsize)
					{	// no sms index if AT+CMGR=n
						ntoken++;
						continue;
					}
					*/
					if (nlen < 8)
						break;
					smsIndex = nmea_atoi(&gsmBuffer[6], 5, 10);
					//!!! gsmSMS.Index = smsIndex;
				}
				else if (ntoken == 2)
				{
					if (nlen == 0 || nlen > sizeof(gsmSenderDA))
						break;	// No or bad Sender DA

					strncpy(gsmSenderDA, token, nlen);
					gsmSenderDA[nlen] = 0;
					//!!! strncpy(&gsmSMS.DA[0], token, nlen);
				}
				else if (ntoken == 4)
				{	// Date
					if (nlen > 0)
					{
						int yy,mm,dd;
						if (nmea_scanf(token, nlen, "\"%2d/%2d/%2d", &yy, &mm, &dd) == 3)
						{
							smsDate = ((yy - 10) * 365 + mm * 12 + dd) * 24 * 60 * 60;
							//!!! gsmSMS.DateTime = smsDate;
						}
						else
							break;
					}
				}
				else if (ntoken == 5)
				{	// Time
					if (nlen > 0)
					{
						int hh,mm,ss;
						if (nmea_scanf(token, nlen, "%2d:%2d:%2d", &hh, &mm, &ss) == 3)
						{
							smsDate += (hh * 24 + mm) * 60 + ss;
							//!!! gsmSMS.DateTime = smsDate;
						}
						else
							break;
					}
				}
				ntoken++;
			} while (ntoken < 6 && (token = TokenNextComma(token)) != NULL);

			if (ntoken != 6 && ntoken != 4)
				goto sms_delete;
		}
	}
	return E_TIMEOUT;
}
/*******************************************************************************
* Function Name	:	gsmScanSMS
* Description	:
*******************************************************************************/
void gsmScanSMS(GsmContext_t * gc)
{
	if	(
		gsmCmd0(GSM_CMGF) == E_OK
	&&	gsmCmd0(GSM_CSDH) == E_OK
	&&	gsmCmd0(GSM_CNMI) == E_OK
	&&	gsmScanSMSInt(gc, false) == E_OK
	&&	gsmScanSMSInt(gc, true) == E_OK
	&&	(GlobalStatus & (SMS_INIT_OK | SMS_INIT_SENT)) == 0
		)
	{
		gsmCmd0("AT+CSCA?\r\n");

		GlobalStatus |= SMS_INIT_SENT;
		DebugPutLineln("Send INIT SMS");

		NmeaAddChecksum(
			strcpyEx(
			strcpyEx(
			strcpyEx( gsmBuffer, "$FRCMD," ),
				GsmIMEI()),
				",_InitSMS,,0"),
			gsmBuffer);

		if (gsmSendSMS(SMS_CENTER_1, gsmBuffer) == E_OK)
		{
			gsmSaveSMS(SMS_CENTER_1, gsmBuffer);
			gsmSendSMS(SMS_CENTER_2, gsmBuffer);
		}
	}
}

/*******************************************************************************
* Function Name	:	gsmWaitRegister
* Description	:	Wait GSM registration
*******************************************************************************/
uint8_t gsmWaitRegister(GsmContext_t * gc)
{
	uint8_t retry = 10;

	// Wait for SIM ready
	while (retry-- != 0)
	{
		gsmCheckPIN(gc);
		if (gc->Error == E_OK || gc->Error == E_TIMEOUT)
			break;
		if (gc->Error == E_NO_SIM)
			vTaskDelay(1000);
		else
			vTaskDelay(2000);
	}

	if (gc->Error == E_OK)
	{	// Wait for registration
		retry = 12;
		while (retry-- != 0 && gsmIsRegister(gc) != E_OK)
			vTaskDelay(5000);
	}
	return gc->Error;
}

/*******************************************************************************
* Function Name	:	gsmCheckPIN
* Description	:	Check SIM ready
*******************************************************************************/
uint8_t gsmCheckPIN(GsmContext_t * gc)
{
	if (gsmCmd0(GSM_GET_CPIN) == E_OK)
	{
		if (FindToken(gsmToken(NULL), GSM_CPIN_RDY) == NULL)
			gc->Error = E_SIM_NOT_READY;
		else if (FindToken(gsmToken(NULL), GSM_NO_SIM) != NULL)
			gc->Error = E_NO_SIM;
	}
	return gc->Error;
}

/*******************************************************************************
* Function Name	:	gsmIsRegister
* Description	:	Check registration (CREG and CGATT)
*******************************************************************************/
uint8_t gsmIsRegister(GsmContext_t * gc)
{
	gc->Status &= ~GSM_STATUS_REGISTER;
	if (gsmCmd0(GSM_CREG) == E_OK)
	{
		if (FindToken(gsmToken(NULL), GSM_CREG01) != NULL
		||	FindToken(gsmToken(NULL), GSM_CREG05) != NULL
			)
		{
			if (gsmCmd0(GSM_CGATT) == E_OK
			&&	FindToken(gsmToken(NULL), GSM_CGATT1) == NULL
				)
				gc->Error = E_NOT_REGISTER;
			else
				gc->Status |= GSM_STATUS_REGISTER;
		}
		else if (FindToken(gsmToken(NULL), GSM_CREG03) != NULL)
		{
			gc->Error = E_REG_DENIED;
		}
		else
			gc->Error = E_NOT_REGISTER;
	}
	return gc->Error;
}

/*******************************************************************************
* Function Name	:	gsmSend
* Description	:	Put string to GSM TX queue
* Input			:	string
* Return		:	result code
*******************************************************************************/
void gsmSend(const char *text)
{
	char c;
	DebugPutLine(text);
	while ((c = *text++) != 0)
	{
		while (!rb_Put(&GsmContext.RB_Tx, c))
		{	// Send data to ring buffer
			USART_ITConfig(GSM_USART, USART_IT_TXE, ENABLE);
			vTaskDelay(5);
		}
	}
	USART_ITConfig(GSM_USART, USART_IT_TXE, ENABLE);
}

/*******************************************************************************
* Function Name	:	gsmSends
* Description	:
*******************************************************************************/
void gsmSends(const char * cmd, ...)
{
	va_list arg_ptr;
	const char *p = cmd;

	va_start(arg_ptr, cmd);
	while (p != NULL)
	{
		gsmSend(p);
		p = (const char *)va_arg(arg_ptr, const char *);
	}
	va_end(arg_ptr);
}

/*******************************************************************************
* Function Name	:	gsmToken
* Description	:	Get first/next GSM token
* Input			:	NULL for first, previous token for next
* Return		:	next token or NULL
*******************************************************************************/
char * gsmToken(char * src)
{
	if (src == NULL)
		src = gsmRxBuffer;
	else
		while(*src++ != 0);
	return (*src == 0 ? NULL : src);
}

/*******************************************************************************
* Function Name	:	gsmShowResponse
* Description   :	Display response from GSM
*******************************************************************************/
void gsmShowResponse(void)
{
	char * token = gsmToken(NULL);
	if (token != NULL)
	{
		DebugPutLineln(token);
	}
	/*
	while(token != NULL)
	{
		DebugPutLineln(token);
		token = gsmToken(token);
	}
	*/
}

/*******************************************************************************
* Function Name	:	gsmShowError
* Description   :	Display last GSM Error
*******************************************************************************/
void gsmShowError(void)
{
	if (GsmContext.Error == E_OK)
		DebugPutLineln("E_OK");
	else if (GsmContext.Error == E_ERROR)
		DebugPutLineln("E_ERROR");
	else if (GsmContext.Error == E_TIMEOUT)
		DebugPutLineln("E_TIMEOUT");
	else if (GsmContext.Error == E_SEND_ERROR)
		DebugPutLineln("E_SEND_ERROR");
	else
		DebugPutLineln("Unknown E_?");
}

/*******************************************************************************
* Function Name	:	gsmRxFlush
* Description   :
*******************************************************************************/
void gsmRxFlush(GsmContext_t * gc)
{
	gc->RxMode	= GSM_MODE_IGNORE;
	rb_Init(&gc->RB_Rx, gsmRxBuffer, sizeof(gsmRxBuffer)-4);
	gc->Error	= E_TIMEOUT;
	// Clear semaphore
	while(xSemaphoreTake(gc->ReadySemaphore, 1) == pdTRUE)
	{ }
	gc->RxMode	= GSM_MODE_AT;
}

/*******************************************************************************
* Function Name	:	gsmCmd0
* Description   :	Send string to GSM and wait response within 500ms
* Input         :	string to send
* Return		:	result code
*******************************************************************************/
uint8_t gsmCmd0(const char* cmd)
{
	return gsmCmd2(cmd, 500, NULL, NULL);
}

/*******************************************************************************
* Function Name	:	gsmCmd1
* Description   :	Send string to GSM and wait response with timeout
* Input         :	string to send
*				:	timeout
* Return		:	result code
*******************************************************************************/
uint8_t gsmCmd1(const char* text, uint16_t timeout)
{
	return gsmCmd2(text, timeout, NULL, NULL);
}
uint8_t gsmCmd1E(const char* text, uint16_t timeout)
{
	return gsmCmd2(text, timeout, NULL, NULL);
}

/*******************************************************************************
* Function Name	:	gsmCmd2
* Description   :	Send string to GSM and wait response with timeout or alive,
*				:	always check ERROR, +CME ERROR: and +CMS ERROR:
* Input         :	string to send
*				:	timeout
*				:	wait for token for complete
*				:	wait for alive token for complete
* Return		:	result code
*******************************************************************************/
uint8_t gsmCmd2(const char* text, uint16_t timeout, const char* waitFor, const char* waitAlive)
{
	GsmContext_t * gc = &GsmContext;

	if (text == NULL)
		return (gc->Error = E_OK);

	gc->RxMode	= GSM_MODE_IGNORE;
	gc->WaitAlive = (char *)waitAlive;
	gc->WaitAliveLen = (gc->WaitAlive == NULL) ? 0 : strlen(gc->WaitAlive);
	gc->RxWaitFor = (waitFor == NULL) ? GSM_OK : waitFor;
	gsmRxFlush(gc);

	gsmSend(text);

	// Wait for response and set ignore on timeout
	if (xSemaphoreTake(gc->ReadySemaphore, timeout) != pdTRUE)
		gc->RxMode = GSM_MODE_IGNORE;
#if ( USE_DEBUG )
	else
		gsmShowResponse();
	gsmShowError();
#endif
	return (gc->Error);
}

/*******************************************************************************
* Function Name	:	GSM_USART_IRQHandler
* Description   :	GSM iterrupt handler
*******************************************************************************/
void GSM_USART_IRQHandler(void)
{
	USART_TypeDef * port = GSM_USART;
	register GsmContext_t *gc = &GsmContext;

	if (USART_GetITStatus(port, USART_IT_RXNE) != RESET)
	{
		gc->Data = port->DR;
		if ( port->SR & (USART_FLAG_ORE | USART_FLAG_NE | USART_FLAG_FE | USART_FLAG_PE))
		{
			gc->Data = port->DR;
		}
		else if (gc->Data != '\r')
		{
			if (gc->Data == '\n')	// Receive End Of Line
				gc->Data = 0;

			if (gc->RxMode == GSM_MODE_AT)
			{
				// Save data to buffer (use as linear)
				if (gc->Data == 0)
				{
					if (gc->RB_Rx.Start != gc->RB_Rx.Tail)// Check for empty token and ignore it
					{
						*gc->RB_Rx.Tail++ = 0;
						if (strcmp(gc->RB_Rx.Start, gc->RxWaitFor) == 0)
						{
							*gc->RB_Rx.Tail = 0;		// Write Double 0 as End of tokens
							gc->Error = E_OK;
						}
						else if (strcmp(gc->RB_Rx.Start, GSM_ERROR) == 0
							||	strncmp(gc->RB_Rx.Start, GSM_CME_ERROR, sizeof(GSM_CME_ERROR) - 1) == 0
							||	strncmp(gc->RB_Rx.Start, GSM_CMS_ERROR, sizeof(GSM_CMS_ERROR) - 1) == 0
								)
						{
							*gc->RB_Rx.Tail = 0;		// Write Double 0 as End of tokens
							gc->Error = E_ERROR;
						}
						else
							gc->RB_Rx.Start = gc->RB_Rx.Tail;
					}
				}
				else if (gc->RB_Rx.Tail >= gc->RB_Rx.End)
				{
					gc->Error = E_BAD_RESPONSE;
				}
				else
				{
					*gc->RB_Rx.Tail++ = gc->Data;
					if (gc->WaitAlive != NULL
					&&	(gc->RB_Rx.Tail - gc->RB_Rx.Start) == gc->WaitAliveLen
						)
					{
						// Check for non-CRLF tokens (such as CONNECT)
						if (strncmp(gc->RB_Rx.Start, gc->WaitAlive, gc->WaitAliveLen) == 0)
						{
							*gc->RB_Rx.Tail++ = 0;		// Write Double 0 as End of tokens
							*gc->RB_Rx.Tail   = 0;
							gc->Error = E_OK;
						}
					}
				}

				if (gc->Error != E_TIMEOUT)
				{
					portBASE_TYPE RxNeedTaskSwitch;
					gc->RxMode = GSM_MODE_IGNORE;
					RxNeedTaskSwitch = pdFALSE;
					xSemaphoreGiveFromISR( gc->ReadySemaphore, &RxNeedTaskSwitch );
					portEND_SWITCHING_ISR( RxNeedTaskSwitch );
				}
			}
			else if (gc->RxMode == GSM_MODE_STREAM)
			{
				rb_Put(&gc->RB_Rx, gc->Data);
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

/*******************************************************************************
* Function Name	:	str2hex
* Description	:	Convert hex character to byte
* Return		:	true	char is hex char
*				:	false	bad character
*******************************************************************************/
bool str2hex(char c, uint8_t *data)
{
	if (c >= '0' && c <= '9')
		*data = (c - '0');
	else
	{
		if (c >= 'a')	// Check and convert to upper
			c -= ('a' - 'A');
		if (c >= 'A' && c <= 'F')
			*data = c - ('A' - 0xA);
		else
			return false;
	}
	return true;
}

/*******************************************************************************
* Function Name	:	gsmGetFileFTP
* Description	:
*******************************************************************************/
uint8_t gsmGetFileFTP(GsmContext_t *gc, char * filename)
{
	char * token, *dst;
	int nlen, nlen2;
	uint8_t low, high;
	uint8_t result = E_ERROR;
	uint32_t fw_address = FW_COPY_START;

	if (!gsmHostUndefine())
	{
		if ((gc->Status & GSM_STATUS_TCP_ON) || (*gc->Commands->GprsStop)() == E_TIMEOUT)
			gsmDisconnectTCP(gc);

		if (E_OK == gsmWaitRegister(gc)
		&&	E_OK == gsmCmd0(gc->Commands->StartIP)
		&&	E_OK == (*gc->Commands->GprsSet)()
		&&	E_OK == (*gc->Commands->GprsStart)()
			)
		{
			if (E_OK == gsmCmd0("AT#FTPTO=500\r\n"))
			{
				vTaskDelay(200);
				gsmSends("AT#FTPOPEN=\"", SET_HOST_ADDR, "\",\"anonymous\",\"fwt@mail.com\",0", NULL);
				if (E_OK == gsmCmd1(GSM_CRLF, 50000))
				{
					vTaskDelay(100);
					gsmCmd1("AT#FTPTYPE=0\r\n", 10000);
					gsmSends("AT#FTPGETPKT=\"", filename, "\",1", NULL);
					if (E_OK == gsmCmd1(GSM_CRLF, 20000))
					{
						FwWriteBlock(NULL, NULL, 0);

						for(;;)
						{
							vTaskDelay(100);
							result = E_ERROR;
							if (E_OK != gsmCmd1("AT#FTPGETPKT?\r\n", 10000))
								break;

							token = FindTokenStartWith(gsmToken(NULL), "#FTPGETPKT:");
							token = TokenNextComma(token);
							token = TokenNextComma(token);
							if (token == NULL)
								break;

							if (*token == '1')
							{	// *token == '1' file download complete
								result = E_OK;
								break;
							}
							
							if (E_OK != gsmCmd1("AT#FTPRECV=512\r\n", 15000)
							||	(token = FindTokenStartWith(gsmToken(NULL), "#FTPRECV: ")) == NULL
							||	(nlen = nmea_atoi(token + (sizeof("#FTPRECV: ") - 1), 5, 10)) == 0
							||	(token = gsmToken(token)) == NULL
							||	strlen(token) != nlen * 2
								)
								break;

							dst = gsmBuffer;
							nlen2 = nlen;
							for (; nlen != 0; --nlen)
							{
								if (!str2hex(*token++, &high)
								||	!str2hex(*token++, &low)
									)
									break;
								*dst++ = (high << 4 | low);
							}
							if (nlen != 0)
								break;

							vTaskDelay(100);
							Disable_WWDG();
							result = FwWriteBlock(fw_address, (uint8_t *)gsmBuffer, nlen2);
							Enable_WWDG();
							fw_address += nlen2;
							if (result != E_OK)
								break;
						}
					}
					gsmCmd1("AT#FTPCLOSE\r\n", 5000);
				}
			}
			if (result == E_OK)
			{	// File downloaded successfully.
				if (!FwCheckImage(FW_COPY_START))
					result = E_FW_ERROR;
			}
		}
		(*gc->Commands->GprsStop)();
		gsmRxFlush(gc);
	}
	return result;
}

/*******************************************************************************
* Function Name	:	vGsmTask
* Description	:	GSM Task
*******************************************************************************/
void vGsmTask(void *pvArg)
{
	register GsmContext_t *gc = &GsmContext;

#ifdef DISPLAY_STACK
	DisplayStackHigh("*** GSM Stack 0:");
#endif

	vSemaphoreCreateBinary( gc->ReadySemaphore );
	if (gc->ReadySemaphore == NULL)
		ShowFatalError(ERROR_GSM_TASK_FAIL);
	xSemaphoreTake( gc->ReadySemaphore, 1 );

	gc->TaskQueue = xQueueCreate(GSM_TASK_QUEUE_SIZE, sizeof(GsmTask_t));
	if (gc->TaskQueue == NULL)
		ShowFatalError(ERROR_GSM_TASK_FAIL);

#ifdef DISPLAY_STACK
	DisplayStackHigh("*** GSM Stack 1:");
#endif

	gc->PingTick = 60;
	while (gc->PingTick != 0 && (GlobalStatus & GPS_COMPLETE) == 0)
	{
		gc->PingTick--;
		vTaskDelay(1000);
	}

	gc->PingInterval = GSM_PING_MS * configTICK_RATE_HZ;
	gc->Status = 0;
	gc->AuthErrorCount = 0;
	gc->IMEI[0] = 0;

	GlobalStatus |= GSM_COMPLETE;

	// Initial delay w/o transmite any so GPS can fix up quickly
	gc->LastReplyTime = xTaskGetTickCount();
	while((xTaskGetTickCount() - gc->LastReplyTime) < GSM_INITIAL_WAIT)
	{
		if (uxQueueMessagesWaiting(gc->TaskQueue))
			break;
		vTaskDelay(5000);
	}

	gsmWaitNetwork(gc);

gsm_restart:
	while (gsmHostUndefine())
	{
		vTaskDelay(5000);
		gsmScanSMS(gc);
		if (gsmIsRegister(gc) != E_OK)
			gsmWaitNetwork(gc);
	}

	// Set next time for send Ping
	gc->PingTick = xTaskGetTickCount() + 60000; // first time ping period longer

#ifdef DISPLAY_STACK
	DisplayStackHigh("*** GSM Stack 2:");
#endif

	for(;;)
	{
		GlobalStatus &= ~GPS_STOP;
		if (xQueueReceive(gc->TaskQueue, &GsmTask, 2000) != pdPASS)
		{
			// No command for GSM, check timeout and make a Ping
			if (xTaskGetTickCount() < gc->PingTick)
			{	// No message to host, check host nessages
				if (gc->Status & GSM_STATUS_TCP_ON)
					gsmProcessHostResponse(gc, false);
				continue;
			}
			else
			{
				// Send ping if no task during GSM_PING_MS ms
				GsmTask.Command = GSM_TASK_CMD_PING;
				GsmTask.SenseD1 = (SENS_D1_READ() ? 1 : 0);
				GsmTask.SenseA1 = 0;

#if (USE_CAN_SUPPORT == 1)
				CanEnable = 0;
				while (!CanDisabled)
					vTaskDelay(1);

				for (idx = 0; idx < CAN_VALUES_SIZE; idx ++)
					GsmTask.CanData[idx] = CanSumGet(idx);

				CanSumInit();
				CanEnable = 1;
#endif
			}
		}

		gc->PingTick = xTaskGetTickCount() + gc->PingInterval;

		gc->Error = gsmSendTaskCommand(gc);
		if (gc->Error == E_OK)
			gsmProcessHostResponse(gc, true);
		else if (gc->Error == E_NO_RESPONSE)
			gsmProcessHostResponse(gc, false);
		else if (gsmHostUndefine())
			goto gsm_restart;

		if (GG_FwSmsIndex != 0
		&&	GG_FwFileName[0] != 0
		&&	uxQueueMessagesWaiting(gc->TaskQueue) == 0
			)
		{
			GlobalStatus |= GPS_STOP;
			switch (gsmGetFileFTP(gc, GG_FwFileName))
			{
				case E_ERROR:
					DebugPutLineln("Download error");
					break;
				case E_FW_ERROR:
					DebugPutLineln("Flash error");
					break;
				case E_OK:
					gsmDeleteSMS(GG_FwSmsIndex);
					DebugPutLineln("Reboot");
					while(DebugFlush());
					SystemRestart();
					break;
			}
			GlobalStatus &= ~GPS_STOP;
		}
#ifdef DISPLAY_STACK
		DisplayStackHigh("*** GSM Stack 3:");
#endif
	}
}
