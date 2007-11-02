/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*  $Id: LaunchProcess.cpp,v 1.2 2002/09/27 21:11:12 toonen Exp $
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "LaunchProcess.h"
#include <stdio.h>
#include "global.h"
#include "MPIJobDefs.h"
#include "Translate_Error.h"
//#include "bsocket.h"
#include "mpdutil.h"
#include "mpd.h"
#include "RedirectIO.h"
#include <stdlib.h>

// Function name	: LaunchProcess
// Description	    : 
// Return type		: void 
// Argument         : LaunchProcessArg *arg
void MPIRunLaunchProcess(MPIRunLaunchProcessArg *arg)
{
    DWORD length = 100;
    HANDLE hRIThread = NULL;
    long error;
    int nPid;
    int nPort = MPD_DEFAULT_PORT;
    SOCKET sock;
    int launchid;
    char pszStr[MAX_CMD_LENGTH+1];
    char pszIOE[10];
    
    //printf("connecting to %s:%d rank %d\n", arg->pszHost, nPort, arg->i);fflush(stdout);
    if ((error = ConnectToMPD(arg->pszHost, nPort, arg->pszPassPhrase, &sock)) == 0)
    {
	if (arg->i == 0)
	    strcpy(pszIOE, "012"); // only redirect stdin to the root process
	else
	    strcpy(pszIOE, "12");

	if (g_nNproc > FORWARD_NPROC_THRESHOLD)
	{
	    if (arg->i > 0)
	    {
		while (g_pForwardHost[(arg->i - 1)/2].nPort == 0)
		    Sleep(100);
		sprintf(arg->pszIOHostPort, "%s:%d", g_pForwardHost[(arg->i - 1)/2].pszHost, g_pForwardHost[(arg->i - 1)/2].nPort);
		if (g_nNproc/2 > arg->i)
		{
		    strncpy(g_pForwardHost[arg->i].pszHost, arg->pszHost, MAX_HOST_LENGTH);
		    g_pForwardHost[arg->i].pszHost[MAX_HOST_LENGTH-1] = '\0';
		    sprintf(pszStr, "createforwarder host=%s forward=%s", arg->pszHost, arg->pszIOHostPort);
		    WriteString(sock, pszStr);
		    ReadString(sock, pszStr);
		    int nTempPort = atoi(pszStr);
		    if (nTempPort == -1)
		    {
			// If creating the forwarder fails, redirect output to the root instead
			g_pForwardHost[arg->i] = g_pForwardHost[0];
		    }
		    else
			g_pForwardHost[arg->i].nPort = nTempPort;
		    //printf("forwarder %s:%d\n", g_pForwardHost[arg->i].pszHost, g_pForwardHost[arg->i].nPort);fflush(stdout);
		}
	    }
	}

	if (g_pDriveMapList && !g_bNoDriveMapping)
	{
	    MapDriveNode *pNode = g_pDriveMapList;
	    char *pszEncoded;
	    while (pNode)
	    {
		/*
		if (strlen(arg->pszAccount))
		{
		    pszEncoded = EncodePassword(g_pszPassword);
		    sprintf(pszStr, "map drive=%c share=%s account=%s password=%s", 
			pNode->cDrive, pNode->pszShare, g_pszAccount, pszEncoded);
		    if (pszEncoded != NULL) free(pszEncoded);
		}
		else
		{
		    sprintf(pszStr, "map drive=%c share=%s", pNode->cDrive, pNode->pszShare);
		}
		*/
		pszEncoded = EncodePassword(g_pszPassword);
		sprintf(pszStr, "map drive=%c share=%s account=%s password=%s", 
		    pNode->cDrive, pNode->pszShare, g_pszAccount, pszEncoded);
		if (pszEncoded != NULL) free(pszEncoded);
		if (WriteString(sock, pszStr) == SOCKET_ERROR)
		{
		    printf("ERROR: Unable to send map command to '%s'\r\nError %d", arg->pszHost, WSAGetLastError());
		    easy_closesocket(sock);
		    SetEvent(g_hAbortEvent);
		    delete arg;
		    return;
		}
		if (!ReadString(sock, pszStr))
		{
		    printf("ERROR: Unable to read the result of a map command to '%s'\r\nError %d", arg->pszHost, WSAGetLastError());
		    easy_closesocket(sock);
		    SetEvent(g_hAbortEvent);
		    delete arg;
		    return;
		}
		if (stricmp(pszStr, "SUCCESS"))
		{
		    printf("ERROR: Unable to map %c: to %s on %s\r\n%s", pNode->cDrive, pNode->pszShare, arg->pszHost, pszStr);
		    easy_closesocket(sock);
		    SetEvent(g_hAbortEvent);
		    delete arg;
		    return;
		}
		pNode = pNode->pNext;
	    }
	}

	// LaunchProcess
	//printf("launching on %s, %s\n", arg->pszHost, arg->pszCmdLine);fflush(stdout);
	if (arg->bLogon)
	{
	    char *pszEncoded;
	    pszEncoded = EncodePassword(arg->pszPassword);
	    if (strlen(arg->pszDir) > 0)
	    {
		if (_snprintf(pszStr, MAX_CMD_LENGTH, "launch h=%s c='%s' e='%s' a=%s p=%s %s=%s k=%d d='%s'", 
		    arg->pszHost, arg->pszCmdLine, arg->pszEnv, arg->pszAccount, pszEncoded, 
		    pszIOE, arg->pszIOHostPort, arg->i, arg->pszDir) < 0)
		{
		    printf("ERROR: command exceeds internal buffer size\n");
		    easy_closesocket(sock);
		    SetEvent(g_hAbortEvent);
		    delete arg;
		    if (pszEncoded != NULL) free(pszEncoded);
		    return;
		}
	    }
	    else
	    {
		if (_snprintf(pszStr, MAX_CMD_LENGTH, "launch h=%s c='%s' e='%s' a=%s p=%s %s=%s k=%d", 
		    arg->pszHost, arg->pszCmdLine, arg->pszEnv, arg->pszAccount, pszEncoded, 
		    pszIOE, arg->pszIOHostPort, arg->i) < 0)
		{
		    printf("ERROR: command exceeds internal buffer size\n");
		    easy_closesocket(sock);
		    SetEvent(g_hAbortEvent);
		    delete arg;
		    if (pszEncoded != NULL) free(pszEncoded);
		    return;
		}
	    }
	    if (pszEncoded != NULL) free(pszEncoded);
	}
	else
	{
	    if (strlen(arg->pszDir) > 0)
	    {
		if (_snprintf(pszStr, MAX_CMD_LENGTH, "launch h=%s c='%s' e='%s' %s=%s k=%d d='%s'",
		    arg->pszHost, arg->pszCmdLine, arg->pszEnv, 
		    pszIOE, arg->pszIOHostPort, arg->i, arg->pszDir) < 0)
		{
		    printf("ERROR: command exceeds internal buffer size\n");
		    easy_closesocket(sock);
		    SetEvent(g_hAbortEvent);
		    delete arg;
		    return;
		}
	    }
	    else
	    {
		if (_snprintf(pszStr, MAX_CMD_LENGTH, "launch h=%s c='%s' e='%s' %s=%s k=%d",
		    arg->pszHost, arg->pszCmdLine, arg->pszEnv, 
		    pszIOE, arg->pszIOHostPort, arg->i) < 0)
		{
		    printf("ERROR: command exceeds internal buffer size\n");
		    easy_closesocket(sock);
		    SetEvent(g_hAbortEvent);
		    delete arg;
		    return;
		}
	    }
	}
	if (WriteString(sock, pszStr) == SOCKET_ERROR)
	{
	    printf("ERROR: Unable to send launch command to '%s'\r\nError %d", arg->pszHost, WSAGetLastError());
	    easy_closesocket(sock);
	    SetEvent(g_hAbortEvent);
	    delete arg;
	    return;
	}
	if (!ReadString(sock, pszStr))
	{
	    printf("ERROR: Unable to read the result of the launch command on '%s'\r\nError %d", arg->pszHost, WSAGetLastError());
	    easy_closesocket(sock);
	    SetEvent(g_hAbortEvent);
	    delete arg;
	    return;
	}
	launchid = atoi(pszStr);
	// save the launch id, get the pid
	sprintf(pszStr, "getpid %d", launchid);
	if (WriteString(sock, pszStr) == SOCKET_ERROR)
	{
	    printf("ERROR: Unable to send getpid command to '%s'\r\nError %d", arg->pszHost, WSAGetLastError());
	    easy_closesocket(sock);
	    SetEvent(g_hAbortEvent);
	    delete arg;
	    return;
	}
	if (!ReadString(sock, pszStr))
	{
	    printf("ERROR: Unable to read the result of the getpid command on '%s'\r\nError %d", arg->pszHost, WSAGetLastError());
	    easy_closesocket(sock);
	    SetEvent(g_hAbortEvent);
	    delete arg;
	    return;
	}
	nPid = atoi(pszStr);
	if (nPid == -1)
	{
	    sprintf(pszStr, "geterror %d", launchid);
	    if (WriteString(sock, pszStr) == SOCKET_ERROR)
	    {
		printf("ERROR: Unable to send geterror command after an unsuccessful launch on '%s'\r\nError %d", arg->pszHost, WSAGetLastError());
		easy_closesocket(sock);
		SetEvent(g_hAbortEvent);
		delete arg;
		return;
	    }
	    if (!ReadString(sock, pszStr))
	    {
		printf("ERROR: Unable to read the result of the geterror command on '%s'\r\nError %d", arg->pszHost, WSAGetLastError());
		easy_closesocket(sock);
		SetEvent(g_hAbortEvent);
		delete arg;
		return;
	    }
	    if (strcmp(pszStr, "ERROR_SUCCESS"))
	    {
		printf("Failed to launch process %d:\n'%s'\n%s\n", arg->i, arg->pszCmdLine, pszStr);fflush(stdout);

		if (!UnmapDrives(sock))
		{
		    printf("Drive unmappings failed on %s\n", arg->pszHost);
		}

		sprintf(pszStr, "freeprocess %d", launchid);
		WriteString(sock, pszStr);
		WriteString(sock, "done");
		//easy_closesocket(sock);
		//ExitProcess(0);
		easy_closesocket(sock);
		SetEvent(g_hAbortEvent);
		delete arg;
		return;
	    }
	}
	
	// Wait for the process to exit
	sprintf(pszStr, "getexitcodewait %d", launchid);
	if (WriteString(sock, pszStr) == SOCKET_ERROR)
	{
	    printf("Error: Unable to send a getexitcodewait command to '%s'\r\nError %d", arg->pszHost, WSAGetLastError());fflush(stdout);
	    easy_closesocket(sock);
	    SetEvent(g_hAbortEvent);
	    delete arg;
	    //if (arg->i == 0)
		//ExitProcess(0);
	    return;
	}

	int i = InterlockedIncrement(&g_nNumProcessSockets) - 1;
	g_pProcessSocket[i] = sock;
	g_pProcessLaunchId[i] = launchid;
	g_pLaunchIdToRank[i] = arg->i;

	//printf("(P:%d)", launchid);fflush(stdout);
    }
    else
    {
	printf("MPIRunLaunchProcess: Connect to %s failed, error %d\n", arg->pszHost, error);fflush(stdout);
	//ExitProcess(0);
	SetEvent(g_hAbortEvent);
	delete arg;
	return;
    }
    
    memset(arg->pszPassword, 0, 100);
    delete arg;
}

