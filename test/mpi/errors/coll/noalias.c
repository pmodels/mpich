/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include <stdio.h>
#include <stdlib.h>
#include "mpi.h"
#include "mpitest.h"

int main( int argc, char *argv[] )
{
    int        err, errs = 0, len, i;
    int        rank = -1, size = -1;
    int       *buf;
    int       *recvbuf;
    char       msg[MPI_MAX_ERROR_STRING];

    MTest_Init( &argc, &argv );
    MPI_Errhandler_set( MPI_COMM_WORLD, MPI_ERRORS_RETURN );

    MPI_Comm_rank( MPI_COMM_WORLD, &rank );
    MPI_Comm_size( MPI_COMM_WORLD, &size );

    buf = malloc(size * sizeof(int));
    recvbuf = malloc(size * sizeof(int));
    for (i = 0; i < size; ++i) {
        buf[i] = i;
        recvbuf[i] = -1;
    }

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

    /* check for aliasing detection in MPI_Gather (tt#1006) */
    err = MPI_Gather(buf, 1, MPI_INT, buf, 1, MPI_INT, 0, MPI_COMM_WORLD);
    if (rank == 0) {
        if (!err) {
            errs++;
            printf( "Did not detect aliased arguments in MPI_Gather\n" );
        }
        else {
            /* Check that we can get a message for this error */
            /* (This works if it does not SEGV or hang) */
            MPI_Error_string( err, msg, &len );
        }
    }
    if (err) {
        /* post a correct MPI_Gather on any processes that got an error earlier */
        err = MPI_Gather(buf, 1, MPI_INT, recvbuf, 1, MPI_INT, 0, MPI_COMM_WORLD);
        if (err) {
            errs++;
            printf("make-up gather failed on rank %d\n", rank);
        }
    }

    /* check for aliasing detection in MPI_Scatter (tt#1006) */
    err = MPI_Scatter(buf, 1, MPI_INT, buf, 1, MPI_INT, 0, MPI_COMM_WORLD);
    if (rank == 0) {
        if (!err) {
            errs++;
            printf( "Did not detect aliased arguments in MPI_Scatter\n" );
        }
        else {
            /* Check that we can get a message for this error */
            /* (This works if it does not SEGV or hang) */
            MPI_Error_string( err, msg, &len );
        }
    }
    if (err) {
        /* post a correct MPI_Scatter on any processes that got an error earlier */
        err = MPI_Scatter(buf, 1, MPI_INT, recvbuf, 1, MPI_INT, 0, MPI_COMM_WORLD);
        if (err) {
            errs++;
            printf("make-up scatter failed on rank %d\n", rank);
        }
    }

    free(recvbuf);
    free(buf);

    MTest_Finalize( errs );
    MPI_Finalize( );
    return 0;
}
