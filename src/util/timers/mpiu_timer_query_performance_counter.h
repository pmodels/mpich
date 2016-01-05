/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef MPIU_TIMER_QUERY_PERFORMANCE_COUNTER_H_INCLUDED
#define MPIU_TIMER_QUERY_PERFORMANCE_COUNTER_H_INCLUDED

#include <winsock2.h>
#include <windows.h>

static inline void MPIU_Wtime(MPIU_Time_t *timeval)
{
    QueryPerformanceCounter(timeval);

    return MPIU_TIMER_SUCCESS;
}

extern double MPIU_Seconds_per_tick;

#endif
