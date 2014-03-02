#include <string.h>

#include "hardware.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "main.h"
#include "gps.h"
#include "gsm.h"
#include "acc.h"
#include "can.h"
#include "debug.h"
#include "utility.h"
#include "gpsOne.h"
#include "fw_update.h"
#include "nmea/tok.h"
#include "usb.h"

#define LED_TASK_STACK_SIZE	(configMINIMAL_STACK_SIZE)
#define ACC_TASK_STACK_SIZE	(configMINIMAL_STACK_SIZE)
#define GPS_TASK_STACK_SIZE	(configMINIMAL_STACK_SIZE)
#define CAN_TASK_STACK_SIZE	(configMINIMAL_STACK_SIZE)
#define GSM_TASK_STACK_SIZE	(config_STACK_512)

#define LED_TASK_PRIORITY	(tskIDLE_PRIORITY + 1)
#define CAN_TASK_PRIORITY	(tskIDLE_PRIORITY + 2)
#define GPS_TASK_PRIORITY	(tskIDLE_PRIORITY + 3)
#define GSM_TASK_PRIORITY	(tskIDLE_PRIORITY + 4)

#if   defined( STM32F101RC )
	#warning "STM32F101RC"
#elif defined( STM32F205RE )
	#warning "STM32F205RE"
#elif defined( STM32F405RG )
	#warning "STM32F405RG"
#else
	#error "CPU not define"
#endif

xQueueHandle LedQueue;
uint8_t GlobalStatus    = 0;

volatile uint16_t VBATSum   = 0;
volatile uint16_t VBATCount = 0;
volatile uint16_t VBATValue = 0;

#if ( USE_USB )
	USB_OTG_CORE_HANDLE USB_OTG_dev;
#endif

/*
const char FwVersion[]			= VERSION_INFO;
const char D_GRPS_APN_NAME[]	= DEF_GRPS_APN_NAME;
const char D_GPRS_USERNAME[]	= DEF_GPRS_USERNAME;
const char D_GPRS_PASSWORD[]	= DEF_GPRS_PASSWORD;
const char D_HOST_ADDR[]		= DEF_HOST_ADDR;
const char D_HOST_PORT[]		= DEF_HOST_PORT;
*/
uint8_t SetupHardware(void);
void vLedTask(void * pvArg);

/*******************************************************************************
* Function Name	:	delay_ms
* Description	:	Delay in milliseconds with software loop
* Input			:	delay in ms
*******************************************************************************/
void delay_ms(volatile u16 delay)
{
	volatile uint32_t count;
	volatile uint32_t cpu_delay = (SystemCoreClock / 1000UL) * 11988UL / 72000UL;
	while(delay != 0)
	{
		--delay;
		for(count = cpu_delay; count != 0; count--)
			__NOP();
	}
}

/*******************************************************************************
* Function Name	:	WaitForVDC
* Description	:	Wait for VDC
*******************************************************************************/

void WaitForVDC(void)
{
	while(1)
	{
		ShowFatalErrorOnly(ERROR_LOWEST_POWER);
		delay_ms(10000);
		if (VDC_CONNECT())
			SystemRestart();
	}
}

/*******************************************************************************
* Function Name	:	main
* Description	:	Main program
*******************************************************************************/
int main(void) __attribute__((noreturn));
int main(void)
{
	switch(SetupHardware())
	{
		case 1:	// Very low power, only deep sleep
			WaitForVDC();
			break;
		case 2:	// Low power, only send SMS and sleep
			GlobalStatus |= VBAT_LOW;
			break;
	}
/*
	strcpy(SET_GRPS_APN_NAME,	D_GRPS_APN_NAME);
	strcpy(SET_GPRS_USERNAME,	D_GPRS_USERNAME);
	strcpy(SET_GPRS_PASSWORD,	D_GPRS_PASSWORD);
	strcpy(SET_HOST_ADDR,		D_HOST_ADDR);
	strcpy(SET_HOST_PORT,		D_HOST_PORT);
*/	
#if (USE_FW_UPDATE)
	#warning USE_FW_UPDATE
	FwUpdateInit();
#endif

	LedQueue = xQueueCreate(5, sizeof(u16));
	if (
			xTaskCreate ( vLedTask, NULL, LED_TASK_STACK_SIZE, NULL, LED_TASK_PRIORITY, NULL ) == pdPASS
		&&	xTaskCreate ( vGsmTask, NULL, GSM_TASK_STACK_SIZE, NULL, GSM_TASK_PRIORITY, NULL ) == pdPASS
		&&	xTaskCreate ( vGpsTask, NULL, GPS_TASK_STACK_SIZE, NULL, GPS_TASK_PRIORITY, NULL ) == pdPASS
#if (USE_CAN_SUPPORT)
		&&	xTaskCreate ( vCanTask, NULL, CAN_TASK_STACK_SIZE, NULL, CAN_TASK_PRIORITY, NULL ) == pdPASS
#endif
		)
		vTaskStartScheduler();	// Start the scheduler

	// Task create fail, display error and restart
	ShowFatalError(ERROR_TASK_FAILED);
}

/*******************************************************************************
* Function Name	:	SystemRestart
* Description	:	Make MCU restart
*******************************************************************************/
void SystemRestart(void)
{
	__disable_irq();
	delay_ms(500);
	NVIC_SystemReset();
	while(1);
}

/*******************************************************************************
* Function Name	:	vApplicationIdleHook
* Description	:	User defined function from within the idle task
*******************************************************************************/
void vApplicationIdleHook(void)
{
}

/*******************************************************************************
* Function Name	:	vApplicationIdleHook
* Description	:	User defined function from within the idle task
*******************************************************************************/
static uint8_t KeyState = 0;
static uint8_t KeyNumber;
static uint16_t KeyTimer;

void vApplicationTickHook(void)
{
	static uint8_t key_debounce;

	if (!KEY_PRESS())
	{
		if (KeyState != 0)
		{
			LED_OFF();
			if (KeyState == 3)
			{
				if (KeyNumber == 5)
					SystemRestart();

				ShowErrorFromISR(0x81);
				/*
				if (KeyNumber > 3)
				{
					KeyStage = 0;
					ShowErrorFromISR(0x81);
				}
				else
				{
					switch(KeyStage)
					{
					case 0:
						KeyNumber1 = KeyNumber;
						ShowErrorFromISR(0x82);
						break;
					case 1:
						KeyNumber2 = KeyNumber;
						ShowErrorFromISR(0x83);
						KeyStage = 0xFF;
						break;
					}
					++KeyStage;
				}
				*/
			}
			KeyState = 0;
		}
		return;
	}

	switch(KeyState)
	{
	case 0:
		key_debounce = 10;
		++KeyState;
		break;
	case 1:
		--key_debounce;
		if (key_debounce == 0)
		{
			LED_ON();
			KeyTimer = 2000;
			++KeyState;
		}
		break;
	case 2:
		--KeyTimer;
		if (KeyTimer == 0)
		{
			LED_OFF();
			KeyTimer = 1000;
			KeyNumber = 0;
			++KeyState;
		}
		break;
	case 3:
		--KeyTimer;
		if (KeyTimer == 0)
		{
			LED_ON();
			++KeyState;
			KeyTimer = 500;
		}
		break;
	case 4:
		--KeyTimer;
		if (KeyTimer == 0)
		{
			LED_OFF();
			KeyTimer = 1000;
			KeyState = 3;
			++KeyNumber;
		}
		break;
	}
}

/*******************************************************************************
* Function Name	:	vApplicationMallocFailedHook
* Description	:	Allocate memory fail
*******************************************************************************/
void vApplicationMallocFailedHook( void )
{
	DebugPutLineln("vApplicationMallocFailedHook");
	while(DebugFlush());
	ShowFatalError(ERROR_NO_TASK_MEMORY);
}

/*******************************************************************************
* Function Name	:	Exception_Handler
* Description	:	Unknown exception handler
*******************************************************************************/
void Exception_Handler(void)
{
	ShowFatalError(ERROR_UNKNOWN_EXCEPTION);
}

/*******************************************************************************
* Function Name	:	Exception_Handler
* Description	:	Unknownn interrupt handler
*******************************************************************************/
void Default_Handler(void)
{
	ShowFatalError(ERROR_UNKNOWN_INTERRUPT);
}

/*******************************************************************************
* Function Name	:	ShowFatalError
* Description	:	Show fatal errors and make a SystemReset
* Input			:	error code
*******************************************************************************/
void ShowFatalErrorOnly(u16 errorCode)
{
	u8 count;
	__disable_irq();
	count = errorCode >> 8;
	if (count != 0)
	{
		delay_ms(1000);
		while(count-- != 0)
		{
			LED_ON();
			delay_ms(30);
			LED_OFF();
			delay_ms(500);
		}
		delay_ms(500);
	}
	count = errorCode & 0xFF;
	if (count != 0)
	{
		delay_ms(500);
		while(count-- != 0)
		{
			LED_ON();
			delay_ms(30);
			LED_OFF();
			delay_ms(500);
		}
		delay_ms(1000);
	}
}

/*******************************************************************************
* Function Name	:	ShowFatalError
* Description	:	Add error to queue
* Input			:	error code
*******************************************************************************/
void ShowFatalError(uint16_t errorCode)
{
	ShowFatalErrorOnly(errorCode);
	SystemRestart();
}

/*******************************************************************************
* Function Name	:	ShowErrorFromISR
* Description	:	Add error to queue
* Input			:	error code
*******************************************************************************/
void ShowErrorFromISR(u16 errorCode)
{
	portBASE_TYPE ledTaskSwitchNeed;
	if (LedQueue != NULL)
		xQueueSendFromISR(LedQueue, &errorCode, &ledTaskSwitchNeed);
}

/*******************************************************************************
* Function Name	:	ShowError
* Description	:	Add error to queue
* Input			:	error code
*******************************************************************************/
void ShowError(u16 errorCode)
{
	if (LedQueue != NULL)
		xQueueSend(LedQueue, &errorCode, 5000);
}

/*******************************************************************************
* Function Name	:	vLEDTask
* Description	:	LED Task
*******************************************************************************/
void SetLedWithoutKey(uint8_t state)
{
	if (KeyState == 0)
	{
		if (state)	LED_ON();
		else		LED_OFF();
	}
}

/*******************************************************************************
* Function Name	:	vLEDTask
* Description	:	LED Task
*******************************************************************************/
void vLedTask(void * pvArg)
{
	u16 errorCode;
	u8 count;

	LedQueue = xQueueCreate(5, sizeof(u16));
	if (LedQueue == NULL)
		ShowFatalError(ERROR_TASK_FAILED);

	while(1)
	{
		if (xQueueReceive(LedQueue, &errorCode, 500 ) == pdPASS)
		{
			count = errorCode >> 8;
			if (count != 0)
			{
				vTaskDelay(1000);
				while(count-- != 0)
				{
					SetLedWithoutKey(SET);
					vTaskDelay(50);
					SetLedWithoutKey(RESET);
					vTaskDelay(500);
				}
			}
			count = errorCode & 0xFF;
			if (count != 0)
			{
				if (count & 0x80)
				{
					vTaskDelay(300);
					count &= 0x7F;
					while(count-- != 0)
					{
						SetLedWithoutKey(SET);
						vTaskDelay(50);
						SetLedWithoutKey(RESET);
						vTaskDelay(300);
					}
					vTaskDelay(300);
				}
				else if (count == 1)
				{
					SetLedWithoutKey(SET);
					vTaskDelay(50);
					SetLedWithoutKey(RESET);
				}
				else
				{
					vTaskDelay(500);
					while(count-- != 0)
					{
						SetLedWithoutKey(SET);
						vTaskDelay(50);
						SetLedWithoutKey(RESET);
						vTaskDelay(500);
					}
					vTaskDelay(1000);
				}
			}
		}
		else
		{
#if (USE_FW_UPDATE)
			FwUpdateCheck();	// Process FW download
#endif
		}
		
#if ( USE_USB )
		if (USB_CONNECT())
		{
			if (!VCP_Initialize())
				USBD_Init ( &USB_OTG_dev,
							USB_OTG_FS_CORE_ID,
							(USBD_DEVICE *) &USR_desc,
							(USBD_Class_cb_TypeDef *) &USBD_CDC_cb,
							(USBD_Usr_cb_TypeDef *) &USR_cb
							);
		}
		else
		{
			if (VCP_Initialize())
			{
				USBD_USR_DeInit(&USB_OTG_dev);
			}
		}
#endif
	}
}

/*******************************************************************************
* Function Name	:	SetupHardware
* Description	:	Initialize hardware
*******************************************************************************/
uint8_t SetupHardware( void )
{
	NVIC_InitTypeDef NVIC_InitStructure;
	NVIC_InitTypeDef *nvi = &NVIC_InitStructure;
	ADC_InitTypeDef ADC_InitStructure;

#if defined( STM32F2XX ) || defined( STM32F4XX )
	ADC_CommonInitTypeDef ADC_CommonInitStructure;
#endif

#if (USE_SERIAL_DEBUG)
	USART_InitTypeDef USART_InitStructure;
#endif

	/* Configure HCLK clock as SysTick clock source. */
	SysTick_CLKSourceConfig( SysTick_CLKSource_HCLK );

	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_1);
	nvi->NVIC_IRQChannelCmd = ENABLE;

#if defined( STM32F2XX ) || defined( STM32F4XX )
	// Enable CRC clock
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_CRC, ENABLE);
	// Set all pins as analog input
	RCC_AHB1PeriphClockCmd (
		RCC_AHB1Periph_GPIOA |
		RCC_AHB1Periph_GPIOB |
		RCC_AHB1Periph_GPIOC |
		RCC_AHB1Periph_GPIOD,
		ENABLE);
	GPIOA->MODER	= 0x28000000;	// PA14/PA13 as SWD
	GPIOA->OTYPER	= 0;
	GPIOA->OSPEEDR	= 0;
	GPIOA->PUPDR	= 0x24000000;	// PA14 pull-up, PA13 pull-down

	GPIOB->MODER = 0; GPIOB->OTYPER = 0; GPIOB->OSPEEDR = 0; GPIOB->PUPDR = 0;
	GPIOC->MODER = 0; GPIOC->OTYPER = 0; GPIOC->OSPEEDR = 0; GPIOC->PUPDR = 0;
	GPIOD->MODER = 0; GPIOD->OTYPER = 0; GPIOD->OSPEEDR = 0; GPIOD->PUPDR = 0;
#endif

#if defined( STM32F10X_HD ) || defined( STM32F10X_MD )
	// Enable CRC clock
	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_CRC, ENABLE);
	// Set all pins as analog input
	RCC_APB2PeriphClockCmd (
		RCC_APB2Periph_GPIOA |
		RCC_APB2Periph_GPIOB |
		RCC_APB2Periph_GPIOC |
		RCC_APB2Periph_GPIOD |
		RCC_APB2Periph_AFIO,
		ENABLE);
	// Enable SWD/SWC pins PA13, PA14
	// All other to analog in
	GPIOA->CRL = 0; GPIOA->CRH = 0x08800000;
	GPIOB->CRL = 0; GPIOB->CRH = 0;
	GPIOC->CRL = 0; GPIOC->CRH = 0;
	GPIOD->CRL = 0; GPIOD->CRH = 0;

	// Disable JTAG
	GPIO_PinRemapConfig(GPIO_Remap_SWJ_JTAGDisable, ENABLE);
#endif

	CPU_PWR_INIT();
	LED_INIT();
	VDC_INIT();
	VBAT_SENS_INIT();

#if defined ( STM32F10X_MD ) || defined ( STM32F10X_HD )

	nvi->NVIC_IRQChannel = ADC1_2_IRQn;
	nvi->NVIC_IRQChannelPreemptionPriority = 0;
	nvi->NVIC_IRQChannelSubPriority = 0;
	NVIC_Init(nvi);

	RCC_ADCCLKConfig(RCC_PCLK2_Div4);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1, ENABLE);

	ADC_InitStructure.ADC_Mode = ADC_Mode_Independent;
	ADC_InitStructure.ADC_ScanConvMode = ENABLE;
	ADC_InitStructure.ADC_ContinuousConvMode = DISABLE;
	ADC_InitStructure.ADC_ExternalTrigConv = ADC_ExternalTrigConv_None;
	ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right;
	ADC_InitStructure.ADC_NbrOfChannel = 1;
	ADC_Init(ADC1, &ADC_InitStructure);

	ADC_RegularChannelConfig(ADC1, ADC_Channel_9, 1, ADC_SampleTime_55Cycles5);
	ADC_Cmd(ADC1, ENABLE);

	ADC_ResetCalibration(ADC1);
	while(ADC_GetResetCalibrationStatus(ADC1));

	ADC_StartCalibration(ADC1);
	while(ADC_GetCalibrationStatus(ADC1));

	ADC_ITConfig(ADC1, ADC_IT_EOC, ENABLE);

	if (VDC_NOT_CONNECT())
	{	// Need measure VBAT level and stop if low
		ADC_SoftwareStartConvCmd(ADC1, ENABLE);
		while ((GlobalStatus & VBAT_READY) == 0);
		GlobalStatus |= VBAT_STOPPED;
		if (VBATValue <= VBAT_ONLY_OFF)
		{
			ADC_ITConfig(ADC1, ADC_IT_EOC, DISABLE);
			ADC_Cmd(ADC1, DISABLE);
			RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1, DISABLE);
			return 1;	// Very low battery level, stop immediatly
		}
		if (VBATValue <= VBAT_ONLY_OFF)
		{
			ADC_ITConfig(ADC1, ADC_IT_EOC, DISABLE);
			ADC_Cmd(ADC1, DISABLE);
			RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1, DISABLE);
			return 2;	// Low battery level, send SMS and stop
		}
	}

#endif

#if defined ( STM32F2XX) || defined ( STM32F4XX )

	nvi->NVIC_IRQChannel = ADC_IRQn;
	nvi->NVIC_IRQChannelPreemptionPriority = 0;
	nvi->NVIC_IRQChannelSubPriority = 0;
	NVIC_Init(nvi);

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1, ENABLE);

	ADC_CommonInitStructure.ADC_Mode = ADC_Mode_Independent;
	ADC_CommonInitStructure.ADC_Prescaler = ADC_Prescaler_Div2;
	ADC_CommonInitStructure.ADC_DMAAccessMode = ADC_DMAAccessMode_Disabled;
	ADC_CommonInitStructure.ADC_TwoSamplingDelay = ADC_TwoSamplingDelay_5Cycles;
	ADC_CommonInit(&ADC_CommonInitStructure);

	ADC_InitStructure.ADC_Resolution = ADC_Resolution_12b;
	ADC_InitStructure.ADC_ScanConvMode = DISABLE;
	ADC_InitStructure.ADC_ContinuousConvMode = ENABLE;
	ADC_InitStructure.ADC_ExternalTrigConvEdge = ADC_ExternalTrigConvEdge_None;
	ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right;
	ADC_InitStructure.ADC_NbrOfConversion = 1;
	ADC_Init(ADC1, &ADC_InitStructure);

	ADC_RegularChannelConfig(ADC1, VBAT_CHANNEL, 1, ADC_SampleTime_15Cycles);
	ADC_Cmd(ADC1, ENABLE);

	ADC_ITConfig(ADC1, ADC_IT_EOC, ENABLE);

	if (VDC_NOT_CONNECT())
	{
		// Need measure VBAT level and stop if low
		ADC_SoftwareStartConv(ADC1);
		while ((GlobalStatus & VBAT_READY) == 0);
		GlobalStatus |= VBAT_STOPPED;
		if (VBATValue <= VBAT_ONLY_OFF)
		{
			ADC_ITConfig(ADC1, ADC_IT_EOC, DISABLE);
			ADC_Cmd(ADC1, DISABLE);
			RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1, DISABLE);
			return 1;	// Very low battery level, stop immediatly
		}
		if (VBATValue <= VBAT_ONLY_OFF)
		{
			ADC_ITConfig(ADC1, ADC_IT_EOC, DISABLE);
			ADC_Cmd(ADC1, DISABLE);
			RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1, DISABLE);
			return 2;	// Low battery level, send SMS and stop
		}
	}
#endif

	KEY_INIT();

	USB_DISC_INIT();
	USB_P_INIT();

#if (USE_SERIAL_DEBUG)
	nvi->NVIC_IRQChannel = DEBUG_USART_IRQn;
	nvi->NVIC_IRQChannelPreemptionPriority = 0;
	nvi->NVIC_IRQChannelSubPriority = 1;
	NVIC_Init(nvi);

	DEBUG_INIT();

	USART_StructInit(&USART_InitStructure);
	USART_InitStructure.USART_BaudRate = 115200;
	USART_Init(DEBUG_USART, &USART_InitStructure);
	USART_Cmd(DEBUG_USART, ENABLE);

	DebugInit();
#endif

	DebugPutLineln("\nInitialization");

	SD_INIT();
	ACC_INIT();

	SENS_D1_INIT();
	SENS_A1_INIT();

	RELAY_INIT();

	GPS_INIT();
	GSM_INIT();

#if (USE_CAN_SUPPORT)
	CAN_INIT();
#endif

#ifdef GPS_USART_IRQn
	nvi->NVIC_IRQChannel = GPS_USART_IRQn;
	nvi->NVIC_IRQChannelPreemptionPriority = 1;
	nvi->NVIC_IRQChannelSubPriority = 2;
	NVIC_Init(nvi);
#endif

#ifdef GSM_USART_IRQn
	nvi->NVIC_IRQChannel = GSM_USART_IRQn;
	nvi->NVIC_IRQChannelPreemptionPriority = 1;
	nvi->NVIC_IRQChannelSubPriority = 1;
	NVIC_Init(nvi);
#endif

#if (USE_CAN_SUPPORT)
#ifdef CAN_RX0_IRQn
	nvi->NVIC_IRQChannel = CAN_RX0_IRQn;
	nvi->NVIC_IRQChannelPreemptionPriority = 1;
	nvi->NVIC_IRQChannelSubPriority = 0;
	NVIC_Init(nvi);
#endif

#ifdef CAN_RX1_IRQn
	nvi->NVIC_IRQChannel = CAN_RX0_IRQn;
	nvi->NVIC_IRQChannelPreemptionPriority = 1;
	nvi->NVIC_IRQChannelSubPriority = 0;
	NVIC_Init(nvi);
#endif

#ifdef CAN_TX_IRQn
	nvi->NVIC_IRQChannel = CAN_TX_IRQn;
	nvi->NVIC_IRQChannelPreemptionPriority = 1;
	nvi->NVIC_IRQChannelSubPriority = 0;
	NVIC_Init(nvi);
#endif

#ifdef CAN_SCE_IRQn
	nvi->NVIC_IRQChannel = CAN_SCE_IRQn;
	nvi->NVIC_IRQChannelPreemptionPriority = 1;
	nvi->NVIC_IRQChannelSubPriority = 0;
	NVIC_Init(nvi);
#endif
#endif
	return 0;
}

void ADC_IRQHandler(void)
{
	++VBATCount;
	VBATSum += ADC_GetConversionValue(ADC1);
	if (VBATCount == 10)
	{
		if ((GlobalStatus & VBAT_READY) == 0)
		{
			VBATValue = VBATSum / VBATCount;
			GlobalStatus |= VBAT_READY;
		}
		VBATSum = 0;
		VBATCount = 0;
	}

	if ((GlobalStatus & VBAT_STOPPED) == 0)
	{
#if defined ( STM32F10X_HD ) || defined ( STM32F10X_MD )
		ADC_SoftwareStartConvCmd(ADC1, ENABLE);
#endif
#if defined ( STM32F2XX ) || defined ( STM32F4XX )
		ADC_SoftwareStartConv(ADC1);
#endif
	}
}
