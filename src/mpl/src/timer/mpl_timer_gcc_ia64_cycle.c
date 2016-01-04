/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpl.h"

MPL_SUPPRESS_OSX_HAS_NO_SYMBOLS_WARNING;

#if MPL_TIMER_KIND == MPL_TIMER_KIND__GCC_IA64_CYCLE

#include <sys/time.h>

static double seconds_per_tick = 0.0;

int MPL_Wtick(double *wtick)
{
    *wtick = seconds_per_tick;

    return MPL_TIMER_SUCCESS;
}

int MPL_Wtime_init(void)
{
    unsigned long long t1, t2;
    struct timeval tv1, tv2;
    double td1, td2;

    gettimeofday(&tv1, NULL);
    MPL_Wtime(&t1);
    usleep(250000);
    gettimeofday(&tv2, NULL);
    MPL_Wtime(&t2);

    td1 = tv1.tv_sec + tv1.tv_usec / 1000000.0;
    td2 = tv2.tv_sec + tv2.tv_usec / 1000000.0;

    seconds_per_tick = (td2 - td1) / (double) (t2 - t1);

    return MPL_TIMER_SUCCESS;
}

/* Time stamps created by a macro */
int MPL_Wtime_diff(MPL_Time_t * t1, MPL_Time_t * t2, double *diff)
{
    *diff = (double) (*t2 - *t1) * seconds_per_tick;

    return MPL_TIMER_SUCCESS;
}

int MPL_Wtime_todouble(MPL_Time_t * t, double *val)
{
    /* This returns the number of cycles as the "time".  This isn't correct
     * for implementing MPI_Wtime, but it does allow us to insert cycle
     * counters into test programs */
    *val = (double) *t * seconds_per_tick;

    return MPL_TIMER_SUCCESS;
}

int MPL_Wtime_acc(MPL_Time_t * t1, MPL_Time_t * t2, MPL_Time_t * t3)
{
    *t3 += (*t2 - *t1);

    return MPL_TIMER_SUCCESS;
}

#endif
