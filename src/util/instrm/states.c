/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
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
    MPIU_THREADPRIV_DECL;
    MPIU_THREADPRIV_GET;

    MPID_Wtime( time_stamp );
    MPIU_THREADPRIV_FIELD(timestamps[id].count) = 
	MPIU_THREADPRIV_FIELD(timestamps[id].count) + 1;
#endif
}
void MPID_TimerStateEnd( int id, MPID_Time_t *time_stamp )
{
#ifdef HAVE_TIMING 
    MPID_Time_t final_time;
    MPIU_THREADPRIV_DECL;
    MPIU_THREADPRIV_GET;

    MPID_Wtime( &final_time );
    MPID_Wtime_acc( time_stamp, &final_time, 
		    &MPIU_THREADPRIV_FIELD(timestamps[id].stamp) );
#endif
}
