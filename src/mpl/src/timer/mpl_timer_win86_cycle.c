/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpl.h"

MPL_SUPPRESS_OSX_HAS_NO_SYMBOLS_WARNING;

#if (MPL_TIMER_KIND == MPL_TIMER_KIND__WIN86_CYCLE) || (MPL_TIMER_KIND == MPL_TIMER_KIND__WIN64_CYCLE)

double MPL_Seconds_per_tick = 0.0;
double MPL_Wtick(void)
{
    return MPL_Seconds_per_tick;
}

void MPL_Wtime_todouble(MPL_Time_t * t, double *d)
{
    *d = (double) (__int64) * t * MPL_Seconds_per_tick;
}

void MPL_Wtime_diff(MPL_Time_t * t1, MPL_Time_t * t2, double *diff)
{
    *diff = (double) ((__int64) (*t2 - *t1)) * MPL_Seconds_per_tick;
}

int MPL_Wtime_init(void)
{
    MPL_Time_t t1, t2;
    DWORD s1, s2;
    double d;
    int i;

    MPL_Wtime(&t1);
    MPL_Wtime(&t1);

    /* time an interval using both timers */
    s1 = GetTickCount();
    MPL_Wtime(&t1);
    /*Sleep(250); *//* Sleep causes power saving cpu's to stop which stops the counter */
    while (GetTickCount() - s1 < 200) {
        for (i = 2; i < 1000; i++)
            d = (double) i / (double) (i - 1);
    }
    s2 = GetTickCount();
    MPL_Wtime(&t2);

    /* calculate the frequency of the assembly cycle counter */
    MPL_Seconds_per_tick = ((double) (s2 - s1) / 1000.0) / (double) ((__int64) (t2 - t1));
    /*
     * printf("t2-t1 %10d\nsystime diff %d\nfrequency %g\n CPU MHz %g\n",
     * (int)(t2-t1), (int)(s2 - s1), MPL_Seconds_per_tick, MPL_Seconds_per_tick * 1.0e6);
     */
    return 0;
}

#endif
