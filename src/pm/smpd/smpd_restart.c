/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include <stdio.h>
#include "smpd.h"

int smpd_restart()
{
#ifdef HAVE_WINDOWS_H
    int error;
    char szExe[1024];
    char pszStr[2048];
    STARTUPINFO sInfo;
    PROCESS_INFORMATION pInfo;

    if (!GetModuleFileName(NULL, szExe, 1024))
    {
	smpd_translate_win_error(GetLastError(), pszStr, 2048, "GetModuleFileName failed.\nError: ");
	return SMPD_FAIL;
    }

    /* Warning: This function can raise an exception */
    GetStartupInfo(&sInfo);

    snprintf(pszStr, 2048, "\"%s\" -restart", szExe);

    if (!CreateProcess(NULL, 
	    pszStr,
	    NULL, NULL, FALSE, 
	    DETACHED_PROCESS,
	    NULL, NULL, 
	    &sInfo, &pInfo))
    {
	error = GetLastError();
	printf("CreateProcess failed for '%s'\n", pszStr);
	smpd_translate_win_error(error, pszStr, 2048, "Error: ");
	return SMPD_FAIL;
    }
    CloseHandle(pInfo.hProcess);
    CloseHandle(pInfo.hThread);
#else
    /* close all fd's */
    /* exec a new smpd? */
#endif
    return SMPD_SUCCESS;
}
