#ifndef __STM32F_CONF_H
#define __STM32F_CONF_H

#include "hardware.h"

#if	defined (STM32F10X_HD )	|| \
	defined (STM32F10X_MD )	|| \
	defined (STM32F10X_MD_VL )

#if defined( VDC_CONNECT )
	#include "stm32f10x_adc.h"
#endif
//	#include "stm32f10x_bkp.h"
#if ( USE_CAN_SUPPORT )
	#include "stm32f10x_can.h"
#endif
//	#include "stm32f10x_cec.h"
	#include "stm32f10x_crc.h"
//	#include "stm32f10x_dac.h"
	#include "stm32f10x_dbgmcu.h"
//	#include "stm32f10x_dma.h"
//	#include "stm32f10x_exti.h"
	#include "stm32f10x_flash.h"
//	#include "stm32f10x_fsmc.h"
	#include "stm32f10x_gpio.h"
//	#include "stm32f10x_i2c.h"
//	#include "stm32f10x_iwdg.h"
	#include "stm32f10x_pwr.h"
	#include "stm32f10x_rcc.h"
//	#include "stm32f10x_rtc.h"
//	#include "stm32f10x_sdio.h"
//	#include "stm32f10x_spi.h"
//	#include "stm32f10x_tim.h"
	#include "stm32f10x_usart.h"
	#include "stm32f10x_wwdg.h"
	#include "misc.h"

#elif defined ( STM32F2XX )	|| \
	  defined ( STM32F4XX )

#ifdef ( VDC_CONNECT )
	#include "stm32f2xx_adc.h"
#endif
#if ( USE_CAN_SUPPORT )
	#include "stm32f2xx_can.h"
#endif
	#include "stm32f2xx_crc.h"
//	#include "stm32f2xx_cryp.h"
//	#include "stm32f2xx_dac.h"
	#include "stm32f2xx_dbgmcu.h"
//	#include "stm32f2xx_dcmi.h"
//	#include "stm32f2xx_dma.h"
//	#include "stm32f2xx_exti.h"
	#include "stm32f2xx_flash.h"
//	#include "stm32f2xx_fsmc.h"
//	#include "stm32f2xx_hash.h"
	#include "stm32f2xx_gpio.h"
//	#include "stm32f2xx_i2c.h"
	#include "stm32f2xx_iwdg.h"
//	#include "stm32f2xx_pwr.h"
	#include "stm32f2xx_rcc.h"
//	#include "stm32f2xx_rng.h"
//	#include "stm32f2xx_rtc.h"
//	#include "stm32f2xx_sdio.h"
//	#include "stm32f2xx_spi.h"
	#include "stm32f2xx_syscfg.h"
//	#include "stm32f2xx_tim.h"
	#include "stm32f2xx_usart.h"
	#include "stm32f2xx_wwdg.h"
	#include "misc.h"

	/* If an external clock source is used, then the value of the following define 
	   should be set to the value of the external clock source, else, if no external 
	   clock is used, keep this define commented
	*/
	/* #define I2S_EXTERNAL_CLOCK_VAL   12288000 */ /* Value of the external clock in Hz */

#else
	#error "CPU type not define"
#endif
/* Uncomment the line below to expanse the "assert_param" macro in the 
   Standard Peripheral Library drivers code */
/* #define USE_FULL_ASSERT    1 */

#ifdef  USE_FULL_ASSERT
	/**
	* @brief  The assert_param macro is used for function's parameters check.
	* @param  expr: If expr is false, it calls assert_failed function
	*   which reports the name of the source file and the source
	*   line number of the call that failed. 
	*   If expr is true, it returns no value.
	* @retval None
	*/
	#define assert_param(expr) ((expr) ? (void)0 : assert_failed((uint8_t *)__FILE__, __LINE__))
	void assert_failed(uint8_t* file, uint32_t line);
#else
	#define assert_param(expr) ((void)0)
#endif /* USE_FULL_ASSERT */

#endif /* __STM32F_CONF_H */
