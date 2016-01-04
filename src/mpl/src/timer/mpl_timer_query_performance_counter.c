/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpl.h"

MPL_SUPPRESS_OSX_HAS_NO_SYMBOLS_WARNING;

#if MPL_TIMER_KIND == MPL_TIMER_KIND__QUERYPERFORMANCECOUNTER

double MPL_Seconds_per_tick = 0.0;     /* High performance counter frequency */
int MPL_Wtime_init(void)
{
    LARGE_INTEGER n;
    QueryPerformanceFrequency(&n);
    MPL_Seconds_per_tick = 1.0 / (double) n.QuadPart;
    return 0;
}

double MPL_Wtick(void)
{
    return MPL_Seconds_per_tick;
}

void MPL_Wtime_todouble(MPL_Time_t * t, double *val)
{
    *val = (double) t->QuadPart * MPL_Seconds_per_tick;
}

void MPL_Wtime_diff(MPL_Time_t * t1, MPL_Time_t * t2, double *diff)
{
    LARGE_INTEGER n;
    n.QuadPart = t2->QuadPart - t1->QuadPart;
    *diff = (double) n.QuadPart * MPL_Seconds_per_tick;
}

void MPL_Wtime_acc(MPL_Time_t * t1, MPL_Time_t * t2, MPL_Time_t * t3)
{
    t3->QuadPart += ((t2->QuadPart) - (t1->QuadPart));
}

#endif
