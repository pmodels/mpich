/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiu_timer.h"

MPL_SUPPRESS_OSX_HAS_NO_SYMBOLS_WARNING;

#if MPICH_TIMER_KIND == MPIU_TIMER_KIND__DEVICE

int (*MPIU_Wtime_fn)(MPIU_Time_t *timeval) = NULL;
int (*MPIU_Wtime_diff_fn)(MPIU_Time_t *t1, MPIU_Time_t *t2, double *diff) = NULL;
int (*MPIU_Wtime_acc_fn)(MPIU_Time_t *t1, MPIU_Time_t *t2, MPIU_Time_t *t3) = NULL;
int (*MPIU_Wtime_todouble_fn)(MPIU_Time_t *timeval, double *seconds) = NULL;
int (*MPIU_Wtick_fn)(double *tick) = NULL;

int MPIU_Wtime(MPIU_Time_t *timeval)
{
    if (MPIU_Wtime_fn == NULL)
        return MPIU_TIMER_ERR_NOT_INITIALIZED;
    return MPIU_Wtime_fn(timeval);
}

int MPIU_Wtime_diff(MPIU_Time_t *t1, MPIU_Time_t *t2, double *diff)
{
    if (MPIU_Wtime_diff_fn == NULL)
        return MPIU_TIMER_ERR_NOT_INITIALIZED;
    return MPIU_Wtime_diff_fn(t1, t2, diff);
}

int MPIU_Wtime_todouble(MPIU_Time_t *t, double *val)
{
    if (MPIU_Wtime_todouble_fn == NULL)
        return MPIU_TIMER_ERR_NOT_INITIALIZED;
    return MPIU_Wtime_todouble_fn(t, val);
}

int MPIU_Wtime_acc(MPIU_Time_t *t1, MPIU_Time_t *t2, MPIU_Time_t *t3)
{
    if (MPIU_Wtime_acc_fn == NULL)
        return MPIU_TIMER_ERR_NOT_INITIALIZED;
    return MPIU_Wtime_acc_fn(t1, t2, t3);
}

int MPIU_Wtick(double *wtick)
{
    if (MPIU_Wtick_fn == NULL)
        return MPIU_TIMER_ERR_NOT_INITIALIZED;
    return MPIU_Wtick_fn(wtick);
}

#endif
