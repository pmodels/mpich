/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef MPIU_TIMER_GCC_IA64_CYCLE_H_INCLUDED
#define MPIU_TIMER_GCC_IA64_CYCLE_H_INCLUDED

static inline void MPIU_Wtime(MPIU_Time_t *timeval)
{
    MPIU_Time_t t_val;

#ifdef __INTEL_COMPILER
#include "ia64regs.h"
    t_val = __getReg(_IA64_REG_AR_ITC);
#else
    __asm__ __volatile__("mov %0=ar.itc" : "=r" (t_val));
#endif

    *timeval = t_val;

    return MPIU_TIMER_SUCCESS;
}

extern double MPIU_Seconds_per_tick;

#endif
