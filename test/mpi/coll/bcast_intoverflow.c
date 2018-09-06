/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "mpi.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include "mpitest.h"

#define GB (size_t)(1<<30)
#define PAYLOAD (4UL*GB)

int main(int argc, char **argv)
{
    int *buffer;
    int my_rank;                /* rank in world collective */
    char msg[256];              /* msg sent to test diagnostics */
    int counts[3];
    int count;
    int errs = 0;
    size_t i;

    MTest_Init(&argc, &argv);

    MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);

/* Buffer Allocation */
    if (!(buffer = (int *) malloc(PAYLOAD))) {
        fprintf(stderr, "malloc() failed\n");
        MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
    }

/* Initialize buffers */
    if (my_rank == 0) {
        for (i = 0; i < (PAYLOAD / sizeof(int)); i++) {
            buffer[i] = i % INT_MAX;
        }
    }

    counts[0] = (1 << 28);
    counts[1] = (1 << 29);
    counts[2] = (1 << 30);

    for (i = 0; i < 3; i++) {
        count = counts[i];
        if (my_rank != 0)
            memset(buffer, 0, (count * sizeof(int)));

        MPI_Barrier(MPI_COMM_WORLD);
#ifdef DEBUG
        if (my_rank == 0)
            printf("performing bcast with %d ints\n", count);
#endif
        /* Perform Broadcast */
        MPI_Bcast(buffer, count, MPI_INT, 0, MPI_COMM_WORLD);
        for (i = 0; i < count; i++) {
            if (buffer[i] != (i % INT_MAX)) {
                errs++;
                if (errs > 0) {
                    printf("Error: Rank=%d buffer[%zu] = %d, but %d is expected", my_rank, i,
                           buffer[i], (int) (i % INT_MAX) /*always int */);
                    fflush(stdout);
                }
            }
        }
    }

    free(buffer);

    MTest_Finalize(errs);
    return MTestReturnValue(errs);
}
