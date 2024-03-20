/****************************************************************************************
 *   FileName    : WMAParser.c
 *   Description : 
 ****************************************************************************************
 *
 *   TCC Version 1.0
 *   Copyright (c) Telechips Inc.
 *   All rights reserved 
 
This source code contains confidential information of Telechips.
Any unauthorized use without a written permission of Telechips including not limited 
to re-distribution in source or binary form is strictly prohibited.
This source code is provided “AS IS” and nothing contained in this source code 
shall constitute any express or implied warranty of any kind, including without limitation, 
any warranty of merchantability, fitness for a particular purpose or non-infringement of any patent, 
copyright or other third party intellectual property right. 
No warranty is made, express or implied, regarding the information’s accuracy, 
completeness, or performance. 
In no event shall Telechips be liable for any claim, damages or other liability arising from, 
out of or in connection with this source code or the use in the source code. 
This source code is provided subject to the terms of a Mutual Non-Disclosure Agreement 
between Telechips and Company.
*
****************************************************************************************/
#ifdef WMA_INCLUDE
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "TCTagParser.h"
//#include "ID3Parser.h"
#include "nls720.h"
#include "nls_app.h"

typedef struct
{
	unsigned long long	currPacketOffset;
	unsigned long		dwLo;
	unsigned long		dwHi;
	unsigned short		dwObjNum;
	unsigned short		wFormatTag;
	unsigned long		samplingfreq;
	unsigned long		avgbitrate;
} ID3_WMAHeader;

enum ID3_WMA_ExtConDesObjList
{
	ID3_WMA_AlbumTitle = 0,
	ID3_WMA_AlbumArtist,
	ID3_WMA_Genre,
	ID3_WMA_Picture,
#ifdef WMA_EXTEND_DATA_INCLUDE
	ID3_WMA_Year,
	ID3_WMA_TrackNo,
#endif
	ID3_WMA_MAX
};

typedef struct
{
	unsigned long     Data0;
	unsigned long     Data1;
	unsigned long     Data2;
	unsigned long     Data3;
} ID3_WMAGUID;

#define ID3_MIN_OBJECT_SIZE		24
#define ID3_DATA_OBJECT_SIZE	50
#define ID3_WMA_HEADER_SIZE		1024*5
#define ID3_WMA_PRE_READ_BUFFER

#ifdef ID3_WMA_PRE_READ_BUFFER
extern unsigned char WMA_HeaderBuffer[ID3_WMA_HEADER_SIZE];
#endif

typedef struct
{
	unsigned int	uiNameSize;
	unsigned char	*pWM_NAME;
} ID3_WM_ExtContAttr;

#ifdef ID3_WMA_PRE_READ_BUFFER
 int ID3_WMA_HeaderStartOffset;
 int ID3_WMA_HeaderEndOffset;
 unsigned char ID3_WMA_HeaderBuffer[ID3_WMA_HEADER_SIZE];
#endif


static int ID3_WMALoadHeaderObject(int fd, ID3_WMAHeader *Header, unsigned char *pData);
static int ID3_WMAGetData(int fd,  unsigned char **pData, unsigned long ulOffset, unsigned int uiNumBytes);
static int ID3_WMALoadObjectHeader(int fd, ID3_WMAHeader *Header, unsigned char *pData, ID3_WMAGUID *pObjectId);
static int ID3_WMALoadContentDescObject(int fd, ID3_WMAHeader *Header,unsigned char *pData, unsigned long cbSizeHi, unsigned long cbSizeLo,
                                       pstID3V2XInfoType pID3V2XInfo);
static int ID3_WMALoadExtendedContentDescObject(int fd, ID3_WMAHeader *Header,
        unsigned char *pData, unsigned long cbSizeHi, unsigned long cbSizeLo,
        pstID3V2XInfoType pID3V2XInfo);
static int ID3_WMAChkExtObjectAttr ( unsigned char *pData, int size );
static void ID3_CDOLoadLength(unsigned short *usContentSize, unsigned char **pData);
static int ID3_WMALoadStreamPropertiesObject(int fd, ID3_WMAHeader *Header, unsigned char *pData, unsigned long cbSize);



/******************************************************************************
*
*	FUNCTION
*
*	int TCDB_META_WMAParseAsfHeader(int iHandle, META_WMAHeader *Header,
*								unsigned char *pData, pstID3V2XInfoType pID3Info)
*
*	Input	: NONE
*	Output	: NONE
*	Return	: NONE
*	Description :
*
******************************************************************************/
 int ID3_WMAParseAsfHeader(int fd, ID3_WMAHeader *Header, unsigned char *pData, pstID3V2XInfoType pID3V2XInfo)
{
	const unsigned long GUID_CAsfContentDescObject[] = {0x3326b275, 0x8e66cf11, 0xa6d900aa, 0x0062ce6c};
	//const unsigned long GUID_CAsfStreamPropertiesObject[] = {0x9107dcb7, 0xb7a9cf11, 0x8ee600c0, 0x0c205365};
	const unsigned long GUID_CAsfExtendedContentDescObject[] = {0x40a4d0d2, 0x07e3d211, 0x97f000a0, 0xc95ea850};
	int iRet;
	ID3_WMAGUID objId;
	unsigned long long cbFirstPacketOffset;
	unsigned long long TmpData;
	int ObjCnt;
	/* ASF Header Object */

	if((fd < 0)||(Header==NULL)||(pID3V2XInfo==NULL))
	{
		return -ID3_ERROR_BAD_ARGUMENTS;
	}

	iRet = ID3_WMALoadHeaderObject(fd, Header, pData);		// ASF Header Object
	ObjCnt = Header->dwObjNum;

	if (iRet != ID3_NO_ERROR)
	{
		return iRet;
	}

	TmpData = ((unsigned long long)Header->dwHi <<32) | (unsigned long long)Header->dwLo;
	cbFirstPacketOffset = TmpData + ID3_DATA_OBJECT_SIZE;

	while (Header->currPacketOffset < cbFirstPacketOffset)		// Scan Header Objects
	{
		iRet = ID3_WMALoadObjectHeader(fd, Header, pData, &objId);
		ObjCnt--;

		if (iRet != ID3_NO_ERROR)
		{
			return iRet;
		}

		if(ObjCnt < 0) // objcnt is smalller than 0, wma haeder have no meta object
		{
			break;
		}

#if 0

		if ( objId.Data0 == GUID_CAsfStreamPropertiesObject[0] &&
		        objId.Data1 == GUID_CAsfStreamPropertiesObject[1] &&
		        objId.Data2 == GUID_CAsfStreamPropertiesObject[2] &&
		        objId.Data3 == GUID_CAsfStreamPropertiesObject[3] )
		{
			iRet = TCDB_META_WMALoadStreamPropertiesObject(iHandle, Header, pData, Header->dwLo,iDeviceType);

			if (iRet != TCDB_NoError)
			{
				return iRet;
			}
		}
		else
#endif
			if ( (objId.Data0 == GUID_CAsfContentDescObject[0]) &&
			        (objId.Data1 == GUID_CAsfContentDescObject[1]) &&
			        (objId.Data2 == GUID_CAsfContentDescObject[2]) &&
			        (objId.Data3 == GUID_CAsfContentDescObject[3] ))
			{
				iRet = ID3_WMALoadContentDescObject(fd, Header, pData, Header->dwHi, Header->dwLo, pID3V2XInfo);

				if (iRet != ID3_NO_ERROR)
				{
					return iRet;
				}
			}
			else if ( (objId.Data0 == GUID_CAsfExtendedContentDescObject[0]) &&
			          (objId.Data1 == GUID_CAsfExtendedContentDescObject[1]) &&
			          (objId.Data2 == GUID_CAsfExtendedContentDescObject[2]) &&
			          (objId.Data3 == GUID_CAsfExtendedContentDescObject[3] ))
			{
				iRet = ID3_WMALoadExtendedContentDescObject(fd, Header, pData, Header->dwHi, Header->dwLo, pID3V2XInfo);
				if (iRet != ID3_NO_ERROR)
				{
					return iRet;
				}
			}
			else
			{
				/* skip over this object */
				TmpData = ((unsigned long long)Header->dwHi <<32) | (unsigned long long)Header->dwLo;
				Header->currPacketOffset += TmpData - ID3_MIN_OBJECT_SIZE;

				if ( (cbFirstPacketOffset < Header->currPacketOffset) && (ObjCnt > 0))
				{
					return -ID3_ERROR_INVALIDSIZE_OBJECT;
				}
			}
	}

	return ID3_NO_ERROR;
}

/**************************************************************
*
*	FUNCTION
*
*	int TCDB_META_WMALoadHeaderObject(int iHandle,
*								struct tWMA_HdrTCC *Header,
*								unsigned char *pData)
*
*	Input	: NONE
*	Output	: NONE
*	Return	: NONE
*	Description : Initialize variables for copy.
*
***************************************************************/
static int ID3_WMALoadHeaderObject(int fd, ID3_WMAHeader *Header, unsigned char *pData)
{
	const unsigned long GUID_CAsfHeaderObject[] = {0x3026b275, 0x8e66cf11, 0xa6d900aa, 0x0062ce6c};
	ID3_WMAGUID objectId;
	int num_bytes;
	int cbActual;

	if((fd < 0)||(Header==NULL))
	{
		return -ID3_ERROR_BAD_ARGUMENTS;
	}

	num_bytes = 30;	//fix
	cbActual = ID3_WMAGetData(fd, &pData, Header->currPacketOffset, num_bytes);
	if(cbActual == -1)
	{
		return -ID3_ERROR_FILEREAD;
	}

	if (cbActual != num_bytes)
	{
		return -ID3_ERROR_NOT_SUPPORT;
	}

	Header->currPacketOffset += cbActual;
	objectId.Data0 = ((unsigned long)pData[0]<<24) | ((unsigned long)pData[1]<<16) | ((unsigned long)pData[2]<<8) | ((unsigned long)pData[3]);
	objectId.Data1 = ((unsigned long)pData[4]<<24) | ((unsigned long)pData[4+1]<<16) | ((unsigned long)pData[4+2]<<8) | (unsigned long)pData[4+3];
	objectId.Data2 = ((unsigned long)pData[8]<<24) | ((unsigned long)pData[8+1]<<16) | ((unsigned long)pData[8+2]<<8) | (unsigned long)pData[8+3];
	objectId.Data3 = ((unsigned long)pData[12]<<24) | ((unsigned long)pData[12+1]<<16) | ((unsigned long)pData[12+2]<<8) | (unsigned long)pData[12+3];

	if ( (objectId.Data0 != GUID_CAsfHeaderObject[0]) ||
	        (objectId.Data1 != GUID_CAsfHeaderObject[1]) ||
	        (objectId.Data2 != GUID_CAsfHeaderObject[2]) ||
	        (objectId.Data3 != GUID_CAsfHeaderObject[3]) )
	{
		return -ID3_ERROR_NOT_SUPPORT;
	}

	Header->dwLo =((unsigned long)pData[19]<<24) | ((unsigned long)pData[18]<<16) | ((unsigned long)pData[17]<<8) | (unsigned long)pData[16];
	Header->dwHi = ((unsigned long)pData[23]<<24) | ((unsigned long)pData[22]<<16) | ((unsigned long)pData[21]<<8) | (unsigned long)pData[20];
	Header->dwObjNum = pData[24];
	return ID3_NO_ERROR;
}


/******************************************************************************
*
*	FUNCTION
*
*	int TCDB_META_WMALoadObjectHeader(int iHandle, META_WMAHeader *Header,
*									unsigned char *pData, META_WMAGUID *pObjectId)
*
*	Input	: NONE
*	Output	: NONE
*	Return	: NONE
*	Description : Initialize variables for copy.
*
******************************************************************************/
static int ID3_WMALoadObjectHeader(int fd, ID3_WMAHeader *Header, unsigned char *pData, ID3_WMAGUID *pObjectId)
{
	int num_bytes;
	int cbActual;

	if((fd < 0)||(Header==NULL)||(pObjectId==NULL))
	{
		return -ID3_ERROR_BAD_ARGUMENTS;
	}

	num_bytes =ID3_MIN_OBJECT_SIZE; 			//check!!
	cbActual = ID3_WMAGetData(fd, &pData, Header->currPacketOffset, num_bytes);
	if(cbActual == -1)
	{
		return -ID3_ERROR_FILEREAD;
	}
	if (cbActual != num_bytes)
	{
		return -ID3_ERROR_NOT_SUPPORT;
	}

	Header->currPacketOffset += cbActual;
	// extract ObjectID
	pObjectId->Data0 = ((unsigned long)pData[0]<<24) | ((unsigned long)pData[1]<<16) | ((unsigned long)pData[2]<<8) | (unsigned long)pData[3];
	pObjectId->Data1 = ((unsigned long)pData[4]<<24) | ((unsigned long)pData[4+1]<<16) | ((unsigned long)pData[4+2]<<8) | (unsigned long)pData[4+3];
	pObjectId->Data2 = ((unsigned long)pData[8]<<24) | ((unsigned long)pData[8+1]<<16) | ((unsigned long)pData[8+2]<<8) | (unsigned long)pData[8+3];
	pObjectId->Data3 = ((unsigned long)pData[12]<<24) | ((unsigned long)pData[12+1]<<16) | ((unsigned long)pData[12+2]<<8)| (unsigned long)pData[12+3];
	Header->dwLo = ((unsigned long)pData[19] << 24) | ((unsigned long)pData[18] << 16) | ((unsigned long)pData[17] << 8) | (unsigned long)pData[16];		//extract qwSize
	Header->dwHi = ((unsigned long)pData[23] << 24) | ((unsigned long)pData[22] << 16) | ((unsigned long)pData[21] << 8) | (unsigned long)pData[20];		//extract qwSize
	return -ID3_NO_ERROR;
}

static int ID3_WMALoadContentDescObject(int fd, ID3_WMAHeader *Header,
                                       unsigned char *pData, unsigned long cbSizeHi, unsigned long cbSizeLo,
                                       pstID3V2XInfoType pID3V2XInfo)
{
	unsigned long cbWanted;
	unsigned long cbActual;
	unsigned long long cbSize;		// The total size of extended description
	unsigned long long iCurrFileOffset;
	unsigned long long iMaxFileOffset;
	unsigned short ContentsSize[5];
	void * retMem;

	if((fd < 0)||(Header==NULL)||(pID3V2XInfo==NULL))
	{
		return -ID3_ERROR_BAD_ARGUMENTS;
	}

	cbSize = ((unsigned long long)cbSizeHi << 32) + cbSizeLo;
	//cbSize -= MIN_OBJECT_SIZE;	// MIN_OBJECT_SIZE = Extended header object ID(16Byte) + Object size(8Byte).
	iCurrFileOffset = Header->currPacketOffset;
	iMaxFileOffset = (iCurrFileOffset + cbSize) - ID3_MIN_OBJECT_SIZE;
	// Check if reading this next field pushes us past end of object
	cbWanted = 5*sizeof(unsigned short);

	if ((iCurrFileOffset + cbWanted) > iMaxFileOffset)
	{
		return -ID3_ERROR_BAD_ARGUMENTS;
	}

	cbActual =ID3_WMAGetData(fd, &pData, iCurrFileOffset, cbWanted);
	if(cbActual == 0xFFFFFFFFUL)
	{
		return -ID3_ERROR_FILEREAD;
	}
	if (cbActual != cbWanted)
	{
		return -ID3_ERROR_NOT_SUPPORT;
	}
	iCurrFileOffset += cbActual;

	/* jhpark 	2011.01.26	QAC  Rule 19.4		*/
	ID3_CDOLoadLength(&ContentsSize[0],&pData);
	ID3_CDOLoadLength(&ContentsSize[1],&pData);
	ID3_CDOLoadLength(&ContentsSize[2],&pData);
	ID3_CDOLoadLength(&ContentsSize[3],&pData);
	ID3_CDOLoadLength(&ContentsSize[4],&pData);

	//Title
	cbWanted = ContentsSize[0];
	if(cbWanted > (MAX_ID3V2_FIELD))
	{
		cbWanted = (MAX_ID3V2_FIELD);
	}
	cbActual = ID3_WMAGetData(fd, &pData, iCurrFileOffset, cbWanted);
	if(cbActual == 0xFFFFFFFFUL)
	{
		return -ID3_ERROR_FILEREAD;
	}
	if (cbActual != cbWanted)
	{
		return -ID3_ERROR_NOT_SUPPORT;
	}

	if ( cbActual > (MAX_ID3V2_FIELD/2) )
	{
		unsigned char CopyLen=0;

		CopyLen = (MAX_ID3V2_FIELD/2)-1;
		pID3V2XInfo->mMetaData[ID3_TITLE][0] = CopyLen;
		retMem= memcpy(&(pID3V2XInfo->mMetaData[ID3_TITLE][1]), pData,CopyLen*2);
	}
	else
	{
		pID3V2XInfo->mMetaData[ID3_TITLE][0] = (unsigned char)(cbActual >> 1);
		retMem= memcpy(&(pID3V2XInfo->mMetaData[ID3_TITLE][1]), pData, cbActual);
	}
	iCurrFileOffset += ContentsSize[0];

	//Author
	cbWanted = ContentsSize[1];
	if(cbWanted > (MAX_ID3V2_FIELD))
	{
		cbWanted = (MAX_ID3V2_FIELD);
	}
	cbActual = ID3_WMAGetData(fd, &pData, iCurrFileOffset, cbWanted);
	if(cbActual == 0xFFFFFFFFUL)
	{
		return -ID3_ERROR_FILEREAD;
	}

	if ((cbActual != cbWanted))
	{
		return -ID3_ERROR_NOT_SUPPORT;
	}

	if ( cbActual > (MAX_ID3V2_FIELD/2) )
	{
		unsigned char CopyLen=0;

		CopyLen = (MAX_ID3V2_FIELD/2)-1;
		pID3V2XInfo->mMetaData[ID3_ARTIST][0]= CopyLen;
		retMem= memcpy(&(pID3V2XInfo->mMetaData[ID3_ARTIST][1]), pData, CopyLen*2);
	}
	else
	{
		pID3V2XInfo->mMetaData[ID3_ARTIST][0]= (unsigned char )(cbActual >> 1);
		retMem= memcpy(&(pID3V2XInfo->mMetaData[ID3_ARTIST][1]), pData, cbActual);
	}

	iCurrFileOffset += ContentsSize[1];

#if defined (WMA_EXTEND_DATA_INCLUDE) && defined  (ID3_EXT_INCLUDE)
	//Copyright
	cbWanted = ContentsSize[2];
	if(cbWanted > (MAX_ID3V2_FIELD))
	{
		cbWanted = (MAX_ID3V2_FIELD);
	}
	cbActual = ID3_WMAGetData(fd, &pData, iCurrFileOffset, cbWanted);
	if(cbActual == 0xFFFFFFFFUL)
	{
		return -ID3_ERROR_FILEREAD;
	}

	if ((cbActual != cbWanted))
	{
		return -ID3_ERROR_NOT_SUPPORT;
	}

	if ( cbActual > (MAX_ID3V2_FIELD/2) )
	{
		unsigned char CopyLen=0;

		CopyLen = (MAX_ID3V2_FIELD/2)-1;
		pID3V2XInfo->mMetaData[ID3_COPYRIGHT][0] =CopyLen;
		retMem= memcpy(&(pID3V2XInfo->mMetaData[ID3_COPYRIGHT][1]), pData, CopyLen*2);
	}
	else
	{
		pID3V2XInfo->mMetaData[ID3_COPYRIGHT][0] = (unsigned char)(cbActual >> 1);
		retMem= memcpy(&(pID3V2XInfo->mMetaData[ID3_COPYRIGHT][1]), pData, cbActual);
	}
	iCurrFileOffset += ContentsSize[2];
	//Description
	cbWanted = ContentsSize[3];
	if(cbWanted > (MAX_ID3V2_FIELD))
	{
		cbWanted = (MAX_ID3V2_FIELD);
	}
	cbActual = ID3_WMAGetData(fd, &pData, iCurrFileOffset, cbWanted);
	if(cbActual == 0xFFFFFFFFUL)
	{
		return -ID3_ERROR_FILEREAD;
	}

	if (cbActual != cbWanted)
	{
		return -ID3_ERROR_NOT_SUPPORT;
	}

	if ( cbActual > (MAX_ID3V2_FIELD/2) )
	{
		unsigned char CopyLen=0;

		CopyLen = (MAX_ID3V2_FIELD/2)-1;
		pID3V2XInfo->mMetaData[ID3_COMMENT][0] = CopyLen;
		retMem= memcpy(&(pID3V2XInfo->mMetaData[ID3_COMMENT][1]), pData, CopyLen*2);
	}
	else
	{
		pID3V2XInfo->mMetaData[ID3_COMMENT][0] = (unsigned char)(cbActual >> 1);
		retMem= memcpy(&(pID3V2XInfo->mMetaData[ID3_COMMENT][1]), pData, cbActual);
	}
	iCurrFileOffset += ContentsSize[3];
	//Rating
	cbWanted = ContentsSize[4];
	if(cbWanted > (MAX_ID3V2_FIELD))
	{
		cbWanted = (MAX_ID3V2_FIELD);
	}
	cbActual =ID3_WMAGetData(fd, &pData, iCurrFileOffset, cbWanted);
	if(cbActual == 0xFFFFFFFFUL)
	{
		return -ID3_ERROR_FILEREAD;
	}

	if (cbActual != cbWanted)
	{
		return -ID3_ERROR_NOT_SUPPORT;
	}

	#if 0// not support rating tag, we will add rating tag.
	pMetaInfo->Rating[0] = (unsigned char)(cbActual >> 1);
	(void) memcpy(&pMetaInfo->Rating[1], pData, cbActual);
	iCurrFileOffset += ContentsSize[4];
	#endif
#endif
	// Move Offset
	Header->currPacketOffset += (cbSize-ID3_MIN_OBJECT_SIZE);
	return ID3_NO_ERROR;
}

/********************************************************************************************
Function : TCDB_META_WMALoadExtendedContentDescObject
Modfity : jhpark
date : 2011.01.27
description : QAC 적용
********************************************************************************************/
static int ID3_WMALoadExtendedContentDescObject(int fd, ID3_WMAHeader *Header,
        unsigned char *pData, unsigned long cbSizeHi, unsigned long cbSizeLo,
       pstID3V2XInfoType pID3V2XInfo)
{
	unsigned char ucSize;
	unsigned short i;
	int      iObjNum;
	int cbOverFlag=0;			// If wanted data is bigger than MAX_BUFSIZE, set 1.
	int iReturn =ID3_NO_ERROR;
	unsigned int uiMaxNameSize;
	unsigned int AttrCnt;
	int cbWanted;
	int cbActual;
	unsigned long cbOffset=0;
	unsigned long cbWanted1 =0;
	unsigned long cbObjectOffset = 0;
	unsigned long long cbSize;		// The total size of extended description

	if((Header==NULL)||(pID3V2XInfo==NULL))
	{
		return -ID3_ERROR_BAD_ARGUMENTS;
	}

	uiMaxNameSize = MAX_ID3V2_FIELD;
	cbSize = ((unsigned long long)cbSizeHi << 32) + cbSizeLo;
	cbSize -= ID3_MIN_OBJECT_SIZE;	// MIN_OBJECT_SIZE = Extended header object ID(16Byte) + Object size(8Byte).
	cbWanted = 2;
	cbActual = ID3_WMAGetData(fd, &pData, Header->currPacketOffset, cbWanted);

	if (cbActual != cbWanted)
	{
		iReturn = -ID3_ERROR_NOT_SUPPORT;
	}
	else
	{
		cbObjectOffset += cbActual;
		AttrCnt = ((unsigned int)pData[1]<<8) | (unsigned int)pData[0];	//	Get count of attribute.

		for ( i = 0; i < AttrCnt; i++)
		{
			cbOverFlag = 0;
			cbWanted = 2;
			cbActual = ID3_WMAGetData(fd, &pData, Header->currPacketOffset + cbObjectOffset, cbWanted);
			if(cbActual == -1)
			{
				return -ID3_ERROR_FILEREAD;
			}
			if (cbActual != cbWanted)
			{
				return -ID3_ERROR_NOT_SUPPORT;
			}
			else
			{
				cbObjectOffset += cbActual;
				cbWanted = ((unsigned long)pData[1] << 8) | (unsigned long)pData[0];

				if (cbWanted > (int)uiMaxNameSize)
				{
					cbOffset=0;
					do
					{
						cbWanted1 = (cbWanted > (int)uiMaxNameSize) ? uiMaxNameSize : (unsigned long)cbWanted;
						cbActual =ID3_WMAGetData(fd, &pData, Header->currPacketOffset + cbObjectOffset, cbWanted);
						if(cbActual == -1)
						{
							return -ID3_ERROR_FILEREAD;
						}

						if ((unsigned long)cbActual != cbWanted1)
						{
							iReturn =-ID3_ERROR_NOT_SUPPORT;
							break;
						}

						cbObjectOffset += cbActual;
						cbWanted -=cbActual;
						cbOffset +=cbActual;
					}
					while (cbWanted >0);
				}
				else
				{
					cbOverFlag = 0;
					cbActual = ID3_WMAGetData(fd, &pData, Header->currPacketOffset + cbObjectOffset, cbWanted);
					if(cbActual == -1)
					{
						return -ID3_ERROR_FILEREAD;
					}

					if (cbActual != cbWanted)
					{
						iReturn =-ID3_ERROR_NOT_SUPPORT;
					}
					else
					{
						cbObjectOffset += cbActual +2;	//cbActual + data type
						iObjNum = ID3_WMAChkExtObjectAttr( pData, cbWanted );
						cbWanted = 2;
						cbActual = ID3_WMAGetData(fd, &pData, Header->currPacketOffset + cbObjectOffset, cbWanted);		//get length
						if(cbActual == -1)
						{
							return -ID3_ERROR_FILEREAD;
						}
						if (cbActual != cbWanted)
						{
							iReturn =-ID3_ERROR_NOT_SUPPORT;
						}
						else
						{
							cbObjectOffset += cbActual;
							cbWanted = ((unsigned long)pData[1] << 8) | (unsigned long)pData[0];
							if ( iObjNum == -1 )
							{
								// Ignore indicated datas.
								cbObjectOffset += cbWanted;
								//continue;
							}
							else
							{
								// For protecting from a TAG over 128 characters.
								cbActual = ID3_WMAGetData(fd, &pData, Header->currPacketOffset + cbObjectOffset
								                                ,((unsigned int)cbWanted > uiMaxNameSize) ? uiMaxNameSize : (unsigned int)cbWanted); //get data
								if(cbActual == -1)
								{
									return -ID3_ERROR_FILEREAD;
								}
#if 0
								if ((cbWanted > uiMaxNameSize) && (cbActual==uiMaxNameSize)) // For checking the boundary of a tag
								{
									cbActual=cbWanted;
								}
#endif
								if (cbActual != cbWanted)
								{
									iReturn =-ID3_ERROR_NOT_SUPPORT;	// insufficient data...
								}
								else
								{
									cbObjectOffset += cbActual; 		// for setting the next offset.
									if (cbActual > (int)(uiMaxNameSize-2))	//Max : 255 character
									{
										cbWanted = uiMaxNameSize-2;
										pData[uiMaxNameSize-4] = 0x00;
										pData[uiMaxNameSize-3] = 0x00;
									}
									ucSize = cbWanted >> 1;
									if(cbWanted < 0)
									{
										iReturn = - ID3_ERROR_Pasing;
									}
									else
									{
										switch ( iObjNum )
										{
											case	ID3_WMA_AlbumTitle :
												(void) memcpy( &(pID3V2XInfo->mMetaData[ID3_ALBUM][0]) , &ucSize, sizeof(unsigned char));
												(void) memcpy( &(pID3V2XInfo->mMetaData[ID3_ALBUM][1]), pData, cbWanted );
												break;
											case	ID3_WMA_AlbumArtist :
												(void) memcpy( &(pID3V2XInfo->mMetaData[ID3_ARTIST][0]), &ucSize, sizeof(unsigned char));
												(void) memcpy( &(pID3V2XInfo->mMetaData[ID3_ARTIST][1]),pData, cbWanted );
												break;
											case	ID3_WMA_Genre :
												(void) memcpy( &(pID3V2XInfo->mMetaData[ID3_GENRE][0]), &ucSize, sizeof(unsigned char));
												(void) memcpy( &(pID3V2XInfo->mMetaData[ID3_GENRE][1]), pData, cbWanted );
												break;
#if defined ( WMA_EXTEND_DATA_INCLUDE)
											case ID3_WMA_Year :
												// TODO: To copy Track number data
												//cbWanted = (cbWanted > MAX_ID3V2_YEAR) ? MAX_ID3V2_YEAR : cbWanted;	//MIN(cbWanted, MAX_ID3V2_YEAR);
												//TCDB_mem_cpy( &pMetaInfo->mYear[0], &cbWanted, sizeof(unsigned char));
												//TCDB_mem_cpy( &pMetaInfo->mYear[1], pData, cbWanted );
												break;
											case ID3_WMA_TrackNo :
												// TODO: To copy Track number data
												break;
#endif
											case ID3_WMA_Picture :
												// TODO: To copy picture data
												break;
											default :
												break;
										}
									}
								}
							}
						}
					}
				}

			}

			if(iReturn < ID3_NO_ERROR)
			{
				break;
			}
		}

		// use all
		if(iReturn == ID3_NO_ERROR)
		{
			Header->currPacketOffset += cbSize;
		}

	}

	return iReturn;
}


/********************************************************************************************
Function : TCDB_META_WMAChkExtObjectAttr
Modfity : jhpark
date : 2011.01.27
description : QAC 적용
********************************************************************************************/
static int ID3_WMAChkExtObjectAttr ( unsigned char *pData, int size )
{
	int iObjCnt;
	int iReturn =-1;
	static const unsigned char WM_AlbumTitle[]={ 0x57, 0x00, 0x4D, 0x00, 0x2F, 0x00,
									0x41, 0x00, 0x6C, 0x00,0x62, 0x00, 0x75, 0x00, 0x6D, 0x00,
									0x54, 0x00, 0x69, 0x00,0x74, 0x00, 0x6c, 0x00,0x65, 0x00, 0x00, 0x00
                                    }; // "WM/AlbumTitle"
	static const unsigned char WM_AlbumArtist[]={ 0x57, 0x00, 0x4D, 0x00, 0x2F, 0x00, 0x41, 0x00, 0x6C, 0x00,
                                       0x62, 0x00, 0x75, 0x00, 0x6D, 0x00, 0x41, 0x00, 0x72, 0x00,
                                       0x74, 0x00, 0x69, 0x00, 0x73, 0x00, 0x74, 0x00, 0x00, 0x00
                                     }; // "WM/AlbumArtist"

	 static const unsigned char WM_Picture[]= {0x57, 0x00, 0x4D, 0x00, 0x2F, 0x00, 0x50, 0x00, 0x69, 0x00,
                                   0x63, 0x00, 0x74, 0x00, 0x75, 0x00, 0x72, 0x00, 0x65, 0x00, 0x00, 0x00
                                  };

	 static const unsigned char WM_Genre[]={ 0x57, 0x00, 0x4D, 0x00, 0x2F, 0x00, 0x47, 0x00, 0x65, 0x00,
                                 0x6E, 0x00, 0x72, 0x00, 0x65, 0x00, 0x00, 0x00
                               };	// "WM/Genre"

	/*jhpark 	2011.01.25	QAC Rule8.7 */
	static ID3_WM_ExtContAttr stWM_ExtContAttr[ID3_WMA_MAX] =
	{
		{sizeof(WM_AlbumTitle),	(unsigned char *)WM_AlbumTitle},
		{sizeof(WM_AlbumArtist),	(unsigned char *)WM_AlbumArtist},
		{sizeof(WM_Genre),		(unsigned char *)WM_Genre},
		{sizeof(WM_Picture),		(unsigned char *)WM_Picture}
	};

	if((pData !=NULL)&&(size >0))
	{
		for (iObjCnt = 0; iObjCnt < ID3_WMA_MAX; iObjCnt++)
		{
			if ( (unsigned int)size == stWM_ExtContAttr[iObjCnt].uiNameSize )
			{
				if (!memcmp(pData, (void*)stWM_ExtContAttr[iObjCnt].pWM_NAME, size))
				{
					iReturn = iObjCnt;
					break;
				}
			}
		}
	}
	return iReturn;
}

/**************************************************************************************************
function : TCDB_META_CDOLoadLength
Jhpark
description :	QAC Rule 19.4 적용
			#define CDOLoadLength( w, p )	(w) = ((unsigned short)*(p+1) << 8) + *p; (p) += sizeof(unsigned short);
			을 함수로 변경

**************************************************************************************************/
static void ID3_CDOLoadLength(unsigned short *usContentSize, unsigned char **pData)
{
	if(usContentSize !=0X00)
	{
		*usContentSize  =(((unsigned short)*(*pData+sizeof(unsigned char))) <<8 )+ (**pData);
		*pData = *pData +sizeof(unsigned short);
	}
}

/**************************************************************
*
*	FUNCTION
*
*	int WMA_LoadStreamPropertiesObjectTCC(struct tWMA_HdrTCC *Header,
*										 unsigned long cbSize)
*
*	Input	: NONE
*	Output	: NONE
*	Return	: NONE
*	Description : Extract the Contents Data
*
***************************************************************/
static int ID3_WMALoadStreamPropertiesObject(int fd, ID3_WMAHeader *Header, unsigned char *pData, unsigned long cbSize)
{
	const unsigned long GUID_AsfXStreamTypeAcmAudio[]= {0x409e69f8, 0x4d5bcf11, 0xa8fd0080, 0x5f5c442b};
	const unsigned long GUID_AsfXStreamTypeAcmVideo[]= {0xc0ef19bc, 0x4d5bcf11, 0xa8fd0080, 0x5f5c442b};
	int num_bytes;
	int cbActual;
	ID3_WMAGUID objectId;

	if((fd < 0 )||(Header==NULL))
	{
		return -ID3_ERROR_BAD_ARGUMENTS;
	}

	cbSize -= ID3_MIN_OBJECT_SIZE;
	num_bytes = 64;	//read 64 bytes
	if ((unsigned long)num_bytes > cbSize)
	{
		return -ID3_ERROR_INVALIDSIZE_OBJECT;
	}

	cbActual = ID3_WMAGetData(fd, &pData, Header->currPacketOffset, num_bytes);
	if(cbActual  == -1)
	{
		return -ID3_ERROR_FILEREAD;
	}
	if (cbActual != num_bytes)
	{
		return -ID3_ERROR_NOT_SUPPORT;
	}


	objectId.Data0 = ((unsigned long)pData[0]<<24) | ((unsigned long)pData[1]<<16) | ((unsigned long)pData[2]<<8) | (unsigned long)pData[3];
	objectId.Data1 = ((unsigned long)pData[4]<<24) | ((unsigned long)pData[4+1]<<16) | ((unsigned long)pData[4+2]<<8) | (unsigned long)pData[4+3];
	objectId.Data2 = ((unsigned long)pData[8]<<24) | ((unsigned long)pData[8+1]<<16) | ((unsigned long)pData[8+2]<<8 )| (unsigned long)pData[8+3];
	objectId.Data3 = ((unsigned long)pData[12]<<2) | ((unsigned long)pData[12+1]<<16) | ((unsigned long)pData[12+2]<<8) | (unsigned long)pData[12+3];

	if ( (objectId.Data0 != GUID_AsfXStreamTypeAcmAudio[0]) ||
	        (objectId.Data1 != GUID_AsfXStreamTypeAcmAudio[1]) ||
	        (objectId.Data2 != GUID_AsfXStreamTypeAcmAudio[2]) ||
	        (objectId.Data3 != GUID_AsfXStreamTypeAcmAudio[3] ))
	{
		// we may skip video stream property.
		if ( (objectId.Data0 != GUID_AsfXStreamTypeAcmVideo[0]) ||
		        (objectId.Data1 != GUID_AsfXStreamTypeAcmVideo[1]) ||
		        (objectId.Data2 != GUID_AsfXStreamTypeAcmVideo[2]) ||
		        (objectId.Data3 != GUID_AsfXStreamTypeAcmVideo[3] ))
		{
			return -ID3_ERROR_INVALID_HEADER;
		}
	}

	Header->wFormatTag = ((unsigned short)pData[54+1] << 8) | (unsigned short)pData[54];
	Header->samplingfreq = ((unsigned long)pData[58+3]<<24) | ((unsigned long)pData[58+2]<<16) | ((unsigned long)pData[58+1]<<8) | (unsigned long)pData[58];
	Header->avgbitrate=   ((unsigned long)pData[62+1]<<8) | ((unsigned long)pData[62] << 3);
	Header->currPacketOffset += cbSize;
	return ID3_NO_ERROR;
}

static int ID3_WMAGetData(int fd,  unsigned char **pData, unsigned long ulOffset, unsigned int uiNumBytes)
{
	int iRet;

	if((fd < 0)||(pData==NULL))
	{
		return -ID3_ERROR_BAD_ARGUMENTS;
	}
#ifdef ID3_WMA_PRE_READ_BUFFER
	if (ID3_WMA_HeaderEndOffset <(int)(ulOffset + uiNumBytes))
	{
		iRet = lseek (fd, (int)ulOffset, SEEK_SET);
		if (iRet != (int)ulOffset)
		{
			fprintf(stderr,"%s : file seek fail (%d)\n",__FUNCTION__, iRet);
			return -1;	/* Error */
		}

		iRet = read(fd, ID3_WMA_HeaderBuffer, ID3_WMA_HEADER_SIZE);

		ID3_WMA_HeaderStartOffset = ulOffset;
		ID3_WMA_HeaderEndOffset = ulOffset +ID3_WMA_HEADER_SIZE;
		*pData = ID3_WMA_HeaderBuffer;
		return uiNumBytes;
	}
	else
	{
		*pData = &ID3_WMA_HeaderBuffer[ulOffset - ID3_WMA_HeaderStartOffset];
		return uiNumBytes;
	}
#else

	iRet = lseek(fd, (int)ulOffset, SEEK_SET);
	if (iRet != ulOffset)
	{
		fprintf(stderr,"%s : file seek fail (%d)\n",__FUNCTION__, iRet);
		 iRet =-1;	/* Error */
	}
	else
	{
		iRet = read(fd, (void *)*pData, uiNumBytes);
	}
	return iRet;
#endif
}

long ID3_GetWMAData(int fd, pstID3V2XInfoType pID3V2XInfo)
{
	unsigned char *pData=NULL;
#ifndef ID3_WMA_PRE_READ_BUFFER
	unsigned char ID3MetaBuff[MAX_ID3_FIND_SIZE];
#endif
	int iResult;
	ID3_WMAHeader Header;
	stID3V2XInfoType ID3V2XInfoUTF16;
	int iFileSize;
	int ret;
	struct stat sb;

	if ((fd < 0 )||(pID3V2XInfo==NULL))
	{
		return -ID3_ERROR_BAD_ARGUMENTS;		// wrong file handle
	}

	ret = lseek (fd, 0, SEEK_END);
	if(ret < 0)
	{
		ret = -ID3_ERROR_FILEREAD;
		fprintf(stderr, "seek error : offset (%d) \n",ret );
		return ret;
	}

	if(fstat(fd, &sb)== -1)
	{
		fprintf(stderr, "fstat error \n");
		return -1;
	}

	iFileSize= sb.st_size;

	if(iFileSize <30 )// if file size is smaller than 10byte, it is invalid file size to parsing id3 tag,
	{
		return -ID3_ERROR_INVALID_FILE_SIZE;// do not process id3 meta gathering
	}

	(void) memset(&Header, 0x00, sizeof(ID3_WMAHeader));
#ifdef ID3_WMA_PRE_READ_BUFFER
	ID3_WMA_HeaderStartOffset = 0;
	ID3_WMA_HeaderEndOffset = 0;
	pData = NULL;
#else
	pData = ID3MetaBuff;
#endif
	iResult = ID3_WMAParseAsfHeader(fd, &Header, pData, &ID3V2XInfoUTF16);
	if(iResult == ID3_NO_ERROR)
	{
		int i=0;

		pID3V2XInfo->mUnSyncFlag = ID3V2XInfoUTF16.mUnSyncFlag;
		pID3V2XInfo->mTrack = ID3V2XInfoUTF16.mTrack;
		memcpy(pID3V2XInfo->mYear, ID3V2XInfoUTF16.mYear, MAX_ID3V2_YEAR);
		for(i=0; i <ID3_NONINFO; i++ )
		{
			if(ID3V2XInfoUTF16.mMetaData[i][0] != 0x00)
			{
				int utfRet=0;
				utfRet = Unicode2UTF8(&ID3V2XInfoUTF16.mMetaData[i][ID3_UNICODE_START],
					&pID3V2XInfo->mMetaData[i][ID3_UNICODE_START],
					(ID3V2XInfoUTF16.mMetaData[i][0]*2)+1,
					MAX_ID3V2_FIELD);

				if(utfRet > 0)
				{
					pID3V2XInfo->mMetaData[i][0] = utfRet;
				}
				else
				{
					pID3V2XInfo->mMetaData[i][0] = 0;
				}
			}
		}
	}
	else
	{
		fprintf(stderr, "[%s] : WMA parsing error (%d) \n", __FUNCTION__, iResult);
	}
	return iResult;

}

#endif  /* WMA_INCLUDE */


