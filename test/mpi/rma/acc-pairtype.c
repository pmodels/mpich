/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2015 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

/* This test is going to test when Accumulate operation is working
 * with pair types. */

#include "mpi.h"
#include <stdio.h>

#define DATA_SIZE 25

typedef struct long_double_int {
    long double a;
    int b;
} long_double_int_t;

int main(int argc, char *argv[])
{
    MPI_Win win;
    int errors = 0;
    int rank, nproc, i;
    long_double_int_t *orig_buf;
    long_double_int_t *tar_buf;
    MPI_Datatype vector_dtp;

    MPI_Init(&argc, &argv);

    MPI_Comm_size(MPI_COMM_WORLD, &nproc);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    MPI_Alloc_mem(sizeof(long_double_int_t) * DATA_SIZE, MPI_INFO_NULL, &orig_buf);
    MPI_Alloc_mem(sizeof(long_double_int_t) * DATA_SIZE, MPI_INFO_NULL, &tar_buf);

    for (i = 0; i < DATA_SIZE; i++) {
        orig_buf[i].a = 1.0;
        orig_buf[i].b = 1;
        tar_buf[i].a = 0;
        tar_buf[i].b = 0;
    }

    MPI_Type_vector(5 /* count */ , 3 /* blocklength */ , 5 /* stride */ , MPI_LONG_DOUBLE_INT,
                    &vector_dtp);
    MPI_Type_commit(&vector_dtp);

    MPI_Win_create(tar_buf, sizeof(long_double_int_t) * DATA_SIZE, sizeof(long_double_int_t),
                   MPI_INFO_NULL, MPI_COMM_WORLD, &win);

    if (rank == 0) {
        MPI_Win_lock(MPI_LOCK_SHARED, 1, 0, win);
        MPI_Accumulate(orig_buf, 1, vector_dtp, 1, 0, 1, vector_dtp, MPI_MAXLOC, win);
        MPI_Win_unlock(1, win);
    }

    MPI_Win_free(&win);

    if (rank == 1) {
        for (i = 0; i < DATA_SIZE; i++) {
            if (i % 5 < 3) {
                if (tar_buf[i].a != 1.0 || tar_buf[i].b != 1) {
                    errors++;
                }
            }
            else {
                if (tar_buf[i].a != 0.0 || tar_buf[i].b != 0) {
                    errors++;
                }
            }
        }
    }

    MPI_Type_free(&vector_dtp);

    MPI_Free_mem(orig_buf);
    MPI_Free_mem(tar_buf);

    if (rank == 1) {
        if (errors == 0)
            printf(" No Errors\n");
    }

    MPI_Finalize();
    return 0;
}
