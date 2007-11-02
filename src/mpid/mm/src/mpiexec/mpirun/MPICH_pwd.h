/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*  $Id: MPICH_pwd.h,v 1.2 2002/09/27 21:11:12 toonen Exp $
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#ifndef MPICH_PWD_H
#define MPICH_PWD_H

#ifdef WSOCK2_BEFORE_WINDOWS
#include <winsock2.h>
#endif
#include <windows.h>

BOOL SetupCryptoClient();
BOOL SavePasswordToRegistry(TCHAR *szAccount, TCHAR *szPassword, bool persistent=true);
BOOL ReadPasswordFromRegistry(TCHAR *szAccount, TCHAR *szPassword);
BOOL DeleteCurrentPasswordRegistryEntry();

#endif
