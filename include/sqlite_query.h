/****************************************************************************************
 *   FileName    : sqltie_query.h
 *   Description : header file of APIs of sqlite query
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

#ifndef SQLITE_QUERY_H
#define SQLITE_QUERY_H

#ifdef __cplusplus
extern "C" {
#endif


#define SQL_QUERY_STRING_SIZE	512

#define SQL_TAG_QUERY_STRING_SIZE		8192	//4096

#define DEVICE_TABLE				"DeviceTable"
#define FOLDER_TABLE				"FolderTable"
#define AUDIO_FILE_TABLE			"AudioFileTable"
#define VIDEO_FILE_TABLE			"VideoFileTable"
#define IMAGE_FILE_TABLE			"ImageFileTable"
#define FOLDER_SORT_TABLE			"FolderSortTable"
#define AUDIO_SORT_TABLE			"AudioSortTable"
#define VIDEO_SORT_TABLE			"VideoSortTable"
#define IMAGE_SORT_TABLE			"ImageSortTable"
#define TAG_TABLE					"TagTable"

typedef struct  deviceTable_Info
{
	uint32_t _folderCount;
	uint32_t _audioFileCount;
	uint32_t _videoFileCount;
	uint32_t _imageFileCount;
	uint32_t _used;
	char * _rootPath;
}DeviceTable_Info;

typedef struct folderTable_Info
{
	char *_name;
	char *_fullPath;
	uint32_t _parentIndex;
	uint32_t _inode;
	uint32_t _folderCount;
	uint32_t _audioFileCount;
	uint32_t _videoFileCount;
	uint32_t _imageFileCount;
	uint32_t _subFolderIndex;
	uint32_t _nextFolderIndex;
	uint32_t _firstAudioIndex;
	uint32_t _firstVideoIndex;
	uint32_t _firstImageIndex;
}FolderTable_Info;

sqlite3 * Sql_Open_DB(const char *dbpath);
int32_t Sql_Close_DB(sqlite3 *db);
int32_t Sql_CreateCollation( sqlite3* db,  const char *zName,  int32_t eTextRep,  void *pArg,  int32_t(*xCompare)(void *data, int32_t len1, const void* in1, int32_t len2, const void* in2));
int32_t Sql_SetPragma(sqlite3* db);
int32_t Sql_Transaction_Start(sqlite3 *db);
int32_t Sql_Transaction_End(sqlite3 *db);
int32_t Sql_DropTableAll(sqlite3 *db);
int32_t Sql_InitTables(sqlite3 *db);
/* DeviceTable APIs */
int32_t Sql_InsertDeviceInfo(sqlite3 *db, DeviceTable_Info* deviceInfo);
int32_t Sql_DeleteDeviceInfo(sqlite3 *db);
int32_t Sql_GetRootPath(sqlite3 *db, char *Path, uint32_t bufSize, int32_t *isBufLack);
int32_t Sql_SetUsedFlag(sqlite3 *db, int32_t used);
int32_t Sql_GetUsedFlag(sqlite3 *db);
int32_t Sql_SetTotalFolderCount(sqlite3 *db, uint32_t count);
int32_t Sql_GetTotalFolderCount(sqlite3 *db);
int32_t Sql_SetTotalAudioFileCount(sqlite3 *db, uint32_t count);
int32_t Sql_GetTotalAudioFileCount(sqlite3 *db);
int32_t Sql_SetTotalVideoFileCount(sqlite3 *db, uint32_t count);
int32_t Sql_GetTotalVideoFileCount(sqlite3 *db);
int32_t Sql_SetTotalImageFileCount(sqlite3 *db, uint32_t count);
int32_t Sql_GetTotalImageFileCount(sqlite3 *db);

/* FolderTable APIs */
int32_t Sql_InsertFolderList(sqlite3 *db,FolderTable_Info *pfolderInfo);
int32_t Sql_DeleteFolderList(sqlite3 *db, uint32_t folderIndex);
int32_t Sql_GetFolderName(sqlite3 *db,uint32_t folderIndex, char *nameBuffer, uint32_t bufSize, int32_t *isBufLack);
int32_t Sql_SetFolderFullpath(sqlite3 *db, uint32_t folderIndex, char *pathBuffer);
int32_t Sql_GetFolderFullpath(sqlite3 *db, uint32_t folderIndex, char *pathBuffer, uint32_t bufSize, int32_t *isBufLack);
int32_t Sql_GetFolderParentIndex(sqlite3 *db, uint32_t folderIndex);
int32_t Sql_GetFolderInode(sqlite3 *db, uint32_t folderIndex);
int32_t Sql_GetSubFolderCount(sqlite3 *db, uint32_t folderIndex);
int32_t Sql_GetSubFileCount(sqlite3 *db, uint32_t folderIndex, ContentsType type);
int32_t Sql_GetFolderRowCount(sqlite3 *db);
int32_t Sql_GetFolderIndex(sqlite3 *db, const char *name, uint32_t parentIndex);
int32_t Sql_FindFolderIndex(sqlite3 *db, const char *fullPath);
/* FileTable APIs */
int32_t Sql_InsertFileList(sqlite3 *db, const char *name, int32_t parentIndex, int32_t iNode, ContentsType type);
int32_t Sql_DeleteFileList(sqlite3 *db, uint32_t fileIndex, ContentsType type);
int32_t Sql_GetFileName(sqlite3 *db, uint32_t fileIndex, char *buffer, uint32_t bufSize, int32_t *isBufLack , ContentsType type);
int32_t Sql_GetFileParentIndex(sqlite3 *db,uint32_t fileIndex, ContentsType type);
int32_t Sql_GetFileInode(sqlite3 *db,uint32_t fileIndex, ContentsType type);
int32_t Sql_GetFileRowCount(sqlite3 *db, ContentsType type);

int32_t Sql_AddAllFolderList(sqlite3 *db,int32_t (*callback)(void* args, int32_t rowCount, int32_t folderIndex), void* pArg, const char * collationFn);
int32_t Sql_AddSubFolderList(sqlite3 *db, uint32_t parentFolderIndex, int32_t (*callback)(void* args, int32_t rowCount, int32_t folderIndex), void* pArg, const char * collationFn);
int32_t Sql_AddAllFileList(sqlite3 *db,ContentsType type, int32_t (*callback)(void* args, int32_t rowCount, int32_t fileIndex), void* pArg, const char * collationFn);
int32_t Sql_AddSubFileList(sqlite3 *db, uint32_t parentFolderIndex,ContentsType type, int32_t (*callback)(void* args, int32_t rowCount, int32_t fileIndex), void* pArg, const char * collationFn);

/* Sort Table APIs */
int32_t Sql_MakeSortedFolderList(sqlite3 * db);
int32_t Sql_MakeSortedFileList(sqlite3 * db, ContentsType type);
int32_t Sql_GetSortedFolderIndex(sqlite3 *db, uint32_t listIndex);
int32_t Sql_GetSortedFileIndex(sqlite3 *db,uint32_t listIndex, ContentsType type);

#ifdef META_DB_INCLUDE
int32_t Sql_CreateTagTable(sqlite3 *db);
int32_t Sql_Insert_Tag_Info(sqlite3 *db, int32_t fileIndex, const char *title, const char * artist, const char * album, const char *genre);
int32_t Sql_Insert_Tag_Info_UTF16(sqlite3 *db, int32_t fileIndex,
	const char *title, uint32_t titleLength,
	const char * artist, uint32_t artistLength,
	const char * album, uint32_t albumLength,
	const char *genre, uint32_t genreLength);
int32_t Sql_GetTagName(sqlite3 *db, uint32_t fileIndex, char *name, uint32_t bufSize, int32_t *isBufLack,TagCategory type);
int32_t Sql_GetTagNameAll(sqlite3 *db, uint32_t fileIndex, char *title,  char * artist,  char * album, char *genre, uint32_t bufSize);
int32_t Sql_InsertExtendTag_TextType(sqlite3 *db, int32_t fileIndex,const char *TagName, char *value);
int32_t Sql_InsertExtendTag_IntegerType(sqlite3 *db, int32_t fileIndex,const char *TagName, int32_t value);
#endif

int32_t Sql_Select_TextColumn_byIntegerType
	(sqlite3 *db, const char *SelectColumn, const char *FromTable, const char *WhereColumn, int32_t termsValue, char *GetBuf, uint32_t bufSize, int32_t *isBufLack);

#ifdef __cplusplus
}
#endif

#endif  /* _SQLITE_QUERY_H_ */

