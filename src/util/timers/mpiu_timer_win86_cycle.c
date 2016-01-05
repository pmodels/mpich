/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiu_timer.h"

MPL_SUPPRESS_OSX_HAS_NO_SYMBOLS_WARNING;

#if (MPICH_TIMER_KIND == MPIU_TIMER_KIND__WIN86_CYCLE) || (MPICH_TIMER_KIND == MPIU_TIMER_KIND__WIN64_CYCLE)

static double seconds_per_tick = 0.0;

int MPIU_Wtick(double *wtick)
{
    *wtick = seconds_per_tick;

    return MPIU_TIMER_SUCCESS;
}

int MPIU_Wtime_todouble(MPIU_Time_t * t, double *d)
{
    *d = (double) (__int64) * t * seconds_per_tick;

    return MPIU_TIMER_SUCCESS;
}

int MPIU_Wtime_diff(MPIU_Time_t * t1, MPIU_Time_t * t2, double *diff)
{
    *diff = (double) ((__int64) (*t2 - *t1)) * seconds_per_tick;

    return MPIU_TIMER_SUCCESS;
}

int MPIU_Wtime_init(void)
{
    MPIU_Time_t t1, t2;
    DWORD s1, s2;
    double d;
    int i;

    MPIU_Wtime(&t1);
    MPIU_Wtime(&t1);

    /* time an interval using both timers */
    s1 = GetTickCount();
    MPIU_Wtime(&t1);
    /*Sleep(250); *//* Sleep causes power saving cpu's to stop which stops the counter */
    while (GetTickCount() - s1 < 200) {
        for (i = 2; i < 1000; i++)
            d = (double) i / (double) (i - 1);
    }
    s2 = GetTickCount();
    MPIU_Wtime(&t2);

    /* calculate the frequency of the assembly cycle counter */
    seconds_per_tick = ((double) (s2 - s1) / 1000.0) / (double) ((__int64) (t2 - t1));
    /*
     * printf("t2-t1 %10d\nsystime diff %d\nfrequency %g\n CPU MHz %g\n",
     * (int)(t2-t1), (int)(s2 - s1), seconds_per_tick, seconds_per_tick * 1.0e6);
     */

    return MPIU_TIMER_SUCCESS;
}

#endif
