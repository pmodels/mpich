/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpitest.h"
#include "squelch.h"

#ifdef MULTI_TESTS
#define run rma_test2
int run(const char *arg);
#endif

/* tests put and get with post/start/complete/wait on 2 processes */

#define SIZE1 100
#define SIZE2 200

static int use_win_allocate = 0;

int run(const char *arg)
{
    int rank, destrank, nprocs, A[SIZE2], i;
    int B[SIZE2];
    MPI_Comm CommDeuce;
    MPI_Group comm_group, group;
    MPI_Win win;
    int errs = 0;

    MTestArgList *head = MTestArgListCreate_arg(arg);
    use_win_allocate = MTestArgListGetInt_with_default(head, "use-win-allocate", 0);
    MTestArgListDestroy(head);

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
            for (i = 0; i < SIZE2; i++)
                A[i] = B[i] = i;
            if (use_win_allocate) {
                char *base_ptr;
                MPI_Win_allocate(0, 1, MPI_INFO_NULL, CommDeuce, &base_ptr, &win);
            } else {
                MPI_Win_create(NULL, 0, 1, MPI_INFO_NULL, CommDeuce, &win);
            }
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
                    SQUELCH(printf
                            ("Get Error: B[i] is %d, should be %d\n", B[i], (-4) * (i + SIZE1)););
                    errs++;
                }
        } else if (rank == 1) {
            int *use_B = NULL;
            if (use_win_allocate) {
                MPI_Win_allocate(SIZE2 * sizeof(int), sizeof(int), MPI_INFO_NULL, CommDeuce, &use_B,
                                 &win);
            } else {
                use_B = B;
                MPI_Win_create(B, SIZE2 * sizeof(int), sizeof(int), MPI_INFO_NULL, CommDeuce, &win);
            }
            MPI_Win_lock(MPI_LOCK_SHARED, rank, 0, win);
            for (i = 0; i < SIZE2; i++) {
                use_B[i] = (-4) * i;
            }
            MPI_Win_unlock(rank, win);

            destrank = 0;
            MPI_Group_incl(comm_group, 1, &destrank, &group);
            MPI_Win_post(group, 0, win);
            MPI_Win_wait(win);

            for (i = 0; i < SIZE1; i++) {
                if (use_B[i] != i) {
                    SQUELCH(printf("Put Error: B[i] is %d, should be %d\n", use_B[i], i););
                    errs++;
                }
            }
        }

        MPI_Group_free(&group);
        MPI_Group_free(&comm_group);
        MPI_Win_free(&win);
    }
    MPI_Comm_free(&CommDeuce);

    return errs;
}
