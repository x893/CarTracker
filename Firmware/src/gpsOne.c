#include "hardware.h"
#include "gpsOne.h"
#include "utility.h"
#include "main.h"
#include "gsm.h"
#include "gps.h"
#include "fw_update.h"
#include "debug.h"
#include <string.h>
#include <nmea/tok.h>

const char GG_FRCMD_PING[]		= "$FRCMD,%s,_Ping,,vers=%s";
const char GG_FRCMD_INIT_MSG[]	= "$FRCMD,%s,_Init";

const char GG_FRERR_AUTH[]		= DGG_FRERR_AUTH;
const char GG_FRERR_NOTSUP[]	= DGG_FRERR_NOTSUP;
const char GG_FRERR_NOTEXEC[]	= DGG_FRERR_NOTEXEC;

const char GG_FRERR[]			= DGG_FRERR;
const char GG_FRRET[]			= DGG_FRRET;
const char GG_FRCMD[]			= DGG_FRCMD;

char SET_GRPS_APN_NAME[32];
char SET_GPRS_USERNAME[32];
char SET_GPRS_PASSWORD[32];
char SET_HOST_ADDR[32];
char SET_HOST_PORT[8];

uint8_t GGC_GprsSettings	(char *, const GgCommand_t *, bool);
uint8_t GGC_StartTracking	(char *, const GgCommand_t *, bool);
uint8_t GGC_StopTracking	(char *, const GgCommand_t *, bool);
uint8_t GGC_PollPosition	(char *, const GgCommand_t *, bool);
uint8_t GGC_DeviceReset		(char *, const GgCommand_t *, bool);
uint8_t GGC_FwUpdate		(char *, const GgCommand_t *, bool);
uint8_t GGC_InitSMS			(char *, const GgCommand_t *, bool);

uint8_t GGR_Command			(char *, int buf_size, const char *cmd);
uint8_t GGR_General			(char *, int buf_size, bool sms, const GgCommand_t * cmd);
uint8_t GGR_GprsSettings	(char *, int buf_size, bool sms, const GgCommand_t * cmd);
uint8_t GGR_FwUpdate		(char *, int buf_size, bool sms, const GgCommand_t * cmd);

uint8_t GGR_SendMessage(char *, const GgCommand_t *, bool);

GgCommandCounts GprsSetCounts;
GgCommandCounts StartTrackingCounts;
GgCommandCounts FwUpdateCounts;
GgCommandCounts InitSMSCounts;

const char GG_GPRS_SETTINGS[]	= DGG_GPRS_SETTINGS;
const char GG_START_TRACKING[]	= DGG_START_TRACKING;

const GgCommand_t GgCommandMap[] =
{
	{ "_InitSMS",			&InitSMSCounts,			GGC_InitSMS,		NULL				},
	{ GG_GPRS_SETTINGS,		&GprsSetCounts,			GGC_GprsSettings,	GGR_GprsSettings	},
	{ "_FwUpdate",			&FwUpdateCounts,		GGC_FwUpdate,		GGR_FwUpdate		},
	{ GG_START_TRACKING,	&StartTrackingCounts,	GGC_StartTracking,	GGR_General			},
/*
	{ "_StopTracking",		NULL,					GGC_StopTracking,	GGR_General			},
	{ "_PollPosition",		NULL,					GGC_PollPosition,	NULL				},
	{ "_DeviceReset",		NULL,					GGC_DeviceReset	,	GGR_General			},
*/
	{ NULL }
};

const GgCommand_t GgReplayMap[] =
{
	{ "_SendMessage",		NULL,					GGR_SendMessage,	NULL				},
	{ NULL }
};

char GG_FwFileName[32];
int  GG_FwSmsIndex;

void GG_ClearAllCounts(void)
{
	const GgCommand_t * cmd = GgCommandMap;
	while (cmd->Handler != NULL)
	{
		if (cmd->Counts != NULL)
		{
			cmd->Counts->Count = 0;
			cmd->Counts->DateTime = 0;
			cmd->Counts->Index = 0;
		}
		cmd++;
	}
}

/*******************************************************************************
* Function Name	:	GGC_InitSMS
* Description	:
*******************************************************************************/
uint8_t GGC_InitSMS(char * cmd, const GgCommand_t * gg_cmd, bool exec)
{
	if (exec)
		GlobalStatus |= SMS_INIT_OK;
	return E_OK;
}

/*******************************************************************************
* Function Name	:	GGR_SendMessage
* Description	:
*******************************************************************************/
uint8_t GGR_SendMessage(char * cmd, const GgCommand_t * gg_cmd, bool exec)
{
	return E_OK;
}

/*******************************************************************************
* Function Name	:	GGC_FwUpdate
* Description	:
*******************************************************************************/
uint8_t GGC_FwUpdate(char * cmd, const GgCommand_t * gg_cmd, bool exec)
{
	int nsize = TokenSizeComma(cmd);
	if (nsize > 0 && nsize < sizeof(GG_FwFileName) - 1)
	{
		strncpy(GG_FwFileName, cmd, nsize);
		GG_FwFileName[nsize] = 0;
		return E_OK;
	}
	return E_ERROR;
}


/*******************************************************************************
* Function Name	:	GGR_FwUpdate
* Description	:
*******************************************************************************/
uint8_t GGR_FwUpdate(char* buffer, int buf_size, bool sms, const GgCommand_t * cmd)
{
	if (sms)
	{
		if (buffer == NULL && buf_size != 0)
			GG_FwSmsIndex = buf_size;
		return E_ERROR;
	}
	return GGR_Command(buffer, buf_size, cmd->Command);
}

/*******************************************************************************
* Function Name	:	GGC_GprsSettings
* Description	:
*******************************************************************************/
uint8_t GGC_GprsSettings(char * cmd, const GgCommand_t * gg_cmd, bool exec)
{
	int size;
	uint8_t state = 0;
	// apn,username,password,hostname,port
	while (cmd != NULL && state < 99)
	{
		size = TokenSizeComma(cmd);
		switch (state)
		{
			case 0:
				if (size > 0 && size < (sizeof(SET_GRPS_APN_NAME) - 1))
				{
					if (exec)
					{
						strncpy(SET_GRPS_APN_NAME, cmd, size);
						SET_GRPS_APN_NAME[size] = 0;
					}
				}
				else
					state = 99;
				break;
			case 1:
				if (size > 0 && size < (sizeof(SET_GPRS_USERNAME) - 1))
				{
					if (exec)
					{
						strncpy(SET_GPRS_USERNAME, cmd, size);
						SET_GPRS_USERNAME[size] = 0;
					}
				}
				else
					SET_GPRS_USERNAME[0] = 0;
				break;
			case 2:
				if (size > 0 && size < (sizeof(SET_GPRS_PASSWORD) - 1))
				{
					if (exec)
					{
						strncpy(SET_GPRS_PASSWORD, cmd, size);
						SET_GPRS_PASSWORD[size] = 0;
					}
				}
				else
					SET_GPRS_PASSWORD[0] = 0;
				break;
			case 3:
				if (size > 0 && size < (sizeof(SET_HOST_ADDR) - 1))
				{
					if (exec)
					{
						strncpy(SET_HOST_ADDR, cmd, size);
						SET_HOST_ADDR[size] = 0;
					}
				}
				else
					state = 99;
				break;
			case 4:
				if (size > 0 && size < (sizeof(SET_HOST_PORT) - 1))
				{
					if (exec)
					{
						strncpy(SET_HOST_PORT, cmd, size);
						SET_HOST_PORT[size] = 0;
					}
					return E_OK;
				}
				else
					state = 99;
				break;
		}
		cmd = TokenNextComma(cmd);
		state++;
	}
	return E_ERROR;
}

/*******************************************************************************
* Function Name	:	GGC_GprsSettings
* Description	:	Send message to host
*******************************************************************************/
uint8_t GGR_Command(char* buffer, int buf_size, const char *cmd)
{
	if (buffer != NULL && buf_size != 0)
	{
		NmeaAddChecksum( strcpyEx( strcpyEx( strcpyEx( strcpyEx( buffer,
			DGG_FRRET),
			GsmIMEI()),
			","),
			cmd), buffer
		);
		return E_OK;
	}
	return E_ERROR;
}

/*******************************************************************************
* Function Name	:	GGC_GprsSettings
* Description	:	Send message to host
*******************************************************************************/
uint8_t GGR_General(char* buffer, int buf_size, bool sms, const GgCommand_t * cmd)
{
	if (sms)
		return E_ERROR;
	return GGR_Command(buffer, buf_size, cmd->Command);
}

/*******************************************************************************
* Function Name	:	GGC_GprsSettings
* Description	:	Send message to host
*******************************************************************************/
uint8_t GGR_GprsSettings(char* buffer, int buf_size, bool sms, const GgCommand_t * cmd)
{
	if (!sms && buffer != NULL && buf_size != 0)
	{
		NmeaAddChecksum( strcpyEx( strcpyEx( strcpyEx( strcpyEx( strcpyEx( buffer,
			DGG_FRRET),
			GsmIMEI()),
			","),
			GG_GPRS_SETTINGS),
			",,GpsGate TrackerOne,FWTracker,"VERSION_INFO),
			buffer
		);
		return E_OK;
	}	
	return E_ERROR;
}

/*******************************************************************************
* Function Name	:	GGC_StartTracking
* Description	:
*******************************************************************************/
uint8_t GGC_StartTracking(char* cmd, const GgCommand_t * ggCommand, bool exec)
{
	char *token = cmd;
	int value;
	while (token != NULL)
	{
		if (TokenSizeComma(token) >= (sizeof("TimeFilter=") - 1 + 2))
		{
			if (strncmp(token, "TimeFilter=", sizeof("TimeFilter=") - 1) == 0)
			{
				value = nmea_atoi(token + sizeof("TimeFilter=") - 1, 5, 10);
				if (value >= 15 && value <= (60 * 60))
				{
					if (exec && GpsContext.SendInterval != value)
						GpsContext.SendInterval = value;
					return E_OK;
				}
				return E_ERROR;
			}
		}
		token = TokenNextComma(token);
	}
	return E_ERROR;
}

/*******************************************************************************
* Function Name	:	GGC_StopTracking
* Description	:
*******************************************************************************/
uint8_t GGC_StopTracking(char* cmd, const GgCommand_t * ggCommand, bool exec)
{
	return E_OK;
}
/*******************************************************************************
* Function Name	:	GGC_PollPosition
* Description	:
*******************************************************************************/
uint8_t GGC_PollPosition(char* cmd, const GgCommand_t * ggCommand, bool exec)
{
	return E_OK;
}
/*******************************************************************************
* Function Name	:	GGC_DeviceReset
* Description	:
*******************************************************************************/
uint8_t GGC_DeviceReset(char* cmd, const GgCommand_t * ggCommand, bool exec)
{
	return E_OK;
}

/*******************************************************************************
* Function Name	:	ggFindCommand
* Description	:
*******************************************************************************/
const GgCommand_t * ggFindCommand(char *cmd, const GgCommand_t * cmds, int size)
{
	while (cmds->Command != NULL)
	{
		if (strncmp(cmd, cmds->Command, size) == 0)
			return cmds;
		cmds++;
	}
	return NULL;
}

/*******************************************************************************
* Function Name	:	CheckGGCommand
* Description	:	Compare and remove checksum from line
*******************************************************************************/
uint8_t GG_CheckCommand(char * cmd)
{
	uint8_t cs;
	char * pos;
	int len;

	len = strlen(cmd);
	if (len > 10)
	{
		pos = strchr(cmd, '*');
		if (pos != NULL && ((pos - cmd) == (len - 3)))
		{
			cs = nmea_atoi(pos + 1, 2, 16);
			*pos = 0;
			if (cs == nmea_calc_crc(cmd + 1, len - 4))
				return E_OK;
		}
	}
	return E_BAD_COMMAND;
}

/*******************************************************************************
* Function Name	:	ProcessGpsGateCommand
* Description	:
*******************************************************************************/
const GgCommand_t * ProcessGpsGateCommand(char * text, bool exec)
{
	int size;
	uint8_t state = 0;
	const GgCommand_t * gg_cmd = NULL;

	while (text != NULL)
	{
		size = TokenSizeComma(text);
		if (state == 0)
		{	// Check for checksum and command
			if (size == 6 && GG_CheckCommand(text) == E_OK)
			{
				if (strncmp(text, "$FRRET", size) == 0)
					gg_cmd = &GgReplayMap[0];
				else if (strncmp(text, "$FRCMD", size) == 0)
					gg_cmd = &GgCommandMap[0];
			}
			if (gg_cmd == NULL)
				break;
		}
		else if (state == 1)
		{	// Check IMEI code from command and module
			if (size == 0 || strncmp(text, GsmIMEI(), size) != 0)
				break;
		}
		else if (state == 2)
		{	// Find command and execute handler
			if (size != 0
			&&	(gg_cmd = ggFindCommand(text, gg_cmd, size)) != NULL
				)
			{
				text = TokenNextComma(text);	// skip command name
				text = TokenNextComma(text);	// skip inline
				if ((gg_cmd->Handler)(text, gg_cmd, exec) == E_OK)
					return gg_cmd;
			}
			break;
		}

		text = TokenNextComma(text);
		++state;
	}

	return NULL;
}
