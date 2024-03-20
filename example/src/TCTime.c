/****************************************************************************************
 *   FileName    : TCTime.c
 *   Description : 
 ****************************************************************************************
 *
 *   TCC Version 1.0
 *   Copyright (c) Telechips Inc.
 *   All rights reserved 
 
This source code contains confidential information of Telechips.
Any unauthorized use without a written permission of Telechips including not limited 
to re-distribution in source or binary form is strictly prohibited.
This source code is provided ¡°AS IS¡± and nothing contained in this source code 
shall constitute any express or implied warranty of any kind, including without limitation, 
any warranty of merchantability, fitness for a particular purpose or non-infringement of any patent, 
copyright or other third party intellectual property right. 
No warranty is made, express or implied, regarding the information¡¯s accuracy, 
completeness, or performance. 
In no event shall Telechips be liable for any claim, damages or other liability arising from, 
out of or in connection with this source code or the use in the source code. 
This source code is provided subject to the terms of a Mutual Non-Disclosure Agreement 
between Telechips and Company.
*
****************************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <time.h>
#include <math.h>
#define TC_LOCALTIME(__time) localtime_r(&(__time), &g_tm)
#include "TCTime.h"

static time_t g_time;
static int g_us;
static struct tm g_tm;
static char g_output_text[32];

void SetTime(int year, int month, int day, int hour, int minute, double second)
{
	struct tm tmBuf;
	tmBuf.tm_year = year - 1900;
	tmBuf.tm_mon = month - 1;
	tmBuf.tm_mday = day;
	tmBuf.tm_hour = hour;
	tmBuf.tm_min = minute;
	tmBuf.tm_sec = (int)second;
	g_us = (int)((second - floor(second)) * 1000000 + 0.5);
	if (g_us >= 1000000)
	{
		tmBuf.tm_sec++;
		g_us %= 1000000;
	}
	g_time = mktime(&tmBuf);
	TC_LOCALTIME(g_time);
}

void SetCurrentTime()
{
	struct timeval timeVal;
	gettimeofday(&timeVal, NULL);
	g_time = timeVal.tv_sec;
	g_us = timeVal.tv_usec;

	if (g_us >= 1000000)
	{
		g_time++;
		g_us %= 1000000;
	}
	TC_LOCALTIME(g_time);
}

int GetYear()
{
	return g_tm.tm_year + 1900;
}

int GetMonth()
{
	return g_tm.tm_mon + 1;
}

int GetDay()
{
	return g_tm.tm_mday;
}

int GetHour()
{
	return g_tm.tm_hour;
}

int GetMinute()
{
	return g_tm.tm_min;
}

double GetSecond()
{
	return g_tm.tm_sec + g_us / 1000000.0;
}

double GetSecondsUntilNow()
{
	struct timeval now;
	gettimeofday(&now, NULL);
	return now.tv_sec - g_time + (now.tv_usec - g_us) / 1000000.0;
}

const char *GetString()
{
	snprintf(g_output_text, 32, "%d-%02d-%02d %02d:%02d:%02d.%03d",
		g_tm.tm_year + 1900, g_tm.tm_mon + 1, g_tm.tm_mday,
		g_tm.tm_hour, g_tm.tm_min, g_tm.tm_sec, (int)(g_us / pow(10.0, 3) + 0.5));

	return g_output_text;
}

