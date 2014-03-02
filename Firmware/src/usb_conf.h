#ifndef __USB_CONF__H__
#define __USB_CONF__H__

#if   defined( STM32F2XX )
	#include "stm32f2xx.h"
#elif defined( STM32F4XX )
	#include "stm32f4xx.h"
#elif defined( STM32F10X_HD ) || defined( STM32F10X_MD )
	#include "stm32f10x.h"
#else
	#error "CPU type not define"
#endif

/* USB Core and PHY interface configuration.
   Tip: To avoid modifying these defines each time you need to change the USB
        configuration, you can declare the needed define in your toolchain
        compiler preprocessor.
   */
/****************** USB OTG FS PHY CONFIGURATION *******************************
*  The USB OTG FS Core supports one on-chip Full Speed PHY.
*  
*  The USE_EMBEDDED_PHY symbol is defined in the project compiler preprocessor 
*  when FS core is used.
*******************************************************************************/
#ifndef USE_USB_OTG_FS
	#define USE_USB_OTG_FS
#endif

#ifdef USE_USB_OTG_FS 
	#define USB_OTG_FS_CORE
#endif

#ifndef USE_EMBEDDED_PHY
	#define USE_EMBEDDED_PHY
#endif

/*******************************************************************************
*                      FIFO Size Configuration in Device mode
*  
*  (i) Receive data FIFO size = RAM for setup packets + 
*                   OUT endpoint control information +
*                   data OUT packets + miscellaneous
*      Space = ONE 32-bits words
*     --> RAM for setup packets = 10 spaces
*        (n is the nbr of CTRL EPs the device core supports) 
*     --> OUT EP CTRL info      = 1 space
*        (one space for status information written to the FIFO along with each 
*        received packet)
*     --> data OUT packets      = (Largest Packet Size / 4) + 1 spaces 
*        (MINIMUM to receive packets)
*     --> OR data OUT packets  = at least 2*(Largest Packet Size / 4) + 1 spaces 
*        (if high-bandwidth EP is enabled or multiple isochronous EPs)
*     --> miscellaneous = 1 space per OUT EP
*        (one space for transfer complete status information also pushed to the 
*        FIFO with each endpoint's last packet)
*
*  (ii)MINIMUM RAM space required for each IN EP Tx FIFO = MAX packet size for 
*       that particular IN EP. More space allocated in the IN EP Tx FIFO results
*       in a better performance on the USB and can hide latencies on the AHB.
*
*  (iii) TXn min size = 16 words. (n  : Transmit FIFO index)
*   (iv) When a TxFIFO is not used, the Configuration should be as follows: 
*       case 1 :  n > m    and Txn is not used    (n,m  : Transmit FIFO indexes)
*       --> Txm can use the space allocated for Txn.
*       case2  :  n < m    and Txn is not used    (n,m  : Transmit FIFO indexes)
*       --> Txn should be configured with the minimum space of 16 words
*  (v) The FIFO is used optimally when used TxFIFOs are allocated in the top 
*       of the FIFO.Ex: use EP1 and EP2 as IN instead of EP1 and EP3 as IN ones.
*   (vi) In HS case 12 FIFO locations should be reserved for internal DMA registers
*        so total FIFO size should be 1012 Only instead of 1024       
*******************************************************************************/
 
#ifdef USB_OTG_FS_CORE
	#define RX_FIFO_FS_SIZE                          128
	#define TX0_FIFO_FS_SIZE                          32
	#define TX1_FIFO_FS_SIZE                         128
	#define TX2_FIFO_FS_SIZE                          32 
	#define TX3_FIFO_FS_SIZE                           0

// #define USB_OTG_FS_LOW_PWR_MGMT_SUPPORT
// #define USB_OTG_FS_SOF_OUTPUT_ENABLED
#endif

// #define VBUS_SENSING_ENABLED

#define USE_DEVICE_MODE

#define __ALIGN_BEGIN
#define __ALIGN_END   

/* __packed keyword used to decrease the data type alignment to 1-byte */
#if defined (__CC_ARM)         /* ARM Compiler */
	#define __packed    __packed
#elif defined (__ICCARM__)     /* IAR Compiler */
	#define __packed    __packed
#elif defined   ( __GNUC__ )   /* GNU Compiler */                        
	#define __packed    __attribute__ ((__packed__))
#elif defined   (__TASKING__)  /* TASKING Compiler */
	#define __packed    __unaligned
#endif /* __CC_ARM */

#endif //__USB_CONF__H__
