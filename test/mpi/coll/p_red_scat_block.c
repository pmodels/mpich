/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

/*
 * Test of persistent reduce scatter block.
 *
 * Each process contributes its rank + the index to the reduction,
 * then receives the ith sum
 *
 * Can be called with any number of processes.
 */

#include "mpi.h"
#include "mpitest.h"
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv)
{
    int err = 0;
    int size, rank, i, sumval;
    int *sendbuf;
    int *recvbuf;
    MPI_Comm comm;

    MTest_Init(&argc, &argv);
    comm = MPI_COMM_WORLD;

    MPI_Comm_size(comm, &size);
    MPI_Comm_rank(comm, &rank);

#if MTEST_HAVE_MIN_MPI_VERSION(2,2)
    /* MPI_Reduce_scatter block was added in MPI-2.2 */
    sendbuf = (int *) malloc(size * sizeof(int));
    recvbuf = (int *) malloc(size * sizeof(int));
    if (!sendbuf || !recvbuf) {
        err++;
        fprintf(stderr, "unable to allocate send/recv buffers, aborting");
        MPI_Abort(MPI_COMM_WORLD, 1);
    }
    MPI_Info info;
    MPI_Info_create(&info);
    MPI_Request req;

    for (i = 0; i < size; i++)
        sendbuf[i] = rank + i;

    MPI_Reduce_scatter_block_init(sendbuf, recvbuf, 1, MPI_INT, MPI_SUM, comm, info, &req);

    for (i = 0; i < 6; ++i) {

        MPI_Start(&req);
        MPI_Wait(&req, MPI_STATUS_IGNORE);

        sumval = size * rank + ((size - 1) * size) / 2;
        if (recvbuf[0] != sumval) {
            err++;
            fprintf(stdout, "Did not get expected value for reduce scatter block\n");
            fprintf(stdout, "[%d] Got %d expected %d\n", rank, recvbuf[0], sumval);
        }
    }
    MPI_Request_free(&req);
    free(sendbuf);

    /* let's try it again with MPI_IN_PLACE this time */
    MPI_Reduce_scatter_block_init(MPI_IN_PLACE, recvbuf, 1, MPI_INT, MPI_SUM, comm, info, &req);

    for (i = 0; i < 10; ++i) {
        for (int j = 0; j < size; j++)
            recvbuf[j] = rank + j;

        MPI_Start(&req);
        MPI_Wait(&req, MPI_STATUS_IGNORE);

        sumval = size * rank + ((size - 1) * size) / 2;
        if (recvbuf[0] != sumval) {
            err++;
            fprintf(stdout, "Did not get expected value for reduce scatter block\n");
            fprintf(stdout, "[%d] Got %d expected %d\n", rank, recvbuf[0], sumval);
        }
    }
    MPI_Request_free(&req);
    free(recvbuf);

    MPI_Info_free(&info);
#endif
    MTest_Finalize(err);

    return MTestReturnValue(err);
}
