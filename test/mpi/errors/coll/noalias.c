/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include <stdio.h>
#include <stdlib.h>
#include "mpi.h"
#include "mpitest.h"
#include "mpicolltest.h"

int main(int argc, char *argv[])
{
    int err, errs = 0, len, i;
    int rank = -1, size = -1;
    int *buf;
    int *recvbuf;
    char msg[MPI_MAX_ERROR_STRING];

    int is_blocking = 1;

    MTestArgList *head = MTestArgListCreate(argc, argv);
    if (MTestArgListGetInt_with_default(head, "nonblocking", 0)) {
        is_blocking = 0;
    }
    MTestArgListDestroy(head);

    MTest_Init(&argc, &argv);
    MPI_Comm_set_errhandler(MPI_COMM_WORLD, MPI_ERRORS_RETURN);

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    buf = malloc(size * sizeof(int));
    recvbuf = malloc(size * sizeof(int));
    for (i = 0; i < size; ++i) {
        buf[i] = i;
        recvbuf[i] = -1;
    }

    err = MTest_Allreduce(is_blocking, buf, buf, 1, MPI_INT, MPI_SUM, MPI_COMM_WORLD);
    if (!err) {
        errs++;
        if (rank == 0)
            printf("Did not detect aliased arguments in MPI_Allreduce\n");
    } else {
        /* Check that we can get a message for this error */
        /* (This works if it does not SEGV or hang) */
        MPI_Error_string(err, msg, &len);
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
    err =
        MTest_Reduce(is_blocking, buf, ((rank == 0) ? buf : NULL), 1, MPI_INT, MPI_SUM, 0,
                     MPI_COMM_WORLD);
    if (rank == 0) {
        if (!err) {
            errs++;
            if (rank == 0)
                printf("Did not detect aliased arguments in MPI_Reduce\n");
        } else {
            /* Check that we can get a message for this error */
            /* (This works if it does not SEGV or hang) */
            MPI_Error_string(err, msg, &len);
        }
    }
    if (err) {
        /* post a correct MPI_Reduce on any processes that got an error earlier */
        err = MTest_Reduce(is_blocking, buf, recvbuf, 1, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);
        if (err) {
            errs++;
            printf("make-up reduce failed on rank %d\n", rank);
        }
    }

    /* this case should _not_ trigger an error, thanks to Kenneth Inghram for
     * reporting this bug in MPICH */
    err =
        MTest_Reduce(is_blocking, ((rank == 0) ? MPI_IN_PLACE : buf), buf, 1, MPI_INT, MPI_SUM, 0,
                     MPI_COMM_WORLD);
    if (err) {
        errs++;
        printf
            ("Incorrectly reported aliased arguments in MPI_Reduce with MPI_IN_PLACE on rank %d\n",
             rank);
        MPI_Abort(MPI_COMM_WORLD, 1);
        printf("FAILED TO MPI_ABORT!!!\n");
    }

    /* check for aliasing detection in MPI_Gather (tt#1006) */
    err = MTest_Gather(is_blocking, buf, 1, MPI_INT, buf, 1, MPI_INT, 0, MPI_COMM_WORLD);
    if (rank == 0) {
        if (!err) {
            errs++;
            printf("Did not detect aliased arguments in MPI_Gather\n");
        } else {
            /* Check that we can get a message for this error */
            /* (This works if it does not SEGV or hang) */
            MPI_Error_string(err, msg, &len);
        }
    }
    if (err) {
        /* post a correct MPI_Gather on any processes that got an error earlier */
        err = MTest_Gather(is_blocking, buf, 1, MPI_INT, recvbuf, 1, MPI_INT, 0, MPI_COMM_WORLD);
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
            printf("Did not detect aliased arguments in MPI_Scatter\n");
        } else {
            /* Check that we can get a message for this error */
            /* (This works if it does not SEGV or hang) */
            MPI_Error_string(err, msg, &len);
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

    MTest_Finalize(errs);
    return MTestReturnValue(errs);
}
