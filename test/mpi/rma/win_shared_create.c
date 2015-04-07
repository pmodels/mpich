/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2014 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <mpi.h>
#include "mpitest.h"

#define ELEM_PER_PROC 4
int local_buf[ELEM_PER_PROC];

const int verbose = 0;

int main(int argc, char **argv)
{
    int i, rank, nproc;
    int shm_rank, shm_nproc;
    MPI_Aint size;
    int errors = 0, all_errors = 0;
    int **bases = NULL, *my_base = NULL;
    int disp_unit;
    MPI_Win shm_win = MPI_WIN_NULL, win = MPI_WIN_NULL;
    MPI_Comm shm_comm = MPI_COMM_NULL;
    MPI_Group shm_group = MPI_GROUP_NULL, world_group = MPI_GROUP_NULL;
    int dst_shm_rank, dst_world_rank;
    MPI_Info create_info = MPI_INFO_NULL;

    MPI_Init(&argc, &argv);

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &nproc);

    MPI_Comm_split_type(MPI_COMM_WORLD, MPI_COMM_TYPE_SHARED, rank, MPI_INFO_NULL, &shm_comm);

    MPI_Comm_rank(shm_comm, &shm_rank);
    MPI_Comm_size(shm_comm, &shm_nproc);

    /* Platform does not support shared memory, just return. */
    if (shm_nproc < 2) {
        goto exit;
    }

    /* Specify the last process in the node as the target process */
    dst_shm_rank = shm_nproc - 1;
    MPI_Comm_group(shm_comm, &shm_group);
    MPI_Comm_group(MPI_COMM_WORLD, &world_group);
    MPI_Group_translate_ranks(shm_group, 1, &dst_shm_rank, world_group, &dst_world_rank);

    bases = calloc(shm_nproc, sizeof(int *));

    /* Allocate shm window among local processes, then create a global window with
     * those shm window buffers */
    MPI_Win_allocate_shared(sizeof(int) * ELEM_PER_PROC, sizeof(int), MPI_INFO_NULL,
                            shm_comm, &my_base, &shm_win);
    if (verbose)
        printf("%d -- allocate shared: my_base = %p, absolute base\n", shm_rank, my_base);

    for (i = 0; i < shm_nproc; i++) {
        MPI_Win_shared_query(shm_win, i, &size, &disp_unit, &bases[i]);
        if (verbose)
            printf("%d --    shared query: base[%d]=%p, size %ld, unit %d\n",
                   shm_rank, i, bases[i], size, disp_unit);
    }

#ifdef USE_INFO_ALLOC_SHM
    MPI_Info_create(&create_info);
    MPI_Info_set(create_info, "alloc_shm", "true");
#else
    create_info = MPI_INFO_NULL;
#endif

    /* Reset data */
    for (i = 0; i < ELEM_PER_PROC; i++) {
        my_base[i] = 0;
        local_buf[i] = i + 1;
    }

    MPI_Win_create(my_base, sizeof(int) * ELEM_PER_PROC, sizeof(int), create_info, MPI_COMM_WORLD,
                   &win);

    /* Do RMA through global window, then check value through shared window */
    MPI_Win_lock_all(0, win);
    MPI_Win_lock_all(0, shm_win);

    if (shm_rank == 0) {
        MPI_Put(&local_buf[0], 1, MPI_INT, dst_world_rank, 0, 1, MPI_INT, win);
        MPI_Put(&local_buf[ELEM_PER_PROC - 1], 1, MPI_INT, dst_world_rank, ELEM_PER_PROC - 1, 1,
                MPI_INT, win);
        MPI_Win_flush(dst_world_rank, win);
    }

    MPI_Win_sync(shm_win);
    MPI_Barrier(shm_comm);
    MPI_Win_sync(shm_win);

    if (bases[dst_shm_rank][0] != local_buf[0]) {
        errors++;
        printf("%d -- Got %d at rank %d index %d, expected %d\n", rank,
               bases[dst_shm_rank][0], dst_shm_rank, 0, local_buf[0]);
    }
    if (bases[dst_shm_rank][ELEM_PER_PROC - 1] != local_buf[ELEM_PER_PROC - 1]) {
        errors++;
        printf("%d -- Got %d at rank %d index %d, expected %d\n", rank,
               bases[dst_shm_rank][ELEM_PER_PROC - 1], dst_shm_rank,
               ELEM_PER_PROC - 1, local_buf[ELEM_PER_PROC - 1]);
    }

    MPI_Win_unlock_all(shm_win);
    MPI_Win_unlock_all(win);

    MPI_Reduce(&errors, &all_errors, 1, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);

    MPI_Win_free(&win);
    MPI_Win_free(&shm_win);

  exit:
    if (rank == 0 && all_errors == 0)
        printf(" No Errors\n");

    if (create_info != MPI_INFO_NULL)
        MPI_Info_free(&create_info);
    if (shm_comm != MPI_COMM_NULL)
        MPI_Comm_free(&shm_comm);
    if (shm_group != MPI_GROUP_NULL)
        MPI_Group_free(&shm_group);
    if (world_group != MPI_GROUP_NULL)
        MPI_Group_free(&world_group);

    MPI_Finalize();

    if (bases)
        free(bases);

    return 0;
}
