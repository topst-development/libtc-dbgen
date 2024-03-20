/****************************************************************************************
 *   FileName    : UTF.c
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

#include "UTF.h"


static unsigned char * confirm_utf(unsigned char *p, int *utf_size);

/**************************************************************************
*  FUNCTION NAME :
*      unsigned char * confirm_utf(unsigned char *p, int *utf_size);
*
*  DESCRIPTION : Skip UTF-8's first 3 bytes (0xEF, 0xBB, 0xBF)
*  INPUT:
*			p	=
*			utf_size	=
*
*  OUTPUT:	char - Return Type
*  			=
*  REMARK  :
**************************************************************************/
static unsigned char * confirm_utf(unsigned char *p, int *utf_size)
{
	if ( (*p == 0xEF) && (*(p + 1) == 0xBB) && (*(p + 2) == 0xBF) )
	{
		p += 3;
		*utf_size -= 3;
	}

	return p;
}


/**************************************************************************
*  FUNCTION NAME :
*      int iUTF2Unicode(unsigned char *utf_code, unsigned char *unicode, int utf_size);
*
*  DESCRIPTION :
*  INPUT:
*			unicode	=
*			utf_code	=
*			utf_size	=
*
*  OUTPUT:	int - Return Type
*  			=
*  REMARK  :
**************************************************************************/
int iUTF2Unicode(unsigned char *utf_code, unsigned char *unicode, int utf_size)
{
	int index;
	unsigned char *p_UTF, *p_Unicode;
	int unicode_size = 0, sizeFlag=0;
	p_UTF = utf_code;
	p_Unicode = unicode;
	//p_Unicode = init_uni(p_Unicode, utf_size);
	p_UTF = confirm_utf(p_UTF, &utf_size);
	//if( p_UTF == NULL ) return -1;
	/*
		-. Read first bytes.
		-. '&' operation with respectively 0x80, 0xC0, 0xE0.
		   Results means that you must read 1 bytes or 2~3 bytes, respectively.
		-. When 0xC0 '&' operation, there can be 3 byte read case.
		   So, it must not be 0xE0.
	 */
	index = 0;

	while (index < utf_size)	/*for( index = 0; index < utf_size; )*/
	{
		/* Read 1 byte operation */
		if ( (*p_UTF & 0x80) == 0x00 )
		{
			/* If it is 0xFE, 0xFF method, this must be changed. */
#if 1
			*p_Unicode = *p_UTF;
			*(p_Unicode + 1) = 0x00;
#else
			*p_Unicode = 0x00;
			*(p_Unicode + 1) = *p_UTF;
#endif
			p_UTF++;
			index += 1;
			sizeFlag = index;
		}
		/* Read 2 bytes operation */
		else if ( ((*p_UTF & 0xC0) == 0xC0) && ((*p_UTF & 0xE0) != 0xE0))
		{
#if 1
			*(p_Unicode) = (*p_UTF & 0x03) << 6;
			*(p_Unicode) = *(p_Unicode) | (*(p_UTF + 1) & 0x3F);
			*(p_Unicode+1) = (*p_UTF & 0x1C) >> 2;
#else
			*p_Unicode = (*p_UTF & 0x1C) >> 2;
			*(p_Unicode + 1) = (*p_UTF & 0x03) << 6;
			*(p_Unicode + 1) = *(p_Unicode + 1) | (*(p_UTF + 1) & 0x3F);
#endif
			p_UTF += 2;
			index += 2;
			sizeFlag = index;
		}
		/* Read 3 bytes operation */
		else if ( (*p_UTF & 0xE0) == 0xE0 )
		{
#if 1
			*(p_Unicode) = (*(p_UTF + 1) & 0x03) << 6;
			*(p_Unicode) = *(p_Unicode) | (*(p_UTF + 2) & 0x3F);
			*(p_Unicode+1) = *p_UTF << 4;
			*(p_Unicode+1) = *(p_Unicode +1)| ((*(p_UTF + 1) & 0x3C) >> 2);
#else
			*p_Unicode = *p_UTF << 4;
			*p_Unicode = *p_Unicode | ((*(p_UTF + 1) & 0x3C) >> 2);
			*(p_Unicode + 1) = (*(p_UTF + 1) & 0x03) << 6;
			*(p_Unicode + 1) = *(p_Unicode + 1) | (*(p_UTF + 2) & 0x3F);
#endif
			p_UTF += 3;
			index += 3;
			sizeFlag = index;
		}
		/* Othe cases are removed (over than 4 bytes) */
		else
		{
			index += 4;
		}

		unicode_size += 1;
		p_Unicode += 2;
	}

	if (sizeFlag == 0)
	{
		return 0;
	}

	//unicode_size = p_Unicode-unicode;
	return unicode_size;
}

/* end of file */

