/****************************************************************************************
 *   FileName    : TCTagParser.h
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

#ifndef __TC_TAG_PARSER_H__
#define __TC_TAG_PARSER_H__

#define MAX_ID3V2_FIELD		512
#define MAX_ID3V2_YEAR		40

#define ID3_UNICODE_START	  1

enum ID3FrameInfo
{

	ID3_TITLE = 0,
	ID3_ARTIST,
	ID3_ALBUM,
	ID3_GENRE,
#ifdef ID3_EXT_INCLUDE	/* EXT Tag not supported */
	ID3_ALBUMARTIST,
	ID3_COMMENT,
	ID3_COMPOSER,
	ID3_ORIGARTIST,
	ID3_COPYRIGHT,
	ID3_URL,
	ID3_ENCODEDBY,
#endif
	ID3_NONINFO,
	ID3_YEAR,
	ID3_TRACK,
	ID3_PICTURE,	/* PICTURE is not supported */
	ID3_NOTUSEDFRAME
};

enum
{
	ID3_NO_ERROR =0,
	ID3_ERROR_Pasing, // 1  // not used
	ID3_ERROR_INVALID_FILE_SIZE, // 2
	ID3_ERROR_INVALID_HEADER, // 3
	ID3_ERROR_INVALID_DATA,// 4
	ID3_ERROR_BAD_ARGUMENTS,// 5
	ID3_ERROR_MALOC, // 6
	ID3_ERROR_FILEREAD, //7
	ID3_ERROR_NOT_SUPPORT, //8
	ID3_ERROR_INVALIDSIZE_OBJECT // 0x9
}ID3_PARSING_ERROR;

typedef  struct __attribute__((__packed__)) stID3V2XInfo
{
	unsigned int mUnSyncFlag;
	unsigned short mTrack;
	unsigned char  mYear[MAX_ID3V2_YEAR];
	unsigned char  mMetaData[ID3_NONINFO][MAX_ID3V2_FIELD];
} stID3V2XInfoType,*pstID3V2XInfoType;

typedef int (*ConvertStringToUTF8_cb)(const unsigned char *pInString,unsigned char *pOutUTF8String,unsigned long bufferSize);

void TC_ClearID3V2Info(stID3V2XInfoType * ID3Data);
long TC_GetMP3Tag(const char *pfolderPath, const char * pfileName, stID3V2XInfoType * ID3Data);
long TC_GetWMATag(const char *pfolderPath, const char * pfileName, stID3V2XInfoType * ID3Data);

#endif	// __TC_TAG_PARSER_H__

/* end of file */

