#ifndef __GPSONE_H__
#define __GPSONE_H__

#include <stdint.h>
#include <stdbool.h>

#define DGG_FRCMD			"$FRCMD,"
#define DGG_FRERR			"$FRERR,"
#define DGG_FRRET			"$FRRET,"
#define DGG_FRERR_AUTH		"$FRERR,AuthError,"
#define DGG_FRERR_NOTSUP	"$FRERR,NotSupported,"
#define DGG_FRERR_NOTEXEC	"$FRERR,CannotExecute,"
#define DGG_GPRS_SETTINGS	"_GprsSettings"
#define DGG_START_TRACKING	"_StartTracking"

#define GG_FRCMD_S			(sizeof(DGG_FRCMD) - 1)
#define GG_FRERR_S			(sizeof(DGG_FRERR) - 1)
#define GG_FRRET_S			(sizeof(DGG_FRRET) - 1)
#define GG_FRERR_AUTH_S		(sizeof(DGG_FRERR_AUTH) - 1)
#define GG_FRERR_NOTSUP_S	(sizeof(DGG_FRERR_NOTSUP) - 1)
#define GG_FRERR_NOTEXEC_S	(sizeof(DGG_FRERR_NOTEXEC) - 1)

typedef struct {
	uint32_t	DateTime;
	uint32_t	DateTimeInt;
	int			Index;
	int			Count;
} GgCommandCounts;

struct GgCommand_s {
	const char		* Command;
	GgCommandCounts * Counts;
			uint8_t	(*Handler)(char *, const struct GgCommand_s *, bool);
			uint8_t	(*Response)(char * buffer, int buf_size, bool sms, const struct GgCommand_s * cmd);
};

typedef struct GgCommand_s GgCommand_t;

extern const char GG_FRCMD_PING[];
extern const char GG_FRCMD_INIT_MSG[];
extern const char GG_FRCMD_UPD_FW[];
extern const char GG_FRCMD_UPD_FDB[];
extern const char GG_FRCMD_UPD_BLOCK[];

extern const char GG_FRERR[];
extern const char GG_FRRET[];
extern const char GG_FRCMD[];
extern const char GG_FRERR_AUTH[];
extern const char GG_FRERR_NOTSUP[];
extern const char GG_FRERR_NOTEXEC[];

extern const GgCommand_t GgCommandMap[];
extern const char GG_GPRS_SETTINGS[];
extern const char GG_START_TRACKING[];

extern char GG_FwFileName[32];
extern int  GG_FwSmsIndex;

const GgCommand_t * ProcessGpsGateCommand(char * cmd, bool);
uint8_t GG_CheckCommand(char *);
void GG_ClearAllCounts(void);

#endif
