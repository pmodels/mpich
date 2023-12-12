/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
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

#include "mpitest.h"
#include "mpicolltest.h"

#ifdef MULTI_TESTS
#define run coll_redscat3
int run(const char *arg);
#endif

/* Limit the number of error reports */
#define MAX_ERRORS 10

int run(const char *arg)
{
    int errs = 0;
    int *sendbuf, *recvbuf, *recvcounts;
    int size, rank, i, j, idx, mycount, sumval;
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
    recvcounts = (int *) malloc(size * sizeof(int));
    if (!recvcounts) {
        fprintf(stderr, "Could not allocate %d ints for recvcounts\n", size);
        MPI_Abort(MPI_COMM_WORLD, 1);
    }
    mycount = (1024 * 1024) / size;
    for (i = 0; i < size; i++)
        recvcounts[i] = mycount;
    sendbuf = (int *) malloc(mycount * size * sizeof(int));
    if (!sendbuf) {
        fprintf(stderr, "Could not allocate %d ints for sendbuf\n", mycount * size);
        MPI_Abort(MPI_COMM_WORLD, 1);
    }
    idx = 0;
    for (i = 0; i < size; i++) {
        for (j = 0; j < mycount; j++) {
            sendbuf[idx++] = rank + i;
        }
    }
    recvbuf = (int *) malloc(mycount * sizeof(int));
    if (!recvbuf) {
        fprintf(stderr, "Could not allocate %d ints for recvbuf\n", mycount);
        MPI_Abort(MPI_COMM_WORLD, 1);
    }
    for (i = 0; i < mycount; i++) {
        recvbuf[i] = -1;
    }

    MTest_Reduce_scatter(is_blocking, sendbuf, recvbuf, recvcounts, MPI_INT, MPI_SUM, comm);

    sumval = size * rank + ((size - 1) * size) / 2;
    /* recvbuf should be size * (rank + i) */
    for (i = 0; i < mycount; i++) {
        if (recvbuf[i] != sumval) {
            errs++;
            if (errs < MAX_ERRORS) {
                fprintf(stdout, "Did not get expected value for reduce scatter\n");
                fprintf(stdout, "[%d] Got recvbuf[%d] = %d expected %d\n",
                        rank, i, recvbuf[i], sumval);
            }
        }
    }

#if MTEST_HAVE_MIN_MPI_VERSION(2,2)
    MTest_Reduce_scatter(is_blocking, MPI_IN_PLACE, sendbuf, recvcounts, MPI_INT, MPI_SUM, comm);

    sumval = size * rank + ((size - 1) * size) / 2;
    /* recv'ed values for my process should be size * (rank + i) */
    for (i = 0; i < mycount; i++) {
        if (sendbuf[i] != sumval) {
            errs++;
            if (errs < MAX_ERRORS) {
                fprintf(stdout, "Did not get expected value for reduce scatter (in place)\n");
                fprintf(stdout, "[%d] Got buf[%d] = %d expected %d\n", rank, i, sendbuf[i], sumval);
            }
        }
    }
#endif

    free(sendbuf);
    free(recvbuf);
    free(recvcounts);

    return errs;
}
