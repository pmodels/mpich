/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

#include <sys/time.h>
/* 
 * This file provides a time-limit capability.
 * NOT YET COMPLETE
 */

/* 
   Generate a SIGALRM after the specified number of seconds.
   If seconds is zero, turn off the alarm 
*/
void MPIU_SetTimeout( int seconds )
{
#ifdef HAVE_SETITIMER
    struct itimerval timelimit;
    struct timeval tval;
    struct timeval tzero;

    if (seconds > 0) {
	tzero.tv_sec	      = 0;
	tzero.tv_usec	      = 0;
	tval.tv_sec	      = seconds;
	tval.tv_usec	      = 0;
	timelimit.it_interval = tzero;       /* Only one alarm */
	timelimit.it_value    = tval;
	setitimer( ITIMER_REAL, &timelimit, 0 );
    }
    else {
	tzero.tv_sec	  = 0;
	tzero.tv_usec	  = 0;
	timelimit.it_value	  = tzero;   /* Turn off timer */
	setitimer( ITIMER_REAL, &timelimit, 0 );
    }
    
#elif defined(HAVE_ALARM)
    /* alarm(0) turns off the alarm */
    alarm( seconds );
#else
#error "No timer available"    
#endif
}


