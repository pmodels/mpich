/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpl.h"

MPL_SUPPRESS_OSX_HAS_NO_SYMBOLS_WARNING;

#if MPL_TIMER_KIND == MPL_TIMER_KIND__DEVICE

int (*MPL_Wtime_fn)(MPL_Time_t *timeval) = NULL;
int (*MPL_Wtime_diff_fn)(MPL_Time_t *t1, MPL_Time_t *t2, double *diff) = NULL;
int (*MPL_Wtime_acc_fn)(MPL_Time_t *t1, MPL_Time_t *t2, MPL_Time_t *t3) = NULL;
int (*MPL_Wtime_todouble_fn)(MPL_Time_t *timeval, double *seconds) = NULL;
int (*MPL_Wtick_fn)(double *tick) = NULL;

int MPL_Wtime(MPL_Time_t *timeval)
{
    if (MPL_Wtime_fn == NULL)
        return MPL_TIMER_ERR_NOT_INITIALIZED;
    return MPL_Wtime_fn(timeval);
}

int MPL_Wtime_diff(MPL_Time_t *t1, MPL_Time_t *t2, double *diff)
{
    if (MPL_Wtime_diff_fn == NULL)
        return MPL_TIMER_ERR_NOT_INITIALIZED;
    return MPL_Wtime_diff_fn(t1, t2, diff);
}

int MPL_Wtime_todouble(MPL_Time_t *t, double *val)
{
    if (MPL_Wtime_todouble_fn == NULL)
        return MPL_TIMER_ERR_NOT_INITIALIZED;
    return MPL_Wtime_todouble_fn(t, val);
}

int MPL_Wtime_acc(MPL_Time_t *t1, MPL_Time_t *t2, MPL_Time_t *t3)
{
    if (MPL_Wtime_acc_fn == NULL)
        return MPL_TIMER_ERR_NOT_INITIALIZED;
    return MPL_Wtime_acc_fn(t1, t2, t3);
}

int MPL_Wtick(double *wtick)
{
    if (MPL_Wtick_fn == NULL)
        return MPL_TIMER_ERR_NOT_INITIALIZED;
    return MPL_Wtick_fn(wtick);
}

#endif
