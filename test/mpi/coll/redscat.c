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

#include "mpitest.h"
#include "mpicolltest.h"

#ifdef MULTI_TESTS
#define run coll_redscat
int run(const char *arg);
#endif

int run(const char *arg)
{
    int err = 0;
    int *sendbuf, recvbuf, *recvcounts;
    int size, rank, i, sumval;
    MPI_Comm comm;

    int is_blocking = 1;

    MTestArgList *head = MTestArgListCreate_arg(arg);
    if (MTestArgListGetInt_with_default(head, "nonblocking", 0)) {
        is_blocking = 0;
    }
    MTestArgListDestroy(head);

    comm = MPI_COMM_WORLD;

    MPI_Comm_size(comm, &size);
    MPI_Comm_rank(comm, &rank);
    sendbuf = (int *) malloc(size * sizeof(int));
    for (i = 0; i < size; i++)
        sendbuf[i] = rank + i;
    recvcounts = (int *) malloc(size * sizeof(int));
    for (i = 0; i < size; i++)
        recvcounts[i] = 1;

    MTest_Reduce_scatter(is_blocking, sendbuf, &recvbuf, recvcounts, MPI_INT, MPI_SUM, comm);

    sumval = size * rank + ((size - 1) * size) / 2;
/* recvbuf should be size * (rank + i) */
    if (recvbuf != sumval) {
        err++;
        fprintf(stdout, "Did not get expected value for reduce scatter\n");
        fprintf(stdout, "[%d] Got %d expected %d\n", rank, recvbuf, sumval);
    }

    free(sendbuf);
    free(recvcounts);

    return err;
}
