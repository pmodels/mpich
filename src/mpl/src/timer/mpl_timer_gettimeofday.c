/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpl.h"

MPL_SUPPRESS_OSX_HAS_NO_SYMBOLS_WARNING;

#if MPL_TIMER_KIND == MPL_TIMER_KIND__GETTIMEOFDAY

#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

int MPL_Wtime(MPL_Time_t * tval)
{
    gettimeofday(tval, NULL);

    return MPL_TIMER_SUCCESS;
}

int MPL_Wtime_diff(MPL_Time_t * t1, MPL_Time_t * t2, double *diff)
{
    *diff = ((double) (t2->tv_sec - t1->tv_sec) + .000001 * (double) (t2->tv_usec - t1->tv_usec));

    return MPL_TIMER_SUCCESS;
}

int MPL_Wtime_todouble(MPL_Time_t * t, double *val)
{
    *val = (double) t->tv_sec + .000001 * (double) t->tv_usec;

    return MPL_TIMER_SUCCESS;
}

int MPL_Wtime_acc(MPL_Time_t * t1, MPL_Time_t * t2, MPL_Time_t * t3)
{
    int usec, sec;

    usec = t2->tv_usec - t1->tv_usec;
    sec = t2->tv_sec - t1->tv_sec;
    t3->tv_usec += usec;
    t3->tv_sec += sec;
    /* Handle carry to the integer seconds field */
    while (t3->tv_usec > 1000000) {
        t3->tv_usec -= 1000000;
        t3->tv_sec++;
    }

    return MPL_TIMER_SUCCESS;
}

int MPL_Wtick(double *wtick)
{
    *wtick = tickval;

    return MPL_TIMER_SUCCESS;
}

int MPL_Wtime_init(void)
{
    init_wtick();

    return MPL_TIMER_SUCCESS;
}

#endif
