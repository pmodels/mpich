/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2003 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpi.h"
#include "mpitest.h"
#include <stdlib.h>
#include <stdio.h>

/* Tests Allgather on array of doubles. Same as allgather2 test
 * but without MPI_IN_PLACE. */

int main(int argc, char **argv)
{
    double *vecout, *invec;
    MPI_Comm comm;
    int count, minsize = 2;
    int i, errs = 0;
    int rank, size;

    MTest_Init(&argc, &argv);

    while (MTestGetIntracommGeneral(&comm, minsize, 1)) {
        if (comm == MPI_COMM_NULL)
            continue;
        /* Determine the sender and receiver */
        MPI_Comm_rank(comm, &rank);
        MPI_Comm_size(comm, &size);

        for (count = 1; count < 9000; count = count * 2) {
            invec = (double *) malloc(count * sizeof(double));
            vecout = (double *) malloc(size * count * sizeof(double));

            for (i = 0; i < count; i++) {
                invec[i] = rank * count + i;
            }
            MPI_Allgather(invec, count, MPI_DOUBLE, vecout, count, MPI_DOUBLE, comm);
            for (i = 0; i < count * size; i++) {
                if (vecout[i] != i) {
                    errs++;
                    if (errs < 10) {
                        fprintf(stderr, "vecout[%d]=%d\n", i, (int) vecout[i]);
                    }
                }
            }
            free(invec);
            free(vecout);
        }

        MTestFreeComm(&comm);
    }

    /* Do a zero byte gather */
    MPI_Allgather(MPI_IN_PLACE, -1, MPI_DATATYPE_NULL, NULL, 0, MPI_BYTE, MPI_COMM_WORLD);

    MTest_Finalize(errs);
    return MTestReturnValue(errs);
}
