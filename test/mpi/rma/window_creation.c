/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include <stdio.h>
#include <stdlib.h>

#include <mpi.h>
#include "mpitest.h"

#define DATA_NELTS  1000
/* NOTE: with sysv, there is a limit on shared memory IDs */
#define NUM_WIN     500
#define DATA_SZ     (DATA_NELTS*sizeof(int))

int main(int argc, char **argv)
{
    int rank, nproc, i;
    void *base_ptrs[NUM_WIN];
    MPI_Win windows[NUM_WIN];

    MTest_Init(&argc, &argv);

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &nproc);

    if (rank == 0)
        MTestPrintfMsg(1, "Starting MPI window creation test with %d processes\n", nproc);

    /* Perform a pile of window creations */
    for (i = 0; i < NUM_WIN; i++) {
#ifdef USE_WIN_ALLOCATE
        if (rank == 0)
            MTestPrintfMsg(1, " + Allocating window %d\n", i);
        MPI_Win_allocate(DATA_SZ, 1, MPI_INFO_NULL, MPI_COMM_WORLD, &base_ptrs[i], &windows[i]);
#elif defined(USE_WIN_ALLOCATE_NONCONTIG)
        MPI_Info info = MPI_INFO_NULL;
        MPI_Info_create(&info);
        MPI_Info_set(info, "alloc_shared_noncontig", "true");
        if (rank == 0)
            MTestPrintfMsg(1, " + Allocating window (alloc_shared_noncontig=true) %d\n", i);
        MPI_Win_allocate(DATA_SZ, 1, info, MPI_COMM_WORLD, &base_ptrs[i], &windows[i]);
        MPI_Info_free(&info);
#else
        if (rank == 0)
            MTestPrintfMsg(1, " + Creating window %d\n", i);

        MPI_Alloc_mem(DATA_SZ, MPI_INFO_NULL, &base_ptrs[i]);
        MPI_Win_create(base_ptrs[i], DATA_SZ, 1, MPI_INFO_NULL, MPI_COMM_WORLD, &windows[i]);
#endif
    }

    MPI_Barrier(MPI_COMM_WORLD);

    /* Free all the windows */
    for (i = 0; i < NUM_WIN; i++) {
        if (rank == 0)
            MTestPrintfMsg(1, " + Freeing window %d\n", i);

        MPI_Win_free(&windows[i]);
#if !defined(USE_WIN_ALLOCATE) && !defined(USE_WIN_ALLOCATE_NONCONTIG)
        MPI_Free_mem(base_ptrs[i]);
#endif
    }

    MTest_Finalize(0);

    return 0;
}
