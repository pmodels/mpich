/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#ifndef MPIU_PROCESS_WRAPPERS_H_INCLUDED
#define MPIU_PROCESS_WRAPPERS_H_INCLUDED
/* MPIU_PW --> MPI Util Process Wrapper */
#include "mpichconf.h"

/* MPIU_PW_SCHED_YIELD() - Yield the processor to OS scheduler */
#if defined(USE_SWITCHTOTHREAD_FOR_YIELD)
    #include <winsock2.h>
    #include <windows.h>
    #define MPIU_PW_Sched_yield() SwitchToThread()
#elif defined(USE_WIN32_SLEEP_FOR_YIELD)
    #include <winsock2.h>
    #include <windows.h>
    #define MPIU_PW_Sched_yield() Sleep(0)
#elif defined(USE_SCHED_YIELD_FOR_YIELD)
    #ifdef HAVE_SCHED_H
        #include <sched.h>
    #endif
    #define MPIU_PW_Sched_yield() sched_yield()
#elif defined(USE_YIELD_FOR_YIELD)
    #ifdef HAVE_SCHED_H
        #include <sched.h>
    #endif
    #define MPIU_PW_Sched_yield() yield()
#elif defined (USE_SELECT_FOR_YIELD)
    #ifdef HAVE_SYS_SELECT_H
        #include <sys/select.h>
    #endif
    #define MPIU_PW_Sched_yield() do { struct timeval t; t.tv_sec = 0; t.tv_usec = 0; select(0,0,0,0,&t); } while (0)
#elif defined (USE_USLEEP_FOR_YIELD)
    #ifdef HAVE_UNISTD_H
        #include <unistd.h>
    #endif
    #define MPIU_PW_Sched_yield() usleep(0)
#elif defined (USE_SLEEP_FOR_YIELD)
    #ifdef HAVE_UNISTD_H
        #include <unistd.h>
    #endif
    #define MPIU_PW_Sched_yield() sleep(0)
#elif defined (USE_NOTHING_FOR_YIELD)
    #define MPIU_PW_Sched_yield() do {} while (0)
#else
    #error "No mechanism available to yield"
#endif

#endif /* MPIU_PROCESS_WRAPPERS_H_INCLUDED */
