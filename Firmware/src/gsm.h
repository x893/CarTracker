#ifndef __GSM_H__
#define __GSM_H__

#include <stdint.h>
#include "nmea/nmea.h"
#include "can.h"
#include "gpsOne.h"

#define GSM_TASK_QUEUE_SIZE	12

#define GSM_TASK_CMD_NOP	0
#define GSM_TASK_CMD_GPS	1
#define GSM_TASK_CMD_PING	2
#define GSM_TASK_CMD_RET	3
#define GSM_TASK_CMD_ERR	4
#define GSM_TASK_CMD_SMS	5

struct GsmParams_s {
	const GgCommand_t	* GG_Command;
				int		SmsIndex;
};

typedef struct GsmParams_s GsmParams_t;

struct GsmTask_s {
	
#if (USE_CAN_SUPPORT)
		double CanData[CAN_VALUES_SIZE];
#endif
		nmeaGPRMC	GPRMC;
const GgCommand_t *	GG_Command;
		int			SmsIndex;
		uint16_t	SenseA1;
		uint8_t		SenseD1;
		uint8_t		Command;
		uint8_t		Result;
};

typedef struct GsmTask_s GsmTask_t;

uint8_t GsmAddTaskAlways(GsmTask_t * cmd);
uint8_t GsmAddTask(GsmTask_t * cmd);
char * GsmIMEI(void);
void vGsmTask(void * arg);

#endif
