/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*  $Id: global.h,v 1.2 2002/09/27 21:11:13 toonen Exp $
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#ifndef GLOBAL_H
#define GLOBAL_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifdef WSOCK2_BEFORE_WINDOWS
#include <winsock2.h>
#endif
#include <windows.h>
#include <tchar.h>

#define MAX_CMD_LENGTH  8192
#define MAX_HOST_LENGTH	64

struct HostNode
{
    TCHAR host[100];
    TCHAR exe[MAX_CMD_LENGTH];
    long nSMPProcs;
    HostNode *next;
};

struct ForwardHostStruct
{
    char pszHost[MAX_HOST_LENGTH];
    int nPort;
};

struct MapDriveNode
{
    char cDrive;
    char pszShare[MAX_PATH];
    //char pszAccount[40];
    //char pszPassword[40];
    MapDriveNode *pNext;
};

bool UnmapDrives(SOCKET sock);

// Global variables
extern MapDriveNode *g_pDriveMapList;
extern bool g_bNoDriveMapping;
extern HANDLE g_hRedirectIOListenThread;
extern SOCKET g_sockStopIOSignalSocket;
extern HANDLE g_hAbortEvent;
extern SOCKET *g_pProcessSocket;
extern int *g_pProcessLaunchId;
extern int *g_pLaunchIdToRank;
extern LONG g_nNumProcessSockets;
#define FORWARD_NPROC_THRESHOLD 8
extern ForwardHostStruct *g_pForwardHost;
extern SOCKET g_sockBreak;
extern HANDLE g_hBreakReadyEvent;
extern HostNode *g_pHosts;
extern long g_nHosts;
extern int g_nNproc;
extern long g_nRootPort;
extern TCHAR g_pszAccount[100], g_pszPassword[100];
extern bool g_bNoMPI;
extern char g_pszExe[MAX_CMD_LENGTH], g_pszArgs[MAX_CMD_LENGTH], g_pszEnv[MAX_CMD_LENGTH];
extern char g_pszDir[MAX_PATH];
extern char g_pszExeOrig[MAX_CMD_LENGTH];
extern char g_pszFirstHost[MAX_HOST_LENGTH];
extern HANDLE g_hFinishedEvent;
extern HANDLE g_hConsoleOutputMutex;
extern char g_pszIOHost[MAX_HOST_LENGTH];
extern int g_nIOPort;
extern bool g_bDoMultiColorOutput;
extern WORD g_ConsoleAttribute;
extern char pmi_host[MAX_HOST_LENGTH];
extern int pmi_port;
extern char pmi_kvsname[128];

#include <wincon.h>
#define NUM_OUTPUT_COLORS 32

// foregrounds
#define frgnd_RGB	FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY
#define frgnd_RG	FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY
#define frgnd_RB	FOREGROUND_RED | FOREGROUND_BLUE | FOREGROUND_INTENSITY
#define frgnd_R		FOREGROUND_RED | FOREGROUND_INTENSITY
#define frgnd_GB	FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY
#define frgnd_G		FOREGROUND_GREEN | FOREGROUND_INTENSITY
#define frgnd_B		FOREGROUND_BLUE | FOREGROUND_INTENSITY
#define frgnd_rgb	FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE
#define frgnd_rg	FOREGROUND_RED | FOREGROUND_GREEN
#define frgnd_rb	FOREGROUND_RED | FOREGROUND_BLUE
#define frgnd_r		FOREGROUND_RED
#define frgnd_gb	FOREGROUND_GREEN | FOREGROUND_BLUE
#define frgnd_g		FOREGROUND_GREEN
#define frgnd_b		FOREGROUND_BLUE

// backgrounds
#define bkgnd_RGB	BACKGROUND_RED | BACKGROUND_GREEN | BACKGROUND_BLUE | BACKGROUND_INTENSITY
#define bkgnd_RG	BACKGROUND_RED | BACKGROUND_GREEN | BACKGROUND_INTENSITY
#define bkgnd_RB	BACKGROUND_RED | BACKGROUND_BLUE | BACKGROUND_INTENSITY
#define bkgnd_R		BACKGROUND_RED | BACKGROUND_INTENSITY
#define bkgnd_GB	BACKGROUND_GREEN | BACKGROUND_BLUE | BACKGROUND_INTENSITY
#define bkgnd_G		BACKGROUND_GREEN | BACKGROUND_INTENSITY
#define bkgnd_B		BACKGROUND_BLUE | BACKGROUND_INTENSITY
#define bkgnd_rgb	BACKGROUND_RED | BACKGROUND_GREEN | BACKGROUND_BLUE
#define bkgnd_rg	BACKGROUND_RED | BACKGROUND_GREEN
#define bkgnd_rb	BACKGROUND_RED | BACKGROUND_BLUE
#define bkgnd_r		BACKGROUND_RED
#define bkgnd_gb	BACKGROUND_GREEN | BACKGROUND_BLUE
#define bkgnd_g		BACKGROUND_GREEN
#define bkgnd_b		BACKGROUND_BLUE
extern WORD aConsoleColorAttribute[NUM_OUTPUT_COLORS];

#endif
