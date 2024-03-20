/****************************************************************************************
 *   FileName    : ID3Parser.h
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

#ifndef __ID3_PARSER_H__
#define __ID3_PARSER_H__
#include "TCTagParser.h"
//#define WMA_INCLUDE
//#define ID3_WMA_PRE_READ_BUFFER

#define MAX_ID3V1_SIZE		128
#define MAX_ID3_FIND_SIZE	2048

#define ID3_FLAG_UNSYNCH   0x80 // a. Unsynchronisation
#define ID3_FLAG_EXTHEADER 0x40 // b. Extended Header
#define ID3_FLAG_INDICATOR 0x20 // c. Experimental Indicator
#define ID3_FLAG_FOOTER    0x10	// d. Footer Present

#define ID3_FLAG_UPDATE    0x40
#define ID3_FLAG_CRCDATA   0x20
#define ID3_FLAG_RESTRICT  0x10

#define ID3_ENCODE_MASK 	  0x03
#define ID3_ENCODE_ISO88591   0x00
#define ID3_ENCODE_UTF16	  0x01
#define ID3_ENCODE_UTF16BE	  0x02
#define ID3_ENCODE_UTF8 	  0x03

#define ID3_UNICODE_BOM1	  0xFFFE
#define ID3_UNICODE_BOM2	  0xFEFF

#define ID3_GENRE_MAX_CNT	  148

#define ID3_NULL 			0 //'\0'
#define ID3_SPACE 			32 //' '
#define ID3_OPEN_BRACKET	40
#define ID3_CLOSE_BRACKET	41
#define ID3_ZERO 			48 //'0'
#define ID3_ONE 				49
#define ID3_TWO 				50
#define ID3_THREE 			51
#define ID3_FOUR 			52
#define ID3_NINE 			57

#define ID3_UPPERCASE_A 	65
#define ID3_UPPERCASE_B 	66
#define ID3_UPPERCASE_C	67
#define ID3_UPPERCASE_D 	68
#define ID3_UPPERCASE_E 	69
#define ID3_UPPERCASE_F 	70
#define ID3_UPPERCASE_G 	71
#define ID3_UPPERCASE_H 	72
#define ID3_UPPERCASE_I 	73
#define ID3_UPPERCASE_J 	74
#define ID3_UPPERCASE_K 	75
#define ID3_UPPERCASE_L 	76
#define ID3_UPPERCASE_M 	77
#define ID3_UPPERCASE_N 	78
#define ID3_UPPERCASE_O 	79
#define ID3_UPPERCASE_P 	80
#define ID3_UPPERCASE_Q 	81
#define ID3_UPPERCASE_R 	82
#define ID3_UPPERCASE_S 	83
#define ID3_UPPERCASE_T 	84
#define ID3_UPPERCASE_U 	85
#define ID3_UPPERCASE_V 	86
#define ID3_UPPERCASE_W 	87
#define ID3_UPPERCASE_X 	88
#define ID3_UPPERCASE_Y 	89
#define ID3_UPPERCASE_Z 	90

enum ID3Version
{
	ID3_V10 = 0,
	ID3_V11,
	ID3_V2X
};

enum {
	ID3V2_3_FRAME_STATUS_TAG_ALTER_PRESERVE	= 0x8000,
	ID3V2_3_FRAME_STATUS_FILE_ALTER_PRESERVE	= 0x4000,
	ID3V2_3_FRAME_STATUS_READ_ONLY			= 0x2000,
	ID3V2_3_FRAME_FORMAT_COMPRESSION		= 0x0080,
	ID3V2_3_FRAME_FORMAT_ENCRYPTION			= 0x0040,
	ID3V2_3_FRAME_FORMAT_GROUPING_ID		= 0x0020,
	ID3V2_FRAME_STATUS_TAG_ALTER_PRESERVE	= 0x4000,
	ID3V2_FRAME_STATUS_FILE_ALTER_PRESERVE	= 0x2000,
	ID3V2_FRAME_STATUS_READ_ONLY			= 0x1000,
	ID3V2_FRAME_FORMAT_GROUPING_ID			= 0x0040,
	ID3V2_FRAME_FORMAT_COMPRESSION			= 0x0008,
	ID3V2_FRAME_FORMAT_ENCRYPTION			= 0x0004,
	ID3V2_FRAME_FORMAT_UNSYNCHRONISATION	= 0x0002,
	ID3V2_FRAME_FORMAT_DATA_LENGTH_INDICATOR = 0x0001
};


typedef struct __attribute__((__packed__)) stID3Info
{
	unsigned char mTAG[3];
	unsigned char mTitle[30];
	unsigned char mArtist[30];
	unsigned char mAlbum[30];
	unsigned char mYear[4];
	unsigned char mComment[29];
	unsigned char mTrack;
	unsigned char mGenre;
} stID3InfoType,*pstID3InfoType;

typedef struct __attribute__((__packed__)) _ID3V2XHeaderType
{
	unsigned char  mID3[3];
	unsigned short mVersion;
	unsigned char  mFlags;
	unsigned char  mSize[4];
} ID3V2XHeaderType, *pID3V2XHeaderType;

typedef struct __attribute__((__packed__)) _ID3V2XFooterType
{
	unsigned char  mID3[3];
	unsigned short mVersion;
	unsigned char  mFlags;
	unsigned char  mSize[4];
} ID3V2XFooterType,*pID3V2XFooterType;

typedef struct __attribute__((__packed__)) _ID3V2XExtHeaderType
{
	unsigned char  mHeaderSize[4];
	unsigned char  mFlagNum;
	unsigned char  mExtFlags;
} ID3V2XExtHeaderType,*pID3V2XExtHeaderType;

typedef struct __attribute__((__packed__)) _ID3V2XFrameInfoType
{
	unsigned char  mFrameID[4];
	unsigned char  mSize[4];
	unsigned char  mFlags[2];
} ID3V2XFrameInfoType,*pID3V2XFrameInfoType;

typedef struct __attribute__((__packed__)) _ID3V22FrameInfoType
{
	unsigned char  mFrameID[3];
	unsigned char  mSize[3];
} ID3V22FrameInfoType,*pID3V22FrameInfoType;

typedef struct __attribute__((__packed__)) _ID3V2XEncodeInfoType
{
	unsigned char   mEncodeType;
	unsigned short  mUnicodeBOM;
} ID3V2XEncodeInfoType,*pID3V2XEncodeInfoType;

long ID3_GetMP3Data(int fd, pstID3V2XInfoType pID3V2XInfo);

#endif	// __ID3_PARSER_H__

/* end of file */

