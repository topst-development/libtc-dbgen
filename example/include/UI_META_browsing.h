/****************************************************************************************
 *   FileName    : UI_META_Browsing.h
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


/*****************************************************************************
******************************************************************************
*
*        File : UI_META_browsing.h
*
* Description :
*
*      Author : 
*
*      Status :
*
*     History :
*               Preliminary release 2004-4-07
*    
******************************************************************************
******************************************************************************/

#ifndef __UI_META_NAVIGATION_H__
#define __UI_META_NAVIGATION_H__

#if defined(__cplusplus)
extern "C"
{
#endif

#define UI_META_NAVI_EXTERN	extern

typedef enum
{
	META_INVALID_CATEGORY = -1,
	META_ALL, /*Top-level*/
	META_PLAYLIST,
	META_ARTIST,
	META_ALBUM,
	META_GENRE,
	META_TRACK
} CategoryType;/*Database category types for commands*/

typedef void (*MetaCategoryMenuName_cb)( unsigned int dbNum, unsigned char mode, const char *pNenu);
typedef void (*MetaCategoryIndexChange_cb)(unsigned int dbNum, unsigned int totalNum, unsigned int currentNum);
typedef void (*MetaCategoryName_cb)(unsigned int dbNum, unsigned short index, const char *pName);
typedef void (*MetaCategoryNameCommit_cb)(unsigned int dbNum);
typedef void (*MetaSelectTrack_cb)(unsigned int dbNum, unsigned int selectIndex);

typedef struct _TCDB_MetaBrwsEventCB{
	MetaCategoryMenuName_cb			_MetaCategoryMenuName;
	MetaCategoryIndexChange_cb		_MetaCategoryIndexChange;
	MetaCategoryName_cb				_MetaCategoryName;
	MetaCategoryNameCommit_cb		_MetaCategoryNameCommit;
	MetaSelectTrack_cb				_MetaSelectTrack;	
}TCDB_MetaBrwsEventCB;


void UI_META_SetEventCallbackFunctions(TCDB_MetaBrwsEventCB *cb);

UI_META_NAVI_EXTERN int UI_META_NAVI_ClearAllNaviInfo( void );		//Meta Brwosing Initialize 

UI_META_NAVI_EXTERN int UI_META_NAVI_AddNaviInfo( unsigned int  dbNum );
UI_META_NAVI_EXTERN int UI_META_NAVI_DeleteNaviInfo( unsigned int dbNum );		//Device Remove

UI_META_NAVI_EXTERN int UI_META_NAVI_StartBrowse(unsigned int  dbNum, unsigned int ListNumInPage);	//Meta Browsing Start and Restart
UI_META_NAVI_EXTERN void UI_META_NAVI_SelectChildItem(unsigned int  dbNum, unsigned int CurrentIndex);		
UI_META_NAVI_EXTERN void UI_META_NAVI_SelectParentItem(unsigned int  dbNum);
UI_META_NAVI_EXTERN void UI_META_NAVI_Move(unsigned int  dbNum, unsigned int CurrentIndex);
UI_META_NAVI_EXTERN void UI_META_NAVI_ExitBrowse(unsigned int  dbNum, unsigned int nSelectIndex);			// ex, mode change 
UI_META_NAVI_EXTERN int UI_META_NAVI_HomeBrowse(unsigned int  dbNum, unsigned int ListNumInPage);	// Go Home Menu


#define FIRST_SONG_INDEX (0x00)
#define FIRST_SONG  (0x01)

#if defined(DUAL_PLAYBACK_INCLUDE)
#define UI_META_NAVI_MAX_NUM	(2)
#else
#define UI_META_NAVI_MAX_NUM	(1)
#endif

#define UI_META_NAVI_MAX_DEPTH	(5)

#define UI_META_NAVI_ITEM_EMPTY_VALUE	(0xffff)

#if defined(__cplusplus)
};
#endif

#endif /* __UI_NAVIGATION_H__ */

/* End Of File */

