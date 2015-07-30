/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2012 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <mpi.h>
#include "mpitest.h"

#define ELEM_PER_PROC 10000

const int verbose = 0;

int main(int argc, char **argv)
{
    int i, j, rank, nproc;
    int shm_rank, shm_nproc;
    MPI_Info alloc_shared_info;
    int errors = 0, all_errors = 0;
    int disp_unit;
    int *my_base;
    MPI_Win shm_win;
    MPI_Comm shm_comm;

    MPI_Init(&argc, &argv);

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &nproc);

    MPI_Info_create(&alloc_shared_info);
    MPI_Info_set(alloc_shared_info, "alloc_shared_noncontig", "true");

    MPI_Comm_split_type(MPI_COMM_WORLD, MPI_COMM_TYPE_SHARED, rank, MPI_INFO_NULL, &shm_comm);

    MPI_Comm_rank(shm_comm, &shm_rank);
    MPI_Comm_size(shm_comm, &shm_nproc);

    /* Allocate ELEM_PER_PROC integers for each process */
    MPI_Win_allocate_shared(sizeof(int) * ELEM_PER_PROC, sizeof(int), alloc_shared_info,
                            shm_comm, &my_base, &shm_win);

    MPI_Win_lock_all(MPI_MODE_NOCHECK, shm_win);

    /* Write to all my data */
    for (i = 0; i < ELEM_PER_PROC; i++) {
        my_base[i] = i;
    }

    MPI_Win_sync(shm_win);
    MPI_Barrier(shm_comm);
    MPI_Win_sync(shm_win);

    /* Read and verify everyone's data */
    for (i = 0; i < shm_nproc; i++) {
        int *base;
        MPI_Aint size;

        MPI_Win_shared_query(shm_win, i, &size, &disp_unit, &base);
        assert(size >= ELEM_PER_PROC * sizeof(int));

        for (j = 0; j < ELEM_PER_PROC; j++) {
            if (base[j] != j) {
                errors++;
                printf("%d -- Got %d at rank %d index %d, expected %d\n", shm_rank,
                       base[j], i, j, j);
            }
        }
    }

    MPI_Win_unlock_all(shm_win);
    MPI_Win_free(&shm_win);
    MPI_Comm_free(&shm_comm);

    MPI_Info_free(&alloc_shared_info);

    MPI_Reduce(&errors, &all_errors, 1, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);

    if (rank == 0 && all_errors == 0)
        printf(" No Errors\n");

    MPI_Finalize();

    return 0;
}
