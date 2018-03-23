/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "mpi.h"
#include "stdio.h"
#include "stdlib.h"
#include "mpitest.h"
#include "squelch.h"

/* Tests the example in Fig 6.8, pg 142, MPI-2 standard. Process 1 has
   a blocking MPI_Recv between the Post and Wait. Therefore, this
   example will not run if the one-sided operations are simply
   implemented on top of MPI_Isends and Irecvs. They either need to be
   implemented inside the progress engine or using threads with Isends
   and Irecvs. In MPICH-2, they are implemented in the progress engine. */

/* same as test3.c but uses alloc_mem */

#define SIZE 1048576

int main(int argc, char *argv[])
{
    int rank, destrank, nprocs, *A, *B, i;
    MPI_Comm CommDeuce;
    MPI_Group comm_group, group;
    MPI_Win win;
    int errs = 0;

    MTest_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &nprocs);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    if (nprocs < 2) {
        printf("Run this program with 2 or more processes\n");
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    MPI_Comm_split(MPI_COMM_WORLD, (rank < 2), rank, &CommDeuce);

    if (rank < 2) {
        i = MPI_Alloc_mem(SIZE * sizeof(int), MPI_INFO_NULL, &A);
        if (i) {
            printf("Can't allocate memory in test program\n");
            MPI_Abort(MPI_COMM_WORLD, 1);
        }

        MPI_Comm_group(CommDeuce, &comm_group);

        if (rank == 0) {
            i = MPI_Alloc_mem(SIZE * sizeof(int), MPI_INFO_NULL, &B);
            if (i) {
                printf("Can't allocate memory in test program\n");
                MPI_Abort(MPI_COMM_WORLD, 1);
            }

            for (i = 0; i < SIZE; i++) {
                A[i] = i;
                B[i] = SIZE + i;
            }
#ifdef USE_WIN_ALLOCATE
            char *base_ptr;
            MPI_Win_allocate(0, 1, MPI_INFO_NULL, CommDeuce, &base_ptr, &win);
#else
            MPI_Win_create(NULL, 0, 1, MPI_INFO_NULL, CommDeuce, &win);
#endif
            destrank = 1;
            MPI_Group_incl(comm_group, 1, &destrank, &group);
            MPI_Win_start(group, 0, win);
            MPI_Put(A, SIZE, MPI_INT, 1, 0, SIZE, MPI_INT, win);
            MPI_Win_complete(win);
            MPI_Send(B, SIZE, MPI_INT, 1, 100, MPI_COMM_WORLD);

            MPI_Free_mem(B);
        } else {        /* rank=1 */
#ifdef USE_WIN_ALLOCATE
            MPI_Win_allocate(SIZE * sizeof(int), sizeof(int), MPI_INFO_NULL, CommDeuce, &B, &win);
#else
            i = MPI_Alloc_mem(SIZE * sizeof(int), MPI_INFO_NULL, &B);
            if (i) {
                printf("Can't allocate memory in test program\n");
                MPI_Abort(MPI_COMM_WORLD, 1);
            }
            MPI_Win_create(B, SIZE * sizeof(int), sizeof(int), MPI_INFO_NULL, CommDeuce, &win);
#endif
            MPI_Win_lock(MPI_LOCK_SHARED, rank, 0, win);
            for (i = 0; i < SIZE; i++)
                A[i] = B[i] = (-4) * i;
            MPI_Win_unlock(rank, win);

            destrank = 0;
            MPI_Group_incl(comm_group, 1, &destrank, &group);
            MPI_Win_post(group, 0, win);
            MPI_Recv(A, SIZE, MPI_INT, 0, 100, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            MPI_Win_wait(win);

            for (i = 0; i < SIZE; i++) {
                if (B[i] != i) {
                    SQUELCH(printf("Rank 1: Put Error: B[i] is %d, should be %d\n", B[i], i););
                    errs++;
                }
                if (A[i] != SIZE + i) {
                    SQUELCH(printf
                            ("Rank 1: Send/Recv Error: A[i] is %d, should be %d\n", A[i],
                             SIZE + i););
                    errs++;
                }
            }

#ifndef USE_WIN_ALLOCATE
            MPI_Free_mem(B);
#endif
        }

        MPI_Group_free(&group);
        MPI_Group_free(&comm_group);
        MPI_Win_free(&win);
        MPI_Free_mem(A);
    }
    MPI_Comm_free(&CommDeuce);
    MTest_Finalize(errs);
    return MTestReturnValue(errs);
}
