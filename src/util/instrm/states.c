/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*  $Id: states.c,v 1.4 2004/11/29 17:04:49 toonen Exp $
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/*
 * We pass in a variable that will hold the time stamp so that these routines
 * are reentrant.
 */

void MPID_TimerStateBegin( int id, MPID_Time_t *time_stamp )
{
#ifdef HAVE_TIMING 
    MPICH_PerThread_t *p;

    MPID_Wtime( time_stamp );
    MPIR_GetPerThread( &p );
    p->timestamps[id].count++;
#endif
}
void MPID_TimerStateEnd( int id, MPID_Time_t *time_stamp )
{
#ifdef HAVE_TIMING 
    MPICH_PerThread_t *p;
    MPID_Time_t final_time;

    MPID_Wtime( &final_time );
    MPIR_GetPerThread( &p );
    MPID_Wtime_acc( time_stamp, &final_time, &p->timestamps[id].stamp );
#endif
}
