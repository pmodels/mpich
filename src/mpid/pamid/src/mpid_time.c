/* begin_generated_IBM_copyright_prolog                             */
/*                                                                  */
/* This is an automatically generated copyright prolog.             */
/* After initializing,  DO NOT MODIFY OR MOVE                       */
/*  --------------------------------------------------------------- */
/* Licensed Materials - Property of IBM                             */
/* Blue Gene/Q 5765-PER 5765-PRP                                    */
/*                                                                  */
/* (C) Copyright IBM Corp. 2011, 2012 All Rights Reserved           */
/* US Government Users Restricted Rights -                          */
/* Use, duplication, or disclosure restricted                       */
/* by GSA ADP Schedule Contract with IBM Corp.                      */
/*                                                                  */
/*  --------------------------------------------------------------- */
/*                                                                  */
/* end_generated_IBM_copyright_prolog                               */
/*  (C)Copyright IBM Corp.  2007, 2011  */
/**
 * \file src/mpid_time.c
 * \brief Devince interface between MPI_Wtime() and PAMI_Wtime()
 */
#include <mpidimpl.h>

int MPIDI_PAMID_Timer_is_ready = 0;

static int wtime(MPL_time_t *tval)
{
    if (MPIDI_PAMID_Timer_is_ready) {
        *((MPID_Time_t *) tval) = PAMI_Wtime(MPIDI_Client);
        return MPID_TIMER_SUCCESS;
    }
    else
        return MPID_TIMER_ERR_NOT_INITIALIZED;
}

int MPID_Wtime(MPID_Time_t *tval)
{
    return wtime((MPL_time_t *) tval);
}

static int wtick(double *wtick)
{
    if (MPIDI_PAMID_Timer_is_ready) {
        *((double *) wtick) = PAMIX_Client_query(MPIDI_Client, PAMI_CLIENT_WTICK).value.doubleval;
        return MPID_TIMER_SUCCESS;
    }
    else
        return MPID_TIMER_ERR_NOT_INITIALIZED;
}

int MPID_Wtick(double *tick)
{
    return wtick((MPL_time_t *) tick);
}

static int wtime_diff(MPL_time_t *t1, MPL_time_t *t2, double *diff)
{
    if (MPIDI_PAMID_Timer_is_ready) {
        *diff = *((MPID_Time_t *) t2) - *((MPID_Time_t *) t1);
        return MPID_TIMER_SUCCESS;
    }
    else
        return MPID_TIMER_ERR_NOT_INITIALIZED;
}

int MPID_Wtime_diff(MPID_Time_t *t1, MPID_Time_t *t2, double *diff)
{
    return wtime_diff((MPL_time_t *) t1, (MPL_time_t *) t2, diff);
}

static int wtime_todouble(MPL_time_t *t, double *val)
{
    if (MPIDI_PAMID_Timer_is_ready) {
        *val = *((MPID_Time_t *) t);
        return MPID_TIMER_SUCCESS;
    }
    else
        return MPID_TIMER_ERR_NOT_INITIALIZED;
}

int MPID_Wtime_todouble(MPID_Time_t *t, double *val)
{
    return wtime_todouble((MPL_time_t *) t, val);
}

static int wtime_acc(MPL_time_t *t1, MPL_time_t *t2, MPL_time_t *t3)
{
    if (MPIDI_PAMID_Timer_is_ready) {
        *((MPID_Time_t *) t3) += *((MPID_Time_t *) t1) - *((MPID_Time_t *) t2);
        return MPID_TIMER_SUCCESS;
    }
    else
        return MPID_TIMER_ERR_NOT_INITIALIZED;
}

int MPID_Wtime_acc(MPID_Time_t *t1, MPID_Time_t *t2, MPID_Time_t *t3)
{
    return wtime_acc((MPL_time_t *) t1, (MPL_time_t *) t2, (MPL_time_t *) t3);
}

int MPID_Wtime_init( void )
{
    MPL_wtime_fn = wtime;
    MPL_wtick_fn = wtick;
    MPL_wtime_diff_fn = wtime_diff;
    MPL_wtime_todouble_fn = wtime_todouble;
    MPL_wtime_acc_fn = wtime_acc;

    return MPID_TIMER_SUCCESS;
}

#endif
