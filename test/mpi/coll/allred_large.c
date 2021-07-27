/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  Copyright (C) by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 */

#include <stdio.h>
#include "mpi.h"
#include "mpitest.h"
#define NUM_REP 10
/*
 * This example should be run with multiple ranks and tests the ability of the
 * implementation to handle large message Allreduce.
 */
int main(int argc, char **argv)
{
    int numprocs, myid, i, j, count;
    int *buf;
    int errs = 0;
    int result = 0;

    MTest_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &numprocs);
    MPI_Comm_rank(MPI_COMM_WORLD, &myid);

    for (i = 0; i < NUM_REP; i++) {
        count = 1024 * (i + 1);
        buf = (int *) malloc(count * sizeof(int));
        if (!buf) {
            MPI_Abort(MPI_COMM_WORLD, 1);
        }

        for (j = 0; j < count; j++) {
            buf[j] = myid + 1;
        }

        MPI_Allreduce(MPI_IN_PLACE, buf, count, MPI_INT, MPI_SUM, MPI_COMM_WORLD);

        if (myid == 0) {
            result = numprocs * (numprocs + 1) / 2;
            for (j = 0; j < count; j++) {
                if (buf[j] != result) {
                    errs++;
                    if (errs < 10) {
                        fprintf(stderr, "buf[%d] = %d expected %d\n", j, buf[j], result);
                    }
                }
            }
        }
        free(buf);
    }
    MTest_Finalize(0);

    return 0;
}
