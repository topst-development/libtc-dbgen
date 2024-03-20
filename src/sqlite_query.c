/****************************************************************************************
 *   FileName    : sqlite_query.c
 *   Description : c file for APIs of sqlite query
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
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <linux/types.h>
#include <stdint.h>
#include "sqlite3.h"
#include "TCDBDefine.h"
#include "sqlite_query.h"
#include "TCUtil.h"

#ifdef __cplusplus
extern "C" {
#endif

static int32_t Sql_InsertFolderSortTable(sqlite3 *db, uint32_t folderIndex);
static int32_t Sql_InsertFileSortTable(sqlite3 *db, uint32_t fileIndex, const char *tableName);

static int32_t Sql_CreateDeviceTable(sqlite3 *db);
static int32_t Sql_CreateFolderTable(sqlite3 *db);
static int32_t Sql_CreateAudioFileTable(sqlite3 *db);
static int32_t Sql_CreateVideoFileTable(sqlite3 *db);
static int32_t Sql_CreateImageFileTable(sqlite3 *db);
static int32_t Sql_CreateFolderSortTable(sqlite3 *db);
static int32_t Sql_CreateAudioSortTable(sqlite3 *db);
static int32_t Sql_CreateVideoSortTable(sqlite3 *db);
static int32_t Sql_CreateImageSortTable(sqlite3 *db);
static int32_t Sql_DropTable(sqlite3 *db, const char *tableName);
static int32_t Sql_InsertAudioFileList(sqlite3 *db, const char *name, int32_t parentIndex, int32_t iNode);
static int32_t Sql_InsertVideoFileList(sqlite3 *db, const char *name, int32_t parentIndex, int32_t iNode);
static int32_t Sql_InsertImageFileList(sqlite3 *db, const char *name, int32_t parentIndex, int32_t iNode);


/* SQLite common wrapping functions*/
static int32_t Sql_Select_IntegerColumn_byIntegerType
	(sqlite3 *db, const char *SelectColumn, const char *FromTable, const char *WhereColumn, int32_t termsValue);
static int32_t sql_Select_RowCount_byIntegerType
	(sqlite3 *db, const char *FromTable, const char *WhereColumn, int32_t termsValue);
static int32_t Sql_Select_TotalRowCount(sqlite3 *db, const char *TableName);
static int32_t Sql_Update_IntegerColumn
	(sqlite3 *db, const char *UpdatTable, const char *UpdateColumn, int32_t SetValue, const char *TermsColumn,int32_t TermsValue);
static int32_t Sql_Update_TextColumn
	(sqlite3 *db, const char *UpdatTable, const char *UpdateColumn, const char *SetValue, const char *TermsColumn,int32_t TermsValue);
static int32_t Sql_Delete_Record(sqlite3 *db, const char *TargetTable, const char *TermsColumn, int32_t TermsValue);

/********************************************************************************************
Function : Open_DB()
description : open datasbase file - call sqlite3_open()
argument
	dbpath : databaes file full path
	device
return
	Succes  : 0
	Err	: 1 ~ 
********************************************************************************************/
sqlite3 * Sql_Open_DB(const char *dbpath)
{
	int32_t err;
	sqlite3* db=NULL;
	if(dbpath != NULL)
	{
		INFO_TCDB_PRINTF("Open DB (%s)\n", dbpath)
		err = sqlite3_open(dbpath, &db);
		if(err != SQLITE_OK)
		{
			db = NULL;
			ERROR_TCDB_PRINTF("sqlite3 open err (%d)\n",err );
		}
	}
	return db;
}

/********************************************************************************************
Function : Close_DB()
description : close datasbase file - call sqlite3_close()
argument
return
	Succes  : 0
	Err	: 1 ~
********************************************************************************************/
int32_t Sql_Close_DB(sqlite3 *db)
{
	int32_t err = SQLITE_OK;
	INFO_TCDB_PRINTF("Close DB\n");
	if( db != NULL)
	{
		err = sqlite3_close(db);
		ERROR_TCDB_PRINTF("sqlite3 close err (%d)\n",err );
	}
	return err;
}

/*
eTextRep : Encoding Type
	SQLITE_UTF8
	SQLITE_UTF16LE
	SQLITE_UTF16BE
	SQLITE_UTF16
	SQLITE_ANY
	SQLITE_UTF16_ALIGNED
*/
int32_t Sql_CreateCollation( sqlite3* db,  const char *zName,  int32_t eTextRep,  void *pArg,  int32_t(*xCompare)(void *data, int32_t len1, const void* in1, int32_t len2, const void* in2))
{
	int32_t ret;

	ret = sqlite3_create_collation(db, zName, eTextRep, pArg, xCompare);

	return ret;
}

int32_t Sql_SetPragma(sqlite3* db)
{
	int32_t rc = SQLITE_ERROR;
	char *errMsg = NULL;

	if(db != NULL)
	{
		rc = sqlite3_exec(db, "PRAGMA PAGE_SIZE = 4096", NULL, NULL, &errMsg);
		if(rc != SQLITE_OK)
		{
			ERROR_TCDB_PRINTF("Sqlite Set PRAGMA error (%s)\n", errMsg);
		}

		rc = sqlite3_exec(db, "PRAGMA default_cache_size = 10", NULL, NULL, &errMsg);
		if(rc != SQLITE_OK)
		{
			ERROR_TCDB_PRINTF("Sqlite Set PRAGMA error (%s)\n", errMsg);
		}

		rc = sqlite3_exec(db, "PRAGMA cache_size = 10", NULL, NULL, &errMsg);
		if(rc != SQLITE_OK)
		{
			ERROR_TCDB_PRINTF("Sqlite Set PRAGMA error (%s)\n", errMsg);
		}

		rc = sqlite3_exec(db, "PRAGMA synchronous = OFF", NULL, NULL, &errMsg);
		if(rc != SQLITE_OK)
		{
			ERROR_TCDB_PRINTF("Sqlite Set PRAGMA error (%s)\n", errMsg);
		}

		rc = sqlite3_exec(db, "PRAGMA journal_mode = OFF", NULL, NULL, &errMsg);	
		if(rc != SQLITE_OK)
		{
			ERROR_TCDB_PRINTF("Sqlite Set PRAGMA error (%s)\n", errMsg);
		}
		rc = sqlite3_exec(db, "PRAGMA count_changes=OFF", NULL, NULL, &errMsg);
		if(rc != SQLITE_OK)
		{
			ERROR_TCDB_PRINTF("Sqlite Set PRAGMA error (%s)\n", errMsg);
		}

		rc = sqlite3_exec(db, "PRAGMA temp_store=file", NULL, NULL, &errMsg);
		if(rc != SQLITE_OK)
		{
			ERROR_TCDB_PRINTF("Sqlite Set PRAGMA error (%s)\n", errMsg);
		}
	}
	return rc;

}


/********************************************************************************************
Function : Transaction_Start
description : transaction start
arguments
	sqlite3 *db		 	: sqlite3 handle
return
	void
********************************************************************************************/
int32_t Sql_Transaction_Start(sqlite3 *db)
{
	int32_t rc = SQLITE_ERROR;
	char *errMsg = NULL;

	if(db != NULL)
	{
		rc = sqlite3_exec(db, "BEGIN transaction;", NULL, NULL, &errMsg);
		if(rc != SQLITE_OK)
		{
			ERROR_TCDB_PRINTF("Sqlite Set PRAGMA error (%s)\n", errMsg);
		}
	}
	return rc;
}

/********************************************************************************************
Function : Transaction_END
description : transaction end
arguments
	sqlite3 *db		 	: sqlite3 handle
return
	void
********************************************************************************************/
int32_t Sql_Transaction_End(sqlite3 *db)
{
	int32_t rc = SQLITE_ERROR;
	if(db != NULL)
	{
		rc = sqlite3_exec(db, "END transaction;", NULL, NULL, NULL);
		if(rc != SQLITE_OK)
		{
			ERROR_TCDB_PRINTF("sqlite3_preparev2 error: %d(%s)\n", rc, sqlite3_errmsg(db));
		}
	}
	return rc;
}

int32_t Sql_DropTableAll(sqlite3 *db)
{
	int32_t rc = SQLITE_ERROR;

	if(db != NULL)
	{
		rc = Sql_DropTable(db, DEVICE_TABLE);
		if(rc == SQLITE_OK)
		{
			rc = Sql_DropTable(db, FOLDER_SORT_TABLE);
		}
		if(rc == SQLITE_OK)
		{
			rc = Sql_DropTable(db, AUDIO_FILE_TABLE);
		}
		if(rc == SQLITE_OK)
		{
			rc = Sql_DropTable(db, VIDEO_FILE_TABLE);
		}
		if(rc == SQLITE_OK)
		{
			rc = Sql_DropTable(db, IMAGE_FILE_TABLE);
		}
		if(rc == SQLITE_OK)
		{
			rc = Sql_DropTable(db, FOLDER_TABLE);	
		}

#ifdef META_DB_INCLUDE		
		if(rc == SQLITE_OK)
		{
			rc = Sql_DropTable(db, TAG_TABLE);
		}
#endif
	}

	return rc;
}

int32_t Sql_InitTables(sqlite3 *db)
{
	int32_t rc = SQLITE_ERROR;
	char *errMsg = NULL;

	if (db != NULL)
	{
		rc = Sql_CreateDeviceTable(db);

		if (rc == SQLITE_OK)
		{
			/* init folerlist tble */
			rc = Sql_CreateFolderTable(db);
		}
		
		if (rc == SQLITE_OK)
		{
			/* make filelist tble */
			rc = Sql_CreateAudioFileTable(db);
		}

		if (rc == SQLITE_OK)
		{
			/* make filelist tble */
			rc = Sql_CreateVideoFileTable(db);
		}

		if (rc == SQLITE_OK)
		{
			/* make filelist tble */
			rc = Sql_CreateImageFileTable(db);
		}

#ifdef META_DB_INCLUDE
		if(rc == SQLITE_OK)
		{
			rc = Sql_CreateTagTable(db);
		}
#endif
		if (rc == SQLITE_OK)
		{
			/* enable foreign key */
			rc = sqlite3_exec(db, "PRAGMA foreign_keys=1;", NULL , NULL, &errMsg);
			if (rc != SQLITE_OK)
			{
				ERROR_TCDB_PRINTF("set PRAGMA foreign key error : %s \n" , errMsg);
				sqlite3_free(errMsg);
			}		
		}
	}

	return rc;	
}

/********************************************************************************************
Function : InsertDeviceInfo
description : Insert Device Info into Device Table
arguments
	sqlite3 * db
	char * sqlQuery
	DeviceTable_Info* deviceInfo
return 
	Success : SQLITE_OK (0)
	Err		: SQLITE_ERROR (1) ~ 
********************************************************************************************/
int32_t Sql_InsertDeviceInfo(sqlite3 *db, DeviceTable_Info* deviceInfo)
{
	sqlite3_stmt *state;
	const char *tail;
	char *errmsg;
	int32_t rc = SQLITE_ERROR;
	if ((db != NULL) && (deviceInfo != NULL) &&(deviceInfo->_rootPath != NULL))
	{
		DEBUG_TCDB_PRINTF("TotalFolderCount (%d), TotalAudioFileCount (%d), TotalVideoFileCount (%d), TotalImageFileCount (%d), Used(%d), RootPath(%s) \n",
			deviceInfo->_folderCount,
			deviceInfo->_audioFileCount,
			deviceInfo->_videoFileCount,
			deviceInfo->_imageFileCount,
			deviceInfo->_used,
			deviceInfo->_rootPath);

		rc = sqlite3_exec(db, "DELETE FROM "DEVICE_TABLE, NULL, NULL, &errmsg);
		if (rc != SQLITE_OK)
		{
			ERROR_TCDB_PRINTF("DeviceTable Deleted fail, error : %d(%s) \n", rc, errmsg);
		}		
		else
		{	
			char sqlQuery[] = "insert into "DEVICE_TABLE" (TotalFolderCount, TotalAudioFileCount, TotalVideoFileCount, TotalImageFileCount,Used, RootPath) "
					"values(:Folder_Count ,:audioCount,:videoCount,:imageCount,:used,:path)";

			rc = sqlite3_prepare_v2(db, sqlQuery, (int32_t)strlen(sqlQuery), &state, &tail);
			if (rc == SQLITE_OK)
			{
				(void)sqlite3_bind_int(state, sqlite3_bind_parameter_index(state,":Folder_Count"),(int32_t)deviceInfo->_folderCount);
				(void)sqlite3_bind_int(state, sqlite3_bind_parameter_index(state,":audioCount"),(int32_t)deviceInfo->_audioFileCount);
				(void)sqlite3_bind_int(state, sqlite3_bind_parameter_index(state,":videoCount"),(int32_t)deviceInfo->_videoFileCount);
				(void)sqlite3_bind_int(state, sqlite3_bind_parameter_index(state,":imageCount"),(int32_t)deviceInfo->_imageFileCount);
				(void)sqlite3_bind_int(state, sqlite3_bind_parameter_index(state,":used"),(int32_t)deviceInfo->_used);
				(void)sqlite3_bind_text(state, sqlite3_bind_parameter_index(state,":path"), deviceInfo->_rootPath, (int32_t)strlen(deviceInfo->_rootPath), SQLITE_STATIC);
				rc = sqlite3_step(state);

				if ((rc == SQLITE_OK) || (rc == SQLITE_DONE))
				{
					rc = SQLITE_OK;
				}
				else
				{
					ERROR_TCDB_PRINTF("Set "DEVICE_TABLE" error: %d(%s)\n", rc, sqlite3_errmsg(db));
				}
				(void)sqlite3_finalize(state);
			}
			else
			{
				ERROR_TCDB_PRINTF("sqlite3_preparev2 error: %d(%s)\n", rc, sqlite3_errmsg(db));
			}
		}
	}
	else
	{
		ERROR_TCDB_PRINTF("arguments error \n");
	}
	return rc;
}

/********************************************************************************************
Function : DeleteDeviceInfo
description : delete Device Info of Device Table
arguments
	sqlite3 *db
	char *sqlQuery
return 
	Success : SQLITE_OK (0)
	Err		: SQLITE_ERROR (1) ~
********************************************************************************************/
int32_t Sql_DeleteDeviceInfo(sqlite3 *db)
{
	int32_t rc = SQLITE_ERROR;
	char *errmsg;

	if(db != NULL)
	{
		rc = sqlite3_exec(db, "DELETE FROM "DEVICE_TABLE, NULL, NULL, &errmsg);
		if (rc != SQLITE_OK)
		{
			ERROR_TCDB_PRINTF(DEVICE_TABLE" Deleted fail, error : %d(%s) \n", rc, errmsg);
		}
	}
	return rc;
}

/********************************************************************************************
Function : GetRootPath
description : Get Mount Path from Device Table
arguments
	sqlite3 *db
	char *sqlQuery
	char *Path
return 	
	Success : SQLITE_OK (0)
	Err		: SQLITE_ERROR (1) ~ 
********************************************************************************************/
int32_t Sql_GetRootPath(sqlite3 *db, char *Path, uint32_t bufSize, int32_t *isBufLack)
{
	int32_t rc = SQLITE_ERROR;
	if ((db != NULL) && (Path!=NULL))
	{
		rc = Sql_Select_TextColumn_byIntegerType(db,"RootPath", DEVICE_TABLE, "DeviceIndex", 1, Path , bufSize, isBufLack);
	}
	return rc;
}

/********************************************************************************************
Function : GetUsedFlag
description : get used flag from Device Table
arguments
	sqlite3 *db
	char *sqlQuery
	int32_t used
return 	
	Success : 0 or 1
	Err		: -1
********************************************************************************************/
int32_t Sql_SetUsedFlag(sqlite3 *db, int32_t used)
{
	int32_t rc =SQLITE_ERROR;
	if (db != NULL)
	{
		rc = Sql_Update_IntegerColumn(db, DEVICE_TABLE, "Used", used, "DeviceIndex", 1);
		if(rc != SQLITE_OK)
		{
			ERROR_TCDB_PRINTF("Set Used flag(%d) error \n",used);
		}
	}	
	return rc;
	
}

/********************************************************************************************
Function : GetUsedFlag
description : return used flag from Device Table
arguments
	sqlite3 *db
	char *sqlQuery
return 	
	Success : SQLITE_OK (0)
	Err		: SQLITE_ERROR (1) ~
********************************************************************************************/
int32_t Sql_GetUsedFlag(sqlite3 *db)
{
	int32_t rc = -1;
	if (db != NULL) 
	{
		rc = Sql_Select_IntegerColumn_byIntegerType(db,"Used", DEVICE_TABLE, "DeviceIndex",1);
	}
	return rc;
}

/********************************************************************************************
Function : SetTotalFolderCount
description : Set Total Folder Count to Device Table
arguments
	sqlite3 *db
	char *sqlQuery
	int32_t count					: Folder number
return 	
	Success : SQLITE_OK (0)
	Err		: SQLITE_ERROR (1) ~ 
********************************************************************************************/
int32_t Sql_SetTotalFolderCount(sqlite3 *db, uint32_t count)
{
	int32_t rc = SQLITE_ERROR;
	if (db != NULL) 
	{
		rc = Sql_Update_IntegerColumn(db, DEVICE_TABLE, "TotalFolderCount", (int32_t)count, "DeviceIndex", 1);
	}
	return rc;	
}

/********************************************************************************************
Function : GetTotalFolderCount
description : return Folder Count from Device Table
arguments
	sqlite3 *db
	char *sqlQuery
return 	
	Folder Count : 0 ~
	Err		: -1
********************************************************************************************/
int32_t Sql_GetTotalFolderCount(sqlite3 *db)
{
	int32_t rc = -1;
	if (db != NULL) 
	{
		rc = Sql_Select_IntegerColumn_byIntegerType(db,"TotalFolderCount", DEVICE_TABLE, "DeviceIndex",1);
	}
	return rc;

}

/********************************************************************************************
Function : SetTotalAudioFileCount
description : Set Fil Count to Device Table
arguments
	sqlite3 *db
	char *sqlQuery
	int32_t count					: file number
return 	
	Success : SQLITE_OK (0)
	Err		: SQLITE_ERROR (1) ~ 
********************************************************************************************/
int32_t Sql_SetTotalAudioFileCount(sqlite3 *db, uint32_t count)
{
	int32_t rc = SQLITE_ERROR;
	if (db != NULL)
	{
		rc = Sql_Update_IntegerColumn(db, DEVICE_TABLE, "TotalAudioFileCount", (int32_t)count, "DeviceIndex", 1);
	}
	return rc;

}

/********************************************************************************************
Function : GetTotalAudioFileCount
description : return file Count from Device Table
arguments
	sqlite3 *db
	char *sqlQuery
return 	
	file count : 0 ~  
	Err		: -1
********************************************************************************************/
int32_t Sql_GetTotalAudioFileCount(sqlite3 *db)
{
	int32_t rc = -1;
	if (db != NULL)
	{
		rc = Sql_Select_IntegerColumn_byIntegerType(db,"TotalAudioFileCount", DEVICE_TABLE, "DeviceIndex",1);
	}
	return rc;
	
}

/********************************************************************************************
Function : SetTotalVideoFileCount
description : Set Fil Count to Device Table
arguments
	sqlite3 *db
	char *sqlQuery
	int32_t count					: file number
return 	
	Success : SQLITE_OK (0)
	Err		: SQLITE_ERROR (1) ~
********************************************************************************************/
int32_t Sql_SetTotalVideoFileCount(sqlite3 *db, uint32_t count)
{
	int32_t rc = SQLITE_ERROR;
	if (db != NULL) 
	{
		rc = Sql_Update_IntegerColumn(db, DEVICE_TABLE, "TotalVideoFileCount", (int32_t)count, "DeviceIndex", 1);
	}
	return rc;

}

/********************************************************************************************
Function : GetTotalVideoFileCount
description : return file Count from Device Table
arguments
	sqlite3 *db
	char *sqlQuery
return 	
	file count : 0 ~
	Err		: -1
********************************************************************************************/
int32_t Sql_GetTotalVideoFileCount(sqlite3 *db)
{
	int32_t rc = -1;
	if (db != NULL) 
	{
		rc = Sql_Select_IntegerColumn_byIntegerType(db,"TotalVideoFileCount", DEVICE_TABLE, "DeviceIndex",1);
	}
	return rc;
	
}

/********************************************************************************************
Function : SetTotalImageFileCount
description : Set Fil Count to Device Table
arguments
	sqlite3 *db
	char *sqlQuery
	int32_t count					: file number
return
	Success : SQLITE_OK (0)
	Err		: SQLITE_ERROR (1) ~
********************************************************************************************/
int32_t Sql_SetTotalImageFileCount(sqlite3 *db, uint32_t count)
{
	int32_t rc = SQLITE_ERROR;
	if (db != NULL) 
	{
		rc = Sql_Update_IntegerColumn(db, DEVICE_TABLE, "TotalImageFileCount", (int32_t)count, "DeviceIndex", 1);
	}
	return rc;

}

/********************************************************************************************
Function : GetTotalImageFileCount
description : return file Count from Device Table
arguments
	sqlite3 *db
	char *sqlQuery
return
	file count : 0 ~
	Err		: -1
********************************************************************************************/
int32_t Sql_GetTotalImageFileCount(sqlite3 *db)
{
	int32_t rc = -1;
	if (db != NULL)
	{
		rc = Sql_Select_IntegerColumn_byIntegerType(db,"TotalImageFileCount", DEVICE_TABLE, "DeviceIndex",1);
	}
	return rc;
}

/********************************************************************************************
Function : InsertFolderList
description : Insert Folder info to FolderList table
arguments

return 	
	Success : SQLITE_OK (0)
	Err		: SQLITE_ERROR (1) ~
********************************************************************************************/
int32_t Sql_InsertFolderList(sqlite3 *db, FolderTable_Info *pfolderInfo)
{
	int32_t rc = SQLITE_ERROR;
	if ((db != NULL) && (pfolderInfo!=NULL)&&(pfolderInfo->_name!=NULL))
	{
		sqlite3_stmt *state;
		const char *tail;

		DEBUG_TCDB_PRINTF("name(%s), full path (%s), parent index (%d), inode (%d), \n", 
			pfolderInfo->_name, 
			pfolderInfo->_fullPath, 
			pfolderInfo->_parentIndex, 
			pfolderInfo->_inode);

		char sqlQuery[] = 
				"insert into "FOLDER_TABLE "(Name, FullPath , ParentIndex, Inode) "
				"values(:name,:full_path,:parent_index,:inode)";

		rc = sqlite3_prepare_v2(db, sqlQuery, (int32_t)strlen(sqlQuery), &state, &tail);
		if (rc == SQLITE_OK)
		{
			(void)sqlite3_bind_text(state, sqlite3_bind_parameter_index(state,":name"), (char *)pfolderInfo->_name, (int32_t)strlen((char *)pfolderInfo->_name), SQLITE_STATIC);
			if(pfolderInfo->_fullPath != NULL)
			{
				(void)sqlite3_bind_text(state, sqlite3_bind_parameter_index(state,":full_path"), pfolderInfo->_fullPath, (int32_t)strlen(pfolderInfo->_fullPath), SQLITE_STATIC);
			}
			else
			{
				(void)sqlite3_bind_text(state, sqlite3_bind_parameter_index(state,":full_path"), NULL, 0, SQLITE_STATIC);
			}
			(void)sqlite3_bind_int(state, sqlite3_bind_parameter_index(state,":parent_index"), (int32_t)pfolderInfo->_parentIndex);
			(void)sqlite3_bind_int(state, sqlite3_bind_parameter_index(state,":inode"), (int32_t)pfolderInfo->_inode);
			
			rc = sqlite3_step(state);
			if ((rc == SQLITE_OK) || (rc == SQLITE_DONE))
			{
				rc = SQLITE_OK;
			}
			else
			{
				ERROR_TCDB_PRINTF("Set folderlist error: %d(%s)\n", rc, sqlite3_errmsg(db));
			}

			(void)sqlite3_finalize(state);
		}
		else
		{
			ERROR_TCDB_PRINTF("sqlite3_preparev2 error: %d(%s)\n", rc, sqlite3_errmsg(db));
		}
	}
	else
	{
		ERROR_TCDB_PRINTF("arguments error\n");
	}
	return rc;
}

/********************************************************************************************
Function : DeleteFolderList
description : delete folder from FolderList Table
arguments
	sqlite3 *db
	char *sqlQuery
	uint32_t index
return 
	Success : SQLITE_OK (0)
	Err		: SQLITE_ERROR (1) ~

Note :
Folder_Index was used foreign key of another FolderList or Filelist.
 In this case, you can't delete this folderList, because dependence.
If target folder has sub folders, You must delete all sub folders before delete target folder. 
********************************************************************************************/
int32_t Sql_DeleteFolderList(sqlite3 *db, uint32_t folderIndex)
{
	int32_t rc = SQLITE_ERROR;
	if (db != NULL)
	{
		rc = Sql_Delete_Record(db,FOLDER_TABLE, "FolderIndex", (int32_t)folderIndex);
	}

	return rc;
}

/********************************************************************************************
Function : GetFolderName
description : Get Folder Name from FolderList Table
arguments
	sqlite3 *db
	char *sqlQuery
	uint32_t index
	char *nameBuffer
return 
	Success : SQLITE_OK (0)
	Err		: SQLITE_ERROR (1) ~
********************************************************************************************/
int32_t Sql_GetFolderName(sqlite3 *db,uint32_t folderIndex, char *nameBuffer, uint32_t bufSize, int32_t *isBufLack)
{
	int32_t rc = SQLITE_ERROR;
	if ((db != NULL) && (nameBuffer != NULL) &&((int32_t)bufSize !=0) )
	{
		rc = Sql_Select_TextColumn_byIntegerType(db,"Name" ,FOLDER_TABLE , "FolderIndex", (int32_t)folderIndex,nameBuffer, bufSize,isBufLack);
	}
	return rc;
}

/********************************************************************************************
Function : Sql_SetFolderFullpath
description : Get Folder path from FolderList Table
arguments
	sqlite3 *db
	char *sqlQuery
	uint32_t index
	char *pathBuffer
return 
	Success : SQLITE_OK (0)
	Err		: SQLITE_ERROR (1) ~ 
********************************************************************************************/
int32_t Sql_SetFolderFullpath(sqlite3 *db, uint32_t folderIndex, char *pathBuffer)
{
	int32_t rc = SQLITE_ERROR;
	if ((db != NULL) && (pathBuffer!=NULL))
	{
		rc =Sql_Update_TextColumn(db, FOLDER_TABLE, "FullPath", pathBuffer, "FolderIndex",(int32_t)folderIndex);
	}
	
	return rc;
}

/********************************************************************************************
Function : GetFolderFullpath
description : Get Folder path from FolderList Table
arguments
	sqlite3 *db
	char *sqlQuery
	uint32_t index
	char *pathBuffer
return 
	Success : SQLITE_OK (0)
	Err		: SQLITE_ERROR (1) ~
********************************************************************************************/
int32_t Sql_GetFolderFullpath(sqlite3 *db, uint32_t folderIndex, char *pathBuffer, uint32_t bufSize, int32_t *isBufLack)
{
	int32_t rc = SQLITE_ERROR;
	if ((db != NULL) && (pathBuffer != NULL))
	{
		rc = Sql_Select_TextColumn_byIntegerType(db,"FullPath" ,FOLDER_TABLE , "FolderIndex", (int32_t)folderIndex,pathBuffer, bufSize, isBufLack);
	}
	return rc;
}

/********************************************************************************************
Function : 
description : 
arguments
	sqlite3 *db
	char *sqlQuery
	uint32_t index
return 
	Success :  1 ~
	Err		:  return < 1 
********************************************************************************************/
int32_t Sql_GetFolderParentIndex(sqlite3 *db,uint32_t folderIndex)
{
	int32_t rc =-1;
	if (db != NULL)
	{
		rc = Sql_Select_IntegerColumn_byIntegerType(db,"ParentIndex", FOLDER_TABLE, "FolderIndex",(int32_t)folderIndex);
	}
	return rc;
}

/********************************************************************************************
Function :
description :
arguments
	sqlite3 *db
	char *sqlQuery
	uint32_t index
return 
	Success :  1 ~
	Err		:  return < 1 
********************************************************************************************/
int32_t Sql_GetFolderInode(sqlite3 *db, uint32_t folderIndex)
{
	int32_t rc =-1;
	if (db != NULL) 
	{
		rc = Sql_Select_IntegerColumn_byIntegerType(db,"Inode", FOLDER_TABLE, "FolderIndex",(int32_t)folderIndex);
	}
	return rc;
}

/********************************************************************************************
Function : 
description : 
arguments
	sqlite3 *db
	char *sqlQuery
	uint32_t index
return 
	Success :  1 ~
	Err		:  return < 1 
********************************************************************************************/
int32_t Sql_GetSubFolderCount(sqlite3 *db, uint32_t folderIndex)
{
	int32_t rc =-1;
	if (db != NULL)
	{
		rc = sql_Select_RowCount_byIntegerType(db, FOLDER_TABLE, "ParentIndex", (int32_t)folderIndex);
	}
	return rc;
}

/********************************************************************************************
Function : 
description : 
arguments
	sqlite3 *db
	char *sqlQuery
	uint32_t index
return 
	Success :  1 ~
	Err		:  return < 1 
********************************************************************************************/
int32_t Sql_GetSubFileCount(sqlite3 *db,uint32_t folderIndex, ContentsType type)
{
	int32_t rc =-1;
	const char *tableName;

	if(type == AudioContents)
	{
		tableName = (const char *)&AUDIO_FILE_TABLE;
	}
	else if(type == VideoContents)
	{
		tableName = (const char *)&VIDEO_FILE_TABLE;
	}
	else if(type == ImageContents)
	{
		tableName = (const char *)&IMAGE_FILE_TABLE;
	}
	else
	{
		tableName = NULL;
	}
	
	if ((db != NULL) &&(tableName != NULL))
	{
		rc = sql_Select_RowCount_byIntegerType(db, tableName, "ParentIndex", (int32_t)folderIndex);
	}
	return rc;
}


/********************************************************************************************
Function : GetFolderRowCount
description : return Total Folder ccount from FolderList Table
arguments
	sqlite3 *db
	char *sqlQuery
return 
	Folder Index  : 0 ~
	Err		      : -1
********************************************************************************************/
int32_t Sql_GetFolderRowCount(sqlite3 *db)
{
	int32_t Count =-1;
	if(db != NULL)
	{
		Count = Sql_Select_TotalRowCount(db,FOLDER_TABLE);
	}
	return Count;
}

int32_t Sql_GetFolderIndex(sqlite3 *db, const char *name, uint32_t parentIndex)
{
	int32_t folderIndex = -1;
	if((db != NULL) && (name != NULL))
	{
		sqlite3_stmt *state;
		const char *tail;
		int32_t rc;
		int32_t columns;
		char sqlQuery[] = "select [FolderIndex] from "FOLDER_TABLE" where [Name] = :name and [ParentIndex]= :parent_index";
		rc = sqlite3_prepare_v2(db, sqlQuery, (int32_t)strlen(sqlQuery), &state, &tail);
		if (rc == SQLITE_OK)
		{
			(void)sqlite3_bind_text(state, sqlite3_bind_parameter_index(state, ":name"), name, (int32_t)strlen(name), SQLITE_STATIC);
			(void)sqlite3_bind_int(state, sqlite3_bind_parameter_index(state, ":parent_index"), (int32_t)parentIndex);

			rc = sqlite3_step(state);
			columns = sqlite3_column_count(state);
			if (columns == 1)
			{
				if (SQLITE_ROW == rc)
				{
					folderIndex = sqlite3_column_int(state, 0);
				}
			}

			(void)sqlite3_finalize(state);
		}
		else
		{
			ERROR_TCDB_PRINTF("sqlite3_preparev2 error: %d(%s)\n", rc, sqlite3_errmsg(db));
		}
	}

	return folderIndex;
}

/********************************************************************************************
Function : Sql_FindFolderName
description : select FolderIndex where FullPath..
arguments
	sqlite3 *db
	char *sqlQuery
	const char *fullPath
return 
	Find Folder  	 : 1 ~ 
	Not Find folder : 0
	Err		        : -1
********************************************************************************************/
int32_t Sql_FindFolderIndex(sqlite3 *db, const char *fullPath)
{
	int32_t ret;
	int32_t findIndex = 0;
	if((db != NULL) && (fullPath != NULL))
	{
		sqlite3_stmt *state;
		const char *tail;
		int32_t rc;
		int32_t columns,i;
		int32_t sameFolderIndex;
		const char *name;
		char *folderPath;
		char sqlQuery[] = "select [FolderIndex] from "FOLDER_TABLE" where [Name] = :name";
		int32_t isBufLack;
		rc = sqlite3_prepare_v2(db, sqlQuery, (int32_t)strlen(sqlQuery), &state, &tail);
		if (rc == SQLITE_OK)
		{
			folderPath = malloc(MAX_PATH_LENGTH);
			if(folderPath != NULL)
			{
				(void)memset(folderPath, 0x00, MAX_PATH_LENGTH);
				name = strrchr(fullPath, (int32_t)'/');
				if(name == NULL)
				{
					name = fullPath;
				}

				findIndex = -1;
				(void)sqlite3_bind_text(state, sqlite3_bind_parameter_index(state, ":name"), name, (int32_t)strlen(name), SQLITE_STATIC);
				rc = sqlite3_step(state);
				columns = sqlite3_column_count(state);
				while (SQLITE_ROW == rc)
				{
					for(i=0; i< columns; i++)
					{
						sameFolderIndex = sqlite3_column_int(state, i);
						ret = Sql_GetFolderFullpath(db, (uint32_t)sameFolderIndex, folderPath,MAX_PATH_LENGTH,&isBufLack);
						if(ret == SQLITE_OK)
						{
							int32_t cmpLength;

							cmpLength = (int32_t)strlen(fullPath);
							if (*(fullPath + cmpLength - 1) == '/')
							{
								//not compare last '/'
								cmpLength--;
							}
							
							if(strncmp(folderPath, fullPath,(uint32_t)cmpLength)==0)
							{
								findIndex = sameFolderIndex;
								rc = SQLITE_ROW;
								break;
							}
						}
					}
				}
			}
			else
			{
				findIndex = -1;
			}
			(void)sqlite3_finalize(state);

			if(folderPath != NULL)
			{
				free(folderPath);
			}
		}
		else
		{
			/* for QAC : Reule-2.2.*/
			(void)findIndex;
			ERROR_TCDB_PRINTF("sqlite3_preparev2 error: %d(%s)\n", rc, sqlite3_errmsg(db));
		}
	}
	return findIndex;
	
}



/********************************************************************************************
Function : InsertFileList
description : Insert File info to FileList table
arguments
	DeviceSource device		 	: Device Type
	const char *name				: File Name
	int32_t parentIndex				: Parent Folder Index
return 	
	Success : SQLITE_OK (0)
	Err		: SQLITE_ERROR (1) ~ 
********************************************************************************************/
int32_t Sql_InsertFileList(sqlite3 *db, const char *name, int32_t parentIndex, int32_t iNode, ContentsType type)
{
	int32_t rc = SQLITE_ERROR;

	if(type == AudioContents)
	{
		rc = Sql_InsertAudioFileList(db, name, parentIndex, iNode);
	}
	else if(type == VideoContents)
	{
		rc = Sql_InsertVideoFileList(db, name, parentIndex, iNode);
	}
	else if(type == ImageContents)
	{
		rc = Sql_InsertImageFileList(db, name, parentIndex, iNode);
	}
	else
	{
		ERROR_TCDB_PRINTF("Not Support File Type\n");
	}

	return rc;
}

static int32_t Sql_InsertAudioFileList(sqlite3 *db, const char *name, int32_t parentIndex, int32_t iNode)
{
	int32_t rc = SQLITE_ERROR;

	if ((db != NULL) && (name!=NULL)) 
	{
		sqlite3_stmt *state;
		const char *tail;
		char sqlBuffer[] = "insert into "AUDIO_FILE_TABLE" (Name, ParentIndex, Inode) values(:name,:parent_index,:inode)";
	
		rc = sqlite3_prepare_v2(db, sqlBuffer, (int32_t)strlen(sqlBuffer), &state, &tail);
		if (rc == SQLITE_OK)
		{
			(void)sqlite3_bind_text(state, sqlite3_bind_parameter_index(state,":name"), name, (int32_t)strlen(name), SQLITE_STATIC);
			(void)sqlite3_bind_int(state, sqlite3_bind_parameter_index(state,":parent_index"), parentIndex);
			(void)sqlite3_bind_int(state, sqlite3_bind_parameter_index(state,":inode"), iNode);

			rc = sqlite3_step(state);
			if ((rc == SQLITE_OK) || (rc == SQLITE_DONE))
			{
				rc = SQLITE_OK;
			}
			else
			{
				ERROR_TCDB_PRINTF("Set filelist error: %d(%s)\n", rc, sqlite3_errmsg(db)); 
			}
			(void)sqlite3_finalize(state);
		}
		else
		{
			ERROR_TCDB_PRINTF("sqlite3_preparev2 error: %d(%s)\n", rc, sqlite3_errmsg(db));
		}
	}

	return rc;
}


static int32_t Sql_InsertVideoFileList(sqlite3 *db, const char *name, int32_t parentIndex, int32_t iNode)
{
	int32_t rc = SQLITE_ERROR;

	if ((db != NULL) && (name!=NULL))
	{
		sqlite3_stmt *state;
		const char *tail;
		char sqlBuffer[] = "insert into "VIDEO_FILE_TABLE" (Name, ParentIndex, Inode) values(:name,:parent_index,:inode)";
	
		rc = sqlite3_prepare_v2(db, sqlBuffer, (int32_t)strlen(sqlBuffer), &state, &tail);
		if (rc == SQLITE_OK)
		{
			(void)sqlite3_bind_text(state, sqlite3_bind_parameter_index(state,":name"), name, (int32_t)strlen(name), SQLITE_STATIC);
			(void)sqlite3_bind_int(state, sqlite3_bind_parameter_index(state,":parent_index"), parentIndex);
			(void)sqlite3_bind_int(state, sqlite3_bind_parameter_index(state,":inode"), iNode);

			rc = sqlite3_step(state);
			if ((rc == SQLITE_OK) || (rc == SQLITE_DONE))
			{
				rc = SQLITE_OK;
			}
			else
			{
				ERROR_TCDB_PRINTF("Set filelist error: %d(%s)\n", rc, sqlite3_errmsg(db)); 
			}
			(void)sqlite3_finalize(state);
		}
		else
		{
			ERROR_TCDB_PRINTF("sqlite3_preparev2 error: %d(%s)\n", rc, sqlite3_errmsg(db));
		}
	}

	return rc;
}


static int32_t Sql_InsertImageFileList(sqlite3 *db, const char *name, int32_t parentIndex, int32_t iNode)
{
	int32_t rc = SQLITE_ERROR;

	if ((db != NULL) && (name!=NULL))
	{
		sqlite3_stmt *state;
		const char *tail;
		char sqlBuffer[] = "insert into "IMAGE_FILE_TABLE" (Name, ParentIndex, Inode) values(:name,:parent_index,:inode)";
	
		rc = sqlite3_prepare_v2(db, sqlBuffer, (int32_t)strlen(sqlBuffer), &state, &tail);
		if (rc == SQLITE_OK)
		{
			(void)sqlite3_bind_text(state, sqlite3_bind_parameter_index(state,":name"), name, (int32_t)strlen(name), SQLITE_STATIC);
			(void)sqlite3_bind_int(state, sqlite3_bind_parameter_index(state,":parent_index"), parentIndex);
			(void)sqlite3_bind_int(state, sqlite3_bind_parameter_index(state,":inode"), iNode);

			rc = sqlite3_step(state);
			if ((rc == SQLITE_OK) || (rc == SQLITE_DONE))
			{
				rc = SQLITE_OK;
			}
			else
			{
				ERROR_TCDB_PRINTF("Set filelist error: %d(%s)\n", rc, sqlite3_errmsg(db)); 
			}
			(void)sqlite3_finalize(state);
		}
		else
		{
			ERROR_TCDB_PRINTF("sqlite3_preparev2 error: %d(%s)\n", rc, sqlite3_errmsg(db));
		}
	}

	return rc;
}


/********************************************************************************************
Function : DeleteFileList
description : delete file from FolderList Table
arguments
return 
	Success : SQLITE_OK (0)
	Err		: SQLITE_ERROR (1) ~ 
********************************************************************************************/
int32_t Sql_DeleteFileList(sqlite3 *db, uint32_t fileIndex, ContentsType type)
{
	int32_t rc = SQLITE_ERROR;
	const char *tableName;

	if(type == AudioContents)
	{
		tableName = (const char *)&AUDIO_FILE_TABLE;
	}
	else if(type == VideoContents)
	{
		tableName = (const char *)&VIDEO_FILE_TABLE;
	}
	else if(type == ImageContents)
	{
		tableName = (const char *)&IMAGE_FILE_TABLE;
	}
	else
	{
		tableName = NULL;
	}
	
	if ((db!= NULL) &&  (tableName != NULL))
	{
		rc = Sql_Delete_Record(db,tableName, "FileIndex", (int32_t)fileIndex);
	}
	return rc;
}

/********************************************************************************************
Function : GetFileName
description : Get File Name from FileList Table
arguments
	sqlite3 *db
	char *sqlQuery
	uint32_t fileIndex
	char *buffer
return 
	Success : SQLITE_OK (0)
	Err		: SQLITE_ERROR (1) ~ 
********************************************************************************************/
int32_t Sql_GetFileName(sqlite3 *db, uint32_t fileIndex, char *buffer, uint32_t bufSize, int32_t *isBufLack , ContentsType type)
{
	int32_t rc = SQLITE_ERROR;
	const char *tableName=NULL;

	if(type == AudioContents)
	{
		tableName = (const char *)&AUDIO_FILE_TABLE;
	}
	else if(type == VideoContents)
	{
		tableName = (const char *)&VIDEO_FILE_TABLE;
	}
	else if(type == ImageContents)
	{
		tableName = (const char *)&IMAGE_FILE_TABLE;
	}
	else
	{
		(void)tableName;
	}
	
	if ((db != NULL) && (buffer != NULL) && (tableName != NULL))
	{
		rc = Sql_Select_TextColumn_byIntegerType(db,"Name" ,tableName, "FileIndex", (int32_t)fileIndex,buffer, bufSize, isBufLack);
	}
	return rc;
}

/********************************************************************************************
Function : GetFileParentIndex
description : return File Parent_Index from FileList Table
arguments
	sqlite3 *db
	char *sqlQuery
	uint32_t fileIndex
return 
	Length  : 0 ~ 
	Err		: -1
********************************************************************************************/
int32_t Sql_GetFileParentIndex(sqlite3 *db,uint32_t fileIndex, ContentsType type)
{
	int32_t parentIndex =-1;
	const char *tableName=NULL;

	if(type == AudioContents)
	{
		tableName = (const char *)&AUDIO_FILE_TABLE;
	}
	else if(type == VideoContents)
	{
		tableName = (const char *)&VIDEO_FILE_TABLE;
	}
	else if(type == ImageContents)
	{
		tableName = (const char *)&IMAGE_FILE_TABLE;
	}
	else
	{
		(void)tableName;
	}
	
	if ((db != NULL) && (tableName != NULL))
	{
		parentIndex = Sql_Select_IntegerColumn_byIntegerType(db,"ParentIndex", tableName, "FileIndex",(uint32_t )fileIndex);
	}	
	return parentIndex;
}

/********************************************************************************************
Function : Sql_GetFileInode
description : return File inode from FileList Table
arguments
	sqlite3 *db
	char *sqlQuery
	uint32_t fileIndex
return 
	Length  : 0 ~ 
	Err		: -1
********************************************************************************************/
int32_t Sql_GetFileInode(sqlite3 *db, uint32_t fileIndex, ContentsType type)
{
	int32_t iNode =-1;
	const char *tableName;

	if(type == AudioContents)
	{
		tableName = (const char *)&AUDIO_FILE_TABLE;
	}
	else if(type == VideoContents)
	{
		tableName = (const char *)&VIDEO_FILE_TABLE;
	}
	else if(type == ImageContents)
	{
		tableName = (const char *)&IMAGE_FILE_TABLE;
	}
	else
	{
		tableName = NULL;
	}
	
	if ((db != NULL) && (tableName != NULL))
	{
		iNode = Sql_Select_IntegerColumn_byIntegerType(db,"Inode", tableName, "FileIndex",(int32_t )fileIndex);
	}	
	return iNode;
}


/********************************************************************************************
Function : GetFileRowCount
description : return Total File count from FileList Table
arguments
	sqlite3 *db
	char *sqlQuery
return 
	Folder Index  : 0 ~ 
	Err		      : -11
********************************************************************************************/
int32_t Sql_GetFileRowCount(sqlite3 *db, ContentsType type)
{
	int32_t Count = -1;
	const char *tableName;

	if(type == AudioContents)
	{
		tableName = (const char *)&AUDIO_FILE_TABLE;
	}
	else if(type == VideoContents)
	{
		tableName = (const char *)&VIDEO_FILE_TABLE;
	}
	else if(type == ImageContents)
	{
		tableName = (const char *)&IMAGE_FILE_TABLE;
	}
	else
	{
		tableName = NULL;
	}
	
	if((db != NULL) && (tableName != NULL))
	{
		Count = Sql_Select_TotalRowCount(db ,tableName);
	}
	return Count;	
}

int32_t Sql_AddAllFolderList(sqlite3 *db,int32_t (*callback)(void* args, int32_t rowCount, int32_t folderIndex), void* pArg, const char * collationFn)
{
	int32_t rc = SQLITE_ERROR;
	
	if((db!=NULL)&&(callback != NULL )&&(pArg != NULL))
	{
		sqlite3_stmt *state;
		const char *tail;
		int32_t columns;
		int32_t rowCount=0;
		int32_t getIndex;
		int32_t callRet;
		char* sqlQuery;

		if(collationFn == NULL)
		{
			sqlQuery = (char *)newformat("select [FolderIndex] from "FOLDER_TABLE" order by Name ASC");
		}
		else
		{
			sqlQuery = (char *)newformat("select [FolderIndex] from "FOLDER_TABLE" order by Name collate %s",collationFn);
		}	

		if(sqlQuery != NULL)
		{
			rc = sqlite3_prepare_v2(db, sqlQuery, (int32_t)strlen(sqlQuery), &state, &tail);
			if (rc == SQLITE_OK)
			{
				columns = sqlite3_column_count(state);
				if (columns == 1)
				{
					rc = sqlite3_step(state);
					while(rc == SQLITE_ROW)
					{
						getIndex = sqlite3_column_int(state, 0);
						callRet = callback(pArg, rowCount, getIndex);
						rowCount++;
						if(callRet !=0)
						{
							break;
						}
						rc = sqlite3_step(state);
					}
					rc = SQLITE_OK;
				}
				else
				{
					ERROR_TCDB_PRINTF("column count not matched\n");
				}
				(void)sqlite3_finalize(state);
			}
			else
			{
				ERROR_TCDB_PRINTF("sqlite3_prepare_v2 error: %d(%s)\n", rc, sqlite3_errmsg(db));
			}

			free(sqlQuery);
			
		}
		else
		{
			ERROR_TCDB_PRINTF("Not enough memory\n");
		}
	}
	
	return rc;
}

int32_t Sql_AddSubFolderList(sqlite3 *db, uint32_t parentFolderIndex, int32_t (*callback)(void* args, int32_t rowCount, int32_t folderIndex), void* pArg, const char * collationFn)
{
	int32_t rc = SQLITE_ERROR;

	if((db!=NULL) && (callback != NULL) && (pArg != NULL))
	{
		sqlite3_stmt *state;
		const char *tail;
		int32_t columns;
		int32_t rowCount=0;
		int32_t getIndex;
		int32_t callRet;
		char *sqlQuery;

		if(collationFn == NULL)
		{
			sqlQuery = (char *)newformat("select [FolderIndex] from "FOLDER_TABLE" where ParentIndex = %d order by Name ASC", parentFolderIndex);
		}
		else
		{
			sqlQuery = (char *)newformat("select [FolderIndex] from "FOLDER_TABLE" where ParentIndex = %d order by Name collate %s", parentFolderIndex, collationFn);
		}

		if(sqlQuery != NULL)
		{
			rc = sqlite3_prepare_v2(db, sqlQuery, (int32_t)strlen(sqlQuery), &state, &tail);
			if (rc == SQLITE_OK)
			{
				columns = sqlite3_column_count(state);
				if (columns == 1)
				{
					rc = sqlite3_step(state);
					while( rc == SQLITE_ROW)
					{
						getIndex = sqlite3_column_int(state, 0);
						callRet = callback(pArg, rowCount, getIndex);
						rowCount++;
						if(callRet !=0)
						{
							break;
						}
						rc = sqlite3_step(state);
					}
					rc = SQLITE_OK;
				}
				else
				{
					ERROR_TCDB_PRINTF("column count not matched\n");
				}
				(void)sqlite3_finalize(state);
			}
			else
			{
				ERROR_TCDB_PRINTF("sqlite3_prepare_v2 error: %d(%s)\n", rc, sqlite3_errmsg(db));
			}
			free(sqlQuery);
		}
		else
		{
			ERROR_TCDB_PRINTF("Not enough memory\n");
		}
	}

	
	return rc;
}


int32_t Sql_AddAllFileList(sqlite3 *db,ContentsType type, int32_t (*callback)(void* args, int32_t rowCount, int32_t fileIndex), void* pArg, const char * collationFn)
{
	int32_t rc = SQLITE_ERROR;
	const char *tableName;

	if(type == AudioContents)
	{
		tableName = (const char *)&AUDIO_FILE_TABLE;
	}
	else if(type == VideoContents)
	{
		tableName = (const char *)&VIDEO_FILE_TABLE;
	}
	else if(type == ImageContents)
	{
		tableName = (const char *)&IMAGE_FILE_TABLE;
	}
	else
	{
		tableName = NULL;
	}

	if ((db!=NULL) && (tableName!=NULL) && (callback != NULL) && (pArg != NULL))
	{
		sqlite3_stmt *state;
		const char *tail;
		int32_t columns;
		int32_t rowCount=0;
		int32_t getIndex;
		int32_t callRet;
		char *sqlQuery;

		if(collationFn == NULL)
		{
			sqlQuery = (char *)newformat("select [FileIndex] from %s order by Name ASC", tableName);
		}
		else
		{
			sqlQuery = (char *)newformat("select [FileIndex] from %s order by Name collate %s", tableName, collationFn);
		}

		if(sqlQuery != NULL)
		{
			rc = sqlite3_prepare_v2(db, sqlQuery, (int32_t)strlen(sqlQuery), &state, &tail);
			if (rc == SQLITE_OK)
			{
				columns = sqlite3_column_count(state);
				if (columns == 1)
				{
					rc = sqlite3_step(state);
					while( rc == SQLITE_ROW)
					{
						getIndex = sqlite3_column_int(state, 0);
						callRet = callback(pArg, rowCount, getIndex);
						rowCount++;
						if(callRet !=0)
						{
							break;
						}
						rc = sqlite3_step(state);
					}
					rc = SQLITE_OK;
				}
				else
				{
					ERROR_TCDB_PRINTF("column count not matched\n");
				}
				(void)sqlite3_finalize(state);
			}
			else
			{
				ERROR_TCDB_PRINTF("sqlite3_prepare_v2 error: %d(%s)\n", rc, sqlite3_errmsg(db));
			}	
			free(sqlQuery);
		}
		else
		{
			ERROR_TCDB_PRINTF("Not enough memory\n");
		}
	}

	
	return rc;
}

int32_t Sql_AddSubFileList(sqlite3 *db, uint32_t parentFolderIndex,ContentsType type, int32_t (*callback)(void* args, int32_t rowCount, int32_t fileIndex), void* pArg, const char * collationFn)
{
	int32_t rc = SQLITE_ERROR;
	const char *tableName;

	if(type == AudioContents)
	{
		tableName = (const char *)&AUDIO_FILE_TABLE;
	}
	else if(type == VideoContents)
	{
		tableName = (const char *)&VIDEO_FILE_TABLE;
	}
	else if(type == ImageContents)
	{
		tableName = (const char *)&IMAGE_FILE_TABLE;
	}
	else
	{
		tableName = NULL;
	}

	if ((db!=NULL) && (tableName!=NULL))
	{
		sqlite3_stmt *state;
		const char *tail;
		int32_t columns;
		int32_t rowCount=0;
		int32_t getIndex;
		int32_t callRet;
		char *sqlQuery;

		if(collationFn == NULL)
		{
			sqlQuery = (char *)newformat("select [FileIndex] from %s where ParentIndex = %d order by Name ASC", tableName, parentFolderIndex);
		}
		else
		{
			sqlQuery = (char *)newformat("select [FileIndex] from %s where ParentIndex = %d order by Name collate %s", tableName, parentFolderIndex, collationFn);
		}

		if(sqlQuery != NULL)
		{
			rc = sqlite3_prepare_v2(db, sqlQuery, (int32_t)strlen(sqlQuery), &state, &tail);
			if (rc == SQLITE_OK)
			{
				columns = sqlite3_column_count(state);
				if (columns == 1)
				{
					rc = sqlite3_step(state);
					while( rc == SQLITE_ROW)
					{
						getIndex = sqlite3_column_int(state, 0);
						callRet = callback(pArg, rowCount, getIndex);
						rowCount++;
						if(callRet !=0)
						{
							break;
						}
						rc = sqlite3_step(state);
					}
					rc = SQLITE_OK;
				}
				else
				{
					ERROR_TCDB_PRINTF("column count not matched\n");
				}
				(void)sqlite3_finalize(state);
			}
			else
			{
				ERROR_TCDB_PRINTF("sqlite3_prepare_v2 error: %d(%s)\n", rc, sqlite3_errmsg(db));
			}	
			free(sqlQuery);
		}
		else
		{
			ERROR_TCDB_PRINTF("Not enough memory\n");
		}
	}

	
	return rc;
}

int32_t Sql_MakeSortedFolderList(sqlite3 * db)
{
	int32_t rc = SQLITE_ERROR;
	if(db != NULL) 
	{
		const char *tail;
		sqlite3_stmt *state;
		int32_t columns;
		int32_t columnIndex;
		int32_t folderIndex;
		int32_t totalCount;
		char sqlQuery[] = "select FolderIndex from "FOLDER_TABLE" order by Name asc";

		totalCount = Sql_GetFolderRowCount(db);
		
		rc = sqlite3_prepare_v2(db, sqlQuery, (int32_t)strlen(sqlQuery), &state, &tail);
		if (rc == SQLITE_OK)
		{
			columns = sqlite3_column_count(state);
			if (columns == 1)
			{
				rc = sqlite3_step(state);
				for (columnIndex = 0; columnIndex < totalCount && rc == SQLITE_ROW; columnIndex++)
				{
					folderIndex = sqlite3_column_int(state, 0);
					rc = Sql_InsertFolderSortTable(db, (uint32_t)folderIndex);
					if(rc == SQLITE_OK)
					{
						rc = sqlite3_step(state);
					}
				}
				rc = SQLITE_OK;
			}
			else
			{
				ERROR_TCDB_PRINTF("column count not matched\n");
			}
			(void)sqlite3_finalize(state);
		}
		else
		{
			ERROR_TCDB_PRINTF("sqlite3_prepare_v2 error: %d(%s)\n", rc, sqlite3_errmsg(db));
		}		
		
	}
	return rc;	
}

int32_t Sql_MakeSortedFileList(sqlite3 * db, ContentsType type)
{
	int32_t rc = SQLITE_ERROR;
	const char *SortTableName, * fileTableName;
	char *sqlQuery;

	if(type == AudioContents )
	{
		fileTableName = AUDIO_FILE_TABLE;
		SortTableName = AUDIO_SORT_TABLE;
	}
	else if(type == VideoContents)
	{
		fileTableName = VIDEO_FILE_TABLE;
		SortTableName = VIDEO_SORT_TABLE;
	}
	else if(type == ImageContents)
	{
		fileTableName = IMAGE_FILE_TABLE;
		SortTableName = IMAGE_SORT_TABLE;
	}
	else 
	{
		fileTableName = NULL;
		SortTableName = NULL;
	}

	/*
	DEBUG_TCDB_PRINTF("Make List(%s) : Target Table(%s) \n", SortTableName, fileTableName);
	*/
	
	if((db != NULL) && (fileTableName != NULL))
	{
		const char *tail;
		sqlite3_stmt *state;
		int32_t columns;
		int32_t columnIndex;
		int32_t folderIndex;
		int32_t totalCount;

		totalCount = Sql_GetFileRowCount(db, type);

		sqlQuery = (char*)newformat("select FileIndex from [%s] order by Name asc",fileTableName);
		if(sqlQuery != NULL)
		{
			rc = sqlite3_prepare_v2(db, sqlQuery, (int32_t)strlen(sqlQuery), &state, &tail);
			if (rc == SQLITE_OK)
			{
				columns = sqlite3_column_count(state);
				if (columns == 1)
				{
					rc = sqlite3_step(state);
					for (columnIndex = 0; columnIndex < totalCount && rc == SQLITE_ROW; columnIndex++)
					{
						folderIndex = sqlite3_column_int(state, 0);
						rc = Sql_InsertFileSortTable(db, (uint32_t)folderIndex, SortTableName);
						if(rc == SQLITE_OK)
						{
							rc = sqlite3_step(state);
						}
					}
					rc = SQLITE_OK;
				}
				else
				{
					ERROR_TCDB_PRINTF("column count not matched\n");
				}
				(void)sqlite3_finalize(state);
			}
			else
			{
				ERROR_TCDB_PRINTF("sqlite3_prepare_v2 error: %d(%s)\n", rc, sqlite3_errmsg(db));
			}
			free(sqlQuery);
		}	
		else
		{
			/* for QAC */
			(void)SortTableName;
			ERROR_TCDB_PRINTF("Not enough memory\n");
		}
	}
	else
	{
		/* for QAC : Reule-2.2.*/
		(void)fileTableName;		
		(void)SortTableName;	
		ERROR_TCDB_PRINTF("Bad argument\n");
	}
	return rc;	
}

static int32_t Sql_InsertFolderSortTable(sqlite3 *db, uint32_t folderIndex)
{
	int32_t rc = SQLITE_ERROR;
	if(db != NULL)
	{
		sqlite3_stmt *state;
		const char *tail;

		char sqlQuery[] = "insert into "FOLDER_SORT_TABLE" (FolderIndex) values(:folder_index)";
		rc = sqlite3_prepare_v2(db, sqlQuery, (int32_t)strlen(sqlQuery), &state, &tail);
		if (rc == SQLITE_OK)
		{
			(void)sqlite3_bind_int(state, sqlite3_bind_parameter_index(state,":folder_index"), (int32_t)folderIndex);

			rc = sqlite3_step(state);
			if ((rc == SQLITE_OK) || (rc == SQLITE_DONE))
			{
				rc = SQLITE_OK;
			}
			else
			{
				ERROR_TCDB_PRINTF("Insert Sorted Folder List error: %d(%s)\n", rc, sqlite3_errmsg(db));
			}
			(void)sqlite3_finalize(state);
		}
		else
		{
			ERROR_TCDB_PRINTF("sqlite3_preparev2 error: %d(%s)\n", rc, sqlite3_errmsg(db));
		}		
	}
	return rc;
}

/********************************************************************************************
Function : Sql_GetSortedFolderIndex
description : 
arguments
	sqlite3 *db
	char *sqlQuery
	uint32_t sortedIndex
return 	
	Success : SQLITE_OK (0)
	Err		: SQLITE_ERROR (1) ~ 
********************************************************************************************/
int32_t Sql_GetSortedFolderIndex(sqlite3 *db, uint32_t listIndex)
{
	int32_t rc =-1;
	if (db != NULL) 
	{
		rc = Sql_Select_IntegerColumn_byIntegerType(db,"FolderIndex", FOLDER_SORT_TABLE, "FolderListIndex",(int32_t)listIndex);
	}
	return rc;
}

static int32_t Sql_InsertFileSortTable(sqlite3 *db, uint32_t fileIndex, const char *tableName)
{
	int32_t rc = SQLITE_ERROR;

	if((db != NULL) && ( tableName != NULL))
	{
		sqlite3_stmt *state;
		const char *tail;
		char *sqlQuery;

		sqlQuery = (char *)newformat("insert into %s (FileIndex) values(:file_List)",tableName);
		if(sqlQuery != NULL)
		{
			rc = sqlite3_prepare_v2(db, sqlQuery, (int32_t)strlen(sqlQuery), &state, &tail);
			if (rc == SQLITE_OK)
			{
				(void)sqlite3_bind_int(state, sqlite3_bind_parameter_index(state,":file_List"), (int32_t)fileIndex);

				rc = sqlite3_step(state);
				if ((rc == SQLITE_OK) || (rc == SQLITE_DONE))
				{
					rc = SQLITE_OK;
				}
				else
				{
					ERROR_TCDB_PRINTF("Insert Sorted File List(%s) error: %d(%s)\n", tableName, rc, sqlite3_errmsg(db));
				}
				(void)sqlite3_finalize(state);
			}
			else
			{
				ERROR_TCDB_PRINTF("sqlite3_preparev2 error: %d(%s)\n", rc, sqlite3_errmsg(db));
			}

			free(sqlQuery);
		}
		else
		{
			ERROR_TCDB_PRINTF("malloc error");
		}
	}
	return rc;
}

/********************************************************************************************
Function : Sql_GetSortedFileIndex
description : 
arguments
	sqlite3 *db
	char *sqlQuery
	uint32_t sortedIndex
return 	
	Success : SQLITE_OK (0)
	Err		: SQLITE_ERROR (1) ~ 
********************************************************************************************/
int32_t Sql_GetSortedFileIndex(sqlite3 *db,uint32_t listIndex, ContentsType type)
{
	int32_t rc =-1;
	if (db != NULL) 
	{
		if(type == AudioContents)
		{
			rc = Sql_Select_IntegerColumn_byIntegerType(db,"FileIndex", AUDIO_SORT_TABLE, "FileListIndex",(int32_t)listIndex);
		}
		else if(type == VideoContents)
		{
			rc = Sql_Select_IntegerColumn_byIntegerType(db,"FileIndex", VIDEO_SORT_TABLE, "FileListIndex",(int32_t)listIndex);
		}
		else if(type == ImageContents)
		{
			rc = Sql_Select_IntegerColumn_byIntegerType(db,"FileIndex", IMAGE_SORT_TABLE, "FileListIndex",(int32_t)listIndex);
		}
		else
		{
			(void)rc;
		}
	}
	return rc;
}


static int32_t Sql_CreateDeviceTable(sqlite3 *db)
{
	int32_t rc = SQLITE_ERROR;
	char *errMsg = NULL;

	if(db != NULL)
	{
		char sqlQuery[]=	"create table if not exists "DEVICE_TABLE
			" (DeviceIndex integer primary key autoincrement, "
			"TotalFolderCount integer, "
			"TotalAudioFileCount integer, "
			"TotalVideoFileCount integer, "
			"TotalImageFileCount integer, "
			"Used integer,"
			"RootPath text check(typeof(RootPath)='text') not NULL)";

		rc = sqlite3_exec(db, sqlQuery, NULL, NULL, &errMsg);
		if(rc != SQLITE_OK)
		{
			ERROR_TCDB_PRINTF("Query : %s\n", sqlQuery);
			ERROR_TCDB_PRINTF("Create "DEVICE_TABLE" error : %s \n", errMsg);
			sqlite3_free(errMsg);
		}
	}

	return rc;	
}

static int32_t Sql_CreateFolderTable(sqlite3 *db)
{
	int32_t rc = SQLITE_ERROR;
	char *errMsg = NULL;

	if(db != NULL )
	{
		char sqlQuery[]=	"create table if not exists "FOLDER_TABLE
			" (FolderIndex integer primary key autoincrement, "
			"Name text check(typeof(Name)='text') not NULL, "
			"FullPath text, "
			"ParentIndex integer, "
			"Inode integer)";
		
		rc = sqlite3_exec(db, sqlQuery, NULL, NULL, &errMsg);
		if(rc != SQLITE_OK)
		{
			ERROR_TCDB_PRINTF("Query : %s\n", sqlQuery);
			ERROR_TCDB_PRINTF("Create "FOLDER_TABLE" error : %s \n", errMsg);
			sqlite3_free(errMsg);
		}
	}

	return rc;		
}

static int32_t Sql_CreateAudioFileTable(sqlite3 *db)
{
	int32_t rc = SQLITE_ERROR;
	char *errMsg = NULL;

	if(db != NULL )
	{
		char sqlQuery[] = "create table if not exists "AUDIO_FILE_TABLE
			" (FileIndex integer primary key autoincrement, "
			"Name text check(typeof(Name)='text') not NULL, "
			"ParentIndex integer, "
			"Inode integer, "
			"foreign key(ParentIndex) references FolderTable (FolderIndex))";
		rc = sqlite3_exec(db, sqlQuery, NULL, NULL, &errMsg);
		if(rc != SQLITE_OK)
		{
			ERROR_TCDB_PRINTF("Query : %s\n", sqlQuery);
			ERROR_TCDB_PRINTF("Create "AUDIO_FILE_TABLE" error : %s \n", errMsg);
			sqlite3_free(errMsg);
		}		
	}

	return rc;		
}

static int32_t Sql_CreateVideoFileTable(sqlite3 *db)
{
	int32_t rc = SQLITE_ERROR;
	char *errMsg = NULL;

	if(db != NULL )
	{
		char sqlQuery[] = "create table if not exists "VIDEO_FILE_TABLE
			" (FileIndex integer primary key autoincrement, "
			"Name text check(typeof(Name)='text') not NULL, "
			"ParentIndex integer, "
			"Inode integer, "
			"foreign key(ParentIndex) references FolderTable (FolderIndex))";

		rc = sqlite3_exec(db, sqlQuery, NULL, NULL, &errMsg);
		if(rc != SQLITE_OK)
		{
			ERROR_TCDB_PRINTF("Query : %s\n", sqlQuery);
			ERROR_TCDB_PRINTF("Create "VIDEO_FILE_TABLE" error : %s \n", errMsg);
			sqlite3_free(errMsg);
		}		
	}
	return rc;		
}

static int32_t Sql_CreateImageFileTable(sqlite3 *db)
{
	int32_t rc = SQLITE_ERROR;
	char *errMsg = NULL;

	if(db != NULL)
	{
		char sqlQuery[] = "create table if not exists "IMAGE_FILE_TABLE
			" (FileIndex integer primary key autoincrement, "
			"Name text check(typeof(Name)='text') not NULL, "
			"ParentIndex integer, "
			"Inode integer, "
			"foreign key(ParentIndex) references FolderTable (FolderIndex))";

		rc = sqlite3_exec(db, sqlQuery, NULL, NULL, &errMsg);
		if(rc != SQLITE_OK)
		{
			ERROR_TCDB_PRINTF("Query : %s\n", sqlQuery);
			ERROR_TCDB_PRINTF("Create "IMAGE_FILE_TABLE" error : %s \n", errMsg);
			sqlite3_free(errMsg);
		}		
	}

	return rc;		
}

static int32_t Sql_CreateFolderSortTable(sqlite3 *db)
{
	int32_t rc = SQLITE_ERROR;
	char *errMsg = NULL;

	if(db != NULL)
	{
		char sqlQuery[] = "create table if not exists "FOLDER_SORT_TABLE
			" (FolderListIndex integer primary key autoincrement, "
			"FolderIndex integer, "
			"foreign key(FolderIndex) references FolderTable (FolderIndex))";

		rc = sqlite3_exec(db, sqlQuery, NULL, NULL, &errMsg);
		if(rc != SQLITE_OK)
		{
			ERROR_TCDB_PRINTF("Query : %s\n", sqlQuery);
			ERROR_TCDB_PRINTF("Create "FOLDER_SORT_TABLE" error : %s \n", errMsg);
			sqlite3_free(errMsg);
		}		
	}

	return rc;		
}

static int32_t Sql_CreateAudioSortTable(sqlite3 *db)
{
	int32_t rc = SQLITE_ERROR;
	char *errMsg = NULL;

	if(db != NULL)
	{
		char sqlQuery[] =	"create table if not exists "AUDIO_SORT_TABLE
			" (FileListIndex integer primary key autoincrement, "
			"FileIndex integer, "
			"foreign key(FileIndex) references AudioFileTable (FileIndex))";

		rc = sqlite3_exec(db, sqlQuery, NULL, NULL, &errMsg);
		if(rc != SQLITE_OK)
		{
			ERROR_TCDB_PRINTF("Query : %s\n", sqlQuery);
			ERROR_TCDB_PRINTF("Create "AUDIO_SORT_TABLE" error : %s \n", errMsg);
			sqlite3_free(errMsg);
		}				
	}

	return rc;		
}

static int32_t Sql_CreateVideoSortTable(sqlite3 *db)
{
	int32_t rc = SQLITE_ERROR;
	char *errMsg = NULL;

	if(db != NULL )
	{
		char sqlQuery[] = "create table if not exists "VIDEO_SORT_TABLE
			" (FileListIndex integer primary key autoincrement, "
			"FileIndex integer, "
			"foreign key(FileIndex) references VideoFileTable (FileIndex))";
		
		rc = sqlite3_exec(db, sqlQuery, NULL, NULL, &errMsg);
		if(rc != SQLITE_OK)
		{
			ERROR_TCDB_PRINTF("Query : %s\n", sqlQuery);
			ERROR_TCDB_PRINTF("Create "VIDEO_SORT_TABLE" error : %s \n", errMsg);
			sqlite3_free(errMsg);
		}		
	}
	return rc;		
}

static int32_t Sql_CreateImageSortTable(sqlite3 *db)
{
	int32_t rc = SQLITE_ERROR;
	char *errMsg = NULL;
	if(db != NULL)
	{
		char sqlQuery[] = "create table if not exists "IMAGE_SORT_TABLE
			" (FileListIndex integer primary key autoincrement, "
			"FileIndex integer, "
			"foreign key(FileIndex) references ImageFileTable (FileIndex))";

		rc = sqlite3_exec(db, sqlQuery, NULL, NULL, &errMsg);
		if(rc != SQLITE_OK)
		{
			ERROR_TCDB_PRINTF("Query : %s\n", sqlQuery);
			ERROR_TCDB_PRINTF("Create "IMAGE_SORT_TABLE" error : %s \n", errMsg);
			sqlite3_free(errMsg);
		}		
	}

	return rc;		
}

static int32_t Sql_DropTable(sqlite3 *db, const char *tableName)
{
	char *errStr = NULL;
	int32_t rc;
	char sqlQuery[64];

	/* init folerlist tble */
	(void)sprintf(sqlQuery, "drop table IF EXISTS [%s]", tableName);
	rc = sqlite3_exec(db, sqlQuery, NULL , NULL, &errStr);
	if (rc != SQLITE_OK)
	{
		ERROR_TCDB_PRINTF("Drop failed (%s)\n", sqlQuery);
		ERROR_TCDB_PRINTF("drop %s table failed.(%s)\n", tableName, errStr);
		sqlite3_free(errStr);
	}
	return rc;	
}

#ifdef META_DB_INCLUDE
int32_t Sql_CreateTagTable(sqlite3 *db)
{
	int32_t rc = SQLITE_ERROR;
	char *errMsg = NULL;

	if(db != NULL )
	{
		char sqlQuery[] = "create table if not exists "TAG_TABLE
			" (TagIndex integer primary key autoincrement, "
			"Title text,"
			"Artist text,"
			"Album text,"
			"Genre text,"
			"FileIndex,"
			"foreign key(FileIndex) references AudioFileTable (FileIndex))";
		rc = sqlite3_exec(db, sqlQuery, NULL, NULL, &errMsg);
		if(rc != SQLITE_OK)
		{
			ERROR_TCDB_PRINTF("Query : %s\n", sqlQuery);
			ERROR_TCDB_PRINTF("Create "TAG_TABLE" error : %s \n", errMsg);
			sqlite3_free(errMsg);
		}		
	}
	return rc;		
}

int32_t Sql_Insert_Tag_Info(sqlite3 *db, int32_t fileIndex, const char *title, const char * artist, const char * album, const char *genre)
{
	sqlite3_stmt *state;
	const char *tail;
	int32_t rc = SQLITE_ERROR;

	if((db != NULL)&&(fileIndex > -1))
	{
		char sqlQuery[] = "Insert into "TAG_TABLE" (Title, Artist, Album, Genre,FileIndex) values(:title,:artist,:album,:genre,:fileindex)"; 
		rc = sqlite3_prepare_v2(db, sqlQuery, (int32_t)strlen(sqlQuery), &state, &tail);
		if (rc == SQLITE_OK)
		{
			(void)sqlite3_bind_text(state, sqlite3_bind_parameter_index(state,":title"), title, (int32_t)strlen(title), SQLITE_STATIC);
			(void)sqlite3_bind_text(state, sqlite3_bind_parameter_index(state,":artist"), artist, (int32_t)strlen(artist), SQLITE_STATIC);
			(void)sqlite3_bind_text(state, sqlite3_bind_parameter_index(state,":album"), album, (int32_t)strlen(album), SQLITE_STATIC);
			(void)sqlite3_bind_text(state, sqlite3_bind_parameter_index(state,":genre"), genre, (int32_t)strlen(genre), SQLITE_STATIC);
			(void)sqlite3_bind_int(state, sqlite3_bind_parameter_index(state,":fileindex"),fileIndex);
			rc = sqlite3_step(state);
			if ((rc == SQLITE_OK) || (rc == SQLITE_DONE))
			{
				rc = SQLITE_OK;
			}
			else
			{
				ERROR_TCDB_PRINTF("Insert Meta "AUDIO_FILE_TABLE"  error: %d(%s)\n", rc, sqlite3_errmsg(db));
			}
			(void)sqlite3_finalize(state);
		}
		else
		{
			ERROR_TCDB_PRINTF("sqlite3_preparev2 error: %d(%s)\n", rc, sqlite3_errmsg(db));
		}		
	}
	else
	{
		ERROR_TCDB_PRINTF("%s: Parameter Error\n", __FUNCTION__);
	}	

	return rc;	
}
int32_t Sql_Insert_Tag_Info_UTF16(sqlite3 *db, int32_t fileIndex,
	const char *title, uint32_t titleLength,
	const char * artist, uint32_t artistLength,
	const char * album, uint32_t albumLength,
	const char *genre, uint32_t genreLength)
{
	sqlite3_stmt *state;
	const char *tail;
	int32_t rc = SQLITE_ERROR;

	if((db != NULL)&&(fileIndex > -1))
	{
		char sqlQuery[] = "Insert into "TAG_TABLE" (Title, Artist, Album, Genre,FileIndex) value(:title,:artist,:album,:genre,:fileindex)"; 
		rc = sqlite3_prepare_v2(db, sqlQuery, (int32_t)strlen(sqlQuery), &state, &tail);
		if (rc == SQLITE_OK)
		{
			(void)sqlite3_bind_int(state, sqlite3_bind_parameter_index(state,":fileindex"),fileIndex);
			(void)sqlite3_bind_text(state, sqlite3_bind_parameter_index(state,":title"), title, (int32_t)titleLength, SQLITE_STATIC);
			(void)sqlite3_bind_text(state, sqlite3_bind_parameter_index(state,":artist"), artist, (int32_t)artistLength, SQLITE_STATIC);
			(void)sqlite3_bind_text(state, sqlite3_bind_parameter_index(state,":album"), album, (int32_t)albumLength, SQLITE_STATIC);
			(void)sqlite3_bind_text(state, sqlite3_bind_parameter_index(state,":genre"), genre, (int32_t)genreLength, SQLITE_STATIC);
			rc = sqlite3_step(state);
			if ((rc == SQLITE_OK) || (rc == SQLITE_DONE))
			{
				rc = SQLITE_OK;
			}
			else
			{
				ERROR_TCDB_PRINTF("Insert Meta "AUDIO_FILE_TABLE"  error: %d(%s)\n", rc, sqlite3_errmsg(db));
			}
			(void)sqlite3_finalize(state);
		}
		else
		{
			ERROR_TCDB_PRINTF("sqlite3_preparev2 error: %d(%s)\n", rc, sqlite3_errmsg(db));
		}		
	}
	else
	{
		ERROR_TCDB_PRINTF("%s: Parameter Error\n", __FUNCTION__);
	}	

	return rc;	
}


int32_t Sql_GetTagName(sqlite3 *db, uint32_t fileIndex, char *name, uint32_t bufSize, int32_t *isBufLack,TagCategory type)
{
	int32_t rc  = SQLITE_ERROR;

	if ((db != NULL) && (name !=NULL))
	{
		if(type == Tag_Title)
		{
			rc = Sql_Select_TextColumn_byIntegerType(db,"Title" ,TAG_TABLE, "FileIndex", (int32_t)fileIndex, name, bufSize, isBufLack);
		}
		else if(type == Tag_Album)
		{
			rc = Sql_Select_TextColumn_byIntegerType(db,"Album" ,TAG_TABLE , "FileIndex", (int32_t)fileIndex,name,bufSize, isBufLack);
		}
		else if(type == Tag_Artist)
		{
			rc = Sql_Select_TextColumn_byIntegerType(db,"Artist" ,TAG_TABLE , "FileIndex", (int32_t)fileIndex,name,bufSize, isBufLack);
		}
		else if(type == Tag_Genre)
		{
			rc = Sql_Select_TextColumn_byIntegerType(db,"Genre" ,TAG_TABLE , "FileIndex", (int32_t)fileIndex,name,bufSize, isBufLack);
		}
		else
		{
			(void)rc;
		}
	}	
	return rc; 
}

int32_t Sql_GetTagNameAll(sqlite3 *db, uint32_t fileIndex, char *title,  char * artist,  char * album, char *genre, uint32_t bufSize)
{
	int32_t ret  = SQLITE_ERROR;

	if((db != NULL) && (title != NULL) && (artist != NULL) && (album != NULL) && (genre != NULL))
	{
		sqlite3_stmt *state;
		const char *tail;
		int32_t rc;
		int32_t columns, i;
		char sqlQuery[] = "select Title, Artist, Album, Genre from "TAG_TABLE" where [FileIndex] = :fileindex"; 
		rc = sqlite3_prepare_v2(db,sqlQuery, (int32_t)strlen(sqlQuery), &state, &tail);
		if (rc == SQLITE_OK)
		{
			(void)sqlite3_bind_int(state, sqlite3_bind_parameter_index(state, ":fileindex"), (int32_t)fileIndex);

			rc = sqlite3_step(state);
			if(rc == SQLITE_ROW)
			{
				columns = sqlite3_column_count(state);				
				if(columns == 4)
				{
					for(i=0; i<columns; i++)
					{
						const char *column_name;
						const char *column_value;
						int32_t value_size;
						column_name = sqlite3_column_name(state, i);
						column_value = (const char *)sqlite3_column_text(state, i);
						value_size = (int32_t)strlen(column_value);
						if((uint32_t)value_size >= bufSize)
						{
							value_size = (int32_t)bufSize-1;
						}
						
						if(strncmp(column_name, "title", 5)==0)
						{
							(void)memset(title, 0x00, bufSize);
							(void)strncpy(title, column_value,(size_t)value_size);
						}
						else if(strncmp(column_name, "artist", 6)==0)
						{
							(void)memset(artist, 0x00, bufSize);
							(void)strncpy(artist, column_value, (size_t)value_size);
						}
						else if(strncmp(column_name, "album", 5)==0)
						{
							(void)memset(album, 0x00, bufSize);
							(void)strncpy(album, column_value, (size_t)value_size);
						}
						else if(strncmp(column_name, "genre", 5)==0)
						{
							(void)memset(genre, 0x00, bufSize);
							(void)strncpy(genre, column_value,(size_t)value_size);
						}
						else
						{
							(void)ret;
						}
					}
				}				
			}
			else
			{
				;
			}
			(void)sqlite3_finalize(state);
			ret = SQLITE_OK;
		}
		else
		{
			ERROR_TCDB_PRINTF("sqlite3_preparev2 error: %d(%s)\n", rc, sqlite3_errmsg(db));
		}		
	}
	else
	{
		ERROR_TCDB_PRINTF("%s: Parameter Error\n", __FUNCTION__);
	}
	
	return ret;
}

int32_t Sql_InsertExtendTag_TextType(sqlite3 *db, int32_t fileIndex,const char *TagName, char *value)
{
	int32_t rc = SQLITE_ERROR;
	if (db != NULL)
	{
		rc = Sql_Update_TextColumn(db, TAG_TABLE, TagName, value, "FileIndex", fileIndex);
	}
	return rc;
}

int32_t Sql_InsertExtendTag_IntegerType(sqlite3 *db, int32_t fileIndex,const char *TagName, int32_t value)
{
	int32_t rc = SQLITE_ERROR;
	if (db != NULL)
	{
		rc = Sql_Update_IntegerColumn(db, TAG_TABLE, TagName, value, "FileIndex", fileIndex);
	}
	return rc;
}

#endif	/* META_DB_INCLUDE */

/* SQLite common wrapping functions*/
/*
char * GetBuf : read buffer pointer
uint32_t bufSize : Size of GetBuf
int32_t getState : [out]
	0 - read OK
	1 - read OK buf not enought memory
*/

int32_t Sql_Select_TextColumn_byIntegerType
	(sqlite3 *db, const char *SelectColumn, const char *FromTable, const char *WhereColumn, int32_t termsValue, char *GetBuf, uint32_t bufSize, int32_t *isBufLack)
{
	int32_t rc = SQLITE_ERROR;

	if ( (db!=NULL) && (SelectColumn!=NULL) && (FromTable!=NULL) && (WhereColumn!=NULL) && (GetBuf!=NULL)&&((int32_t)bufSize > 0))
	{
		const char *getColumn;
		sqlite3_stmt *state;
		const char *tail;
		int32_t strLength;
		char *sqlQuery;

		sqlQuery = (char*)newformat("select %s from %s where %s = %d",SelectColumn, FromTable, WhereColumn,termsValue);
		if(sqlQuery != NULL)
		{
			rc = sqlite3_prepare_v2(db, sqlQuery, (int32_t)strlen(sqlQuery), &state, &tail);
			if (rc == SQLITE_OK)
			{
				rc = sqlite3_step(state);
				if (rc == SQLITE_ROW)
				{
					getColumn = (const char *)sqlite3_column_text(state, 0);
					strLength = (int32_t)strlen(getColumn);
					if((uint32_t)strLength >= bufSize)
					{
						strLength = (int32_t)bufSize-1;
						*isBufLack = 1;
					}
					else
					{
						*isBufLack = 0;
					}
					(void)memset(GetBuf, 0x00, bufSize);
					(void)strncpy(GetBuf, getColumn, (size_t)strLength);

					rc = SQLITE_OK;
				}
				(void)sqlite3_finalize(state);
			}
			else
			{
				ERROR_TCDB_PRINTF("sqlite3_preparev2 error: %d(%s)\n", rc, sqlite3_errmsg(db));
			}
			free(sqlQuery);
		}
		else
		{
			ERROR_TCDB_PRINTF("Not enough memory\n");
		}
	}
	if(rc==SQLITE_DONE)
	{
		rc = SQLITE_OK;
	}

	return rc;
}

static int32_t Sql_Select_IntegerColumn_byIntegerType
	(sqlite3 *db, const char *SelectColumn, const char *FromTable, const char *WhereColumn, int32_t termsValue)
{
	int32_t rc;
	int32_t GetValue =-1;

	if ( (db!=NULL) && (SelectColumn!=NULL) && (FromTable!=NULL) && (WhereColumn!=NULL))
	{
		sqlite3_stmt *state;
		const char *tail;
		int32_t columns;
		char *sqlQuery;

		sqlQuery = (char *)newformat("select %s from %s where %s = %d",SelectColumn, FromTable, WhereColumn,termsValue);
		if(sqlQuery != NULL)
		{
			rc = sqlite3_prepare_v2(db, sqlQuery, (int32_t)strlen(sqlQuery), &state, &tail);
			if (rc == SQLITE_OK)
			{
				columns = sqlite3_column_count(state);
				rc = sqlite3_step(state);
				if ((rc == SQLITE_ROW) && (columns == 1))
				{
					GetValue = sqlite3_column_int(state, 0);
				}
				(void)sqlite3_finalize(state);
			}
			else
			{
				ERROR_TCDB_PRINTF("sqlite3_preparev2 error: %d(%s)\n", rc, sqlite3_errmsg(db));
			}
			free(sqlQuery);
		}
		else
		{
			ERROR_TCDB_PRINTF("Not enough memory\n");
		}
	}
	return GetValue;
}

static int32_t Sql_Select_IntegerColumn_byTextType
	(sqlite3 *db, const char *SelectColumn, const char *FromTable, const char *WhereColumn, int32_t  termsValue)
{
	int32_t rc = SQLITE_ERROR;

	return rc;
}

static int32_t sql_Select_RowCount_byIntegerType
	(sqlite3 *db, const char *FromTable, const char *WhereColumn, int32_t termsValue)
{
	int32_t rc;
	int32_t GetValue =-1;

	if ( (db!=NULL) && (FromTable!=NULL) && (WhereColumn!=NULL))
	{
		sqlite3_stmt *state;
		const char *tail;
		int32_t columns;
		char *sqlQuery;

		sqlQuery = (char *)newformat("select count(*) from %s where %s = %d", FromTable, WhereColumn,termsValue);
		if(sqlQuery != NULL)
		{
			rc = sqlite3_prepare_v2(db, sqlQuery, (int32_t)strlen(sqlQuery), &state, &tail);
			if (rc == SQLITE_OK)
			{
				columns = sqlite3_column_count(state);
				rc = sqlite3_step(state);
				if ((rc == SQLITE_ROW) && (columns == 1))
				{
					GetValue = sqlite3_column_int(state, 0);
				}
				(void)sqlite3_finalize(state);
			}
			else
			{
				ERROR_TCDB_PRINTF("sqlite3_preparev2 error: %d(%s)\n", rc, sqlite3_errmsg(db));
			}
			free(sqlQuery);
		}
		else
		{
			ERROR_TCDB_PRINTF("Not enough memory\n");
		}
	}
	
	return GetValue;	
}

static int32_t Sql_Select_TotalRowCount(sqlite3 *db, const char *TableName)
{
	char *errMsg;
	int32_t rc;
	char **cResult;
	int32_t rows, columns;
	int32_t count = -1;
	char *sqlQuery;

	if ( (db!=NULL) && (TableName!=NULL) )
	{
		sqlQuery = (char *)newformat("select * from [%s]", TableName);
		if(sqlQuery != NULL)
		{
			rc = sqlite3_get_table(db, sqlQuery, &cResult, &rows, &columns, &errMsg);
			if (rc == SQLITE_OK)
			{
				count = rows;
				sqlite3_free_table(cResult);
			}
			else
			{
				(void)fprintf(stderr, "Get Tatal RowCount err : %s \n", errMsg);
			}
			free(sqlQuery);
		}
		else
		{
			ERROR_TCDB_PRINTF("Not enough memory\n");
		}
	}

	return count;
}

static int32_t Sql_Update_IntegerColumn
	(sqlite3 *db, const char *UpdatTable, const char *UpdateColumn, int32_t SetValue, const char *TermsColumn,int32_t TermsValue)
{
	int32_t rc = SQLITE_ERROR;
	sqlite3_stmt *state;
	const char *tail;
	
	if ( (db!=NULL) && (UpdatTable!=NULL) && (UpdateColumn!=NULL) && (TermsColumn!=NULL))
	{
		char *sqlQuery;
		sqlQuery = (char *)newformat("update [%s] set [%s] = :value where [%s] = :terms_value", UpdatTable, UpdateColumn, TermsColumn);
		if(sqlQuery != NULL)
		{
			rc = sqlite3_prepare_v2(db, sqlQuery, (int32_t)strlen(sqlQuery), &state, &tail);
			if (rc == SQLITE_OK)
			{
				(void)sqlite3_bind_int(state, sqlite3_bind_parameter_index(state, ":value"), SetValue);
				(void)sqlite3_bind_int(state, sqlite3_bind_parameter_index(state, ":terms_value"), TermsValue);

				rc = sqlite3_step(state);
				if ((rc == SQLITE_OK)||(rc == SQLITE_DONE))
				{
					rc = SQLITE_OK;
				}
				else
				{
					ERROR_TCDB_PRINTF("Update Table(Integer) error: %d(%s)\n", rc, sqlite3_errmsg(db));
				}
				(void)sqlite3_finalize(state);
			}
			else
			{
				ERROR_TCDB_PRINTF("sqlite3_preparev2 error: %d(%s)\n", rc, sqlite3_errmsg(db));
			}
			free(sqlQuery);
		}
		else
		{
			ERROR_TCDB_PRINTF("Not enough memory\n");
		}
	}
	return rc;
}

static int32_t Sql_Update_TextColumn
	(sqlite3 *db, const char *UpdatTable, const char *UpdateColumn, const char *SetValue, const char *TermsColumn,int32_t TermsValue)
{
	int32_t rc = SQLITE_ERROR;
	sqlite3_stmt *state;
	const char *tail;
	
	if ( (db!=NULL) && (UpdatTable!=NULL) && (UpdateColumn!=NULL) && (SetValue != NULL) && (TermsColumn!=NULL))
	{
		char *sqlQuery;

		sqlQuery = (char *)newformat("update [%s] set [%s] = :value where [%s] = :terms_value", UpdatTable, UpdateColumn, TermsColumn);
		if(sqlQuery != NULL)
		{
			rc = sqlite3_prepare_v2(db, sqlQuery, (int32_t)strlen(sqlQuery), &state, &tail);
			if (rc == SQLITE_OK)
			{
				(void)sqlite3_bind_text(state, sqlite3_bind_parameter_index(state,":value"), SetValue, (int32_t)strlen(SetValue), SQLITE_STATIC);
				(void)sqlite3_bind_int(state, sqlite3_bind_parameter_index(state, ":terms_value"), TermsValue);

				rc = sqlite3_step(state);
				if ((rc == SQLITE_OK)||(rc == SQLITE_DONE))
				{
					rc = SQLITE_OK;
				}
				else
				{
					ERROR_TCDB_PRINTF("Update Table(Integer) error: %d(%s)\n", rc, sqlite3_errmsg(db));
				}
				(void)sqlite3_finalize(state);
			}
			else
			{
				ERROR_TCDB_PRINTF("sqlite3_preparev2 error: %d(%s)\n", rc, sqlite3_errmsg(db));
			}
			free(sqlQuery);
		}
		else
		{
			ERROR_TCDB_PRINTF("Not enough memory\n");
		}
	}
	return rc;
}

static int32_t Sql_Delete_Record(sqlite3 *db, const char *TargetTable, const char *TermsColumn, int32_t TermsValue)
{
	int32_t rc = SQLITE_ERROR;
	sqlite3_stmt *state;
	const char *tail;
	char *sqlQuery;

	if((db!=NULL) && (TargetTable!=NULL) && (TermsColumn!=NULL) )
	{
		sqlQuery = (char *)newformat("delete from [%s] where [%s] = :target_index",TargetTable,TermsColumn);
		if(sqlQuery != NULL)
		{
			rc = sqlite3_prepare_v2(db, sqlQuery, (int32_t)strlen(sqlQuery), &state, &tail);
			if (rc == SQLITE_OK)
			{
				(void)sqlite3_bind_int(state, sqlite3_bind_parameter_index(state, ":target_index"), TermsValue);
				rc = sqlite3_step(state);

				if ((rc == SQLITE_OK) || (rc == SQLITE_DONE))
				{
					rc = SQLITE_OK;
				}
				else
				{
					ERROR_TCDB_PRINTF("delete Record error: %d(%s)\n", rc,sqlite3_errmsg(db));
				}

				(void)sqlite3_finalize(state);
			}
			else
			{
				ERROR_TCDB_PRINTF("sqlite3_preparev2(%s) error: %d(%s)\n", sqlQuery, rc, sqlite3_errmsg(db));
			}
			free(sqlQuery);
		}
	}

	return rc;
}

#ifdef __cplusplus
}
#endif
