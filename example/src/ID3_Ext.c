/****************************************************************************************
 *   FileName    : ID3_Ext.c
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

/****************************************************************************

  Revision History

 ****************************************************************************

 [1065] : PROFILE_GetMetaData(), execute TC_Close() when file size is less than 1bytes.
 [1277] : PROFILE_GetMetaData(), Support for StillImage.

****************************************************************************/

#include "Config/SDK/config.h"
#include "Common/BufferCtrl/PCM_buffer.h"
#include "SERVICE/Codec/pCODECS.h"
#include "DRV/FS/disk.h"
#include "Common/globals.h"

#ifdef MTP_INCLUDE
#ifdef AUDIBLE_INCLUDE
#include "OAAProfileFile.h"
#endif
#include "MTP.H"
#include "Id3.h"
#include "OS/TC_Kernel.h" /* Header File : Misra-C Rule 19.15 */
#include "DRV/FS/file.h" /* Header File : Misra-C Rule 19.15 */
#include "DRV/FS/FSAPP.h"
#include "SERVICE/browse/browse.h"
#ifdef THUMBNAILDB_INCLUDE		/* 07.12.13 */
#include "tcdb_thumbnail.h"
#endif
#ifdef ENHANCED_THUMBNAILDB_INCLUDE
#include "SERVICE/Codec/JPEGDEC/pAlbumArt.h"
#endif

#ifdef TRUSTED_FLASH_INCLUDE
#include "TF_Profile.h"
#endif

extern _MTP_OBJECT_TYPE		stMTPFileDB;

#ifdef WMA_INCLUDE
// ASF Format Signature
const unsigned char ASF_Header_Object[] =
{
	0x30,0x26,0xb2,0x75,
	0x8e,0x66,0xcf,0x11,
	0xa6,0xd9,0x00,0xaa,
	0x00,0x62,0xce,0x6c
};

// ASF_Content_Description_Object
const unsigned char ASF_Content_Description_Object[] =
{
	0x33,0x26,0xb2,0x75,
	0x8e,0x66,0xcf,0x11,
	0xa6,0xd9,0x00,0xaa,
	0x00,0x62,0xce,0x6c
};

// Signature to get Time Duration
const unsigned char ASF_File_Properties_Object[] =
{
	0xa1,0xdc,0xab,0x8c,
	0x47,0xa9,0xcf,0x11,
	0x8e,0xe4,0x00,0xc0,
	0x0c,0x20,0x53,0x65
};

// ASF_Extended_Content_Description_Object
const unsigned char ASF_Extended_Content_Description_Object[] =
{
	0x40,0xA4,0xD0,0xD2,
	0x07,0xE3,0xD2,0x11,
	0x97,0xF0,0x00,0xA0,
	0xC9,0x5E,0xA8,0x50
};

const unsigned char ContentDescriptorID3TAG[]	=
{
	'I',0x00,
	'D',0x00,
	'3',0x00,
	0x00,0x00,
};
const unsigned char ContentDescriptorAlbum[] =
{
	'W',0x00,
	'M',0x00,
	'/',0x00,
	'A',0x00,
	'l',0x00,
	'b',0x00,
	'u',0x00,
	'm',0x00,
	'T',0x00,
	'i',0x00,
	't',0x00,
	'l',0x00,
	'e',0x00,
	0x00,0x00,
};

const unsigned char ContentDescriptorGenre[] =
{
	'W',0x00,
	'M',0x00,
	'/',0x00,
	'G',0x00,
	'e',0x00,
	'n',0x00,
	'r',0x00,
	'e',0x00,
//	'I',0x00,
//	'D',0x00,
	0x00,0x00,
};

const unsigned char ContentDescriptorTrack[] =
{
	'W',0x00,
	'M',0x00,
	'/',0x00,
	'T',0x00,
	'r',0x00,
	'a',0x00,
	'c',0x00,
	'k',0x00,
	0x00,0x00,
};

const unsigned char ContentDescriptorYear[] =
{
	'W',0x00,
	'M',0x00,
	'/',0x00,
	'Y',0x00,
	'e',0x00,
	'a',0x00,
	'r',0x00,
	0x00,0x00,
};

const unsigned char ContentDescriptorAlbumArtist[] =
{
	'W',0x00,
	'M',0x00,
	'/',0x00,
	'A',0x00,
	'l',0x00,
	'b',0x00,
	'u',0x00,
	'm',0x00,
	'A',0x00,
	'r',0x00,
	't',0x00,
	'i',0x00,
	's',0x00,
	't',0x00,
	0x00,0x00,
};

const unsigned char ContentDescriptorPicture[] =
{
	'W',0x00,
	'M',0x00,
	'/',0x00,
	'P',0x00,
	'i',0x00,
	'c',0x00,
	't',0x00,
	'u',0x00,
	'r',0x00,
	'e',0x00,
	0x00,0x00,
};

ASFContentDescriptorType ASFContentDescriptor[] =
{
	{sizeof(ContentDescriptorID3TAG),WMA_DESCRIPTOR_ID3TAG, ContentDescriptorID3TAG},
	{sizeof(ContentDescriptorAlbum),WMA_DESCRIPTOR_ALBUM, ContentDescriptorAlbum},
	{sizeof(ContentDescriptorGenre),WMA_DESCRIPTOR_GENRE, ContentDescriptorGenre},
	{sizeof(ContentDescriptorTrack),WMA_DESCRIPTOR_TRACK, ContentDescriptorTrack},
	{sizeof(ContentDescriptorYear),WMA_DESCRIPTOR_YEAR, ContentDescriptorYear},
	{sizeof(ContentDescriptorAlbumArtist),WMA_DESCRIPTOR_ALBUMARTIST, ContentDescriptorAlbumArtist},
	{sizeof(ContentDescriptorPicture),WMA_DESCRIPTOR_PICTURE, ContentDescriptorPicture},
};
#endif

#ifdef THUMBNAILDB_INCLUDE		/* 07.12.13 */
extern _ALBUMART_PICTURE_TYPE   ALBUMART_JPEG;

// ASF_MetaData_Library_Object
const unsigned char ASF_MetaData_Library_Object[] =
{
	0x94,0x1C,0x23,0x44,
	0x98,0x94,0xD1,0x49,
	0xA1,0x41,0x1D,0x13,
	0x4E,0x45,0x70,0x54
};

int PROFILE_GetASFMetaDataLibraryObject(char *tempbuf,  void *pID3Info);
int PROFILE_GetWMAPictureData(int iDataSize,char *SrcBuf,unsigned int Start_Pos, _ALBUMART_PICTURE_TYPE	*ALBUMART_JPEG);
#endif /* THUMBNAILDB_INCLUDE */

///////////////////////////////////////////////////////////////////////////////////
//
// Function    : PROFILE_GetMP3HeaderInfo
//
// Description : Get the MP3 Header Info
//
///////////////////////////////////////////////////////////////////////////////////
int PROFILE_SearchTwoHeader(int fHandle,unsigned char* pucBuffer, unsigned long ulInputHeaderInfo)
{
	unsigned char* InputBuffer=0;
	unsigned short searchcount = 1440;
	unsigned char returnvalue=0;
	unsigned long ulNextHeader=0;
	unsigned long ulSumOfBitrate=0;
	unsigned long temp=0;
	unsigned long ulCheckHeaderInfo;
	int result;
	InputBuffer	= pucBuffer;

	while (searchcount)
	{
		returnvalue	= CheckValidHeader(InputBuffer);

		if (returnvalue)
		{
			ulCheckHeaderInfo = GetHeaderInfo(InputBuffer);

			if ((ulCheckHeaderInfo & 0x001E0C00) == (ulInputHeaderInfo & 0x001E0C00))
			{
				// get next	header position
				ulNextHeader	= GetFrameSize(InputBuffer);
				// save	first bitrate
				ulSumOfBitrate	= GetBitrate(InputBuffer);
				// increase	header position
				temp = (int)InputBuffer	+ ulNextHeader - (int)pucBuffer;

				if (	temp >=	2044)
				{
					InputBuffer	= pucBuffer;
					memcpy(InputBuffer,	InputBuffer+2044, 4);
					result = fread(fHandle,InputBuffer+4, 0x600);
					InputBuffer	+= temp	- 2044;
				}
				else
				{
					InputBuffer	+= ulNextHeader;
				}

				// check next header
				returnvalue		= CheckValidHeader(InputBuffer);

				if (returnvalue)
				{
					ulSumOfBitrate += GetBitrate(InputBuffer);
					return ulSumOfBitrate;
				}
			}
		}

		searchcount--;
		InputBuffer++;
	}

	return 0;
}


///////////////////////////////////////////////////////////////////////////////////
//
// Function    : PROFILE_FillVBRInfo
//
// Description : Fill the MP3 Header Info of VBR file by Xing and VBRI
//
///////////////////////////////////////////////////////////////////////////////////
void PROFILE_FillVBRInfo(char *MP3Data,int iFileSize,unsigned int firstheaderposition,
                         unsigned long  ulFramesize,unsigned long  ulTotalframes, _MTP_OBJECT_TYPE *pstMTPFileDB)
{
	int iRemainSize;
	pstMTPFileDB->OBJProp.AudioObj.AudioProp.NumerOfChannel = GetChannels(MP3Data);
	pstMTPFileDB->OBJProp.AudioObj.AudioProp.SampleRating   = GetSamplingRate(MP3Data);
	iRemainSize					 = (unsigned long)(iFileSize - firstheaderposition);

	if (ulTotalframes)
	{
		pstMTPFileDB->OBJProp.AudioObj.AudioProp.AudioBitRate   = ((iRemainSize / ulTotalframes)	*
		        pstMTPFileDB->OBJProp.AudioObj.AudioProp.SampleRating)	/ (ulFramesize / 8);
		pstMTPFileDB->OBJProp.AudioObj.AudioProp.Duration 		= (((iRemainSize * 8) / pstMTPFileDB->OBJProp.AudioObj.AudioProp.AudioBitRate) * 1000) +
		        ((((iRemainSize * 8) % pstMTPFileDB->OBJProp.AudioObj.AudioProp.AudioBitRate) * 1000) /
		         pstMTPFileDB->OBJProp.AudioObj.AudioProp.AudioBitRate);
	}
}

///////////////////////////////////////////////////////////////////////////////////
//
// Function    : PROFILE_FillMP3Info
//
// Description : Fill the MP3 Header Info
//
///////////////////////////////////////////////////////////////////////////////////
void PROFILE_FillMP3Info(char *MP3Data,int iFileSize,unsigned int firstheaderposition,unsigned long  ulFramesize,
                         unsigned long  ulFirstBitrate,unsigned long  ulAverageBitrate, _MTP_OBJECT_TYPE *pstMTPFileDB)
{
	int iRemainSize;
	pstMTPFileDB->OBJProp.AudioObj.AudioProp.NumerOfChannel = GetChannels(MP3Data);
	pstMTPFileDB->OBJProp.AudioObj.AudioProp.SampleRating   = GetSamplingRate(MP3Data);
	iRemainSize					 = (unsigned long)(iFileSize - firstheaderposition);

	if (ulFirstBitrate != ulAverageBitrate)
	{
		pstMTPFileDB->OBJProp.AudioObj.AudioProp.AudioBitRate	= ulAverageBitrate * 1000;
	}
	else
	{
		pstMTPFileDB->OBJProp.AudioObj.AudioProp.AudioBitRate	= ulFirstBitrate * 1000;
	}

	if (pstMTPFileDB->OBJProp.AudioObj.AudioProp.AudioBitRate)
	{
		pstMTPFileDB->OBJProp.AudioObj.AudioProp.Duration 		= (((iRemainSize * 8) / pstMTPFileDB->OBJProp.AudioObj.AudioProp.AudioBitRate) * 1000) +
		        ((((iRemainSize * 8) % pstMTPFileDB->OBJProp.AudioObj.AudioProp.AudioBitRate) * 1000) /
		         pstMTPFileDB->OBJProp.AudioObj.AudioProp.AudioBitRate);
	}
}
///////////////////////////////////////////////////////////////////////////////////
//
// Function    : PROFILE_GetMP3HeaderInfo
//
// Description : Get the MP3 Header Info
//
///////////////////////////////////////////////////////////////////////////////////
int	PROFILE_GetMP3HeaderInfo(int fHandle, _MTP_OBJECT_TYPE *pstMTPFileDB)
{
	unsigned char* MP3Data;
	unsigned char* BufferStart;
	unsigned int   firstheaderposition = 0;
	unsigned int   searchcount = 70*1024;//1000;
	unsigned long  ulFirstHeaderInfo;
	unsigned long  ulSecondHeaderInfo;
	unsigned long  ulNextHeader;
	unsigned long  ulFramesize;
	unsigned long  ulTotalframes;
	unsigned long  ulFirstBitrate;
	unsigned long  temp;
	unsigned char  returnvalue;
	unsigned char  ulBitrateCount = 0;
	unsigned short comparecount	= 0;
	unsigned long  ulAverageBitrate;
	unsigned int   iFileSize;
//	BufferStart	= (unsigned	char *)ulEndOfRAM;
	BufferStart	= (unsigned	char *)FSAPP_GetFileBuffer();
	MP3Data		= BufferStart;
	(void) fseek(fHandle,      0,SEEK_SET);
	(void) fread(fHandle,MP3Data,     MAX_ID3_FIND_SIZE);
	iFileSize = TC_Length(fHandle);
	// Check ID3 V2
	//////////////////////////////////////////////////////////////
	firstheaderposition	= CheckID3V2Tag(MP3Data);

	if (firstheaderposition)
	{
		firstheaderposition	+= 10;

		if (	firstheaderposition	> 0xC00	)
		{
			(void) fseek(fHandle,firstheaderposition&~511,SEEK_SET);
			(void) fread(fHandle,MP3Data,     MAX_ID3_FIND_SIZE);
			MP3Data	+= firstheaderposition & 511;
		}
		else
		{
			if ( firstheaderposition > 0x600 )
			{
				memcpy(MP3Data,	MP3Data+0x600, 0x200);
				(void) fread(fHandle,MP3Data+0x200, 0x600);
				MP3Data	+= firstheaderposition - 0x600;
			}
			else
			{
				MP3Data	+= firstheaderposition;
			}
		}
	}

	while (searchcount)
	{
		returnvalue	= CheckValidHeader(MP3Data);

		if (returnvalue)
		{
			// Check Xing &	VBRI
			//////////////////////////////////////////////////////////////
			ulTotalframes =	DecodeVBRHeader(MP3Data, &ulFramesize);

			if (ulTotalframes)
			{
				PROFILE_FillVBRInfo(MP3Data,iFileSize,firstheaderposition,\
				                    ulFramesize,ulTotalframes,pstMTPFileDB);
				return 1;
			}

			// Get next	header pointer
			//////////////////////////////////////////////////////////////
			ulFirstHeaderInfo	= GetHeaderInfo(MP3Data); // get first header raw data
			ulNextHeader		= GetFrameSize(MP3Data);  // calculate next header position
			ulFirstBitrate		= GetBitrate(MP3Data);	  // get first bitrate
			// Jump	to next	header
			//////////////////////////////////////////////////////////////
			temp = (int)MP3Data	+ ulNextHeader - (int)BufferStart;

			if (	temp >=	2044)
			{
				MP3Data	= BufferStart;
				memcpy(MP3Data,	MP3Data+2044, 4);
				(void) fread(fHandle,MP3Data+4, 0x600);
				MP3Data	+= temp	- 2044;
			}
			else
			{
				MP3Data	+= ulNextHeader;
			}

			// check next header
			returnvalue	= CheckValidHeader(MP3Data);

			if (returnvalue)
			{
				unsigned long ulFirstFrame;
				unsigned long ulLength;
				// get second header raw data
				ulSecondHeaderInfo = GetHeaderInfo(MP3Data);

				// compare previous	and	current	header
				if (	(ulFirstHeaderInfo & 0x001E0D03) ==	(ulSecondHeaderInfo	& 0x001E0D03) )
				{
					// get header information
					ulFirstFrame	= firstheaderposition;
					ulLength		= (int)(iFileSize - ulFirstFrame);
					// check 20	header's bitrate ===================================================
					ulAverageBitrate = 0;

					for (comparecount = 0; comparecount<20; comparecount++)
					{
						(void) fseek(fHandle, (int)(ulFirstFrame + ((ulLength * (10 + comparecount))/40)&~511),0);
						(void) fread(fHandle,MP3Data, 2048);
						// get header value
						temp = PROFILE_SearchTwoHeader(fHandle,MP3Data, ulFirstHeaderInfo);

						if (temp)
						{
							ulAverageBitrate +=	temp;
							ulBitrateCount +=2;
						}
					}

					if (ulBitrateCount)
					{
						ulAverageBitrate = ulAverageBitrate/ulBitrateCount;
					}

					PROFILE_FillMP3Info(MP3Data,iFileSize,firstheaderposition,\
					                    ulFramesize,ulFirstBitrate,ulAverageBitrate,pstMTPFileDB);
					return 1;
				}
			}

			// The first searched header is	not	valid header
			// MP3Data pointer back	to first header
			MP3Data	= BufferStart;
			(void) fseek(fHandle,firstheaderposition & ~511, 0);
			(void) fread(fHandle,MP3Data, MAX_ID3_FIND_SIZE);
			MP3Data	+= firstheaderposition & 511;
		}

		// increase	pointer	and	another	variables =====================================
		searchcount--;

		// If buffer empty,	read 512byte from mp3 file
		if (	(int)MP3Data - (int)BufferStart	>= 0x600)
		{
			MP3Data	= BufferStart;
			memcpy(MP3Data,	MP3Data+0x600, 0x200);
			(void) fread(fHandle,MP3Data+0x200,0x600);
		}

		// else, update	pointer
		firstheaderposition++;
		MP3Data++;
		//=============================================================================
	}

	PROFILE_FillMP3Info(MP3Data,iFileSize,firstheaderposition,\
	                    ulFramesize,ulFirstBitrate,ulAverageBitrate,pstMTPFileDB);
	return 0;
}


#ifdef WMA_INCLUDE

///////////////////////////////////////////////////////////////////////////////////
//
// Function    : PROFILE_FillWMAMetaData
//
// Description :
//
///////////////////////////////////////////////////////////////////////////////////
int PROFILE_FillWMAMetaData(int iDataSize,char *SrcBuf,char *TrgBuf)
{
	int iSize;

	//if(iDataSize && SrcBuf!=NULL && TrgBuf != NULL)
	if (iDataSize > 2 && SrcBuf!=NULL && TrgBuf != NULL)	// because of NULL chararter
	{
		if (iDataSize > 252 )
		{
			iSize = 252;
			TrgBuf[253] = 0;
			TrgBuf[254] = 0;
		}
		else
		{
			iSize = iDataSize;
		}

		memcpy(&TrgBuf[1],SrcBuf,iSize);
		TrgBuf[0] = (iSize/2);
	}
	else
	{
		memset(TrgBuf,0x00,256);
	}

	return iDataSize;
}

///////////////////////////////////////////////////////////////////////////////////
//
// Function    : PROFILE_GetASFFilePropertiesObject
//
// Description : Get the ASF File Properties
//
///////////////////////////////////////////////////////////////////////////////////
int PROFILE_GetASFFilePropertiesObject(char *tempbuf,void *pID3Info)
{
	pASFFilePropertiesObjectType pASFFilePropertiesObject;
	_MTP_OBJECT_TYPE *pstMTPFileDB =(_MTP_OBJECT_TYPE*)pID3Info;
	int iSize;
	int iOffset = 0;

	if ( tempbuf[0] == 0xA1 && tempbuf[0+1] == 0xDC )
	{
		pASFFilePropertiesObject = (pASFFilePropertiesObjectType)&tempbuf[0];
		iSize = sizeof(ASF_File_Properties_Object);

		if (!memcmp(pASFFilePropertiesObject->mObjectID,ASF_File_Properties_Object,iSize))
		{
			pstMTPFileDB->OBJProp.AudioObj.AudioProp.AudioBitRate = pASFFilePropertiesObject->mMaxBitrate;
			pstMTPFileDB->OBJProp.AudioObj.AudioProp.Duration = (pASFFilePropertiesObject->mPlayDuration[0]/10000)
			        - *(unsigned int *)pASFFilePropertiesObject->mPreroll;
			iOffset = (int)(pASFFilePropertiesObject->mObjectSize[0]);
		}
	}

	return iOffset;
}

///////////////////////////////////////////////////////////////////////////////////
//
// Function    : PROFILE_GetASFContentDescriptionObject
//
// Description : Get the ASF Content Description Object
//
///////////////////////////////////////////////////////////////////////////////////
int PROFILE_GetASFContentDescriptionObject(char *tempbuf,void *pID3Info)
{
	pWMAMetaHeaderType 			 pWMAMetaHeader;
	_MTP_OBJECT_TYPE *pstMTPFileDB =(_MTP_OBJECT_TYPE*)pID3Info;
	int iSize;
	int iOffset = 0;

	if ( tempbuf[0] == 0x33 && tempbuf[1] == 0x26 )
	{
		pWMAMetaHeader = (pWMAMetaHeaderType)&tempbuf[0];
		iSize = sizeof(ASF_Content_Description_Object);

		if (!memcmp(pWMAMetaHeader->mObjectID,ASF_Content_Description_Object,iSize))
		{
			iOffset += sizeof(WMAMetaHeaderType);	// 16(signature) + 8(?)
			// extract the Title data from file
			iOffset += PROFILE_FillWMAMetaData(pWMAMetaHeader->cbCDTitle,&tempbuf[iOffset],pstMTPFileDB->OBJProp.AudioObj.Name);
			// extract the Author data from file
			iOffset += PROFILE_FillWMAMetaData(pWMAMetaHeader->cbCDAuthor,&tempbuf[iOffset],pstMTPFileDB->OBJProp.AudioObj.Artist);
#ifdef ID3_EXT_INCLUDE
			// extract the Copyright data from file
			iOffset += PROFILE_FillWMAMetaData(pWMAMetaHeader->cbCDCopyright,NULL,NULL);
			// extract the Description data from file
			iOffset += PROFILE_FillWMAMetaData(pWMAMetaHeader->cbCDDescription,NULL,NULL);

			// extract the Rating data from file
			if (pWMAMetaHeader->cbCDRating)
			{
				/*cbActual = WMA_GetData(cbCDRating);
				if(cbActual != cbCDRating)
				{
					return WMAERR_BUFFERTOOSMALL;
				}
				cbObjectOffset += cbActual;*/
				iOffset+=pWMAMetaHeader->cbCDRating;
			}

#endif
			iOffset = pWMAMetaHeader->mObjectSize[0];
		}
	}

	return iOffset;
}

///////////////////////////////////////////////////////////////////////////////////
//
// Function    : PROFILE_GetContentDescriptors
//
// Description : Get the content descriptor
//
///////////////////////////////////////////////////////////////////////////////////
int PROFILE_GetDescriptorName(char *tempbuf,int iSize)
{
	int i;
	int iDescriptorSize;

	for ( i = 0 ; i < (WMA_DESCRIPTOR_MAX-1); i++ )
	{
		iDescriptorSize = ASFContentDescriptor[i].mSize;
		//if(iDescriptorSize == iSize)
		{
			if (!memcmp(tempbuf,ASFContentDescriptor[i].mDescriptor,iDescriptorSize))
			{
				return 	ASFContentDescriptor[i].mID;
			}
		}
	}

	return WMA_DESCRIPTOR_UNKNOWN;
}

///////////////////////////////////////////////////////////////////////////////////
//
// Function    : PROFILE_GetContentDescriptors
//
// Description : Get the content descriptor
//
///////////////////////////////////////////////////////////////////////////////////
int PROFILE_GetContentDescriptors(char *tempbuf,void *pID3Info)
{
	int iOffset = 0;
	unsigned short  usNameLength;
	unsigned short  usDataType;
	unsigned short  usDataLength;
	int iDescriptorID;
	_MTP_OBJECT_TYPE *pstMTPFileDB =(_MTP_OBJECT_TYPE*)pID3Info;
	usNameLength  = (tempbuf[1]<<8)|tempbuf[0];
	iOffset += sizeof(unsigned short);
#ifdef THUMBNAILDB_INCLUDE		/* 07.12.13 */

	if (ALBUMART_JPEG.FileSize ==0)
		ALBUMART_JPEG.StartDataPosition +=sizeof(unsigned short);

#endif /* THUMBNAILDB_INCLUDE */
	iDescriptorID = PROFILE_GetDescriptorName(&tempbuf[iOffset],usNameLength);
	iOffset += usNameLength;
#ifdef THUMBNAILDB_INCLUDE		/* 07.12.13 */

	if (ALBUMART_JPEG.FileSize ==0)
		ALBUMART_JPEG.StartDataPosition+=usNameLength;

#endif /* THUMBNAILDB_INCLUDE */
	usDataType = (tempbuf[iOffset+1]<<8)|tempbuf[iOffset];
	iOffset += sizeof(unsigned short);
#ifdef THUMBNAILDB_INCLUDE		/* 07.12.13 */

	if (ALBUMART_JPEG.FileSize ==0)
		ALBUMART_JPEG.StartDataPosition+=sizeof(unsigned short);

#endif /* THUMBNAILDB_INCLUDE */
	usDataLength =  (tempbuf[iOffset+1]<<8)|tempbuf[iOffset];
	iOffset +=sizeof(unsigned short);
#ifdef THUMBNAILDB_INCLUDE		/* 07.12.13 */

	if (iDescriptorID !=WMA_DESCRIPTOR_PICTURE && ALBUMART_JPEG.FileSize ==0)
	{
		ALBUMART_JPEG.StartDataPosition+=usDataLength;
		ALBUMART_JPEG.StartDataPosition+=sizeof(unsigned short);
	}

#endif /* THUMBNAILDB_INCLUDE */

	if (!usDataType)	// Unicode is 0
	{
		switch (iDescriptorID)
		{
			case WMA_DESCRIPTOR_ALBUM:
				iOffset += PROFILE_FillWMAMetaData(usDataLength,&tempbuf[iOffset],pstMTPFileDB->OBJProp.AudioObj.AlbumName);
				break;
			case WMA_DESCRIPTOR_GENRE:
				iOffset += PROFILE_FillWMAMetaData(usDataLength,&tempbuf[iOffset],pstMTPFileDB->OBJProp.AudioObj.Genre);
				break;
			case WMA_DESCRIPTOR_TRACK:
				pstMTPFileDB->OBJProp.AudioObj.Track =  (int)(((int)(tempbuf[iOffset]-'0')*10) + (tempbuf[iOffset+2]-'0'));
				break;
			case WMA_DESCRIPTOR_YEAR:
				iOffset += PROFILE_FillWMAMetaData(usDataLength,&tempbuf[iOffset],pstMTPFileDB->OBJProp.AudioObj.OriginalReleasedDate);
				break;
			case WMA_DESCRIPTOR_ALBUMARTIST:
				iOffset += PROFILE_FillWMAMetaData(usDataLength,&tempbuf[iOffset], pstMTPFileDB->OBJProp.AudioObj.AlbumArtist);
				break;
			case WMA_DESCRIPTOR_ID3TAG:
				break;
#ifdef THUMBNAILDB_INCLUDE		/* 07.12.13 */

				if (ALBUMART_JPEG.FileSize ==0)
					ALBUMART_JPEG.StartDataPosition +=iOffset;

#endif /* THUMBNAILDB_INCLUDE */
		}
	}

#ifdef THUMBNAILDB_INCLUDE		/* 07.12.13 */
	else if (usDataType == 0x0001)	//BYTE array
	{
		switch (iDescriptorID)
		{
			case WMA_DESCRIPTOR_PICTURE:

				if (ALBUMART_JPEG.FileSize ==0)
					ALBUMART_JPEG.StartDataPosition+=sizeof(unsigned short);

				iOffset += PROFILE_GetWMAPictureData(usDataLength,&tempbuf[iOffset],(unsigned int)iOffset, &ALBUMART_JPEG);
				break;
		}
	}

#endif /* THUMBNAILDB_INCLUDE */
	iOffset = (int)(sizeof(unsigned short)*3) + usNameLength + usDataLength;
	return iOffset;
}

///////////////////////////////////////////////////////////////////////////////////
//
// Function    : PROFILE_GetASFExtendedContentDescriptionObject
//
// Description : Get the extended content description
//
///////////////////////////////////////////////////////////////////////////////////
int PROFILE_GetASFExtContentDescriptionObject(char *tempbuf,void *pID3Info)
{
	pASFExtContentDescripObjType pASFExtContentDescripObj;
	int iSize;
	int iOffset = 0;
	int i;
	int iObjectSize = 0;
	int iDescriptorSize;

	if ( tempbuf[iOffset] == 0x40 && tempbuf[iOffset+1] == 0xA4 )
	{
		pASFExtContentDescripObj = (pASFExtContentDescripObjType)&tempbuf[iOffset];
		iSize = sizeof(ASF_Extended_Content_Description_Object);
#ifdef THUMBNAILDB_INCLUDE		/* 07.12.13 */

		if (ALBUMART_JPEG.FileSize ==0)
			ALBUMART_JPEG.StartDataPosition += ALBUMART_JPEG.Header_Ref;

#endif /* THUMBNAILDB_INCLUDE */

		if (!memcmp(pASFExtContentDescripObj->mObjectID,ASF_Extended_Content_Description_Object,	iSize))
		{
			iSize = sizeof(ASFExtContentDescripObjType);
			iOffset += iSize;
			iObjectSize += iSize;
#ifdef THUMBNAILDB_INCLUDE		/* 07.12.13 */

			if (ALBUMART_JPEG.FileSize ==0)
				ALBUMART_JPEG.StartDataPosition+=iSize;

#endif /* THUMBNAILDB_INCLUDE */

			for ( i = 0 ; i < pASFExtContentDescripObj->mContentDescriptorsCnt ; i++ )
			{
				iDescriptorSize = PROFILE_GetContentDescriptors(&tempbuf[iOffset],pID3Info);
				iOffset += iDescriptorSize;
				iObjectSize += iDescriptorSize;
			}

			iOffset = iObjectSize;
		}
	}

	return iOffset;
}

///////////////////////////////////////////////////////////////////////////////////
//
// Function    : PROFILE_GetWMAInfo
//
// Description : Get the Meta data for WMA file
//
///////////////////////////////////////////////////////////////////////////////////
int PROFILE_GetWMAInfo(int fHandle,unsigned char *tempbuf,int iBuffSize,void *pID3Info)
{
	unsigned int i, Ref;
	int iFileSize;
	int iHeaderSize;
	int iReadSize;
	_MTP_OBJECT_TYPE	*pstMTPFileDB =(_MTP_OBJECT_TYPE*)pID3Info;
	pASFHeaderObjectType 		 pASFHeaderObject;
	iFileSize = TC_Length(fHandle);

	if (iFileSize < 128)
	{
		return -1;
	}

	iReadSize = sizeof(ASFHeaderObjectType);
#ifdef THUMBNAILDB_INCLUDE		/* 07.12.13 */
	ALBUMART_JPEG.Header_Ref= iReadSize;
#endif /* THUMBNAILDB_INCLUDE */
	(void) fseek(fHandle,0,SEEK_SET);
	(void) fread(fHandle,tempbuf,iReadSize);
	pASFHeaderObject = (pASFHeaderObjectType)tempbuf;

	if (!memcmp(pASFHeaderObject->mObjectID,ASF_Header_Object,sizeof(ASF_Header_Object)))
	{
		iHeaderSize = pASFHeaderObject->mObjectSize[0];
	}
	else
	{
		return 1;	// Wrong WMA File
	}

	if (iHeaderSize > iBuffSize)
	{
		iReadSize = iBuffSize;
	}
	else
	{
		iReadSize = iHeaderSize;
	}

	(void) fread(fHandle,tempbuf,iReadSize);

	for ( i = 0; i < iReadSize && i < iHeaderSize  ;)
	{
		Ref = i;
		// Find the signature of ASF_File_Properties_Object
		//////////////////////////////////////////////////////////////
		i += PROFILE_GetASFFilePropertiesObject(&tempbuf[i],pstMTPFileDB);
		// Find the signature of ASF_Content_Description_Object
		/////////////////////////////////////////////////////////////////
		i += PROFILE_GetASFContentDescriptionObject(&tempbuf[i],pstMTPFileDB);
		// Find Extended Content Descriptor
		////////////////////////////////////////////////////
#ifdef THUMBNAILDB_INCLUDE		/* 07.12.13 */

		if (ALBUMART_JPEG.FileSize == 0)
			ALBUMART_JPEG.StartDataPosition = i;

#endif /* THUMBNAILDB_INCLUDE */
		i += PROFILE_GetASFExtContentDescriptionObject(&tempbuf[i],pstMTPFileDB);

		if (Ref == i)
			i++ ;
	}

	return 1;
}

#endif // WMA_INCLUDE

#if defined(WAV_INCLUDE)

#ifdef WAV_HEADER_PARSING_INCLUDE
#include "SERVICE/Codec/ADPCM/TCC7xx_PCM.h"
#include "SERVICE/TC_File/FileBufferCtrl.h"

extern tINFO *pWAVHeaderINFO;
tPCM *pWAVHeaderPCM;
extern void ID3_MakeAscii2Unicode(char *SourceBuf, char *TargetBuf,int iSourceMaxSize,int iTargetMaxSize);
#endif

///////////////////////////////////////////////////////////////////////////////////
//
// Function    : PROFILE_GetWAVInfo
//
// Description : Get the Meta data for wave file
//
///////////////////////////////////////////////////////////////////////////////////
#ifdef WAV_HEADER_PARSING_INCLUDE
int PROFILE_GetWAVInfo(int fHandle,unsigned char *tempbuf, int iBufferSize, void *pID3Info)
{
	_MTP_OBJECT_TYPE *pstMTPFileDB = (_MTP_OBJECT_TYPE*)pID3Info;
	///////////////////////////////////////////////////////////
	//Memory Allocation & Initialize

	if (iBufferSize < sizeof(tINFO)+sizeof(tPCM) )
	{
		return 0;
	}

	pWAVHeaderINFO 	= (tINFO *)(tempbuf);
	pWAVHeaderPCM 	= (tPCM *)(tempbuf + sizeof(tINFO));
	memset(pWAVHeaderINFO, 0, sizeof(tINFO));
	memset(pWAVHeaderPCM, 0, sizeof(tPCM));
	//
	///////////////////////////////////////////////////////////
	///////////////////////////////////////////////////////////
	//variable setting for WAVE Header Parsing
	WAVE_SetHeaderParsingStatus(1);
	InitPCMDecoder(pWAVHeaderPCM,fHandle);
	WAVE_SetHeaderParsingStatus(0);
	///////////////////////////////////////////////////////////
	//Save to DB
	pstMTPFileDB->OBJProp.AudioObj.AudioProp.AudioBitRate 	= (int)((int)pWAVHeaderPCM->ucChannels* pWAVHeaderPCM->usSampleRate* (int)pWAVHeaderPCM->uBitsPerSample);

	if ( pWAVHeaderPCM->usSampleRate != 0 )
		pstMTPFileDB->OBJProp.AudioObj.AudioProp.Duration 	   	= pWAVHeaderPCM->ulTimeLength;

	pstMTPFileDB->OBJProp.AudioObj.AudioProp.SampleRating 	= (int)pWAVHeaderPCM->usSampleRate;
	ID3_MakeAscii2Unicode(&pWAVHeaderINFO->Artist[1], 	&pstMTPFileDB->OBJProp.AudioObj.Artist, (pWAVHeaderINFO->Artist[0]-1),64);
	ID3_MakeAscii2Unicode(&pWAVHeaderINFO->Name[1], 	&pstMTPFileDB->OBJProp.AudioObj.Name, (pWAVHeaderINFO->Name[0]-1),64);
	ID3_MakeAscii2Unicode(&pWAVHeaderINFO->Genre[1], 	&pstMTPFileDB->OBJProp.AudioObj.Genre, (pWAVHeaderINFO->Genre[0]-1),64);
	ID3_MakeAscii2Unicode(&pWAVHeaderINFO->Product[1], &pstMTPFileDB->OBJProp.AudioObj.AlbumName, (pWAVHeaderINFO->Product[0]-1),64);
	{
		int i;
		int iFrameSize;
		int temp_track = 0;
		int temp_decimal = 1;
		iFrameSize = (int)pWAVHeaderINFO->Track[0];

		for (i = 0; i < (iFrameSize-2); i++)
		{
			temp_decimal *=10;
		}

		for (i = 0; i < (iFrameSize-1); i++)
		{
			if (pWAVHeaderINFO->Track[1+i] >= '0' && pWAVHeaderINFO->Track[1+i] <= '9')
			{
				temp_track += (pWAVHeaderINFO->Track[1+i]-'0')*temp_decimal;
			}

			temp_decimal /= 10;
		}

		pstMTPFileDB->OBJProp.AudioObj.Track = temp_track;
	}
	return 1;
}

#else
int PROFILE_GetWAVInfo(int fHandle,unsigned char *tempbuf,void *pID3Info)
{
	_MTP_OBJECT_TYPE *pstMTPFileDB = (_MTP_OBJECT_TYPE*)pID3Info;
	pWAVMetaHeaderType  pWAVMetaHeader = (pWAVMetaHeaderType)tempbuf;
	(void) fread(fHandle,tempbuf,WAVE_HEAD_SIZE);
	pstMTPFileDB->OBJProp.AudioObj.AudioProp.AudioBitRate 	= (int)((int)pWAVMetaHeader->mChannel * pWAVMetaHeader->mSamplePerSec \
	        *  (int)pWAVMetaHeader->mBitPerSample);

	if ( pWAVMetaHeader->mSamplePerSec != 0 )
		pstMTPFileDB->OBJProp.AudioObj.AudioProp.Duration 	   	= (int)((int)pWAVMetaHeader->mDuration / pWAVMetaHeader->mSamplePerSec)*1000; // milliseconds

	pstMTPFileDB->OBJProp.AudioObj.AudioProp.SampleRating 	= (int)pWAVMetaHeader->mSamplePerSec;
	return 1;
}
#endif // PROFILE_GetWAVInfo
#endif // WAV_INCLUDE

#ifdef AUDIBLE_INCLUDE
///////////////////////////////////////////////////////////////////////////////////
//
// Function    : PROFILE_GetAudibleFIleInfo
//
// Description : Get the Meta data for Audible file
//
///////////////////////////////////////////////////////////////////////////////////

int PROFILE_GetAudibleFileInfo(int fHandle,unsigned char *tempbuf,void *pID3Info)
{
	_MTP_OBJECT_TYPE *pstMTPFileDB = (_MTP_OBJECT_TYPE*)pID3Info;
	Audible_PROFILE_FileTitle(fHandle, (unsigned char *)pstMTPFileDB->OBJProp.AudibleObj.Name,(unsigned char *)pstMTPFileDB->OBJProp.AudibleObj.Genre);
	Audible_PROFILE_FileAuthor(fHandle, (unsigned char *)pstMTPFileDB->OBJProp.AudibleObj.Artist,(unsigned char *)pstMTPFileDB->OBJProp.AudibleObj.Genre);
	//Audible_PROFILE_CodecInfo(int fHandle, unsigned char * tempbuf, void * pID3Info);
	return 1;
}
#endif // AUDIBLE_INCLUDE
#ifdef AUDIBLE_AAXSDK_V132_INCLUDE
///////////////////////////////////////////////////////////////////////////////////
//
// Function    : PROFILE_GetAudibleFIleInfo
//
// Description : Get the Meta data for Audible file
//
///////////////////////////////////////////////////////////////////////////////////

int PROFILE_GetAudibleFileInfo(int fHandle,unsigned char *tempbuf,void *pID3Info)
{
	_MTP_OBJECT_TYPE *pstMTPFileDB = (_MTP_OBJECT_TYPE*)pID3Info;
	//Audible_PROFILE_FileTitle(fHandle, (unsigned char *)pstMTPFileDB->OBJProp.AudibleObj.Name,(unsigned char *)pstMTPFileDB->OBJProp.AudibleObj.Genre);
	//Audible_PROFILE_FileAuthor(fHandle, (unsigned char *)pstMTPFileDB->OBJProp.AudibleObj.Artist,(unsigned char *)pstMTPFileDB->OBJProp.AudibleObj.Genre);
	//Audible_PROFILE_CodecInfo(int fHandle, unsigned char * tempbuf, void * pID3Info);
	return 1;
}
#endif // AUDIBLE_INCLUDE
#endif // MTP_INCLUDE


///////////////////////////////////////////////////////////////////////////////////
//
// Function    : PROFILE_GetMetaData
//
// Description : Profiling the meta data from each file
//
///////////////////////////////////////////////////////////////////////////////////

#ifdef MTP_INCLUDE
extern unsigned int uiTotalTrackCount;

#ifdef STILLIMAGE_INCLUDE
extern unsigned int uiTotalPictureCount;
#endif	//STILLIMAGE_INCLUDE

#ifdef _MULTIMEDIA_PLAYER_
extern unsigned int uiTotalMovieCount;
#endif

#ifdef AUDIBLE_INCLUDE
extern unsigned int uiTotalAudibleCount;
#endif
#ifdef AUDIBLE_AAXSDK_V132_INCLUDE
extern unsigned int uiTotalAudibleCount;
#endif


/**************************************************************************
*  FUNCTION NAME :
*      int PROFILE_GetMetaData(unsigned uIndex, int iCodec);
*
*  DESCRIPTION :
*  INPUT:
*			iCodec	=
*			uIndex	=
*
*  OUTPUT:	int - Return Type
*  			=
*  REMARK  :
**************************************************************************/
int PROFILE_GetMetaData(int iIndex, unsigned uRealFileIndex, int iCodec, int iPartID, unsigned JPGInMediaCheckFlag)
{
#ifdef MTP_INCLUDE
	int fHandle;
	int dir_num = 2;
	memset(&stMTPFileDB, 0 , sizeof(_MTP_OBJECT_TYPE));
	fHandle = TC_OpenIndex(uRealFileIndex, TC_O_RDONLY, TC_A_READ, dir_num);

	if (TC_ISHERR(fHandle))
	{
		return -1;
	}

	if (TC_Length(fHandle) <= 0)
	{
		stMTPFileDB.OBJInfo.ObjectFormat = PTP_FORMATCODE_UNDEFINED;
		(void) TC_Close(fHandle);
		return 0;
	}

#ifdef THUMBNAILDB_INCLUDE		/* 07.12.13 */
	Rest_Albumart_JPEGInfo();
#endif /* THUMBNAILDB_INCLUDE */

	switch (iCodec)
	{
		case CODEC_MP3:
		case CODEC_MP2:
			PROFILE_GetID3MP3Info(fHandle,FSAPP_GetFileBuffer(),&stMTPFileDB.OBJProp.AudioObj.Track);
			PROFILE_GetMP3HeaderInfo(fHandle,&stMTPFileDB);
			stMTPFileDB.OBJInfo.ObjectFormat = PTP_FORMATCODE_MP3;
			uiTotalTrackCount++;
			break;
#ifdef WMA_INCLUDE
		case CODEC_WMA:
#ifdef TRUSTED_FLASH_INCLUDE
		case CODEC_SMA:
#endif
			PROFILE_GetWMAInfo(fHandle,FSAPP_GetFileBuffer(),FSAPP_GetMaxCopySize(),&stMTPFileDB);
			stMTPFileDB.OBJInfo.ObjectFormat = MTP_FORMATCODE_WMA;
			uiTotalTrackCount++;
			break;
#endif
#ifdef FLAC_INCLUDE
		case CODEC_FLAC:
			stMTPFileDB.OBJInfo.ObjectFormat = MTP_FORMATCODE_UNDEFINED_AUDIO;
			uiTotalTrackCount++;
			break;
#endif
#if 0//def APE_INCLUDE
		case CODEC_APE:
			stMTPFileDB.OBJInfo.ObjectFormat = MTP_FORMATCODE_UNDEFINED_AUDIO;
			uiTotalTrackCount++;
			break;
#endif
#ifdef WAV_INCLUDE
		case CODEC_WAV:
#ifdef WAV_HEADER_PARSING_INCLUDE
			PROFILE_GetWAVInfo(fHandle,FSAPP_GetFileBuffer(), FSAPP_GetMaxCopySize(),&stMTPFileDB);
#else
			PROFILE_GetWMAInfo(fHandle,FSAPP_GetFileBuffer(),&stMTPFileDB);
#endif
			stMTPFileDB.OBJInfo.ObjectFormat = PTP_FORMATCODE_WAVE;
			uiTotalTrackCount++;
			break;
#endif
#if	defined(JPG_DEC_INCLUDE)||defined(MULTI_CODEC_INCLUDE)
		case CODEC_JPG:
			stMTPFileDB.OBJInfo.ObjectFormat = PTP_FORMATCODE_IMAGE_EXIF;
			uiTotalPictureCount++;
			break;
#endif
			/* [1277] Support for StillImage. */
#ifdef	BMP_DEC_INCLUDE
		case CODEC_BMP:
			stMTPFileDB.OBJInfo.ObjectFormat = PTP_FORMATCODE_IMAGE_EXIF;
			uiTotalPictureCount++;
			break;
#endif
#ifdef	GIF_DEC_INCLUDE
		case CODEC_GIF:
			stMTPFileDB.OBJInfo.ObjectFormat = PTP_FORMATCODE_IMAGE_EXIF;
			uiTotalPictureCount++;
			break;
#endif
#ifdef MP4_DEC_INCLUDE
		case CODEC_M4A_DEC:
			stMTPFileDB.OBJInfo.ObjectFormat = PTP_FORMATCODE_AVI;
			uiTotalMovieCount++;
			break;
#endif
#ifdef WMV_INCLUDE
		case CODEC_WMV_AUD:
#ifdef TRUSTED_FLASH_INCLUDE
		case CODEC_SMV_AUD:
#endif
			stMTPFileDB.OBJInfo.ObjectFormat = MTP_FORMATCODE_WMV;
			uiTotalMovieCount++;
			break;
#endif
#ifdef 	TES_CONTAINER_INCLUDE
		case CODEC_TES_AUD:
			stMTPFileDB.OBJInfo.ObjectFormat = PTP_FORMATCODE_AVI;
			uiTotalMovieCount++;
			break;
#endif
#ifdef AAC_LC_INCLUDE
		case CODEC_AAC_AUD:
			stMTPFileDB.OBJInfo.ObjectFormat = PTP_FORMATCODE_AVI;
			uiTotalMovieCount++;
			break;
#endif
#ifdef TXT_DEC_INCLUDE
		case CODEC_TEXT:
			stMTPFileDB.OBJInfo.ObjectFormat = PTP_FORMATCODE_IMAGE_EXIF;
			uiTotalPictureCount++;
			break;
#endif
#ifdef AUDIBLE_INCLUDE
		case CODEC_AUDIBLE:

			if (Check_AudibleFormat(fHandle))
			{
				PROFILE_GetAudibleFileInfo(fHandle,FSAPP_GetFileBuffer(),&stMTPFileDB);
			}

			stMTPFileDB.OBJInfo.ObjectFormat = MTP_FORMATCODE_AUDIBLE;
			uiTotalAudibleCount++;
			break;
#endif
#ifdef AUDIBLE_AAXSDK_V132_INCLUDE
		case CODEC_NEW_AUDIBLE_AAX:
		case CODEC_NEW_AUDIBLE_M4B:
		case CODEC_NEW_AUDIBLE_Format4:
			PROFILE_GetAudibleFileInfo(fHandle,FSAPP_GetFileBuffer(),&stMTPFileDB);

			if (iCodec == CODEC_NEW_AUDIBLE_AAX)
			{
				stMTPFileDB.OBJInfo.ObjectFormat = MTP_FORMATCODE_AUDIBLE_AAX;
			}
			else
			{
				stMTPFileDB.OBJInfo.ObjectFormat = MTP_FORMATCODE_AUDIBLE;
			}

			uiTotalAudibleCount++;
			break;
#endif
#ifdef M3U_INCLUDE
		case CODEC_M3U:
		{
			_MTP_OBJECT_TYPE *pstPayList =& stMTPFileDB;
			pstPayList = &stMTPFileDB;
			pstPayList->OBJInfo.ObjectFormat	= MTP_FORMATCODE_M3U_PLAYLIST;
			pstPayList->OBJProp.PlayListObj.NumOfObject		= 0;
		}
		break;
#endif
		//20060814
		default:
			stMTPFileDB.OBJInfo.ObjectFormat = PTP_FORMATCODE_UNDEFINED;
			break;
	}

#ifdef THUMBNAILDB_INCLUDE		/* 07.12.13 */

	if (JPGInMediaCheckFlag)
		TCDB_THUMBNAIL_SaveAlbumartToDBUsingTagInfo(iIndex, iPartID, fHandle, iCodec);

#endif /* THUMBNAILDB_INCLUDE */
	(void) TC_Close(fHandle);
#else
//	iIndex = iIndex;
#endif
	return -1;
}

#ifdef THUMBNAILDB_INCLUDE		/* 07.12.13 */
///////////////////////////////////////////////////////////////////////////////////
//
// Function    : PROFILE_GetWMAPictureData
//
// Description : Get JPEG Offset in File & JPEG Field Size
//
//
// Input :
//	fHandle 	==> File Handler to read JPEG Offset int WMA file
//	data_size	==> JPEG Field Size in WMA file
//	tempbuf		==> temp buffer
//
//	return :
//
//	Offset 		==> JPEG Start Position (Offset) in WMA file
//
///////////////////////////////////////////////////////////////////////////////////
int PROFILE_GetWMAPictureData(int iDataSize,char *SrcBuf,unsigned int Start_Pos, _ALBUMART_PICTURE_TYPE	*ALBUMART_JPEG)
{
	unsigned int 		i;
	unsigned int 		iOffset = 0;
	unsigned int	 	usDataLength;
	usDataLength = (unsigned int)(SrcBuf[iOffset+2]<<8)|SrcBuf[iOffset+1];
	ALBUMART_JPEG->FileSize= usDataLength;	//Data Fild size

	for (i=0; i<512 ; i++)
	{
		if (SrcBuf[i] == 0xFF &  SrcBuf[i+1] == 0xD8)
		{
			ALBUMART_JPEG->Flag = 1;
			ALBUMART_JPEG->StartDataPosition += i;
			return iDataSize;
		}
		else
		{
			ALBUMART_JPEG->Flag = 0;   // Not Support Image Type
		}
	}

	return 0;
}
#endif /* THUMBNAILDB_INCLUDE */

#endif /* MTP_INCLUDE */

/* end of file */

