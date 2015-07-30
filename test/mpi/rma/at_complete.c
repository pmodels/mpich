/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2014 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include <stdio.h>
#include <mpi.h>

#define PUT_SIZE 1
#define GET_SIZE 100000
#define WIN_SIZE (PUT_SIZE+GET_SIZE)
#define LOOP 100

int main(int argc, char **argv)
{
    MPI_Win win;
    int i, k, rank, nproc;
    int win_buf[WIN_SIZE], orig_get_buf[GET_SIZE], orig_put_buf[PUT_SIZE];
    int orig_rank = 0, dest_rank = 1;
    int errors = 0;
    MPI_Group comm_group, orig_group, dest_group;

    MPI_Init(&argc, &argv);

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &nproc);

    MPI_Comm_group(MPI_COMM_WORLD, &comm_group);
    MPI_Group_incl(comm_group, 1, &orig_rank, &orig_group);
    MPI_Group_incl(comm_group, 1, &dest_rank, &dest_group);

    for (i = 0; i < PUT_SIZE; i++)
        orig_put_buf[i] = 1;
    for (i = 0; i < GET_SIZE; i++)
        orig_get_buf[i] = 0;
    for (i = 0; i < WIN_SIZE; i++)
        win_buf[i] = 1;

    MPI_Win_create(win_buf, sizeof(int) * WIN_SIZE, sizeof(int), MPI_INFO_NULL,
                   MPI_COMM_WORLD, &win);

    for (k = 0; k < LOOP; k++) {

        /* test for FENCE */

        if (rank == orig_rank) {
            MPI_Win_fence(MPI_MODE_NOPRECEDE, win);
            MPI_Get(orig_get_buf, GET_SIZE, MPI_INT,
                    dest_rank, PUT_SIZE /*disp */ , GET_SIZE, MPI_INT, win);
            MPI_Put(orig_put_buf, PUT_SIZE, MPI_INT,
                    dest_rank, 0 /*disp */ , PUT_SIZE, MPI_INT, win);
            MPI_Win_fence(MPI_MODE_NOSUCCEED, win);

            /* check GET result values */
            for (i = 0; i < GET_SIZE; i++) {
                if (orig_get_buf[i] != 1) {
                    printf("LOOP=%d, FENCE, orig_get_buf[%d] = %d, expected 1.\n",
                           k, i, orig_get_buf[i]);
                    errors++;
                }
            }
        }
        else if (rank == dest_rank) {
            MPI_Win_fence(MPI_MODE_NOPRECEDE, win);
            MPI_Win_fence(MPI_MODE_NOSUCCEED, win);

            /* modify the last element in window */
            MPI_Win_lock(MPI_LOCK_SHARED, rank, 0, win);
            win_buf[WIN_SIZE - 1] = 2;
            MPI_Win_unlock(rank, win);
        }

        MPI_Barrier(MPI_COMM_WORLD);
        /* reset buffers */
        for (i = 0; i < PUT_SIZE; i++)
            orig_put_buf[i] = 1;
        for (i = 0; i < GET_SIZE; i++)
            orig_get_buf[i] = 0;
        MPI_Win_lock(MPI_LOCK_SHARED, rank, 0, win);
        for (i = 0; i < WIN_SIZE; i++)
            win_buf[i] = 1;
        MPI_Win_unlock(rank, win);
        MPI_Barrier(MPI_COMM_WORLD);

        /* test for PSCW */

        if (rank == orig_rank) {
            MPI_Win_start(dest_group, 0, win);
            MPI_Get(orig_get_buf, GET_SIZE, MPI_INT,
                    dest_rank, PUT_SIZE /*disp */ , GET_SIZE, MPI_INT, win);
            MPI_Put(orig_put_buf, PUT_SIZE, MPI_INT,
                    dest_rank, 0 /*disp */ , PUT_SIZE, MPI_INT, win);
            MPI_Win_complete(win);

            /* check GET result values */
            for (i = 0; i < GET_SIZE; i++) {
                if (orig_get_buf[i] != 1) {
                    printf("LOOP=%d, PSCW, orig_get_buf[%d] = %d, expected 1.\n",
                           k, i, orig_get_buf[i]);
                    errors++;
                }
            }
        }
        else if (rank == dest_rank) {
            MPI_Win_post(orig_group, 0, win);
            MPI_Win_wait(win);

            /* modify the last element in window */
            MPI_Win_lock(MPI_LOCK_SHARED, rank, 0, win);
            win_buf[WIN_SIZE - 1] = 2;
            MPI_Win_unlock(rank, win);
        }

        MPI_Barrier(MPI_COMM_WORLD);
        /* reset buffers */
        for (i = 0; i < PUT_SIZE; i++)
            orig_put_buf[i] = 1;
        for (i = 0; i < GET_SIZE; i++)
            orig_get_buf[i] = 0;
        MPI_Win_lock(MPI_LOCK_SHARED, rank, 0, win);
        for (i = 0; i < WIN_SIZE; i++)
            win_buf[i] = 1;
        MPI_Win_unlock(rank, win);
        MPI_Barrier(MPI_COMM_WORLD);
    }

    MPI_Win_free(&win);

    MPI_Group_free(&orig_group);
    MPI_Group_free(&dest_group);
    MPI_Group_free(&comm_group);

    if (rank == orig_rank && errors == 0) {
        printf(" No Errors\n");
    }

    MPI_Finalize();
    return 0;
}
