#include "hardware.h"
#include "fw_update.h"
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "gsm.h"
#include "gpsOne.h"
#include "utility.h"
#include <nmea/tok.h>
#include <string.h>
#include "debug.h"

#if !defined( STM32F2XX )		&& \
	!defined( STM32F4XX )		&& \
	!defined( STM32F10X_HD )	&& \
	!defined( STM32F10X_MD )	&& \
	!defined( STM32F10X_MD_VL )
	#error "CPU type not define"
#endif

/*******************************************************************************
* Function Name	:	FwCheckBlank
* Description	:	Check block for blank
*******************************************************************************/
bool FwCheckBlank(uint32_t * address)
{
	while(*address++ == 0xFFFFFFFF)
	{
		if (((uint32_t)address & (uint32_t)(FLASH_PAGE_SIZE - 1)) == 0)
			return true;
	}
	return false;
}

/*******************************************************************************
* Function Name	:	FwFlashErase
* Description	:	Erase flash page
*******************************************************************************/
bool FwFlashErase(uint32_t address)
{
	FLASH_Status status;

#if defined( STM32F2XX ) || defined( STM32F4XX )
	uint32_t sector = FLASH_Sector_0;

	FLASH_Unlock();

	if (address < 16 * 1024)
		sector = FLASH_Sector_0;
	else if (address < 32 * 1024)
		sector = FLASH_Sector_1;
	else if (address < 48 * 1024)
		sector = FLASH_Sector_2;
	else if (address < 64 * 1024)
		sector = FLASH_Sector_3;
	else if (address < 128 * 1024)
		sector = FLASH_Sector_4;
	else if (address < 256 * 1024)
		sector = FLASH_Sector_5;
	else if (address < 384 * 1024)
		sector = FLASH_Sector_6;
	else if (address < 512 * 1024)
		sector = FLASH_Sector_7;
	else if (address < 640 * 1024)
		sector = FLASH_Sector_8;
	else if (address < 768 * 1024)
		sector = FLASH_Sector_9;
	else if (address < 896 * 1024)
		sector = FLASH_Sector_10;
	else
		sector = FLASH_Sector_11;

	status = FLASH_EraseSector(sector, VoltageRange_3);

#elif defined( STM32F10X_HD ) || defined( STM32F10X_MD ) || defined( STM32F10X_MD_VL )

	vTaskDelay(2);
	FLASH_Unlock();
	vTaskDelay(2);
	status = FLASH_ErasePage(address);
	vTaskDelay(2);
	FLASH_Lock();

#else
	#error "CPU not define"
#endif

	if (status != FLASH_COMPLETE)
	{
		ShowError(ERROR_FLASH_ERASE);
		return false;
	}
	return true;
}

/*******************************************************************************
* Function Name	:	FwGetBlInfo
* Description	:
* Parameters	:
*******************************************************************************/
extern uint32_t FirmwareMagic;
extern uint32_t __Vectors;

BL_INFO_t * FwGetBlInfo(uint32_t base)
{
	return (BL_INFO_t *)(base + ((uint32_t)(&FirmwareMagic) - ((uint32_t)(&__Vectors))));
}

/*******************************************************************************
* Function Name	:	FwCheckImage
* Description	:
* Parameters	:
*******************************************************************************/
bool FwCheckImage(uint32_t base)
{
	BL_INFO_t *bl_info = FwGetBlInfo(base);
	
	if (bl_info->Signature == FW_SIGNATURE
	&&	((*(volatile uint32_t*)base) & 0x2FFE0000) == 0x20000000
		)
	{
		if (bl_info->FwSize == 0 && bl_info->FwCRC32 == 0xAA55BB66)
			return true;
		if (bl_info->FwSize <= MAX_FW_SIZE
		&&	GetCRC32((uint32_t *)base, (bl_info->FwSize + 3) / 4, &bl_info->FwCRC32) == bl_info->FwCRC32
			)
		return true;
	}
	return false;
}

#ifdef BL_VERSION
/*******************************************************************************
* Function Name	:	FwCopy
* Description	:
* Parameters	:
*******************************************************************************/
void FwCopyNew(void)
{
	uint32_t nbytes;
	int16_t nlen;
	uint8_t *fw_src;
	uint8_t *fw_dst;
	uint8_t result = E_OK;

	if (FwCheckImage(FW_COPY_START))
	{	// New version valid, clear block pointer
		DEBUG(" new");

		FwWriteBlock(NULL, NULL, 0);
		fw_src = (uint8_t *)FW_COPY_START;
		fw_dst = (uint8_t *)FW_WORK_START;
		nbytes = FwGetBlInfo(FW_COPY_START)->FwSize;

		while (nbytes != 0)
		{
			nlen = (nbytes >= FLASH_PAGE_SIZE) ? FLASH_PAGE_SIZE : nbytes;
			if (memcmp(fw_dst, fw_src, nlen) != 0)
			{
				vTaskDelay(1);
				if ((result = FwWriteBlock((uint32_t)fw_dst, fw_src, nlen)) != E_OK
				||	memcmp(fw_dst, fw_src, nlen) != 0)
					break;
			}
			nbytes -= nlen;
			fw_src += nlen;
			fw_dst += nlen;
		}

		if (result == E_OK && FwCheckImage(FW_WORK_START))
		{	// Work version normal, clear copy
			FwFlashErase(FW_COPY_START);
			DEBUG(" update");
		}
		else
		{
			DEBUG(" failed");
		}
	}
}
#endif

/*******************************************************************************
* Function Name	:	FwWriteBlock
* Description	:
* Parameters	:
*******************************************************************************/
uint8_t FwWriteBlock(uint32_t dst, uint8_t *src, int bytes)
{
	static uint32_t fw_block;
	uint8_t result = E_OK;
	uint16_t data;

	if (bytes == 0)
	{
		fw_block = 0;
	}
	else if ((dst + bytes) > FLASH_TOP)
	{	// No room for data
		result = E_FW_ERROR;
	}
	else
	{
		if (fw_block != (dst & FLASH_BLOCK_MASK))
		{	// Block change, check for blank and erase
			fw_block = dst & FLASH_BLOCK_MASK;
			if (!FwCheckBlank((uint32_t *)fw_block)
			&&	!FwFlashErase(fw_block)
				)
				result = E_FW_ERROR;	// Can't erase flash
		}

		if (result == E_OK)
		{
			FLASH_Unlock();
			while (bytes != 0)
			{
				if (bytes >= 2)
				{
					data = *(uint16_t *)src;
					src++;
					bytes--;
				}
				else
				{
					data = *src;
				}
				src++;
				bytes--;

				if (*(uint16_t*)dst != data)
				{
					if (FLASH_ProgramHalfWord(dst, data) != FLASH_COMPLETE)
					{
						bytes++;
						break;
					}
				}
				dst += 2;
			}
			FLASH_Lock();
			if (bytes != 0)
				result = E_FW_ERROR;
		}
	}
	return result;
}
