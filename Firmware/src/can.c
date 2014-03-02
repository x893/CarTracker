#include "hardware.h"
#include "FreeRTOS.h"
#include "queue.h"
#include "semphr.h"
#include "task.h"
#include "can.h"
#include "debug.h"
#include "main.h"

#include <stdio.h>

#if (USE_CAN_SUPPORT)

#define CAN_RX_QUEUE_SIZE		4

#define CAN_MODE_ENGINE_COOLANT	0
#define CAN_MODE_ENGINE_RPM		1
#define CAN_MODE_VEHICLE_SPEED	2
#define CAN_MODE_MAF_SENSOR		3
#define CAN_MODE_THROTTLE		4
#define CAN_MODE_O2_VOLTAGE		5
#define CAN_MODE_VIN_COUNT		6
#define CAN_MODE_VIN_DATA		7
#define CAN_MODE_FUELLEVEL		8
#define CAN_MODE_DTC			9
#define CAN_MODE_LAST			10

#define CAN_MODE_SEARCH_125		100
#define CAN_MODE_SEARCH_250		101
#define CAN_MODE_SEARCH_500		102
#define CAN_MODE_SEARCH_1000	103

volatile uint8_t canMode;
xQueueHandle CanRxQueue;
CanTxMsg TxMessage;
CanRxMsg RxMessage;

typedef struct avg_double_s {
	double count;
	double sum;
} avg_double_t;

#define I_OBD_EngineRPM		0
#define I_OBD_EngineCoolant	1
#define I_OBD_VehicleSpeed	2
#define I_OBD_MAFSensor		3
#define I_OBD_Throttle		4
#define I_OBD_O2Voltage		5
#define I_OBD_FuelLevel		6

avg_double_t CanValues[CAN_VALUES_SIZE];

/*******************************************************************************
* Function Name	:	CanSum...
* Description	:
*******************************************************************************/
uint8_t CanEnable;
uint8_t CanDisabled;

void CanSumInit(void)
{
	int i;
	for(i = 0; i < CAN_VALUES_SIZE; i++)
	{
		CanValues[i].count = 0;
		CanValues[i].sum = 0;
	}
}

void CanSumAdd(int index, double value)
{
	CanValues[index].count++;
	CanValues[index].sum += value;
}

double CanSumGet(int index)
{
	double value = 0;
	if (CanValues[index].count > 0)
		value = CanValues[index].sum / CanValues[index].count;
	return value;
}

/*******************************************************************************
* Function Name	:	CanInit
* Description	:	Initialize CAN interface
*******************************************************************************/
void CanInit(u8 CANSpeed)
{
	CAN_InitTypeDef			can_init;
	CAN_FilterInitTypeDef	can_filter_init;

	CAN_ITConfig(CAN_CAN, CAN_IT_FMP0, DISABLE);
	CAN_DeInit(CAN_CAN);

	CAN_StructInit(&can_init);

	can_init.CAN_NART = ENABLE;
	can_init.CAN_ABOM = ENABLE;

#if   defined( STM32F10X_HD )	// 72 MHz	(CAN 36MHz)
	can_init.CAN_SJW = CAN_SJW_3tq;
	can_init.CAN_BS1 = CAN_BS1_12tq;
	can_init.CAN_BS2 = CAN_BS2_5tq;
#elif defined ( STM32F2XX )		// 120 MHz	(CAN 30MHz)
	can_init.CAN_SJW = CAN_SJW_1tq;
	can_init.CAN_BS1 = CAN_BS1_6tq;
	can_init.CAN_BS2 = CAN_BS2_8tq;
#else
	#error "CPU not define"
#endif

	if (CANSpeed == CAN_SPEED_1000)
		can_init.CAN_Prescaler = 2;
	else if (CANSpeed == CAN_SPEED_500)
		can_init.CAN_Prescaler = 4;
	else if (CANSpeed == CAN_SPEED_250)
		can_init.CAN_Prescaler = 8;
	else if (CANSpeed == CAN_SPEED_125)
		can_init.CAN_Prescaler = 16;
	else	// if (CANSpeed == CAN_SPEED_100)
		can_init.CAN_Prescaler = 22;

	CAN_Init(CAN_CAN, &can_init);

	can_filter_init.CAN_FilterNumber			= 0;
	can_filter_init.CAN_FilterMode				= CAN_FilterMode_IdMask;
	can_filter_init.CAN_FilterScale				= CAN_FilterScale_32bit;
	can_filter_init.CAN_FilterIdHigh			= (((u32)PID_REPLY << 21) & 0xFFFF0000) >> 16;
	can_filter_init.CAN_FilterIdLow				= (((u32)PID_REPLY << 21) | CAN_ID_STD | CAN_RTR_DATA) & 0xFFFF;
	can_filter_init.CAN_FilterMaskIdHigh		= 0xFFFF;
	can_filter_init.CAN_FilterMaskIdLow			= 0xFFFF;
	can_filter_init.CAN_FilterFIFOAssignment	= 0;
	can_filter_init.CAN_FilterActivation		= ENABLE;

	CAN_FilterInit(&can_filter_init);
	CAN_ITConfig(CAN_CAN, CAN_IT_FMP0, ENABLE);
}

/*******************************************************************************
* Function Name	:	OBDMessage
* Description		:	Prepare OBD message
* Return			:	pointer to message
*******************************************************************************/
CanTxMsg* PrepareMessage(CanTxMsg* msg, u8 mode, u8 pid_code)
{
	msg->StdId		= PID_REQUEST;
	msg->ExtId		= 0;
	msg->RTR		= CAN_RTR_DATA;
	msg->IDE		= CAN_ID_STD;
	msg->DLC		= 8;
	msg->Data[0]	= 0x02;
	msg->Data[1]	= mode;
	msg->Data[2]	= pid_code;
	msg->Data[3]	= 0;
	msg->Data[4]	= 0;
	msg->Data[5]	= 0;
	msg->Data[6]	= 0;
	return msg;
}

u8 IsCanRxAvailable(void)
{
	if (uxQueueMessagesWaiting(CanRxQueue) != 0)
		return 1;
	return 0;
}

#if ( USE_DEBUG )
void PrintCanMsg(CanTxMsg *msg)
{
	printf("StdId: %4X\n", msg->StdId);
	printf("ExtId: %4X\n", msg->ExtId);
	printf("  IDE: %4X\n", msg->IDE);
	printf("  RTR: %4X\n", msg->RTR);
	printf("  DLC: %d\n", msg->DLC);
	printf(" Data: %2X %2X %2X %2X %2X %2X %2X %2X\n",
		msg->Data[0],
		msg->Data[1],
		msg->Data[2],
		msg->Data[3],
		msg->Data[4],
		msg->Data[5],
		msg->Data[6],
		msg->Data[7]
		);
}

void PrintCanMsgTx(CanTxMsg *msg)
{
	DebugPutLineln("---------------");
	DebugPutLineln("TX Message:");
	PrintCanMsg((CanTxMsg *)msg);
}

void PrintCanMsgRx(CanRxMsg *msg)
{
	DebugPutLineln("---------------");
	DebugPutLineln("RX Message:");
	PrintCanMsg((CanTxMsg *)msg);
	// printf("  FMI: %X\n", msg->FMI);
}
#else
	#define PrintCanMsg(x)
	#define PrintCanMsgTx(x)
	#define PrintCanMsgRx(x)
#endif

/*******************************************************************************
* Function Name	:	ObdSend
* Description		:	Initialize hardware
* Return			:	mailbox number or CAN_NO_MB if no free mailbox
*******************************************************************************/
u8 SendRequest(u8 mode, u8 pid_code)
{
	PrepareMessage(&TxMessage, mode, pid_code);	// Build package
	PrintCanMsgTx(&TxMessage);
	return CAN_Transmit(CAN_CAN, &TxMessage);
}

void CollectData(CanRxMsg *msg)
{
}

/*******************************************************************************
* Function Name	:	vCanTask
* Description	:	CAN Task
*******************************************************************************/
void vCanTask(void *args)
{
	__IO u8 canModeSave;
	__IO u8 canSpeed = CAN_SPEED_125;
	u8 pid_mode, pid_request;
	u8 can_last_mb;

	DebugWaitConnect();
	DebugPutLine("CAN Initialize ... ");

	CanRxQueue = xQueueCreate(CAN_RX_QUEUE_SIZE, sizeof(CanRxMsg));
	if (CanRxQueue == NULL)
	{
		DebugPutLineln("Error");
		ShowError(ERROR_CAN_TASK_FAIL);
		return;
	}

	DebugPutLineln("OK");
	
	CanSumInit();
	CanEnable = 1;
	canMode = CAN_MODE_SEARCH_500;	// Start search from 500 kbits speed

	for(;;)
	{
		if (!CanEnable)
		{
			CanDisabled = 1;
			vTaskDelay(1000);
			continue;
		}
		CanDisabled = 0;
		pid_mode	= PID_MODE_01;
		pid_request = PID_01_SUPPORTED;		// Send PID supported request
		switch (canMode)
		{
		case CAN_MODE_SEARCH_125:
			DebugPutLineln("CAN Speed: 125 Kb/s");
			canSpeed = CAN_SPEED_125;
			canMode = CAN_MODE_SEARCH_250;
			break;
		case CAN_MODE_SEARCH_250:
			DebugPutLineln("CAN Speed: 250 Kb/s");
			canSpeed = CAN_SPEED_250;
			canMode = CAN_MODE_SEARCH_500;
			break;
		case CAN_MODE_SEARCH_500:
			DebugPutLineln("CAN Speed: 500 Kb/s");
			canSpeed = CAN_SPEED_500;
			canMode = CAN_MODE_SEARCH_1000;
			break;
		case CAN_MODE_SEARCH_1000:
			DebugPutLineln("CAN Speed: 1 Mb/s");
			canSpeed = CAN_SPEED_1000;
			canMode = CAN_MODE_SEARCH_125;
			break;
		
		case CAN_MODE_LAST:
			canMode = CAN_MODE_ENGINE_COOLANT;
		case CAN_MODE_ENGINE_COOLANT:
			DebugPutLineln("CAN Req: OBD_ENGINE_COOLANT");
			pid_request = OBD_ENGINE_COOLANT;
			break;
		case CAN_MODE_ENGINE_RPM:
			DebugPutLineln("CAN Req: OBD_ENGINE_RPM");
			pid_request = OBD_ENGINE_RPM;
			break;
		case CAN_MODE_VEHICLE_SPEED:
			DebugPutLineln("CAN Req: OBD_VEHICLE_SPEED");
			pid_request = OBD_VEHICLE_SPEED;
			break;
		case CAN_MODE_MAF_SENSOR:
			DebugPutLineln("CAN Req: OBD_MAF_SENSOR");
			pid_request = OBD_MAF_SENSOR;
			break;
		case CAN_MODE_THROTTLE:
			DebugPutLineln("CAN Req: OBD_THROTTLE");
			pid_request = OBD_THROTTLE;
			break;
		case CAN_MODE_O2_VOLTAGE:
			DebugPutLineln("CAN Req: OBD_O2_VOLTAGE");
			pid_request = OBD_O2_VOLTAGE;
			break;
		case CAN_MODE_FUELLEVEL:
			DebugPutLineln("CAN Req: OBD_FUELLEVEL");
			pid_request = OBD_FUELLEVEL;
			break;
		case CAN_MODE_DTC:
			DebugPutLineln("CAN Req: OBD_DTC");
			pid_mode	= PID_MODE_01;
			pid_request = OBD_DTC;
			break;
		/*
		case CAN_MODE_VIN_COUNT:
			DebugPutLineln("CAN Req: OBD_VIN_COUNT");
			pid_mode	= PID_MODE_09;
			pid_request = OBD_VIN_COUNT;
			break;
		case CAN_MODE_VIN_DATA:
			DebugPutLineln("CAN Req: OBD_VIN_DATA");
			pid_mode	= PID_MODE_09;
			pid_request = OBD_VIN_DATA;
			break;
		*/
		}

		if (canMode >= CAN_MODE_SEARCH_125)
		{	// Autospeed mode, initialize CAN
			DebugPutLineln("CAN initalize");
			CanInit(canSpeed);
			DebugPutLineln("CAN Req: PID_SUPPORTED");
		}
		
		vTaskDelay(500);
		if ((can_last_mb = SendRequest(pid_mode, pid_request)) == CAN_NO_MB)
		{	// Send package error (no mailbox)
			DebugPutLineln("No Tx Mailboxes");
			ShowError(ERROR_CAN_TRANSMITE);
			vTaskDelay(1000);
			continue;
		}

		canModeSave = canMode;
		while (canMode == canModeSave)
		{	// Repeat untill mode change or receive timeout
			if (xQueueReceive(CanRxQueue, &RxMessage, 1000) == pdPASS)
			{
				//	CAN message recieved, check it
				if (RxMessage.StdId == PID_REPLY)
				{
					PrintCanMsgRx(&RxMessage);
					switch(RxMessage.Data[2])
					{
					case PID_01_SUPPORTED:
						// Parse supported PIDs
						canMode = CAN_MODE_ENGINE_COOLANT;
						break;
					case OBD_ENGINE_COOLANT:	// A-40 [degree C]
						CanSumAdd(
							I_OBD_EngineCoolant,
							(((double)(RxMessage.Data[3])) - 40.0)
						);
						canMode = CAN_MODE_ENGINE_RPM;
						break;
					case OBD_ENGINE_RPM:		// ((A*256)+B)/4 [RPM]
						CanSumAdd(
							I_OBD_EngineRPM,
							(((double)(RxMessage.Data[3])) * 256.0 + ((double)(RxMessage.Data[4]))) / 4.0
						);
						canMode = CAN_MODE_VEHICLE_SPEED;
						break;
					case OBD_VEHICLE_SPEED:	// A [km]
						CanSumAdd(
							I_OBD_VehicleSpeed,
							((double)(RxMessage.Data[3]))
						);
						canMode = CAN_MODE_MAF_SENSOR;
						break;
					case OBD_MAF_SENSOR:		// ((256*A)+B) / 100 [g/s]
						CanSumAdd(
							I_OBD_MAFSensor,
							(((double)(RxMessage.Data[3])) * 256.0 + ((double)(RxMessage.Data[4]))) / 100.0
						);
						canMode = CAN_MODE_THROTTLE;
						break;
					case OBD_THROTTLE:		// A*100/255
						CanSumAdd(
							I_OBD_Throttle,
							((double)(RxMessage.Data[3] * 100)) / 255
						);
						canMode = CAN_MODE_O2_VOLTAGE;
						break;
					case OBD_O2_VOLTAGE:		// A * 0.005  (B-128) * 100/128 (if B == 0xFF, sensor is not used in trim calc)
						CanSumAdd(
							I_OBD_O2Voltage,
							((double)(RxMessage.Data[3])) * 0.005
						);
						canMode = CAN_MODE_FUELLEVEL;
						break;
					case OBD_FUELLEVEL:		// 100*A/255
						CanSumAdd(
							I_OBD_FuelLevel,
							(((double)(RxMessage.Data[3])) * 100.0 / 255)
						);
						canMode = CAN_MODE_DTC;
						break;
					case OBD_DTC:
						canMode = CAN_MODE_ENGINE_COOLANT;
					/*
						break;
					case OBD_VIN_COUNT:
						canMode = CAN_MODE_VIN_DATA;
						break;
					case OBD_VIN_DATA:
						canMode = CAN_MODE_ENGINE_COOLANT;
						break;
					*/
					}
				}
			}
			else
			{
				CAN_CancelTransmit(CAN_CAN, can_last_mb);

				if (canMode < CAN_MODE_SEARCH_125)
					canMode++;

				DebugPutLineln("No response");
				break;
			}
		}
	}
}

CanRxMsg RxMessageISR;

#ifdef CAN_RX0_IRQHandler
void CAN_RX0_IRQHandler(void)
{
	portBASE_TYPE canTaskSwitchNeed;
	CAN_Receive(CAN_CAN, CAN_FIFO0, &RxMessageISR);

	canTaskSwitchNeed = pdFALSE;
	xQueueSendFromISR(CanRxQueue, &RxMessageISR, &canTaskSwitchNeed);
	portEND_SWITCHING_ISR( canTaskSwitchNeed );
}
#endif

#ifdef CAN_RX1_IRQHandler
void CAN1_RX1_IRQHandler(void)
{
	portBASE_TYPE canTaskSwitchNeed;
	CAN_Receive(CAN_CAN, CAN_FIFO1, &RxMessageISR);

	portBASE_TYPE canRxTaskSwitchNeed = pdFALSE;
	xQueueSendFromISR(CanRxQueue, &RxMessageISR, &canTaskSwitchNeed);
	portEND_SWITCHING_ISR( canTaskSwitchNeed );
}
#endif

#ifdef CAN_TX_IRQHandler
void CAN_TX_IRQHandler(void)
{

}
#endif

#ifdef CAN_SCE_IRQHandler
void CAN1_SCE_IRQHandler(void)
{

}
#endif
#endif
