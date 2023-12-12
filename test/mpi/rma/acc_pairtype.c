/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

/* This test is going to test when Accumulate operation is working
 * with pair types. */

#include "mpitest.h"

#ifdef MULTI_TESTS
#define run rma_acc_pairtype
int run(const char *arg);
#endif

#define DATA_SIZE 25

typedef struct long_double_int {
    long double a;
    int b;
} long_double_int_t;

static int use_win_allocate = 0;

int run(const char *arg)
{
    MPI_Win win;
    int errs = 0;
    int rank, nproc, i;
    long_double_int_t *orig_buf;
    long_double_int_t *tar_buf;
    MPI_Datatype vector_dtp;

    MTestArgList *head = MTestArgListCreate_arg(arg);
    use_win_allocate = MTestArgListGetInt_with_default(head, "use-win-allocate", 0);
    MTestArgListDestroy(head);

    MPI_Comm_size(MPI_COMM_WORLD, &nproc);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    MPI_Alloc_mem(sizeof(long_double_int_t) * DATA_SIZE, MPI_INFO_NULL, &orig_buf);
    for (i = 0; i < DATA_SIZE; i++) {
        orig_buf[i].a = 1.0;
        orig_buf[i].b = 1;
    }

    MPI_Type_vector(5 /* count */ , 3 /* blocklength */ , 5 /* stride */ , MPI_LONG_DOUBLE_INT,
                    &vector_dtp);
    MPI_Type_commit(&vector_dtp);

    if (use_win_allocate) {
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
    } else {
        MPI_Alloc_mem(sizeof(long_double_int_t) * DATA_SIZE, MPI_INFO_NULL, &tar_buf);
        /* Reset window buffer */
        for (i = 0; i < DATA_SIZE; i++) {
            tar_buf[i].a = 0;
            tar_buf[i].b = 0;
        }
        MPI_Win_create(tar_buf, sizeof(long_double_int_t) * DATA_SIZE, sizeof(long_double_int_t),
                       MPI_INFO_NULL, MPI_COMM_WORLD, &win);
    }

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
    if (!use_win_allocate) {
        MPI_Free_mem(tar_buf);
    }

    return errs;
}
