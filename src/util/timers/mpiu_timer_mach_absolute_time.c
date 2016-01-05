/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiu_timer.h"

MPL_SUPPRESS_OSX_HAS_NO_SYMBOLS_WARNING;

#if MPICH_TIMER_KIND == MPIU_TIMER_KIND__MACH_ABSOLUTE_TIME

static double MPIR_Wtime_mult;

int MPIU_Wtime_init(void)
{
    mach_timebase_info_data_t info;
    mach_timebase_info(&info);
    MPIR_Wtime_mult = 1.0e-9 * ((double) info.numer / (double) info.denom);
    init_wtick();

    return MPIU_TIMER_SUCCESS;
}

int MPIU_Wtime(MPIU_Time_t * timeval)
{
    *timeval = mach_absolute_time();

    return MPIU_TIMER_SUCCESS;
}

int MPIU_Wtime_diff(MPIU_Time_t * t1, MPIU_Time_t * t2, double *diff)
{
    *diff = (*t2 - *t1) * MPIR_Wtime_mult;

    return MPIU_TIMER_SUCCESS;
}

int MPIU_Wtime_todouble(MPIU_Time_t * t, double *val)
{
    *val = *t * MPIR_Wtime_mult;

    return MPIU_TIMER_SUCCESS;
}

int MPIU_Wtime_acc(MPIU_Time_t * t1, MPIU_Time_t * t2, MPIU_Time_t * t3)
{
    *t3 += *t2 - *t1;

    return MPIU_TIMER_SUCCESS;
}

int MPIU_Wtick(double *wtick)
{
    *wtick = tickval;

    return MPIU_TIMER_SUCCESS;
}

#endif
