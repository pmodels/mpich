/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
/*
 * Test of reduce scatter.
 *
 * Each processor contributes its rank + the index to the reduction,
 * then receives the ith sum
 *
 * Can be called with any number of processors.
 */

#include "mpi.h"
#include <stdio.h>
#include <stdlib.h>
#include "mpicolltest.h"
#include "mpitest.h"

int main(int argc, char **argv)
{
    int err = 0;
    int *sendbuf, recvbuf, *recvcounts;
    int size, rank, i, sumval;
    MPI_Comm comm;


    MTest_Init(&argc, &argv);
    comm = MPI_COMM_WORLD;

    MPI_Comm_size(comm, &size);
    MPI_Comm_rank(comm, &rank);
    sendbuf = (int *) malloc(size * sizeof(int));
    for (i = 0; i < size; i++)
        sendbuf[i] = rank + i;
    recvcounts = (int *) malloc(size * sizeof(int));
    for (i = 0; i < size; i++)
        recvcounts[i] = 1;

    MTest_Reduce_scatter(sendbuf, &recvbuf, recvcounts, MPI_INT, MPI_SUM, comm);

    sumval = size * rank + ((size - 1) * size) / 2;
/* recvbuf should be size * (rank + i) */
    if (recvbuf != sumval) {
        err++;
        fprintf(stdout, "Did not get expected value for reduce scatter\n");
        fprintf(stdout, "[%d] Got %d expected %d\n", rank, recvbuf, sumval);
    }

    free(sendbuf);
    free(recvcounts);
    MTest_Finalize(err);

    return MTestReturnValue(err);
}
