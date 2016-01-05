/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiu_timer.h"

MPL_SUPPRESS_OSX_HAS_NO_SYMBOLS_WARNING;

#if MPICH_TIMER_KIND == MPIU_TIMER_KIND__GCC_IA64_CYCLE

#include <sys/time.h>
double MPIU_Seconds_per_tick = 0.0;
int MPIU_Wtick(double *wtick)
{
    *wtick = MPIU_Seconds_per_tick;

    return MPIU_TIMER_SUCCESS;
}

int MPIU_Wtime_init(void)
{
    unsigned long long t1, t2;
    struct timeval tv1, tv2;
    double td1, td2;

    gettimeofday(&tv1, NULL);
    MPIU_Wtime(&t1);
    usleep(250000);
    gettimeofday(&tv2, NULL);
    MPIU_Wtime(&t2);

    td1 = tv1.tv_sec + tv1.tv_usec / 1000000.0;
    td2 = tv2.tv_sec + tv2.tv_usec / 1000000.0;

    MPIU_Seconds_per_tick = (td2 - td1) / (double) (t2 - t1);

    return MPIU_TIMER_SUCCESS;
}

/* Time stamps created by a macro */
int MPIU_Wtime_diff(MPIU_Time_t * t1, MPIU_Time_t * t2, double *diff)
{
    *diff = (double) (*t2 - *t1) * MPIU_Seconds_per_tick;

    return MPIU_TIMER_SUCCESS;
}

int MPIU_Wtime_todouble(MPIU_Time_t * t, double *val)
{
    /* This returns the number of cycles as the "time".  This isn't correct
     * for implementing MPI_Wtime, but it does allow us to insert cycle
     * counters into test programs */
    *val = (double) *t * MPIU_Seconds_per_tick;

    return MPIU_TIMER_SUCCESS;
}

int MPIU_Wtime_acc(MPIU_Time_t * t1, MPIU_Time_t * t2, MPIU_Time_t * t3)
{
    *t3 += (*t2 - *t1);

    return MPIU_TIMER_SUCCESS;
}

#endif
