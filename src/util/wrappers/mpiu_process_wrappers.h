/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#ifndef MPIU_PROCESS_WRAPPERS_H_INCLUDED
#define MPIU_PROCESS_WRAPPERS_H_INCLUDED
/* MPIU_PW --> MPI Util Process Wrapper */
#include "mpichconf.h"
#if defined(HAVE_SCHED_H)
    #include <sched.h>
#elif defined(HAVE_WINDOWS_H)
    #include<winsock2.h>
    #include <windows.h>
#endif

/* MPIU_PW_SCHED_YIELD() - Yield the processor to OS scheduler */
#if defined(HAVE_SWITCHTOTHREAD)
    #define MPIU_PW_Sched_yield() SwitchToThread()
#elif defined(HAVE_WIN32_SLEEP)
    #define MPIU_PW_Sched_yield() Sleep(0)
#elif defined(HAVE_SCHED_YIELD)
    #define MPIU_PW_Sched_yield() sched_yield()
#elif defined(HAVE_YIELD)
    #define MPIU_PW_Sched_yield() yield()
#elif defined (HAVE_SELECT)
    #define MPIU_PW_Sched_yield() do { struct timeval t; t.tv_sec = 0; t.tv_usec = 0; select(0,0,0,0,&t); } while (0)
#elif defined (HAVE_USLEEP)
    #define MPIU_PW_Sched_yield() usleep(0)
#elif defined (HAVE_SLEEP)
    #define MPIU_PW_Sched_yield() sleep(0)
#else
    #error "No mechanism available to yield"
#endif

#endif /* MPIU_PROCESS_WRAPPERS_H_INCLUDED */
