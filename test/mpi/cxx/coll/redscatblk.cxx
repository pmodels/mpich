/* -*- Mode: C++; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2014 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
/*
 * Test of reduce scatter with large data (needed in MPICH to trigger the
 * long-data algorithm)
 *
 * Each processor contributes its rank + the index to the reduction,
 * then receives the ith sum
 *
 * Can be called with any number of processors.
 */

#include "mpi.h"
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv)
{
    int err = 0;
    int *sendbuf, *recvbuf;
    int size, rank, i, j, idx, mycount, sumval;

    MPI::Init();

    size = MPI::COMM_WORLD.Get_size();
    rank = MPI::COMM_WORLD.Get_rank();

    mycount = (1024 * 1024) / size;

    sendbuf = new int[mycount * size];
    idx = 0;
    for (i = 0; i < size; i++) {
        for (j = 0; j < mycount; j++) {
            sendbuf[idx++] = rank + i;
        }
    }
    recvbuf = new int[mycount];

    MPI::COMM_WORLD.Reduce_scatter_block(sendbuf, recvbuf, mycount, MPI::INT, MPI::SUM);

    sumval = size * rank + ((size - 1) * size) / 2;
    /* recvbuf should be size * (rank + i) */
    for (i = 0; i < mycount; i++) {
        if (recvbuf[i] != sumval) {
            err++;
            fprintf(stdout, "Did not get expected value for reduce scatter\n");
            fprintf(stdout, "[%d] Got %d expected %d\n", rank, recvbuf[i], sumval);
        }
    }

    MPI::COMM_WORLD.Reduce_scatter_block(MPI_IN_PLACE, sendbuf, mycount, MPI::INT, MPI::SUM);

    sumval = size * rank + ((size - 1) * size) / 2;
    /* recv'ed values for my process should be size * (rank + i) */
    for (i = 0; i < mycount; i++) {
        if (sendbuf[rank * mycount + i] != sumval) {
            err++;
            fprintf(stdout, "Did not get expected value for reduce scatter (in place)\n");
            fprintf(stdout, "[%d] Got %d expected %d\n", rank, sendbuf[rank * mycount + i], sumval);
        }
    }

    delete [] sendbuf;
    delete [] recvbuf;

    MPI_Finalize();

    if (err == 0 && rank == 0)
        printf(" No Errors\n");

    return 0;
}
