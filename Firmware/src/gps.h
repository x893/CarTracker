#ifndef __GPS_H__
#define __GPS_H__

#include <stdint.h>
#include "hardware.h"
#include "gsm.h"
#include "ring_buffer.h"
#include "FreeRTOS.h"
#include "semphr.h"

struct GpsContext_s
{
			const char *WaitAliveText;
const struct GpsCmds_s *GpsCommand;
			uint32_t	SendInterval;
			uint32_t	SendTimer;
			uint32_t	NoDataTimer;
			uint32_t	LastFilterTime;
			uint16_t	RxIndex;
			uint16_t	WaitAliveSize;
			uint32_t	AverageTime;
			uint8_t		AverageIndex;
			uint8_t		AverageCount;
			uint8_t		AverageMaxCount;

	volatile uint8_t	RxMode;
			char		Data;
		RingBuffer_t	RB_Tx;

			GsmTask_t	GsmTask;
			nmeaGPRMC	RMC;
};

struct GpsCmds_s {
	uint8_t (*Init)(struct GpsContext_s * gc);
};

typedef struct GpsCmds_s GpsCommands_t;
typedef struct GpsContext_s GpsContext_t;

extern GpsContext_t GpsContext;

void vGpsTask(void * pvArg);
void GpsPut(uint8_t data);
void GpsPutLine(const char * text);
void GpsPutLineCS(const char * text);
char GpsGet(void);

#endif
