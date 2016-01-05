/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef MPIU_TIMER_WIN86_CYCLE_H_INCLUDED
#define MPIU_TIMER_WIN86_CYCLE_H_INCLUDED

#include <winsock2.h>
#include <windows.h>

static inline void MPIU_Wtime(MPIU_Time_t *timeval)
{
    register int *f1 = (int*)var_ptr;
    __asm cpuid;
    __asm rdtsc;
    __asm mov ecx, f1;
    __asm mov [ecx], eax;
    __asm mov [ecx + TYPE int], edx;

    return MPIU_TIMER_SUCCESS;
}

extern double MPIU_Seconds_per_tick;

#endif
