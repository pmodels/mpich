/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpitest.h"

#ifdef MULTI_TESTS
#define run rma_fence_shm
int run(const char *arg);
#endif

#define ELEM_PER_PROC 1

static int errs = 0;

int run(const char *arg)
{
    int rank, nprocs;
    int shm_rank, shm_nprocs;
    MPI_Comm shm_comm;
    MPI_Win shm_win;
    int *my_base;
    int one = 1;
    int result_data;

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &nprocs);

    /* run with two processes. */

    MPI_Comm_split_type(MPI_COMM_WORLD, MPI_COMM_TYPE_SHARED, rank, MPI_INFO_NULL, &shm_comm);

    MPI_Comm_rank(shm_comm, &shm_rank);
    MPI_Comm_size(shm_comm, &shm_nprocs);

    if (shm_nprocs >= 2) {
        MPI_Win_allocate_shared(sizeof(int) * ELEM_PER_PROC, sizeof(int), MPI_INFO_NULL,
                                shm_comm, &my_base, &shm_win);

        /* Test for FENCE with no asserts. */

        if (shm_rank == 1) {
            *my_base = 0;

            MPI_Win_fence(0, shm_win);
            MPI_Win_fence(0, shm_win);

            if (my_base[0] != one) {
                errs++;
                printf("Expected: my_base[0] = %d   Actual: my_base[0] = %d\n", one, my_base[0]);
            }
        }

        if (shm_rank == 0) {
            MPI_Win_fence(0, shm_win);
            MPI_Put(&one, 1, MPI_INT, 1, 0, 1, MPI_INT, shm_win);
            MPI_Win_fence(0, shm_win);
        }

        /* Other ranks simply join fence */
        if (shm_rank > 1) {
            MPI_Win_fence(0, shm_win);
            MPI_Win_fence(0, shm_win);
        }

        /* Test for FENCE with assert MPI_MODE_NOPRECEDE. */

        if (shm_rank == 1) {
            MPI_Win_lock(MPI_LOCK_EXCLUSIVE, 1, 0, shm_win);
            MPI_Put(&one, 1, MPI_INT, 1, 0, 1, MPI_INT, shm_win);
            MPI_Win_unlock(1, shm_win);

            MPI_Win_fence(MPI_MODE_NOPRECEDE, shm_win);
            MPI_Win_fence(0, shm_win);
        }

        if (shm_rank == 0) {
            result_data = 0;
            MPI_Win_fence(MPI_MODE_NOPRECEDE, shm_win);
            MPI_Get(&result_data, 1, MPI_INT, 1, 0, 1, MPI_INT, shm_win);
            MPI_Win_fence(0, shm_win);

            if (result_data != one) {
                errs++;
                printf("Expected: result_data = %d   Actual: result_data = %d\n", one, result_data);
            }
        }

        /* Other ranks simply join fence */
        if (shm_rank > 1) {
            MPI_Win_fence(MPI_MODE_NOPRECEDE, shm_win);
            MPI_Win_fence(0, shm_win);
        }

        MPI_Win_free(&shm_win);
    }

    MPI_Comm_free(&shm_comm);

    return errs;
}
