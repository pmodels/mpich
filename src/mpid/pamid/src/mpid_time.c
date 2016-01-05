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

#if MPICH_TIMER_KIND == USE_GETTIMEOFDAY
#warning Compiling mpid/pamid/src/mpid_time.c when MPICH_TIMER_KIND == USE_GETTIMEOFDAY
#elif MPICH_TIMER_KIND != MPIU_TIMER_KIND__DEVICE
#error "Not using DEVICE TIMEBASE"
#else

int MPIDI_PAMID_Timer_is_ready = 0;

int MPID_Wtime( MPID_Time_t *tval )
{
    if (MPIDI_PAMID_Timer_is_ready) {
        *tval = PAMI_Wtime(MPIDI_Client);
        return MPID_TIMER_SUCCESS;
    }
    else
        return MPID_TIMER_ERR_NOT_INITIALIZED;
}
int MPID_Wtick(double *wtick)
{
    if (MPIDI_PAMID_Timer_is_ready) {
        *wtick = PAMIX_Client_query(MPIDI_Client, PAMI_CLIENT_WTICK).value.doubleval;
        return MPID_TIMER_SUCCESS;
    }
    else
        return MPID_TIMER_ERR_NOT_INITIALIZED;
}
int MPID_Wtime_diff( MPID_Time_t *t1, MPID_Time_t *t2, double *diff )
{
    if (MPIDI_PAMID_Timer_is_ready) {
        *diff = *t2 - *t1;
        return MPID_TIMER_SUCCESS;
    }
    else
        return MPID_TIMER_ERR_NOT_INITIALIZED;
}
int MPID_Wtime_todouble( MPID_Time_t *t, double *val )
{
    if (MPIDI_PAMID_Timer_is_ready) {
        *val = *t;
        return MPID_TIMER_SUCCESS;
    }
    else
        return MPID_TIMER_ERR_NOT_INITIALIZED;
}
int MPID_Wtime_acc( MPID_Time_t *t1, MPID_Time_t *t2, MPID_Time_t *t3 )
{
    if (MPIDI_PAMID_Timer_is_ready) {
        *t3 += *t1 - *t2;
        return MPID_TIMER_SUCCESS;
    }
    else
        return MPID_TIMER_ERR_NOT_INITIALIZED;
}

int MPID_Wtime_init( void )
{
    return MPID_TIMER_SUCCESS;
}

#endif
