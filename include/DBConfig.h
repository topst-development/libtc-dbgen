/****************************************************************************************
 *   FileName    : DBconfig.h
 *   Description : DBconfig.h
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
#ifndef DB_CONFIG_H
#define DB_CONFIG_H

typedef struct supportExtList
{
	char *_pExtName;
	struct supportExtList *_pNextExt;
}SupportExtList;

typedef struct blackListDir
{
	char *_pDirName;
	struct blackListDir *_pNextList;
}BlackListDir;

typedef struct audioTableInfo
{
	uint32_t	_maxFileCount;
	struct supportExtList *_pSupportList;
}AudioTableInfo;

typedef struct videoTableInfo
{
	uint32_t	_maxFileCount;
	struct supportExtList *_pSupportList;
}VideoTableInfo;

typedef struct imageTableInfo
{
	uint32_t	_maxFileCount;
	struct supportExtList *_pSupportList;
}ImageTableInfo;

typedef struct dbTagList
{
	char * _pTagName;
	struct dbTagList *_pNextList;
}DBTagList;

typedef struct  db_Config
{
	char *_dirName;
	uint32_t	_maxDBNum;
	uint32_t	_maxFolderNum;
	AudioTableInfo _audioTableInfo;
	VideoTableInfo _videoTableInfo;
	ImageTableInfo _imageTableInfo;
	DBTagList * _pMetaTable;
	uint32_t 	_fileUpdateCount;
	uint32_t 	_metaUpdateCount;
	BlackListDir	* _pBlackList;
}DB_Config;

extern DB_Config gDB_Config;

int32_t FindSupportExtName(struct supportExtList * extList, const char * extName);
int32_t FindBlackListDirName(BlackListDir * blackList, const char * dirName);
int32_t defautlConfig(DB_Config *dbConfig);
int32_t loadConfig(const char *fileName, DB_Config *dbConfig);
void releaseConfig(DB_Config *dbConfig);
void printfConfig(DB_Config *dbConfig);

#endif
