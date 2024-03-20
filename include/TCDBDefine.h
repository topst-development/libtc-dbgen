/****************************************************************************************
 *   FileName    : TCDBDefine.h
 *   Description : header file for define of libtcdben
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
#ifndef TC_DB_DEFINE_H
#define TC_DB_DEFINE_H

#define META_DB_INCLUDE

#define TCDB_VERSION        	"1.0.1"
#define TCDB_VERSION_NUMBER 	1000000

#define MAX_PATH_LENGTH			32768
#define MAX_NAME_LENGTH			(512 + 2)
#define MAX_TAG_LENGTH				4096

/* err case */
#define DB_SUCCESS					0
#define DB_UNKOWN_ERR				-1
#define DB_ARGUMENT_ERR				-2
#define DB_DIR_OPEN_ERR				-3
#define DB_GATHERING_ERR			-4
#define DB_MAX_INSERTCOUNT			-5
#define DB_MEMORY_ALLOC_ERR			-6
#define DB_NOT_INITIALIZE			-7
#define DB_ACCESS_FS_ERR			-8
#define DB_NO_CONTENTS_ERR			-9
#define DB_INVALIDE_INDEX_ERR		-10
#define DB_INVALIDE_CATEGORY_ERR	-11
#define DB_CREATE_TASK_ERR			-12
#define DB_DISK_FULL_ERR			-13
#define DB_TASK_BUSY				-14
#define DB_FORCE_STOP				-15
#define DB_FS_READ_ERR				-16
#define DB_NOT_ENOUGH_BUFFER		-17			/*Success is reading the data from DB, but buffer is too small*/
#define DB_TAG_PARSING_ERR		-18
#define DB_NOT_SUPPORT_CODEC		-19
#define DB_SQLITE_OPEN_ERR			-100
#define DB_SQLITE_CLOSE_ERR		-101
#define DB_CREATE_TABLE_ERR		-102
#define DB_SQLITE_INITTABLE		-103
#define DB_SQLITE_INSERT_ERR		-104
#define DB_SQLITE_SELECT_ERR		-105
#define DB_SQLITE_DELETE_ERR		-106
#define DB_SQLITE_PRAGMA_ERR		-107

#define	INVALID_INDEX				0xFFFFFFFFU
#define	INVALID_COUNT				-1

#ifdef __cplusplus
extern "C" {
#endif

typedef enum
{
	NoneContents,
	AudioContents,
	VideoContents,
	ImageContents,
	TotalContentsType
}ContentsType;

typedef enum
{
	Tag_Title,
	Tag_Album,
	Tag_Artist,
	Tag_Genre,
	Tag_All
}TagCategory;

#ifdef __cplusplus
}
#endif

#endif /* TC_DB_DEFINE_H */

