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
#elif MPICH_TIMER_KIND != USE_DEVICE
#error "Not using DEVICE TIMEBASE"
#else


void MPID_Wtime( MPID_Time_t *tval )
{
  *tval = PAMI_Wtime(MPIDI_Client);
}
double MPID_Wtick()
{
  return PAMIX_Client_query(MPIDI_Client, PAMI_CLIENT_WTICK).value.doubleval;
}
void MPID_Wtime_diff( MPID_Time_t *t1, MPID_Time_t *t2, double *diff )
{
  *diff = *t2 - *t1;
}
void MPID_Wtime_todouble( MPID_Time_t *t, double *val )
{
  *val = *t;
}
void MPID_Wtime_acc( MPID_Time_t *t1, MPID_Time_t *t2, MPID_Time_t *t3 )
{
  *t3 += *t1 - *t2;
}
/*
  Return Values:
  0 on success.  -1 on Failure.  1 means that the timer may not be used
  until after MPID_Init completes.  This allows the device to set up the
  timer (first needed for Blue Gene support).
*/
int MPID_Wtime_init( void )
{
  return 1;
}

#endif
