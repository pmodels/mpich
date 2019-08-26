/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2018 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 */

#include "mpidimpl.h"
#include "mpidu_shm.h"
#include "mpidu_bc.h"

static MPIDU_shm_seg_t memory;
static MPIDU_shm_barrier_t *barrier;
static size_t *indices;
static char *segment;

int MPIDU_bc_table_destroy(void *bc_table)
{
    int mpi_errno = MPI_SUCCESS;

    int local_size = MPIR_Process.local_size;

    mpi_errno = MPIDU_shm_barrier(barrier, local_size);
    MPIR_ERR_CHECK(mpi_errno);
    mpi_errno = MPIDU_shm_seg_destroy(&memory, local_size);
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    MPL_free(indices);

    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

/* allgather (local_size - 1) bc over node roots */
int MPIDU_bc_allgather(MPIR_Comm * comm, int *nodemap, void *bc, int bc_len, int same_len,
                       void **bc_table, size_t ** bc_indices)
{
    int mpi_errno = MPI_SUCCESS;

    int rank = MPIR_Process.rank;
    int size = MPIR_Process.size;
    int local_rank = MPIR_Process.local_rank;
    int local_size = MPIR_Process.local_size;

    int node_id = MPIR_Process.node_map[rank];
    int local_leader = MPIR_Process.node_root_map[node_id];

    mpi_errno = MPIDU_shm_barrier(barrier, local_size);
    MPIR_ERR_CHECK(mpi_errno);

    if (!same_len) {
        bc_len *= 2;
        *bc_indices = indices;
    }

    if (local_rank > 0) {
        int start = local_leader - node_id + (local_rank - 1);

        memcpy(&segment[start * bc_len], bc, bc_len);
    }

    mpi_errno = MPIDU_shm_barrier(barrier, local_size);
    MPIR_ERR_CHECK(mpi_errno);

    if (rank == local_leader) {
        MPIR_Errflag_t errflag = MPIR_ERR_NONE;
        /* TODO: assume comm is node roots comm */
        MPIR_Comm *allgather_comm = comm->node_roots_comm ? comm->node_roots_comm : comm;

        MPIR_Allgather_impl(MPI_IN_PLACE, 0, MPI_DATATYPE_NULL, segment, (local_size - 1) * bc_len,
                            MPI_BYTE, allgather_comm, &errflag);
    }

    mpi_errno = MPIDU_shm_barrier(barrier, local_size);
    MPIR_ERR_CHECK(mpi_errno);

    *bc_table = segment;

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIDU_bc_table_create(int rank, int size, int *nodemap, void *bc, int bc_len, int same_len,
                          int roots_only, void **bc_table, size_t ** bc_indices)
{
    int mpi_errno = MPI_SUCCESS;

    int rank = MPIR_Process.rank;
    int size = MPIR_Process.size;
    int local_rank = MPIR_Process.local_rank;
    int local_size = MPIR_Process.local_size;

    int node_id = MPIR_Process.node_map[rank];
    int local_leader = MPIR_Process.node_root_map[node_id];

    int max_val_size = MPIR_pmi_max_val_size();

    int my_bc_len = bc_len;
    /* if business cards can be different length, use the max value length */
    if (!same_len)
        bc_len = max_val_size;

    mpi_errno = MPIDU_shm_seg_alloc(bc_len * size, (void **) &segment, MPL_MEM_ADDRESS);
    MPIR_ERR_CHECK(mpi_errno);
    mpi_errno = MPIDU_shm_seg_commit(&memory, &barrier, local_size, local_rank, local_leader, rank,
                                     MPL_MEM_ADDRESS);
    MPIR_ERR_CHECK(mpi_errno);

    if (size == 1) {
        memcpy(segment, bc, my_bc_len);
    } else {
        int domain = roots_only ? PMI_DOMAIN_NODE_ROOTS : PMI_DOMAIN_ALL;
        mpi_errno = MPIR_pmi_allgather_shm(bc, my_bc_len, segment, bc_len, domain);
        MPIR_ERR_CHECK(mpi_errno);

        mpi_errno = MPIDU_shm_barrier(barrier, local_size);
        MPIR_ERR_CHECK(mpi_errno);
    }

    if (!same_len) {
        indices = MPL_malloc(size * sizeof(size_t), MPL_MEM_ADDRESS);
        for (i = 0; i < size; i++)
            indices[i] = bc_len * i;
        *bc_indices = indices;
    }

  fn_exit:
    *bc_table = segment;

    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
