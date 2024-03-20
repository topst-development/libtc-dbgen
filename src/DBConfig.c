/****************************************************************************************
 *   FileName    : DBconfig.c
 *   Description : make db config
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
#include <stdio.h>
#include <string.h>
#include <linux/types.h>
#include <libxml/parser.h>
#include <stdint.h>
#include "TCDBDefine.h"
#include "TCUtil.h"
#include "DBConfig.h"

static struct supportExtList *CreateSupportExtList(const char *extName)
{
	struct supportExtList * newList = (struct supportExtList *)malloc (sizeof(struct supportExtList));
	if(newList != NULL)
	{
		newList->_pExtName = calloc(1,strlen(extName)+(uint32_t)1);
		if(newList->_pExtName != NULL)
		{
			(void)strncpy(newList->_pExtName, extName, strlen(extName));
			newList->_pNextExt = NULL;
		}
		else
		{
			free(newList);
			newList = NULL;
		}
	}
	return newList;
}

static void DesteorySupportExtList(struct supportExtList *extList)
{
	struct supportExtList *curList, *nextList;

	curList =extList;
	while(curList != NULL)
	{
		if(curList->_pExtName != NULL)
		{
			free(curList->_pExtName);
			curList->_pExtName  = NULL;
		}
		nextList = curList->_pNextExt ;
		free(curList);
		curList = nextList;
	}
}

static int32_t AddSupportExtList(struct supportExtList ** extList, char *extName)
{
	int32_t ret =-1;
	if(extName!=NULL)
	{
		if((*extList) == NULL)
		{
			*extList = CreateSupportExtList((const char *)extName);
			if(*extList != NULL)
			{
				ret =0;
			}
		}
		else
		{
			struct supportExtList *tail = (*extList);
			while(tail->_pNextExt!=NULL)
			{
				tail = tail->_pNextExt;
			}
			tail->_pNextExt = CreateSupportExtList((const char *)extName);
			if(tail->_pNextExt != NULL)
			{
				ret =0;
			}
		}

	}
	return ret;
}

int32_t FindSupportExtName(struct supportExtList * extList, const char * extName)
{
	int32_t ret = -1;

	if((extList != NULL)&&(extName != NULL))
	{
		ret =0;
		struct supportExtList *newList = extList;
		while(newList != NULL)
		{
			if (strcasecmp (newList->_pExtName, extName) == 0)
			{
				ret =1;
				break;
			}
			newList =  newList->_pNextExt;
		}
	}
	return ret;
}

static DBTagList *CreateTagList(char *tagName)
{
	DBTagList *newTag=(DBTagList *)malloc(sizeof(DBTagList));

	if(newTag != NULL)
	{
		newTag->_pTagName = calloc(1, strlen(tagName)+(uint32_t)1);
		if(newTag->_pTagName != NULL)
		{
			(void)strncpy(newTag->_pTagName, tagName, strlen(tagName));
			newTag->_pNextList = NULL;
		}
		else
		{
			free(newTag);
			newTag = NULL;
		}
	}

	return newTag;
}

static void DesteoryTagList(DBTagList *extList)
{
	DBTagList *curList, *nextList;

	curList =extList;
	while(curList != NULL)
	{
		if(curList->_pTagName != NULL)
		{
			free(curList->_pTagName);
			curList->_pTagName  = NULL;
		}
		nextList = curList->_pNextList;
		free(curList);
		curList = nextList;
	}
}

static int32_t AddTagList(DBTagList ** tagList, char *tagName)
{
	int32_t ret =-1;
	if(tagName!=NULL)
	{
		if((*tagList) == NULL)
		{
			*tagList = CreateTagList(tagName);
			if(*tagList != NULL)
			{
				ret =0;
			}
		}
		else
		{
			DBTagList *tail = (*tagList);
			while(tail->_pNextList!=NULL)
			{
				tail = tail->_pNextList;
			}
			tail->_pNextList = CreateTagList(tagName);
			if(tail->_pNextList != NULL)
			{
				ret =0;
			}
		}

	}
	return ret;
}

static BlackListDir *CreateBlackList(const char *dirName)
{
	BlackListDir * newList = (BlackListDir *)malloc (sizeof(BlackListDir));
	if(newList != NULL)
	{
		newList->_pDirName = calloc(1,strlen(dirName)+(uint32_t)1);
		if(newList->_pDirName != NULL)
		{
			(void)strncpy(newList->_pDirName, dirName, strlen(dirName));
			newList->_pNextList = NULL;
		}
		else
		{
			free(newList);
			newList = NULL;
		}
	}
	return newList;
}

static void DesteoryBlackList(BlackListDir *blackList)
{
	BlackListDir *curList, *nextList;

	curList =blackList;
	while(curList != NULL)
	{
		if(curList->_pDirName != NULL)
		{
			free(curList->_pDirName);
			curList->_pDirName  = NULL;
		}
		nextList = curList->_pNextList ;
		free(curList);
		curList = nextList;
	}
}

static int32_t AddBlackList(BlackListDir ** blackList, const char *dirName)
{
	int32_t ret =-1;
	if(dirName!=NULL)
	{
		if((*blackList) == NULL)
		{
			*blackList = CreateBlackList(dirName);
			if(*blackList != NULL)
			{
				ret =0;
			}
		}
		else
		{
			BlackListDir *tail = (*blackList);
			while(tail->_pNextList!=NULL)
			{
				tail = tail->_pNextList;
			}
			tail->_pNextList = CreateBlackList(dirName);
			if(tail->_pNextList != NULL)
			{
				ret =0;
			}
		}

	}
	return ret;
}

int32_t FindBlackListDirName(BlackListDir * blackList, const char * dirName)
{
	int32_t ret = -1;

	if((blackList != NULL)&&(dirName != NULL))
	{
		ret =0;
		BlackListDir *newList = blackList;
		while(newList != NULL)
		{
			if (strcasecmp (newList->_pDirName, dirName) == 0)
			{
				ret =1;
				break;
			}
			newList =  newList->_pNextList;
		}
	}
	return ret;
}

int32_t defautlConfig(DB_Config *dbConfig)
{
	int32_t ret =-1;
	if(dbConfig != NULL)
	{
		dbConfig->_dirName = newformat("/home/root");
		if(dbConfig->_dirName != NULL)
		{
			dbConfig->_maxDBNum = 4;
			dbConfig->_maxFolderNum = 2000;

			dbConfig->_audioTableInfo._maxFileCount = 1000;
			dbConfig->_audioTableInfo._pSupportList = CreateSupportExtList(".mp3");

			dbConfig->_videoTableInfo._maxFileCount = 100;
			dbConfig->_videoTableInfo._pSupportList =CreateSupportExtList(".avi");;

			dbConfig->_imageTableInfo._maxFileCount =0;
			dbConfig->_imageTableInfo._pSupportList = NULL;

			dbConfig->_fileUpdateCount =0;
			dbConfig->_metaUpdateCount =0;

			dbConfig->_pBlackList = CreateBlackList("$RECYCLE.BIN");
			ret = DB_SUCCESS;
		}
		else
		{
			ret = DB_MEMORY_ALLOC_ERR;
		}
	}

	return ret;
}

int32_t loadConfig(const char *fileName, DB_Config *dbConfig)
{
	int32_t ret = -1;
	xmlDoc         *document;
	xmlNode        *root, *node;

	if((fileName != NULL)&&(dbConfig != NULL))
	{
		document = xmlReadFile(fileName, NULL, 0);
		if(document != NULL)
		{
			ret =0;
			root = xmlDocGetRootElement(document);

			dbConfig->_maxFolderNum = 2000;
			dbConfig->_fileUpdateCount = 0;
			dbConfig->_metaUpdateCount = 0;

			for(node = root->children; node!= NULL; node = node->next)
			{
				if(xmlStrcmp(node->name, (const xmlChar *) "DBConf") == 0)
				{
					xmlNode *dbConfNode;
					xmlChar *maxfolder, *updateCount;

					dbConfNode = node;
					maxfolder = xmlGetProp(dbConfNode, (const xmlChar *)"MaxFolder");
					if(maxfolder != NULL)
					{
						int32_t readCount = atoi((const char *)maxfolder);
						if(readCount > -1)
						{
							dbConfig->_maxFolderNum = (uint32_t)readCount;
						}
						else
						{
							dbConfig->_maxFolderNum = 0;
						}

						xmlFree(maxfolder);
					}

					updateCount = xmlGetProp(dbConfNode, (const xmlChar *)"FileUpdateCount");
					if(updateCount != NULL)
					{
						int32_t readCount = atoi((const char *)updateCount);
						if(readCount > -1)
						{
							dbConfig->_fileUpdateCount = (uint32_t)readCount;
						}
						else
						{
							dbConfig->_fileUpdateCount = 0;
						}
						xmlFree(maxfolder);
					}

					updateCount = xmlGetProp(dbConfNode, (const xmlChar *)"MetaUpdateCount");
					if(updateCount != NULL)
					{
						int32_t readCount = atoi((const char *)updateCount);
						if(readCount > -1)
						{
							dbConfig->_metaUpdateCount = (uint32_t)readCount;
						}
						else
						{
							dbConfig->_metaUpdateCount = 0;
						}
						xmlFree(updateCount);
					}
				}

				if(xmlStrcmp(node->name, (const xmlChar *) "AudioFileTable") == 0)
				{
					xmlNode *filetable;

					dbConfig->_audioTableInfo._maxFileCount  = 6000;
					for(filetable = node->xmlChildrenNode; filetable != NULL; filetable = filetable->next)
					{

						if(xmlStrcmp(filetable->name, (const xmlChar *) "Config") == 0)
						{
							xmlChar *maxfile;
							maxfile  = xmlGetProp(filetable, (const xmlChar *)"MaxFile");
							if(maxfile != NULL)
							{
								int32_t readCount = atoi((const char *)maxfile);

								if(readCount > -1)
								{
									dbConfig->_audioTableInfo._maxFileCount = (uint32_t)readCount;
								}
								else
								{
									dbConfig->_audioTableInfo._maxFileCount = 0;
								}
								xmlFree(maxfile);
							}
						}

						if(xmlStrcmp(filetable->name, (const xmlChar *) "SupportList") == 0)
						{
							xmlChar *ext_name;
							xmlNode *extnode;

							for(extnode = filetable->xmlChildrenNode; extnode != NULL; extnode = extnode->next)
							{
								if(xmlStrcmp(extnode->name, (const xmlChar *) "Ext") == 0)
								{
									ext_name  = xmlGetProp(extnode, (const xmlChar *)"Name");
									if(ext_name != NULL)
									{
										if(AddSupportExtList(&dbConfig->_audioTableInfo._pSupportList, (char *)ext_name) != 0)
										{
											DEBUG_TCDB_PRINTF("Add Support List error to AudioTableInfo\n");
										}
										xmlFree(ext_name);
									}
								}
							}
						}
					}
				}

				if(xmlStrcmp(node->name, (const xmlChar *) "VideoFileTable") == 0)
				{
					xmlNode *filetable;

					dbConfig->_videoTableInfo._maxFileCount =0;

					for(filetable = node->xmlChildrenNode; filetable != NULL; filetable = filetable->next)
					{
						if(xmlStrcmp(filetable->name, (const xmlChar *) "Config") == 0)
						{
							xmlChar *maxfile;
							maxfile  = xmlGetProp(filetable, (const xmlChar *)"MaxFile");
							if(maxfile != NULL)
							{
								int32_t getCount = atoi((const char *)maxfile);
								if(getCount < 0)
								{
									dbConfig->_videoTableInfo._maxFileCount = 0;
								}
								else
								{
									dbConfig->_videoTableInfo._maxFileCount = (uint32_t)getCount;
								}
								xmlFree(maxfile);
							}
						}

						if(xmlStrcmp(filetable->name, (const xmlChar *) "SupportList") == 0)
						{
							xmlChar *ext_name;
							xmlNode *extnode;

							for(extnode = filetable->xmlChildrenNode; extnode != NULL; extnode = extnode->next)
							{
								if(xmlStrcmp(extnode->name, (const xmlChar *) "Ext") == 0)
								{
									ext_name  = xmlGetProp(extnode, (const xmlChar *)"Name");
									if(ext_name != NULL)
									{
										if(AddSupportExtList(&dbConfig->_videoTableInfo._pSupportList, (char *)ext_name) != 0)
										{
											ERROR_TCDB_PRINTF("Add Support List error to AudioTableInfo\n");
										}
										xmlFree(ext_name);
									}
								}
							}
						}
					}
				}

				if(xmlStrcmp(node->name, (const xmlChar *) "ImageFileTable") == 0)
				{
					xmlNode *filetable;

					dbConfig->_imageTableInfo._maxFileCount = 0;

					for(filetable = node->xmlChildrenNode; filetable != NULL; filetable = filetable->next)
					{
						if(xmlStrcmp(filetable->name, (const xmlChar *) "Config") == 0)
						{
							xmlChar *maxfile;
							maxfile  = xmlGetProp(filetable, (const xmlChar *)"MaxFile");
							if(maxfile != NULL)
							{
								int32_t readCount = atoi((const char *)maxfile);
								if(readCount > -1)
								{
									dbConfig->_imageTableInfo._maxFileCount = (uint32_t)readCount;
								}
								else
								{
									dbConfig->_imageTableInfo._maxFileCount = 0;
								}
								xmlFree(maxfile);
							}
						}

						if(xmlStrcmp(filetable->name, (const xmlChar *) "SupportList") == 0)
						{
							xmlChar *ext_name;
							xmlNode *extnode;

							for(extnode = filetable->xmlChildrenNode; extnode != NULL; extnode = extnode->next)
							{
								if(xmlStrcmp(extnode->name, (const xmlChar *) "Ext") == 0)
								{
									ext_name  = xmlGetProp(extnode, (const xmlChar *)"Name");
									if(ext_name != NULL)
									{
										if(AddSupportExtList(&dbConfig->_imageTableInfo._pSupportList, (char *)ext_name) != 0)
										{
											ERROR_TCDB_PRINTF("Add Support List error to AudioTableInfo\n");
										}
										xmlFree(ext_name);
									}
								}
							}
						}
					}
				}
				if(xmlStrcmp(node->name, (const xmlChar *) "MetaTable") == 0)
				{	xmlNode *metatable;
					for(metatable = node->xmlChildrenNode; metatable != NULL; metatable = metatable->next)
					{
						xmlChar *tag;
						if(xmlStrcmp(metatable->name, (const xmlChar *) "tag") == 0)
						{
							tag  = xmlGetProp(metatable, (const xmlChar *)"name");
							if(tag != NULL)
							{
								(void)AddTagList(&dbConfig->_pMetaTable, (char *)tag);
								xmlFree(tag);
							}
						}
					}
				}
				if(xmlStrcmp(node->name, (const xmlChar *) "BlackListTable") == 0)
				{
					xmlNode *blacklist;

					for(blacklist = node->xmlChildrenNode; blacklist != NULL; blacklist = blacklist->next)
					{
						if(xmlStrcmp(blacklist->name, (const xmlChar *) "BlackList") == 0)
						{
							xmlChar *dir_name;
							xmlNode *dirnode;

							for(dirnode = blacklist->xmlChildrenNode; dirnode != NULL; dirnode = dirnode->next)
							{
								if(xmlStrcmp(dirnode->name, (const xmlChar *) "Dir") == 0)
								{
									dir_name  = xmlGetProp(dirnode, (const xmlChar *)"Name");
									if(dir_name != NULL)
									{
										if(AddBlackList(&dbConfig->_pBlackList, (const char *)dir_name) != 0)
										{
											ERROR_TCDB_PRINTF("Add Black List error\n");
										}
										xmlFree(dir_name);
									}
								}
							}
						}
					}
				}
			}

			xmlFreeDoc(document);
		}
	}
	return ret;
}

void releaseConfig(DB_Config *dbConfig)
{
	if(dbConfig != NULL)
	{
		if(dbConfig->_dirName != NULL)
		{
			free((char *)dbConfig->_dirName);
			dbConfig->_dirName  = NULL;
		}

		DesteorySupportExtList(dbConfig->_audioTableInfo._pSupportList);
		DesteorySupportExtList(dbConfig->_videoTableInfo._pSupportList);
		DesteorySupportExtList(dbConfig->_imageTableInfo._pSupportList);

		if(dbConfig->_pMetaTable != NULL)
		{
			DesteoryTagList(dbConfig->_pMetaTable);
			dbConfig->_pMetaTable = NULL;
		}

		DesteoryBlackList(dbConfig->_pBlackList);
	}
}

void printfConfig(DB_Config *dbConfig)
{
	if(dbConfig != NULL)
	{
		INFO_TCDB_PRINTF("==================== DB Table Info ====================\n");
		INFO_TCDB_PRINTF("DB path : %s \n", dbConfig->_dirName);
		INFO_TCDB_PRINTF("Max DB Number : %d \n",  dbConfig->_maxDBNum);
		INFO_TCDB_PRINTF("Max Folder Number : %d \n", dbConfig->_maxFolderNum);
		INFO_TCDB_PRINTF("File Update Counter : %d, Meta Update Count %d \n", dbConfig->_fileUpdateCount, dbConfig->_metaUpdateCount);

		INFO_TCDB_PRINTF("Audio Table Info\n");
		INFO_TCDB_PRINTF("\tMax File Number : %d \n", dbConfig->_audioTableInfo._maxFileCount);
		if(dbConfig->_audioTableInfo._pSupportList != NULL)
		{
			struct supportExtList *pSupportList = dbConfig->_audioTableInfo._pSupportList;

			INFO_TCDB_PRINTF("\tSupport List\n");
			while(pSupportList != NULL)
			{
				INFO_TCDB_PRINTF("\t\textension: %s \n", pSupportList->_pExtName);
				pSupportList = pSupportList->_pNextExt;
			}
		}

		INFO_TCDB_PRINTF("Video Table Info\n");
		INFO_TCDB_PRINTF("\tMax File Number : %d \n", dbConfig->_videoTableInfo._maxFileCount);
		if(dbConfig->_videoTableInfo._pSupportList != NULL)
		{
			struct supportExtList *pSupportList = dbConfig->_videoTableInfo._pSupportList;
			INFO_TCDB_PRINTF("\tSupport List\n");
			while(pSupportList != NULL)
			{
				INFO_TCDB_PRINTF("\t\textension: %s \n", pSupportList->_pExtName);
				pSupportList = pSupportList->_pNextExt;
			}
		}

		INFO_TCDB_PRINTF("Image Table Info\n");
		INFO_TCDB_PRINTF("\tMax File Number : %d \n", dbConfig->_imageTableInfo._maxFileCount);
		if(dbConfig->_videoTableInfo._pSupportList != NULL)
		{
			struct supportExtList *pSupportList = dbConfig->_imageTableInfo._pSupportList;
			INFO_TCDB_PRINTF("\tSupport List\n");
			while(pSupportList != NULL)
			{
				INFO_TCDB_PRINTF("\t\textension: %s \n", pSupportList->_pExtName);
				pSupportList = pSupportList->_pNextExt;
			}
		}

		if(dbConfig->_pMetaTable != NULL)
		{
			DBTagList *tagList = dbConfig->_pMetaTable;

			INFO_TCDB_PRINTF("Meta Table\n");
			while(tagList != NULL)
			{
				INFO_TCDB_PRINTF("\tGathering Tag : %s\n", tagList->_pTagName);
				tagList = tagList->_pNextList;
			}
		}
		else
		{
			;
		}

		if(dbConfig->_pBlackList != NULL)
		{
			BlackListDir *blackList = dbConfig->_pBlackList;

			INFO_TCDB_PRINTF("Black List Table\n");
			while(blackList != NULL)
			{
				INFO_TCDB_PRINTF("\tDir name : %s\n", blackList->_pDirName);
				blackList = blackList->_pNextList;
			}
		}
		INFO_TCDB_PRINTF("=======================================================\n");
	}
}

