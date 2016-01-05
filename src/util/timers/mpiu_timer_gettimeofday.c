/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiu_timer.h"

MPL_SUPPRESS_OSX_HAS_NO_SYMBOLS_WARNING;

#if MPICH_TIMER_KIND == MPIU_TIMER_KIND__GETTIMEOFDAY

#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
int MPIU_Wtime(MPIU_Time_t * tval)
{
    gettimeofday(tval, NULL);

    return MPIU_TIMER_SUCCESS;
}

int MPIU_Wtime_diff(MPIU_Time_t * t1, MPIU_Time_t * t2, double *diff)
{
    *diff = ((double) (t2->tv_sec - t1->tv_sec) + .000001 * (double) (t2->tv_usec - t1->tv_usec));

    return MPIU_TIMER_SUCCESS;
}

int MPIU_Wtime_todouble(MPIU_Time_t * t, double *val)
{
    *val = (double) t->tv_sec + .000001 * (double) t->tv_usec;

    return MPIU_TIMER_SUCCESS;
}

int MPIU_Wtime_acc(MPIU_Time_t * t1, MPIU_Time_t * t2, MPIU_Time_t * t3)
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

    return MPIU_TIMER_SUCCESS;
}

int MPIU_Wtick(double *wtick)
{
    *wtick = tickval;

    return MPIU_TIMER_SUCCESS;
}

int MPIU_Wtime_init(void)
{
    init_wtick();

    return MPIU_TIMER_SUCCESS;
}

#endif
