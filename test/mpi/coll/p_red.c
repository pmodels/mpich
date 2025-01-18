/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

/* This is a very weak sanity test that persistent Reduce collectives specified by
 * proposed MPI-4 is present in the library and take arguments as expected.  This test
 * does not check for progress or matching issues. */

#include "mpitest.h"

#ifdef MULTI_TESTS
#define run coll_p_red
int run(const char *arg);
#endif

#define NUM_INTS (2)

#define my_assert(cond_)                                                  \
    do {                                                                  \
        if (!(cond_)) {                                                   \
            fprintf(stderr, "assertion (%s) failed, aborting\n", #cond_); \
            MPI_Abort(MPI_COMM_WORLD, 1);                                 \
        }                                                                 \
    } while (0)

int run(const char *arg)
{
    int errs = 0;
    int i;
    int rank, size;
    int *sbuf = NULL;
    int *rbuf = NULL;
    MPI_Comm comm;
    int count = 10;
    int j = 0;
    int root = 1;

    comm = MPI_COMM_WORLD;

    MPI_Comm_size(comm, &size);
    MPI_Comm_rank(comm, &rank);

    sbuf = malloc(NUM_INTS * sizeof(int));
    rbuf = malloc(NUM_INTS * sizeof(int));
    my_assert(sbuf);
    my_assert(rbuf);

    for (i = 0; i < NUM_INTS; ++i) {
        sbuf[i] = rank + i;
    }

    MPI_Info info;
    MPI_Info_create(&info);
    MPI_Request req;

    MPI_Reduce_init(sbuf, rbuf, NUM_INTS, MPI_INT, MPI_SUM, root, comm, info, &req);

    for (i = 0; i < count; i++) {
        MPI_Start(&req);
        MPI_Wait(&req, MPI_STATUS_IGNORE);
        /* Check the results */
        if (rank == root) {
            for (j = 0; j < NUM_INTS; j++) {
                int result = j * size + (size * (size - 1)) / 2;
                if (rbuf[j] != result) {
                    errs++;
                    if (errs < 10) {
                        fprintf(stderr, "rbuf[%d] = %d expected %d\n", j, rbuf[j], result);
                    }
                }
            }
        }
    }
    MPI_Request_free(&req);
    MPI_Info_free(&info);

    free(sbuf);
    free(rbuf);

    return errs;
}
