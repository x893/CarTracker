#ifndef __CAN_H__
#define __CAN_H__

#include <stdint.h>

#if (USE_CAN_SUPPORT)

#define CAN_VALUES_SIZE		7

enum CAN_SPEED {
	CAN_SPEED_100,
	CAN_SPEED_125,
	CAN_SPEED_250,
	CAN_SPEED_500,
	CAN_SPEED_1000
};

#define PID_REQUEST			0x7DF
#define PID_REPLY			0x7E8

#define	 PID_MODE_01		0x01
	#define PID_01_SUPPORTED	0x00
	#define OBD_DTC				0x01
	#define OBD_ENGINE_COOLANT	0x05
	#define OBD_ENGINE_RPM		0x0C
	#define OBD_VEHICLE_SPEED	0x0D
	#define OBD_MAF_SENSOR		0x10
	#define OBD_THROTTLE		0x11
	#define OBD_O2_VOLTAGE		0x14
	#define OBD_FUELLEVEL		0x2F

#define	 PID_MODE_09		0x09
	#define PID_09_SUPPORTED	0x00
	#define OBD_VIN_COUNT		0x01
	#define OBD_VIN_DATA		0x02
/*
#define STFT				0x06
#define LTFT				0x07
#define CYL_ADVANCE			0x0E
#define IAT					0x0F
#define RUNTIME				0x1F
#define FUEL_PRESS			0x23
#define INTAKE_PRESS		0x33
#define WB_AFR				0x34
#define CAT_TEMP			0x3C
#define CONTROL_MODULE		0x42
#define LOAD				0x43
#define OL_AFR				0x44
#define AMBIENT_TEMP		0x46
*/

void vCanTask(void *pvArg);
double CanSumGet(int index);
void  CanSumInit(void);
extern uint8_t CanEnable;
extern uint8_t CanDisabled;

#endif
#endif
