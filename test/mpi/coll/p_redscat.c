/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
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

    MPI_Request req;
    MPI_Info info;

    MTest_Init(&argc, &argv);
    comm = MPI_COMM_WORLD;

    MPI_Comm_size(comm, &size);
    MPI_Comm_rank(comm, &rank);

    MPI_Info_create(&info);

    sendbuf = (int *) malloc(size * sizeof(int));
    for (i = 0; i < size; i++)
        sendbuf[i] = rank + i;
    recvcounts = (int *) malloc(size * sizeof(int));
    for (i = 0; i < size; i++)
        recvcounts[i] = 1;

    MPI_Reduce_scatter_init(sendbuf, &recvbuf, recvcounts, MPI_INT, MPI_SUM, comm, info, &req);
    for (i = 0; i < 10; ++i) {
        MPI_Start(&req);
        MPI_Wait(&req, MPI_STATUS_IGNORE);

        sumval = size * rank + ((size - 1) * size) / 2;
        /* recvbuf should be size * (rank + i) */
        if (recvbuf != sumval) {
            err++;
            fprintf(stdout, "Did not get expected value for reduce scatter\n");
            fprintf(stdout, "[%d] Got %d expected %d\n", rank, recvbuf, sumval);
        }

    }

    MPI_Request_free(&req);
    MPI_Info_free(&info);

    free(sendbuf);
    free(recvcounts);
    MTest_Finalize(err);

    return MTestReturnValue(err);
}
