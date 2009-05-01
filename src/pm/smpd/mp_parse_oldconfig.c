/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include <stdio.h>
#include <stdlib.h>
#include "mpiexec.h"
#include "smpd.h"
#ifdef HAVE_CTYPE_H
#include <ctype.h>
#endif

typedef struct HostNode
{
    char host[100];
    char exe[SMPD_MAX_EXE_LENGTH];
    long nSMPProcs;
    struct HostNode *next;
} HostNode;

typedef struct MapDriveNode
{
    char cDrive;
    char pszShare[SMPD_MAX_DIR_LENGTH];
    struct MapDriveNode *pNext;
} MapDriveNode;

static char g_pszExe[SMPD_MAX_EXE_LENGTH];
static char g_pszArgs[SMPD_MAX_EXE_LENGTH];
static char g_pszEnv[SMPD_MAX_EXE_LENGTH];
static char g_pszDir[SMPD_MAX_EXE_LENGTH];
static MapDriveNode *g_pDriveMapList;
static HostNode *g_pHosts;
static int g_nHosts;

#ifdef HAVE_WINDOWS_H
#undef FCNAME
#define FCNAME "mkstemp"
int mkstemp(char *template)
{
    FILE *fout;
    int fd=-1;
    if(mktemp(template) != NULL){
        if((fout = fopen(template, "w")) != NULL){
            fd = fileno(fout);
        }
    }
    return fd;
}

#undef FCNAME
#define FCNAME "ExeToUnc"
static void ExeToUnc(char *pszExe, int length)
{
    DWORD dwResult;
    DWORD dwLength;
    char pBuffer[4096];
    REMOTE_NAME_INFO *info = (REMOTE_NAME_INFO*)pBuffer;
    char pszTemp[SMPD_MAX_EXE_LENGTH];
    SMPD_BOOL bQuoted = SMPD_FALSE;
    char *pszOriginal;

    smpd_enter_fn(FCNAME);

    pszOriginal = pszExe;

    if (*pszExe == '"')
    {
	bQuoted = SMPD_TRUE;
	strncpy(pszTemp, &pszExe[1], SMPD_MAX_EXE_LENGTH);
	pszTemp[SMPD_MAX_EXE_LENGTH-1] = '\0';
	if (pszTemp[strlen(pszTemp)-1] == '"')
	    pszTemp[strlen(pszTemp)-1] = '\0';
	pszExe = pszTemp;
    }
    dwLength = 4096;
    info->lpConnectionName = NULL;
    info->lpRemainingPath = NULL;
    info->lpUniversalName = NULL;
    dwResult = WNetGetUniversalName(pszExe, REMOTE_NAME_INFO_LEVEL, info, &dwLength);
    if (dwResult == NO_ERROR)
    {
	if (bQuoted)
	    snprintf(pszOriginal, length, "\"%s\"", info->lpUniversalName);
	else
	{
	    strncpy(pszOriginal, info->lpUniversalName, length);
	    pszOriginal[length-1] = '\0';
	}
    }
    smpd_exit_fn(FCNAME);
}
#endif

#undef FCNAME
#define FCNAME "ParseLineIntoHostNode"
static HostNode* ParseLineIntoHostNode(char * line)
{
    char buffer[1024];
    char *pChar, *pChar2;
    HostNode *node = NULL;

    smpd_enter_fn(FCNAME);

    strncpy(buffer, line, 1024);
    buffer[1023] = '\0';
    pChar = buffer;
    
    /* Advance over white space */
    while (*pChar != '\0' && isspace(*pChar))
	pChar++;
    if (*pChar == '#' || *pChar == '\0')
    {
	smpd_exit_fn(FCNAME);
	return NULL;
    }
    
    /* Trim trailing white space */
    pChar2 = &buffer[strlen(buffer)-1];
    while (isspace(*pChar2) && (pChar >= pChar))
    {
	*pChar2 = '\0';
	pChar2--;
    }
    
    /* If there is anything left on the line, consider it a host name */
    if (strlen(pChar) > 0)
    {
	node = (HostNode*)MPIU_Malloc(sizeof(HostNode));
	node->nSMPProcs = 1;
	node->next = NULL;
	node->exe[0] = '\0';
	
	/* Copy the host name */
	pChar2 = node->host;
	while (*pChar != '\0' && !isspace(*pChar))
	{
	    *pChar2 = *pChar;
	    pChar++;
	    pChar2++;
	}
	*pChar2 = '\0';
	
	/* Advance over white space */
	while (*pChar != '\0' && isspace(*pChar))
	    pChar++;
	/* Get the number of SMP processes */
	if (*pChar != '\0')
	{
	    node->nSMPProcs = atoi(pChar);
	    if (node->nSMPProcs < 1)
		node->nSMPProcs = 1;
	}
	/* Advance over the number */
	while (*pChar != '\0' && isdigit(*pChar))
	    pChar++;
	
	/* Advance over white space */
	while (*pChar != '\0' && isspace(*pChar))
	    pChar++;
	/* Copy the executable */
	if (*pChar != '\0')
	{
	    strncpy(node->exe, pChar, SMPD_MAX_EXE_LENGTH);
	    node->exe[SMPD_MAX_EXE_LENGTH-1] = '\0';
#ifdef HAVE_WINDOWS_H
	    ExeToUnc(node->exe, SMPD_MAX_EXE_LENGTH);
#endif
	}
    }

    smpd_exit_fn(FCNAME);
    return node;
}

#undef FCNAME
#define FCNAME "print_configfile"
static void print_configfile(FILE *fout)
{
    HostNode *node;
    char *exe;
    char *var, *val;
    char env[SMPD_MAX_ENV_LENGTH];
    char *cur_env;
    int env_length, num;

    smpd_enter_fn(FCNAME);

    if (g_pHosts == NULL)
    {
	printf("Error: unable to parse the specified configuration file.\n");
	smpd_exit_fn(FCNAME);
	return;
    }

    env[0] = '\0';
    if (g_pszEnv[0] != '\0')
    {
	env_length = SMPD_MAX_ENV_LENGTH;
	cur_env = env;
	var = strtok(g_pszEnv, "|");
	while (var != NULL)
	{
	    val = var;
	    while (*val != '\0')
	    {
		if (*val == '=')
		{
		    *val = '\0';
		    val++;
		    break;
		}
		val++;
	    }
	    if (strlen(var) > 0 && strlen(val) > 0)
	    {
		num = snprintf(cur_env, env_length, "-env %s %s ", var, val);
		if (num < 0)
		{
		    env[SMPD_MAX_ENV_LENGTH-1] = '\0';
		    break;
		}
		cur_env += num;
		env_length -= num;
	    }
	    var = strtok(NULL, "|");
	}
	/*printf("env = '%s'\n", env);*/
    }

    node = g_pHosts;
    while (node)
    {
	if (g_pDriveMapList)
	{
	    MapDriveNode *iter;
	    iter = g_pDriveMapList;
	    while (iter)
	    {
		fprintf(fout, "-map %c:%s ", iter->cDrive, iter->pszShare);
		iter = iter->pNext;
	    }
	}
	if (g_pszDir[0] != '\0')
	{
	    fprintf(fout, "-wdir %s ", g_pszDir);
	}
	if (env[0] != '\0')
	{
	    fprintf(fout, "%s", env);
	}
	fprintf(fout, "-host %s -n %d ", node->host, (int)(node->nSMPProcs));
	if (node->exe[0] != '\0')
	{
	    exe = node->exe;
	}
	else
	{
	    exe = g_pszExe;
	}
	if (g_pszArgs[0] != '\0')
	{
	    fprintf(fout, "%s %s\n", exe, g_pszArgs);
	}
	else
	{
	    fprintf(fout, "%s\n", exe);
	}
	node = node->next;
    }
    smpd_exit_fn(FCNAME);
}

#undef FCNAME
#define FCNAME "cleanup"
static void cleanup(void)
{
    MapDriveNode *map;
    HostNode *node;
    smpd_enter_fn(FCNAME);
    while (g_pDriveMapList)
    {
	map = g_pDriveMapList;
	g_pDriveMapList = g_pDriveMapList->pNext;
	MPIU_Free(map);
    }
    while (g_pHosts)
    {
	node = g_pHosts;
	g_pHosts = g_pHosts->next;
	MPIU_Free(node);
    }
    smpd_exit_fn(FCNAME);
}

#undef FCNAME
#define FCNAME "mp_parse_mpich1_configfile"
int mp_parse_mpich1_configfile(char *filename, char *configfilename, int length)
{
    FILE *fin, *fout;
    int fd;
    char buffer[1024] = "";
    char temp_filename[256] = "tmpXXXXXX";

    smpd_enter_fn(FCNAME);

    fin = fopen(filename, "r");
    if (fin == NULL)
    {
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }

    /* initialize variables */
    g_pszExe[0] = '\0';
    g_pszArgs[0] = '\0';
    g_pszEnv[0] = '\0';
    g_pszDir[0] = '\0';
    g_pDriveMapList = NULL;
    g_pHosts = NULL;
    g_nHosts = 0;

    while (fgets(buffer, 1024, fin))
    {
	/* Check for the name of the executable */
	if (strncmp(buffer, "exe ", 4) == 0)
	{
	    char *pChar = &buffer[4];
	    while (isspace(*pChar))
		pChar++;
	    strncpy(g_pszExe, pChar, SMPD_MAX_EXE_LENGTH);
	    g_pszExe[SMPD_MAX_EXE_LENGTH-1] = '\0';
	    pChar = &g_pszExe[strlen(g_pszExe)-1];
	    while (isspace(*pChar) && (pChar >= g_pszExe))
	    {
		*pChar = '\0';
		pChar--;
	    }
#ifdef HAVE_WINDOWS_H
	    ExeToUnc(g_pszExe, SMPD_MAX_EXE_LENGTH);
#endif
	}
	else
	{
	    /* Check for program arguments */
	    if (strncmp(buffer, "args ", 5) == 0)
	    {
		char *pChar = &buffer[5];
		while (isspace(*pChar))
		    pChar++;
		strncpy(g_pszArgs, pChar, SMPD_MAX_EXE_LENGTH);
		g_pszArgs[SMPD_MAX_EXE_LENGTH-1] = '\0';
		pChar = &g_pszArgs[strlen(g_pszArgs)-1];
		while (isspace(*pChar) && (pChar >= g_pszArgs))
		{
		    *pChar = '\0';
		    pChar--;
		}
	    }
	    else
	    {
		/* Check for environment variables */
		if (strncmp(buffer, "env ", 4) == 0)
		{
		    char *pChar = &buffer[4];
		    while (isspace(*pChar))
			pChar++;
		    if (strlen(pChar) >= SMPD_MAX_EXE_LENGTH)
		    {
			printf("Warning: environment variables truncated.\n");
			fflush(stdout);
		    }
		    strncpy(g_pszEnv, pChar, SMPD_MAX_EXE_LENGTH);
		    g_pszEnv[SMPD_MAX_EXE_LENGTH-1] = '\0';
		    pChar = &g_pszEnv[strlen(g_pszEnv)-1];
		    while (isspace(*pChar) && (pChar >= g_pszEnv))
		    {
			*pChar = '\0';
			pChar--;
		    }
		}
		else
		{
		    if (strncmp(buffer, "map ", 4) == 0)
		    {
			char *pszMap;
			pszMap = &buffer[strlen(buffer)-1];
			while (isspace(*pszMap) && (pszMap >= buffer))
			{
			    *pszMap = '\0';
			    pszMap--;
			}
			pszMap = &buffer[4];
			while (isspace(*pszMap))
			    pszMap++;
			if (*pszMap != '\0' && strlen(pszMap) > 6 && pszMap[1] == ':')
			{
			    MapDriveNode *pNode = (MapDriveNode*)MPIU_Malloc(sizeof(MapDriveNode));
			    pNode->cDrive = pszMap[0];
			    strcpy(pNode->pszShare, &pszMap[2]);
			    pNode->pNext = g_pDriveMapList;
			    g_pDriveMapList = pNode;
			}
		    }
		    else
		    {
			if (strncmp(buffer, "dir ", 4) == 0)
			{
			    char *pChar = &buffer[4];
			    while (isspace(*pChar))
				pChar++;
			    strcpy(g_pszDir, pChar);
			    pChar = &g_pszDir[strlen(g_pszDir)-1];
			    while (isspace(*pChar) && (pChar >= g_pszDir))
			    {
				*pChar = '\0';
				pChar--;
			    }
			}
			else
			{
			    /* Check for hosts */
			    if (strncmp(buffer, "hosts", 5) == 0)
			    {
				HostNode *node, dummy;
				g_nHosts = 0;
				g_pHosts = NULL;
				dummy.next = NULL;
				node = &dummy;
				while (fgets(buffer, 1024, fin))
				{
				    node->next = ParseLineIntoHostNode(buffer);
				    if (node->next != NULL)
				    {
					node = node->next;
					g_nHosts++;
				    }
				}
				g_pHosts = dummy.next;
				
				fclose(fin);
				if ((fd = mkstemp(temp_filename)) < 0)
				{
				    smpd_exit_fn(FCNAME);
				    return SMPD_FAIL;
				}
				/*printf("printing output to <%s>\n", configfilename);*/
				strncpy(configfilename, temp_filename, length);
                if((fout = fdopen(fd, "w")) == NULL)
                {
				    smpd_exit_fn(FCNAME);
				    return SMPD_FAIL;
                }
				print_configfile(fout);
				fclose(fout);
				cleanup();
				smpd_exit_fn(FCNAME);
				return SMPD_SUCCESS;
			    }
			}
		    }
		}
	    }
	}
    }
    fclose(fin);
    if ((fd = mkstemp(temp_filename)) < 0)
    {
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }
    /*printf("printing output to <%s>\n", configfilename);*/
	strncpy(configfilename, temp_filename, length);
    if((fout = fdopen(fd, "w")) == NULL)
    {
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }
    print_configfile(fout);
    fclose(fout);
    cleanup();
    smpd_exit_fn(FCNAME);
    return SMPD_SUCCESS;
}
