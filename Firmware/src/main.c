#include <string.h>
#include <stdint.h>

#include "hardware.h"
#include "main.h"
#include "debug.h"
#include "utility.h"
#include "fw_update.h"

#ifndef BL_VERSION

	#include "FreeRTOS.h"
	#include "task.h"
	#include "semphr.h"
	#include "gps.h"
	#include "gsm.h"
	#include "acc.h"
	#include "can.h"
	#include "gpsOne.h"
	#include "nmea/tok.h"
	#include "usb.h"

	#if ( USE_USB )
		USB_OTG_CORE_HANDLE USB_OTG_dev;
	#endif

	#if (USE_AVERAGE == 1)
		#warning AVERAGE FILTER
	#endif
	#if (USE_KALMAN == 1)
		#warning KALMAN FILTER
	#endif

#endif

#if   defined( STM32F101RC )
	#warning STM32F101RC
#elif defined( STM32F100CB )
	#warning STM32F100CB
#elif defined( STM32F205RE )
	#warning STM32F205RE
#elif defined( STM32F405RG )
	#warning STM32F405RG
#else
	#error "CPU not define"
#endif

const char VERSION[] = ",vers="VERSION_INFO;

uint8_t HardwareInit(void);

volatile uint16_t GlobalStatus;

#if defined ( VBAT_SENS_PIN )
	volatile uint16_t VBATSum;
	volatile uint16_t VBATCount;
	volatile uint16_t VBATValue;
#endif

typedef void (*pFunction)(void);

/*******************************************************************************
 * Function Name	:	main
 * Description	:	Main program
 *******************************************************************************/
int main(void) __attribute__((noreturn));
int main(void)
{
	RCC_ClearFlag();
	SystemCoreClockUpdate();

	DBGMCU_Config(DBGMCU_SLEEP | DBGMCU_STOP | DBGMCU_STANDBY, ENABLE);

	GlobalStatus	= 0;
	
#if defined ( VDC_NOT_CONNECT )
	VBATSum			= 0;
	VBATCount		= 0;
	VBATValue		= 0;

	switch (HardwareInit())
	{
		case 1:	// Very low power, only deep sleep
			WaitForVDC();
			break;
		case 2:	// Low power, only send SMS and sleep
			GlobalStatus |= VBAT_LOW;
			break;
	}
#else
	HardwareInit();
#endif

#ifndef BL_VERSION

	DEBUG("\nFW "VERSION_INFO"\n");

	#define GPS_TASK_STACK	(( unsigned short ) ( 680 / 4))
	#define GSM_TASK_STACK	(( unsigned short ) ( 740 / 4))

	if (xTaskCreate ( vGpsTask, "GPS", GPS_TASK_STACK, NULL, (tskIDLE_PRIORITY + 1), NULL ) == pdPASS
	&&	xTaskCreate ( vGsmTask, "GSM", GSM_TASK_STACK, NULL, (tskIDLE_PRIORITY + 2), NULL ) == pdPASS
	#if (USE_CAN_SUPPORT)
	&&	xTaskCreate ( vCanTask, NULL, CAN_TASK_STACK, NULL, (tskIDLE_PRIORITY + 3), NULL ) == pdPASS
	#endif
		)
		vTaskStartScheduler();			// Start the scheduler
	ShowFatalError(ERROR_TASK_FAILED);	// Task create fail, display error and restart

#else	// BL_VERSION

	DEBUG("\nBL "VERSION_INFO);

	FwCopyNew();

	if (FwCheckImage(FW_WORK_START))
	{
		DEBUG(" run\n");
		FLASH_Lock();
		/* Initialize user application's Stack Pointer */
		__set_MSP(*(volatile uint32_t*) FW_WORK_START);
		((pFunction)(*(uint32_t*)(FW_WORK_START + 4)))();
	}

	DEBUG(" no app\n");
	ShowFatalError(ERROR_FLASH_ERASE);

#endif	// BL_VERSION
}


#ifndef BL_VERSION

	#if defined ( STM32F10X_HD ) || defined ( STM32F10X_MD ) || defined ( STM32F10X_MD_VL )
		void Disable_WWDG(void)
		{
			WWDG_DeInit();
		}

		void Reset_WWDG(void)
		{
			uint8_t cnt = WWDG->CR & 0x7F;
			if (cnt < 80)
				WWDG_SetCounter(127);
		}

		void Enable_WWDG(void)
		{
			RCC_APB1PeriphClockCmd(RCC_APB1Periph_WWDG, ENABLE);
			DBGMCU_Config(DBGMCU_WWDG_STOP, ENABLE);
			WWDG_SetPrescaler(WWDG_Prescaler_8);
			WWDG_SetWindowValue(80);
			WWDG_Enable(127);
		}
	#else
		void Disable_WWDG(void)
		{ }
		void Reset_WWDG(void)
		{ }
		void Enable_WWDG(void)
		{ }
	#endif

	/*******************************************************************************
	* Function Name	:	vApplicationIdleHook
	* Description	:	User defined function from within the idle task
	*******************************************************************************/
	void vApplicationIdleHook(void)
	{
	#if ( USE_USB == 1)
		#warning USE_USB
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

	/*******************************************************************************
	* Function Name	:	vApplicationMallocFailedHook
	* Description	:	Allocate memory fail
	*******************************************************************************/
	void vApplicationMallocFailedHook( void )
	{
		DEBUG("*** vApplicationMallocFailedHook\n");
		while(DebugFlush())
		{ }
		ShowFatalError(ERROR_NO_TASK_MEMORY);
	}

	/*******************************************************************************
	* Function Name	:	vApplicationIdleHook
	* Description	:	User defined function from within the idle task
	*******************************************************************************/
	extern uint32_t __initial_sp;
	void vApplicationStackOverflowHook( xTaskHandle *pxTask, signed char *pcTaskName )
	{
		__set_MSP(__initial_sp);

		DebugPutLine("\nStack overflow:");
		DebugPutLine((const char *)pcTaskName);
		DebugPutLine("\n");
		ShowFatalError(ERROR_STACK_LOW);
	}

#ifdef DISPLAY_STACK
	extern size_t xFreeBytesRemaining;
	void DisplayStackHigh(const char *msg)
	{
		static char local_buffer[20];
		unsigned portBASE_TYPE stackHigh = uxTaskGetStackHighWaterMark(NULL);
		DEBUG(msg);
		itoa(local_buffer, stackHigh, 0);
		DEBUG(local_buffer);
		DEBUG(" (");
		itoa(local_buffer, xFreeBytesRemaining, 0);
		DEBUG(local_buffer);
		DEBUG_LINE(")");
	}
#endif
#endif

/*******************************************************************************
* Function Name	:	WaitForVDC
* Description	:	Wait for VDC
*******************************************************************************/

void WaitForVDC(void)
{
#if defined ( VDC_CONNECT )
	while(1)
	{
		ShowFatalErrorOnly(ERROR_LOWEST_POWER);
		DWT_Delay_ms(10000);
		if (VDC_CONNECT())
			SystemRestart();
	}
#else
	DWT_Delay_ms(10000);
	SystemRestart();
#endif
}

/*******************************************************************************
* Function Name	:	SystemRestart
* Description	:	Make MCU restart
*******************************************************************************/
void SystemRestart(void)
{
	__disable_irq();

#ifndef BL_VERSION
	#if defined ( STM32F10X_HD ) || defined ( STM32F10X_MD )
		WWDG_DeInit();
	#endif
#endif

#if	defined ( STM32F10X_MD )	|| \
	defined ( STM32F10X_HD )	|| \
	defined ( STM32F10X_MD_VL )

	RCC_APB2PeriphResetCmd(0xFFFFFFFF, ENABLE);
	RCC_APB1PeriphResetCmd(0xFFFFFFFF, ENABLE);
	RCC_APB2PeriphResetCmd(0xFFFFFFFF, DISABLE);
	RCC_APB1PeriphResetCmd(0xFFFFFFFF, DISABLE);
#else
	#error "CPU not define"
#endif

	DWT_Delay_ms(500);
	NVIC_SystemReset();
	while(1);
}

#ifndef BL_VERSION
	/*******************************************************************************
	* Function Name	:	vApplicationIdleHook
	* Description	:	User defined function from within the idle task
	*******************************************************************************/

	#if defined ( KEY_PIN )
		static uint8_t KeyState = 0;
		static uint8_t KeyNumber;
		static uint16_t KeyTimer;
		static uint8_t key_debounce;
	#endif

	void vApplicationTickHook(void)
	{
		Reset_WWDG();

	#if defined ( KEY_PRESS )
		if (!KEY_PRESS())
		{
			if (KeyState != 0)
			{
				LED_OFF();
				if (KeyState == 5)
				{
					SystemRestart();
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
	#endif
	}

#endif

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
* Function Name	:	Deinitialize GSM/GPS
* Description	:
* Input			:
*******************************************************************************/
void GsmGpsDeInit(void)
{
	GSM_DEINIT();
	GPS_DEINIT();
}


/*******************************************************************************
* Function Name	:	ShowFatalError
* Description	:	Show fatal errors and make a SystemReset
* Input			:	error code
*******************************************************************************/
void ShowFatalErrorOnly(uint8_t errorCode)
{
	uint8_t count;

	__disable_irq();

	GsmGpsDeInit();

	LED2_ON();
	count = (errorCode & 0xF0) >> 4;
	if (count != 0)
	{
		DWT_Delay_ms(1000);
		while(count != 0)
		{
			--count;
			LED1_ON();
			DWT_Delay_ms(100);
			LED1_OFF();
			DWT_Delay_ms(400);
		}
		DWT_Delay_ms(500);
	}
	count = errorCode & 0x0F;
	if (count != 0)
	{
		DWT_Delay_ms(500);
		while(count != 0)
		{
			--count;
			LED1_ON();
			DWT_Delay_ms(100);
			LED1_OFF();
			DWT_Delay_ms(400);
		}
		DWT_Delay_ms(1000);
	}
	LED2_OFF();
	DWT_Delay_ms(1000);
}

/*******************************************************************************
* Function Name	:	ShowFatalError
* Description	:	Add error to queue
* Input			:	error code
*******************************************************************************/
void ShowFatalError(uint8_t errorCode)
{
	ShowFatalErrorOnly(errorCode);
	SystemRestart();
}

#ifndef BL_VERSION

/*******************************************************************************
* Function Name	:	vLEDTask
* Description	:	LED Task
*******************************************************************************/
void SetLedWithoutKey(uint8_t state)
{
#if defined ( KEY_PRESS )
	if (KeyState == 0)
	{
		if (state)	LED1_ON();
		else		LED2_ON();
	}
#else
	if (state)	LED1_ON();
	else		LED2_ON();
#endif
}

/*******************************************************************************
* Function Name	:	ShowError
* Description	:	Add error to queue
* Input			:	error code
*******************************************************************************/
void ShowError(uint8_t errorCode)
{
	uint8_t count;

	LED2_ON();
	count = (errorCode & 0xF0) >> 4;
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
	count = errorCode & 0x0F;
	if (count == 1)
	{
		SetLedWithoutKey(SET);
		vTaskDelay(50);
		SetLedWithoutKey(RESET);
	}
	else
	{
		DEBUG_PRINTF("SOFT ERROR: %02x\n", errorCode);
		if (count > 1)
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
		vTaskDelay(1000);
	}
	LED2_OFF();
}

#endif

/*******************************************************************************
* Function Name	:	HardwareInit
* Description	:	Initialize hardware
*******************************************************************************/
uint8_t HardwareInit(void)
{
#ifndef BL_VERSION
	NVIC_InitTypeDef nvi;
#endif
#if defined ( VBAT_SENS_PIN )
	ADC_InitTypeDef acdi;

	#if defined( STM32F2XX ) || defined( STM32F4XX )
	ADC_CommonInitTypeDef ADC_CommonInitStructure;
	#endif

#endif

#if (USE_SERIAL_DEBUG)
	USART_InitTypeDef USART_InitStructure;
#endif

	/* Configure HCLK clock as SysTick clock source. */
	SysTick_CLKSourceConfig( SysTick_CLKSource_HCLK );

#if	defined( STM32F2XX ) || \
	defined( STM32F4XX )

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

#elif	defined( STM32F10X_HD ) || \
		defined( STM32F10X_MD ) || \
		defined( STM32F10X_MD_VL )

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

	// Disable JTAG
	GPIO_PinRemapConfig(GPIO_Remap_SWJ_JTAGDisable, ENABLE);

	// Enable SWD/SWC pins PA13, PA14
	// All other to analog in
	GPIOA->CRL = 0x44444444; GPIOA->CRH = 0x48844444;
	GPIOB->CRL = 0x44444444; GPIOB->CRH = 0x44444444;
	GPIOC->CRL = 0x44444444; GPIOC->CRH = 0x44444444;
	GPIOD->CRL = 0x44444444; GPIOD->CRH = 0x44444444;

#endif

	GsmGpsDeInit();
	LED_INIT();
	CPU_PWR_INIT();
	VDC_INIT();
	VBAT_SENS_INIT();
	KEY_INIT();
	CUSTOM_INIT();

#ifndef BL_VERSION
	nvi.NVIC_IRQChannelCmd = ENABLE;
	nvi.NVIC_IRQChannelPreemptionPriority = 0;
	nvi.NVIC_IRQChannelSubPriority = 0;
#endif

#if defined ( VBAT_SENS_PIN )

	nvi.NVIC_IRQChannel = VBAT_SENS_IRQ;
	NVIC_Init(&nvi);
	NVIC_SetPriority (VBAT_SENS_IRQ, 15);

	VBAT_SENS_CLOCK(ENABLE);

	#if defined ( STM32F10X_MD ) || defined ( STM32F10X_HD )

		RCC_ADCCLKConfig(RCC_PCLK2_Div4);

		acdi.ADC_Mode = ADC_Mode_Independent;
		acdi.ADC_ScanConvMode = ENABLE;
		acdi.ADC_ContinuousConvMode = DISABLE;
		acdi.ADC_ExternalTrigConv = ADC_ExternalTrigConv_None;
		acdi.ADC_DataAlign = ADC_DataAlign_Right;
		acdi.ADC_NbrOfChannel = 1;
		ADC_Init(VBAT_SENS_ADC, &acdi);

		ADC_RegularChannelConfig(VBAT_SENS_ADC, VBAT_SENS_CHANNEL, 1, ADC_SampleTime_55Cycles5);
		ADC_Cmd(VBAT_SENS_ADC, ENABLE);

		ADC_ResetCalibration(VBAT_SENS_ADC);
		while(ADC_GetResetCalibrationStatus(VBAT_SENS_ADC));

		ADC_StartCalibration(VBAT_SENS_ADC);
		while(ADC_GetCalibrationStatus(VBAT_SENS_ADC));

	#else
	#if defined ( STM32F2XX) || defined ( STM32F4XX )

		ADC_CommonInitStructure.ADC_Mode = ADC_Mode_Independent;
		ADC_CommonInitStructure.ADC_Prescaler = ADC_Prescaler_Div2;
		ADC_CommonInitStructure.ADC_DMAAccessMode = ADC_DMAAccessMode_Disabled;
		ADC_CommonInitStructure.ADC_TwoSamplingDelay = ADC_TwoSamplingDelay_5Cycles;
		ADC_CommonInit(&ADC_CommonInitStructure);

		acdi.ADC_Resolution = ADC_Resolution_12b;
		acdi.ADC_ScanConvMode = DISABLE;
		acdi.ADC_ContinuousConvMode = ENABLE;
		acdi.ADC_ExternalTrigConvEdge = ADC_ExternalTrigConvEdge_None;
		acdi.ADC_DataAlign = ADC_DataAlign_Right;
		acdi.ADC_NbrOfConversion = 1;
		ADC_Init(VBAT_SENS_ADC, &acdi);

		ADC_RegularChannelConfig(VBAT_SENS_ADC, VBAT_SENS_CHANNEL, 1, ADC_SampleTime_15Cycles);
		ADC_Cmd(VBAT_SENS_ADC, ENABLE);

	#else
		#error "CPU not define"
	#endif
	#endif

	if (VDC_NOT_CONNECT())
	{
		ADC_ITConfig(VBAT_SENS_ADC, ADC_IT_EOC, ENABLE);
		// Need measure VBAT level and stop if low
		ADC_START();
		while ((GlobalStatus & VBAT_READY) == 0);
		GlobalStatus |= VBAT_STOPPED;
		if (VBATValue <= VBAT_ONLY_OFF)
		{
			ADC_ITConfig(VBAT_SENS_ADC, ADC_IT_EOC, DISABLE);
			ADC_Cmd(VBAT_SENS_ADC, DISABLE);
			VBAT_SENS_CLOCK(DISABLE);
			return 1;	// Very low battery level, stop immediatly
		}
		if (VBATValue <= VBAT_ONLY_OFF)
		{
			ADC_ITConfig(VBAT_SENS_ADC, ADC_IT_EOC, DISABLE);
			ADC_Cmd(VBAT_SENS_ADC, DISABLE);
			VBAT_SENS_CLOCK(DISABLE);
			return 2;	// Low battery level, send SMS and stop
		}
		ADC_ITConfig(VBAT_SENS_ADC, ADC_IT_EOC, DISABLE);
	}
#endif

	USB_DISC_INIT();
	USB_P_INIT();

#if (USE_SERIAL_DEBUG)

	USART_StructInit(&USART_InitStructure);
	USART_InitStructure.USART_BaudRate = 115200;
	USART_Init(DEBUG_USART, &USART_InitStructure);
	USART_Cmd(DEBUG_USART, ENABLE);

	DEBUG_PIN_INIT();
	DebugInit();

#endif

	SD_INIT();
	ACC_INIT();

	SENS_D1_INIT();
	SENS_A1_INIT();

	RELAY_INIT();

	GPS_REMAP();
	GPS_PWR_INIT();
	GPS_TX_DEINIT();
	GPS_RX_DEINIT();

	GSM_REMAP();
	GSM_PWR_INIT();
	GSM_ON_INIT();
	GSM_READY_INIT();
	GSM_PIN_DEINIT();

	CAN_PIN_DEINIT();

#if (USE_CAN_SUPPORT)
	CAN_INIT();
#endif

#ifndef BL_VERSION

	#if (USE_SERIAL_DEBUG)
		nvi.NVIC_IRQChannel = DEBUG_USART_IRQn;
		NVIC_Init(&nvi);
		NVIC_SetPriority (DEBUG_USART_IRQn, 14);
	#endif

	#ifdef GPS_USART_IRQn
		nvi.NVIC_IRQChannel = GPS_USART_IRQn;
		NVIC_Init(&nvi);
		NVIC_SetPriority (GPS_USART_IRQn, 3);
	#endif

	#ifdef GSM_USART_IRQn
		nvi.NVIC_IRQChannel = GSM_USART_IRQn;
		NVIC_Init(&nvi);
		NVIC_SetPriority (GSM_USART_IRQn, 4);
	#endif

	#if (USE_CAN_SUPPORT)
		#ifdef CAN_RX0_IRQn
			nvi->NVIC_IRQChannel = CAN_RX0_IRQn;
			NVIC_Init(nvi);
			NVIC_SetPriority (CAN_RX0_IRQn, 1);
		#endif

		#ifdef CAN_RX1_IRQn
			nvi.NVIC_IRQChannel = CAN_RX1_IRQn;
			NVIC_Init(&nvi);
			NVIC_SetPriority (CAN_RX1_IRQn, 1);
		#endif

		#ifdef CAN_TX_IRQn
			nvi.NVIC_IRQChannel = CAN_TX_IRQn;
			NVIC_Init(&nvi);
			NVIC_SetPriority (CAN_TX_IRQn, 2);
		#endif

		#ifdef CAN_SCE_IRQn
			nvi.NVIC_IRQChannel = CAN_SCE_IRQn;
			NVIC_Init(&nvi);
			NVIC_SetPriority (CAN_TX_IRQn, 2);
		#endif

	#endif	// USE_CAN_SUPPORT

#endif	// BL_VERSION

#ifndef BL_VERSION
	Enable_WWDG();
#endif

	return 0;
}


#if defined ( VBAT_SENS_PIN )

void VBAT_SENS_IRQHandler(void)
{
	++VBATCount;
	VBATSum += ADC_GetConversionValue(VBAT_SENS_ADC);
	if (VBATCount >= 10)
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
		ADC_START();
	}
}
#endif

extern uint32_t SystemCoreClock;

void DWT_Init(void)
{
	if (!(CoreDebug->DEMCR & CoreDebug_DEMCR_TRCENA_Msk))
	{
		CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
		DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
	}
	DWT->CYCCNT = 0;
}

uint32_t DWT_Get(void)
{
	return DWT->CYCCNT;
}

void DWT_Delay_ms(uint16_t ms)
{
	uint32_t end = (uint32_t)ms * (SystemCoreClock / 1000);
	DWT_Init();
	while (DWT->CYCCNT < end)
	{ }
}

void DWT_Delay_us(uint16_t us)
{
	uint32_t end = us * (SystemCoreClock / 1000000);
	DWT_Init();
	while (DWT->CYCCNT < end)
	{ }
}
