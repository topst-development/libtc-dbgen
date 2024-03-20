/****************************************************************************************
 *   FileName    : TCDBGen.c
 *   Description : main c file of libtcdbgen
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

#include <dirent.h> 
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <linux/types.h>
#include <math.h>
#include <ctype.h>
#include <errno.h>
#include <pthread.h>
#include <limits.h>
#include <stdint.h>
#include "sqlite3.h"
#include "TCDBDefine.h"
#include "sqlite_query.h"
#include "TCUtil.h"
#include "DBConfig.h"
#include "TCDBGen.h"

#define	ROOT_INODE (0xFFFFFFFFU -2)
#define USE_RECURSIVE

int32_t g_tcdb_debug;

#define TCDB_DATABASE_NAME 	"TCDB_v%s_%d.db"

#define MAX_FOLDER_NUM		2000
#define MAX_FILE_NUM 		6000

#define DEFAULT_COLLATEFUNCTION "BINARY"
#define CUSTOM_COLLATEFUNCTION "myCompare"

static const char * gCollateFunction = DEFAULT_COLLATEFUNCTION;

#ifdef __cplusplus
extern "C" {
#endif

typedef struct dbInfo
{
	int32_t	_deviceIndex;
	sqlite3 * _handle;
	int32_t _status;
	char _dbName[MAX_PATH_LENGTH];
	char _mountPath[MAX_PATH_LENGTH];	
	
	uint32_t _totalFolderCount;
	uint32_t _totalAudioFileCount;
	uint32_t _totalVideoFileCount;
	uint32_t _totalImageFileCount;

	uint32_t _updateAudioCount;
	uint32_t _updateVideoCount;
	uint32_t _updateImageCount;
	
	pthread_t _fileThread;
	pthread_t _metaThread;

	uint32_t _isFileThreadCreated;
	uint32_t _isMetaThreadCreated;

	uint32_t _isRun;
	uint32_t	_forceStop;

	pthread_mutex_t _handleLock;
}DB_Info, *pDB_Info;

DB_Config gDB_Config;
static pDB_Info pgDB_Info = NULL;

#ifdef META_DB_INCLUDE
#define META_SELECT_ALL	0xFFFFFFFFU
typedef struct metaCategoryInfo
{
	char *_pCategory;
	char _selectTagName[MAX_TAG_LENGTH];
	uint32_t _selectIndex;
	uint32_t *_pList;
	uint32_t _totalIndex;
	struct metaCategoryInfo *_pNextCategory;
}MetaCategoryInfo;

typedef struct metaBrwsHandle
{
	uint32_t _dbNum;
	sqlite3 * _db;
	MetaCategoryInfo *_pCategorytree;
	uint32_t *_pPlayList;
	int32_t _selectTrackNum;
	int32_t _totalTrakNum;
}MetaBrwsHandle, *pMetaBrwsHandle;

static pMetaBrwsHandle pgMetaBrwsHandle = NULL;

#endif

#ifdef CONFIG_ARCH64
struct linux_dirent64 {
	__ino64_t		d_ino;    /* 64-bit inode number */
	__off64_t		d_off;    /* 64-bit offset to next structure */
	unsigned short	d_reclen; /* Size of this dirent */
	unsigned char	d_type;   /* File type */
	char			d_name[]; /* Filename (null-terminated) */
};
#else
struct linux_dirent {
   long           d_ino;
   unsigned long  d_off;
   unsigned short d_reclen;
   char           d_name[];
};
#endif

static pthread_mutex_t dbMutex;
static pthread_mutex_t *dbMutexPtr = NULL;

/****************************************************************************
Callbackl functions ( Static )
****************************************************************************/
static FileDBResult_cb			fileDBResult_cb = NULL;
static MetaDBResult_cb			metaDBResult_cb = NULL;

static FileDBUpdate_cb			fileDBUpdate_cb = NULL;
static MetaDBUpdate_cb			metaDBUpdate_cb = NULL;

static CompareFunction_cb		compareFunction_cb = NULL;
static GatheringTag_cb			gatheringTag_cb = NULL;

static void initializeDBInfo(uint32_t dbNum);
static pDB_Info  GetDBInfo(uint32_t dbNum);
static pDB_Info  GetEmptyDBInfo(uint32_t dbNum);
static ContentsType CheckSupportCodecList(DB_Config *dbConfig ,const char *name);
static int32_t Init_FileDB(uint32_t dbNum);
static void Release_DB(uint32_t dbNum);
static int32_t CreateCollationFuction( sqlite3* db, int32_t(*xCompare)(void *data, int32_t len1, const void* in1, int32_t len2, const void* in2));
static int32_t Make_FileDB(uint32_t dbNum, const char *mountPath);
static int32_t Search_FileDB(pDB_Info pDBInfo, const char *mountPath);
static int32_t Get_DirEntry(pDB_Info pDBInfo, const char *dir, uint32_t parentIndex);
static int32_t Sort_FileDB(pDB_Info pDBInfo);
static int32_t Set_DatabaseName(uint32_t dbNum);
static const char * Get_DatabaseName(uint32_t dbNum);
#ifdef META_DB_INCLUDE
static void Meta_Release(uint32_t dbNum);
static int32_t TCDB_Meta_Initialize (void);
static void TCDB_Meta_Release(void);
static int32_t GatheringMetaDB(uint32_t dbNum,sqlite3 *db, pDB_Info pDBInfo);
static int32_t Meta_MakeDB(uint32_t dbNum);
static MetaCategoryInfo *CreateMetaTree(const char * category);
static void AppendMetaTree(MetaCategoryInfo ** head, MetaCategoryInfo * newTree);
static MetaCategoryInfo * FindMetaTree(MetaCategoryInfo * head, int32_t depth);
static void DestroyMetaTree(MetaCategoryInfo * metaTree);
static int32_t GetTreeDepth(MetaCategoryInfo * head);
static void RemoveLastMetaTree(MetaCategoryInfo ** head);
static void ResetMetaTree(MetaCategoryInfo * head);
static void PrintMetaTree(MetaCategoryInfo *pCategory);
#endif

void TCDB_SetEventCallbackFunctions(TCDB_EventCB *cb)
{
	DEBUG_TCDB_PRINTF("\n");
	if(NULL != cb)
	{
		fileDBResult_cb = cb->_fileDBResult;
		metaDBResult_cb = cb->_metaDBResult;

		fileDBUpdate_cb = cb->_fileDBUpdate;
		metaDBUpdate_cb = cb->_metaDBUpdate;
	}
}

void TCDB_SetUserCallbackFunctions(TCDB_UserCB *cb)
{
	DEBUG_TCDB_PRINTF("\n");
	if(NULL != cb)
	{
		compareFunction_cb = cb->_compareFunction;
		gatheringTag_cb = cb->_gatheringTag;
	}
}

const char *TCDB_libversion(void)
{
	static const char TCDB_version[] = TCDB_VERSION;
	return TCDB_version;
}

int32_t TCDB_libversion_number(void)
{
	return TCDB_VERSION_NUMBER;
}

void TCDB_SetDebug(int32_t debugLevel)
{
	g_tcdb_debug = debugLevel;
}

int32_t TCDB_Initialize(const char *dbPath, uint32_t maxDBNum, const char *configFilePath)
{
	uint32_t allocSize;
	int32_t ret=-1;

	INFO_TCDB_PRINTF("TCDB Version %s\n", TCDB_libversion());

	if(dbPath != NULL)
	{
		/* 1. Set system config */
		gDB_Config._dirName = newformat(dbPath);
		gDB_Config._maxDBNum = maxDBNum;

		/* 2. Set lod config xml */
		if(gDB_Config._dirName != NULL)
		{
			ret  = loadConfig(configFilePath, &gDB_Config);
			if(ret < 0)
			{
				ERROR_TCDB_PRINTF("Load %s fail, set default config\n",configFilePath);
				if(gDB_Config._dirName != NULL)
				{
					free((char*)gDB_Config._dirName);
					gDB_Config._dirName = NULL;
				}
				ret = defautlConfig(&gDB_Config);
			}

			if(g_tcdb_debug >= TCDB_DEBUG_LEVEL)
			{
				printfConfig(&gDB_Config);
			}
		}

		/* 2. init DBConfig*/
		if(ret == DB_SUCCESS)
		{
			(void)mkdir(gDB_Config._dirName, S_IRWXU|S_IRGRP|S_IXGRP);

			if (sizeof(DB_Info) < (UINT_MAX / gDB_Config._maxDBNum))
			{
				allocSize =  gDB_Config._maxDBNum* sizeof(DB_Info);
				pgDB_Info = malloc(allocSize);
				if(pgDB_Info != NULL)
				{
					(void)memset(pgDB_Info, 0x00, allocSize);
				}
				else
				{
					ret = DB_MEMORY_ALLOC_ERR;
					ERROR_TCDB_PRINTF("malloc fail \n");
				}
			}
			else
			{
				ret = DB_MEMORY_ALLOC_ERR;
				ERROR_TCDB_PRINTF("malloc fail \n");
			}
		}	
#ifdef META_DB_INCLUDE
		/* 2. 1 Meta Init*/
		if( ret == DB_SUCCESS )
		{
			ret = TCDB_Meta_Initialize();
		}
#endif

		/* 3. init DBInfo of each device*/
		if(ret ==DB_SUCCESS)
		{
			uint32_t i;
			for(i=0; i < gDB_Config._maxDBNum; i++)
			{
				initializeDBInfo(i);
			}
		}

		/* 4. Init Mutex */
		if(ret == DB_SUCCESS)
		{
			if(pthread_mutex_init(&dbMutex, NULL) == 0)
			{
				dbMutexPtr = &dbMutex;
				ret = DB_SUCCESS;
			}
		}

		if(ret != DB_SUCCESS)
		{
			releaseConfig(&gDB_Config);
		}
	}
	return ret;
}

void TCDB_Release(void)
{
	uint32_t i;
	INFO_TCDB_PRINTF("\n");

	releaseConfig(&gDB_Config);

	if(pgDB_Info!=NULL)
	{
		for(i=0; i <gDB_Config._maxDBNum; i++)
		{
			Release_DB(i);
		}
		free(pgDB_Info);
		pgDB_Info = NULL;
	}
#ifdef META_DB_INCLUDE
	TCDB_Meta_Release();
#endif

	if(dbMutexPtr != NULL)
	{
		if(pthread_mutex_destroy(dbMutexPtr) ==0)
		{
			dbMutexPtr = NULL;
		}
		else
		{
			ERROR_TCDB_PRINTF("dbMutex mutext destory failed \n");
		}
	}

}

static void * filedb_task_function(void *arg)
{
	int32_t ret;
	pDB_Info pDBInfo = arg;

	INFO_TCDB_PRINTF("dbNum (%d)\n", pDBInfo->_deviceIndex);

	if(pDBInfo  != NULL)
	{
		ret = Make_FileDB((uint32_t)pDBInfo->_deviceIndex, pDBInfo->_mountPath);
		if(ret == DB_SUCCESS)
		{
			pDBInfo->_status = (int32_t)FileDB_Completed;
		}
		else
		{
			if((int32_t)pDBInfo->_forceStop == 1)
			{
				ret = DB_FORCE_STOP;
			}
			pDBInfo->_status = (int32_t)DB_Error;
			ERROR_TCDB_PRINTF("File DB Fail : (%d) - Task End\n", ret);
		}
		pDBInfo->_isRun = 0;
		if((int32_t)pDBInfo->_forceStop != 1)
		{
			if(fileDBResult_cb != NULL)
			{
				fileDBResult_cb((uint32_t)pDBInfo->_deviceIndex, ret);
			}
		}
	}
	else
	{
		ret = DB_ARGUMENT_ERR;
	}

	(void)ret;
	pthread_exit((void *) 0);
}

int32_t TCDB_GenerateFileDB(uint32_t dbNum, const char *mountPath)
{
	int32_t ret = DB_UNKOWN_ERR;
	pDB_Info		pDBInfo;

	INFO_TCDB_PRINTF("dbNum(%d), mountPath(%s)\n", dbNum, mountPath);
	if((pgDB_Info != NULL)&&(dbNum < gDB_Config._maxDBNum) &&(mountPath != NULL))
	{
		pDBInfo = &pgDB_Info[dbNum];
		if(pDBInfo != NULL)
		{
			(void)pthread_mutex_lock(&dbMutex);
			if(((int32_t)pDBInfo->_isRun == 0)&&((int32_t)pDBInfo->_forceStop==0))
			{
				pDBInfo->_isRun =1;
				pDBInfo->_forceStop =0;
				
				pDBInfo->_deviceIndex = (int32_t)dbNum;
				pDBInfo->_handle = NULL;
				pDBInfo->_status = (int32_t)Initialzie_DB;

				(void)memset(pDBInfo->_dbName, 0x00, MAX_PATH_LENGTH);
				(void)strcpy(pDBInfo->_mountPath, mountPath);

				pDBInfo->_totalFolderCount =0;
				pDBInfo->_totalAudioFileCount =0;
				pDBInfo->_totalVideoFileCount =0;
				pDBInfo->_totalImageFileCount =0;

				pDBInfo->_updateAudioCount=0;
				pDBInfo->_updateVideoCount=0;
				pDBInfo->_updateImageCount=0;

				if(pthread_mutex_init(&pDBInfo->_handleLock, NULL)==0)
				{
					if(pthread_create(&pDBInfo->_fileThread, NULL, filedb_task_function, pDBInfo) == 0 )
					{
						pDBInfo->_isFileThreadCreated =1;
						ret = DB_SUCCESS;
					}
					else
					{
						ERROR_TCDB_PRINTF("create DB thread failed(%d)\n", ret);
						ret = DB_CREATE_TASK_ERR;
					}
				}
				else
				{
					ERROR_TCDB_PRINTF("mutex(handlelock) init error (%d)\n", ret);
					ret = DB_CREATE_TASK_ERR;
				}
			}
			else
			{
				ERROR_TCDB_PRINTF("Already DB Task run\n");
				ret = DB_TASK_BUSY;
			}	
			(void)pthread_mutex_unlock(&dbMutex);
		}
	}
	else
	{
		ERROR_TCDB_PRINTF("Bad Argument %d, %s\n", dbNum, mountPath);
		ret = DB_ARGUMENT_ERR;
	}
	return ret;
}

int32_t TCDB_DeleteDB(uint32_t dbNum)
{
	int32_t ret;

	INFO_TCDB_PRINTF("dbNum(%d)\n", dbNum);

	if((pgDB_Info != NULL)&&(dbNum < gDB_Config._maxDBNum) )
	{
		pDB_Info		pDBInfo;
		pDBInfo = &pgDB_Info[dbNum];
		if(pDBInfo != NULL)
		{
			pDBInfo->_forceStop = 1;
			if((int32_t)pDBInfo->_isFileThreadCreated == 1)
			{
				void *res;
				ret = pthread_join(pDBInfo->_fileThread, &res);
				if(ret !=0)
				{
					ERROR_TCDB_PRINTF("File DB Thread join failed(%d)\n", ret);
				}
				pDBInfo->_isFileThreadCreated = 0;
			}
			if((int32_t)pDBInfo->_isMetaThreadCreated == 1)
			{
				void *res;
				ret = pthread_join(pDBInfo->_metaThread, &res);
				if(ret !=0)
				{
					ERROR_TCDB_PRINTF("Meta DB Thread join failed(%d)\n", ret);
				}
				pDBInfo->_isMetaThreadCreated =  0;
			}
			pDBInfo->_status = (int32_t)Empty_DB;
			pDBInfo->_isRun =0;
			pDBInfo->_forceStop = 0;

			ret = Sql_DropTableAll(pDBInfo->_handle);
			if(ret == SQLITE_OK)
			{
				ret = DB_SUCCESS;
			}

			(void)pthread_mutex_lock(&pDBInfo->_handleLock);
			if(pDBInfo->_handle != NULL)
			{
#ifdef META_DB_INCLUDE
				pMetaBrwsHandle pMetaHandle;
#endif
				(void)Sql_Close_DB(pDBInfo->_handle);
				pDBInfo->_handle = NULL;
#ifdef META_DB_INCLUDE
				pMetaHandle = &pgMetaBrwsHandle[dbNum];
				if(pMetaHandle != NULL)
				{
					pMetaHandle->_db = NULL;
				}
#endif
			}
			(void)pthread_mutex_unlock(&pDBInfo->_handleLock);
			(void)pthread_mutex_destroy(&pDBInfo->_handleLock);
		}
		else
		{
			ERROR_TCDB_PRINTF("Not Initialize DB : (%d)\n", dbNum);
			ret = DB_NOT_INITIALIZE;
		}
	}
	else
	{
		ERROR_TCDB_PRINTF("Invalid Argument : (%d)\n", dbNum);
		ret = DB_ARGUMENT_ERR;
	}
	INFO_TCDB_PRINTF("End\n");
	return ret;
}

uint32_t TCDB_GetTotalFolderCount(uint32_t dbNum)
{
	pDB_Info		pDBInfo;
	uint32_t totalCount = 0;
	DEBUG_TCDB_PRINTF("dbNum (%d)\n", dbNum);
	if((pgDB_Info != NULL)&&(dbNum < gDB_Config._maxDBNum))
	{
		pDBInfo = &pgDB_Info[dbNum];
		if(pDBInfo != NULL)
		{
			totalCount = pDBInfo->_totalFolderCount;
		}
		else
		{
			ERROR_TCDB_PRINTF("Not Initialize DB : (%d)\n", dbNum);
		}
	}
	else
	{
		ERROR_TCDB_PRINTF("Invalid Argument : (%d)\n", dbNum);
	}
	DEBUG_TCDB_PRINTF("return TotalCount (%d)\n", totalCount);
	return totalCount;
}

uint32_t TCDB_GetTotalFileCount(uint32_t dbNum, ContentsType type)
{
	pDB_Info		pDBInfo;
	uint32_t totalCount = 0;

	DEBUG_TCDB_PRINTF("dbNum (%d), type (%d)\n", (int32_t)dbNum, (int32_t)type);
	if((pgDB_Info != NULL)&&(dbNum < gDB_Config._maxDBNum))
	{
		pDBInfo = &pgDB_Info[dbNum];
		if(pDBInfo != NULL)
		{
			if(type == AudioContents)
			{
				totalCount= pDBInfo->_totalAudioFileCount;
			}
			else if(type == VideoContents)
			{
				totalCount= pDBInfo->_totalVideoFileCount;
			}
			else if(type == ImageContents)
			{
				totalCount= pDBInfo->_totalImageFileCount;
			}
			else
			{
				(void)totalCount;
			}
		}
		else
		{
			ERROR_TCDB_PRINTF("Not Initialize DB index : (%d)\n", dbNum);
		}
	}
	else
	{
		ERROR_TCDB_PRINTF("Invalid Argument : (%d)\n", dbNum);
	}
	return (uint32_t)totalCount;
}

int32_t TCDB_AddAllFolderList(uint32_t dbNum, int32_t (*callback)(void* args, int32_t rowCount, int32_t folderIndex), void *pArg)
{
	pDB_Info		pDBInfo;
	int32_t ret = DB_ARGUMENT_ERR;

	if((pgDB_Info != NULL)&&(dbNum < gDB_Config._maxDBNum)&&(callback != NULL) &&(pArg != NULL))
	{
		pDBInfo = &pgDB_Info[dbNum];
		if(pDBInfo != NULL)
		{
			(void)pthread_mutex_lock(&pDBInfo->_handleLock);
			ret = Sql_AddAllFolderList(pDBInfo->_handle, callback, pArg, gCollateFunction);
			if(ret != 0)
			{
				ret = DB_INVALIDE_INDEX_ERR;
			}
			(void)pthread_mutex_unlock(&pDBInfo->_handleLock);
		}
	}
	else
	{
		ERROR_TCDB_PRINTF("Invalid Argument : (%d)\n", dbNum);
	}

	return ret;
}


int32_t TCDB_AddSubFolderList(uint32_t dbNum, uint32_t parentFolderIndex,int32_t (*callback)(void* args, int32_t rowCount, int32_t folderIndex), void *pArg)
{
	pDB_Info		pDBInfo;
	int32_t ret = DB_ARGUMENT_ERR;

	if((pgDB_Info != NULL)&&(dbNum < gDB_Config._maxDBNum))
	{
		pDBInfo = &pgDB_Info[dbNum];
		if((pDBInfo != NULL) && (callback != NULL) && (pArg != NULL))
		{
			(void)pthread_mutex_lock(&pDBInfo->_handleLock);
			ret = Sql_AddSubFolderList(pDBInfo->_handle,parentFolderIndex, callback, pArg, gCollateFunction);
			if(ret != 0)
			{
				ret = DB_INVALIDE_INDEX_ERR;
			}
			(void)pthread_mutex_unlock(&pDBInfo->_handleLock);
		}
	}
	else
	{
		ERROR_TCDB_PRINTF("Invalid Argument : (%d)\n", dbNum);
	}

	return ret;
}

int32_t TCDB_AddSubFileList(uint32_t dbNum, uint32_t parentFolderIndex, ContentsType type, int32_t (*callback)(void* args, int32_t rowCount, int32_t fileIndex), void *pArg)
{
	pDB_Info		pDBInfo;
	int32_t ret = DB_ARGUMENT_ERR;

	if((pgDB_Info != NULL)&&(dbNum < gDB_Config._maxDBNum))
	{
		pDBInfo = &pgDB_Info[dbNum];
		if((pDBInfo != NULL) && (callback != NULL) && (pArg != NULL))
		{
			(void)pthread_mutex_lock(&pDBInfo->_handleLock);
			ret = Sql_AddSubFileList(pDBInfo->_handle, parentFolderIndex, type, callback, pArg, gCollateFunction);
			if(ret != 0)
			{
				ret = DB_INVALIDE_INDEX_ERR;
			}
			(void)pthread_mutex_unlock(&pDBInfo->_handleLock);
		}
	}
	else
	{
		ERROR_TCDB_PRINTF("Invalid Argument : (%d)\n", dbNum);
	}

	return ret;
}

int32_t TCDB_AddAllFileList(uint32_t dbNum, ContentsType type, int32_t (*callback)(void* args, int32_t rowCount, int32_t fileIndex), void *pArg)
{
	pDB_Info		pDBInfo;
	int32_t ret = DB_ARGUMENT_ERR;

	DEBUG_TCDB_PRINTF("dbNum (%d), type (%d)\n", dbNum, type);

	if((pgDB_Info != NULL)&&(dbNum < gDB_Config._maxDBNum))
	{
		pDBInfo = &pgDB_Info[dbNum];
		if((pDBInfo != NULL) && (callback != NULL) && (pArg != NULL))
		{
			(void)pthread_mutex_lock(&pDBInfo->_handleLock);
			ret = Sql_AddAllFileList(pDBInfo->_handle,type, callback, pArg, gCollateFunction);
			if(ret != 0)
			{
				ret = DB_INVALIDE_INDEX_ERR;
			}
			(void)pthread_mutex_unlock(&pDBInfo->_handleLock);
		}
	}
	else
	{
		ERROR_TCDB_PRINTF("Invalid Argument : (%d)\n", dbNum);
	}

	return ret;
}

uint32_t TCDB_GetSubFolderCount(uint32_t dbNum, uint32_t folderIndex)
{
	pDB_Info		pDBInfo;
	int32_t rc;
	uint32_t ret = (uint32_t)INVALID_INDEX;
	DEBUG_TCDB_PRINTF("dbNum (%d), folderIndex (%d)\n", dbNum, folderIndex);
	if((pgDB_Info != NULL)&&(dbNum < gDB_Config._maxDBNum))
	{
		pDBInfo = &pgDB_Info[dbNum];
		if(pDBInfo != NULL)
		{
			(void)pthread_mutex_lock(&pDBInfo->_handleLock);
			rc = Sql_GetSubFolderCount(pDBInfo->_handle, folderIndex);
			if(rc > 0)
			{
				ret = (uint32_t)rc;
			}
			(void)pthread_mutex_unlock(&pDBInfo->_handleLock);
		}
	}
	else
	{
		ERROR_TCDB_PRINTF("Invalid Argument : (%d)\n", dbNum);
	}

	return ret;
}

uint32_t TCDB_GetSubFileCount(uint32_t dbNum, uint32_t folderIndex ,ContentsType type)
{
	pDB_Info		pDBInfo;
	int32_t rc;
	uint32_t ret = 0;

	DEBUG_TCDB_PRINTF("dbNum (%d), folderIndex (%d), type (%d)\n", (int32_t)dbNum, (int32_t)folderIndex, (int32_t)type);

	if((pgDB_Info != NULL)&&(dbNum < gDB_Config._maxDBNum))
	{
		pDBInfo = &pgDB_Info[dbNum];
		if(pDBInfo != NULL)
		{
			(void)pthread_mutex_lock(&pDBInfo->_handleLock);
			rc = Sql_GetSubFileCount(pDBInfo->_handle, folderIndex, type);
			if(rc > 0)
			{
				ret = (uint32_t)rc;
			}
			(void)pthread_mutex_unlock(&pDBInfo->_handleLock);
		}
	}
	else
	{
		ERROR_TCDB_PRINTF("Invalid Argument : (%d)\n", dbNum);
	}

	return ret;
}

int32_t TCDB_GetFolderName(uint32_t dbNum, uint32_t folderIndex, char *folderName, uint32_t bufSize)
{
	pDB_Info		pDBInfo;
	int32_t ret = DB_UNKOWN_ERR;
	int32_t isBufLack = -1;
	DEBUG_TCDB_PRINTF("dbNum (%d), folderIndex (%d) \n", dbNum, folderIndex );
	if((pgDB_Info != NULL)&&(dbNum < gDB_Config._maxDBNum)&& (folderName!=NULL) &&((int32_t)bufSize!=0))
	{
		pDBInfo = &pgDB_Info[dbNum];
		if(pDBInfo != NULL)
		{
			(void)pthread_mutex_lock(&pDBInfo->_handleLock);
			if(Sql_GetFolderName(pDBInfo->_handle,folderIndex, folderName, bufSize, &isBufLack) == SQLITE_OK)
			{
				if(isBufLack == 0)
				{
					ret = DB_SUCCESS;
				}
				else
				{
					ret = DB_NOT_ENOUGH_BUFFER;
				}
			}
			else
			{
				ret = DB_SQLITE_SELECT_ERR;
			}
			(void)pthread_mutex_unlock(&pDBInfo->_handleLock);
		}
		else
		{
			ret  = DB_NOT_INITIALIZE;
			ERROR_TCDB_PRINTF("Not Initialize DB index : (%d)\n", dbNum);
		}
	}
	else
	{
		ret = DB_ARGUMENT_ERR;
		ERROR_TCDB_PRINTF("Invalid Argument : (%d), folderIndex (%d)\n", dbNum, folderIndex);
	}

	return ret;
}

int32_t TCDB_GetFolderFullPath(uint32_t dbNum, uint32_t folderIndex, char *path, uint32_t bufSize)
{
	pDB_Info		pDBInfo;
	int32_t ret;
	int32_t isBufLack = -1;
	DEBUG_TCDB_PRINTF("dbNum (%d), folderIndex (%d)\n", dbNum, folderIndex);
	if((pgDB_Info != NULL)&&(dbNum < gDB_Config._maxDBNum)&& (path !=NULL))
	{
		pDBInfo = &pgDB_Info[dbNum];
		if(pDBInfo != NULL)
		{
			(void)pthread_mutex_lock(&pDBInfo->_handleLock);
			if(Sql_GetFolderFullpath(pDBInfo->_handle, folderIndex, path, bufSize, &isBufLack) == SQLITE_OK)
			{
				if(isBufLack == 0)
				{
					ret = DB_SUCCESS;
				}
				else
				{
					ret = DB_NOT_ENOUGH_BUFFER;
				}
			}
			else
			{
				ret = DB_SQLITE_SELECT_ERR;
			}
			(void)pthread_mutex_unlock(&pDBInfo->_handleLock);
		}
		else
		{
			ret  = DB_NOT_INITIALIZE;
			ERROR_TCDB_PRINTF("Not Initialize DB index : (%d)\n", dbNum);
		}
	}
	else
	{
		ret = DB_ARGUMENT_ERR;
		ERROR_TCDB_PRINTF("Invalid Argument : (%d), folderIndex (%d)\n", dbNum, folderIndex);
	}

	return ret;
}

uint32_t TCDB_GetParentFolderIndexofFolder(uint32_t dbNum, uint32_t folderIndex)
{
	pDB_Info		pDBInfo;
	int32_t rc;
	uint32_t ret = (uint32_t)INVALID_INDEX;
	if((pgDB_Info != NULL)&&(dbNum < gDB_Config._maxDBNum))
	{
		pDBInfo = &pgDB_Info[dbNum];
		if(pDBInfo != NULL)
		{
			(void)pthread_mutex_lock(&pDBInfo->_handleLock);
			rc = Sql_GetFolderParentIndex(pDBInfo->_handle, folderIndex);
			if(rc > 0)
			{
				ret = (uint32_t)rc;
			}
			(void)pthread_mutex_unlock(&pDBInfo->_handleLock);
		}
	}
	else
	{
		ERROR_TCDB_PRINTF("Invalid Argument : (%d)\n", dbNum);
	}

	return ret;
}


int32_t TCDB_GetFileName(uint32_t dbNum, uint32_t fileIndex, char *fileName, uint32_t bufSize,ContentsType type)
{
	pDB_Info		pDBInfo;
	int32_t ret;
	int32_t isBufLack = -1;
	DEBUG_TCDB_PRINTF("dbNum (%d), fileIndex (%d), type (%d)\n", (int32_t)dbNum, (int32_t)fileIndex, (int32_t)type);
	if((pgDB_Info != NULL)&&(dbNum < gDB_Config._maxDBNum)&& (fileName !=NULL))
	{
		pDBInfo = &pgDB_Info[dbNum];
		if(pDBInfo != NULL)
		{
			(void)pthread_mutex_lock(&pDBInfo->_handleLock);
			if( Sql_GetFileName(pDBInfo->_handle, fileIndex, fileName, bufSize, &isBufLack,type) == SQLITE_OK)
			{
				if(isBufLack ==0)
				{
					ret = DB_SUCCESS;
				}
				else
				{
					ret = DB_NOT_ENOUGH_BUFFER;
				}
			}
			else
			{
				ret = DB_SQLITE_SELECT_ERR;
			}
			(void)pthread_mutex_unlock(&pDBInfo->_handleLock);
		}
		else
		{
			ret  = DB_NOT_INITIALIZE;
			ERROR_TCDB_PRINTF("Not Initialize DB index : (%d)\n", dbNum);
		}
	}
	else
	{
		ret = DB_ARGUMENT_ERR;
		ERROR_TCDB_PRINTF("Invalid Argument : (%d), fileIndex (%d), type(%d)\n", (int32_t)dbNum, (int32_t)fileIndex, (int32_t)type);
	}

	return ret;
}

int32_t TCDB_GetFileFullPath(uint32_t dbNum, uint32_t fileIndex, char *path, uint32_t bufSize,ContentsType type)
{
	pDB_Info		pDBInfo;
	int32_t ret;

	if((pgDB_Info != NULL)&&(dbNum < gDB_Config._maxDBNum)&& (path !=NULL) )
	{
		pDBInfo = &pgDB_Info[dbNum];
		if(pDBInfo != NULL)
		{
			int32_t err =SQLITE_ERROR;
			int32_t parentIndex;
			int32_t isBufLack = -1;

			(void)memset(path, 0x00, MAX_PATH_LENGTH);
			(void)pthread_mutex_lock(&pDBInfo->_handleLock);
			parentIndex = Sql_GetFileParentIndex(pDBInfo->_handle, fileIndex, type);
			if(parentIndex > 0)
			{
				err = Sql_GetFolderFullpath(pDBInfo->_handle, (uint32_t)parentIndex, path,bufSize, &isBufLack);
				if(err == SQLITE_OK)
				{
					if(isBufLack == 0)
					{
						uint32_t pathLength;
						uint32_t fileBufSize;

						pathLength = strlen(path);
						path[pathLength] = '/';
						pathLength++;
						fileBufSize = bufSize -pathLength;
						err = Sql_GetFileName(pDBInfo->_handle,fileIndex, &path[pathLength],fileBufSize, &isBufLack, type);
						if(err== SQLITE_OK)
						{
							if(isBufLack ==0)
							{
								ret = DB_SUCCESS;
							}
							else
							{
								ret = DB_NOT_ENOUGH_BUFFER;
							}
						}
					}
					else
					{
						ret = DB_NOT_ENOUGH_BUFFER;
					}
				}
			}

			if(err != SQLITE_OK)
			{
				ret = DB_SQLITE_SELECT_ERR;
			}
			(void)pthread_mutex_unlock(&pDBInfo->_handleLock);
		}
		else
		{
			ret  = DB_NOT_INITIALIZE;
			ERROR_TCDB_PRINTF("Not Initialize DB index : (%d)\n", dbNum);
		}
	}
	else
	{
		ret = DB_ARGUMENT_ERR;
		ERROR_TCDB_PRINTF("Invalid Argument : (%d), fileIndex (%d), type(%d)\n", (int32_t)dbNum, (int32_t)fileIndex, (int32_t)type);
	}

	return ret;
}

int32_t TCDB_GetFileInode(uint32_t dbNum, uint32_t fileIndex, ContentsType type)
{
	pDB_Info		pDBInfo;
	int32_t rc;
	uint32_t ret = 0;
	DEBUG_TCDB_PRINTF("dbNum (%d), fileIndex (%d), type (%d)\n", (int32_t)dbNum, (int32_t)fileIndex, (int32_t)type);
	if((pgDB_Info != NULL)&&(dbNum < gDB_Config._maxDBNum))
	{
		pDBInfo = &pgDB_Info[dbNum];
		if(pDBInfo != NULL)
		{
			(void)pthread_mutex_lock(&pDBInfo->_handleLock);
			rc = Sql_GetFileInode(pDBInfo->_handle,fileIndex, type);
			if(rc > 0)
			{
				ret = (uint32_t)rc;
			}
			(void)pthread_mutex_unlock(&pDBInfo->_handleLock);
		}
	}
	else
	{
		ERROR_TCDB_PRINTF("Invalid Argument : (%d)\n", dbNum);
	}

	return (int32_t)ret;
}

int32_t TCDB_AddFile_withFullpath(uint32_t dbNum, const char *path, ContentsType type)
{
	pDB_Info		pDBInfo;
	int32_t ret = DB_UNKOWN_ERR;
	int32_t rc;
	DEBUG_TCDB_PRINTF("dbNum (%d), path(%s), type (%d)\n", (int32_t)dbNum, (char *)path, (int32_t)type);
	if((pgDB_Info != NULL)&&(dbNum < gDB_Config._maxDBNum)&& (path !=NULL) )
	{
		int32_t folderIndex;
		char * realFolderPath;
		int32_t realFolderlength, tempLen;
		char *strOffset;

		strOffset = strrchr(path, (int32_t)'/');
		if(strOffset != NULL)
		{
			tempLen = strlen(strOffset);
		}
		else
		{
			tempLen = 0;
		}

		realFolderlength = (int32_t)strlen(path) - tempLen;

		realFolderPath = malloc((size_t)(realFolderlength + 1));
		pDBInfo = &pgDB_Info[dbNum];
		if((pDBInfo != NULL)&&(realFolderPath != NULL))
		{
			(void)memset(realFolderPath, 0x00, (size_t)(realFolderlength + 1));
			(void)memcpy(realFolderPath, path, (size_t)realFolderlength);

			(void)pthread_mutex_lock(&pDBInfo->_handleLock);
			folderIndex = Sql_FindFolderIndex(pDBInfo->_handle, realFolderPath);
			if(folderIndex == 0)
			{
				//Insert New Folder : Not suppport yet
				ret = DB_ARGUMENT_ERR;
			}
			else if(folderIndex > 0)
			{
				//Current Folder already exist, Insert only file list
				const char *name = strrchr(path, (int32_t)'/');
				if(name != NULL)
				{
					rc = Sql_InsertFileList(pDBInfo->_handle, name, folderIndex, 0,type);
					if(rc == SQLITE_OK)
					{
						ret = DB_SUCCESS;
					}
					else
					{
						ret = DB_SQLITE_INSERT_ERR;
					}
				}
				else
				{
					ret = DB_ARGUMENT_ERR;
				}
			}
			else
			{
				//error
				ret = DB_ARGUMENT_ERR;
			}
			(void)pthread_mutex_unlock(&pDBInfo->_handleLock);
		}

		if(realFolderPath != NULL)
		{
			free(realFolderPath);
		}
	}

	return ret;
}

uint32_t TCDB_GetParentFolderIndexofFile(uint32_t dbNum, uint32_t fileIndex, ContentsType type)
{
	pDB_Info		pDBInfo;
	int32_t rc;
	uint32_t ret = 0;
	if((pgDB_Info != NULL)&&(dbNum < gDB_Config._maxDBNum))
	{
		pDBInfo = &pgDB_Info[dbNum];
		if(pDBInfo != NULL)
		{
			(void)pthread_mutex_lock(&pDBInfo->_handleLock);
			rc = Sql_GetFileParentIndex(pDBInfo->_handle, fileIndex, type);
			if(rc > 0)
			{
				ret = (uint32_t)rc;
			}
			(void)pthread_mutex_unlock(&pDBInfo->_handleLock);
		}
	}
	else
	{
		ERROR_TCDB_PRINTF("Invalid Argument : (%d)\n", dbNum);
	}

	return ret;
}

int32_t TCDB_DeleteFile(uint32_t dbNum, uint32_t fileIndex,ContentsType type)
{
	pDB_Info		pDBInfo;
	int32_t ret;
	int32_t rc;
	DEBUG_TCDB_PRINTF("dbNum (%d), fileIndex (%d), type (%d)\n", (int32_t)dbNum, (int32_t)fileIndex, (int32_t)type);
	if((pgDB_Info != NULL)&&(dbNum < gDB_Config._maxDBNum))
	{
		pDBInfo = &pgDB_Info[dbNum];
		if(pDBInfo != NULL)
		{
			(void)pthread_mutex_lock(&pDBInfo->_handleLock);
			rc = Sql_DeleteFileList(pDBInfo->_handle, fileIndex, type);
			if(rc == SQLITE_OK)
			{
				ret = DB_SUCCESS;
			}
			else
			{
				ret = DB_SQLITE_INSERT_ERR;
			}
			(void)pthread_mutex_unlock(&pDBInfo->_handleLock);
		}
		else
		{
			ret = DB_NOT_INITIALIZE;
		}
	}
	else
	{
		ret = DB_ARGUMENT_ERR;
	}

	return ret;
}

int32_t TCDB_SQL_Errcode(uint32_t dbNum)
{
	pDB_Info		pDBInfo;
	int32_t rc = SQLITE_OK;
	
	if((pgDB_Info != NULL)&&(dbNum < gDB_Config._maxDBNum))
	{
		pDBInfo = &pgDB_Info[dbNum];
		(void)pthread_mutex_lock(&pDBInfo->_handleLock);
		if((pDBInfo != NULL)&&(pDBInfo->_handle != NULL))
		{
			rc = sqlite3_errcode(pDBInfo->_handle);
		}
		(void)pthread_mutex_unlock(&pDBInfo->_handleLock);
	}
	return rc;
}

int32_t TCDB_SQL_Extended_ErrCode(uint32_t dbNum)
{
	pDB_Info		pDBInfo;
	int32_t rc = SQLITE_OK;
	
	if((pgDB_Info != NULL)&&(dbNum < gDB_Config._maxDBNum))
	{
		pDBInfo = &pgDB_Info[dbNum];
		(void)pthread_mutex_lock(&pDBInfo->_handleLock);
		if((pDBInfo != NULL)&&(pDBInfo->_handle != NULL))
		{
			rc = sqlite3_extended_errcode(pDBInfo->_handle);
		}
		(void)pthread_mutex_unlock(&pDBInfo->_handleLock);
	}
	return rc;
}

static void initializeDBInfo(uint32_t dbNum)
{
	pDB_Info		pDBInfo;
	if(dbNum < gDB_Config._maxDBNum)
	{
		if(pgDB_Info != NULL)
		{
			pDBInfo = &pgDB_Info[dbNum];

			pDBInfo->_deviceIndex = -1;
			pDBInfo->_handle = NULL;
			pDBInfo->_status = (int32_t)Empty_DB;

			(void)memset(pDBInfo->_dbName, 0x00, MAX_PATH_LENGTH);
			(void)memset(pDBInfo->_mountPath, 0x00, MAX_PATH_LENGTH);

			pDBInfo->_totalFolderCount =0;
			pDBInfo->_totalAudioFileCount =0;
			pDBInfo->_totalVideoFileCount =0;
			pDBInfo->_totalImageFileCount =0;

			pDBInfo->_updateAudioCount=0;
			pDBInfo->_updateVideoCount=0;
			pDBInfo->_updateImageCount=0;

			pDBInfo->_isRun =0;
			pDBInfo->_forceStop =0;
		}
	}
}

static pDB_Info  GetDBInfo(uint32_t dbNum)
{
	uint32_t i;
	pDB_Info		pDBInfo, pFindDBInfo= NULL;

	for(i=0; i < gDB_Config._maxDBNum; i++)
	{
		pDBInfo = &pgDB_Info[dbNum];
		if((uint32_t)pDBInfo->_deviceIndex == dbNum)
		{
			pFindDBInfo = pDBInfo;
		}
	}
	return pFindDBInfo;
}

static pDB_Info  GetEmptyDBInfo(uint32_t dbNum)
{
	uint32_t i;
	pDB_Info		pDBInfo, pFindDBInfo= NULL;

	for(i=0; i < gDB_Config._maxDBNum; i++)
	{
		pDBInfo = &pgDB_Info[dbNum];
		if(pDBInfo->_status == (int32_t)Empty_DB)
		{
			pFindDBInfo = pDBInfo;
		}
	}
	return pFindDBInfo;
}

const char * TCDB_SQL_ErrStr(int32_t errcode)
{
	return sqlite3_errstr(errcode);
}

static ContentsType CheckSupportCodecList(DB_Config *dbConfig ,const char *name)
{
	ContentsType ret = NoneContents;
	int32_t find;
	const char *extName;

	extName = strrchr(name, (int32_t)'.');
	
	find = FindSupportExtName(dbConfig->_audioTableInfo._pSupportList, extName);
	if(find == 1)
	{
		ret = AudioContents;
	}
	else
	{
		find = FindSupportExtName(dbConfig->_videoTableInfo._pSupportList, extName);
		if(find == 1)
		{
			ret = VideoContents;
		}
		else
		{
			find = FindSupportExtName(dbConfig->_imageTableInfo._pSupportList, extName);
			if(find == 1)
			{
				ret = ImageContents;
			}
		}
	}

	return ret;
}

static int32_t Init_FileDB(uint32_t dbNum)
{
	pDB_Info		pDBInfo;
	int32_t err;
	int32_t ret;

	INFO_TCDB_PRINTF("Init File DB Start\n");

	if((pgDB_Info != NULL)&&(dbNum < gDB_Config._maxDBNum))
	{
		pDBInfo = &pgDB_Info[dbNum];
		ret = Set_DatabaseName(dbNum);
		if((ret == DB_SUCCESS)&&(pDBInfo != NULL))
		{
			err = SQLITE_OK;
			if(pDBInfo->_handle != NULL)
			{
				err = Sql_Close_DB(pDBInfo->_handle);
			}

			if(err == SQLITE_OK)
			{
				if( chdir(gDB_Config._dirName) == -1 )
				{
					char mkdirCmd[4096];
					INFO_TCDB_PRINTF("(%s) is not exist. Create Dir\n", gDB_Config._dirName);
					(void)sprintf(mkdirCmd, "mkdir -p %s",gDB_Config._dirName);
					(void)system(mkdirCmd);

					if( chdir(gDB_Config._dirName) == -1 )
					{
						ERROR_TCDB_PRINTF("dir access ERROR_TCDB_PRINTF(%s)\n",gDB_Config._dirName);
						/* For print error log */
						if( mkdir(gDB_Config._dirName, S_IRWXU|S_IRGRP|S_IXGRP) == -1)
						{
							ERROR_TCDB_PRINTF("mkdir error (%s), errno(%d) - %s\n",gDB_Config._dirName, errno, strerror (errno));
						}
						ret = DB_DIR_OPEN_ERR;
					}
				}

				if(ret == DB_SUCCESS)
				{
					(void)remove(pDBInfo->_dbName);
					pDBInfo->_handle = Sql_Open_DB(pDBInfo->_dbName);
					if(pDBInfo->_handle)
					{
						INFO_TCDB_PRINTF("Init File DB Completed\n");
					}
					else
					{
						ERROR_TCDB_PRINTF("DB Open(or create) Fail\n");
						ret = DB_ACCESS_FS_ERR;
					}
				}
			}
		}
	}
	else
	{
		ERROR_TCDB_PRINTF("Bad Arguement\n");
		ret = DB_ARGUMENT_ERR;
	}
	return  ret;
}

static void Release_DB(uint32_t dbNum)
{
	DB_Info *pDBInfo;

	INFO_TCDB_PRINTF("DB Number (%d)\n",dbNum);

	if(pgDB_Info!=NULL)
	{
		pDBInfo = &pgDB_Info[dbNum];
		if(pDBInfo != NULL)
		{
			pDBInfo->_deviceIndex = -1;

			if(pDBInfo->_handle != NULL)
			{
				(void)Sql_Close_DB(pDBInfo->_handle);
				pDBInfo->_handle = NULL;
			}
		}
	}
}

static int32_t CreateCollationFuction( sqlite3* db, int32_t(*xCompare)(void *data, int32_t len1, const void* in1, int32_t len2, const void* in2))
{
	int32_t ret;
	int32_t rc;

	DEBUG_TCDB_PRINTF("Create Compare Function\n");
		
	if(( db != NULL ) && (xCompare  != NULL))
	{
		rc = Sql_CreateCollation( db, CUSTOM_COLLATEFUNCTION, SQLITE_UTF8,  NULL,  xCompare);
		if(rc == SQLITE_OK)
		{
			gCollateFunction = CUSTOM_COLLATEFUNCTION;
			ret =  DB_SUCCESS;
		}			
		else
		{
			ret = DB_SQLITE_INITTABLE;
			ERROR_TCDB_PRINTF("Create Colltion Function Fail (%d)\n", ret);
		}
	}
	else
	{
		ret = DB_ARGUMENT_ERR;
	}	

	return ret;
}

static int32_t Make_FileDB(uint32_t dbNum, const char *mountPath)
{
	int32_t ret;
	int32_t err;
	pDB_Info		pDBInfo= NULL;
	DeviceTable_Info deviceInfo;

	INFO_TCDB_PRINTF("dbNum(%d), mountPath(%s)\n", dbNum, mountPath);

	if((pgDB_Info != NULL)&&(dbNum < gDB_Config._maxDBNum) &&(mountPath != NULL))
	{
		pDBInfo = &pgDB_Info[dbNum];
		pDBInfo->_deviceIndex = (int32_t)dbNum;
		pDBInfo->_status = (int32_t)FileDB_Starting;
		ret = Init_FileDB(dbNum);
		if((ret == DB_SUCCESS)&&((int32_t)pDBInfo->_forceStop == 0))
		{
			err = Sql_SetPragma(pDBInfo->_handle);
			if(err != SQLITE_OK)
			{
				ret = DB_SQLITE_PRAGMA_ERR;
			}
			else
			{
				/* sqlite transcation mode */
				err= Sql_Transaction_Start(pDBInfo->_handle);
				if(err != SQLITE_OK)
				{
					ret = DB_SQLITE_PRAGMA_ERR;
				}
				else
				{
					err = Sql_InitTables(pDBInfo->_handle);
					if(err == SQLITE_OK)
					{
						ret=DB_SUCCESS;
					}
					else
					{
						ret=DB_CREATE_TABLE_ERR;
					}
				}
			}

			if((ret == DB_SUCCESS)&&(compareFunction_cb != NULL))
			{ 
				ret = CreateCollationFuction(pDBInfo->_handle, compareFunction_cb);
			}		
			
			if((ret == DB_SUCCESS)&&((int32_t)pDBInfo->_forceStop == 0))
			{
				char pathBuffer[MAX_PATH_LENGTH];
				uint32_t length = strlen(mountPath);

				(void)memset(pathBuffer, 0x00, MAX_PATH_LENGTH);
				(void)strncpy(pathBuffer, mountPath, length);

				if (*(pathBuffer + length - 1) == '/')
				{
					*(pathBuffer + length - 1) = '\0';
				}

				/* set Device Info */
				deviceInfo._rootPath = pathBuffer;
				deviceInfo._used = 0;
				deviceInfo._folderCount = 0;
				deviceInfo._audioFileCount = 0;
				deviceInfo._videoFileCount = 0;
				deviceInfo._imageFileCount = 0;
				err = Sql_InsertDeviceInfo(pDBInfo->_handle,  &deviceInfo);
				if(err == SQLITE_OK)
				{
					ret=DB_SUCCESS;
				}
				else
				{
					ret=DB_CREATE_TABLE_ERR;
				}

				if(ret == DB_SUCCESS)
				{
					pDBInfo->_status = (int32_t)FileDB_Starting;
					ret = Search_FileDB(pDBInfo, pathBuffer);	//temp functoin
				}
				else
				{
					ret = DB_SQLITE_INSERT_ERR;
					pDBInfo->_status =  (int32_t)DB_Error;
					ERROR_TCDB_PRINTF("Make File DB failed (%d)\n", ret);
				}

			}
			else
			{
				ERROR_TCDB_PRINTF("Init File Table failed\n");
			}

			err = Sql_Transaction_End(pDBInfo->_handle);
			if(err != SQLITE_OK)
			{
				if(err == SQLITE_FULL)
				{
					ret = DB_DISK_FULL_ERR;
					ERROR_TCDB_PRINTF("Disk Full : Please Check Disk free space\n");
				}
				else
				{
					ret = DB_SQLITE_PRAGMA_ERR;
				}
			}
		}

	}
	else
	{
		ERROR_TCDB_PRINTF("Bad Argument %d, %s\n", dbNum, mountPath);
		ret = DB_ARGUMENT_ERR;
	}

	if((pDBInfo != NULL)&&((int32_t)pDBInfo->_forceStop == 1))
	{
		ret = DB_FORCE_STOP;
	}

	if(ret != DB_SUCCESS)
	{
		ERROR_TCDB_PRINTF("Generate File DB fail (%d)\n", ret);
		Release_DB(dbNum);
	}

	INFO_TCDB_PRINTF("End (%d)\n", ret);
	return ret;
}

static int32_t Search_FileDB(pDB_Info pDBInfo, const char *mountPath)
{
	int32_t ret, err;
	FolderTable_Info folderInfo;
	sqlite3 * db;
	int32_t parentIndex=0;
	uint32_t curFolderIndex;

	INFO_TCDB_PRINTF("Make File DB (%s)\n", mountPath);
	if((pDBInfo != NULL)&&(mountPath != NULL) && (pDBInfo->_handle != NULL)&&((int32_t)pDBInfo->_forceStop == 0))
	{
		db = pDBInfo->_handle;
		/* 1. Set Root folder */
		DEBUG_TCDB_PRINTF("Insert Root Folder \n");

		folderInfo._name = (char *)"Root";
		folderInfo._fullPath = (char *)mountPath;
		folderInfo._parentIndex = (uint32_t)parentIndex;
		folderInfo._inode = (uint32_t)ROOT_INODE;			/* Do not use the inode in root folder. */
		pDBInfo->_totalFolderCount =1;
		err = Sql_InsertFolderList(db, &folderInfo);
		if(err == SQLITE_OK)
		{
			ret=DB_SUCCESS;
		}
		else
		{
			ret=DB_CREATE_TABLE_ERR;
		}

		if(ret == DB_SUCCESS)
		{
			parentIndex++;
			ret = Get_DirEntry(pDBInfo, mountPath, (uint32_t)parentIndex);
		}

		DEBUG_TCDB_PRINTF("total Folder (%d), max Folder (%d), forceStop(%d)\n",
			pDBInfo->_totalFolderCount, gDB_Config._maxFolderNum,pDBInfo->_forceStop);

		if(ret == DB_SUCCESS)
		{
			curFolderIndex =2;
			while((curFolderIndex <= pDBInfo->_totalFolderCount)&&(curFolderIndex <= gDB_Config._maxFolderNum)&&((int32_t)pDBInfo->_forceStop==0))
			{
				char folderPath[MAX_PATH_LENGTH];
				int32_t isBufLack;

				(void)memset(folderPath, 0x00, MAX_PATH_LENGTH);
				err = Sql_GetFolderFullpath(db, curFolderIndex, folderPath, MAX_PATH_LENGTH,&isBufLack);
				if(err == SQLITE_OK)
				{
					ret = Get_DirEntry(pDBInfo, folderPath, curFolderIndex);
					if(ret != DB_SUCCESS)
					{
						DEBUG_TCDB_PRINTF("Get_DirEntry error (%d)\n", ret);
					}
				}
				curFolderIndex++;
			}
		}

		DEBUG_TCDB_PRINTF(" _totalFolderCount(%d), _totalAudioFileCount(%d)\n",pDBInfo->_totalFolderCount,pDBInfo->_totalAudioFileCount);
		if((ret == DB_SUCCESS)&&((int32_t)pDBInfo->_forceStop == 0))
		{
			err = Sql_SetTotalFolderCount(pDBInfo->_handle, pDBInfo->_totalFolderCount);
			if(err == SQLITE_OK)
			{
				ret=DB_SUCCESS;
			}
			else
			{
				ret=DB_SQLITE_INSERT_ERR;
			}
		}
		if((ret == DB_SUCCESS)&&((int32_t)pDBInfo->_forceStop == 0))
		{
			err = Sql_SetTotalAudioFileCount(pDBInfo->_handle,pDBInfo->_totalAudioFileCount);
			if(err == SQLITE_OK)
			{
				ret=DB_SUCCESS;
			}
			else
			{
				ret=DB_SQLITE_INSERT_ERR;
			}
		}
		if((ret == DB_SUCCESS)&&((int32_t)pDBInfo->_forceStop == 0))
		{
			err = Sql_SetTotalVideoFileCount(pDBInfo->_handle, pDBInfo->_totalVideoFileCount);
			if(err == SQLITE_OK)
			{
				ret=DB_SUCCESS;
			}
			else
			{
				ret=DB_SQLITE_INSERT_ERR;
			}
		}
		if((ret == DB_SUCCESS)&&((int32_t)pDBInfo->_forceStop == 0))
		{
			err = Sql_SetTotalImageFileCount(pDBInfo->_handle,pDBInfo->_totalImageFileCount);
			if(err == SQLITE_OK)
			{
				ret=DB_SUCCESS;
			}
			else
			{
				ret=DB_SQLITE_INSERT_ERR;
			}
		}
		if((ret == DB_SUCCESS)&&((int32_t)pDBInfo->_forceStop == 0))
		{
			err = Sql_SetUsedFlag(pDBInfo->_handle, 1);
			if(err == SQLITE_OK)
			{
				ret=DB_SUCCESS;
			}
			else
			{
				ret=DB_SQLITE_INSERT_ERR;
			}
		}

	}
	else
	{
		ret = DB_ARGUMENT_ERR;
	}

	if((pDBInfo!=NULL)&&((int32_t)pDBInfo->_forceStop == 1))
	{
		ret = DB_FORCE_STOP;
	}

	INFO_TCDB_PRINTF("Make File DB Done (result : %d)\n", ret);

	return ret;
}

static int32_t Get_DirEntry(pDB_Info pDBInfo, const char *dir, uint32_t parentIndex)
{
#ifdef CONFIG_ARCH64
	struct linux_dirent64 *d;
#else
	struct linux_dirent *d;
#endif
	int32_t ret = DB_UNKOWN_ERR, err;
	int32_t result;
	int32_t fd,nread;
	int32_t bpos;
	char d_type;
	char *buf=malloc(READ_ENTRY_BUF_SIZE);
	sqlite3 *db;

	DEBUG_TCDB_PRINTF("pDBInfo(%p), handle(%p), dir (%s),buf (%p) parentIndex(%d), forceStop(%d)\n",
		pDBInfo, pDBInfo->_handle,dir,buf, parentIndex,pDBInfo->_forceStop)

	if((pDBInfo != NULL)  && (pDBInfo->_handle!=NULL) && (dir != NULL)&&(buf!=NULL)&&((int32_t)pDBInfo->_forceStop==0))
	{
		db = pDBInfo->_handle;

		fd = open(dir, O_RDONLY | O_DIRECTORY);
		if(fd < 0)
		{
			ret = DB_DIR_OPEN_ERR;
			ERROR_TCDB_PRINTF("Open Dir error (%s)\n", dir);
		}
		else
		{
			for ( ; ; )
			{
				if((int32_t)pDBInfo->_forceStop == 1)
				{
					ret = DB_FORCE_STOP;
					break;
				}
#ifdef CONFIG_ARCH64
				nread = syscall(SYS_getdents64, fd, buf, READ_ENTRY_BUF_SIZE);
#else
				nread = syscall(SYS_getdents, fd, buf, READ_ENTRY_BUF_SIZE);
#endif
				if(nread < 0)
				{
					ret = DB_DIR_OPEN_ERR;
					ERROR_TCDB_PRINTF("read Dir Entry error (%s),(%d)\n", dir,nread);
					break;
				}
				else if (nread ==0)
				{
					ret = DB_SUCCESS;
					break;
				}
				else
				{
					for (bpos = 0; bpos < nread;)
					{
						if((int32_t)pDBInfo->_forceStop == 1)
						{
							ret = DB_FORCE_STOP;
							break;
						}
#ifdef CONFIG_ARCH64
						d = (struct linux_dirent64 *) (buf + bpos);
						d_type = d->d_type;
#else
						d = (struct linux_dirent *) (buf + bpos);
						d_type = *(buf + bpos + d->d_reclen - 1);
#endif
						bpos += d->d_reclen;
						if((d->d_ino != 0) && (strcmp(d->d_name, ".") != 0) && (strcmp(d->d_name, "..") != 0))
						{
							/* directory */
							if(d_type == (char)DT_DIR)
							{
								int32_t isBlakList;

								isBlakList = FindBlackListDirName(gDB_Config._pBlackList, d->d_name);
								if(isBlakList != 1)
								{
									FolderTable_Info folderInfo;
									int32_t dirname_len = (int32_t)strlen(dir);
									if(pDBInfo->_totalFolderCount < gDB_Config._maxFolderNum)
									{
										char * subdir = calloc(1, MAX_PATH_LENGTH + 1);
										if(subdir != NULL)
										{
											(void)strcat(subdir, dir);
											(void)strcat(subdir + dirname_len, "/");
											(void)strcat(subdir + dirname_len + 1, d->d_name);

											folderInfo._name = (char *)d->d_name;
											folderInfo._fullPath = subdir;
											folderInfo._parentIndex = parentIndex;
											folderInfo._inode = d->d_ino;
											err = Sql_InsertFolderList(db,	&folderInfo);
											if(err == SQLITE_OK)
											{
												pDBInfo->_totalFolderCount++;
											}

											free(subdir);
										}
									}
								}
							}
							else if (d_type == (char)DT_REG)
							{
								ContentsType fileType;
								fileType = CheckSupportCodecList( &gDB_Config ,d->d_name);
								if(fileType == AudioContents)
								{
									if(pDBInfo->_totalAudioFileCount < gDB_Config._audioTableInfo._maxFileCount)
									{

										err = Sql_InsertFileList(db, d->d_name, (int32_t)parentIndex, 0,fileType);
										if(err == SQLITE_OK)
										{
											pDBInfo->_totalAudioFileCount++;
											pDBInfo->_updateAudioCount++;
											if(((int32_t)gDB_Config._fileUpdateCount != 0)
												&&(pDBInfo->_updateAudioCount >= gDB_Config._fileUpdateCount))
											{
												if(fileDBUpdate_cb != NULL)
												{
													fileDBUpdate_cb((uint32_t)pDBInfo->_deviceIndex, AudioContents);
												}
												pDBInfo->_updateAudioCount = 0;
											}
										}
									}
								}
								else if(fileType == VideoContents)
								{
									if(pDBInfo->_totalVideoFileCount < gDB_Config._videoTableInfo._maxFileCount)
									{

										err = Sql_InsertFileList(db, d->d_name, (int32_t)parentIndex, 0,fileType);
										if(err == SQLITE_OK)
										{
											pDBInfo->_totalVideoFileCount++;
											pDBInfo->_updateVideoCount++;
											if(((int32_t)gDB_Config._fileUpdateCount != 0)
												&&(pDBInfo->_updateVideoCount >= gDB_Config._fileUpdateCount))
											{
												if(fileDBUpdate_cb != NULL)
												{
													fileDBUpdate_cb((uint32_t)pDBInfo->_deviceIndex, VideoContents);
												}
												pDBInfo->_updateVideoCount = 0;
											}
										}
	 								}
								}
								else if(fileType == ImageContents)
								{
									if(pDBInfo->_totalImageFileCount < gDB_Config._imageTableInfo._maxFileCount)
									{

										err = Sql_InsertFileList(db, d->d_name, (int32_t)parentIndex, 0,fileType);
										if(err == SQLITE_OK)
										{
											pDBInfo->_totalImageFileCount++;
											if(((int32_t)gDB_Config._fileUpdateCount != 0)
												&&(pDBInfo->_updateImageCount >= gDB_Config._fileUpdateCount))
											{
												if(fileDBUpdate_cb != NULL)
												{
													fileDBUpdate_cb((uint32_t)pDBInfo->_deviceIndex, ImageContents);
												}
												pDBInfo->_updateImageCount = 0;
											}
										}
									}
	 							}
								else
								{
									/* Not support File  */;
								}
							}
							else
							{
								continue;
							}
						}
					}
				}
			}
			result = close(fd);
			if(result < 0)
			{
				ret = DB_ACCESS_FS_ERR;
			}
		}
	}

	if(buf!=NULL)
	{
		free(buf);
	}
	return ret;
}

static int32_t Sort_FileDB(pDB_Info pDBInfo)
{
	int32_t ret = DB_UNKOWN_ERR;

	INFO_TCDB_PRINTF("File DB Sorting Start\n");
	if(pDBInfo != NULL)
	{
		DEBUG_TCDB_PRINTF("Sorting Folder List\n");
		ret = Sql_MakeSortedFolderList(pDBInfo->_handle);
		if(ret == SQLITE_OK)
		{
			DEBUG_TCDB_PRINTF("Sorting Audio File List\n");
			ret = Sql_MakeSortedFileList(pDBInfo->_handle, AudioContents);
		}
		if(ret == SQLITE_OK)
		{
			DEBUG_TCDB_PRINTF("Sorting Video File List\n");
			ret = Sql_MakeSortedFileList(pDBInfo->_handle, VideoContents);
		}
		if(ret == SQLITE_OK)
		{
			DEBUG_TCDB_PRINTF("Sorting Image File List\n");
			ret = Sql_MakeSortedFileList(pDBInfo->_handle, ImageContents);
		}
	}
	return ret;
}

static int32_t Set_DatabaseName(uint32_t dbNum)
{
	int32_t ret = DB_UNKOWN_ERR;
	const char	*ver;
	pDB_Info		pDBInfo;

	if((pgDB_Info != NULL)&&(dbNum < gDB_Config._maxDBNum))
	{
		pDBInfo = &pgDB_Info[dbNum];
		ver = TCDB_libversion();
		(void)sprintf(pDBInfo->_dbName, "%s/"TCDB_DATABASE_NAME,gDB_Config._dirName,ver,dbNum);

		ret = DB_SUCCESS;
	}
	return ret;
}

static const char * Get_DatabaseName(uint32_t dbNum)
{
	const char	*DatabaseName=NULL;
	pDB_Info		pDBInfo;

	if((pgDB_Info != NULL)&&(dbNum < gDB_Config._maxDBNum))
	{
		pDBInfo = &pgDB_Info[dbNum];
		if(pDBInfo != NULL)
		{
			DatabaseName = pDBInfo->_dbName;
		}
	}
	return DatabaseName;
}

#ifdef META_DB_INCLUDE

static int32_t TCDB_Meta_Initialize (void)
{
	int32_t ret;
	INFO_TCDB_PRINTF("\n");

	if (sizeof(MetaBrwsHandle) < (UINT_MAX / gDB_Config._maxDBNum))
	{
		pgMetaBrwsHandle = malloc(gDB_Config._maxDBNum * sizeof(MetaBrwsHandle));
		if(pgMetaBrwsHandle != NULL)
		{
			pMetaBrwsHandle pMetaHandle;
			uint32_t i;
			for(i=0; i<gDB_Config._maxDBNum; i++)
			{
				pMetaHandle = &pgMetaBrwsHandle[i];
				pMetaHandle->_db = NULL;
				pMetaHandle->_dbNum =0;
				pMetaHandle->_pCategorytree = NULL;
				pMetaHandle->_pPlayList = NULL;
				pMetaHandle->_selectTrackNum = (int32_t)INVALID_INDEX;
				pMetaHandle->_totalTrakNum = 0;
			}
			ret = DB_SUCCESS;
		}
		else
		{
			ret = DB_MEMORY_ALLOC_ERR;
		}
	}
	else
	{
		ret = DB_ARGUMENT_ERR;
	}

	return ret;

}

static void Meta_Release(uint32_t dbNum)
{
	pMetaBrwsHandle pMetaHandle;
	INFO_TCDB_PRINTF("\n");

	pMetaHandle = &pgMetaBrwsHandle[dbNum];

	if(pMetaHandle != NULL)
	{
		pMetaHandle->_db = NULL;
		pMetaHandle->_dbNum =0;
		if(NULL != pMetaHandle->_pCategorytree)
		{
			ResetMetaTree(pMetaHandle->_pCategorytree);
			pMetaHandle->_pCategorytree = NULL;
		}

		if(NULL != pMetaHandle->_pPlayList)
		{
			free(pMetaHandle->_pPlayList);
			pMetaHandle->_pPlayList = NULL;
		}
		pMetaHandle->_selectTrackNum = (int32_t)INVALID_INDEX;
		pMetaHandle->_totalTrakNum = 0;
	}
}

static void TCDB_Meta_Release(void)
{
	INFO_TCDB_PRINTF("\n");
	if(pgMetaBrwsHandle != NULL)
	{
		uint32_t i;
		for(i=0; i<gDB_Config._maxDBNum; i++)
		{
			Meta_Release(i);
		}

		free(pgMetaBrwsHandle);
		pgMetaBrwsHandle = NULL;
	}
}

#if 0
void Dump_MetaTree(uint32_t dbNum)
{
	char **result;
	char *errmsg;
	FILE * pFile;
	int32_t rows, columns;
	int32_t rc, i, j;
	char * sqlQuery;
	pMetaBrwsHandle pMetaHandle = &pgMetaBrwsHandle[dbNum];

	if((NULL != pMetaHandle)&&(pMetaHandle->_db != NULL))
	{

/****************************************************************************************************************/
		pFile = fopen ("Genre-Artist-Album-Title.txt","w");
		if(pFile !=NULL)
		{
			sqlQuery = (char *)newformat("SELECT Genre, Artist, Album, Title from AudioFileTable WHERE Genre IS NOT NULL ORDER BY Genre ASC, Artist ASC, Album ASC, Title ASC");
			rc = sqlite3_get_table(pMetaHandle->_db, sqlQuery, &result, &rows, &columns, &errmsg);
			if(SQLITE_OK == rc)
			{
				DEBUG_TCDB_PRINTF("rows count : %d \n", rows);
				// first row is column header
				for(i=0; i<(rows+1); i ++)
				{
					for(j=0;j<columns;j++)
					{
						char tagBuffer[512];
						(void)sprintf(tagBuffer, "%s \t", result[i*columns+j]);
						(void)fwrite(tagBuffer, sizeof(char),strlen(tagBuffer),pFile);
					}
					(void)fwrite("\n", sizeof(char),strlen("\n"),pFile);
				}
			}

			sqlite3_free_table(result);

			(void)fclose(pFile);
			pFile = NULL;
		}
/****************************************************************************************************************/

/****************************************************************************************************************/
		pFile = fopen ("Genre-Artist-Title.txt","w");
		if(pFile !=NULL)
		{
			sqlQuery = (char *)newformat("SELECT Genre, Artist, Title from AudioFileTable WHERE Genre IS NOT NULL ORDER BY Genre ASC, Artist ASC, Title ASC");
			rc = sqlite3_get_table(pMetaHandle->_db, sqlQuery, &result, &rows, &columns, &errmsg);
			if(SQLITE_OK == rc)
			{
				DEBUG_TCDB_PRINTF("rows count : %d \n", rows);
				// first row is column header
				for(i=0; i<(rows+1); i ++)
				{
					for(j=0;j<columns;j++)
					{
						char tagBuffer[512];
						(void)sprintf(tagBuffer, "%s \t", result[i*columns+j]);
						(void)fwrite(tagBuffer, sizeof(char),strlen(tagBuffer),pFile);
					}
					(void)fwrite("\n", sizeof(char),strlen("\n"),pFile);
				}
			}

			sqlite3_free_table(result);
			(void)fclose(pFile);
			pFile = NULL;
		}

/****************************************************************************************************************/

/****************************************************************************************************************/
		pFile = fopen ("Genre-Album-Title.txt","w");
		if(pFile !=NULL)
		{
			sqlQuery = (char *)newformat("SELECT Genre,  Album, Title from AudioFileTable WHERE Genre IS NOT NULL ORDER BY Genre ASC, Album ASC, Title ASC");
			rc = sqlite3_get_table(pMetaHandle->_db, sqlQuery, &result, &rows, &columns, &errmsg);
			if(SQLITE_OK == rc)
			{
				DEBUG_TCDB_PRINTF("rows count : %d \n", rows);
				// first row is column header
				for(i=0; i<(rows+1); i ++)
				{
					for(j=0;j<columns;j++)
					{
						char tagBuffer[512];
						(void)sprintf(tagBuffer, "%s \t", result[i*columns+j]);
						(void)fwrite(tagBuffer, sizeof(char),strlen(tagBuffer),pFile);
					}
					(void)fwrite("\n", sizeof(char),strlen("\n"),pFile);
				}
			}

			sqlite3_free_table(result);
			(void)fclose(pFile);
			pFile = NULL;
		}
/****************************************************************************************************************/

/****************************************************************************************************************/
		pFile = fopen ("Genre-Title.txt","w");
		if(pFile !=NULL)
		{
			sqlQuery = (char *)newformat("SELECT Genre, Title from AudioFileTable WHERE Genre IS NOT NULL ORDER BY Genre ASC, Title ASC");
			rc = sqlite3_get_table(pMetaHandle->_db, sqlQuery, &result, &rows, &columns, &errmsg);
			if(SQLITE_OK == rc)
			{
				DEBUG_TCDB_PRINTF("rows count : %d \n", rows);
				// first row is column header
				for(i=0; i<(rows+1); i ++)
				{
					for(j=0;j<columns;j++)
					{
						char tagBuffer[512];
						(void)sprintf(tagBuffer, "%s \t", result[i*columns+j]);
						(void)fwrite(tagBuffer, sizeof(char),strlen(tagBuffer),pFile);
					}
					(void)fwrite("\n", sizeof(char),strlen("\n"),pFile);
				}
			}

			sqlite3_free_table(result);
			(void)fclose(pFile);

			pFile = NULL;
		}
/****************************************************************************************************************/

/****************************************************************************************************************/
		pFile = fopen ("Artist-Album-Title.txt","w");
		if(pFile !=NULL)
		{
			sqlQuery = (char *)newformat("SELECT Artist, Album, Title from AudioFileTable WHERE Artist IS NOT NULL ORDER BY Artist ASC, Album ASC, Title ASC");
			rc = sqlite3_get_table(pMetaHandle->_db, sqlQuery, &result, &rows, &columns, &errmsg);
			if(SQLITE_OK == rc)
			{
				DEBUG_TCDB_PRINTF("rows count : %d \n", rows);
				// first row is column header
				for(i=0; i<(rows+1); i ++)
				{
					for(j=0;j<columns;j++)
					{
						char tagBuffer[512];
						(void)sprintf(tagBuffer, "%s \t", result[i*columns+j]);
						(void)fwrite(tagBuffer, sizeof(char),strlen(tagBuffer),pFile);
					}
					(void)fwrite("\n", sizeof(char),strlen("\n"),pFile);
				}
			}

			sqlite3_free_table(result);
			(void)fclose(pFile);

			pFile = NULL;
		}
/****************************************************************************************************************/

/****************************************************************************************************************/
		pFile = fopen ("Artist-Title.txt","w");
		if(pFile !=NULL)
		{
			sqlQuery = (char *)newformat("SELECT Artist, Title from AudioFileTable WHERE Artist IS NOT NULL ORDER BY Artist ASC, Title ASC");
			rc = sqlite3_get_table(pMetaHandle->_db, sqlQuery, &result, &rows, &columns, &errmsg);
			if(SQLITE_OK == rc)
			{
				DEBUG_TCDB_PRINTF("rows count : %d \n", rows);
				// first row is column header
				for(i=0; i<(rows+1); i ++)
				{
					for(j=0;j<columns;j++)
					{
						char tagBuffer[512];
						(void)sprintf(tagBuffer, "%s \t", result[i*columns+j]);
						(void)fwrite(tagBuffer, sizeof(char),strlen(tagBuffer),pFile);
					}
					(void)fwrite("\n", sizeof(char),strlen("\n"),pFile);
				}
			}

			sqlite3_free_table(result);
			(void)fclose(pFile);

			pFile = NULL;
		}
/****************************************************************************************************************/

/****************************************************************************************************************/
		pFile = fopen ("Album-Title.txt","w");
		if(pFile !=NULL)
		{
			sqlQuery = (char *)newformat("SELECT Album, Title from AudioFileTable WHERE Album IS NOT NULL ORDER BY Album ASC, Title ASC");
			rc = sqlite3_get_table(pMetaHandle->_db, sqlQuery, &result, &rows, &columns, &errmsg);
			if(SQLITE_OK == rc)
			{
				DEBUG_TCDB_PRINTF("rows count : %d \n", rows);
				// first row is column header
				for(i=0; i<(rows+1); i ++)
				{
					for(j=0;j<columns;j++)
					{
						char tagBuffer[512];
						(void)sprintf(tagBuffer, "%s \t", result[i*columns+j]);
						(void)fwrite(tagBuffer, sizeof(char),strlen(tagBuffer),pFile);
					}
					(void)fwrite("\n", sizeof(char),strlen("\n"),pFile);
				}
			}

			sqlite3_free_table(result);
			(void)fclose(pFile);

			pFile = NULL;
		}
/****************************************************************************************************************/

/****************************************************************************************************************/
		pFile = fopen ("Title.txt","w");
		if(pFile !=NULL)
		{
			sqlQuery = (char *)newformat("SELECT Title from AudioFileTable WHERE Title IS NOT NULL ORDER BY Title ASC");
			rc = sqlite3_get_table(pMetaHandle->_db, sqlQuery, &result, &rows, &columns, &errmsg);
			if(SQLITE_OK == rc)
			{
				DEBUG_TCDB_PRINTF("rows count : %d \n", rows);
				// first row is column header
				for(i=0; i<(rows+1); i ++)
				{
					for(j=0;j<columns;j++)
					{
						char tagBuffer[512];
						(void)sprintf(tagBuffer, "%s \t", result[i*columns+j]);
						(void)fwrite(tagBuffer, sizeof(char),strlen(tagBuffer),pFile);
					}
					(void)fwrite("\n", sizeof(char),strlen("\n"),pFile);
				}
			}

			sqlite3_free_table(result);
			(void)fclose(pFile);

			pFile = NULL;
		}
/****************************************************************************************************************/

	}
}
#endif

static void * meta_gathering_function(void *arg)
{
	int32_t ret;
	pDB_Info pDBInfo = arg;
	INFO_TCDB_PRINTF("dbNum (%d)\n", pDBInfo->_deviceIndex);
	
	if(pDBInfo  != NULL)
	{
		ret = Meta_MakeDB((uint32_t)pDBInfo->_deviceIndex);
		if(ret == DB_SUCCESS)
		{
			INFO_TCDB_PRINTF("Meta DB Success - Task End\n");
			pDBInfo->_status = (int32_t)MetaDB_Completed;
		}
		else
		{
			if((int32_t)pDBInfo->_forceStop == 1)
			{
				ret = DB_FORCE_STOP;
			}		
			pDBInfo->_status = (int32_t)DB_Error;
			ERROR_TCDB_PRINTF("Meta DB Fail - Task End\n");
		}

		pDBInfo->_isRun = 0;
		if(metaDBResult_cb != NULL)
		{
			metaDBResult_cb((uint32_t)pDBInfo->_deviceIndex, ret);
		}

	}
	else
	{
		ret = DB_ARGUMENT_ERR;
	}
	(void)ret;
	return NULL;
}

int32_t TCDB_Meta_MakeDB(uint32_t dbNum)
{
	pDB_Info pDBInfo;
	int32_t ret;
	INFO_TCDB_PRINTF("dbNum (%d)\n", dbNum);

	if((pgDB_Info != NULL)&&(dbNum < gDB_Config._maxDBNum)&&(gatheringTag_cb != NULL))
	{
		pDBInfo = &pgDB_Info[dbNum];

		if(pDBInfo != NULL)
		{
			(void)pthread_mutex_lock(&dbMutex);
			if((int32_t)pDBInfo->_isRun == 0)
			{
				int32_t err;
				pDBInfo->_isRun =1;
				pDBInfo->_status = (int32_t)MetaDB_Starting;
				err = pthread_create(&pDBInfo->_metaThread, NULL, meta_gathering_function, pDBInfo);
				if( err ==0 )
				{
					ret = DB_SUCCESS;
					pDBInfo->_isMetaThreadCreated = 1;
				}
				else
				{
					ret = DB_CREATE_TASK_ERR;
					ERROR_TCDB_PRINTF("create Meta thread failed(%d)\n", ret);
				}
			}
			else
			{	ERROR_TCDB_PRINTF("Already Meta DB Task run\n");
				ret = DB_TASK_BUSY;
			}
			(void)pthread_mutex_unlock(&dbMutex);

		}
		else
		{
			ret = DB_NOT_INITIALIZE;
		}
	}	
	else if(gatheringTag_cb == NULL)
	{
		ERROR_TCDB_PRINTF("GatheringTag_cb function that has not yet been registered!!!\n");
		ERROR_TCDB_PRINTF("Please register 'GatheringTag_cb' before creating the meta db.\n");
		ret = DB_NOT_INITIALIZE;
	}
	else
	{
		ret = DB_ARGUMENT_ERR;
		ERROR_TCDB_PRINTF("Bad Arugment");
	}

	INFO_TCDB_PRINTF("End (%d)\n", ret);
	return ret;
}

static int32_t Meta_MakeDB(uint32_t dbNum)
{
	pDB_Info pDBInfo;
	int32_t ret, err;
	INFO_TCDB_PRINTF("dbNum (%d)\n", dbNum);

	if((pgDB_Info != NULL)&&(dbNum < gDB_Config._maxDBNum))
	{
		pDBInfo = &pgDB_Info[dbNum];

		if(pDBInfo != NULL)
		{
			if((int32_t)pDBInfo->_forceStop == 1)
			{
				ret = DB_FORCE_STOP;
			}
			else
			{
				 err = Sql_Transaction_Start(pDBInfo->_handle);
				 if(err != SQLITE_OK)
				{
					ret = DB_SQLITE_PRAGMA_ERR;
				}
				 else
				{
					ret = GatheringMetaDB(dbNum, pDBInfo->_handle,pDBInfo);
					if(ret == DB_SUCCESS)
					{
						INFO_TCDB_PRINTF("Meta gathering success\n");
					}
					else
					{
						ERROR_TCDB_PRINTF("Meta gathering fail : ERROR_TCDB_PRINTF(%d)\n", ret);
					}
					err = Sql_Transaction_End(pDBInfo->_handle);
					 if(err != SQLITE_OK)
					{
						ret = DB_SQLITE_PRAGMA_ERR;
					}
				}
			}
		}
		else
		{
			ret = DB_NOT_INITIALIZE;
		}
	}
	else
	{
		ret = DB_ARGUMENT_ERR;
		ERROR_TCDB_PRINTF("Bad Arugment");
	}

	INFO_TCDB_PRINTF("End (%d)\n", ret);
	return ret;
}

static int32_t GatheringMetaDB(uint32_t dbNum,sqlite3 *db, pDB_Info pDBInfo)
{
	int32_t ret = DB_UNKOWN_ERR;
	int32_t err;
	uint32_t updateCount =0;

	INFO_TCDB_PRINTF("\n");

	if((db != NULL)&&(pDBInfo != NULL))
	{
		uint32_t currentInode;
		uint32_t preIndoe =0;
		int32_t parentFolder;
		int32_t totalFileNum;
		int32_t fileIndex;
		char folderPath[MAX_PATH_LENGTH];
		char fileName[MAX_NAME_LENGTH];
		int32_t stop=0;
		int32_t folderFD = -1;
		int32_t isBufLack;

		(void)memset(folderPath, 0x00, MAX_PATH_LENGTH);

		totalFileNum = Sql_GetTotalAudioFileCount(db);
		if(totalFileNum > 0)
		{
			for(fileIndex=1; fileIndex <= totalFileNum; fileIndex++)
			{
				parentFolder = Sql_GetFileParentIndex(db, (uint32_t)fileIndex, AudioContents);
				if(parentFolder > 0)
				{
					int32_t inode = Sql_GetFolderInode(db,(uint32_t)parentFolder);
					if(inode < 0)
					{
						currentInode = (uint32_t)INVALID_INDEX;
					}
					else
					{
						currentInode =  (uint32_t)inode;
					}
					if(currentInode != (uint32_t)INVALID_INDEX)
					{
						if(currentInode != preIndoe)
						{
							// change folder
							(void)memset(folderPath, 0x00, MAX_PATH_LENGTH);
							err = Sql_GetFolderFullpath(db, (uint32_t)parentFolder, folderPath, MAX_PATH_LENGTH, &isBufLack);
							if(err == SQLITE_OK)
							{
								if(folderFD > -1)
								{
									(void)close(folderFD);
								}
								folderFD = open(folderPath, O_RDONLY);
								preIndoe = currentInode;
							}
						}

						if(folderFD > 0 )
						{
							(void)memset(fileName, 0x00, MAX_NAME_LENGTH);
							err = Sql_GetFileName(db, (uint32_t)fileIndex,  fileName,MAX_NAME_LENGTH, &isBufLack ,AudioContents);
							if(err == SQLITE_OK)
							{
								ret = gatheringTag_cb(dbNum, fileIndex, folderPath, fileName);
								if(ret != DB_SUCCESS)
								{
									ERROR_TCDB_PRINTF("_gatheringTag error : (%d)\n", ret);
									stop=1;
								}
							}
						}
						else
						{
							ERROR_TCDB_PRINTF("open dir error (%s), (%d)\n", folderPath, folderFD);
							ret = DB_FS_READ_ERR;
							stop =1;
						}
					}
					else
					{
						stop=1;
						ret = DB_SQLITE_SELECT_ERR;
					}
				}
				else
				{
					stop =1;
					ret = DB_SQLITE_SELECT_ERR;
				}

				if((int32_t)pDBInfo->_forceStop != 0)
				{
					stop = 1;
					ret = DB_FORCE_STOP;
				}

				if(stop ==1 )
				{
					break;
				}

				updateCount++;

				if(((int32_t)gDB_Config._metaUpdateCount != 0) 
					&& (updateCount >= gDB_Config._metaUpdateCount))
				{
					if(metaDBUpdate_cb != NULL)
					{
						metaDBUpdate_cb((uint32_t)pDBInfo->_deviceIndex);
					}
					updateCount = 0;
				}
			}
			if(folderFD > 0)
			{
				(void)close(folderFD);
			}
		}
		else
		{
			ret	= DB_NO_CONTENTS_ERR;
		}
	}
	return ret;
}

int32_t TCDB_Meta_InsertTag(uint32_t dbNum, int32_t fileIndex, const char *title, const char * artist, const char * album, const char *genre)
{
	int32_t ret;

	if((fileIndex > -1)&&(title != NULL)&&(artist!=NULL)&&(album != NULL)&&(genre!=NULL))
	{
		if((pgDB_Info != NULL)&&(dbNum < gDB_Config._maxDBNum))
		{
			pDB_Info pDBInfo;
			pDBInfo = &pgDB_Info[dbNum];

			if(pDBInfo != NULL)
			{
				int32_t rc;
				(void)pthread_mutex_lock(&pDBInfo->_handleLock);
				rc = Sql_Insert_Tag_Info(pDBInfo->_handle, fileIndex, title, artist, album, genre);
				if(rc != SQLITE_OK)
				{
					ret = DB_SQLITE_INSERT_ERR;
				}
				else
				{
					ret = DB_SUCCESS;
				}
				(void)pthread_mutex_unlock(&pDBInfo->_handleLock);
			}
			else
			{
				ret = DB_NOT_INITIALIZE;
			}		
		}
		else
		{
			ret = DB_NOT_INITIALIZE;
		}
	}
	else
	{
		ret = DB_ARGUMENT_ERR;
	}

	return ret;
}

int32_t TCDB_Meta_GetTagName(uint32_t dbNum, uint32_t fileIndex, char *name, uint32_t bufSize,TagCategory type)
{
	pDB_Info pDBInfo;
	int32_t ret = DB_ARGUMENT_ERR;
	int32_t err;
	int32_t isBufLack = -1;
	DEBUG_TCDB_PRINTF("dbNum (%d), fileIndex (%d)\n", dbNum, fileIndex);
	if((pgDB_Info != NULL)&&(dbNum < gDB_Config._maxDBNum) && (name != NULL))
	{
		pDBInfo = &pgDB_Info[dbNum];
		if(pDBInfo != NULL)
		{
			(void)pthread_mutex_lock(&pDBInfo->_handleLock);
			err = Sql_GetTagName(pDBInfo->_handle, fileIndex, name, bufSize, &isBufLack,type);
			if(err == SQLITE_OK)
			{
				if(isBufLack == 0)
				{
					ret = DB_SUCCESS;
				}
				else
				{
					ret = DB_NOT_ENOUGH_BUFFER;
				}
			}
			else
			{
				ret = DB_SQLITE_SELECT_ERR;
			}
			(void)pthread_mutex_unlock(&pDBInfo->_handleLock);
		}
		else
		{
			ret =DB_NOT_INITIALIZE;
		}
	}

	return ret;
}

int32_t TCDB_Meta_GetTagNameAll(uint32_t dbNum, uint32_t fileIndex, char *title, char *artist, char *album, char *genre, uint32_t bufSize)
{
	pDB_Info pDBInfo;
	int32_t ret = DB_ARGUMENT_ERR;
	int32_t err;

	if((pgDB_Info != NULL)&&(dbNum < gDB_Config._maxDBNum) && (title != NULL) && (artist != NULL) && (album != NULL) && ( genre != NULL))
	{
		pDBInfo = &pgDB_Info[dbNum];
		if(pDBInfo != NULL)
		{
			(void)pthread_mutex_lock(&pDBInfo->_handleLock);
			err = Sql_GetTagNameAll(pDBInfo->_handle, fileIndex, title, artist, album, genre,bufSize);
			if(err == SQLITE_OK)
			{
				ret = DB_SUCCESS;
			}
			else
			{
				ret = DB_SQLITE_SELECT_ERR;
			}
			(void)pthread_mutex_unlock(&pDBInfo->_handleLock);
		}
		else
		{
			ret =DB_NOT_INITIALIZE;
		}
	}

	return ret;
}

static MetaCategoryInfo *CreateMetaTree(const char * category)
{
	MetaCategoryInfo *newTree = (MetaCategoryInfo *) malloc(sizeof (MetaCategoryInfo));

	if(newTree != NULL)
	{
		newTree->_pCategory = (char *)newformat(category);
		if(newTree->_pCategory != NULL)
		{
			newTree->_pNextCategory = NULL;
			newTree->_pList = NULL;
			newTree->_totalIndex =0;
			newTree->_selectIndex = (uint32_t)INVALID_INDEX;
			(void)memset(newTree->_selectTagName, 0x00, MAX_TAG_LENGTH);
		}
		else
		{
			free(newTree);
			newTree = NULL;
		}
	}
	return newTree;
}

static void AppendMetaTree(MetaCategoryInfo ** head, MetaCategoryInfo * newTree)
{
	if((*head) == NULL)
	{
		*head = newTree;
	}
	else
	{
		MetaCategoryInfo * tail = (*head);
		while(tail->_pNextCategory != NULL)
		{
			tail = tail->_pNextCategory;
		}
		tail->_pNextCategory = newTree;
	}
}

static MetaCategoryInfo * FindMetaTree(MetaCategoryInfo * head, int32_t depth)
{
	MetaCategoryInfo * current = head;
	int32_t treeDepth = depth;
	while(current != NULL && (--treeDepth) >= 0)
	{
		current = current->_pNextCategory;
	}
	return current;
}


static void DestroyMetaTree(MetaCategoryInfo * metaTree)
{
	if(metaTree != NULL)
	{
		if(metaTree->_pCategory != NULL)
		{
			free(metaTree->_pCategory);
			metaTree->_pCategory = NULL;
		}
		if(metaTree->_pList != NULL)
		{
			free(metaTree->_pList);
			metaTree->_pList = NULL;
		}
		free(metaTree);
	}
}

static int32_t GetTreeDepth(MetaCategoryInfo * head)
{
	int32_t count =0;
	MetaCategoryInfo * current = head;
	while(current->_pNextCategory  != NULL)
	{
		current = current->_pNextCategory;
		count++;
	}

	return count;
}

static void RemoveLastMetaTree(MetaCategoryInfo ** head)
{
	DEBUG_TCDB_PRINTF("\n");
	if(*head != NULL)
	{
		int32_t i;
		MetaCategoryInfo * current = *head;
		int32_t categoryDepth = GetTreeDepth(current);

		for(i=0; i <(categoryDepth-1); i++)
		{
			current = current->_pNextCategory;
		}
		DestroyMetaTree(current->_pNextCategory);
		current->_pNextCategory = NULL;
		current->_selectIndex = (uint32_t)INVALID_INDEX;
		(void)memset(current->_selectTagName, 0x00, MAX_TAG_LENGTH);
 	}
}

static void ResetMetaTree(MetaCategoryInfo * head)
{
	if(head != NULL)
	{
		while(head->_pNextCategory != NULL)
		{
			RemoveLastMetaTree((MetaCategoryInfo **)&head);
		}
 		PrintMetaTree(head);
		DestroyMetaTree(head);
	}
}

static void PrintMetaTree(MetaCategoryInfo *pCategory)
{
#if 0
	int32_t depth=0;

	while(pCategory !=NULL)
	{
		(void)fprintf(stderr,"=============== Depth (%d) ===============\n", depth);
		(void)fprintf(stderr, "Level %d : Category (%s) -(%p)  \n", depth, pCategory->_pCategory, pCategory);
		(void)fprintf(stderr, "Level %d : Select Index (%d)  \n", depth,pCategory->_selectIndex);
		(void)fprintf(stderr, "Level %d : Select Tag (%s) \n", depth,pCategory->_selectTagName);
		(void)fprintf(stderr, "Level %d : List (%p), total index (%d) \n", depth,pCategory->_pList, pCategory->_totalIndex);
		(void)fprintf(stderr, "Level %d : NextCategory (%p)  \n", depth,pCategory->_pNextCategory);
		pCategory = pCategory->_pNextCategory;
		depth++;
	}
	(void)fprintf(stderr, "\n");
#endif
	(void)pCategory;
}

int32_t TCDB_Meta_ResetBrowser(uint32_t dbNum)
{
	int32_t ret;
	pDB_Info pDBInfo = &pgDB_Info[dbNum];
	pMetaBrwsHandle pMetaHandle = &pgMetaBrwsHandle[dbNum];

	DEBUG_TCDB_PRINTF("dbNum = %d\n", dbNum);
	if((pDBInfo != NULL)&&(pDBInfo->_handle != NULL)&&(NULL != pMetaHandle))
	{
		(void)pthread_mutex_lock(&pDBInfo->_handleLock);
		pMetaHandle->_db = pDBInfo->_handle ;
		(void)pthread_mutex_unlock(&pDBInfo->_handleLock);
		pMetaHandle->_dbNum = dbNum;
		if(pMetaHandle->_pCategorytree  != NULL)
		{
			ResetMetaTree(pMetaHandle->_pCategorytree);
			pMetaHandle->_pCategorytree = NULL;
		}
		ret = DB_SUCCESS;
		PrintMetaTree(pMetaHandle->_pCategorytree);
	}
	else
	{
		ret = DB_NOT_INITIALIZE;
	}

	return ret;
}

int32_t TCDB_Meta_GetNumberCategory(uint32_t dbNum,const char * categoryName)
{
	int32_t count = (int32_t)INVALID_INDEX;
	char **result;
	char *errmsg;
	int32_t rows, columns;
	int32_t rc, i;
	pDB_Info pDBInfo =  &pgDB_Info[dbNum];
	MetaCategoryInfo *pCategoryInfo;
	pMetaBrwsHandle pMetaHandle = &pgMetaBrwsHandle[dbNum];

	DEBUG_TCDB_PRINTF("dbNum (%d), category Name = %s\n", dbNum, categoryName);

	if((NULL != pMetaHandle)&&(pMetaHandle->_db != NULL)&&(NULL != pDBInfo))
	{
		(void)pthread_mutex_lock(&pDBInfo->_handleLock);
		 /* first category */
		if(pMetaHandle->_pCategorytree == NULL)
		{
			char * sqlQuery;

			pCategoryInfo = CreateMetaTree(categoryName);
			if(NULL != pCategoryInfo)
			{
				pMetaHandle->_pCategorytree = pCategoryInfo;

				if(strcasecmp(categoryName, "Title")!=0)
				{
					sqlQuery = (char *)newformat("SELECT FileIndex from TagTable WHERE %s IS NOT NULL GROUP BY %s ORDER BY %s collate %s ASC", categoryName, categoryName,categoryName,gCollateFunction);
				}
				else
				{
					sqlQuery = (char *)newformat("SELECT FileIndex from TagTable WHERE %s IS NOT NULL ORDER BY %s collate %s ASC", categoryName,categoryName,gCollateFunction);
				}
				DEBUG_TCDB_PRINTF("query : %s \n", sqlQuery);
				if(sqlQuery != NULL)
				{
					rc = sqlite3_get_table(pMetaHandle->_db, sqlQuery, &result, &rows, &columns, &errmsg);
					if(SQLITE_OK == rc)
					{
						DEBUG_TCDB_PRINTF("rows count : %d \n", rows);
						pCategoryInfo->_totalIndex = (uint32_t)rows;

						if(pCategoryInfo->_totalIndex < (UINT_MAX / sizeof(uint32_t)))
						{
							pCategoryInfo->_pList = malloc(sizeof(uint32_t)*pCategoryInfo->_totalIndex);
							if(NULL != pCategoryInfo->_pList )
							{
								// first row is column header
								for(i=1; i<(rows+1); i ++)
								{
									int32_t listIndex = atoi(result[i]);
									if(listIndex < 0)
									{
										pCategoryInfo->_pList[i-1] = (uint32_t)INVALID_INDEX;
									}
									else
									{
										pCategoryInfo->_pList[i-1] = (uint32_t)listIndex;
									}
								}
								count = rows;
							}
							else
							{
								ERROR_TCDB_PRINTF("Allocate fail\n");
							}
						}
						else
						{
							ERROR_TCDB_PRINTF("Allocate fail\n");
						}

					}
					else
					{
						ERROR_TCDB_PRINTF("Get sqlite table error (%d)\n", rc);
					}

					sqlite3_free_table(result);
					free(sqlQuery);
				}
				else
				{
					ERROR_TCDB_PRINTF("Not enough memory\n");
				}
			}
			else
			{
				ERROR_TCDB_PRINTF("Allocate fail\n");
			}

		}
		/* sub category */
		else
		{
			char queryBuffer[SQL_TAG_QUERY_STRING_SIZE];
			int32_t queryLength;
			int32_t preSelect = 1;
			MetaCategoryInfo *findCategory;

			findCategory  = pMetaHandle->_pCategorytree;

			while(findCategory != NULL)
			{
				if(strcasecmp(findCategory->_pCategory, categoryName)==0)
				{
					count = (int32_t)findCategory->_totalIndex;
					break;
				}
				findCategory = findCategory->_pNextCategory;
			}

			if(count == (int32_t)INVALID_INDEX)
			{
				pCategoryInfo = pMetaHandle->_pCategorytree;

				if((pCategoryInfo->_pCategory != NULL) && (pCategoryInfo->_selectIndex == META_SELECT_ALL))
				{
					(void)sprintf(queryBuffer, "SELECT FileIndex from TagTable WHERE %s IS NOT NULL", categoryName);
				}
				else if((pCategoryInfo->_pCategory != NULL) && (pCategoryInfo->_selectIndex != (uint32_t)INVALID_INDEX))
				{
					(void)sprintf(queryBuffer, "SELECT FileIndex from TagTable WHERE %s IS NOT NULL and %s = \"%s\"", categoryName, pCategoryInfo->_pCategory, pCategoryInfo->_selectTagName);
				}
				else
				{
					ERROR_TCDB_PRINTF("Pre Category select error : category (%s), subcategory (%s) \n", pCategoryInfo->_pCategory,pCategoryInfo->_selectTagName);
					preSelect = 0;
				}

				while((pCategoryInfo ->_pNextCategory != NULL)&&(preSelect != 0))
				{
					pCategoryInfo = pCategoryInfo ->_pNextCategory;
					if((pCategoryInfo->_pCategory != NULL) && (pCategoryInfo->_selectIndex == META_SELECT_ALL))
					{
						;
					}
					else if((pCategoryInfo->_pCategory != NULL) && (pCategoryInfo->_selectIndex != (uint32_t)INVALID_INDEX))
					{
						queryLength = (int32_t)strlen(queryBuffer);
						(void)sprintf(&queryBuffer[queryLength], " and %s = \"%s\"",pCategoryInfo->_pCategory, pCategoryInfo->_selectTagName);
					}
					else
					{
						ERROR_TCDB_PRINTF("Pre Category select error : category (%s), subcategory (%s) \n", pCategoryInfo->_pCategory,pCategoryInfo->_selectTagName);
						preSelect = 0;
					}
				}
				if(preSelect == 1)
				{
					MetaCategoryInfo *newCategory = CreateMetaTree(categoryName);
					if(newCategory != NULL)
					{
						AppendMetaTree(&pMetaHandle->_pCategorytree, newCategory);
						if(strcasecmp(categoryName, "Title")!=0)
						{
							queryLength = (int32_t)strlen(queryBuffer);
							(void)sprintf(&queryBuffer[queryLength], " GROUP BY %s",categoryName);
						}
						queryLength = (int32_t)strlen(queryBuffer);
						(void)sprintf(&queryBuffer[queryLength], " ORDER BY %s collate %s ASC",categoryName,gCollateFunction);
						DEBUG_TCDB_PRINTF("query : %s \n", queryBuffer);
						rc = sqlite3_get_table(pMetaHandle->_db, queryBuffer, &result, &rows, &columns, &errmsg);
						if(SQLITE_OK == rc)
						{
							newCategory->_totalIndex = (uint32_t)rows;

							if (newCategory->_totalIndex < (UINT_MAX / sizeof(uint32_t)))
							{
								newCategory->_pList = malloc(sizeof(uint32_t) * newCategory->_totalIndex);
								if(NULL != newCategory->_pList )
								{
									// first row is column header
									for(i=1; i<rows+1; i ++)
									{
										int32_t listIndex = atoi(result[i]);
										if(listIndex < 0)
										{
											newCategory->_pList[i-1] = (uint32_t)INVALID_INDEX;
										}
										else
										{
											newCategory->_pList[i-1] = (uint32_t)listIndex;
										}
									}
									count = (int32_t)newCategory->_totalIndex;
								}
								else
								{
									ERROR_TCDB_PRINTF("Allocate fail\n");
								}
							}
							else
							{
								ERROR_TCDB_PRINTF("Allocate fail\n");
							}
						}
						else
						{
							ERROR_TCDB_PRINTF("Get sqlite table error (%d)\n", rc);
						}
						sqlite3_free_table(result);
					}
					else
					{
						ERROR_TCDB_PRINTF("Allocate fail\n");
					}
				}
				else
				{
					ERROR_TCDB_PRINTF("Pre Category select error\n");
				}
			}
		}

		(void)pthread_mutex_unlock(&pDBInfo->_handleLock);
		PrintMetaTree(pMetaHandle->_pCategorytree);
	}
	else
	{
		ERROR_TCDB_PRINTF("Not find DB\n");
	}

	DEBUG_TCDB_PRINTF("return : %d\n", count);
	return count;

}

int32_t TCDB_Meta_GetNumberCategory_Update(uint32_t dbNum,const char * categoryName)
{
	int32_t count = (int32_t)INVALID_INDEX;
	char **result;
	char *errmsg;
	int32_t rows, columns;
	int32_t rc, i;
	MetaCategoryInfo *pCategoryInfo;
	pDB_Info pDBInfo =  &pgDB_Info[dbNum];
	pMetaBrwsHandle pMetaHandle = &pgMetaBrwsHandle[dbNum];

	DEBUG_TCDB_PRINTF("dbNum (%d), category Name = %s\n", dbNum, categoryName);

	if((NULL != pMetaHandle)&&(pMetaHandle->_db != NULL)&&(NULL != pDBInfo))
	{
		(void)pthread_mutex_lock(&pDBInfo->_handleLock);
		 /* first category */
		if(pMetaHandle->_pCategorytree == NULL)
		{
			char * sqlQuery;

			pCategoryInfo = CreateMetaTree(categoryName);
			if(NULL != pCategoryInfo)
			{
				pMetaHandle->_pCategorytree = pCategoryInfo;

				if(strcasecmp(categoryName, "Title")!=0)
				{
					sqlQuery = (char *)newformat("SELECT FileIndex from TagTable WHERE %s IS NOT NULL GROUP BY %s ORDER BY %s collate %s ASC", categoryName, categoryName,categoryName,gCollateFunction);
				}
				else
				{
					sqlQuery = (char *)newformat("SELECT FileIndex from TagTable WHERE %s IS NOT NULL ORDER BY %s collate %s ASC", categoryName,categoryName,gCollateFunction);
				}
				DEBUG_TCDB_PRINTF("query : %s \n", sqlQuery);
				if(sqlQuery != NULL)
				{
					rc = sqlite3_get_table(pMetaHandle->_db, sqlQuery, &result, &rows, &columns, &errmsg);
					if(SQLITE_OK == rc)
					{
						DEBUG_TCDB_PRINTF("rows count : %d \n", rows);
						pCategoryInfo->_totalIndex = (uint32_t)rows;

						if(pCategoryInfo->_totalIndex < (UINT_MAX / sizeof(uint32_t)))
						{
							pCategoryInfo->_pList = malloc(sizeof(uint32_t)*pCategoryInfo->_totalIndex);
							if(NULL != pCategoryInfo->_pList )
							{
								// first row is column header
								for(i=1; i<(rows+1); i ++)
								{
									int32_t listIndex = atoi(result[i]);
									if(listIndex < 0 )
									{
										pCategoryInfo->_pList[i-1] = (uint32_t)INVALID_INDEX;
									}
									else
									{
										pCategoryInfo->_pList[i-1] = (uint32_t)listIndex;
									}
								}
								count = rows;
							}
							else
							{
								ERROR_TCDB_PRINTF("Allocate fail\n");
							}
						}
						else
						{
							ERROR_TCDB_PRINTF("Allocate fail\n");
						}
					}
					else
					{
						ERROR_TCDB_PRINTF("Get sqlite table error (%d)\n", rc);
					}

					sqlite3_free_table(result);
					free(sqlQuery);
				}
				else
				{
					ERROR_TCDB_PRINTF("Not enough memory\n");
				}
			}
			else
			{
				ERROR_TCDB_PRINTF("Allocate fail\n");
			}

		}
		/* sub category */
		else
		{
			char queryBuffer[SQL_TAG_QUERY_STRING_SIZE];
			int32_t queryLength;
			int32_t preSelect = 1;
			MetaCategoryInfo *findCategory;

			findCategory  = pMetaHandle->_pCategorytree;

			while(findCategory != NULL)
			{
				if(strcasecmp(findCategory->_pCategory, categoryName)==0)
				{
					RemoveLastMetaTree(&findCategory);
					break;
				}
				findCategory = findCategory->_pNextCategory;
			}

			pCategoryInfo = pMetaHandle->_pCategorytree;

			if((pCategoryInfo->_pCategory != NULL) && (pCategoryInfo->_selectIndex == META_SELECT_ALL))
			{
				(void)sprintf(queryBuffer, "SELECT FileIndex from TagTable WHERE %s IS NOT NULL", categoryName);
			}
			else if((pCategoryInfo->_pCategory != NULL) && (pCategoryInfo->_selectIndex != (uint32_t)INVALID_INDEX))
			{
				(void)sprintf(queryBuffer, "SELECT FileIndex from TagTable WHERE %s IS NOT NULL and %s = \"%s\"", categoryName, pCategoryInfo->_pCategory, pCategoryInfo->_selectTagName);
			}
			else
			{
				ERROR_TCDB_PRINTF("Pre Category select error : category (%s), subcategory (%s) \n", pCategoryInfo->_pCategory,pCategoryInfo->_selectTagName);
				preSelect = 0;
			}

			while((pCategoryInfo ->_pNextCategory != NULL)&&(preSelect != 0))
			{
				pCategoryInfo = pCategoryInfo ->_pNextCategory;
				if((pCategoryInfo->_pCategory != NULL) && (pCategoryInfo->_selectIndex == META_SELECT_ALL))
				{
					;
				}
				else if((pCategoryInfo->_pCategory != NULL) && (pCategoryInfo->_selectIndex != (uint32_t)INVALID_INDEX))
				{
					queryLength = (int32_t)strlen(queryBuffer);
					(void)sprintf(&queryBuffer[queryLength], " and %s = \"%s\"",pCategoryInfo->_pCategory, pCategoryInfo->_selectTagName);
				}
				else
				{
					ERROR_TCDB_PRINTF("Pre Category select error : category (%s), subcategory (%s) \n", pCategoryInfo->_pCategory,pCategoryInfo->_selectTagName);
					preSelect = 0;
				}
			}
			if(preSelect == 1)
			{
				MetaCategoryInfo *newCategory = CreateMetaTree(categoryName);
				if(newCategory != NULL)
				{
					AppendMetaTree(&pMetaHandle->_pCategorytree, newCategory);
					if(strcasecmp(categoryName, "Title")!=0)
					{
						queryLength = (int32_t)strlen(queryBuffer);
						(void)sprintf(&queryBuffer[queryLength], " GROUP BY %s",categoryName);
					}
					queryLength = (int32_t)strlen(queryBuffer);
					(void)sprintf(&queryBuffer[queryLength], " ORDER BY %s collate %s ASC",categoryName,gCollateFunction);
					DEBUG_TCDB_PRINTF("query : %s \n", queryBuffer);
					rc = sqlite3_get_table(pMetaHandle->_db, queryBuffer, &result, &rows, &columns, &errmsg);
					if(SQLITE_OK == rc)
					{
						newCategory->_totalIndex = (uint32_t)rows;
						if (newCategory->_totalIndex < (UINT_MAX / sizeof(uint32_t)))
						{
							newCategory->_pList = malloc(sizeof(uint32_t)*newCategory->_totalIndex);
							if(NULL != newCategory->_pList )
							{
								// first row is column header
								for(i=1; i<rows+1; i ++)
								{

									int32_t listIndex = atoi(result[i]);
									if(listIndex < 0 )
									{
										newCategory->_pList[i-1] = (uint32_t)INVALID_INDEX;
									}
									else
									{
										newCategory->_pList[i-1] = (uint32_t)listIndex;
									}
								}
								count = (int32_t)newCategory->_totalIndex;
							}
							else
							{
								ERROR_TCDB_PRINTF("Allocate fail\n");
							}
						}
						else
						{
							ERROR_TCDB_PRINTF("Allocate fail\n");
						}
					}
					else
					{
						ERROR_TCDB_PRINTF("Get sqlite table error (%d)\n", rc);
					}

					sqlite3_free_table(result);
				}
				else
				{
					ERROR_TCDB_PRINTF("Allocate fail\n");
				}
			}
			else
			{
				ERROR_TCDB_PRINTF("Pre Category select error\n");
			}
		}
		(void)pthread_mutex_unlock(&pDBInfo->_handleLock);
		PrintMetaTree(pMetaHandle->_pCategorytree);
	}
	else
	{
		ERROR_TCDB_PRINTF("Not find DB\n");
	}

	DEBUG_TCDB_PRINTF("return : %d\n", count);
	return count;

}

int32_t TCDB_Meta_FindCategoryList(uint32_t dbNum, const char * category, uint32_t findDBIndex)
{
	int32_t listIndex = (int32_t)INVALID_INDEX;
	MetaCategoryInfo *pCategoryInfo, *pFindCategory=NULL;
	pMetaBrwsHandle pMetaHandle = &pgMetaBrwsHandle[dbNum];

	DEBUG_TCDB_PRINTF("dbNum (%d), category Name = %s , readIndex = %d \n", dbNum,category, findDBIndex);
	if((pMetaHandle != NULL)&&((pMetaHandle->_db != NULL))&&(category != NULL))
	{
		pCategoryInfo = pMetaHandle->_pCategorytree;
		while(pCategoryInfo != NULL)
		{
			if(strcasecmp(pCategoryInfo->_pCategory, category)==0)
			{
				pFindCategory = pCategoryInfo;
			}
			pCategoryInfo = pCategoryInfo->_pNextCategory;
		}

		if(pFindCategory!=NULL)
		{
			uint32_t i;
			for(i=0; i< pFindCategory->_totalIndex; i++)
			{
				if(pFindCategory->_pList[i] == findDBIndex)
				{
					listIndex =i;
					break;
				}
			}
		}
		else
		{
			ERROR_TCDB_PRINTF("Not find category\n");
		}
	}
	return listIndex;
}

int32_t TCDB_Meta_GetCategoryList(uint32_t dbNum, const char * category, uint32_t readIndex)
{
	int32_t fileIndex = (int32_t)INVALID_INDEX;
	MetaCategoryInfo *pCategoryInfo, *pFindCategory=NULL;
	pMetaBrwsHandle pMetaHandle = &pgMetaBrwsHandle[dbNum];

	DEBUG_TCDB_PRINTF("dbNum (%d), category Name = %s , readIndex = %d \n", dbNum,category, readIndex);
	if((pMetaHandle != NULL)&&((pMetaHandle->_db != NULL))&&(category != NULL))
	{
		pCategoryInfo = pMetaHandle->_pCategorytree;
		DEBUG_TCDB_PRINTF("category Name = %s , readIndex = %d \n", category, readIndex);
		while(pCategoryInfo != NULL)
		{
			if(strcasecmp(pCategoryInfo->_pCategory, category)==0)
			{
				pFindCategory = pCategoryInfo;
			}
			pCategoryInfo = pCategoryInfo->_pNextCategory;
		}

		if(pFindCategory!=NULL)
		{
			fileIndex = (int32_t)pFindCategory->_pList[readIndex];
		}
		else
		{
			ERROR_TCDB_PRINTF("Not find category\n");
		}
	}
	return fileIndex;

}

int32_t TCDB_Meta_GetCategoryListName(uint32_t dbNum,const char * category, uint32_t readIndex, char *name, uint32_t bufSize)
{
	int32_t ret = DB_SUCCESS;
	int32_t fileIndex;
	int32_t rc;
	MetaCategoryInfo *pCategoryInfo, *pFindCategory=NULL;
	pDB_Info pDBInfo =  &pgDB_Info[dbNum];
	pMetaBrwsHandle pMetaHandle = &pgMetaBrwsHandle[dbNum];

	if((pMetaHandle != NULL)&&(pMetaHandle->_db != NULL)&&(name != NULL)&&(pDBInfo != NULL))
	{
		(void)pthread_mutex_lock(&pDBInfo->_handleLock);
		pCategoryInfo = pMetaHandle->_pCategorytree;

		DEBUG_TCDB_PRINTF("dbNum (%d) category Name = %s , readIndex = %d \n", dbNum, category, readIndex);
		while(pCategoryInfo != NULL)
		{
			if(strcasecmp(pCategoryInfo->_pCategory, category)==0)
			{
				pFindCategory = pCategoryInfo;
			}
			pCategoryInfo = pCategoryInfo->_pNextCategory;
		}

		if(pFindCategory!=NULL)
		{
			if(readIndex < pFindCategory->_totalIndex)
			{
				fileIndex = (int32_t)pFindCategory->_pList[readIndex];
			}
			else
			{
				fileIndex = (int32_t)INVALID_INDEX;
			}
			if(fileIndex > 0)
			{
				int32_t isBufLack = -1;
				DEBUG_TCDB_PRINTF("category Name = %s , fileIndex = %d \n", category, fileIndex);
				rc = Sql_Select_TextColumn_byIntegerType(pMetaHandle->_db,category ,TAG_TABLE , "FileIndex", fileIndex,name,bufSize,&isBufLack);
				if(rc == SQLITE_OK)
				{
					DEBUG_TCDB_PRINTF("Tag Info = %s \n",name);
					if(isBufLack == 0)
					{
						ret = DB_SUCCESS;
					}
					else
					{
						ret = DB_NOT_ENOUGH_BUFFER;
					}
				}
				else
				{
					ret = DB_SQLITE_SELECT_ERR;
				}
			}
			else
			{
				ret = DB_INVALIDE_INDEX_ERR;
			}
		}
		else
		{
			ERROR_TCDB_PRINTF("Not find category\n");
		}
	(void)pthread_mutex_unlock(&pDBInfo->_handleLock);
	}
	return ret;

}

int32_t TCDB_Meta_SelectCategory(uint32_t dbNum, const char * category, int32_t selectIndex)
{
	int32_t ret = DB_UNKOWN_ERR;
	MetaCategoryInfo *pCurrentCategory;
	pMetaBrwsHandle pMetaHandle = &pgMetaBrwsHandle[dbNum];

	DEBUG_TCDB_PRINTF("dbNum(%d), category Name = %s , selectIndex = %d \n", dbNum, category, selectIndex);

	if((pMetaHandle != NULL)&&(pMetaHandle->_db != NULL))
	{
		pCurrentCategory = pMetaHandle->_pCategorytree;

		if(pCurrentCategory != NULL)
		{
			while(pCurrentCategory->_pNextCategory!= NULL)
			{
				pCurrentCategory = pCurrentCategory->_pNextCategory;
			}

			if(strcasecmp(pCurrentCategory->_pCategory, category)==0)
			{
				if(selectIndex >-1)
				{
					pCurrentCategory->_selectIndex = (uint32_t)selectIndex;
					ret = TCDB_Meta_GetCategoryListName(dbNum,pCurrentCategory->_pCategory, (uint32_t)selectIndex , pCurrentCategory->_selectTagName,MAX_TAG_LENGTH);
					if(ret != DB_SUCCESS)
					{
						ERROR_TCDB_PRINTF("Get Gategory Name read error (%d)\n",ret);
					}
				}
				else
				{
					pCurrentCategory->_selectIndex = META_SELECT_ALL;
				}
			}
			else
			{
				ERROR_TCDB_PRINTF("last Category (%s), select Category (%s)\n", pCurrentCategory->_pCategory, category);
				ret = DB_INVALIDE_CATEGORY_ERR;
			}
		}
		else
		{
			ret = DB_INVALIDE_INDEX_ERR;
		}
		PrintMetaTree(pMetaHandle->_pCategorytree);
	}
	else
	{
		ret = DB_NOT_INITIALIZE;
	}

	return ret;
}

int32_t TCDB_Meta_UndoCategory(uint32_t dbNum)
{
	int32_t ret = DB_UNKOWN_ERR;
	MetaCategoryInfo *pCurrentCategory;

	DEBUG_TCDB_PRINTF("dbNum(%d)\n",dbNum);
	pMetaBrwsHandle pMetaHandle = &pgMetaBrwsHandle[dbNum];

	if((pMetaHandle != NULL)&&(pMetaHandle->_db != NULL))
	{
		pCurrentCategory = pMetaHandle->_pCategorytree;

		if(pCurrentCategory != NULL)
		{
			int32_t categoryDepth, i;
			categoryDepth = GetTreeDepth(pCurrentCategory);
			if(categoryDepth ==0)
			{
				DestroyMetaTree(pCurrentCategory);
				pMetaHandle->_pCategorytree = NULL;
			}
			else
			{
				for(i=0; i <(categoryDepth-1); i++)
				{
					pCurrentCategory = pCurrentCategory->_pNextCategory;
				}
				DestroyMetaTree(pCurrentCategory->_pNextCategory);
				pCurrentCategory->_pNextCategory = NULL;
				pCurrentCategory->_selectIndex = (uint32_t)INVALID_INDEX;
				(void)memset(pCurrentCategory->_selectTagName, 0x00, MAX_TAG_LENGTH);
				ret = DB_SUCCESS;
			}
		}
		else
		{
			ret = DB_INVALIDE_INDEX_ERR;
		}

		PrintMetaTree(pMetaHandle->_pCategorytree);
	}
	else
	{
		ret = DB_NOT_INITIALIZE;
	}
	return ret;
}

int32_t TCDB_Meta_MakePlayList(uint32_t dbNum,int32_t selectIndex)
{
	int32_t iReturn = DB_UNKOWN_ERR;

	DEBUG_TCDB_PRINTF("dbNum(%d), selectIndex(%d)\n",dbNum, selectIndex);
	pMetaBrwsHandle pMetaHandle = &pgMetaBrwsHandle[dbNum];

	if((pMetaHandle != NULL)&&(pMetaHandle->_db != NULL) && ( NULL != pMetaHandle->_pCategorytree))
	{
		MetaCategoryInfo * pCurrentCategory;
		pCurrentCategory = pMetaHandle->_pCategorytree;

		while(NULL != pCurrentCategory->_pNextCategory)
		{
			pCurrentCategory = pCurrentCategory->_pNextCategory;
		}

		if((strcasecmp(pCurrentCategory->_pCategory, "Title")==0)&&(NULL != pCurrentCategory->_pList))
		{
			if(NULL != pMetaHandle->_pPlayList)
			{
				free(pMetaHandle->_pPlayList);
				pMetaHandle->_pPlayList = NULL;
			}
			pMetaHandle->_pPlayList = malloc(sizeof(uint32_t)*pCurrentCategory->_totalIndex);
			if(NULL != pMetaHandle->_pPlayList )
			{
				(void)memcpy(pMetaHandle->_pPlayList, pCurrentCategory->_pList,(sizeof(uint32_t)*pCurrentCategory->_totalIndex));
				pMetaHandle->_selectTrackNum = selectIndex;
				pMetaHandle->_totalTrakNum = (int32_t)pCurrentCategory->_totalIndex;
				iReturn = DB_SUCCESS;
			}
		}
		else
		{
			iReturn = DB_INVALIDE_CATEGORY_ERR;
		}
		PrintMetaTree(pMetaHandle->_pCategorytree);
	}
	else
	{
		iReturn = DB_INVALIDE_CATEGORY_ERR;
		ERROR_TCDB_PRINTF("Not find DB\n");
	}

	return iReturn;

}

uint32_t TCDB_Meta_GetTotalTrack(uint32_t dbNum)
{
	uint32_t totalTrack=0;
	pMetaBrwsHandle pMetaHandle = &pgMetaBrwsHandle[dbNum];

	DEBUG_TCDB_PRINTF("dbNum(%d) \n", dbNum);

	if((pMetaHandle->_db != NULL) && (pMetaHandle->_pPlayList != NULL))
	{
		totalTrack = (uint32_t)pMetaHandle->_totalTrakNum;
	}
	return totalTrack;
}

uint32_t TCDB_Meta_GetTrackList(uint32_t dbNum, uint32_t listIndex)
{
	uint32_t trackList = (uint32_t)INVALID_INDEX;
	pMetaBrwsHandle pMetaHandle = &pgMetaBrwsHandle[dbNum];

	if((pMetaHandle->_db != NULL) && ( 0 != pMetaHandle->_totalTrakNum)&& (pMetaHandle->_pPlayList != NULL))
	{
		if(listIndex < (uint32_t)pMetaHandle->_totalTrakNum)
		{
			trackList = pMetaHandle->_pPlayList[listIndex];
		}
	}
	return trackList;
}

uint32_t TCDB_Meta_GetFirstSelectTrackIndex(uint32_t dbNum)
{
	uint32_t trackIndex = (uint32_t)INVALID_INDEX;
	pMetaBrwsHandle pMetaHandle = &pgMetaBrwsHandle[dbNum];

	DEBUG_TCDB_PRINTF("dbNum(%d) \n", dbNum);

	if((pMetaHandle->_db != NULL) && ( 0 != pMetaHandle->_totalTrakNum)&& (pMetaHandle->_pPlayList != NULL))
	{
		trackIndex = (uint32_t)pMetaHandle->_selectTrackNum;
	}
	return trackIndex;
}

#endif		//META_DB_INCLUDE

#ifdef __cplusplus
}
#endif
