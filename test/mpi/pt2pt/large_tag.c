/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2010 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include "mpitest.h"

/* Tests send/receive with large tag values. It mimics the usage of tags in
 * some MPI-based libraries (e.g., PETSc). */

#define ITER 5
#define BUF_COUNT (16)

int recvbuf[BUF_COUNT], sendbuf[BUF_COUNT];

int main(int argc, char *argv[])
{
    int x, size, rank, errs = 0;
    void *tag_ub_val = NULL;
    int tag = 0, flag = 0;

    MTest_Init(&argc, &argv);

    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    /* perform test only with two or more processes */
    if (size < 2)
        goto exit;

    /* query the upper bound of tag value */
    MPI_Comm_get_attr(MPI_COMM_WORLD, MPI_TAG_UB, &tag_ub_val, &flag);
    tag = *(int *) tag_ub_val;
    if (!flag || tag < 32767) {
        fprintf(stdout, "%d -- Incorrect MPI_TAG_UB, found flag=%d, tag=%d\n", rank, flag, tag);
        fflush(stdout);
        errs++;
        goto exit;
    }

    /* send/receive with large tags from the upper bound */
    for (x = 0; x < ITER; x++) {
        int i;

        if (rank == 0) {
            /* reset send buffer */
            for (i = 0; i < BUF_COUNT; i++)
                sendbuf[i] = x * BUF_COUNT + i;

            MPI_Send(sendbuf, BUF_COUNT, MPI_INT, 1, tag, MPI_COMM_WORLD);
        } else if (rank == 1) {
            MPI_Recv(recvbuf, BUF_COUNT, MPI_INT, 0, tag, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

            /* check correctness of received data */
            for (i = 0; i < BUF_COUNT; i++) {
                int expected_val = x * BUF_COUNT + i;
                if (recvbuf[i] != expected_val) {
                    errs++;
                    fprintf(stdout, "%d -- Received %d at index %d with tag %d, expected %d\n",
                            rank, recvbuf[i], i, tag, expected_val);
                    fflush(stdout);
                }
            }
        }
        tag--;
    }

  exit:
    MTest_Finalize(errs);

    return MTestReturnValue(errs);
}
