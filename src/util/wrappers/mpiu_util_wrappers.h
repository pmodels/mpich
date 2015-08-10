/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#ifndef MPIU_UTIL_WRAPPERS_H_INCLUDED
#define MPIU_UTIL_WRAPPERS_H_INCLUDED

#ifdef HAVE_WINDOWS_H
#include<winsock2.h>
#include<windows.h>
#endif

#ifdef HAVE_ERRNO_H
    #include <errno.h>
#endif
#ifdef HAVE_STRING_H
    #include <string.h>
#endif

#include "mpichconf.h"
#include "mpimem.h"

#ifdef HAVE_GETLASTERROR
#   define MPIU_OSW_Get_errno()   GetLastError()
#else
#   define MPIU_OSW_Get_errno()   errno
#endif

/* MPIU_OSW_strerror() is not Thread safe */
#ifdef HAVE_FORMATMESSAGE
    static char errMsg[1024];
#   define MPIU_OSW_Strerror(errno) (                               \
        (FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL,            \
            errno, 0, (LPTSTR) errMsg, 1024, NULL)                  \
        )                                                           \
        ? errMsg : "Error while retrieving the error message"       \
    )
#else
#   define MPIU_OSW_Strerror(errno) strerror(errno)
#endif

#if defined (HAVE_QUERYPERFORMANCECOUNTER)
/*
 * Returns size of uniqStr, 0 on error
 */
static inline int MPIU_OSW_Get_uniq_str(char *str, int strlen)
{
    LARGE_INTEGER perfCnt;
    QueryPerformanceCounter(&perfCnt);
    return(MPL_snprintf(str, strlen, "MPICH_NEM_%d_%I64d", 
            GetCurrentThreadId(), (perfCnt.QuadPart)));
}
#endif

#ifdef HAVE_WINDOWS_H
#   define MPIU_OSW_EINTR WSAEINTR
#   define MPIU_OSW_ENOBUFS WSAENOBUFS
#else
#   define MPIU_OSW_EINTR EINTR
#   define MPIU_OSW_ENOBUFS ENOBUFS
#endif

/* If after func returns (errCond == true) then retry on EINTR */
#   define MPIU_OSW_RETRYON_INTR(errCond, func)  do{                \
        func;                                                       \
    }while((errCond) && (MPIU_OSW_Get_errno() == MPIU_OSW_EINTR))


#endif /* MPIU_UTIL_WRAPPERS_H_INCLUDED */
