/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "smpd.h"

static void get_uuid(char *str)
{
#ifdef HAVE_WINDOWS_H
#if 0
    UUID guid;
    char *rpcstr;
    UuidCreate(&guid);
    UuidToString(&guid, &rpcstr);
    strcpy(str, rpcstr);
    RpcStringFree(&rpcstr);
#else
    UUID guid;
    UuidCreate(&guid);
    sprintf(str, "%08lX-%04X-%04x-%02X%02X-%02X%02X%02X%02X%02X%02X",
	guid.Data1, guid.Data2, guid.Data3,
	guid.Data4[0], guid.Data4[1], guid.Data4[2], guid.Data4[3],
	guid.Data4[4], guid.Data4[5], guid.Data4[6], guid.Data4[7]);
#endif
#elif defined(HAVE_CFUUIDCREATE)
    CFUUIDRef       myUUID;
    CFStringRef     myUUIDString;
    char            strBuffer[100];
    myUUID = CFUUIDCreate(kCFAllocatorDefault);
    myUUIDString = CFUUIDCreateString(kCFAllocatorDefault, myUUID);/* This is the safest way to obtain a C string from a CFString.*/
    CFStringGetCString(myUUIDString, str, SMPD_MAX_DBS_NAME_LEN, kCFStringEncodingASCII);
    CFRelease(myUUIDString);
#elif defined(HAVE_UUID_GENERATE)
    uuid_t guid;
    uuid_generate(guid);
    uuid_unparse(guid, str);
#else
    sprintf(str, "%X%X%X%X", rand(), rand(), rand(), rand());
#endif
}

#undef FCNAME
#define FCNAME "smpd_dbs_init"
int smpd_dbs_init()
{
    smpd_enter_fn(FCNAME);
#ifdef USE_WIN_MUTEX_PROTECT
    if (smpd_process.nInitDBSRefCount == 0)
    {
	smpd_process.hDBSMutex = CreateMutex(NULL, FALSE, NULL);
    }
#endif
    smpd_process.nInitDBSRefCount++;
    if (smpd_process.domain_name[0] == '\0')
    {
	/*get_uuid(smpd_process.domain_name);*/
	/* create a database for the domain to be used for global operations like name publishing */
	smpd_dbs_create(smpd_process.domain_name);
    }
    smpd_exit_fn(FCNAME);
    return SMPD_DBS_SUCCESS;
}

#undef FCNAME
#define FCNAME "smpd_dbs_finalize"
int smpd_dbs_finalize()
{
    smpd_database_node_t *pNode, *pNext;
    smpd_database_element_t *pElement;

    smpd_enter_fn(FCNAME);

    smpd_process.nInitDBSRefCount--;

    if (smpd_process.nInitDBSRefCount == 0)
    {
#ifdef USE_WIN_MUTEX_PROTECT
	WaitForSingleObject(smpd_process.hDBSMutex, INFINITE);
#endif

	pNode = smpd_process.pDatabase;
	while (pNode)
	{
	    pNext = pNode->pNext;

	    while (pNode->pData)
	    {
		pElement = pNode->pData;
		pNode->pData = pNode->pData->pNext;
		free(pElement);
	    }
	    free(pNode);

	    pNode = pNext;
	}

	smpd_process.pDatabase = NULL;
	smpd_process.pDatabaseIter = NULL;

#ifdef USE_WIN_MUTEX_PROTECT
	ReleaseMutex(smpd_process.hDBSMutex);
	CloseHandle(smpd_process.hDBSMutex);
	smpd_process.hDBSMutex = NULL;
#endif
    }

    smpd_exit_fn(FCNAME);
    return SMPD_DBS_SUCCESS;
}

#undef FCNAME
#define FCNAME "smpd_dbs_create"
int smpd_dbs_create(char *name)
{
    /*char guid_str[100];*/
    smpd_database_node_t *pNode, *pNodeTest;

    smpd_enter_fn(FCNAME);

#ifdef USE_WIN_MUTEX_PROTECT
    /* Lock */
    WaitForSingleObject(smpd_process.hDBSMutex, INFINITE);
#endif

    pNode = smpd_process.pDatabase;
    if (pNode)
    {
	while (pNode->pNext)
	    pNode = pNode->pNext;
	pNode->pNext = (smpd_database_node_t*)malloc(sizeof(smpd_database_node_t));
	pNode = pNode->pNext;
    }
    else
    {
	smpd_process.pDatabase = (smpd_database_node_t*)malloc(sizeof(smpd_database_node_t));
	pNode = smpd_process.pDatabase;
    }
    pNode->pNext = NULL;
    pNode->pData = NULL;
    pNode->pIter = NULL;
    do
    {
	/*
	sprintf(pNode->pszName, "%d", smpd_process.nNextAvailableDBSID);
	smpd_process.nNextAvailableDBSID++;
	*/
	get_uuid(pNode->pszName);
	if (pNode->pszName[0] == '\0')
	{
#ifdef USE_WIN_MUTEX_PROTECT
	    /* Unlock */
	    ReleaseMutex(smpd_process.hDBSMutex);
#endif
	    smpd_exit_fn(FCNAME);
	    return SMPD_DBS_FAIL;
	}
	pNodeTest = smpd_process.pDatabase;
	while (strcmp(pNodeTest->pszName, pNode->pszName) != 0)
	    pNodeTest = pNodeTest->pNext;
    } while (pNodeTest != pNode);
    strcpy(name, pNode->pszName);

#ifdef USE_WIN_MUTEX_PROTECT
    /* Unlock */
    ReleaseMutex(smpd_process.hDBSMutex);
#endif

    /* put a unique id in the kvs database */
    /*
    get_uuid(guid_str);
    smpd_dbs_put(name, PMI_KVS_ID_KEY, guid_str);
    */

    smpd_exit_fn(FCNAME);
    return SMPD_DBS_SUCCESS;
}

#undef FCNAME
#define FCNAME "smpd_dbs_create_name_in"
int smpd_dbs_create_name_in(char *name)
{
    /*char guid_str[100];*/
    smpd_database_node_t *pNode;

    smpd_enter_fn(FCNAME);

    if (strlen(name) < 1 || strlen(name) > SMPD_MAX_DBS_NAME_LEN)
    {
	smpd_exit_fn(FCNAME);
	return SMPD_DBS_FAIL;
    }

#ifdef USE_WIN_MUTEX_PROTECT
    /* Lock */
    WaitForSingleObject(smpd_process.hDBSMutex, INFINITE);
#endif

    /* Check if the name already exists */
    pNode = smpd_process.pDatabase;
    while (pNode)
    {
	if (strcmp(pNode->pszName, name) == 0)
	{
#ifdef USE_WIN_MUTEX_PROTECT
	    /* Unlock */
	    ReleaseMutex(smpd_process.hDBSMutex);
#endif
	    /*return SMPD_DBS_FAIL;*/
	    /* Empty database? */
	    smpd_exit_fn(FCNAME);
	    return SMPD_DBS_SUCCESS;
	}
	pNode = pNode->pNext;
    }

    pNode = smpd_process.pDatabase;
    if (pNode)
    {
	while (pNode->pNext)
	    pNode = pNode->pNext;
	pNode->pNext = (smpd_database_node_t*)malloc(sizeof(smpd_database_node_t));
	pNode = pNode->pNext;
    }
    else
    {
	smpd_process.pDatabase = (smpd_database_node_t*)malloc(sizeof(smpd_database_node_t));
	pNode = smpd_process.pDatabase;
    }
    pNode->pNext = NULL;
    pNode->pData = NULL;
    pNode->pIter = NULL;
    strcpy(pNode->pszName, name);
    
#ifdef USE_WIN_MUTEX_PROTECT
    /* Unlock */
    ReleaseMutex(smpd_process.hDBSMutex);
#endif

    /* put a unique id in the kvs database */
    /*
    get_uuid(guid_str);
    smpd_dbs_put(name, PMI_KVS_ID_KEY, guid_str);
    */

    smpd_exit_fn(FCNAME);
    return SMPD_DBS_SUCCESS;
}

#undef FCNAME
#define FCNAME "smpd_dbs_get"
int smpd_dbs_get(const char *name, const char *key, char *value)
{
    smpd_database_node_t *pNode;
    smpd_database_element_t *pElement;

    smpd_enter_fn(FCNAME);

#ifdef USE_WIN_MUTEX_PROTECT
    WaitForSingleObject(smpd_process.hDBSMutex, INFINITE);
#endif

    pNode = smpd_process.pDatabase;
    while (pNode)
    {
	if (strcmp(pNode->pszName, name) == 0)
	{
	    pElement = pNode->pData;
	    while (pElement)
	    {
		if (strcmp(pElement->pszKey, key) == 0)
		{
		    strcpy(value, pElement->pszValue);
#ifdef USE_WIN_MUTEX_PROTECT
		    ReleaseMutex(smpd_process.hDBSMutex);
#endif
		    smpd_exit_fn(FCNAME);
		    return SMPD_DBS_SUCCESS;
		}
		pElement = pElement->pNext;
	    }
	}
	pNode = pNode->pNext;
    }

#ifdef USE_WIN_MUTEX_PROTECT
    ReleaseMutex(smpd_process.hDBSMutex);
#endif

    smpd_exit_fn(FCNAME);
    return SMPD_DBS_FAIL;
}

#undef FCNAME
#define FCNAME "smpd_dbs_put"
int smpd_dbs_put(const char *name, const char *key, const char *value)
{
    smpd_database_node_t *pNode;
    smpd_database_element_t *pElement;

    smpd_enter_fn(FCNAME);

#ifdef USE_WIN_MUTEX_PROTECT
    WaitForSingleObject(smpd_process.hDBSMutex, INFINITE);
#endif

    pNode = smpd_process.pDatabase;
    while (pNode)
    {
	if (strcmp(pNode->pszName, name) == 0)
	{
	    pElement = pNode->pData;
	    while (pElement)
	    {
		if (strcmp(pElement->pszKey, key) == 0)
		{
		    strcpy(pElement->pszValue, value);
#ifdef USE_WIN_MUTEX_PROTECT
		    ReleaseMutex(smpd_process.hDBSMutex);
#endif
		    smpd_exit_fn(FCNAME);
		    return SMPD_DBS_SUCCESS;
		}
		pElement = pElement->pNext;
	    }
	    pElement = (smpd_database_element_t*)malloc(sizeof(smpd_database_element_t));
	    pElement->pNext = pNode->pData;
	    strcpy(pElement->pszKey, key);
	    strcpy(pElement->pszValue, value);
	    pNode->pData = pElement;
#ifdef USE_WIN_MUTEX_PROTECT
	    ReleaseMutex(smpd_process.hDBSMutex);
#endif
	    smpd_exit_fn(FCNAME);
	    return SMPD_DBS_SUCCESS;
	}
	pNode = pNode->pNext;
    }

#ifdef USE_WIN_MUTEX_PROTECT
    ReleaseMutex(smpd_process.hDBSMutex);
#endif

    smpd_exit_fn(FCNAME);
    return SMPD_DBS_FAIL;
}

#undef FCNAME
#define FCNAME "smpd_dbs_delete"
int smpd_dbs_delete(const char *name, const char *key)
{
    smpd_database_node_t *pNode;
    smpd_database_element_t *pElement, *pElementTrailer;

    smpd_enter_fn(FCNAME);

#ifdef USE_WIN_MUTEX_PROTECT
    WaitForSingleObject(smpd_process.hDBSMutex, INFINITE);
#endif

    pNode = smpd_process.pDatabase;
    while (pNode)
    {
	if (strcmp(pNode->pszName, name) == 0)
	{
	    pElementTrailer = pElement = pNode->pData;
	    while (pElement)
	    {
		if (strcmp(pElement->pszKey, key) == 0)
		{
		    if (pElementTrailer != pElement)
			pElementTrailer->pNext = pElement->pNext;
		    else
			pNode->pData = pElement->pNext;
		    free(pElement);
#ifdef USE_WIN_MUTEX_PROTECT
		    ReleaseMutex(smpd_process.hDBSMutex);
#endif
		    smpd_exit_fn(FCNAME);
		    return SMPD_DBS_SUCCESS;
		}
		pElementTrailer = pElement;
		pElement = pElement->pNext;
	    }
#ifdef USE_WIN_MUTEX_PROTECT
	    ReleaseMutex(smpd_process.hDBSMutex);
#endif
	    smpd_exit_fn(FCNAME);
	    return SMPD_DBS_FAIL;
	}
	pNode = pNode->pNext;
    }

#ifdef USE_WIN_MUTEX_PROTECT
    ReleaseMutex(smpd_process.hDBSMutex);
#endif

    smpd_exit_fn(FCNAME);
    return SMPD_DBS_FAIL;
}

#undef FCNAME
#define FCNAME "smpd_dbs_destroy"
int smpd_dbs_destroy(const char *name)
{
    smpd_database_node_t *pNode, *pNodeTrailer;
    smpd_database_element_t *pElement;

    smpd_enter_fn(FCNAME);

#ifdef USE_WIN_MUTEX_PROTECT
    WaitForSingleObject(smpd_process.hDBSMutex, INFINITE);
#endif

    pNodeTrailer = pNode = smpd_process.pDatabase;
    while (pNode)
    {
	if (strcmp(pNode->pszName, name) == 0)
	{
	    while (pNode->pData)
	    {
		pElement = pNode->pData;
		pNode->pData = pNode->pData->pNext;
		free(pElement);
	    }
	    if (pNodeTrailer == pNode)
		smpd_process.pDatabase = smpd_process.pDatabase->pNext;
	    else
		pNodeTrailer->pNext = pNode->pNext;
	    free(pNode);
#ifdef USE_WIN_MUTEX_PROTECT
	    ReleaseMutex(smpd_process.hDBSMutex);
#endif
	    smpd_exit_fn(FCNAME);
	    return SMPD_DBS_SUCCESS;
	}
	pNodeTrailer = pNode;
	pNode = pNode->pNext;
    }

#ifdef USE_WIN_MUTEX_PROTECT
    ReleaseMutex(smpd_process.hDBSMutex);
#endif

    smpd_exit_fn(FCNAME);
    return SMPD_DBS_FAIL;
}

#undef FCNAME
#define FCNAME "smpd_dbs_first"
int smpd_dbs_first(const char *name, char *key, char *value)
{
    smpd_database_node_t *pNode;

    smpd_enter_fn(FCNAME);

#ifdef USE_WIN_MUTEX_PROTECT
    WaitForSingleObject(smpd_process.hDBSMutex, INFINITE);
#endif

    if (key != NULL)
	key[0] = '\0';
    pNode = smpd_process.pDatabase;
    while (pNode)
    {
	if (strcmp(pNode->pszName, name) == 0)
	{
	    if (key != NULL)
	    {
		if (pNode->pData)
		{
		    strcpy(key, pNode->pData->pszKey);
		    strcpy(value, pNode->pData->pszValue);
		    pNode->pIter = pNode->pData->pNext;
		}
		else
		    key[0] = '\0';
	    }
	    else
	    {
		pNode->pIter = pNode->pData;
	    }
#ifdef USE_WIN_MUTEX_PROTECT
	    ReleaseMutex(smpd_process.hDBSMutex);
#endif
	    smpd_exit_fn(FCNAME);
	    return SMPD_DBS_SUCCESS;
	}
	pNode = pNode->pNext;
    }

#ifdef USE_WIN_MUTEX_PROTECT
    ReleaseMutex(smpd_process.hDBSMutex);
#endif

    smpd_exit_fn(FCNAME);
    return SMPD_DBS_FAIL;
}

#undef FCNAME
#define FCNAME "smpd_dbs_next"
int smpd_dbs_next(const char *name, char *key, char *value)
{
    smpd_database_node_t *pNode;

    smpd_enter_fn(FCNAME);

#ifdef USE_WIN_MUTEX_PROTECT
    WaitForSingleObject(smpd_process.hDBSMutex, INFINITE);
#endif

    key[0] = '\0';
    pNode = smpd_process.pDatabase;
    while (pNode)
    {
	if (strcmp(pNode->pszName, name) == 0)
	{
	    if (pNode->pIter)
	    {
		strcpy(key, pNode->pIter->pszKey);
		strcpy(value, pNode->pIter->pszValue);
		pNode->pIter = pNode->pIter->pNext;
	    }
	    else
		key[0] = '\0';
#ifdef USE_WIN_MUTEX_PROTECT
	    ReleaseMutex(smpd_process.hDBSMutex);
#endif
	    smpd_exit_fn(FCNAME);
	    return SMPD_DBS_SUCCESS;
	}
	pNode = pNode->pNext;
    }

#ifdef USE_WIN_MUTEX_PROTECT
    ReleaseMutex(smpd_process.hDBSMutex);
#endif

    smpd_exit_fn(FCNAME);
    return SMPD_DBS_FAIL;
}

#undef FCNAME
#define FCNAME "smpd_dbs_firstdb"
int smpd_dbs_firstdb(char *name)
{
    smpd_enter_fn(FCNAME);

#ifdef USE_WIN_MUTEX_PROTECT
    WaitForSingleObject(smpd_process.hDBSMutex, INFINITE);
#endif

    smpd_process.pDatabaseIter = smpd_process.pDatabase;
    if (name != NULL)
    {
	if (smpd_process.pDatabaseIter)
	    strcpy(name, smpd_process.pDatabaseIter->pszName);
	else
	    name[0] = '\0';
    }

#ifdef USE_WIN_MUTEX_PROTECT
    ReleaseMutex(smpd_process.hDBSMutex);
#endif

    smpd_exit_fn(FCNAME);
    return SMPD_DBS_SUCCESS;
}

#undef FCNAME
#define FCNAME "smpd_dbs_nextdb"
int smpd_dbs_nextdb(char *name)
{
    smpd_enter_fn(FCNAME);

#ifdef USE_WIN_MUTEX_PROTECT
    WaitForSingleObject(smpd_process.hDBSMutex, INFINITE);
#endif

    if (smpd_process.pDatabaseIter == NULL)
	name[0] = '\0';
    else
    {
	smpd_process.pDatabaseIter = smpd_process.pDatabaseIter->pNext;
	if (smpd_process.pDatabaseIter)
	    strcpy(name, smpd_process.pDatabaseIter->pszName);
	else
	    name[0] = '\0';
    }

#ifdef USE_WIN_MUTEX_PROTECT
    ReleaseMutex(smpd_process.hDBSMutex);
#endif

    smpd_exit_fn(FCNAME);
    return SMPD_DBS_SUCCESS;
}
