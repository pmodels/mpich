/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2018 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include "mpitest.h"

/*
 * Check return values of MPI_Win_allocate_shared with MPI_PROC_NULL.
 * It should return the first non-zero window segments on the node.
 * In this test, rank 0 allocates zero window and others allocate non-zero.
 * Thus, it should return the segment belonging to rank 1.
 */

int main(int argc, char **argv)
{
    int rank, errors = 0;
    int shm_rank, shm_nproc;
    MPI_Aint size, query_size;
    int *query_base, *my_base;
    int query_disp_unit;
    MPI_Win shm_win;
    MPI_Comm shm_comm;

    MTest_Init(&argc, &argv);

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_split_type(MPI_COMM_WORLD, MPI_COMM_TYPE_SHARED, rank, MPI_INFO_NULL, &shm_comm);

    MPI_Comm_rank(shm_comm, &shm_rank);
    MPI_Comm_size(shm_comm, &shm_nproc);

    /* Only single process is on the node or the platform does not support shared memory.
     * Just wait for others' completion*/
    if (shm_nproc < 2)
        goto exit;

    size = sizeof(int) * 4;

    /* Allocate zero-byte window on rank 0 and non-zero for others */
    if (shm_rank == 0) {
        MPI_Win_allocate_shared(0, sizeof(int), MPI_INFO_NULL, shm_comm, &my_base, &shm_win);
    } else {
        MPI_Win_allocate_shared(size, sizeof(int), MPI_INFO_NULL, shm_comm, &my_base, &shm_win);
    }

    /* Query the segment belonging the lowest rank with size > 0 */
    MPI_Win_shared_query(shm_win, MPI_PROC_NULL, &query_size, &query_disp_unit, &query_base);
    MTestPrintfMsg(1, "%d --   shared query with PROC_NULL: base %p, size %ld, unit %d\n",
                   shm_rank, query_base, query_size, query_disp_unit);

    if (query_base == NULL) {
        fprintf(stderr, "%d --   shared query with PROC_NULL: base %p, expected non-NULL pointer\n",
                shm_rank, query_base);
        fflush(stderr);
        errors++;
    }

    if (query_size != size) {
        fprintf(stderr, "%d --   shared query with PROC_NULL: size %ld, expected %ld\n",
                shm_rank, query_size, size);
        fflush(stderr);
        errors++;
    }

    if (query_disp_unit != sizeof(int)) {
        fprintf(stderr, "%d --   shared query with PROC_NULL: disp_unit %d, expected %ld\n",
                shm_rank, query_disp_unit, sizeof(int));
        fflush(stderr);
        errors++;
    }

    MPI_Win_free(&shm_win);

  exit:

    MPI_Comm_free(&shm_comm);
    MTest_Finalize(errors);

    return MTestReturnValue(errors);
}
