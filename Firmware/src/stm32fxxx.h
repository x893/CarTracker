#ifndef __STM32XXX_H__
#define __STM32XXX_H__

#if   defined( STM32F2XX )
	#include "stm32f2xx.h"
#elif defined( STM32F4XX )
	#include "stm32f4xx.h"
#elif defined( STM32F10X_HD ) || defined( STM32F10X_MD_VL )
	#include "stm32f10x.h"
#else
	#error "CPU type not define"
#endif

#endif
