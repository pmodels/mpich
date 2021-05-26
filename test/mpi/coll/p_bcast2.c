/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2011 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

/* A basic test of persistent broadcast collective operation specified by the
 * propsoed MPI-4 standard.  It only exercises the intracommunicator functionality,
 * and only transmits/receives simple integer types with relatively small counts. */

#include "mpi.h"
#include "mpitest.h"
#include <stdlib.h>
#include <stdio.h>

#define COUNT (10)
#define PRIME (17)

#define my_assert(cond_)                                                  \
    do {                                                                  \
        if (!(cond_)) {                                                   \
            fprintf(stderr, "assertion (%s) failed, aborting\n", #cond_); \
            MPI_Abort(MPI_COMM_WORLD, 1);                                 \
        }                                                                 \
    } while (0)

int main(int argc, char **argv)
{
    int i;
    int errs = 0;
    int rank, size;
    int *buf = NULL;
    signed char *buf_alias = NULL;

    MTest_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    buf = malloc(COUNT * size * sizeof(int));

    MPI_Info info;
    MPI_Info_create(&info);
    MPI_Request req;

    for (i = 0; i < COUNT; ++i) {
        if (rank == 0) {
            buf[i] = i;
        } else {
            buf[i] = 0xdeadbeef;
        }
    }

    MPI_Bcast_init(buf, COUNT, MPI_INT, 0, MPI_COMM_WORLD, info, &req);
    MPI_Start(&req);
    MPI_Wait(&req, MPI_STATUS_IGNORE);
    MPI_Request_free(&req);

    for (i = 0; i < COUNT; ++i) {
        if (buf[i] != i)
            printf("buf[%d]=%d i=%d\n", i, buf[i], i);
        my_assert(buf[i] == i);
    }

    /* Testing broadcast again, but designed to stress scatter/allgather impls */
    buf_alias = (signed char *) buf;
    my_assert(COUNT * size * sizeof(int) > PRIME);      /* sanity */
    for (i = 0; i < PRIME; ++i) {
        if (rank == 0)
            buf_alias[i] = i;
        else
            buf_alias[i] = 0xdb;
    }
    for (i = PRIME; i < COUNT * size * sizeof(int); ++i) {
        buf_alias[i] = 0xbf;
    }
    MPI_Bcast_init(buf_alias, PRIME, MPI_SIGNED_CHAR, 0, MPI_COMM_WORLD, info, &req);
    MPI_Start(&req);
    MPI_Wait(&req, MPI_STATUS_IGNORE);
    MPI_Request_free(&req);
    for (i = 0; i < PRIME; ++i) {
        if (buf_alias[i] != i) {
            errs++;
            if (errs < 10) {
                fprintf(stderr, "buf_alias[%d]=%d i=%d\n", i, buf_alias[i], i);
            }
        }
        my_assert(buf_alias[i] == i);
    }

    free(buf);
    MPI_Info_free(&info);

    MTest_Finalize(errs);
    return MTestReturnValue(errs);
}
