#ifndef __FW_UPDATE_H__
#define __FW_UPDATE_H__

#include <stdbool.h>
#include "hardware.h"

#define FW_SIGNATURE	0x19630605

typedef struct BL_INFO_s {
	uint32_t		Signature;
	uint32_t		FwCRC32;
	uint32_t		FwSize;
} BL_INFO_t;

bool FwCheckBlank(uint32_t * address);
bool FwFlashErase(uint32_t address);
bool FwCheckImage(uint32_t base);

#ifdef BL_VERSION
	void FwCopyNew(void);
#endif
	uint8_t FwWriteBlock(uint32_t dst, uint8_t *src, int bytes);
#endif
