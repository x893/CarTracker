#ifndef __MAIN_H__
#define __MAIN_H__

#include <stdint.h>

void ShowError			(uint8_t errorCode);
void ShowFatalError		(uint8_t errorCode) __attribute__((noreturn));
void ShowFatalErrorOnly	(uint8_t errorCode);
void SystemRestart		(void) __attribute__((noreturn));

extern char SET_GRPS_APN_NAME[];
extern char SET_GPRS_USERNAME[];
extern char SET_GPRS_PASSWORD[];
extern char SET_HOST_ADDR[];
extern char SET_HOST_PORT[];

#define VBAT_ONLY_OFF	500
#define VBAT_SEND_SMS	800


#define GSM_COMPLETE	0x001
#define GPS_COMPLETE	0x002
#define GPS_STOP		0x004
#define SMS_INIT_OK		0x008
#define SMS_INIT_SENT	0x010
#define VBAT_LOW		0x020
#define VBAT_STOPPED	0x040
#define VBAT_READY		0x080

extern volatile uint16_t GlobalStatus;

uint8_t  GsmIsRegister(void);
void	 Reset_WWDG(void);
void	 Enable_WWDG(void);
void	 Disable_WWDG(void);

void	 DWT_Init(void);
void	 DWT_Delay_ms(uint16_t ms);
void	 DWT_Delay_us(uint16_t us);
uint32_t DWT_Get(void);

#ifdef DISPLAY_STACK
	void DisplayStackHigh(const char *msg);
#endif

#ifdef BL_VERSION

	void ShowFatalErrorOnly(uint8_t errorCode);

	#define vTaskDelay(ms)		DWT_Delay_ms(ms)
	#define ShowError(error)	\
		do {					\
			ShowFatalErrorOnly(error);	\
			__enable_irq();				\
		} while(0)

#endif

#endif
