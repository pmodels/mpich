/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpidimpl.h"
#include "mpidu_bc.h"
#include "mpidu_init_shm.h"

static char *segment;
static int *rank_map;

int MPIDU_bc_table_destroy(void)
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = MPIDU_Init_shm_barrier();
    MPIR_ERR_CHECK(mpi_errno);
    mpi_errno = MPIDU_Init_shm_free((void *) segment);
    MPIR_ERR_CHECK(mpi_errno);

    if (rank_map) {
        MPL_free(rank_map);
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

/* allgather (local_size - 1) bc over node roots */
int MPIDU_bc_allgather(MPIR_Comm * allgather_comm, void *bc, int bc_len, int same_len,
                       void **bc_table, int **bc_ranks, int *ret_bc_len)
{
    int mpi_errno = MPI_SUCCESS;

    int rank = MPIR_Process.rank;
    int size = MPIR_Process.size;
    int local_rank = MPIR_Process.local_rank;
    int local_size = MPIR_Process.local_size;

    int num_nodes = MPIR_Process.num_nodes;
    int node_root = MPIR_Process.node_root_map[MPIR_Process.node_map[rank]];

    int i;
    /* construct a consecutive node rank map */
    rank_map = MPL_malloc(sizeof(int) * size, MPL_MEM_OTHER);

    if (size == num_nodes) {
        for (i = 0; i < size; i++) {
            rank_map[i] = -1;
        }
        *bc_table = NULL;
        *bc_ranks = rank_map;
        *ret_bc_len = bc_len;
        return mpi_errno;
    }

    MPI_Aint *recv_cnts = MPL_calloc(num_nodes, sizeof(MPI_Aint), MPL_MEM_OTHER);
    MPI_Aint *recv_offs = MPL_calloc(num_nodes, sizeof(MPI_Aint), MPL_MEM_OTHER);
    for (i = 0; i < size; i++) {
        int node_id = MPIR_Process.node_map[i];
        recv_cnts[node_id]++;
    }
    int disp = 0;
    for (i = 0; i < num_nodes; i++) {
        recv_offs[i] = disp;
        disp += recv_cnts[i];
    }
    /* rank -> consecutive rank */
    for (i = 0; i < size; i++) {
        int node_id = MPIR_Process.node_map[i];
        rank_map[i] = recv_offs[node_id];
        recv_offs[node_id]++;
    }
    /* shift offset back */
    for (i = 0; i < num_nodes; i++) {
        recv_offs[i] -= recv_cnts[i];
    }
    /* mark node roots in rank_map */
    for (i = 0; i < num_nodes; i++) {
        int root_rank = MPIR_Process.node_root_map[i];
        rank_map[root_rank] = -1;
    }
    /* prepare for Allgatherv */
    for (i = 0; i < num_nodes; i++) {
        recv_cnts[i] *= bc_len;
        recv_offs[i] *= bc_len;
    }

    int recv_bc_len = bc_len;
    if (!same_len) {
        recv_bc_len = MPID_MAX_BC_SIZE;
    }

    mpi_errno = MPIDU_Init_shm_barrier();
    MPIR_ERR_CHECK(mpi_errno);

    memcpy(segment + local_rank * recv_bc_len, bc, bc_len);

    mpi_errno = MPIDU_Init_shm_barrier();
    MPIR_ERR_CHECK(mpi_errno);

    /* a 64k memcpy is small (< 1ms), MPI_IN_PLACE not critical here */
    void *recv_buf = segment + local_size * recv_bc_len;
    if (rank == node_root) {
        MPIR_Errflag_t errflag = MPIR_ERR_NONE;
        MPIR_Allgatherv_fallback(segment, local_size * recv_bc_len, MPI_BYTE, recv_buf,
                                 recv_cnts, recv_offs, MPI_BYTE, allgather_comm, &errflag);

    }

    mpi_errno = MPIDU_Init_shm_barrier();
    MPIR_ERR_CHECK(mpi_errno);

    *bc_table = recv_buf;
    *bc_ranks = rank_map;
    *ret_bc_len = recv_bc_len;

  fn_exit:
    MPL_free(recv_cnts);
    MPL_free(recv_offs);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIDU_bc_table_create(int rank, int size, int *nodemap, void *bc, int bc_len, int same_len,
                          int roots_only, void **bc_table, int *ret_bc_len)
{
    int mpi_errno = MPI_SUCCESS;

    /* FIXME: rank, size, nodemap parameters are not needed */
    MPIR_Assert(MPIR_Process.rank == rank);
    MPIR_Assert(MPIR_Process.size == size);

    int recv_bc_len = bc_len;
    if (!same_len) {
        /* if business cards can be different length, use the max value length */
        recv_bc_len = MPID_MAX_BC_SIZE;
        /* Note: ret_bc_len is only touched if !same_len */
        *ret_bc_len = recv_bc_len;
    }

    mpi_errno = MPIDU_Init_shm_alloc(recv_bc_len * size, (void **) &segment);
    MPIR_ERR_CHECK(mpi_errno);

    if (size == 1) {
        memcpy(segment, bc, bc_len);
    } else {
        MPIR_PMI_DOMAIN domain = roots_only ? MPIR_PMI_DOMAIN_NODE_ROOTS : MPIR_PMI_DOMAIN_ALL;
        mpi_errno = MPIR_pmi_allgather_shm(bc, bc_len, segment, recv_bc_len, domain);
        MPIR_ERR_CHECK(mpi_errno);

        mpi_errno = MPIDU_Init_shm_barrier();
        MPIR_ERR_CHECK(mpi_errno);
    }

  fn_exit:
    *bc_table = segment;

    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
