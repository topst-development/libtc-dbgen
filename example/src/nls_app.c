/****************************************************************************************
 *   FileName    : ID3.c
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
#include <stdlib.h>
#include <stdio.h>
#include "nls720.h"

#ifdef __cplusplus
extern "C" {
#endif


/**************************************************************************
*  FUNCTION NAME	:
*  		UNI_MappingFontOneUnicode
*
*  DESCRIPTION		:
*
*  INPUT			:
*  			onechar =
*  			WideChar =
*
*  OUTPUT			: int - Return Type
*
*  REMARK			:
*
**************************************************************************/
int UNI_MappingFontOneUnicode(unsigned char *onechar,void *WideChar)
{
	int retval;
	unsigned short int lTempCode;
	wchar_t *fontmp = (wchar_t *)WideChar;

//#ifdef ONEBYTE_UNICODE_CONVERT_INCLUDE
	if (*onechar < 0x80)
	{
		retval = 1;
	}
//#else
//	if (*onechar <= 0x80)
//	{
//		retval = 1;
//	}
//#endif
	else
	{
		if (char2uni != 0)
		{
			retval = char2uni(onechar, fontmp);
		}
		else
		{
			lTempCode = (unsigned short)(onechar[0] | ((unsigned short)onechar[1] << 8));	/* QAC : 12.8 */
			*fontmp = lTempCode;
			retval = 2;
		}
	}

	return retval;
}

///////////////////////////////////////////////////////////////////////////////////
//
// Function    : UNI_ConvertStringToUnicode
//
// Description : Conver National character string into Unicode
//
///////////////////////////////////////////////////////////////////////////////////
int	UNI_ConvertStringToUnicode(const unsigned char *pString,unsigned char *pUnicodeBuf,unsigned long iMaxStringSize)
{
	unsigned int 	i, j;
	unsigned char 	tempchar[2];
	unsigned char 	ucWideChar[2];
	unsigned long		iWide;
	j = 0;
	for (i = 0; i < iMaxStringSize; i += iWide)
	{
		if (pString[i] == 0 )
		{
			break;
		}

		tempchar[0] = (unsigned char)pString[i+0];
		tempchar[1] = (unsigned char)pString[i+1];
		iWide = UNI_MappingFontOneUnicode(tempchar,ucWideChar);

		switch (iWide)
		{
			case 2: // Wide Character
				break;
			case 1:	// Not Wide Character

				if (tempchar[0] <0x80)
				{
					ucWideChar[0] = tempchar[0];
					ucWideChar[1] = 0x00;
				}
				else
				{
					ucWideChar[1] =  0x00;
				}

				break;
			default:
				return -2;		//error
		}

		pUnicodeBuf[j+0] =  ucWideChar[0];
		pUnicodeBuf[j+1] =  ucWideChar[1];
		j+=2;
	}

	pUnicodeBuf[j+0] =  0;
	pUnicodeBuf[j+1] =  0;
	return (int)j;	// SUCCESS
}

/* Utility code - temperary */
int Unicode2UTF8(const unsigned char *pUnicode,unsigned char *pUTF8, int Unicodelen ,int maxlen)
{
           unsigned short unicode;
           int len=0;

           do {
                     if (len>=maxlen) {
                                break;
                     }
                     unicode=(short)(pUnicode[0])+((short)pUnicode[1])*256;
                     if (unicode<0x80) {
                                *pUTF8++=(unsigned char)unicode;
                                if (++len>=maxlen) {
                                          break;
                                }
                     } else if (unicode<0x0800) {
                                *pUTF8++=(unsigned char)(0xc0 | (((unsigned char)(unicode>>6))&0x1F));
                                if (++len>=maxlen) {
                                          break;
                                }
                                *pUTF8++=(unsigned char)(0x80 | ((unsigned char)(unicode&0x003F)));
                                if (++len>=maxlen) {
                                          break;
                                }
                     } else {
                                *pUTF8++=(unsigned char)(0xe0 | (((unsigned char)(unicode>>12))&0x0F));
                                if (++len>=maxlen) {
                                          break;
                                }
                                *pUTF8++=(unsigned char)(0x80 | (((unsigned char)(unicode>>6))&0x3F));
                                if (++len>=maxlen) {
                                          break;
                                }
                                *pUTF8++=(unsigned char)(0x80 | ((unsigned char)(unicode&0x003F)));
                                if (++len>=maxlen) {
                                          break;
                                }
                     }
                    pUnicode+=2;
 		      Unicodelen-=2;
           } while ((unicode!=0)&&(Unicodelen > 0));

           if(unicode !=0)
           {
		*pUTF8++=0x00;
           }

           return len;
}


#ifdef __cplusplus
}
#endif
