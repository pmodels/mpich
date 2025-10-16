/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

/* This test is going to test when Accumulate operation is working
 * with pair types. */

#include "mpi.h"
#include <stdio.h>
#include "mpitest.h"

#define COUNT 10
#define DATA_SIZE COUNT * 5

typedef struct long_double_int {
    long double a;
    int b;
} long_double_int_t;

int main(int argc, char *argv[])
{
    MPI_Win win;
    int errs = 0;
    int rank, nproc, i;
    long_double_int_t *orig_buf;
    long_double_int_t *tar_buf;
    MPI_Datatype vector_dtp;

    MTest_Init(&argc, &argv);

    MPI_Comm_size(MPI_COMM_WORLD, &nproc);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    MPI_Alloc_mem(sizeof(long_double_int_t) * DATA_SIZE, MPI_INFO_NULL, &orig_buf);
    for (i = 0; i < DATA_SIZE; i++) {
        orig_buf[i].a = 1.0;
        orig_buf[i].b = 1;
    }

    MPI_Type_vector(COUNT /* count */ , 3 /* blocklength */ , 5 /* stride */ , MPI_LONG_DOUBLE_INT,
                    &vector_dtp);
    MPI_Type_commit(&vector_dtp);

#ifdef USE_WIN_ALLOCATE
    MPI_Win_allocate(sizeof(long_double_int_t) * DATA_SIZE, sizeof(long_double_int_t),
                     MPI_INFO_NULL, MPI_COMM_WORLD, &tar_buf, &win);
    /* Reset window buffer */
    MPI_Win_lock(MPI_LOCK_SHARED, rank, 0, win);
    for (i = 0; i < DATA_SIZE; i++) {
        tar_buf[i].a = 0;
        tar_buf[i].b = 0;
    }
    MPI_Win_unlock(rank, win);
    MPI_Barrier(MPI_COMM_WORLD);
#else
    MPI_Alloc_mem(sizeof(long_double_int_t) * DATA_SIZE, MPI_INFO_NULL, &tar_buf);
    /* Reset window buffer */
    for (i = 0; i < DATA_SIZE; i++) {
        tar_buf[i].a = 0;
        tar_buf[i].b = 0;
    }
    MPI_Win_create(tar_buf, sizeof(long_double_int_t) * DATA_SIZE, sizeof(long_double_int_t),
                   MPI_INFO_NULL, MPI_COMM_WORLD, &win);
#endif

    if (rank == 0) {
        MPI_Win_lock(MPI_LOCK_SHARED, 1, 0, win);
        MPI_Accumulate(orig_buf, 1, vector_dtp, 1, 0, 1, vector_dtp, MPI_MAXLOC, win);
        MPI_Win_unlock(1, win);
    }

    MPI_Barrier(MPI_COMM_WORLD);

    if (rank == 1) {
        MPI_Win_lock(MPI_LOCK_SHARED, 1, 0, win);
        for (i = 0; i < DATA_SIZE; i++) {
            if (i % 5 < 3) {
                if (tar_buf[i].a != 1.0 || tar_buf[i].b != 1) {
                    errs++;
                }
            } else {
                if (tar_buf[i].a != 0.0 || tar_buf[i].b != 0) {
                    errs++;
                }
            }
        }
        MPI_Win_unlock(1, win);
    }

    MPI_Win_free(&win);
    MPI_Type_free(&vector_dtp);

    MPI_Free_mem(orig_buf);
#ifndef USE_WIN_ALLOCATE
    MPI_Free_mem(tar_buf);
#endif

    MTest_Finalize(errs);
    return MTestReturnValue(errs);
}
