/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

/*
 * Test of reduce scatter block.
 *
 * Each process contributes its rank + the index to the reduction,
 * then receives the ith sum
 *
 * Can be called with any number of processes.
 */

#include "mpitest.h"

#ifdef MULTI_TESTS
#define run coll_red_scat_block
int run(const char *arg);
#endif

int run(const char *arg)
{
    int err = 0;
    int size, rank, i, sumval;
    int *sendbuf;
    int *recvbuf;
    MPI_Comm comm;

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
    for (i = 0; i < size; i++)
        sendbuf[i] = rank + i;

    MPI_Reduce_scatter_block(sendbuf, recvbuf, 1, MPI_INT, MPI_SUM, comm);

    sumval = size * rank + ((size - 1) * size) / 2;
    if (recvbuf[0] != sumval) {
        err++;
        fprintf(stdout, "Did not get expected value for reduce scatter block\n");
        fprintf(stdout, "[%d] Got %d expected %d\n", rank, recvbuf[0], sumval);
    }

    free(sendbuf);

    /* let's try it again with MPI_IN_PLACE this time */
    for (i = 0; i < size; i++)
        recvbuf[i] = rank + i;

    MPI_Reduce_scatter_block(MPI_IN_PLACE, recvbuf, 1, MPI_INT, MPI_SUM, comm);

    sumval = size * rank + ((size - 1) * size) / 2;
    if (recvbuf[0] != sumval) {
        err++;
        fprintf(stdout, "Did not get expected value for reduce scatter block\n");
        fprintf(stdout, "[%d] Got %d expected %d\n", rank, recvbuf[0], sumval);
    }
    free(recvbuf);
#endif

    return err;
}
