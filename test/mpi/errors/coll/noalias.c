/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include <stdio.h>
#include "mpi.h"

int main( int argc, char *argv[] )
{
    int        err, errs = 0, len;
    int        buf[1], rank;
    int        recvbuf[1];
    char       msg[MPI_MAX_ERROR_STRING];

    MTest_Init( &argc, &argv );
    MPI_Errhandler_set( MPI_COMM_WORLD, MPI_ERRORS_RETURN );

    MPI_Comm_rank( MPI_COMM_WORLD, &rank );
    buf[0] = 1;
    err = MPI_Allreduce( buf, buf, 1, MPI_INT, MPI_SUM, MPI_COMM_WORLD );
    if (!err) {
	errs++;
	if (rank == 0) 
	    printf( "Did not detect aliased arguments in MPI_Allreduce\n" );
    }
    else {
	/* Check that we can get a message for this error */
	/* (This works if it does not SEGV or hang) */
	MPI_Error_string( err, msg, &len );
    }

    /* This case is a bit stranger than the MPI_Allreduce case above, because
     * the recvbuf argument is only relevant at the root.  So without an extra
     * communication step to return errors everywhere, it will be typical for
     * rank 0 (the root) to return an error and all other ranks will return
     * MPI_SUCCESS.  In many implementations this can leave the non-root
     * processes hung or yield unmatched unexpected messages on the root.  So we
     * do our best to carry on in this case by posting a second non-erroneous
     * MPI_Reduce on any process that got back an error from the intentionally
     * erroneous MPI_Reduce. */
    err = MPI_Reduce( buf, ((rank == 0) ? buf : NULL), 1, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD );
    if (rank == 0) {
        if (!err) {
            errs++;
            if (rank == 0)
                printf( "Did not detect aliased arguments in MPI_Reduce\n" );
        }
        else {
            /* Check that we can get a message for this error */
            /* (This works if it does not SEGV or hang) */
            MPI_Error_string( err, msg, &len );
        }
    }
    if (err) {
        /* post a correct MPI_Reduce on any processes that got an error earlier */
        err = MPI_Reduce( buf, recvbuf, 1, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD );
        if (err) {
            errs++;
            printf("make-up reduce failed on rank %d\n", rank);
        }
    }

    /* this case should _not_ trigger an error, thanks to Kenneth Inghram for
     * reporting this bug in MPICH2 */
    err = MPI_Reduce( ((rank == 0) ? MPI_IN_PLACE : buf), buf, 1, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD );
    if (err) {
        errs++;
        printf("Incorrectly reported aliased arguments in MPI_Reduce with MPI_IN_PLACE on rank %d\n", rank);
        MPI_Abort(MPI_COMM_WORLD, 1);
        printf("FAILED TO MPI_ABORT!!!\n");
    }


    MTest_Finalize( errs );
    MPI_Finalize( );
    return 0;
}
