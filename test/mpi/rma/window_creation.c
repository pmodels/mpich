/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include <stdio.h>
#include <stdlib.h>

#include <mpi.h>

#define DATA_NELTS  1000
#define NUM_WIN     1000
#define DATA_SZ     (DATA_NELTS*sizeof(int))

static int verbose = 0;

int main(int argc, char **argv)
{
    int rank, nproc, i;
    void *base_ptrs[NUM_WIN];
    MPI_Win windows[NUM_WIN];

    MPI_Init(&argc, &argv);

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &nproc);

    if (rank == 0)
        if (verbose)
            printf("Starting MPI window creation test with %d processes\n", nproc);

    /* Perform a pile of window creations */
    for (i = 0; i < NUM_WIN; i++) {
        if (rank == 0)
            if (verbose)
                printf(" + Creating window %d\n", i);

        MPI_Alloc_mem(DATA_SZ, MPI_INFO_NULL, &base_ptrs[i]);
        MPI_Win_create(base_ptrs[i], DATA_SZ, 1, MPI_INFO_NULL, MPI_COMM_WORLD, &windows[i]);
    }

    MPI_Barrier(MPI_COMM_WORLD);

    /* Free all the windows */
    for (i = 0; i < NUM_WIN; i++) {
        if (rank == 0)
            if (verbose)
                printf(" + Freeing window %d\n", i);

        MPI_Win_free(&windows[i]);
        MPI_Free_mem(base_ptrs[i]);
    }

    if (rank == 0)
        printf(" No Errors\n");

    MPI_Finalize();

    return 0;
}
