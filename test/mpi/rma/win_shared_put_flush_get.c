/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2014 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include "mpitest.h"

#define ITER 100
#define BUF_CNT 4
double local_buf[BUF_CNT], check_buf[BUF_CNT];

const int verbose = 0;

int main(int argc, char *argv[])
{
    int rank, nproc, i, x;
    int errors = 0, all_errors = 0;
    MPI_Win win = MPI_WIN_NULL;

    MPI_Comm shm_comm = MPI_COMM_NULL;
    int shm_nproc, shm_rank;
    double **shm_bases = NULL, *my_base;
    MPI_Win shm_win = MPI_WIN_NULL;
    MPI_Group shm_group = MPI_GROUP_NULL, world_group = MPI_GROUP_NULL;
    int *shm_ranks = NULL, *shm_ranks_in_world = NULL;
    MPI_Aint get_target_base_offsets = 0;

    int win_size = sizeof(double) * BUF_CNT;
    int new_win_size = win_size;
    int win_unit = sizeof(double);
    int shm_root_rank_in_world;
    int origin = -1, put_target, get_target;

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &nproc);
    MPI_Comm_group(MPI_COMM_WORLD, &world_group);

    if (nproc != 4) {
        if (rank == 0)
            printf("Error: must be run with four processes\n");
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    MPI_Comm_split_type(MPI_COMM_WORLD, MPI_COMM_TYPE_SHARED, rank, MPI_INFO_NULL, &shm_comm);
    MPI_Comm_rank(shm_comm, &shm_rank);
    MPI_Comm_size(shm_comm, &shm_nproc);
    MPI_Comm_group(shm_comm, &shm_group);

    /* Platform does not support shared memory or wrong host file, just return. */
    if (shm_nproc != 2) {
        goto exit;
    }

    shm_bases = (double **) calloc(shm_nproc, sizeof(double *));
    shm_ranks = (int *) calloc(shm_nproc, sizeof(int));
    shm_ranks_in_world = (int *) calloc(shm_nproc, sizeof(int));

    if (shm_rank == 0)
        shm_root_rank_in_world = rank;
    MPI_Bcast(&shm_root_rank_in_world, 1, MPI_INT, 0, shm_comm);

    /* Identify ranks of target processes which are located on node 0 */
    if (rank == 0) {
        for (i = 0; i < shm_nproc; i++) {
            shm_ranks[i] = i;
        }
        MPI_Group_translate_ranks(shm_group, shm_nproc, shm_ranks, world_group, shm_ranks_in_world);
    }
    MPI_Bcast(shm_ranks_in_world, shm_nproc, MPI_INT, 0, MPI_COMM_WORLD);

    put_target = shm_ranks_in_world[shm_nproc - 1];
    get_target = shm_ranks_in_world[0];

    /* Identify the rank of origin process which are located on node 1 */
    if (shm_root_rank_in_world == 1 && shm_rank == 0) {
        origin = rank;
        if (verbose) {
            printf("----   I am origin = %d, get_target = %d, put_target = %d\n",
                   origin, get_target, put_target);
        }
    }

    /* Allocate shared memory among local processes */
    MPI_Win_allocate_shared(win_size, win_unit, MPI_INFO_NULL, shm_comm, &my_base, &shm_win);

    if (shm_root_rank_in_world == 0 && verbose) {
        MPI_Aint size;
        int disp_unit;
        for (i = 0; i < shm_nproc; i++) {
            MPI_Win_shared_query(shm_win, i, &size, &disp_unit, &shm_bases[i]);
            printf("%d --    shared query: base[%d]=%p, size %ld, "
                   "unit %d\n", rank, i, shm_bases[i], size, disp_unit);
        }
    }

    /* Get offset of put target(1) on get target(0) */
    get_target_base_offsets = (shm_nproc - 1) * win_size / win_unit;

    if (origin == rank && verbose)
        printf("%d --    base_offset of put_target %d on get_target %d: %ld\n",
               rank, put_target, get_target, get_target_base_offsets);

    /* Create using MPI_Win_create(). Note that new window size of get_target(0)
     * is equal to the total size of shm segments on this node, thus get_target
     * process can read the byte located on put_target process.*/
    for (i = 0; i < BUF_CNT; i++) {
        local_buf[i] = (i + 1) * 1.0;
        my_base[i] = 0.0;
    }

    if (get_target == rank)
        new_win_size = win_size * shm_nproc;

    MPI_Win_create(my_base, new_win_size, win_unit, MPI_INFO_NULL, MPI_COMM_WORLD, &win);

    if (verbose)
        printf("%d --    new window my_base %p, size %d\n", rank, my_base, new_win_size);

    MPI_Barrier(MPI_COMM_WORLD);

    /* Check if flush guarantees the completion of put operations on target side.
     *
     * P exclusively locks 2 processes whose windows are shared with each other.
     * P first put and flush to a process, then get the updated data from another process.
     * If flush returns before operations are done on the target side, the data may be
     * incorrect.*/
    for (x = 0; x < ITER; x++) {
        for (i = 0; i < BUF_CNT; i++) {
            local_buf[i] += x;
            check_buf[i] = 0;
        }

        if (rank == origin) {
            MPI_Win_lock(MPI_LOCK_EXCLUSIVE, put_target, 0, win);
            MPI_Win_lock(MPI_LOCK_EXCLUSIVE, get_target, 0, win);

            for (i = 0; i < BUF_CNT; i++) {
                MPI_Put(&local_buf[i], 1, MPI_DOUBLE, put_target, i, 1, MPI_DOUBLE, win);
            }
            MPI_Win_flush(put_target, win);

            MPI_Get(check_buf, BUF_CNT, MPI_DOUBLE, get_target,
                    get_target_base_offsets, BUF_CNT, MPI_DOUBLE, win);
            MPI_Win_flush(get_target, win);

            for (i = 0; i < BUF_CNT; i++) {
                if (check_buf[i] != local_buf[i]) {
                    printf("%d(iter %d) - Got check_buf[%d] = %.1lf, expected %.1lf\n",
                           rank, x, i, check_buf[i], local_buf[i]);
                    errors++;
                }
            }

            MPI_Win_unlock(put_target, win);
            MPI_Win_unlock(get_target, win);
        }
    }

    MPI_Barrier(MPI_COMM_WORLD);

    MPI_Reduce(&errors, &all_errors, 1, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);

  exit:

    if (rank == 0 && all_errors == 0)
        printf(" No Errors\n");

    if (shm_bases)
        free(shm_bases);
    if (shm_ranks)
        free(shm_ranks);
    if (shm_ranks_in_world)
        free(shm_ranks_in_world);

    if (shm_win != MPI_WIN_NULL)
        MPI_Win_free(&shm_win);

    if (win != MPI_WIN_NULL)
        MPI_Win_free(&win);

    if (shm_comm != MPI_COMM_NULL)
        MPI_Comm_free(&shm_comm);

    if (shm_group != MPI_GROUP_NULL)
        MPI_Group_free(&shm_group);

    if (world_group != MPI_GROUP_NULL)
        MPI_Group_free(&world_group);

    MPI_Finalize();

    return 0;
}
