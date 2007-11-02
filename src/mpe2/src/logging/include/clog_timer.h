/*
   (C) 2001 by Argonne National Laboratory.
       See COPYRIGHT in top-level directory.
*/
#if !defined( _CLOG_TIMER )
#define _CLOG_TIMER

#define CLOG_Time_t           double
#define CLOG_TIME_MPI_TYPE    MPI_DOUBLE

/* where timestamps come from */

void         CLOG_Timer_start( void );
CLOG_Time_t  CLOG_Timer_get( void );

#endif  /* of _CLOG_TIME */
