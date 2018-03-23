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
    MPI_Aint size;
    int errors = 0;
    int *base, *my_base;
    int disp_unit;
    MPI_Win shm_win;
    MPI_Comm shm_comm;

    MTest_Init(&argc, &argv);

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &nproc);

    MPI_Comm_split_type(MPI_COMM_WORLD, MPI_COMM_TYPE_SHARED, rank, MPI_INFO_NULL, &shm_comm);

    MPI_Comm_rank(shm_comm, &shm_rank);
    MPI_Comm_size(shm_comm, &shm_nproc);

    /* Allocate ELEM_PER_PROC integers for each process */
    MPI_Win_allocate_shared(sizeof(int) * ELEM_PER_PROC, sizeof(int), MPI_INFO_NULL,
                            shm_comm, &my_base, &shm_win);

    /* Locate absolute base */
    MPI_Win_shared_query(shm_win, MPI_PROC_NULL, &size, &disp_unit, &base);

    /* make sure the query returned the right values */
    if (disp_unit != sizeof(int))
        errors++;
    if (size != ELEM_PER_PROC * sizeof(int))
        errors++;
    if ((shm_rank == 0) && (base != my_base))
        errors++;
    if (shm_rank && (base == my_base))
        errors++;

    if (verbose)
        printf("%d -- size = %d baseptr = %p my_baseptr = %p\n", shm_rank,
               (int) size, (void *) base, (void *) my_base);

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
        for (j = 0; j < ELEM_PER_PROC; j++) {
            if (base[i * ELEM_PER_PROC + j] != j) {
                errors++;
                printf("%d -- Got %d at rank %d index %d, expected %d\n", shm_rank,
                       base[i * ELEM_PER_PROC + j], i, j, j);
            }
        }
    }

    MPI_Win_unlock_all(shm_win);
    MPI_Win_free(&shm_win);
    MPI_Comm_free(&shm_comm);

    MTest_Finalize(errors);

    return MTestReturnValue(errors);
}
