/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include <stdio.h>
#include <stdlib.h>

#include <mpi.h>
#include "mpitest.h"

#define DATA_NELTS  1000
#define NUM_WIN     1000
#define DATA_SZ     (DATA_NELTS*sizeof(int))

int main(int argc, char **argv)
{
    int rank, nproc, i;
    void *base_ptrs[NUM_WIN];
    MPI_Win window;

    MTest_Init(&argc, &argv);

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &nproc);

    if (rank == 0)
        MTestPrintfMsg(1, "Starting MPI dynamic window attach test with %d processes\n", nproc);

    MPI_Info info = MPI_INFO_NULL;
#ifdef USE_INFO_COLL_ATTACH
    MPI_Info_create(&info);
    MPI_Info_set(info, "coll_attach", "true");
#endif
    MPI_Win_create_dynamic(info, MPI_COMM_WORLD, &window);
#ifdef USE_INFO_COLL_ATTACH
    MPI_Info_free(&info);
#endif

    for (i = 0; i < NUM_WIN; i++) {
        MPI_Alloc_mem(DATA_SZ, MPI_INFO_NULL, &base_ptrs[i]);
        if (rank == 0)
            MTestPrintfMsg(1, " + Attaching segment %d to window\n", i);
        MPI_Win_attach(window, base_ptrs[i], DATA_SZ);
    }

    MPI_Barrier(MPI_COMM_WORLD);

    for (i = 0; i < NUM_WIN; i++) {
        if (rank == 0)
            MTestPrintfMsg(1, " + Detaching segment %d to window\n", i);
        MPI_Win_detach(window, base_ptrs[i]);
        MPI_Free_mem(base_ptrs[i]);
    }

    MPI_Win_free(&window);
    MTest_Finalize(0);

    return 0;
}
