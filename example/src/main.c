/****************************************************************************************
 *   FileName    : main.c
 *   Description : It is a test file.
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

#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <glib.h>
#include "sqlite3.h"
#include "TCDBDefine.h"
#include "TCDBGen.h"
#include "UI_META_browsing.h"
#include "main.h"
#ifdef META_DB_INCLUDE
#ifndef USE_TAGLIB
#include "nls720.h"
#endif
#include "MetaParser.h"
#endif

typedef struct _CustomData
{ 
	uint32_t dbNum;
	uint32_t currentIndex;
	GMainLoop *loop;
} CustomData;

int32_t gDBExit =0;
	
int32_t gDBReady =0;

static void TEST_MetaBrowsing(uint32_t dbNum);
static gboolean handle_keyboard (GIOChannel *source, GIOCondition cond, CustomData *data);
void PritnfFileListInfo(uint32_t dbNum);
void TEST_MetaLowAPIs(uint32_t dbNum);

static void FileDBResult(uint32_t dbNum, int32_t result)
{
	if(result == DB_SUCCESS)
	{
		(void)fprintf(stderr, "[%s]File Gathering Completed : device(%d)\n", __FUNCTION__,dbNum);

		int32_t iReturn;
		SetCurrentTime();
		iReturn = TCDB_Meta_MakeDB(dbNum);
		if(iReturn != DB_SUCCESS)
		{
			gDBExit =0;
			 (void)fprintf(stderr, "Genreate Meta DB Failed, error : (%d) \n", iReturn );
		}
		else
		{
		/*
			(void)fprintf(stderr,"Maked Meta DB (%0.6f)\n", GetSecondsUntilNow());
			TEST_MetaLowAPIs(dbNum);
		*/
			TEST_MetaBrowsing( dbNum);
		}

	}
	else
	{
		(void)fprintf(stderr, "[%s]File Gathering failed : device(%d), result (%d)\n", __FUNCTION__,dbNum, result);
	}
}

static void FileDBUpdate(uint32_t dbNum, ContentsType type)
{
	(void)fprintf(stderr, "[%s]File update : device(%d), Contents Type (%d), totalCount (%d)\n", __FUNCTION__, dbNum, type, TCDB_GetTotalFileCount(dbNum, type));
	/* PritnfFileListInfo(dbNum); */
}


static void MetaDBResult(uint32_t dbNum, int32_t result)
{
	(void)fprintf(stderr,"[%s]Maked Meta DB (%0.6f)\n", __FUNCTION__,GetSecondsUntilNow());
	if(result == DB_SUCCESS)
	{
		(void)fprintf(stderr, "[%s]Meta Gathering Completed : device(%d)\n", __FUNCTION__,dbNum);
	}
	else
	{
		gDBExit =0;
		(void)fprintf(stderr, "[%s]Meta Gathering failed : device(%d), result (%d)\n", __FUNCTION__,dbNum, result);
	}
}

static void MetaDBUpdate(uint32_t dbNum)
{
	(void)fprintf(stderr, "[%s]Meta update : device(%d)\n", __FUNCTION__, dbNum);
}


#define DATABASE_FOLDER 	"/database"

uint32_t gFolderList[2000];
uint32_t gFileList[6000];

int32_t AddFolderList(void *args, int32_t rowCount, int32_t folderIndex)
{
	int32_t iReturn;
	uint32_t dbNum =1;
	char name[MAX_NAME_LENGTH], fullPath[MAX_PATH_LENGTH];
	if(args != NULL)
	{
		uint32_t *pList = (uint32_t *)args;
/*
		pList[rowCount] = fileIndex;
	*/
		iReturn = TCDB_GetFolderName( dbNum, folderIndex, name,MAX_NAME_LENGTH);
		if(iReturn != DB_SUCCESS)
		{
			(void)fprintf(stderr, "TCDB_GetFolderName Failed, error : (%d) \n", iReturn) ;
		}			
		iReturn = TCDB_GetFolderFullPath(dbNum, folderIndex,fullPath,MAX_PATH_LENGTH);
		if(iReturn != DB_SUCCESS)
		{
			(void)fprintf(stderr, "TCDB_GetFolderFullPath Failed, error : (%d) \n", iReturn) ;
		}
		(void)fprintf(stderr, "[%d] : FolderIndex (%d), name (%s), full path (%s)  \n",rowCount, folderIndex, name, fullPath);
		(void)fprintf(stderr, "[%d] : SubFolder (%d), SubAudio(%d), SubVideo(%d), SubImage(%d) \n",rowCount,
			TCDB_GetSubFolderCount(dbNum, folderIndex),
			TCDB_GetSubFileCount(dbNum, folderIndex, AudioContents), 
			TCDB_GetSubFileCount(dbNum, folderIndex, VideoContents), 
			TCDB_GetSubFileCount(dbNum, folderIndex, ImageContents));
	}
	return 0;
}

int32_t AddFileList(void *args, int32_t rowCount, int32_t folderIndex)
{
	int32_t iReturn;
	char name[MAX_NAME_LENGTH], fullPath[MAX_PATH_LENGTH];
	if(args != NULL)
	{
		uint32_t *pList = (uint32_t *)args;

		iReturn = TCDB_GetFileName( 1, folderIndex, name, MAX_NAME_LENGTH, AudioContents);
		if(iReturn != DB_SUCCESS)
		{
			(void)fprintf(stderr, "TCDB_GetFolderName Failed, error : (%d) \n", iReturn );
		}			
		iReturn = TCDB_GetFileFullPath(1, folderIndex,fullPath,MAX_PATH_LENGTH, AudioContents);
		if(iReturn != DB_SUCCESS)
		{
			(void)fprintf(stderr, "TCDB_GetFolderFullPath Failed, error : (%d) \n", iReturn);
		}
		(void)fprintf(stderr, "[%d] : File Index (%d), name (%s), full path (%s)  \n", rowCount, folderIndex, name, fullPath);	
	}
	return 0;
}


void PritnfFileListInfo(uint32_t dbNum)
{
	int32_t iReturn;
	uint32_t totalFolder, totalAudio, totalVideo, totalImage;
	uint32_t index;
	char name[MAX_NAME_LENGTH], fullPath[MAX_PATH_LENGTH];
	uint32_t i;
	ContentsType type;

	totalFolder = TCDB_GetTotalFolderCount(dbNum);
	totalAudio = TCDB_GetTotalFileCount(dbNum, AudioContents);
	totalVideo = TCDB_GetTotalFileCount(dbNum, VideoContents);
	totalImage = TCDB_GetTotalFileCount(dbNum, ImageContents);

	printf("Total Folder : (%d), Total Audio : (%d), Total Video : (%d), Total Image (%d) \n",
		totalFolder, totalAudio, totalVideo, totalImage);

	(void)fprintf(stderr,"\n========================== Folder List ==========================\n");
	iReturn = TCDB_AddAllFolderList(dbNum, AddFolderList, (void*)gFolderList);

	(void)fprintf(stderr,"\n========================== Aduio File List ==========================\n");
	type = AudioContents;
	iReturn = TCDB_AddAllFileList(dbNum, type, AddFileList, (void*)gFileList);		

}


void TEST_MetaLowAPIs(uint32_t dbNum)
{
	int32_t iReturn;
	int32_t iFileIndex;
	int32_t numberCategory;
	int32_t i;
	char CategoryName[MAX_TAG_LENGTH];
	
	iReturn = TCDB_Meta_ResetBrowser(dbNum);
	if(iReturn != DB_SUCCESS)
	{
		 (void)fprintf(stderr, "TCDB_Meta_ResetBrowser Failed, error : (%d) \n", iReturn );
	}

	numberCategory = TCDB_Meta_GetNumberCategory(dbNum,"Genre");
	(void)fprintf(stderr, "TCDB_GetNumberCategory : (%d) \n", numberCategory );

	for(i=0; i < 2; i++)
	{
		iFileIndex = TCDB_Meta_GetCategoryList(dbNum,"Genre", i);
		iReturn = TCDB_Meta_GetCategoryListName(dbNum,"Genre", i, CategoryName,MAX_TAG_LENGTH);
		if(iReturn == DB_SUCCESS)
		{
			(void)fprintf(stderr, "Category List %d: (%d), (%s) \n", i, iFileIndex,CategoryName);
		}
		else
		{
			(void)fprintf(stderr, "Get Category Name error %d: (%d)\n", i, iFileIndex);
		}
	}
	iReturn = TCDB_Meta_SelectCategory(dbNum,"Genre", 2);
	(void)fprintf(stderr, "TCDB_Meta_SelectCategory : (%d) \n", iReturn );

	iReturn = TCDB_Meta_GetNumberCategory(dbNum,"Album");
	(void)fprintf(stderr, "TCDB_GetNumberCategory : (%d) \n", iReturn );

	iReturn = TCDB_Meta_SelectCategory(dbNum,"Album", 3);
	(void)fprintf(stderr, "TCDB_Meta_SelectCategory : (%d) \n", iReturn );

	iReturn = TCDB_Meta_GetNumberCategory(dbNum,"title");
	(void)fprintf(stderr, "TCDB_GetNumberCategory : (%d) \n", iReturn );

	iFileIndex =0;
	iReturn = TCDB_Meta_GetCategoryListName(dbNum, "title", iFileIndex, CategoryName,MAX_TAG_LENGTH);
	if(iReturn == DB_SUCCESS)
	{
		(void)fprintf(stderr, "Category Name (%d), (%s) \n", iFileIndex, CategoryName);
	}
	else
	{
		(void)fprintf(stderr, "Get Category Name error (%d)\n",  iFileIndex);
	}

	iReturn = TCDB_Meta_UndoCategory(dbNum);
	(void)fprintf(stderr, "Undo Category (%d)\n", iReturn);
	iReturn = TCDB_Meta_UndoCategory(dbNum);
	(void)fprintf(stderr, "Undo Category (%d)\n", iReturn);

	iReturn = TCDB_Meta_SelectCategory(dbNum,"Genre", 4);
	(void)fprintf(stderr, "TCDB_Meta_SelectCategory : (%d) \n", iReturn );

}

static int32_t MetaMenuIndex =1;

static void MetaCategoryMenuNameEvent(uint32_t dbNum,uint8_t mode, const uint8_t *pStr)
{
	(void)fprintf(stderr, "[%s] dbNum(%d) mode (%d), name (%s) \n",__FUNCTION__, dbNum,mode, pStr);
}
static void MetaCategoryIndexChangeEvent(uint32_t dbNum,uint32_t totalNum, uint32_t currentNum)
{
	(void)fprintf(stderr, "[%s] dbNum(%d) totalNum (%d), current Num (%d)\n",__FUNCTION__, dbNum,totalNum, currentNum);
}
static void MetaCategoryNameEvent(uint32_t dbNum,__u16 index, const uint8_t *pStr)
{
	(void)fprintf(stderr, "[%s] dbNum(%d) index (%d), Name (%s), Categorylist(%d)\n",__FUNCTION__,dbNum, MetaMenuIndex, pStr,index+1);
	MetaMenuIndex++;
}

static void MetaCategoryNameCommit(uint32_t dbNum)
{
	(void)fprintf(stderr, "[%s] dbNum(%d) \n",__FUNCTION__,dbNum);
	MetaMenuIndex =1;
}

static void MetaSelectTrack(uint32_t dbNum,uint32_t selectIndex)
{
	uint32_t totalTrackNum, i;
	uint32_t selectTrack;
	uint32_t ret;
	uint32_t *pList=NULL;
	char trackName[MAX_NAME_LENGTH];
	char trackPath[MAX_PATH_LENGTH];

	totalTrackNum =  TCDB_Meta_GetTotalTrack(dbNum);
	pList = malloc(sizeof(uint32_t)*totalTrackNum);
	selectTrack =  TCDB_Meta_GetFirstSelectTrackIndex(dbNum);
	for(i =0; i <totalTrackNum; i++)
	{
		pList[i] = TCDB_Meta_GetTrackList(dbNum, i);

		ret = TCDB_GetFileName(dbNum, pList[i], trackName, MAX_NAME_LENGTH, AudioContents);
		if(ret != DB_SUCCESS)
		{
			(void)fprintf(stderr,"Get Name Error : (%d)\n", ret);
		}
		ret = TCDB_GetFileFullPath(dbNum,pList[i], trackPath,MAX_PATH_LENGTH,AudioContents);
		if(ret != DB_SUCCESS)
		{
			(void)fprintf(stderr,"Get File Path Error : (%d)\n", ret);
		}

		(void)fprintf(stderr,"[%d] :FileIndex(%d) Name (%s)", i, pList[i], trackName);

		if(i == selectTrack)
		{
			(void)fprintf(stderr,"=========> select track\n");
		}
		else
		{
			(void)fprintf(stderr,"\n");
		}

	}
}

void Init_MetaBrowsing(void)
{
	TCDB_MetaBrwsEventCB cb;

	cb._MetaCategoryIndexChange = (void *)MetaCategoryIndexChangeEvent;
	cb._MetaCategoryMenuName = (void *)MetaCategoryMenuNameEvent;
	cb._MetaCategoryName = (void *)MetaCategoryNameEvent;
	cb._MetaCategoryNameCommit = (void *)MetaCategoryNameCommit;
	cb._MetaSelectTrack = (void *)MetaSelectTrack;

	UI_META_SetEventCallbackFunctions(&cb);
}

#define MAX_META_LIST_NUM 5

static void TEST_MetaBrowsing(uint32_t dbNum)
{
	int32_t iReturn;
	int32_t selectIndex=0;
	GIOChannel *io_stdin;   	
	CustomData data;

	(void)memset (&data, 0, sizeof (data));
	
	Init_MetaBrowsing();

	g_print (    
		"USAGE: Choose one of the following options, then press enter:\n"    
		" 'U' Up\n"
		" 'D' Down\n"
		" 'B' Back(undo)\n"
		" '0' Select List 1\n"
		" '1' Select List 1\n"
		" '2' Select List 2\n"
		" 'n' Select List n\n"
		" 'Q' to quit\n");	

	iReturn = UI_META_NAVI_ClearAllNaviInfo();
	if(iReturn != DB_SUCCESS)
	{
		(void)fprintf(stderr, "UI_META_NAVI_ClearAllNaviInfo Error : (%d)\n", iReturn);
	}

	iReturn = UI_META_NAVI_StartBrowse(dbNum, MAX_META_LIST_NUM);
	if(iReturn != DB_SUCCESS)
	{
		(void)fprintf(stderr, "UI_META_NAVI_StartBrowse Error : (%d)\n", iReturn);
	}
	else
	{
		data.dbNum = dbNum;
		data.currentIndex = 0;
#ifdef _WIN32 
		io_stdin = g_io_channel_win32_new_fd (fileno (stdin));
#else  
		io_stdin = g_io_channel_unix_new (fileno (stdin));
#endif
		 g_io_add_watch (io_stdin, G_IO_IN, (GIOFunc)handle_keyboard, &data);

		data.loop	= g_main_loop_new (NULL, FALSE);
		g_main_loop_run (data.loop);
		g_main_loop_unref (data.loop);
		g_io_channel_unref (io_stdin);
	}

	iReturn = UI_META_NAVI_DeleteNaviInfo( dbNum);
	if(iReturn != DB_SUCCESS)
	{
		(void)fprintf(stderr, "UI_META_NAVI_DeleteNaviInfo Error : (%d)\n", iReturn);
	}
}


/* Process keyboard input */
static gboolean handle_keyboard (GIOChannel *source, GIOCondition cond, CustomData *data)
{
	gchar *str = NULL;

	if (g_io_channel_read_line (source, &str, NULL, NULL, NULL) != G_IO_STATUS_NORMAL) {
		return TRUE;
	}

	switch (g_ascii_tolower (str[0])) {
		case 'u':
			if(data->currentIndex > MAX_META_LIST_NUM)
			{
				data->currentIndex-=MAX_META_LIST_NUM;
			}
			else
			{
				data->currentIndex=0;
			}
			UI_META_NAVI_Move(data->dbNum, data->currentIndex);
		break;

		case 'd':
			data->currentIndex+=MAX_META_LIST_NUM;
			UI_META_NAVI_Move(data->dbNum, data->currentIndex);
		break;

		case 'b':
			UI_META_NAVI_SelectParentItem(data->dbNum);
		break;
		
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
			UI_META_NAVI_SelectChildItem(data->dbNum, atoi(&str[0]));
		break;

		case 'q':
			g_main_loop_quit(data->loop);
			g_print ("Good Bye\n");
		break;
		
		default:
		break;	
	}

	g_free (str);

	return TRUE;
}

int32_t main(int32_t argc, char **argv)
{
	int32_t iReturn;
	int32_t iPathlength=0;
	char *pFilePath=NULL;
	uint32_t dbNum =1;
	uint32_t i;
	TCDB_EventCB dbEventcallback;
	TCDB_UserCB dbUsercallback;

	gDBExit =1;
	
	if(argc <2 )
	{
		printf( "Input parameter errr \n");
		iReturn =-1;
	}
	else
	{
		TCDB_SetDebug(0);
		
		if(argv[2]!=NULL)
		{
			if(strcmp(argv[2], "debug_on" )==0)
			{
				TCDB_SetDebug(1);
			}
		}
		
		printf("Mount path %s\n", argv[1]);

		iReturn =  TCDB_Initialize("/home/root",4,"/usr/share/tc-dbgen/tc-dbconf.xml");

		/* regist callback fucntion for Check Extention */
		dbEventcallback._fileDBResult = FileDBResult;
		dbEventcallback._metaDBResult = MetaDBResult;
		dbEventcallback._fileDBUpdate = FileDBUpdate;
		dbEventcallback._metaDBUpdate = MetaDBUpdate;
		TCDB_SetEventCallbackFunctions(&dbEventcallback);

		dbUsercallback._compareFunction = NULL;
		dbUsercallback._gatheringTag = TagParsingCB;
		TCDB_SetUserCallbackFunctions(&dbUsercallback);

#ifndef USE_TAGLIB
		InitNLS();
#endif
		SetCurrentTime();
		/* Make File DB */
		iReturn = TCDB_GenerateFileDB(dbNum, argv[1]);
		if(iReturn != DB_SUCCESS)
		{
			gDBExit =0;
			(void)fprintf(stderr, "Genreate File DB Failed, error : (%d) \n", iReturn );
		}
		else
		{
			(void)fprintf(stderr,"Maked File DB (%0.6f)\n", GetSecondsUntilNow());
		}
		
		/*
		PritnfFileListInfo(dbNum);
		*/

		while(gDBExit)
		{
			usleep(5);
		}

		TCDB_DeleteDB(dbNum);
		TCDB_Release();
		usleep(10);

	}

	return iReturn;
}

