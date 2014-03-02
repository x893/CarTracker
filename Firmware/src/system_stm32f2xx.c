#include "stm32f2xx.h"

// Uncomment the following line if you need to relocate your vector Table in Internal SRAM.
/* #define VECT_TAB_SRAM */
#define VECT_TAB_OFFSET  0x00	/*	Vector Table base offset field. This value must be a multiple of 0x200. */

// <h> PLL Configuration
//	<o0> PLL_M <0-25>
//		<i> PLL_VCO = (HSE_VALUE or HSI_VALUE / PLL_M) * PLL_N
//	<o1> PLL_N <0-240>
//		<i> PLL_VCO = (HSE_VALUE or HSI_VALUE / PLL_M) * PLL_N
//	<o2> PLL_P
//		<i> SYSCLK = PLL_VCO / PLL_P */
//	<o3> PLL_Q
//		<i> USB OTG FS, SDIO and RNG Clock =  PLL_VCO / PLLQ */
// </h>
#define PLL_M      8
#define PLL_N      240
#define PLL_P      2
#define PLL_Q      5

uint32_t SystemCoreClock = (((HSE_VALUE / PLL_M) * PLL_N) / PLL_P);

const uint8_t AHBPrescTable[16] = {0, 0, 0, 0, 0, 0, 0, 0, 1, 2, 3, 4, 6, 7, 8, 9};

static void SetSysClock(void);

#ifdef DATA_IN_ExtSRAM
	static void SystemInit_ExtMemCtl(void);
#endif /* DATA_IN_ExtSRAM */

/**
  * @brief  Setup the microcontroller system
  *         Initialize the Embedded Flash Interface, the PLL and update the 
  *         SystemFrequency variable.
  * @param  None
  * @retval None
  */
void SystemInit(void)
{
	/* Reset the RCC clock configuration to the default reset state ------------*/
	/* Set HSION bit */
	RCC->CR |= RCC_CR_HSION;

	/* Reset CFGR register */
	RCC->CFGR = 0;

	/* Reset HSEON, CSSON and PLLON bits */
	RCC->CR &= ~(RCC_CR_PLLON | RCC_CR_CSSON | RCC_CR_HSEON);

	/* Reset PLLCFGR register */
	RCC->PLLCFGR = 0x24003010;

	/* Reset HSEBYP bit */
	RCC->CR &= ~RCC_CR_HSEBYP;

	/* Disable all interrupts */
	RCC->CIR = 0x00000000;

	/*	Configure the System clock source, PLL Multiplier and Divider factors, 
		AHB/APBx prescalers and Flash settings
	*/
	SetSysClock();

	/* Configure the Vector Table location add offset address */
#ifdef VECT_TAB_SRAM
	SCB->VTOR = SRAM_BASE | VECT_TAB_OFFSET; /* Vector Table Relocation in Internal SRAM */
#else
	SCB->VTOR = FLASH_BASE | VECT_TAB_OFFSET; /* Vector Table Relocation in Internal FLASH */
#endif
}

/**
  * @brief  Update SystemCoreClock variable according to Clock Register Values.
  *         The SystemCoreClock variable contains the core clock (HCLK), it can
  *         be used by the user application to setup the SysTick timer or configure
  *         other parameters.
  *           
  * @note   Each time the core clock (HCLK) changes, this function must be called
  *         to update SystemCoreClock variable value. Otherwise, any configuration
  *         based on this variable will be incorrect.         
  *     
  * @note   - The system frequency computed by this function is not the real 
  *           frequency in the chip. It is calculated based on the predefined 
  *           constant and the selected clock source:
  *             
  *           - If SYSCLK source is HSI, SystemCoreClock will contain the HSI_VALUE(*)
  *                                              
  *           - If SYSCLK source is HSE, SystemCoreClock will contain the HSE_VALUE(**)
  *                          
  *           - If SYSCLK source is PLL, SystemCoreClock will contain the HSE_VALUE(**) 
  *             or HSI_VALUE(*) multiplied/divided by the PLL factors.
  *         
  *         (*) HSI_VALUE is a constant defined in stm32f2xx.h file (default value
  *             16 MHz) but the real value may vary depending on the variations
  *             in voltage and temperature.   
  *    
  *         (**) HSE_VALUE is a constant defined in stm32f2xx.h file (default value
  *              25 MHz), user has to ensure that HSE_VALUE is same as the real
  *              frequency of the crystal used. Otherwise, this function may
  *              have wrong result.
  *                
  *         - The result of this function could be not correct when using fractional
  *           value for HSE crystal.
  *     
  * @param  None
  * @retval None
  */
void SystemCoreClockUpdate(void)
{
	uint32_t tmp, pllvco = 0, pllp = 2, pllsource = 0, pllm = 2;
  
	/* Get SYSCLK source -------------------------------------------------------*/
	tmp = RCC->CFGR & RCC_CFGR_SWS;

	switch (tmp)
	{
	case 0x00:  /* HSI used as system clock source */
		SystemCoreClock = HSI_VALUE;
		break;
	case 0x04:  /* HSE used as system clock source */
		SystemCoreClock = HSE_VALUE;
		break;
	case 0x08:  /* PLL used as system clock source */
		/*
			PLL_VCO = (HSE_VALUE or HSI_VALUE / PLL_M) * PLL_N
			SYSCLK = PLL_VCO / PLL_P
		*/
		pllsource = (RCC->PLLCFGR & RCC_PLLCFGR_PLLSRC) >> 22;
		pllm = RCC->PLLCFGR & RCC_PLLCFGR_PLLM;

		pllvco = ((pllsource != 0 ? HSE_VALUE : HSI_VALUE) / pllm) * ((RCC->PLLCFGR & RCC_PLLCFGR_PLLN) >> 6);
		pllp = (((RCC->PLLCFGR & RCC_PLLCFGR_PLLP) >>16) + 1 ) *2;
		SystemCoreClock = pllvco/pllp;
		break;
	default:
		SystemCoreClock = HSI_VALUE;
		break;
	}
	/* Compute HCLK frequency --------------------------------------------------*/
	/* Get HCLK prescaler */
	tmp = AHBPrescTable[((RCC->CFGR & RCC_CFGR_HPRE) >> 4)];
	/* HCLK frequency */
	SystemCoreClock >>= tmp;
}

/**
  * @brief  Configures the System clock source, PLL Multiplier and Divider factors, 
  *         AHB/APBx prescalers and Flash settings
  * @Note   This function should be called only once the RCC clock configuration  
  *         is reset to the default reset state (done in SystemInit() function).
  * @param  None
  * @retval None
  */
void ShowFatalError(uint16_t errorCode);
static void SetSysClock(void)
{
/******************************************************************************/
/*            PLL (clocked by HSE) used as System clock source                */
/******************************************************************************/
	__IO uint32_t StartUpCounter = 0;

	/* Enable HSE */
	RCC->CR |= ((uint32_t)RCC_CR_HSEON);
 
	/* Wait till HSE is ready and if Time out is reached exit */
	do
	{
		StartUpCounter++;
	} while(((RCC->CR & RCC_CR_HSERDY) == RESET) && (StartUpCounter != HSE_STARTUP_TIMEOUT));

	if ((RCC->CR & RCC_CR_HSERDY) == RESET)
	{	/*	If HSE fails to start-up, the application will have wrong clock
			configuration. User can add here some code to deal with this error
		*/
		ShowFatalError(ERROR_MCU_HARD);
	}
	else
	{
		/* HCLK = SYSCLK / 1*/
		RCC->CFGR |= RCC_CFGR_HPRE_DIV1;
      
		/* PCLK2 = HCLK / 2*/
		RCC->CFGR |= RCC_CFGR_PPRE2_DIV2;
    
		/* PCLK1 = HCLK / 4*/
		RCC->CFGR |= RCC_CFGR_PPRE1_DIV4;

		/* Configure the main PLL */
		RCC->PLLCFGR = PLL_M						|
						(PLL_N << 6)				|
						(((PLL_P >> 1) - 1) << 16)	|
						(RCC_PLLCFGR_PLLSRC_HSE)	|
						(PLL_Q << 24);

		/* Enable the main PLL */
		RCC->CR |= RCC_CR_PLLON;

		/* Wait till the main PLL is ready */
		while((RCC->CR & RCC_CR_PLLRDY) == 0) { }
   
		/* Configure Flash prefetch, Instruction cache, Data cache and wait state */
		FLASH->ACR = FLASH_ACR_PRFTEN | FLASH_ACR_ICEN | FLASH_ACR_DCEN | FLASH_ACR_LATENCY_3WS;

		/* Select the main PLL as system clock source */
		RCC->CFGR &= (uint32_t)((uint32_t)~(RCC_CFGR_SW));
		RCC->CFGR |= RCC_CFGR_SW_PLL;

		/* Wait till the main PLL is used as system clock source */
		while ((RCC->CFGR & (uint32_t)RCC_CFGR_SWS ) != RCC_CFGR_SWS_PLL) { }
	}
}
