/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include <stdio.h>
#include "mpi.h"
#include "mpitest.h"

/*
 * This program checks that MPI_Irecv and MPI_Recv correctly handle
 * a source of MPI_PROC_NULL
 */

int main(int argc, char **argv)
{
    int flag;
    int errs = 0;
    MPI_Status status;
    MPI_Request req;

    MTest_Init(&argc, &argv);

    MPI_Irecv(NULL, 0, MPI_DATATYPE_NULL, MPI_PROC_NULL, 10, MPI_COMM_WORLD, &req);
    MPI_Waitall(1, &req, &status);
    if (status.MPI_SOURCE != MPI_PROC_NULL) {
        printf("Status.MPI_SOURCE was %d, should be MPI_PROC_NULL\n", status.MPI_SOURCE);
        errs++;
    }
    if (status.MPI_TAG != MPI_ANY_TAG) {
        printf("Status.MPI_TAG was %d, should be MPI_ANY_TAGL\n", status.MPI_TAG);
        errs++;
    }

    /* If Irecv failed, Recv is likely to as well.  Avoid a possible hang
     * by testing Recv only if Irecv test passed */
    if (errs == 0) {
        MPI_Recv(NULL, 0, MPI_DATATYPE_NULL, MPI_PROC_NULL, 10, MPI_COMM_WORLD, &status);
        if (status.MPI_SOURCE != MPI_PROC_NULL) {
            printf("Status.MPI_SOURCE was %d, should be MPI_PROC_NULL\n", status.MPI_SOURCE);
            errs++;
        }
        if (status.MPI_TAG != MPI_ANY_TAG) {
            printf("Status.MPI_TAG was %d, should be MPI_ANY_TAGL\n", status.MPI_TAG);
            errs++;
        }
    }

    MTest_Finalize(errs);
    return MTestReturnValue(errs);
}
