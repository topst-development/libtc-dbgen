/****************************************************************************************
 *   FileName    : TCUtil.c
 *   Description : c file of util
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

#include <errno.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <linux/types.h>
#include <stdint.h>
#include "TCUtil.h"

#ifndef NULL
#define NULL 0
#endif

char *newformat(const char *format, ...)
{
	int32_t length;
	char *s;
	va_list va;
	va_start(va, format);

	length = vsnprintf(NULL, 0, format, va) + 1;

	s = (char *)malloc((size_t)length);
	if(s!=NULL)
	{
		(void)vsnprintf(s, (size_t)length, format, va);
	}

	va_end(va);

	return s;
}


