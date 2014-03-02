/*
 *
 * NMEA library
 * URL: http://nmea.sourceforge.net
 * Author: Tim (xtimor@gmail.com)
 * Licence: http://www.gnu.org/licenses/lgpl.html
 * $Id: time.c 4 2007-08-27 13:11:03Z xtimor $
 *
 */

/*! \file time.h */

#include "nmea/time.h"

#ifdef NMEA_WIN
	#pragma warning(disable: 4201)
	#pragma warning(disable: 4214)
	#pragma warning(disable: 4115)
	#include <windows.h>
	#pragma warning(default: 4201)
	#pragma warning(default: 4214)
	#pragma warning(default: 4115)
#else
	#include <time.h>
#endif

#ifdef NMEA_WIN

void nmea_time_now(nmeaTIME *stm)
{
    SYSTEMTIME st;

    GetSystemTime(&st);

    stm->year = st.wYear - 1900;
    stm->mon = st.wMonth - 1;
    stm->day = st.wDay;
    stm->hour = st.wHour;
    stm->min = st.wMinute;
    stm->sec = st.wSecond;
    stm->hsec = st.wMilliseconds / 10;
}

#else /* NMEA_WIN */

void nmea_time_now(nmeaTIME *stm)
{
    stm->year	= 0;
    stm->mon	= 0;
    stm->day	= 0;
    stm->hour	= 0;
    stm->min	= 0;
    stm->sec	= 0;
    stm->hsec	= 0;
}

#endif
