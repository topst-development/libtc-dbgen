/****************************************************************************************
 *   FileName    : MetaParser.c
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
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <fcntl.h>
#include "sqlite3.h"
#include "TCDBDefine.h"
#include "MetaParser.h"
#ifdef USE_TAGLIB
#include <tag_c.h>
#endif
#include "ID3Parser.h"
#include "TCTagParser.h"


#if __GNUC__ > 2 || (__GNUC__ == 2 && __GNUC_MINOR__ >= 95)
#define error(...) do {\
	fprintf(stderr, "[%s:%d] Error : ", __FUNCTION__, __LINE__); \
	fprintf(stderr, __VA_ARGS__); \
} while (0)
#else
#define error(args...) do {\
	fprintf(stderr, "[%s:%d] Error : ", __FUNCTION__, __LINE__); \
	fprintf(stderr, ##args); \
} while (0)
#endif

#ifndef bool
typedef enum {
	false,
	true
} bool;	
#endif

extern int g_tcdb_debug;

#define DEBUG_TCDB_PRINTF(format, arg...) \
	if (g_tcdb_debug) \
	{ \
		fprintf(stderr, "[TCDB] [%s:%d] : "format"", __FUNCTION__, __LINE__, ##arg); \
	}

#ifdef __cplusplus
extern "C" {
#endif


char * gUnknownTag = "Unknown\0";

static int GatheringTAG_UsingTCParser(unsigned int dbNum, int fileIndex, const char * pfolderPath, const char *pfileName);


#ifdef USE_TAGLIB
/*
This is example function for use taglib.
Therefore, this is not a perfect code. It is just sample code using taglib_c.
If you want to use the taglib properly, first you will need to apply string converting function (ex, ISO8859-1 -> your local language) in taglib.
*/
static int GatheringTAG_UsingTaglib(unsigned int dbNum, int fileIndex, const char * pfolderPath, const char *pfileName);

static int GatheringTAG_UsingTaglib(unsigned int dbNum, int fileIndex, const char * pfolderPath, const char *pfileName)
{
	int ret = DB_UNKOWN_ERR;
	unsigned char filePath[MAX_PATH_LENGTH+2];
  	TagLib_File *file=NULL;
	TagLib_Tag *tag=NULL;
	
	//DEBUG_TCDB_PRINTF(" TAG Parsing (%s) \n", filePath);  	
	taglib_set_strings_unicode(true);	

	if((fileIndex > -1)&&(pfolderPath !=NULL)&&(pfileName!=NULL))
	{
		unsigned int filePathLen;
		filePathLen = strlen(pfolderPath) + strlen(pfileName);
		if(filePathLen < MAX_PATH_LENGTH)
		{
			sprintf(filePath,"%s/%s",pfolderPath, pfileName);
			file = taglib_file_new(filePath);
			if(file !=NULL)
			{
				tag = taglib_file_tag(file);
				if(tag !=NULL)
				{
					char * pTitle = NULL;
					char * pArtist = NULL;
					char * pAlbum = NULL;
					char * pGenre = NULL;

					pTitle = taglib_tag_title(tag);
					pArtist = taglib_tag_artist(tag);
					pAlbum = taglib_tag_album(tag);
					pGenre = taglib_tag_genre(tag);

					if( *pTitle == 0)
					{
						pTitle = strrchr(pfileName, '/');
						if(pTitle != NULL)
						{
							pTitle++;
						}
						else
						{
							pTitle = (char *)pfileName;
						}
					}
					if( *pArtist == 0)
					{
						pArtist = gUnknownTag;
					}
					if(*pAlbum==0)
					{
						pAlbum = gUnknownTag;
					}
					if( *pGenre==0)
					{
						pGenre = gUnknownTag;
					}
					ret = TCDB_Meta_InsertTag(dbNum, fileIndex, pTitle, pArtist, pAlbum, pGenre);
				}
				taglib_tag_free_strings();
				taglib_file_free(file);
			}
			else
			{
				char * pTitle = strrchr(filePath, '/');;
				if(pTitle != NULL)
				{
					pTitle++;
				}
				else
				{
					pTitle = filePath;
				}
				DEBUG_TCDB_PRINTF("Taglib file open failed : (%s)\n", filePath);
				ret = TCDB_Meta_InsertTag(dbNum, fileIndex, gUnknownTag, gUnknownTag, gUnknownTag, gUnknownTag);
			}

		}
		else
		{
			ret = DB_ARGUMENT_ERR;
			error("file path too long : (%d) > MAX_PATH_LENGTH(%d)",filePathLen, MAX_PATH_LENGTH);
		}
	}
	else
	{
		ret = DB_ARGUMENT_ERR;
	}

	return ret;	
}
#endif

static int GatheringTAG_UsingTCParser(unsigned int dbNum, int fileIndex, const char * pfolderPath, const char *pfileName)
{
	int tagRet = -ID3_ERROR_Pasing;
	int ret = DB_UNKOWN_ERR;
	stID3V2XInfoType ID3Data;
	int unkwonTagLen =  strlen(gUnknownTag);
	
	if((fileIndex > -1)&&(pfolderPath !=NULL)&&(pfileName!=NULL))
	{
		char *extType;
		size_t extlen;
		char * pTitle = NULL;
		char * pArtist = NULL;
		char * pAlbum = NULL;
		char * pGenre = NULL;
		
		extType = strrchr(pfileName, '.');
		extlen = strlen(extType);

		if(extType != NULL)
		{
			/* TAG Parsing */
			if(strncasecmp(extType, ".MP3", extlen) ==0)
			{
				tagRet = TC_GetMP3Tag(pfolderPath, pfileName, &ID3Data);
			}
			else if(strncasecmp(extType, ".WMA", extlen) ==0)
			{
				tagRet = TC_GetWMATag(pfolderPath, pfileName, &ID3Data);
			}
			else
			{
				ret = DB_NOT_SUPPORT_CODEC;
				error("Not support codec : %s, extension = %s \n",pfileName, extType);
			}

			/* Insert Tag */
			if(tagRet > -1)
			{
				pTitle = (unsigned char *)&ID3Data.mMetaData[ID3_TITLE][1];
				pArtist = (unsigned char *)&ID3Data.mMetaData[ID3_ARTIST][1];
				pAlbum = (unsigned char *)&ID3Data.mMetaData[ID3_ALBUM][1];
				pGenre = (unsigned char *)&ID3Data.mMetaData[ID3_GENRE][1];

				if( *pTitle == 0)
				{
					pTitle = strrchr(pfileName, '/');
					if(pTitle != NULL)
					{
						pTitle++;
					}
					else
					{
						pTitle = (char *)pfileName;
					}
					ID3Data.mMetaData[ID3_TITLE][0] =  strlen(pTitle);
					ID3Data.mMetaData[ID3_TITLE][strlen(pTitle)] = 0x00;
				}
				if( *pArtist == 0)
				{
					ID3Data.mMetaData[ID3_ARTIST][0] = unkwonTagLen;
					pArtist = gUnknownTag;
				}			
				if(*pAlbum==0)
				{
					ID3Data.mMetaData[ID3_ALBUM][0] = unkwonTagLen;
					pAlbum = gUnknownTag;
				}
				if( *pGenre==0)
				{
					ID3Data.mMetaData[ID3_GENRE][0] = unkwonTagLen;
					pGenre = gUnknownTag;
				}

				ret = TCDB_Meta_InsertTag(dbNum, fileIndex, pTitle, pArtist, pAlbum, pGenre);
			}
			else
			{
				pTitle = strrchr(pfileName, '/');
				if(pTitle != NULL)
				{
					pTitle++;
				}
				else
				{
					pTitle = (char *)pfileName;
				}
				/* Insert unkownTag */
				ret = TCDB_Meta_InsertTag(dbNum, fileIndex, gUnknownTag, gUnknownTag, gUnknownTag, gUnknownTag);
			}
		}
		else
		{
			ret = DB_ARGUMENT_ERR;
			error("Unknown file : (%s)\n", DB_ARGUMENT_ERR);
		}
	}
	else
	{
		ret = DB_ARGUMENT_ERR;
		error("Bad Argument Error : (%s)\n", DB_ARGUMENT_ERR);
	}
	return ret;
}

int TagParsingCB(unsigned int dbNum, int fileIndex, const char * pfolderPath, const char *pfileName)
{
	int ret =  DB_UNKOWN_ERR;
	int unkwonTagLen =  strlen(gUnknownTag);

	if((pfolderPath !=NULL)&&(pfileName!=NULL))
	{
		ret =  GatheringTAG_UsingTCParser(dbNum, fileIndex, pfolderPath, pfileName);
		//ret =  GatheringTAG_UsingTaglib(dbNum, fileIndex, pfolderPath, pfileName);
	}
	else
	{
		error("Taglib file open failed : (%s)\n", pfileName);
	}
	return ret;
}

#ifdef __cplusplus
}
#endif

