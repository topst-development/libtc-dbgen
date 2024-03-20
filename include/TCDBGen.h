/****************************************************************************************
 *   FileName    : TCDBGen.h
 *   Description : header file of libtcdbgen
 ****************************************************************************************
 *
 *   TCC Version 1.0
 *   Copyright (c) Telechips Inc.
 *   All rights reserved 
 
This library contains confidential information of Telechips.
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

#ifndef TC_DBGEN_H
#define TC_DBGEN_H

#ifdef __cplusplus
extern "C" {
#endif

#define DB_FILE_NAME_LENGTH	512

#define READ_ENTRY_BUF_SIZE (1024 * 32)

typedef enum{
	Empty_DB =0,
	Initialzie_DB,
	FileDB_Starting,
	FileDB_Completed,
	MetaDB_Starting,
	MetaDB_Completed,
	DB_Error
}DB_Status;

typedef void (*FileDBResult_cb)( uint32_t dbNum, int32_t result);
typedef void (*MetaDBResult_cb)(uint32_t dbNum, int32_t result);

typedef void (*FileDBUpdate_cb)( uint32_t dbNum, ContentsType type);
typedef void (*MetaDBUpdate_cb)(uint32_t dbNum);

typedef int32_t (*CompareFunction_cb)(void *data, int32_t len1, const void* in1, int32_t len2, const void* in2);
typedef int32_t (*GatheringTag_cb)(uint32_t dbNum, int32_t fileIndex, const char * folderPath, const char *fileName);

typedef struct tcdb_EventCB{
	FileDBResult_cb			_fileDBResult;
	MetaDBResult_cb			_metaDBResult;

	FileDBUpdate_cb			_fileDBUpdate;
	MetaDBUpdate_cb			_metaDBUpdate;
}TCDB_EventCB;

typedef struct tcdb_UserCB{
	CompareFunction_cb			_compareFunction;
	GatheringTag_cb				_gatheringTag;
}TCDB_UserCB;

void TCDB_SetEventCallbackFunctions(TCDB_EventCB *cb);
void TCDB_SetUserCallbackFunctions(TCDB_UserCB *cb);

const char *TCDB_libversion(void);
int32_t TCDB_libversion_number(void);
void TCDB_SetDebug(int32_t debugLevel);

int32_t TCDB_Initialize(const char *dbPath, uint32_t maxDBNum, const char *configFilePath);
void TCDB_Release(void);
int32_t TCDB_GenerateFileDB(uint32_t dbNum, const char *mountPath);
int32_t TCDB_DeleteDB(uint32_t dbNum);
uint32_t TCDB_GetTotalFolderCount(uint32_t dbNum);
uint32_t TCDB_GetTotalFileCount(uint32_t dbNum, ContentsType type);

int32_t TCDB_AddAllFolderList(uint32_t dbNum, int32_t (*callback)(void* args, int32_t rowCount, int32_t folderIndex), void *pArg);
int32_t TCDB_AddSubFolderList(uint32_t dbNum, uint32_t parentFolderIndex,int32_t (*callback)(void* args, int32_t rowCount, int32_t folderIndex), void *pArg);
int32_t TCDB_AddSubFileList(uint32_t dbNum, uint32_t parentFolderIndex, ContentsType type, int32_t (*callback)(void* args, int32_t rowCount, int32_t fileIndex), void *pArg);
int32_t TCDB_AddAllFileList(uint32_t dbNum, ContentsType type, int32_t (*callback)(void* args, int32_t rowCount, int32_t fileIndex), void *pArg);

uint32_t TCDB_GetSubFolderCount(uint32_t dbNum, uint32_t folderIndex);
uint32_t TCDB_GetSubFileCount(uint32_t dbNum, uint32_t folderIndex ,ContentsType type);
int32_t TCDB_GetFolderName(uint32_t dbNum, uint32_t folderIndex, char *folderName,uint32_t bufSize);
int32_t TCDB_GetFolderFullPath(uint32_t dbNum, uint32_t folderIndex, char *path, uint32_t bufSize);
uint32_t TCDB_GetParentFolderIndexofFolder(uint32_t dbNum, uint32_t folderIndex);
int32_t TCDB_GetFileName(uint32_t dbNum, uint32_t fileIndex, char *fileName, uint32_t bufSize, ContentsType type);
int32_t TCDB_GetFileFullPath(uint32_t dbNum, uint32_t fileIndex, char *path, uint32_t bufSize,ContentsType type);
int32_t TCDB_GetFileInode(uint32_t dbNum, uint32_t fileIndex, ContentsType type);
int32_t TCDB_AddFile_withFullpath(uint32_t dbNum, const char *path, ContentsType type);
uint32_t TCDB_GetParentFolderIndexofFile(uint32_t dbNum, uint32_t fileIndex, ContentsType type);
int32_t TCDB_DeleteFile(uint32_t dbNum, uint32_t fileIndex,ContentsType type);
int32_t TCDB_SQL_Errcode(uint32_t dbNum);
int32_t TCDB_SQL_Extended_ErrCode(uint32_t dbNum);
const char * TCDB_SQL_ErrStr(int32_t errcode);

#ifdef META_DB_INCLUDE
int32_t TCDB_Meta_MakeDB(uint32_t dbNum);
int32_t TCDB_Meta_InsertTag(uint32_t dbNum, int32_t fileIndex, const char *title, const char * artist, const char * album, const char *genre);
int32_t TCDB_Meta_GetTagName(uint32_t dbNum, uint32_t fileIndex, char *name, uint32_t bufSize,TagCategory type);
int32_t TCDB_Meta_GetTagNameAll(uint32_t dbNum, uint32_t fileIndex, char *title, char *artist, char *album, char *genre, uint32_t bufSize);

/* Meta Brwosing Low APIs */
int32_t TCDB_Meta_ResetBrowser(uint32_t dbNum);
/* categoryName : Title, Artist, Album, Genre, Folder*/
int32_t TCDB_Meta_GetNumberCategory(uint32_t dbNum,const char * categoryName);
int32_t TCDB_Meta_GetNumberCategory_Update(uint32_t dbNum,const char * categoryName);
int32_t TCDB_Meta_FindCategoryList(uint32_t dbNum, const char * category, uint32_t findDBIndex);
int32_t TCDB_Meta_GetCategoryList(uint32_t dbNum, const char * category, uint32_t readIndex);
int32_t TCDB_Meta_GetCategoryListName(uint32_t dbNum,const char * category, uint32_t readIndex, char *name, uint32_t bufSize);
int32_t TCDB_Meta_SelectCategory(uint32_t dbNum, const char * category, int32_t selectIndex);
int32_t TCDB_Meta_UndoCategory(uint32_t dbNum);
int32_t TCDB_Meta_MakePlayList(uint32_t dbNum,int32_t selectIndex);

uint32_t TCDB_Meta_GetTotalTrack(uint32_t dbNum);
uint32_t TCDB_Meta_GetTrackList(uint32_t dbNum, uint32_t listIndex);
uint32_t TCDB_Meta_GetFirstSelectTrackIndex(uint32_t dbNum);

#endif

#ifdef __cplusplus
}
#endif

#endif /* _TC_DBGEN_H_ */

