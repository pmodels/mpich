/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*  $Id: WaitThread.h,v 1.2 2002/09/27 21:11:13 toonen Exp $
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#ifndef WAIT_THREAD_H
#define WAIT_THREAD_H

#ifdef WSOCK2_BEFORE_WINDOWS
#include <winsock2.h>
#endif
#include <windows.h>

void WaitForLotsOfObjects(int nHandles, HANDLE *pHandle);

#endif
