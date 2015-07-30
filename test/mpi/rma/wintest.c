/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "mpi.h"
#include "stdio.h"
#include "mpitest.h"

/* tests put and get with post/start/complete/test on 2 processes */
/* Same as test2.c, but uses win_test instead of win_wait */

#define SIZE1 10
#define SIZE2 20

int main(int argc, char *argv[])
{
    int rank, destrank, nprocs, A[SIZE2], i;
    MPI_Comm CommDeuce;
    MPI_Group comm_group, group;
    MPI_Win win;
    int errs = 0, flag;

    MTest_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &nprocs);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    if (nprocs < 2) {
        printf("Run this program with 2 or more processes\n");
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    MPI_Comm_split(MPI_COMM_WORLD, (rank < 2), rank, &CommDeuce);

    if (rank < 2) {
        MPI_Comm_group(CommDeuce, &comm_group);

        if (rank == 0) {
            int B[SIZE2];
            for (i = 0; i < SIZE2; i++)
                A[i] = B[i] = i;
#ifdef USE_WIN_ALLOCATE
            int *base_ptr = NULL;
            MPI_Win_allocate(0, 1, MPI_INFO_NULL, CommDeuce, &base_ptr, &win);
#else
            MPI_Win_create(NULL, 0, 1, MPI_INFO_NULL, CommDeuce, &win);
#endif
            destrank = 1;
            MPI_Group_incl(comm_group, 1, &destrank, &group);
            MPI_Win_start(group, 0, win);
            for (i = 0; i < SIZE1; i++)
                MPI_Put(A + i, 1, MPI_INT, 1, i, 1, MPI_INT, win);
            for (i = 0; i < SIZE1; i++)
                MPI_Get(B + i, 1, MPI_INT, 1, SIZE1 + i, 1, MPI_INT, win);

            MPI_Win_complete(win);

            for (i = 0; i < SIZE1; i++)
                if (B[i] != (-4) * (i + SIZE1)) {
                    printf("Get Error: B[i] is %d, should be %d\n", B[i], (-4) * (i + SIZE1));
                    errs++;
                }
        }
        else {  /* rank=1 */
#ifdef USE_WIN_ALLOCATE
            int *B;
            MPI_Win_allocate(SIZE2 * sizeof(int), sizeof(int), MPI_INFO_NULL, CommDeuce, &B, &win);
#else
            int B[SIZE2];
            MPI_Win_create(B, SIZE2 * sizeof(int), sizeof(int), MPI_INFO_NULL, CommDeuce, &win);
#endif
            MPI_Win_lock(MPI_LOCK_SHARED, rank, 0, win);
            for (i = 0; i < SIZE2; i++)
                B[i] = (-4) * i;
            MPI_Win_unlock(rank, win);

            destrank = 0;
            MPI_Group_incl(comm_group, 1, &destrank, &group);
            MPI_Win_post(group, 0, win);
            flag = 0;
            while (!flag)
                MPI_Win_test(win, &flag);

            for (i = 0; i < SIZE1; i++) {
                if (B[i] != i) {
                    printf("Put Error: B[i] is %d, should be %d\n", B[i], i);
                    errs++;
                }
            }
        }

        MPI_Group_free(&group);
        MPI_Group_free(&comm_group);
        MPI_Win_free(&win);
    }
    MPI_Comm_free(&CommDeuce);
    MTest_Finalize(errs);
    MPI_Finalize();
    return 0;
}
