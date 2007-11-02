/*
   (C) 2001 by Argonne National Laboratory.
       See COPYRIGHT in top-level directory.
*/
#include "mpe_logging_conf.h"

#if defined( STDC_HEADERS ) || defined( HAVE_STDIO_H )
#include <stdio.h>
#endif

#if !defined( CLOG_NOMPI )
#include "mpi.h"
#else
#include "mpi_null.h"
#endif /* Endof if !defined( CLOG_NOMPI ) */

#include "clog_util.h"
#include "clog_timer.h"

static CLOG_Time_t  clog_time_offset;


void CLOG_Timer_start( void )
{
    CLOG_Time_t   local_time;

    if ( CLOG_Util_is_MPIWtime_synchronized() == CLOG_BOOL_TRUE ) {
        /*  Clocks are synchronized  */
        local_time = PMPI_Wtime();
        PMPI_Allreduce( &local_time, &clog_time_offset, 1, CLOG_TIME_MPI_TYPE,
                        MPI_MAX, MPI_COMM_WORLD );
    }
    else { /*  Clocks are NOT synchronized  */
        clog_time_offset = PMPI_Wtime();
    }
}

CLOG_Time_t  CLOG_Timer_get( void )
{
    return ( PMPI_Wtime() - clog_time_offset );
}
