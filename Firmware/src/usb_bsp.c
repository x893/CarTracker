#include "hardware.h"

#include "usb_bsp.h"
#include "usbd_conf.h"
#include "FreeRTOS.h"
#include "task.h"

/**
* @brief  USB_OTG_BSP_Init
*         Initilizes BSP configurations
* @param  None
* @retval None
*/
#if ( USE_USB )
void USB_OTG_BSP_Init(USB_OTG_CORE_HANDLE *pdev)
{
#if defined( STM32F2XX ) || defined( STM32F4XX )
	/* Configure DM DP Pins */
	USB_DM_INIT();
	USB_DP_INIT();

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_SYSCFG, ENABLE);
	RCC_AHB2PeriphClockCmd(RCC_AHB2Periph_OTG_FS, ENABLE);
#else
	#error "CPU not define"
#endif
}
#endif

/**
* @brief  USB_OTG_BSP_EnableInterrupt
*         Enabele USB Global int`errupt
* @param  None
* @retval None
*/
void USB_OTG_BSP_EnableInterrupt(USB_OTG_CORE_HANDLE *pdev)
{
	NVIC_InitTypeDef NVIC_InitStructure; 

	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_1);
	NVIC_InitStructure.NVIC_IRQChannel = OTG_FS_IRQn;

	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 3;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);
}

/**
* @brief  USB_OTG_BSP_uDelay
*         This function provides delay time in micro sec
* @param  usec : Value of delay required in micro sec
* @retval None
*/
void USB_OTG_BSP_uDelay (const uint32_t usec)
{
  uint32_t count = 0;
  const uint32_t utime = (120 * usec / 7);
  do
  {
    if ( ++count > utime )
    {
      return ;
    }
  }
  while (1);
}

/**
* @brief  USB_OTG_BSP_mDelay
*          This function provides delay time in milli sec
* @param  msec : Value of delay required in milli sec
* @retval None
*/
void USB_OTG_BSP_mDelay (const uint32_t msec)
{
	vTaskDelay(msec);
	//! USB_OTG_BSP_uDelay(msec * 1000);   
}
