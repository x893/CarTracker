#ifndef FREERTOS_CONFIG_H
#define FREERTOS_CONFIG_H

#include "hardware.h"

/*-----------------------------------------------------------
 * Application specific definitions.
 *
 * These definitions should be adjusted for your particular hardware and
 * application requirements.
 *
 * THESE PARAMETERS ARE DESCRIBED WITHIN THE 'CONFIGURATION' SECTION OF THE
 * FreeRTOS API DOCUMENTATION AVAILABLE ON THE FreeRTOS.org WEB SITE.
 *
 * See http://www.freertos.org/a00110.html.
 *----------------------------------------------------------*/
typedef int portSAVE_INT_STATUS;

#define configUSE_PREEMPTION			0
#define configUSE_IDLE_HOOK				0
#define configUSE_TICK_HOOK				1
#define configUSE_MALLOC_FAILED_HOOK	1


#define configCPU_CLOCK_HZ				SystemCoreClock

#define configTICK_RATE_HZ				(( portTickType ) 1000)
#define configMAX_PRIORITIES			(( unsigned portBASE_TYPE ) 2)
#define configMINIMAL_STACK_SIZE		(( unsigned short ) (  256 / 4))

#define configTOTAL_HEAP_SIZE			(( size_t )( 3 * 1024 ))
#define configMAX_TASK_NAME_LEN			1
#define configUSE_TRACE_FACILITY		0
#define configUSE_16_BIT_TICKS			0
#define configIDLE_SHOULD_YIELD			1

/* Co-routine definitions. */
#define configUSE_CO_ROUTINES 			0
#define configMAX_CO_ROUTINE_PRIORITIES ( 2 )

#define configUSE_MUTEXES				0
#define configUSE_COUNTING_SEMAPHORES 	0
#define configUSE_ALTERNATIVE_API 		0

#ifdef DISPLAY_STACK
	#define configCHECK_FOR_STACK_OVERFLOW			2
	#define INCLUDE_uxTaskGetStackHighWaterMark		1
#else
	#define configCHECK_FOR_STACK_OVERFLOW			0
	#define INCLUDE_uxTaskGetStackHighWaterMark		0
#endif

#define configUSE_RECURSIVE_MUTEXES		0
#define configQUEUE_REGISTRY_SIZE		0
#define configGENERATE_RUN_TIME_STATS	0

/* Set the following definitions to 1 to include the API function, or zero
to exclude the API function. */

#define INCLUDE_vTaskPrioritySet		0
#define INCLUDE_uxTaskPriorityGet		0
#define INCLUDE_vTaskDelete				0
#define INCLUDE_vTaskCleanUpResources	0
#define INCLUDE_vTaskSuspend			0
#define INCLUDE_vTaskDelayUntil			0
#define INCLUDE_vTaskDelay				1

/* This is the raw value as per the Cortex-M3 NVIC.  Values can be 255
(lowest) to 0 (1?) (highest). */
#define configKERNEL_INTERRUPT_PRIORITY 		255
#define configMAX_SYSCALL_INTERRUPT_PRIORITY 	191 /* equivalent to 0xb0, or priority 11. */


/* This is the value being used as per the ST library which permits 16
priority values, 0 to 15.  This must correspond to the
configKERNEL_INTERRUPT_PRIORITY setting.  Here 15 corresponds to the lowest
NVIC value of 255. */
#define configLIBRARY_KERNEL_INTERRUPT_PRIORITY	15

/*-----------------------------------------------------------
 * UART configuration.
 *-----------------------------------------------------------*/
#define configCOM0_RX_BUFFER_LENGTH		128
#define configCOM0_TX_BUFFER_LENGTH		128
#define configCOM1_RX_BUFFER_LENGTH		128
#define configCOM1_TX_BUFFER_LENGTH		128

#define vPortSVCHandler		SVC_Handler
#define xPortPendSVHandler	PendSV_Handler
#define xPortSysTickHandler	SysTick_Handler

#endif /* FREERTOS_CONFIG_H */
