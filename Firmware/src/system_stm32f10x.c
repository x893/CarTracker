#include "stm32f10x.h"

#include "hardware.h"
void ShowFatalError(u16 errorCode);
    
#if		defined (STM32F10X_LD_VL)	|| \
		(defined STM32F10X_MD_VL)	|| \
		(defined STM32F10X_HD_VL)

	#define SYSCLK_FREQ_24MHz  24000000

#elif	defined (STM32F10X_MD)		|| \
		defined (STM32F101RC)

	#define SYSCLK_FREQ_36MHz  36000000

#elif defined (STM32F10X_HD)

	#define SYSCLK_FREQ_72MHz  72000000

#else
	#error "CPU not define"
#endif

#ifndef VECT_TAB_OFFSET
	#error "Not define VECT_TAB_OFFSET"
#endif

/*******************************************************************************
*  Clock Definitions
*******************************************************************************/
uint32_t SystemCoreClock = HSI_VALUE;	/*!< System Clock Frequency (Core Clock) */

#ifdef SYSCLK_FREQ_HSE
	void SetSysClockToHSE(void);
#elif defined SYSCLK_FREQ_24MHz
	void SetSysClockTo24(void);
#elif defined SYSCLK_FREQ_36MHz
	void SetSysClockTo36(void);
#elif defined SYSCLK_FREQ_48MHz
	void SetSysClockTo48(void);
#elif defined SYSCLK_FREQ_56MHz
	void SetSysClockTo56(void);  
#elif defined SYSCLK_FREQ_72MHz
	void SetSysClockTo72(void);
#endif

const uint8_t AHBPrescTable[16] = {0, 0, 0, 0, 0, 0, 0, 0, 1, 2, 3, 4, 6, 7, 8, 9};

/**
  * @brief  Setup the microcontroller system
  *         Initialize the Embedded Flash Interface, the PLL and update the 
  *         SystemCoreClock variable.
  * @note   This function should be used only after reset.
  * @param  None
  * @retval None
  */
void SystemInit (void)
{
	/* Reset the RCC clock configuration to the default reset state(for debug purpose) */
	/* Set HSION bit */
	RCC->CR |= RCC_CR_HSION;

	/* Reset SW, HPRE, PPRE1, PPRE2, ADCPRE and MCO bits */
	RCC->CFGR &= (uint32_t)0xF8FF0000;
  
	/* Reset HSEON, CSSON and PLLON bits */
	RCC->CR &= ~(RCC_CR_HSEON | RCC_CR_CSSON | RCC_CR_PLLON);

	/* Reset HSEBYP bit */
	RCC->CR &= ~RCC_CR_HSEBYP;

	/* Reset PLLSRC, PLLXTPRE, PLLMUL and USBPRE/OTGFSPRE bits */
	RCC->CFGR &= (uint32_t)0xFF80FFFF;

	/* Disable all interrupts and clear pending bits  */
	RCC->CIR = (RCC_CIR_LSIRDYC | RCC_CIR_LSERDYC | RCC_CIR_HSIRDYC | RCC_CIR_HSERDYC | RCC_CIR_PLLRDYC | RCC_CIR_CSSC);
#if defined (STM32F10X_LD_VL) || defined (STM32F10X_MD_VL) || (defined STM32F10X_HD_VL)
	/* Reset CFGR2 register */
	RCC->CFGR2 = 0;
#endif
    
	/* Configure the System clock frequency, HCLK, PCLK2 and PCLK1 prescalers */
	/* Configure the Flash Latency cycles and enable prefetch buffer */

	SystemCoreClock = HSI_VALUE;
	
#ifdef SYSCLK_FREQ_HSE
	SetSysClockToHSE();
#elif defined SYSCLK_FREQ_24MHz
	SetSysClockTo24();
#elif defined SYSCLK_FREQ_36MHz
	SetSysClockTo36();
#elif defined SYSCLK_FREQ_48MHz
	SetSysClockTo48();
#elif defined SYSCLK_FREQ_56MHz
	SetSysClockTo56();  
#eli defined SYSCLK_FREQ_72MHz
	SetSysClockTo72();
#endif


#ifdef VECT_TAB_SRAM
	SCB->VTOR = SRAM_BASE | ((uint32_t)VECT_TAB_OFFSET); /* Vector Table Relocation in Internal SRAM. */
#else
	SCB->VTOR = FLASH_BASE | ((uint32_t)VECT_TAB_OFFSET); /* Vector Table Relocation in Internal FLASH. */
#endif
}

/**
  * @brief  Update SystemCoreClock according to Clock Register Values
  * @note   None
  * @param  None
  * @retval None
  */
void SystemCoreClockUpdate (void)
{
	uint32_t tmp = 0, pllmull = 0, pllsource = 0;

#if defined (STM32F10X_LD_VL) || defined (STM32F10X_MD_VL) || (defined STM32F10X_HD_VL)
	uint32_t prediv1factor = 0;
#endif

	/* Get SYSCLK source -------------------------------------------------------*/
	tmp = RCC->CFGR & RCC_CFGR_SWS;

	switch (tmp)
	{
		case 0x00:  /* HSI used as system clock */
			SystemCoreClock = HSI_VALUE;
			break;
		case 0x04:  /* HSE used as system clock */
			SystemCoreClock = HSE_VALUE;
			break;
		case 0x08:  /* PLL used as system clock */
			/* Get PLL clock source and multiplication factor ----------------------*/
			pllmull = RCC->CFGR & RCC_CFGR_PLLMULL;
			pllsource = RCC->CFGR & RCC_CFGR_PLLSRC;
      
			pllmull = ( pllmull >> 18) + 2;

			if (pllsource == 0x00)
			{
				/* HSI oscillator clock divided by 2 selected as PLL clock entry */
				SystemCoreClock = (HSI_VALUE >> 1) * pllmull;
			}
			else
			{
#if defined (STM32F10X_LD_VL) || defined (STM32F10X_MD_VL) || (defined STM32F10X_HD_VL)
				prediv1factor = (RCC->CFGR2 & RCC_CFGR2_PREDIV1) + 1;
				/* HSE oscillator clock selected as PREDIV1 clock entry */
				SystemCoreClock = (HSE_VALUE / prediv1factor) * pllmull; 
#else
				/* HSE selected as PLL clock entry */
				if ((RCC->CFGR & RCC_CFGR_PLLXTPRE) != (uint32_t)RESET)
				{	/* HSE oscillator clock divided by 2 */
					SystemCoreClock = (HSE_VALUE >> 1) * pllmull;
				}
				else
				{
					SystemCoreClock = HSE_VALUE * pllmull;
				}
#endif
			}
			break;
		default:
			SystemCoreClock = HSI_VALUE;
			break;
	}
  
	/* Compute HCLK clock frequency ----------------*/
	/* Get HCLK prescaler */
	tmp = AHBPrescTable[((RCC->CFGR & RCC_CFGR_HPRE) >> 4)];
	/* HCLK clock frequency */
	SystemCoreClock >>= tmp;  
}


#ifdef SYSCLK_FREQ_HSE
/**
  * @brief  Selects HSE as System clock source and configure HCLK, PCLK2
  *          and PCLK1 prescalers.
  * @note   This function should be used only after reset.
  * @param  None
  * @retval None
  */
void SetSysClockToHSE(void)
{
	__IO uint32_t StartUpCounter = 0, HSEStatus = 0;
  
	/* SYSCLK, HCLK, PCLK2 and PCLK1 configuration ---------------------------*/    
	/* Enable HSE */    
	RCC->CR |= ((uint32_t)RCC_CR_HSEON);
 
	/* Wait till HSE is ready and if Time out is reached exit */
	do
	{
		HSEStatus = RCC->CR & RCC_CR_HSERDY;
		StartUpCounter++;  
	} while((HSEStatus == 0) && (StartUpCounter != HSE_STARTUP_TIMEOUT));

	if ((RCC->CR & RCC_CR_HSERDY) != RESET)
	{
		HSEStatus = (uint32_t)0x01;
	}
	else
	{
		HSEStatus = (uint32_t)0x00;
	}  

	if (HSEStatus == (uint32_t)0x01)
	{

#if !defined STM32F10X_LD_VL && !defined STM32F10X_MD_VL && !defined STM32F10X_HD_VL
		/* Enable Prefetch Buffer */
		FLASH->ACR |= FLASH_ACR_PRFTBE;

		/* Flash 0 wait state */
		FLASH->ACR &= (uint32_t)((uint32_t)~FLASH_ACR_LATENCY);
		FLASH->ACR |= (uint32_t)FLASH_ACR_LATENCY_0;
#endif
 
		/* HCLK = SYSCLK */
		RCC->CFGR |= (uint32_t)RCC_CFGR_HPRE_DIV1;
      
		/* PCLK2 = HCLK */
		RCC->CFGR |= (uint32_t)RCC_CFGR_PPRE2_DIV1;
    
		/* PCLK1 = HCLK */
		RCC->CFGR |= (uint32_t)RCC_CFGR_PPRE1_DIV1;
    
		/* Select HSE as system clock source */
		RCC->CFGR &= (uint32_t)((uint32_t)~(RCC_CFGR_SW));
		RCC->CFGR |= (uint32_t)RCC_CFGR_SW_HSE;    

		/* Wait till HSE is used as system clock source */
		while ((RCC->CFGR & (uint32_t)RCC_CFGR_SWS) != (uint32_t)0x04)
		{ }
	}
	else
	{	/* If HSE fails to start-up, the application will have wrong clock 
         configuration. User can add here some code to deal with this error */
	}
}

#elif defined SYSCLK_FREQ_24MHz

/**
  * @brief  Sets System clock frequency to 24MHz and configure HCLK, PCLK2 
  *          and PCLK1 prescalers.
  * @note   This function should be used only after reset.
  * @param  None
  * @retval None
  */
#warning SYSCLK_FREQ_24MHz
void SetSysClockTo24(void)
{
	__IO uint32_t StartUpCounter = HSE_STARTUP_TIMEOUT;
  
	/* SYSCLK, HCLK, PCLK2 and PCLK1 configuration ---------------------------*/    
	/* Enable HSE */    
	RCC->CR |= RCC_CR_HSEON;
 
	/* Wait till HSE is ready and if Time out is reached exit */
	while (((RCC->CR & RCC_CR_HSERDY) == RESET) && (StartUpCounter-- != 0));
	if ((RCC->CR & RCC_CR_HSERDY) != RESET)
	{
		/* HCLK = SYSCLK */
		RCC->CFGR |= RCC_CFGR_HPRE_DIV1;
      
		/* PCLK2 = HCLK */
		RCC->CFGR |= RCC_CFGR_PPRE2_DIV1;
    
		/* PCLK1 = HCLK */
		RCC->CFGR |= RCC_CFGR_PPRE1_DIV1;
    
		/*  PLL configuration:  = (HSE / 2) * 6 = 24 MHz */
		RCC->CFGR &= ~(RCC_CFGR_PLLSRC | RCC_CFGR_PLLXTPRE | RCC_CFGR_PLLMULL);
#if defined (STM32F10X_LD_VL) || defined (STM32F10X_MD_VL) || defined (STM32F10X_HD_VL)
		RCC->CFGR |= (RCC_CFGR_PLLSRC_PREDIV1 | RCC_CFGR_PLLXTPRE_PREDIV1_Div2 | RCC_CFGR_PLLMULL6);
#else    
		/*  PLL configuration:  = (HSE / 2) * 6 = 24 MHz */
		RCC->CFGR |= (RCC_CFGR_PLLSRC_HSE | RCC_CFGR_PLLXTPRE_HSE_Div2 | RCC_CFGR_PLLMULL6);
#endif
		/* Enable PLL */
		RCC->CR |= RCC_CR_PLLON;

		/* Wait till PLL is ready */
		while((RCC->CR & RCC_CR_PLLRDY) == 0) { }

		/* Select PLL as system clock source */
		RCC->CFGR &= ~RCC_CFGR_SW;
		RCC->CFGR |= RCC_CFGR_SW_PLL;

		/* Wait till PLL is used as system clock source */
		while ((RCC->CFGR & RCC_CFGR_SWS) != RCC_CFGR_SWS_PLL) { }
		SystemCoreClock	= SYSCLK_FREQ_24MHz;
	}
	else
	{
		/*	If HSE fails to start-up, the application will have wrong clock
			configuration. User can add here some code to deal with this error
		*/
		ShowFatalError(ERROR_MCU_HARD);
	}
}

#elif defined SYSCLK_FREQ_36MHz
/**
  * @brief  Sets System clock frequency to 36MHz and configure HCLK, PCLK2 
  *          and PCLK1 prescalers. 
  * @note   This function should be used only after reset.
  * @param  None
  * @retval None
  */
	#warning SYSCLK_FREQ_36MHz
void SetSysClockTo36(void)
{
	__IO uint32_t StartUpCounter = HSE_STARTUP_TIMEOUT;
  
	/* SYSCLK, HCLK, PCLK2 and PCLK1 configuration ---------------------------*/    
	/* Enable HSE */    
	RCC->CR |= RCC_CR_HSEON;
	
	/* Wait till HSE is ready and if Time out is reached exit */
	while (((RCC->CR & RCC_CR_HSERDY) == RESET) && (StartUpCounter-- != 0));
	if ((RCC->CR & RCC_CR_HSERDY) != RESET)
	{
		/* Enable Prefetch Buffer */
		FLASH->ACR |= FLASH_ACR_PRFTBE;

		/* Flash 1 wait state */
		FLASH->ACR &= (uint32_t)((uint32_t)~FLASH_ACR_LATENCY);
		FLASH->ACR |= (uint32_t)FLASH_ACR_LATENCY_1;
 
		/* HCLK = SYSCLK */
		RCC->CFGR |= (uint32_t)RCC_CFGR_HPRE_DIV1;
      
		/* PCLK2 = HCLK */
		RCC->CFGR |= (uint32_t)RCC_CFGR_PPRE2_DIV1;
    
		/* PCLK1 = HCLK */
		RCC->CFGR |= (uint32_t)RCC_CFGR_PPRE1_DIV1;
    
		/*  PLL configuration: PLLCLK = (HSE / 2) * 9 = 36 MHz */
		RCC->CFGR &= (uint32_t)((uint32_t)~(RCC_CFGR_PLLSRC | RCC_CFGR_PLLXTPRE | RCC_CFGR_PLLMULL));
		RCC->CFGR |= (uint32_t)(RCC_CFGR_PLLSRC_HSE | RCC_CFGR_PLLXTPRE_HSE_Div2 | RCC_CFGR_PLLMULL9);

		/* Enable PLL */
		RCC->CR |= RCC_CR_PLLON;

		/* Wait till PLL is ready */
		while((RCC->CR & RCC_CR_PLLRDY) == 0)
		{
		}

		/* Select PLL as system clock source */
		RCC->CFGR &= (uint32_t)((uint32_t)~(RCC_CFGR_SW));
		RCC->CFGR |= (uint32_t)RCC_CFGR_SW_PLL;    

		/* Wait till PLL is used as system clock source */
		while ((RCC->CFGR & (uint32_t)RCC_CFGR_SWS) != (uint32_t)0x08) { }
		SystemCoreClock	= SYSCLK_FREQ_36MHz;
	}
	else
	{	/*	If HSE fails to start-up, the application will have wrong clock
			configuration. User can add here some code to deal with this error
		*/
		ShowFatalError(ERROR_MCU_HARD);
	}
}
#elif defined SYSCLK_FREQ_48MHz
/**
  * @brief  Sets System clock frequency to 48MHz and configure HCLK, PCLK2 
  *          and PCLK1 prescalers. 
  * @note   This function should be used only after reset.
  * @param  None
  * @retval None
  */
#error SYSCLK_FREQ_48MHz
void SetSysClockTo48(void)
{
	__IO uint32_t StartUpCounter = 0;
  
	/* SYSCLK, HCLK, PCLK2 and PCLK1 configuration ---------------------------*/    
	/* Enable HSE */    
	RCC->CR |= ((uint32_t)RCC_CR_HSEON);
 
	/* Wait till HSE is ready and if Time out is reached exit */
	do
	{
		StartUpCounter++;  
	} while((RCC->CR & RCC_CR_HSERDY == 0) && (StartUpCounter != HSE_STARTUP_TIMEOUT));

	if ((RCC->CR & RCC_CR_HSERDY) != RESET)
	{
		StartUpCounter = 1;
	}
	else
	{
		StartUpCounter = 0;
	}  

	if (HSEStartUpCounterStatus != 0)
	{
		/* Enable Prefetch Buffer */
		FLASH->ACR |= FLASH_ACR_PRFTBE;

		/* Flash 1 wait state */
		FLASH->ACR &= (uint32_t)((uint32_t)~FLASH_ACR_LATENCY);
		FLASH->ACR |= (uint32_t)FLASH_ACR_LATENCY_1;    
 
		/* HCLK = SYSCLK */
		RCC->CFGR |= (uint32_t)RCC_CFGR_HPRE_DIV1;
      
		/* PCLK2 = HCLK */
		RCC->CFGR |= (uint32_t)RCC_CFGR_PPRE2_DIV1;
    
		/* PCLK1 = HCLK */
		RCC->CFGR |= (uint32_t)RCC_CFGR_PPRE1_DIV2;
    
		/*  PLL configuration: PLLCLK = HSE * 6 = 48 MHz */
		RCC->CFGR &= (uint32_t)((uint32_t)~(RCC_CFGR_PLLSRC | RCC_CFGR_PLLXTPRE | RCC_CFGR_PLLMULL));
		RCC->CFGR |= (uint32_t)(RCC_CFGR_PLLSRC_HSE | RCC_CFGR_PLLMULL6);

		/* Enable PLL */
		RCC->CR |= RCC_CR_PLLON;

		/* Wait till PLL is ready */
		while((RCC->CR & RCC_CR_PLLRDY) == 0)
		{ }

		/* Select PLL as system clock source */
		RCC->CFGR &= (uint32_t)((uint32_t)~(RCC_CFGR_SW));
		RCC->CFGR |= (uint32_t)RCC_CFGR_SW_PLL;    

		/* Wait till PLL is used as system clock source */
		while ((RCC->CFGR & (uint32_t)RCC_CFGR_SWS) != (uint32_t)0x08) { }
		SystemCoreClock	= SYSCLK_FREQ_48MHz;
	}
	else
	{	/*	If HSE fails to start-up, the application will have wrong clock 
			configuration. User can add here some code to deal with this error */
	}
}

#elif defined SYSCLK_FREQ_56MHz
/**
  * @brief  Sets System clock frequency to 56MHz and configure HCLK, PCLK2 
  *          and PCLK1 prescalers. 
  * @note   This function should be used only after reset.
  * @param  None
  * @retval None
  */
#error SYSCLK_FREQ_56MHz
void SetSysClockTo56(void)
{
	__IO uint32_t StartUpCounter = HSE_STARTUP_TIMEOUT;
  
	/* SYSCLK, HCLK, PCLK2 and PCLK1 configuration ---------------------------*/    
	/* Enable HSE */    
	RCC->CR |= RCC_CR_HSEON;
	
	/* Wait till HSE is ready and if Time out is reached exit */
	while (((RCC->CR & RCC_CR_HSERDY) == RESET) && (StartUpCounter-- != 0));
	if ((RCC->CR & RCC_CR_HSERDY) != RESET)
	{
		/* Enable Prefetch Buffer */
		FLASH->ACR |= FLASH_ACR_PRFTBE;

		/* Flash 2 wait state */
		FLASH->ACR &= (uint32_t)((uint32_t)~FLASH_ACR_LATENCY);
		FLASH->ACR |= (uint32_t)FLASH_ACR_LATENCY_2;    
 
		/* HCLK = SYSCLK */
		RCC->CFGR |= (uint32_t)RCC_CFGR_HPRE_DIV1;
      
		/* PCLK2 = HCLK */
		RCC->CFGR |= (uint32_t)RCC_CFGR_PPRE2_DIV1;
    
		/* PCLK1 = HCLK */
		RCC->CFGR |= (uint32_t)RCC_CFGR_PPRE1_DIV2;

		/* PLL configuration: PLLCLK = HSE * 7 = 56 MHz */
		RCC->CFGR &= (uint32_t)((uint32_t)~(RCC_CFGR_PLLSRC | RCC_CFGR_PLLXTPRE | RCC_CFGR_PLLMULL));
		RCC->CFGR |= (uint32_t)(RCC_CFGR_PLLSRC_HSE | RCC_CFGR_PLLMULL7);

		/* Enable PLL */
		RCC->CR |= RCC_CR_PLLON;

		/* Wait till PLL is ready */
		while((RCC->CR & RCC_CR_PLLRDY) == 0);

		/* Select PLL as system clock source */
		RCC->CFGR &= (uint32_t)((uint32_t)~(RCC_CFGR_SW));
		RCC->CFGR |= (uint32_t)RCC_CFGR_SW_PLL;    

		/* Wait till PLL is used as system clock source */
		while ((RCC->CFGR & (uint32_t)RCC_CFGR_SWS) != (uint32_t)0x08) { }
		SystemCoreClock	= SYSCLK_FREQ_56MHz;
	}
	else
	{	/*	If HSE fails to start-up, the application will have wrong clock 
			configuration. User can add here some code to deal with this error
		*/
	} 
}

#elif defined SYSCLK_FREQ_72MHz
/**
  * @brief  Sets System clock frequency to 72MHz and configure HCLK, PCLK2 
  *          and PCLK1 prescalers. 
  * @note   This function should be used only after reset.
  * @param  None
  * @retval None
  */
#warning SYSCLK_FREQ_72MHz
void SetSysClockTo72(void)
{
	__IO uint32_t StartUpCounter = HSE_STARTUP_TIMEOUT;
  
	/* SYSCLK, HCLK, PCLK2 and PCLK1 configuration ---------------------------*/    
	/* Enable HSE */    
	RCC->CR |= RCC_CR_HSEON;
	
	/* Wait till HSE is ready and if Time out is reached exit */
	while (((RCC->CR & RCC_CR_HSERDY) == RESET) && (StartUpCounter-- != 0));
	if ((RCC->CR & RCC_CR_HSERDY) != RESET)
	{
		/* Enable Prefetch Buffer */
		FLASH->ACR |= FLASH_ACR_PRFTBE;

		/* Flash 2 wait state */
		FLASH->ACR &= (uint32_t)((uint32_t)~FLASH_ACR_LATENCY);
		FLASH->ACR |= (uint32_t)FLASH_ACR_LATENCY_2;    

 
		/* HCLK = SYSCLK */
		RCC->CFGR |= (uint32_t)RCC_CFGR_HPRE_DIV1;
      
		/* PCLK2 = HCLK */
		RCC->CFGR |= (uint32_t)RCC_CFGR_PPRE2_DIV1;
    
		/* PCLK1 = HCLK */
		RCC->CFGR |= (uint32_t)RCC_CFGR_PPRE1_DIV2;

		/*  PLL configuration: PLLCLK = HSE * 9 = 72 MHz */
		RCC->CFGR &= (uint32_t)((uint32_t)~(RCC_CFGR_PLLSRC | RCC_CFGR_PLLXTPRE | RCC_CFGR_PLLMULL));
		RCC->CFGR |= (uint32_t)(RCC_CFGR_PLLSRC_HSE | RCC_CFGR_PLLMULL9);

		/* Enable PLL */
		RCC->CR |= RCC_CR_PLLON;

		/* Wait till PLL is ready */
		while((RCC->CR & RCC_CR_PLLRDY) == 0) { }
    
		/* Select PLL as system clock source */
		RCC->CFGR &= (uint32_t)((uint32_t)~(RCC_CFGR_SW));
		RCC->CFGR |= (uint32_t)RCC_CFGR_SW_PLL;    

		/* Wait till PLL is used as system clock source */
		while ((RCC->CFGR & (uint32_t)RCC_CFGR_SWS) != (uint32_t)0x08) { }
		SystemCoreClock	= SYSCLK_FREQ_72MHz;
	}
	else
	{
		/*	If HSE fails to start-up, the application will have wrong clock 
			configuration. User can add here some code to deal with this error
		*/
		ShowFatalError(ERROR_MCU_HARD);
	}
}
#endif

/**
  * @}
  */

/**
  * @}
  */
  
/**
  * @}
  */    
/******************* (C) COPYRIGHT 2010 STMicroelectronics *****END OF FILE****/
