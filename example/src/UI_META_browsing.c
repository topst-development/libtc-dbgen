/****************************************************************************************
 *   FileName    : UI_META_browsing.c
 *   Description : UI_META_browsing.c
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

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <dirent.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <math.h>
#include <ctype.h>
#if __cplusplus
#include <stdbool.h>
#endif 
#include "sqlite3.h"
#include "TCDBDefine.h"
#include "TCDBGen.h"
#include "UI_META_browsing.h"

/****************************************************************************
Description
	one page unit on Navigation
	3 lines ard one unit on CPU interface.	
****************************************************************************/

#ifndef NULL
#define NULL 0
#endif

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

#if __GNUC__ > 2 || (__GNUC__ == 2 && __GNUC_MINOR__ >= 95)
#define error(...) do {\
	fprintf(stderr, "[%s:%d] ", __FUNCTION__, __LINE__); \
	fprintf(stderr, __VA_ARGS__); \
} while (0)
#else
#define error(args...) do {\
	fprintf(stderr, "[%s:%d] ", __FUNCTION__, __LINE__); \
	fprintf(stderr, ##args); \
} while (0)
#endif

extern int g_tcdb_debug;
#define DEBUG_TCDB_PRINTF(format, arg...) \
	if (g_tcdb_debug) \
	{ \
		fprintf(stderr, "[TCDB] [%s:%d] : "format"", __FUNCTION__, __LINE__, ##arg); \
	}

//#define NAVI_OPT_TOTAL_LIST_NUM_CHG

#define ADD_LIST_NUM		(1)
#define NORMAL_LIST_NUM	(0)
#define ALL_INDEX_NUM	(0)

typedef enum{
	UI_META_NAVI_ARTIST,
	UI_META_NAVI_ALBUM,
	UI_META_NAVI_GENRE,
	UI_META_NAVI_TRACK,
	UI_META_NAVI_MAX
} UI_META_NAVI_IPOD_BRWS_TYPE;

typedef struct
{
	char Data[MAX_TAG_LENGTH];

} DBGEN_RECORD_TYPE, *PDBGEN_RECORD_TYPE;

#ifdef __cplusplus
extern "C" {
#endif

/****************************************************************************
Description
	Navigation function list for meta
****************************************************************************/
typedef int (*fUI_META_NAVI_Initialize)( unsigned int DiskType);
typedef int (*fUI_META_NAVI_SelectParentItem)( unsigned int DiskType);
typedef int (*fUI_META_NAVI_SelectChildItem)( unsigned int DiskType, int nSelectIndex );
typedef int (*fUI_META_NAVI_DrawList)( unsigned int DiskType );


typedef struct 
{
	fUI_META_NAVI_Initialize			InitBrowse;
	fUI_META_NAVI_SelectParentItem	SelectParentItem;
	fUI_META_NAVI_SelectChildItem	SelectChildItem;
	fUI_META_NAVI_DrawList			DrawList;
} UI_META_NAVI_FUNCs;

typedef struct
{
	unsigned int CurrentDBType;
	unsigned int SelectedDBType;
	unsigned int bAudioBookSelected;
	unsigned int bNoFile;
}UI_META_IPOD_CLASSIFIED_DATA;

typedef union 
{
	UI_META_IPOD_CLASSIFIED_DATA	stIPODData;
} UI_META_DEVICE_CLASSIFIED_DATA;

typedef struct
{
	unsigned int DiskType;
	unsigned int bIsInitialized;

	unsigned int zPageIndex;
	unsigned int zListNumInPage;
	unsigned int nTotalItemNumber;

	unsigned int RootCategory;
	unsigned int CurrentCategory;
	unsigned int nDepth;
	unsigned int nSavedListIndex[UI_META_NAVI_MAX_DEPTH];

	unsigned int nMaxBrowsingListNumber;
	unsigned short *pBrowsingItemList;
	
	UI_META_DEVICE_CLASSIFIED_DATA ClassifiedData;
	
	UI_META_NAVI_FUNCs BrowsingFunc;
} UI_META_NAVI_INFO;


/****************************************************************************
Description

****************************************************************************/
static UI_META_NAVI_INFO gstMetaNaviInfo[UI_META_NAVI_MAX_NUM];

/****************************************************************************
Callbackl functions ( Static )
****************************************************************************/
static MetaCategoryMenuName_cb		_MetaCategoryMenuName = NULL;
static MetaCategoryIndexChange_cb		_MetaCategoryIndexChange = NULL;
static MetaCategoryName_cb				_MetaCategoryName = NULL;
static MetaCategoryNameCommit_cb		_MetaCategoryNameCommit = NULL;
static MetaSelectTrack_cb				_MetaSelectTrack = NULL;

/****************************************************************************
 Local functions ( Static )
****************************************************************************/

static unsigned int UI_META_NAVI_GetPageIndex( unsigned int DiskType );
static int UI_META_NAVI_SetPageIndex( unsigned int DiskType, unsigned int zPageIndex );

static unsigned int UI_META_NAVI_GetListNumInPage( unsigned int DiskType );
static int UI_META_NAVI_SetListNumInPage( unsigned int DiskType, unsigned int zListNumInPage );

static unsigned int UI_META_NAVI_GetTotalItemNumber( unsigned int DiskType );
static int UI_META_NAVI_SetTotalItemNumber( unsigned int DiskType, unsigned int nTotalItemNumber );

static unsigned int UI_META_NAVI_GetRootCategory( unsigned int DiskType );
static int UI_META_NAVI_SetRootCategory( unsigned int DiskType, unsigned int RootCategory );

static unsigned int UI_META_NAVI_GetCurrentCategory( unsigned int DiskType );
static int UI_META_NAVI_SetCurrentCategory( unsigned int DiskType, unsigned int CurrentCategory );

static unsigned int UI_META_NAVI_GetDepth( unsigned int DiskType );
static int UI_META_NAVI_SetDepth( unsigned int DiskType, unsigned int nDepth );

static unsigned int UI_META_NAVI_GetSavedListIndex( unsigned int DiskType, unsigned int nDepth );
static int UI_META_NAVI_SetSavedListIndex( unsigned int DiskType, unsigned int nDepth, unsigned int nSavedListIndex );

static unsigned short *UI_META_NAVI_GetBrowsingItemList( unsigned int DiskType );
static unsigned int UI_META_NAVI_GetBrowsingItemValue( unsigned int DiskType, unsigned int zIndex );
static unsigned int UI_META_NAVI_GetMaxBrowsingListNumber( unsigned int DiskType );

static void *UI_META_NAVI_GetClassifiedData( unsigned int DiskType );

static UI_META_NAVI_INFO *UI_META_NAVI_GetNaviInfo( unsigned int DiskType );
static int UI_META_NAVI_GetNaviInfoIndex( unsigned int DiskType );

static int UI_META_NAVI_IsListInitialized(  /*DISK_DEVICE*/unsigned int DiskType );
static int UI_META_NAVI_SetListInitializationFlag(  /*DISK_DEVICE*/unsigned int DiskType, unsigned int bIsInitialized );

static UI_META_NAVI_FUNCs *UI_META_NAVI_GetBrowsingFunctions( unsigned int DiskType );

static void UI_META_NAVI_StartPlay(unsigned int DiskType, unsigned int selectIndex);


static unsigned int UI_META_NAVI_DBGEN_IsSelectAllCategory( CategoryType eCategory );
static CategoryType UI_META_NAVI_DBGEN_GetBrowserTopCategory( unsigned int zIndex ) ;
static unsigned char *UI_META_NAVI_DBGEN_GetDBName( CategoryType eCategory) ;
static char *UI_META_NAVI_DBGEN_GetCategoryName( CategoryType eCategory, unsigned int *length ) ;
static char *UI_META_NAVI_DBGEN_GetMenuCategoryName( CategoryType eCategory, unsigned int *length ) ;
static CategoryType UI_META_NAVI_DBGEN_GetUpperCategory( unsigned int DiskType, CategoryType eCurrnetType, unsigned int nDepth );
static CategoryType UI_META_NAVI_DBGEN_GetSubCategory( CategoryType eCurrnetType);
static void UI_META_NAVI_DBGEN_DrawMenuName( unsigned int DiskType, CategoryType eCategory );
static int UI_META_NAVI_DBGEN_InitializeBrowseList(unsigned int DiskType);
static int UI_META_NAVI_DBGEN_SelectParentItem(unsigned int DiskType);
static int UI_META_NAVI_DBGEN_SelectChildItem( unsigned int DiskType, int nSelectIndex );
static int UI_META_NAVI_DBGEN_DrawList( unsigned int DiskType);

void UI_META_SetEventCallbackFunctions(TCDB_MetaBrwsEventCB *cb)
{
	if(NULL != cb)
	{
		_MetaCategoryMenuName = cb->_MetaCategoryMenuName;
		_MetaCategoryIndexChange = cb->_MetaCategoryIndexChange;
		_MetaCategoryName =  cb->_MetaCategoryName;
		_MetaCategoryNameCommit =  cb->_MetaCategoryNameCommit;
		_MetaSelectTrack = cb->_MetaSelectTrack;
	}	
}

static unsigned int UI_META_NAVI_GetPageIndex( unsigned int DiskType )
{
	unsigned int lPageIndex = 0;

	UI_META_NAVI_INFO *pstMetaNaviInfo = NULL;

	pstMetaNaviInfo = UI_META_NAVI_GetNaviInfo( DiskType );
	if ( NULL == pstMetaNaviInfo )
	{
		// Invalid
		lPageIndex = 0;
	}
	else
	{
		lPageIndex = pstMetaNaviInfo->zPageIndex;
	}	

	return lPageIndex;
}

static int UI_META_NAVI_SetPageIndex( unsigned int DiskType, unsigned int zPageIndex )
{
	int nErrorCode = 0;

	UI_META_NAVI_INFO *pstMetaNaviInfo = NULL;

	pstMetaNaviInfo = UI_META_NAVI_GetNaviInfo( DiskType );
	if ( NULL == pstMetaNaviInfo )
	{
		// Invalid
		nErrorCode = -1;
	}
	else
	{
		pstMetaNaviInfo->zPageIndex = zPageIndex;
	}	

	return nErrorCode;
}

static unsigned int UI_META_NAVI_GetListNumInPage( unsigned int DiskType )
{
	unsigned int lPageIndex = 0;

	UI_META_NAVI_INFO *pstMetaNaviInfo = NULL;

	pstMetaNaviInfo = UI_META_NAVI_GetNaviInfo( DiskType );
	if ( NULL == pstMetaNaviInfo )
	{
		// Invalid
		lPageIndex = 0;
	}
	else
	{
		lPageIndex = pstMetaNaviInfo->zListNumInPage;
	}	

	return lPageIndex;
}

static int UI_META_NAVI_SetListNumInPage( unsigned int DiskType, unsigned int zListNumInPage )
{
	int nErrorCode = 0;

	UI_META_NAVI_INFO *pstMetaNaviInfo = NULL;

	pstMetaNaviInfo = UI_META_NAVI_GetNaviInfo( DiskType );
	if ( NULL == pstMetaNaviInfo )
	{
		// Invalid
		nErrorCode = -1;
	}
	else
	{
		pstMetaNaviInfo->zListNumInPage = zListNumInPage;
	}	

	return nErrorCode;
}

static unsigned int UI_META_NAVI_GetTotalItemNumber( unsigned int DiskType )
{
	unsigned int lTotalItemNumber = 0;

	UI_META_NAVI_INFO *pstMetaNaviInfo = NULL;

	pstMetaNaviInfo = UI_META_NAVI_GetNaviInfo( DiskType );
	if ( NULL == pstMetaNaviInfo )
	{
		// Invalid
		lTotalItemNumber = 0;
	}
	else
	{
		lTotalItemNumber = pstMetaNaviInfo->nTotalItemNumber;
	}		

	return lTotalItemNumber;
}

static int UI_META_NAVI_SetTotalItemNumber( unsigned int DiskType, unsigned int nTotalItemNumber )
{
	int nErrorCode = 0;

	UI_META_NAVI_INFO *pstMetaNaviInfo = NULL;

	pstMetaNaviInfo = UI_META_NAVI_GetNaviInfo( DiskType );
	if ( NULL == pstMetaNaviInfo )
	{
		// Invalid
		nErrorCode = -1;
	}
	else
	{
		pstMetaNaviInfo->nTotalItemNumber = nTotalItemNumber;
	}	

	return nErrorCode;
}

static unsigned int UI_META_NAVI_GetRootCategory( unsigned int DiskType )
{
	unsigned int lRootCategory = 0;

	UI_META_NAVI_INFO *pstMetaNaviInfo = NULL;

	pstMetaNaviInfo = UI_META_NAVI_GetNaviInfo( DiskType );
	if ( NULL == pstMetaNaviInfo )
	{
		// Invalid
		lRootCategory = 0;
	}
	else
	{
		lRootCategory = pstMetaNaviInfo->RootCategory;
	}

	return lRootCategory;
}

static int UI_META_NAVI_SetRootCategory( unsigned int DiskType, unsigned int RootCategory )
{
	int nErrorCode = 0;
	UI_META_NAVI_INFO *pstMetaNaviInfo = NULL;

	pstMetaNaviInfo = UI_META_NAVI_GetNaviInfo( DiskType );
	if ( NULL == pstMetaNaviInfo )
	{
		// Invalid
		nErrorCode = -1;
	}
	else
	{
		pstMetaNaviInfo->RootCategory = RootCategory;
	}

	return nErrorCode;
}

static unsigned int UI_META_NAVI_GetCurrentCategory( unsigned int DiskType )
{
	unsigned int lCurrentCategory = 0;
	
	UI_META_NAVI_INFO *pstMetaNaviInfo = NULL;

	pstMetaNaviInfo = UI_META_NAVI_GetNaviInfo( DiskType );
	if ( NULL == pstMetaNaviInfo )
	{
		// Invalid
		lCurrentCategory = 0;
	}
	else
	{
		lCurrentCategory = pstMetaNaviInfo->CurrentCategory;
	}

	return lCurrentCategory;
}

static int UI_META_NAVI_SetCurrentCategory( unsigned int DiskType, unsigned int CurrentCategory )
{
	int nErrorCode = 0;

	UI_META_NAVI_INFO *pstMetaNaviInfo = NULL;

	pstMetaNaviInfo = UI_META_NAVI_GetNaviInfo( DiskType );
	if ( NULL == pstMetaNaviInfo )
	{
		// Invalid
		nErrorCode = -1;
	}
	else
	{
		pstMetaNaviInfo->CurrentCategory = CurrentCategory;
	}

	return nErrorCode;
}

static unsigned int UI_META_NAVI_GetDepth( unsigned int DiskType )
{
	unsigned int lDepth = 0;

	UI_META_NAVI_INFO *pstMetaNaviInfo = NULL;

	pstMetaNaviInfo = UI_META_NAVI_GetNaviInfo( DiskType );
	if ( NULL == pstMetaNaviInfo )
	{
		// Invalid
		lDepth = 0;
	}
	else
	{
		lDepth = pstMetaNaviInfo->nDepth;
	}

	return lDepth;
}

static int UI_META_NAVI_SetDepth( unsigned int DiskType, unsigned int nDepth )
{
	int nErrorCode = 0;

	UI_META_NAVI_INFO *pstMetaNaviInfo = NULL;

	pstMetaNaviInfo = UI_META_NAVI_GetNaviInfo( DiskType );
	if ( NULL == pstMetaNaviInfo )
	{
		// Invalid
		nErrorCode = -1;
	}
	else
	{
		pstMetaNaviInfo->nDepth = nDepth;
	}

	return nErrorCode;
}

static unsigned int UI_META_NAVI_GetSavedListIndex( unsigned int DiskType, unsigned int nDepth )
{
	unsigned int lSavedListIndex = 0;

	UI_META_NAVI_INFO *pstMetaNaviInfo = NULL;

	pstMetaNaviInfo = UI_META_NAVI_GetNaviInfo( DiskType );
	if ( (NULL == pstMetaNaviInfo) || (UI_META_NAVI_MAX_DEPTH<nDepth) )
	{
		// Invalid
		lSavedListIndex = 0;
	}
	else
	{
		lSavedListIndex = pstMetaNaviInfo->nSavedListIndex[nDepth];
	}

	return lSavedListIndex;
}

static int UI_META_NAVI_SetSavedListIndex( unsigned int DiskType, unsigned int nDepth, unsigned int nSavedListIndex )
{
	int nErrorCode = 0;

	UI_META_NAVI_INFO *pstMetaNaviInfo = NULL;

	pstMetaNaviInfo = UI_META_NAVI_GetNaviInfo( DiskType );
	if ( (NULL == pstMetaNaviInfo) || (UI_META_NAVI_MAX_DEPTH<nDepth) )
	{
		// Invalid
		nErrorCode = -1;
	}
	else
	{
		if ( nSavedListIndex <= pstMetaNaviInfo->nTotalItemNumber )
		{
			pstMetaNaviInfo->nSavedListIndex[nDepth] = nSavedListIndex;
		}
		else
		{
			pstMetaNaviInfo->nSavedListIndex[nDepth] = 0;
		}
	}

	return nErrorCode;
}

static unsigned short *UI_META_NAVI_GetBrowsingItemList( unsigned int DiskType )
{
	unsigned short *plBrowsingItemList = NULL;

	UI_META_NAVI_INFO *pstMetaNaviInfo = NULL;

	pstMetaNaviInfo = UI_META_NAVI_GetNaviInfo( DiskType );
	if ( NULL == pstMetaNaviInfo )
	{
		// Invalid
		plBrowsingItemList = 0;
	}
	else
	{
		plBrowsingItemList = pstMetaNaviInfo->pBrowsingItemList;
	}
	
	return plBrowsingItemList;
}

static unsigned int UI_META_NAVI_GetBrowsingItemValue( unsigned int DiskType, unsigned int zIndex )
{
	unsigned short *pBrowsingList = NULL;
	unsigned int BrowsingItem = 0;
	unsigned int lMaxBrowsingListNumber = 0;

	pBrowsingList = UI_META_NAVI_GetBrowsingItemList( DiskType );
	lMaxBrowsingListNumber = UI_META_NAVI_GetMaxBrowsingListNumber( DiskType );
	if ( pBrowsingList && (zIndex < lMaxBrowsingListNumber) )
	{
		BrowsingItem = pBrowsingList[zIndex];
	}
	else
	{
		BrowsingItem = UI_META_NAVI_ITEM_EMPTY_VALUE;
	}

	return BrowsingItem;
}

static unsigned int UI_META_NAVI_GetMaxBrowsingListNumber( unsigned int DiskType )
{
	unsigned int lMaxBrowsingListNumber = 0;

	UI_META_NAVI_INFO *pstMetaNaviInfo = NULL;

	pstMetaNaviInfo = UI_META_NAVI_GetNaviInfo( DiskType );
	if ( NULL == pstMetaNaviInfo )
	{
		// Invalid
		lMaxBrowsingListNumber = 0;
	}
	else
	{
		lMaxBrowsingListNumber = pstMetaNaviInfo->nMaxBrowsingListNumber;
	}

	return lMaxBrowsingListNumber;
}

static void *UI_META_NAVI_GetClassifiedData( unsigned int DiskType )
{
	void *pData = NULL;

	UI_META_NAVI_INFO *pstMetaNaviInfo = NULL;

	pstMetaNaviInfo = UI_META_NAVI_GetNaviInfo( DiskType );
	if ( NULL == pstMetaNaviInfo )
	{
		// Invalid
		pData = 0;
	}
	else
	{
		pData = &(pstMetaNaviInfo->ClassifiedData);
	}	

	return pData;
}

static UI_META_NAVI_FUNCs *UI_META_NAVI_GetBrowsingFunctions( unsigned int DiskType )
{
	UI_META_NAVI_FUNCs *pstBrowsingFunc = NULL ;

	UI_META_NAVI_INFO *pstMetaNaviInfo = NULL;

	pstMetaNaviInfo = UI_META_NAVI_GetNaviInfo( DiskType );
	if ( NULL == pstMetaNaviInfo )
	{
		// Invalid
		pstBrowsingFunc = 0;
	}
	else
	{
		pstBrowsingFunc = &(pstMetaNaviInfo->BrowsingFunc);
	}

	return pstBrowsingFunc;
}

static UI_META_NAVI_INFO *UI_META_NAVI_GetNaviInfo( unsigned int DiskType )
{
	UI_META_NAVI_INFO *pstMetaNaviInfo = NULL ;
	unsigned int nDeviceIndex = 0;
	
	for(nDeviceIndex=0; nDeviceIndex<UI_META_NAVI_MAX_NUM; nDeviceIndex++){
		if ( DiskType == gstMetaNaviInfo[nDeviceIndex].DiskType ){
			pstMetaNaviInfo = &gstMetaNaviInfo[nDeviceIndex];
			break;
		}
	}

	return pstMetaNaviInfo;
}

static int UI_META_NAVI_GetNaviInfoIndex( unsigned int DiskType )
{
	unsigned int nDeviceIndex = 0;
	int zNaviInfoIndex = (-1);
	
	for(nDeviceIndex=0; nDeviceIndex<UI_META_NAVI_MAX_NUM; nDeviceIndex++){
		if ( DiskType == gstMetaNaviInfo[nDeviceIndex].DiskType ){
			zNaviInfoIndex = (int)nDeviceIndex;
			break;
		}
	}

	return zNaviInfoIndex;
}

static int UI_META_NAVI_IsListInitialized( /*DISK_DEVICE*/unsigned int DiskType )
{
	unsigned int nDeviceIndex = 0;
	int lIsInitialized = FALSE;
	
	for(nDeviceIndex=0; nDeviceIndex<UI_META_NAVI_MAX_NUM; nDeviceIndex++)
	{
		if ( DiskType == gstMetaNaviInfo[nDeviceIndex].DiskType )
		{
			if ( TRUE == gstMetaNaviInfo[nDeviceIndex].bIsInitialized )
			{
				lIsInitialized = TRUE;
				break;
			}
		}
	}

	return lIsInitialized;
}

static int UI_META_NAVI_SetListInitializationFlag( unsigned int DiskType, unsigned int bIsInitialized )
{
	int nErrorCode = 0;
	UI_META_NAVI_INFO *pstMetaNaviInfo = NULL;

	pstMetaNaviInfo = UI_META_NAVI_GetNaviInfo( DiskType );

	if ( NULL == pstMetaNaviInfo )
	{
		// Invalid
		nErrorCode = -1;
	}
	else
	{
		if ( 0 == bIsInitialized )
		{
			pstMetaNaviInfo->bIsInitialized = FALSE;
		}
		else
		{
			pstMetaNaviInfo->bIsInitialized = TRUE;
		}
	}

	return nErrorCode;
}

int UI_META_NAVI_ClearAllNaviInfo( void )
{
	int nErrorCode = 0;
	UI_META_NAVI_FUNCs *pstBrowsingFunc = NULL;
	UI_META_NAVI_INFO *pstMetaNaviInfo = NULL;
	unsigned int nDeviceIndex = 0;

	DEBUG_TCDB_PRINTF("\n");

	for(nDeviceIndex=0; nDeviceIndex<UI_META_NAVI_MAX_NUM; nDeviceIndex++)
	{
		pstMetaNaviInfo = &gstMetaNaviInfo[nDeviceIndex];
		pstBrowsingFunc = &(pstMetaNaviInfo->BrowsingFunc);

		pstMetaNaviInfo->DiskType			= 0;
		pstMetaNaviInfo->bIsInitialized		= 0;
		pstMetaNaviInfo->zPageIndex			= 0;
		pstMetaNaviInfo->nTotalItemNumber	= 0;
		pstMetaNaviInfo->RootCategory		= 0;
		pstMetaNaviInfo->CurrentCategory	= 0;
		pstMetaNaviInfo->nDepth				= 0;

		memset ( &(pstMetaNaviInfo->nSavedListIndex), 0x00, sizeof(pstMetaNaviInfo->nSavedListIndex) );
		
		pstMetaNaviInfo->nMaxBrowsingListNumber	= 0;
		if(pstMetaNaviInfo->pBrowsingItemList != 0)
		{
			(void) free(pstMetaNaviInfo->pBrowsingItemList);
			pstMetaNaviInfo->pBrowsingItemList	= 0;
		}

		memset ( &(pstMetaNaviInfo->ClassifiedData), 0x00, sizeof(pstMetaNaviInfo->ClassifiedData) );

		pstBrowsingFunc->InitBrowse			= NULL;
		pstBrowsingFunc->SelectParentItem	= NULL;
		pstBrowsingFunc->SelectChildItem	= NULL;
		//pstBrowsingFunc->MoveUp			= NULL;
		//pstBrowsingFunc->MoveDown			= NULL;
		//pstBrowsingFunc->DrawList			= NULL;
	}
	
	return nErrorCode;
}

int UI_META_NAVI_AddNaviInfo( unsigned int dbNum )
{
	int nErrorCode = 0;
	UI_META_NAVI_FUNCs *pstBrowsingFunc = NULL;
	UI_META_NAVI_INFO *pstMetaNaviInfo = NULL;

	// Check existance
	pstMetaNaviInfo = UI_META_NAVI_GetNaviInfo( dbNum );
	if ( NULL != pstMetaNaviInfo )
	{
		// Invalid
		nErrorCode = -1;
	}
	else
	{
		// Find empty info
		pstMetaNaviInfo = UI_META_NAVI_GetNaviInfo( 0 );
		if ( NULL == pstMetaNaviInfo )
		{
			// Invalid
			nErrorCode = -2;
		}
		else
		{
#if 0 /* Don't need this function for codesonar */
			int zNaviInfoIndex = UI_META_NAVI_GetNaviInfoIndex( NO_DEVICE_NUM );

			if ( zNaviInfoIndex < 0){
				zNaviInfoIndex = 0;
			}
#endif			
			pstBrowsingFunc = &(pstMetaNaviInfo->BrowsingFunc);
			pstMetaNaviInfo->DiskType			= dbNum;
			pstMetaNaviInfo->bIsInitialized		= 0;
			pstMetaNaviInfo->zPageIndex			= 0;
			pstMetaNaviInfo->nTotalItemNumber	= 0;
			pstMetaNaviInfo->RootCategory		= 0;
			pstMetaNaviInfo->CurrentCategory	= 0;
			pstMetaNaviInfo->nDepth				= 0;
			memset( &(pstMetaNaviInfo->nSavedListIndex), 0x00, sizeof(pstMetaNaviInfo->nSavedListIndex) );

			pstMetaNaviInfo->nMaxBrowsingListNumber = 0; /* Not used filed */
			pstMetaNaviInfo->pBrowsingItemList = 0; /* Not used filed */

			pstBrowsingFunc->InitBrowse			= UI_META_NAVI_DBGEN_InitializeBrowseList;
			pstBrowsingFunc->SelectParentItem	= UI_META_NAVI_DBGEN_SelectParentItem;
			pstBrowsingFunc->SelectChildItem	= UI_META_NAVI_DBGEN_SelectChildItem;
			//pstBrowsingFunc->MoveUp			= NULL;
			//pstBrowsingFunc->MoveDown			= NULL;
			pstBrowsingFunc->DrawList			= UI_META_NAVI_DBGEN_DrawList;
		}
	}

	return nErrorCode;
}

int UI_META_NAVI_DeleteNaviInfo( unsigned int dbNum )
{
	int nErrorCode = 0;
	UI_META_NAVI_FUNCs *pstBrowsingFunc = NULL;
	UI_META_NAVI_INFO *pstMetaNaviInfo = NULL;
	DEBUG_TCDB_PRINTF("dbNum (%d)\n", dbNum);
	pstMetaNaviInfo = UI_META_NAVI_GetNaviInfo( dbNum );
	if ( NULL == pstMetaNaviInfo )
	{
		// Invalid
		nErrorCode = -1;
	}
	else
	{
		pstBrowsingFunc = UI_META_NAVI_GetBrowsingFunctions( dbNum );
#if 0 /* Redundant Condition - Codesonar */
		if ( NULL == pstBrowsingFunc )
		{
			// Invalid
			nErrorCode = -2;
		}
		else
#endif
		{
			pstMetaNaviInfo->DiskType			= 0;
			pstMetaNaviInfo->bIsInitialized		= 0;
			pstMetaNaviInfo->zPageIndex			= 0;
			pstMetaNaviInfo->nTotalItemNumber	= 0;
			pstMetaNaviInfo->RootCategory		= 0;
			pstMetaNaviInfo->CurrentCategory	= 0;
			pstMetaNaviInfo->nDepth				= 0;
			memset ( &(pstMetaNaviInfo->nSavedListIndex), 0x00, sizeof(pstMetaNaviInfo->nSavedListIndex) );

			memset ( &(pstMetaNaviInfo->ClassifiedData), 0x00, sizeof(pstMetaNaviInfo->ClassifiedData) );

			pstMetaNaviInfo->nMaxBrowsingListNumber	= 0;
			if(pstMetaNaviInfo->pBrowsingItemList != 0)
			{
				(void) free(pstMetaNaviInfo->pBrowsingItemList);
				pstMetaNaviInfo->pBrowsingItemList	= 0;
			}

			pstBrowsingFunc->InitBrowse			= NULL;
			pstBrowsingFunc->SelectParentItem	= NULL;
			pstBrowsingFunc->SelectChildItem	= NULL;
			//pstBrowsingFunc->MoveUp			= NULL;
			//pstBrowsingFunc->MoveDown			= NULL;
			//pstBrowsingFunc->DrawList			= NULL;
		}	

	}

	return nErrorCode;
}

int UI_META_NAVI_StartBrowse( unsigned int  dbNum, unsigned int ListNumInPage )
{
	int nErrorCode = 0;
	/*Get the current folder's information*/
	DEBUG_TCDB_PRINTF("dbNum (%d), ListNumInPage (%d)\n", dbNum, ListNumInPage);
	UI_META_NAVI_FUNCs *pstBrowsingFunc = UI_META_NAVI_GetBrowsingFunctions( dbNum );
	if ( NULL == pstBrowsingFunc )
	{
		nErrorCode = UI_META_NAVI_AddNaviInfo( dbNum );
		pstBrowsingFunc = UI_META_NAVI_GetBrowsingFunctions( dbNum );
		if ( NULL == pstBrowsingFunc )
		{
			nErrorCode = -2;
		}
	}

	if(nErrorCode == 0)
	{
		unsigned int lDepth = 0;
		unsigned int lSavedListIndex = 0;
		unsigned int bListInitialized = 0;
		unsigned int lListNumInPage = 0;

		bListInitialized = (unsigned int)UI_META_NAVI_IsListInitialized( dbNum );
		if ( FALSE ==  bListInitialized )
		{
			if ( NULL != pstBrowsingFunc->InitBrowse )
			{
				nErrorCode = pstBrowsingFunc->InitBrowse(dbNum);
				(void) UI_META_NAVI_SetListNumInPage( dbNum, ListNumInPage );
				(void) UI_META_NAVI_SetListInitializationFlag( dbNum, TRUE );
			}
		}

		// Recover current object index
		lDepth = UI_META_NAVI_GetDepth( dbNum );
		lSavedListIndex = UI_META_NAVI_GetSavedListIndex( dbNum, lDepth );		
		if ( 0 < lSavedListIndex )
		{
			lListNumInPage = UI_META_NAVI_GetListNumInPage( dbNum );
			if(lListNumInPage != 0)
			{
				lSavedListIndex = ((lSavedListIndex-1) % lListNumInPage) + 1;
			}
		}
		else
		{
			lSavedListIndex = 0;			
		}
		
		if ( NULL != pstBrowsingFunc->DrawList )
		{
			nErrorCode = pstBrowsingFunc->DrawList(dbNum);
		}
	}

	if(nErrorCode != 0)
	{
		error("error : %d\n", nErrorCode);
	}
	return nErrorCode;	
}


void UI_META_NAVI_SelectParentItem(unsigned int  dbNum)
{
	UI_META_NAVI_FUNCs *pstBrowsingFunc = NULL;
	int nErrorCode = 0;
	DEBUG_TCDB_PRINTF("dbNum (%d)\n", dbNum);
	pstBrowsingFunc = UI_META_NAVI_GetBrowsingFunctions( dbNum );
	if ( (NULL != pstBrowsingFunc) && (NULL != pstBrowsingFunc->SelectParentItem) )
	{
		nErrorCode = pstBrowsingFunc->SelectParentItem(dbNum);
	}
	else
	{
		nErrorCode = -2;
	}

	if(nErrorCode != 0)
	{
		error("error : %d\n", nErrorCode);
	}
}

void UI_META_NAVI_SelectChildItem(unsigned int dbNum, unsigned int CurrentIndex)
{
	unsigned int zIndex; 
	UI_META_NAVI_FUNCs *pstBrowsingFunc = NULL;
	int nErrorCode = 0;
	DEBUG_TCDB_PRINTF("dbNum (%d), CurrentIndex (%d)\n", dbNum, CurrentIndex);
	if ( 0 == UI_META_NAVI_GetTotalItemNumber( dbNum ) )
	{
		nErrorCode = -2;
	}
	else
	{
		zIndex = ( UI_META_NAVI_GetPageIndex( dbNum )*UI_META_NAVI_GetListNumInPage( dbNum )) + CurrentIndex;

		pstBrowsingFunc = UI_META_NAVI_GetBrowsingFunctions( dbNum );
		if ( /* (NULL!=pstBrowsingFunc) && */ (pstBrowsingFunc->SelectChildItem)) /* Redundant Condition - Codesonar */
		{
			nErrorCode = pstBrowsingFunc->SelectChildItem( dbNum, (int)zIndex );
		}
	}

	if(nErrorCode != 0)
	{
		error("error : %d\n", nErrorCode);
	}
}

void UI_META_NAVI_Move(unsigned int  dbNum, unsigned int CurrentIndex)
{
	UI_META_NAVI_FUNCs *pstBrowsingFunc = NULL;
	int nErrorCode = 0;
	DEBUG_TCDB_PRINTF("dbNum (%d), CurrentIndex (%d)\n", dbNum, CurrentIndex);
	pstBrowsingFunc = UI_META_NAVI_GetBrowsingFunctions( dbNum );
	if ( NULL == pstBrowsingFunc) 
	{
		nErrorCode = -1;
	}
	else
	{
		unsigned int lTotalItemNumber = UI_META_NAVI_GetTotalItemNumber( dbNum );
		
		if( 0 == lTotalItemNumber )
		{
			nErrorCode = -2;
		}
		else
		{
			unsigned int lPageIndex, lListNumInPage;

			lListNumInPage = UI_META_NAVI_GetListNumInPage( dbNum );
			if(lListNumInPage != 0)
			{
				lPageIndex = CurrentIndex/lListNumInPage;
			}
			else
			{
				lPageIndex = CurrentIndex;
			}
			(void) UI_META_NAVI_SetPageIndex( dbNum, lPageIndex );

			if ( pstBrowsingFunc->DrawList != 0 )
			{
				nErrorCode = pstBrowsingFunc->DrawList(dbNum);
			}
		}
	}

	if(nErrorCode != 0)
	{
		error("error : %d\n", nErrorCode);
	}
}

#if 0
void UI_META_NAVI_MoveUp(unsigned int  dbNum, unsigned int CurrentIndex, unsigned int ListCount)
{
	UI_META_NAVI_FUNCs *pstBrowsingFunc = NULL;
	int nErrorCode = 0;

	pstBrowsingFunc = UI_META_NAVI_GetBrowsingFunctions( dbNum );
	if ( NULL == pstBrowsingFunc) 
	{
		nErrorCode = -1;
	}
	else
	{
		unsigned int lTotalItemNumber = UI_META_NAVI_GetTotalItemNumber( dbNum );
		
		if( 0 == lTotalItemNumber )
		{
			nErrorCode = -2;
		}
		else
		{
			unsigned int lPageIndex, lListNumInPage;

			lListNumInPage = UI_META_NAVI_GetListNumInPage( dbNum );
			if(lListNumInPage != 0)
			{
				lPageIndex = CurrentIndex/lListNumInPage;
			}
			else
			{
				lPageIndex = CurrentIndex;
			}
			(void) UI_META_NAVI_SetPageIndex( dbNum, lPageIndex );

			if ( pstBrowsingFunc->DrawList != 0 )
			{
				nErrorCode = pstBrowsingFunc->DrawList(dbNum);
			}
		}
	}

	if(nErrorCode != 0)
	{
		error("error : %d\n", nErrorCode);
	}
}

void UI_META_NAVI_MoveDown(unsigned int  dbNum, unsigned int CurrentIndex, unsigned int ListCount)
{
	UI_META_NAVI_FUNCs *pstBrowsingFunc = NULL;
	int nErrorCode = 0;

	pstBrowsingFunc = UI_META_NAVI_GetBrowsingFunctions( dbNum );
	if ( NULL == pstBrowsingFunc) 
	{
		nErrorCode = -1;
	}
	else
	{
		unsigned int lTotalItemNumber = UI_META_NAVI_GetTotalItemNumber( dbNum );
	
		if( 0 == lTotalItemNumber )
		{
			nErrorCode = -2;
		}
		else
		{
			unsigned int lPageIndex, lListNumInPage;

			lListNumInPage = UI_META_NAVI_GetListNumInPage( dbNum );
			if(lListNumInPage != 0)
			{
				lPageIndex = CurrentIndex/lListNumInPage;
			}
			else
			{
				lPageIndex = CurrentIndex;
			}
			(void) UI_META_NAVI_SetPageIndex( dbNum, lPageIndex );

			if ( pstBrowsingFunc->DrawList != 0 )
			{
				nErrorCode = pstBrowsingFunc->DrawList(dbNum);
			}
		}
	}

	if(nErrorCode != 0)
	{
		error("error : %d\n", nErrorCode);
	}
}
#endif

void UI_META_NAVI_ExitBrowse(unsigned int  dbNum, unsigned int nSelectIndex)
{
	unsigned int lDepth = 0;
	DEBUG_TCDB_PRINTF("dbNum (%d), SelectIndex (%d) \n", dbNum, nSelectIndex);	
	// Store current object index
	lDepth = UI_META_NAVI_GetDepth( dbNum );

	(void) UI_META_NAVI_SetSavedListIndex( dbNum, lDepth, nSelectIndex );
}

int UI_META_NAVI_HomeBrowse(unsigned int  dbNum, unsigned int ListNumInPage)
{
	UI_META_NAVI_FUNCs *pstBrowsingFunc = NULL;
	int nErrorCode = 0;
	DEBUG_TCDB_PRINTF("dbNum (%d), ListNumInPage (%d)\n", dbNum, ListNumInPage);
	pstBrowsingFunc = UI_META_NAVI_GetBrowsingFunctions( dbNum );
	if ( NULL == pstBrowsingFunc) 
	{
		if(ListNumInPage == 0)
		{
			nErrorCode = -1;
		}
		else
		{
			nErrorCode = UI_META_NAVI_AddNaviInfo( dbNum );
			pstBrowsingFunc = UI_META_NAVI_GetBrowsingFunctions( dbNum );
			if ( NULL == pstBrowsingFunc )
			{
				nErrorCode = -2;
			}
		}
	}

	if(nErrorCode == 0)
	{
		// Set Depth, RootCategory, CurrentCategory
		(void) UI_META_NAVI_SetCurrentCategory( dbNum, 0 );
		(void) UI_META_NAVI_SetRootCategory( dbNum, 0 );
		(void) UI_META_NAVI_SetDepth( dbNum, 0 );
		(void) UI_META_NAVI_SetPageIndex( dbNum, 0 );

		// Call Init Browser
		if(pstBrowsingFunc->InitBrowse != 0)
		{
			nErrorCode = pstBrowsingFunc->InitBrowse(dbNum);
			if(ListNumInPage != 0)
			{
				(void) UI_META_NAVI_SetListNumInPage( dbNum, ListNumInPage );
			}
			(void) UI_META_NAVI_SetListInitializationFlag( dbNum, TRUE );
		}

		// Draw Meta List
		if ( NULL != pstBrowsingFunc->DrawList )
		{
			nErrorCode = pstBrowsingFunc->DrawList(dbNum);
		}
	}

	if(nErrorCode != 0)
	{
		error("error : %d\n", nErrorCode);
	}
	return nErrorCode;
}

static void UI_META_NAVI_StartPlay(unsigned int DiskType, unsigned int selectIndex)
{
	if(NULL != _MetaSelectTrack)
	{
		_MetaSelectTrack(DiskType,selectIndex);
	}
}

///////////////////////////////////////////////////////////////////////////////////
//
//               Example of iPod meta browsing tree
//               
//        
// <Depth>	0				1				2				3				4
//
//		[GENRE]			Genre list		Artist list		Album list		Track list
//
//		[ARTIST]			Artist list		Album list		Track list
//
//		[ALBUM]			Album list	Track list
//
//		[TRACK]			Track list
//
//		[PLAYLIST]		Playlist list	Track list
//
//		[COMPOSER]		Composer list	Album list		Track list
//
//		[PODCAST]		Podcast list	Track list
//
//		[AUDIOBOOK]		Audiobook list
//
///////////////////////////////////////////////////////////////////////////////////

#define UI_META_ROOTCATEGORYDEPTH	(0)

#define UI_META_BACK_INDEX		(0)
#define UI_META_SELECTALL_INDEX	(1)

static unsigned int UI_META_NAVI_DBGEN_IsSelectAllCategory( CategoryType eCategory )
{
	unsigned int bSelectAllCategory;

	switch( eCategory )
	{
		case META_ARTIST:
		case META_ALBUM:
		case META_GENRE:
			bSelectAllCategory = 1;
			break;

		case META_TRACK:
			bSelectAllCategory = 0;
			break;

		default :
			bSelectAllCategory = 1;
			break;
	}
	return bSelectAllCategory;
}

static CategoryType UI_META_NAVI_DBGEN_GetBrowserTopCategory( unsigned int zIndex ) 
{
	CategoryType eCategory = META_ALL;

	switch( zIndex )
	{
		case UI_META_NAVI_GENRE:
			eCategory = META_GENRE;
			break;

		case UI_META_NAVI_ARTIST:
			eCategory = META_ARTIST;
			break;

		case UI_META_NAVI_ALBUM:
			eCategory = META_ALBUM;
			break;

		case UI_META_NAVI_TRACK:
			eCategory = META_TRACK;
			break;
		default :
			eCategory = META_GENRE;
			break;
	}

	return eCategory;
} 

static unsigned char *UI_META_NAVI_DBGEN_GetDBName( CategoryType eCategory) 
{
	unsigned char *pCategoryName = NULL;

	switch( eCategory )
	{
		case META_ARTIST:
			pCategoryName = (unsigned char *)"Artist\0";
			break;

		case META_ALBUM:
			pCategoryName = (unsigned char *)"Album\0";
			break;

		case META_GENRE:
			pCategoryName = (unsigned char *)"Genre\0";
			break;

		case META_TRACK:
			pCategoryName = (unsigned char *)"Title\0";
			break;
		default :
			pCategoryName = NULL;
			break;
	}

	return pCategoryName;
}

static char *UI_META_NAVI_DBGEN_GetCategoryName( CategoryType eCategory, unsigned int *length ) 
{
	char *pCategoryName = NULL;

	switch( eCategory )
	{
		case META_ARTIST:
			pCategoryName = (char *)"ARTIST\0";
			*length = 6;
			break;

		case META_ALBUM:
			pCategoryName = (char *)"ALBUM\0";
			*length = 5;
			break;

		case META_GENRE:
			pCategoryName = (char *)"GENRE\0";
			*length = 5;
			break;

		case META_TRACK:
			pCategoryName = (char *)"TRACK\0";
			*length = 5;
			break;
		default :
			pCategoryName = NULL;
			*length = 0;
			break;
	}

	return pCategoryName;
}

static char *UI_META_NAVI_DBGEN_GetMenuCategoryName( CategoryType eCategory, unsigned int *length ) 
{
	char *pCategoryName = NULL;

	switch( eCategory )
	{
		case META_ALL:
			pCategoryName = (char *)"CATEGORY\0";
			*length = 8;
			break;

		case META_ARTIST:
			pCategoryName = (char *)"ARTIST\0";
			*length = 6;
			break;

		case META_ALBUM:
			pCategoryName = (char *)"ALBUM\0";
			*length = 5;
			break;

		case META_GENRE:
			pCategoryName = (char *)"GENRE\0";
			*length = 5;
			break;

		case META_TRACK:
			pCategoryName = (char *)"TRACK\0";
			*length = 5;
			break;
		default :
			pCategoryName = (char *)"No Category\0";
			*length = 11;
			break;
	}

	return pCategoryName;
}

static CategoryType UI_META_NAVI_DBGEN_GetUpperCategory( unsigned int DiskType, CategoryType eCurrnetType, unsigned int nDepth )
{
	CategoryType eUpperCategory;

	if ( UI_META_ROOTCATEGORYDEPTH + 1 < nDepth )
	{
		switch( eCurrnetType )
		{
			case META_GENRE:
				eUpperCategory = META_ALL;
				break;
			case META_ARTIST:
				eUpperCategory = META_GENRE;
				break;
			case META_ALBUM:
				eUpperCategory = META_ARTIST;
				break;
			case META_TRACK:
				eUpperCategory = META_ALBUM;
				break;

			default:
				eUpperCategory = META_ALL;
				break;
		}
	}
	else
	{
		eUpperCategory = META_ALL;
	}

	return eUpperCategory;
}

static CategoryType UI_META_NAVI_DBGEN_GetSubCategory( CategoryType eCurrnetType)
{
	CategoryType eSubCategory;
	
	switch( eCurrnetType )
	{
		case META_GENRE:
			eSubCategory = META_ARTIST;
			break;
		case META_ARTIST:
			eSubCategory = META_ALBUM;
			break;
		case META_ALBUM:
			eSubCategory = META_TRACK;
			break;
		default:
			eSubCategory = META_ARTIST;
			break;
	}

	return eSubCategory;
}

static void UI_META_NAVI_DBGEN_DrawMenuName( unsigned int DiskType, CategoryType eCategory )
{
	char *pName = NULL;
	unsigned int lNameLength;
	
	/*Get current menu name*/
	if ( META_ALL  == eCategory)
	{
		pName = (char *)"CATEGORY\0";
		lNameLength = 8;
	}
	else
	{
		pName = UI_META_NAVI_DBGEN_GetMenuCategoryName( eCategory, &lNameLength );
	}

	/*Send Current's folder name on screen*/
	if(NULL != _MetaCategoryMenuName )
	{
		_MetaCategoryMenuName(DiskType, (unsigned char)eCategory, pName);
	}
}

static int UI_META_NAVI_DBGEN_InitializeBrowseList(unsigned int DiskType)
{
	int nTotalNumber;
	int nErrorCode = 0;
	nTotalNumber = UI_META_NAVI_MAX;

	(void)TCDB_Meta_ResetBrowser(DiskType);
	(void) UI_META_NAVI_SetTotalItemNumber( DiskType, (unsigned int)nTotalNumber );
	(void) UI_META_NAVI_SetDepth( DiskType, 0 );
	(void) UI_META_NAVI_SetPageIndex( DiskType, 0 );

	if(NULL != _MetaCategoryIndexChange)
	{
		_MetaCategoryIndexChange(DiskType, (unsigned int)nTotalNumber, 0 );
	}

	return nErrorCode;
}

static int UI_META_NAVI_DBGEN_SelectParentItem(unsigned int DiskType)
{
	int nErrorCode = 0;

	DEBUG_TCDB_PRINTF("\n");

	if ( INVALID_INDEX== DiskType )
	{
		// iPod is not connected.
		nErrorCode = -1;
	}
	else
	{
		unsigned int lDepth = UI_META_NAVI_GetDepth( DiskType );
		CategoryType eCurrentCategory = (CategoryType)UI_META_NAVI_GetCurrentCategory( DiskType );
		CategoryType eUpperCategory = UI_META_NAVI_DBGEN_GetUpperCategory( DiskType, eCurrentCategory, lDepth );
		unsigned int nPageIndex = 0;
		unsigned int nListNumInPage = UI_META_NAVI_GetListNumInPage( DiskType );
		unsigned int lSavedListIndex = 0;

		if ( 0 < lDepth )
		{
			lDepth--;
			(void) UI_META_NAVI_SetDepth( DiskType, lDepth );
		}

		lSavedListIndex = UI_META_NAVI_GetSavedListIndex( DiskType, lDepth );
		if ( 0 < lSavedListIndex )
		{
			if(nListNumInPage != 0)
			{
				nPageIndex = (lSavedListIndex-1) / nListNumInPage;
				//lSavedListIndex = ((lSavedListIndex-1) % nListNumInPage) + 1;
			}
			else
			{
				nPageIndex = (lSavedListIndex-1);
			}
		}
		else
		{
			nPageIndex = 0;
			lSavedListIndex = 1;
		}

		if ( META_ALL != eCurrentCategory )
		{
			if ( META_ALL == eUpperCategory )
			{
				(void) UI_META_NAVI_SetCurrentCategory( DiskType, (unsigned int)eUpperCategory );
				(void) UI_META_NAVI_SetRootCategory( DiskType, (unsigned int)eUpperCategory );
				(void) UI_META_NAVI_SetDepth( DiskType, lDepth );
				(void) UI_META_NAVI_SetPageIndex( DiskType, nPageIndex );
				(void) UI_META_NAVI_SetTotalItemNumber( DiskType, UI_META_NAVI_MAX );
				if(NULL != _MetaCategoryIndexChange)
				{
					_MetaCategoryIndexChange(DiskType,(unsigned int)UI_META_NAVI_MAX, lSavedListIndex-1  );
				}				
				(void) UI_META_NAVI_DBGEN_DrawList(DiskType);
			}
			else
			{

				nErrorCode = TCDB_Meta_UndoCategory(DiskType);
				if ( DB_SUCCESS == nErrorCode )
				{
					long nTotalNumber = 0;
					nErrorCode = TCDB_Meta_GetNumberCategory(DiskType,(const char *)UI_META_NAVI_DBGEN_GetDBName(eUpperCategory));
					if ( 0 < nErrorCode )
					{
						nTotalNumber = nErrorCode;
						nErrorCode = DB_SUCCESS;
						(void) UI_META_NAVI_SetCurrentCategory( DiskType, (unsigned int)eUpperCategory );
						(void) UI_META_NAVI_SetDepth( DiskType, lDepth );
						(void) UI_META_NAVI_SetPageIndex( DiskType, nPageIndex );
						if(UI_META_NAVI_DBGEN_IsSelectAllCategory(eUpperCategory) != 0)
						{
							(void) UI_META_NAVI_SetTotalItemNumber( DiskType, (unsigned int)(nTotalNumber + ADD_LIST_NUM)/*Plus one for 'Select all*/ );

							if(NULL != _MetaCategoryIndexChange)
							{
								_MetaCategoryIndexChange(DiskType,(unsigned int)(nTotalNumber + ADD_LIST_NUM), lSavedListIndex-1 );
							}
						}
						else
						{
							(void) UI_META_NAVI_SetTotalItemNumber( DiskType, (unsigned int)(nTotalNumber + NORMAL_LIST_NUM) );

							if(NULL != _MetaCategoryIndexChange)
							{
								_MetaCategoryIndexChange(DiskType,(unsigned int)(nTotalNumber + NORMAL_LIST_NUM), lSavedListIndex-1 );
							}
						}
						(void) UI_META_NAVI_DBGEN_DrawList(DiskType);
					}
				}
			}

		}
	}

	return nErrorCode;
}


static int UI_META_NAVI_DBGEN_SelectChildItem( unsigned int DiskType, int nSelectIndex )
{
	int nErrorCode = 0;
	unsigned int lTotalItemNumber = UI_META_NAVI_GetTotalItemNumber( DiskType );

	DEBUG_TCDB_PRINTF("select index (%d)\n",nSelectIndex);
	if ( INVALID_INDEX== DiskType )
	{
		// iPod is not connected.

		nErrorCode = -1;
	}
	else if ( nSelectIndex < 1 )
	{
		// Invalid parameter

		nErrorCode = -2;
	}
	else if ( lTotalItemNumber < (unsigned int)nSelectIndex )
	//else if ( (1/* Select all */==nTotalItemNumber) || (nTotalItemNumber < nSelectIndex) )
	{
		// Invalid parameter
		nErrorCode = -3;
	}
	else
	{
		//pIPOD_PLAY_INFO pInfo = (pIPOD_PLAY_INFO)IPOD_GetExtInfo(pHandle);
		unsigned int lDepth = UI_META_NAVI_GetDepth( DiskType );

		{
			CategoryType eCurrentCategory = (CategoryType)UI_META_NAVI_GetCurrentCategory( DiskType );

			if ( META_ALL == eCurrentCategory )	
			{
				long nTotalNumber = 0;
				nErrorCode = TCDB_Meta_ResetBrowser(DiskType);

				if( DB_SUCCESS== nErrorCode )
				{
					eCurrentCategory = UI_META_NAVI_DBGEN_GetBrowserTopCategory( (unsigned int)(nSelectIndex-1) );

					(void) UI_META_NAVI_SetCurrentCategory( DiskType, (unsigned int)eCurrentCategory );

					(void) UI_META_NAVI_SetRootCategory( DiskType, (unsigned int)eCurrentCategory );

					nErrorCode = TCDB_Meta_GetNumberCategory(DiskType,(const char *)UI_META_NAVI_DBGEN_GetDBName(eCurrentCategory));
					if( 0 < nErrorCode )
					{
						nTotalNumber = nErrorCode;
						nErrorCode = DB_SUCCESS;
					}
					else
					{
						nErrorCode =-1;
						nTotalNumber = 0;						
					}
				}

				if( DB_SUCCESS == nErrorCode )
				{
					(void) UI_META_NAVI_SetSavedListIndex( DiskType, lDepth, (unsigned int)nSelectIndex );
					(void) UI_META_NAVI_SetDepth( DiskType, (lDepth+1) );
					(void) UI_META_NAVI_SetPageIndex( DiskType, 0 );
					if(UI_META_NAVI_DBGEN_IsSelectAllCategory(eCurrentCategory) != 0)
					{
						(void) UI_META_NAVI_SetTotalItemNumber( DiskType, (unsigned int)(nTotalNumber + ADD_LIST_NUM)/*Plus one for 'Select all*/ );

						if(NULL != _MetaCategoryIndexChange)
						{
							_MetaCategoryIndexChange(DiskType,(unsigned int)(nTotalNumber + ADD_LIST_NUM), 0 );
						}
					}
					else
					{
						if(nTotalNumber == 0)
						{
							nErrorCode =-1;
							nTotalNumber = 1; /* for display "No File" */
						}
						(void) UI_META_NAVI_SetTotalItemNumber( DiskType, (unsigned int)(nTotalNumber + NORMAL_LIST_NUM) );

						if(NULL != _MetaCategoryIndexChange)
						{
							_MetaCategoryIndexChange(DiskType,(unsigned int)(nTotalNumber + NORMAL_LIST_NUM), 0 );
						}						
					}
					(void) UI_META_NAVI_DBGEN_DrawList(DiskType);
				}
			}
			else
			{
				CategoryType eCategory;
				{
					(void) UI_META_NAVI_SetSavedListIndex( DiskType, lDepth, (unsigned int)nSelectIndex );

					if (META_TRACK == eCurrentCategory )
					{
					//	nErrorCode = IPOD_PlayControl( pHandle, STOP );
					#if 0
						if ( UI_META_SELECTALL_INDEX == nSelectIndex )
						{
							nErrorCode = IPOD_SelectSortList( pHandle, eCurrentCategory, 0, eSortOrder/*SORTBYARTIST*/ );
						}
					#endif
					}

					/* Somtimes, PlayContol command returns command timeout on iPod classic.*/
					if(DB_SUCCESS== nErrorCode) 
					{
						if(UI_META_NAVI_DBGEN_IsSelectAllCategory(eCurrentCategory) != 0)
						{
							if ( UI_META_SELECTALL_INDEX < nSelectIndex )
							{
								nErrorCode = TCDB_Meta_SelectCategory(DiskType,(const char *)UI_META_NAVI_DBGEN_GetDBName(eCurrentCategory), (nSelectIndex-(2+UI_META_BACK_INDEX)));
							}
						}
						else
						{
							//if( 0 < nSelectIndex ) /* Always True */
							{
								/* PlayCurrentSelection is deprecated function. but, it is used for iPod compatible issue */
								if( META_TRACK == eCurrentCategory )
								{
									nErrorCode = TCDB_Meta_MakePlayList(DiskType, nSelectIndex-(1+UI_META_BACK_INDEX));
								}
								else
								{
									nErrorCode = TCDB_Meta_SelectCategory(DiskType,(const char *)UI_META_NAVI_DBGEN_GetDBName(eCurrentCategory), (nSelectIndex-(1+UI_META_BACK_INDEX)));
								}
							}
						}
					}

					if(DB_SUCCESS == nErrorCode) 
					{
						if ( META_TRACK == eCurrentCategory)
						{
							(void) UI_META_NAVI_SetTotalItemNumber( DiskType, lTotalItemNumber); /* UI_META_NAVI_GetMaxBrowsingListNumber( pPlayback->mCurrentDisk ) */
							//UI_DRV_SetTotalMusicFileNum(pPlayback, (U16)lTotalItemNumber);
							UI_META_NAVI_StartPlay(DiskType,nSelectIndex-(1+UI_META_BACK_INDEX));
						}
						else
						{
							long nTotalNumber = 0;
							eCategory = UI_META_NAVI_DBGEN_GetSubCategory( eCurrentCategory );
							nErrorCode = TCDB_Meta_GetNumberCategory(DiskType,(const char *) UI_META_NAVI_DBGEN_GetDBName(eCategory));
							if(0 < nErrorCode )
							{	nTotalNumber = nErrorCode;
								nErrorCode = DB_SUCCESS;
							}
							if ( DB_SUCCESS == nErrorCode )
							{
								(void) UI_META_NAVI_SetCurrentCategory( DiskType, (unsigned int)eCategory );

								(void) UI_META_NAVI_SetDepth( DiskType, (lDepth+1) );
								(void) UI_META_NAVI_SetPageIndex( DiskType, 0 );
								if(UI_META_NAVI_DBGEN_IsSelectAllCategory(eCategory) != 0)
								{
									(void) UI_META_NAVI_SetTotalItemNumber( DiskType, (unsigned int)(nTotalNumber + ADD_LIST_NUM)/*Plus one for 'Select all*/ );

									if(NULL != _MetaCategoryIndexChange)
									{
										_MetaCategoryIndexChange(DiskType,(unsigned int)(nTotalNumber + ADD_LIST_NUM), 0 );
									}		
								}
								else
								{
									if(nTotalNumber == 0)
									{
										nTotalNumber = 1; /* for display "No File" */
									}
									(void) UI_META_NAVI_SetTotalItemNumber( DiskType, (unsigned int)(nTotalNumber + NORMAL_LIST_NUM) );		

									if(NULL != _MetaCategoryIndexChange)
									{
										_MetaCategoryIndexChange(DiskType,(unsigned int)(nTotalNumber + NORMAL_LIST_NUM), 0 );
									}								
								}
								(void) UI_META_NAVI_DBGEN_DrawList(DiskType);
							}
						}
					}
				}
			}

		}
	}

	return nErrorCode;
}

static int UI_META_NAVI_DBGEN_DrawList( unsigned int DiskType )
{
	int nErrorCode = 0;
	CategoryType eCategory;
	CategoryType eCurrentCategory;
	unsigned int zListIndex = 0;
	unsigned int lNameLength = 0;
	char *pNameString = NULL;
	unsigned short index;
	unsigned int lPageIndex = UI_META_NAVI_GetPageIndex( DiskType );
	unsigned int lListNumInPage = UI_META_NAVI_GetListNumInPage( DiskType );
	unsigned int lTotalItemNumber = UI_META_NAVI_GetTotalItemNumber( DiskType );
	DBGEN_RECORD_TYPE *pstRecordType = NULL;
	
	eCurrentCategory = (CategoryType)UI_META_NAVI_GetCurrentCategory( DiskType );

	// Draw menu title
	UI_META_NAVI_DBGEN_DrawMenuName( DiskType, eCurrentCategory );
	
	
	/*Set the initial value*/
	index = (unsigned short)(lPageIndex*lListNumInPage);	/* starting ARRAY index( 0 ~ )*/

	if((DiskType != INVALID_INDEX) && (META_ALL != eCurrentCategory)) 
	{
		int zStartIndex = 0;
		int nRequestNumber = 0;

		if(UI_META_NAVI_DBGEN_IsSelectAllCategory(eCurrentCategory) != 0)
		{
			if (0 == index)
			{
				zStartIndex = (int)index;
				if ( lTotalItemNumber < lListNumInPage )
				{
					nRequestNumber = (int)(lTotalItemNumber-ADD_LIST_NUM);
				}
				else
				{
					nRequestNumber = (int)(lListNumInPage-ADD_LIST_NUM);
				}
			}
			else
			{
				zStartIndex = (int)(index-ADD_LIST_NUM);
				if( lTotalItemNumber < ((lPageIndex+1)*lListNumInPage))
				{
					nRequestNumber = (int)(lTotalItemNumber - index);
				}
				else
				{
					nRequestNumber = (int)lListNumInPage;
				}
			}
		}
		else
		{
			{
				zStartIndex = (int)(index-NORMAL_LIST_NUM);
				if ( lTotalItemNumber < ((lPageIndex+1)*lListNumInPage) )
				{
					nRequestNumber = (int)(lTotalItemNumber - index);
				}
				else
				{
					nRequestNumber = (int)lListNumInPage;
				}
			}
		}

		if(nRequestNumber > 0)
		{
			pstRecordType = (DBGEN_RECORD_TYPE *)malloc(sizeof(DBGEN_RECORD_TYPE)*nRequestNumber);
			memset(pstRecordType, 0x00, sizeof(DBGEN_RECORD_TYPE)*nRequestNumber);
			if(pstRecordType != NULL)
			{
				int readIndex, i=0;
				DEBUG_TCDB_PRINTF("start Index (%d), Request Number (%d)\n", zStartIndex, nRequestNumber);
				for(readIndex = zStartIndex; readIndex <(nRequestNumber+zStartIndex);readIndex++)
				{
					DBGEN_RECORD_TYPE *pRecordType = &pstRecordType[i];
					nErrorCode = TCDB_Meta_GetCategoryListName(DiskType,(const char *) UI_META_NAVI_DBGEN_GetDBName(eCurrentCategory), readIndex, (pRecordType->Data),MAX_TAG_LENGTH);
					i++;
					if( DB_SUCCESS != nErrorCode)
					{
						break;
					}
				}
			}
		}
	}

	if(nErrorCode == DB_SUCCESS)
	{
		int i = 0;
		for(zListIndex=0 ; zListIndex<lListNumInPage ; zListIndex++)
		{
			if( ((index+zListIndex) >= lTotalItemNumber) || (nErrorCode != DB_SUCCESS) )
			{
				break;
			}

			/*Display file icon and get file's information*/			
			{
				if ( META_ALL == eCurrentCategory )
				{
					eCategory = UI_META_NAVI_DBGEN_GetBrowserTopCategory( index+zListIndex );
					pNameString = UI_META_NAVI_DBGEN_GetCategoryName( eCategory, &lNameLength );
				}
				else
				{
					if(UI_META_NAVI_DBGEN_IsSelectAllCategory(eCurrentCategory) != 0)
					{
						if( ALL_INDEX_NUM == (index+zListIndex) )
						{
							lNameLength = 11;
							pNameString = (char *)"Select All\0";
						}
						else
						{
							DBGEN_RECORD_TYPE *pRecordType = &pstRecordType[i];
							lNameLength = strlen(pRecordType->Data);
							pNameString = pRecordType->Data;
							i++;
						}
					}
					else
					{
						DBGEN_RECORD_TYPE *pRecordType = &pstRecordType[i];
						lNameLength = strlen(pRecordType->Data);
						pNameString = pRecordType->Data;
						i++;
					}
				}
			}			

			if( nErrorCode == DB_SUCCESS )
			{
				if ( NULL == pNameString )	
				{
					lNameLength = 8;
					pNameString = (char *)"No name\0";
				}

				if( NULL != _MetaCategoryName )
				{
					_MetaCategoryName(DiskType,(index+zListIndex), pNameString);
					DEBUG_TCDB_PRINTF("CategoryName : index (%d), Name (%s)\n",(index+zListIndex), pNameString);
				}
			}
		}
		if( NULL != _MetaCategoryNameCommit)
		{
			_MetaCategoryNameCommit(DiskType);
		}
	}

	if(pstRecordType != NULL)
	{
		free(pstRecordType);
	}

	return nErrorCode;
}


#ifdef __cplusplus
}
#endif

/* End Of File */

