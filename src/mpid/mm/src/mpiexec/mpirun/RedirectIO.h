/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*  $Id: RedirectIO.h,v 1.2 2002/09/27 21:11:12 toonen Exp $
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#ifndef REDIRECT_IO_H
#define REDIRECT_IO_H

/*
#include "resource.h"
#include "guiMPIRunView.h"
#ifdef WSOCK2_BEFORE_WINDOWS
#include <winsock2.h>
#endif
#include <windows.h>
*/
/*
struct RedirectIOArg
{
    CGuiMPIRunView *pDlg;
    HANDLE hReadyEvent;
};

void RedirectIOThread(RedirectIOArg *pArg);
*/

void RedirectIOThread(HANDLE hReadyEvent);

#endif
