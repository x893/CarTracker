#ifndef __DEBUG_H__
#define __DEBUG_H__

#include "hardware.h"

#if ( USE_SERIAL_DEBUG )

	#define DEBUG_RX_QUEUE_SIZE	0
	void DebugInit(void);
	uint8_t DebugWaitConnect(void);
	uint8_t IsDebugAvailable(void);
	char DebugGet(void);
	void DebugPut(char data);
	uint8_t DebugFlush(void);
	
	void DebugPutLine(const char *msg);
	#define DEBUG(m)		DebugPutLine(m)

	void DebugPutLineln(const char *msg);
	#define DEBUG_LINE(m)	DebugPutLineln(m)

#elif ( USE_USB_DEBUG )

	uint8_t IsDebugAvailable(void);
	uint8_t DebugWaitConnect(void);
	char DebugGet(void);
	uint8_t DebugPut(char data);
	void DebugPutLine(const char *msg);
	void DebugPutLineln(const char *msg);

#else

	#define DebugInit()
	#define DebugPutLine(m)
	#define DebugPutLineln(m)
	#define DebugPut(c)
	#define IsDebugAvailable()
	#define DebugWaitConnect()
	#define DebugFlush()		0

#endif

#endif	// __DEBUG_H__
