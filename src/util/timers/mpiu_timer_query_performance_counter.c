/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiu_timer.h"

MPL_SUPPRESS_OSX_HAS_NO_SYMBOLS_WARNING;

#if MPICH_TIMER_KIND == MPIU_TIMER_KIND__QUERYPERFORMANCECOUNTER

double MPIU_Seconds_per_tick = 0.0;     /* High performance counter frequency */
int MPIU_Wtime_init(void)
{
    LARGE_INTEGER n;
    QueryPerformanceFrequency(&n);
    MPIU_Seconds_per_tick = 1.0 / (double) n.QuadPart;

    return MPIU_TIMER_SUCCESS;
}

int MPIU_Wtick(double *wtick)
{
    *wtick = MPIU_Seconds_per_tick;

    return MPIU_TIMER_SUCCESS;
}

int MPIU_Wtime_todouble(MPIU_Time_t * t, double *val)
{
    *val = (double) t->QuadPart * MPIU_Seconds_per_tick;

    return MPIU_TIMER_SUCCESS;
}

int MPIU_Wtime_diff(MPIU_Time_t * t1, MPIU_Time_t * t2, double *diff)
{
    LARGE_INTEGER n;
    n.QuadPart = t2->QuadPart - t1->QuadPart;
    *diff = (double) n.QuadPart * MPIU_Seconds_per_tick;

    return MPIU_TIMER_SUCCESS;
}

int MPIU_Wtime_acc(MPIU_Time_t * t1, MPIU_Time_t * t2, MPIU_Time_t * t3)
{
    t3->QuadPart += ((t2->QuadPart) - (t1->QuadPart));

    return MPIU_TIMER_SUCCESS;
}

#endif
