
	IF		:DEF:BL_VERSION
Stack_Size			EQU		0x000400
	ELSE
Stack_Size			EQU		0x000100
	ENDIF
					EXPORT Stack_Mem
					AREA	STACK, NOINIT, READWRITE, ALIGN = 3
Stack_Mem			SPACE	Stack_Size
__initial_sp

Heap_Size			EQU		0x00000000
					AREA	HEAP, NOINIT, READWRITE, ALIGN = 3
__heap_base
Heap_Mem			SPACE	Heap_Size
__heap_limit

					PRESERVE8
					THUMB

					AREA	RESET, DATA, READONLY
					EXPORT	__Vectors
					EXPORT	__Vectors_End
					EXPORT	__Vectors_Size

__Vectors			DCD	__initial_sp			; Top of Stack
					DCD	Reset_Handler			; Reset Handler
					DCD	NMI_Handler				; NMI Handler
					DCD	HardFault_Handler		; Hard Fault Handler
					DCD	MemManage_Handler		; MPU Fault Handler
					DCD	BusFault_Handler		; Bus Fault Handler
					DCD	UsageFault_Handler		; Usage Fault Handler
					DCD	0						; Reserved
					DCD	0						; Reserved
					DCD	0						; Reserved
					DCD	0						; Reserved
					DCD	SVC_Handler				; SVCall Handler
					DCD	DebugMon_Handler		; Debug Monitor Handler
					DCD	0						; Reserved
					DCD	PendSV_Handler			; PendSV Handler
					DCD	SysTick_Handler			; SysTick Handler

					; External Interrupts
					DCD	WWDG_IRQHandler			; Window Watchdog
					DCD	PVD_IRQHandler			; PVD through EXTI Line detect
					DCD	TAMP_STAMP_IRQHandler	; Tamper and TimeStamps through the EXTI line
					DCD	RTC_WKUP_IRQHandler		; RTC Wakeup through the EXTI line

					DCD	 FLASH_IRQHandler			; Flash
					DCD	 RCC_IRQHandler				; RCC
					DCD	 EXTI0_IRQHandler			; EXTI Line 0
					DCD	 EXTI1_IRQHandler			; EXTI Line 1
					DCD	 EXTI2_IRQHandler			; EXTI Line 2
					DCD	 EXTI3_IRQHandler			; EXTI Line 3
					DCD	 EXTI4_IRQHandler			; EXTI Line 4
					
					DCD	 DMA1_Stream0_IRQHandler	; DMA1 Stream 0
					DCD	 DMA1_Stream1_IRQHandler	; DMA1 Stream 1
					DCD	 DMA1_Stream2_IRQHandler	; DMA1 Stream 2
					DCD	 DMA1_Stream3_IRQHandler	; DMA1 Stream 3
					DCD	 DMA1_Stream4_IRQHandler	; DMA1 Stream 4
					DCD	 DMA1_Stream5_IRQHandler	; DMA1 Stream 5
					DCD	 DMA1_Stream6_IRQHandler	; DMA1 Stream 6
					DCD	 ADC_IRQHandler				; ADC1, ADC2 and ADC3s

					DCD	 CAN1_TX_IRQHandler			; CAN1 TX
					DCD	 CAN1_RX0_IRQHandler		; CAN1 RX0
					DCD	 CAN1_RX1_IRQHandler		; CAN1 RX1
					DCD	 CAN1_SCE_IRQHandler		; CAN1 SCE

					DCD	 EXTI9_5_IRQHandler			; EXTI Line 9..5

					DCD	 TIM1_BRK_TIM9_IRQHandler		; TIM1 Break and TIM9					
					DCD	 TIM1_UP_TIM10_IRQHandler		; TIM1 Update and TIM10				 
					DCD	 TIM1_TRG_COM_TIM11_IRQHandler	; TIM1 Trigger and Commutation and TIM11
					DCD	 TIM1_CC_IRQHandler			; TIM1 Capture Compare

					DCD	 TIM2_IRQHandler			; TIM2
					DCD	 TIM3_IRQHandler			; TIM3
					DCD	 TIM4_IRQHandler			; TIM4

					DCD	 I2C1_EV_IRQHandler			; I2C1 Event
					DCD	 I2C1_ER_IRQHandler			; I2C1 Error
					DCD	 I2C2_EV_IRQHandler			; I2C2 Event
					DCD	 I2C2_ER_IRQHandler			; I2C2 Error
					DCD	 SPI1_IRQHandler			; SPI1
					DCD	 SPI2_IRQHandler			; SPI2
					DCD	 USART1_IRQHandler			; USART1
					DCD	 USART2_IRQHandler			; USART2
					DCD	 USART3_IRQHandler			; USART3
					DCD	 EXTI15_10_IRQHandler		; EXTI Line 15..10

					DCD	 RTC_Alarm_IRQHandler			; RTC Alarm (A and B) through EXTI Line
					DCD	 OTG_FS_WKUP_IRQHandler			; USB OTG FS Wakeup through EXTI line
					
					DCD	 TIM8_BRK_TIM12_IRQHandler		; TIM8 Break and TIM12					
					DCD	 TIM8_UP_TIM13_IRQHandler		; TIM8 Update and TIM13				 
					DCD	 TIM8_TRG_COM_TIM14_IRQHandler	; TIM8 Trigger and Commutation and TIM14
					DCD	 TIM8_CC_IRQHandler			; TIM8 Capture Compare
					DCD	 DMA1_Stream7_IRQHandler	; DMA1 Stream7 / ADC3
					DCD	 FSMC_IRQHandler			; FSMC
					DCD	 SDIO_IRQHandler			; SDIO
					DCD	 TIM5_IRQHandler			; TIM5
					DCD	 SPI3_IRQHandler			; SPI3
					DCD	 UART4_IRQHandler			; UART4
					DCD	 UART5_IRQHandler			; UART5

					DCD  TIM6_DAC_IRQHandler		; TIM6 and DAC1&2 underrun errors / DAC
					DCD	 TIM7_IRQHandler			; TIM7

					DCD	DMA2_Stream0_IRQHandler		; DMA2 Stream 0
					DCD	DMA2_Stream1_IRQHandler		; DMA2 Stream 1
					DCD	DMA2_Stream2_IRQHandler		; DMA2 Stream 2
					DCD	DMA2_Stream3_IRQHandler		; DMA2 Stream 3
					DCD	DMA2_Stream4_IRQHandler		; DMA2 Stream 4
					DCD	ETH_IRQHandler				; Ethernet
					DCD	ETH_WKUP_IRQHandler			; Ethernet Wakeup through EXTI line
					DCD	CAN2_TX_IRQHandler			; CAN2 TX
					DCD	CAN2_RX0_IRQHandler			; CAN2 RX0
					DCD	CAN2_RX1_IRQHandler			; CAN2 RX1
					DCD	CAN2_SCE_IRQHandler			; CAN2 SCE
					DCD	OTG_FS_IRQHandler			; USB OTG FS
					DCD	DMA2_Stream5_IRQHandler		; DMA2 Stream 5
					DCD	DMA2_Stream6_IRQHandler		; DMA2 Stream 6
					DCD	DMA2_Stream7_IRQHandler		; DMA2 Stream 7
					DCD	USART6_IRQHandler			; USART6
					DCD	I2C3_EV_IRQHandler			; I2C3 event
					DCD	I2C3_ER_IRQHandler			; I2C3 error
					DCD	OTG_HS_EP1_OUT_IRQHandler	; USB OTG HS End Point 1 Out
					DCD	OTG_HS_EP1_IN_IRQHandler	; USB OTG HS End Point 1 In
					DCD	OTG_HS_WKUP_IRQHandler		; USB OTG HS Wakeup through EXTI
					DCD	OTG_HS_IRQHandler			; USB OTG HS
					DCD	DCMI_IRQHandler				; DCMI
					DCD	CRYP_IRQHandler				; CRYP crypto
					DCD	HASH_RNG_IRQHandler			; Hash and Rng
					DCD	FPU_IRQHandler				; FPU

					EXPORT	FirmwareMagic
					EXPORT	FirmwareCS
					EXPORT	FirmwareSize
FirmwareMagic		DCD		0x19630605				; Magic word
FirmwareCS			DCD		0xAA55BB66				; Checksum for image
FirmwareSize		DCD		0x0						; Image size

__Vectors_End

__Vectors_Size		EQU	__Vectors_End - __Vectors

					AREA	|.text|, CODE, READONLY
				
					EXPORT	Reset_Handler			 		[WEAK]
					IMPORT	__main
					IMPORT	SystemInit
					
Reset_Handler		PROC
					LDR		R0, = SystemInit
					BLX		R0				
					LDR		R0, = __main
					BX		R0
					ENDP

; Dummy Exception Handlers (infinite loops which can be modified)

					EXPORT	NMI_Handler						[WEAK]
					EXPORT	HardFault_Handler				[WEAK]
					EXPORT	MemManage_Handler				[WEAK]
					EXPORT	BusFault_Handler				[WEAK]
					EXPORT	UsageFault_Handler				[WEAK]
					EXPORT	SVC_Handler						[WEAK]
					EXPORT	DebugMon_Handler				[WEAK]
					EXPORT	PendSV_Handler					[WEAK]
					EXPORT	SysTick_Handler					[WEAK]
NMI_Handler
HardFault_Handler
MemManage_Handler
BusFault_Handler
UsageFault_Handler
SVC_Handler
PendSV_Handler
DebugMon_Handler
SysTick_Handler

					EXPORT	Exception_Handler			[WEAK]
Exception_Handler	PROC
					LDR		R0, =Exception_Handler
					BX		R0
					ENDP

					EXPORT	WWDG_IRQHandler					[WEAK]
					EXPORT	PVD_IRQHandler					[WEAK]
					EXPORT	TAMP_STAMP_IRQHandler			[WEAK]         
					EXPORT	RTC_WKUP_IRQHandler				[WEAK]                     
					EXPORT	TAMPER_IRQHandler				[WEAK]
					EXPORT	RTC_IRQHandler					[WEAK]
					EXPORT	FLASH_IRQHandler				[WEAK]
					EXPORT	RCC_IRQHandler					[WEAK]
					EXPORT	EXTI0_IRQHandler				[WEAK]
					EXPORT	EXTI1_IRQHandler				[WEAK]
					EXPORT	EXTI2_IRQHandler				[WEAK]
					EXPORT	EXTI3_IRQHandler				[WEAK]
					EXPORT	EXTI4_IRQHandler				[WEAK]

					EXPORT	DMA1_Stream0_IRQHandler			[WEAK]
					EXPORT	DMA1_Stream1_IRQHandler			[WEAK]
					EXPORT	DMA1_Stream2_IRQHandler			[WEAK]
					EXPORT	DMA1_Stream3_IRQHandler			[WEAK]
					EXPORT	DMA1_Stream4_IRQHandler			[WEAK]
					EXPORT	DMA1_Stream5_IRQHandler			[WEAK]
					EXPORT	DMA1_Stream6_IRQHandler			[WEAK]
					EXPORT	ADC_IRQHandler					[WEAK]
					EXPORT	CAN1_TX_IRQHandler				[WEAK]
					EXPORT	CAN1_RX0_IRQHandler				[WEAK]

					EXPORT	DMA1_Channel1_IRQHandler		[WEAK]
					EXPORT	DMA1_Channel2_IRQHandler		[WEAK]
					EXPORT	DMA1_Channel3_IRQHandler		[WEAK]
					EXPORT	DMA1_Channel4_IRQHandler		[WEAK]
					EXPORT	DMA1_Channel5_IRQHandler		[WEAK]
					EXPORT	DMA1_Channel6_IRQHandler		[WEAK]
					EXPORT	DMA1_Channel7_IRQHandler		[WEAK]

					EXPORT	CAN1_RX1_IRQHandler				[WEAK]
					EXPORT	CAN1_SCE_IRQHandler				[WEAK]
					EXPORT	EXTI9_5_IRQHandler				[WEAK]

					EXPORT	TIM1_BRK_TIM9_IRQHandler		[WEAK]
					EXPORT	TIM1_UP_TIM10_IRQHandler		[WEAK]
					EXPORT	TIM1_TRG_COM_TIM11_IRQHandler	[WEAK]

					EXPORT	TIM1_BRK_IRQHandler				[WEAK]
					EXPORT	TIM1_UP_IRQHandler				[WEAK]
					EXPORT	TIM1_TRG_COM_IRQHandler			[WEAK]
					EXPORT	TIM1_CC_IRQHandler				[WEAK]
					EXPORT	TIM2_IRQHandler					[WEAK]
					EXPORT	TIM3_IRQHandler					[WEAK]
					EXPORT	TIM4_IRQHandler					[WEAK]
					EXPORT	I2C1_EV_IRQHandler				[WEAK]
					EXPORT	I2C1_ER_IRQHandler				[WEAK]
					EXPORT	I2C2_EV_IRQHandler				[WEAK]
					EXPORT	I2C2_ER_IRQHandler				[WEAK]
					EXPORT	SPI1_IRQHandler					[WEAK]
					EXPORT	SPI2_IRQHandler					[WEAK]
					EXPORT	USART1_IRQHandler				[WEAK]
					EXPORT	USART2_IRQHandler				[WEAK]
					EXPORT	USART3_IRQHandler				[WEAK]
					EXPORT	EXTI15_10_IRQHandler			[WEAK]
					
					EXPORT	RTC_Alarm_IRQHandler			[WEAK]
					EXPORT	OTG_FS_WKUP_IRQHandler			[WEAK]
					EXPORT	TIM8_BRK_TIM12_IRQHandler		[WEAK]
					EXPORT	TIM8_UP_TIM13_IRQHandler		[WEAK]
					EXPORT	TIM8_TRG_COM_TIM14_IRQHandler	[WEAK]

					EXPORT	RTCAlarm_IRQHandler				[WEAK]
					EXPORT	USBWakeUp_IRQHandler			[WEAK]
					EXPORT	TIM8_BRK_IRQHandler				[WEAK]
					EXPORT	TIM8_UP_IRQHandler				[WEAK]
					EXPORT	TIM8_TRG_COM_IRQHandler			[WEAK]
					
					EXPORT	TIM8_CC_IRQHandler				[WEAK]
					
					EXPORT	DMA1_Stream7_IRQHandler			[WEAK]
					
					EXPORT	FSMC_IRQHandler					[WEAK]
					EXPORT	SDIO_IRQHandler					[WEAK]
					EXPORT	TIM5_IRQHandler					[WEAK]
					EXPORT	SPI3_IRQHandler					[WEAK]
					EXPORT	UART4_IRQHandler				[WEAK]
					EXPORT	UART5_IRQHandler				[WEAK]
					
					EXPORT	TIM6_DAC_IRQHandler				[WEAK]
					EXPORT	TIM6_IRQHandler					[WEAK]

					EXPORT	TIM7_IRQHandler					[WEAK]

					EXPORT	DMA2_Stream0_IRQHandler			[WEAK]
					EXPORT	DMA2_Stream1_IRQHandler			[WEAK]
					EXPORT	DMA2_Stream2_IRQHandler			[WEAK]
					EXPORT	DMA2_Stream3_IRQHandler			[WEAK]
					EXPORT	DMA2_Stream4_IRQHandler			[WEAK]
					EXPORT	ETH_IRQHandler					[WEAK]
					EXPORT	ETH_WKUP_IRQHandler				[WEAK]
					EXPORT	CAN2_TX_IRQHandler				[WEAK]
					EXPORT	CAN2_RX0_IRQHandler				[WEAK]
					EXPORT	CAN2_RX1_IRQHandler				[WEAK]
					EXPORT	CAN2_SCE_IRQHandler				[WEAK]
					EXPORT	OTG_FS_IRQHandler				[WEAK]
					EXPORT	DMA2_Stream5_IRQHandler			[WEAK]
					EXPORT	DMA2_Stream6_IRQHandler			[WEAK]
					EXPORT	DMA2_Stream7_IRQHandler			[WEAK]
					EXPORT	USART6_IRQHandler				[WEAK]
					EXPORT	I2C3_EV_IRQHandler				[WEAK]
					EXPORT	I2C3_ER_IRQHandler				[WEAK]
					EXPORT	OTG_HS_EP1_OUT_IRQHandler		[WEAK]
					EXPORT	OTG_HS_EP1_IN_IRQHandler		[WEAK]
					EXPORT	OTG_HS_WKUP_IRQHandler			[WEAK]
					EXPORT	OTG_HS_IRQHandler				[WEAK]
					EXPORT	DCMI_IRQHandler					[WEAK]
					EXPORT	CRYP_IRQHandler					[WEAK]
					EXPORT	HASH_RNG_IRQHandler				[WEAK]
					
					EXPORT	FPU_IRQHandler					[WEAK]

					EXPORT	DMA2_Channel1_IRQHandler		[WEAK]
					EXPORT	DMA2_Channel2_IRQHandler		[WEAK]
					EXPORT	DMA2_Channel3_IRQHandler		[WEAK]
					EXPORT	DMA2_Channel4_5_IRQHandler		[WEAK]

WWDG_IRQHandler
PVD_IRQHandler
TAMP_STAMP_IRQHandler
RTC_WKUP_IRQHandler
TAMPER_IRQHandler
RTC_IRQHandler
FLASH_IRQHandler
RCC_IRQHandler
EXTI0_IRQHandler
EXTI1_IRQHandler
EXTI2_IRQHandler
EXTI3_IRQHandler
EXTI4_IRQHandler

DMA1_Stream0_IRQHandler
DMA1_Stream1_IRQHandler
DMA1_Stream2_IRQHandler
DMA1_Stream3_IRQHandler
DMA1_Stream4_IRQHandler
DMA1_Stream5_IRQHandler
DMA1_Stream6_IRQHandler
ADC_IRQHandler
CAN1_TX_IRQHandler
CAN1_RX0_IRQHandler

DMA1_Channel1_IRQHandler
DMA1_Channel2_IRQHandler
DMA1_Channel3_IRQHandler
DMA1_Channel4_IRQHandler
DMA1_Channel5_IRQHandler
DMA1_Channel6_IRQHandler
DMA1_Channel7_IRQHandler

CAN1_RX1_IRQHandler
CAN1_SCE_IRQHandler
EXTI9_5_IRQHandler

TIM1_BRK_TIM9_IRQHandler
TIM1_UP_TIM10_IRQHandler
TIM1_TRG_COM_TIM11_IRQHandler

TIM1_BRK_IRQHandler
TIM1_UP_IRQHandler
TIM1_TRG_COM_IRQHandler

TIM1_CC_IRQHandler
TIM2_IRQHandler
TIM3_IRQHandler
TIM4_IRQHandler
I2C1_EV_IRQHandler
I2C1_ER_IRQHandler
I2C2_EV_IRQHandler
I2C2_ER_IRQHandler
SPI1_IRQHandler
SPI2_IRQHandler
USART1_IRQHandler
USART2_IRQHandler
USART3_IRQHandler
EXTI15_10_IRQHandler

RTC_Alarm_IRQHandler
OTG_FS_WKUP_IRQHandler
TIM8_BRK_TIM12_IRQHandler
TIM8_UP_TIM13_IRQHandler
TIM8_TRG_COM_TIM14_IRQHandler

RTCAlarm_IRQHandler
USBWakeUp_IRQHandler
TIM8_BRK_IRQHandler
TIM8_UP_IRQHandler
TIM8_TRG_COM_IRQHandler

TIM8_CC_IRQHandler

DMA1_Stream7_IRQHandler

FSMC_IRQHandler
SDIO_IRQHandler
TIM5_IRQHandler
SPI3_IRQHandler
UART4_IRQHandler
UART5_IRQHandler

TIM6_DAC_IRQHandler
TIM6_IRQHandler

TIM7_IRQHandler

DMA2_Stream0_IRQHandler
DMA2_Stream1_IRQHandler
DMA2_Stream2_IRQHandler
DMA2_Stream3_IRQHandler
DMA2_Stream4_IRQHandler
ETH_IRQHandler
ETH_WKUP_IRQHandler
CAN2_TX_IRQHandler
CAN2_RX0_IRQHandler
CAN2_RX1_IRQHandler
CAN2_SCE_IRQHandler
OTG_FS_IRQHandler
DMA2_Stream5_IRQHandler
DMA2_Stream6_IRQHandler
DMA2_Stream7_IRQHandler
USART6_IRQHandler
I2C3_EV_IRQHandler
I2C3_ER_IRQHandler
OTG_HS_EP1_OUT_IRQHandler
OTG_HS_EP1_IN_IRQHandler
OTG_HS_WKUP_IRQHandler
OTG_HS_IRQHandler
DCMI_IRQHandler
CRYP_IRQHandler
HASH_RNG_IRQHandler
FPU_IRQHandler

DMA2_Channel1_IRQHandler
DMA2_Channel2_IRQHandler
DMA2_Channel3_IRQHandler
DMA2_Channel4_5_IRQHandler

				EXPORT Default_Handler		[WEAK]
Default_Handler PROC
				LDR		R0, =Default_Handler
				BX		R0
				ENDP

				ALIGN

;*******************************************************************************
; User Stack and Heap initialization
;*******************************************************************************
	 IF		:DEF:__MICROLIB
				EXPORT	__initial_sp
				EXPORT	__heap_base
				EXPORT	__heap_limit
	 ELSE
				IMPORT	__use_two_region_memory
				EXPORT	__user_initial_stackheap

__user_initial_stackheap
				LDR	R0, = Heap_Mem
				LDR	R1, = (Stack_Mem + Stack_Size)
				LDR	R2, = (Heap_Mem +	Heap_Size)
				LDR	R3, = Stack_Mem
				BX	LR

				ALIGN

	 ENDIF

				END
