/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2019 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include <mpidimpl.h>
#include "mpidu_init_shm.h"
#include "build_nodemap.h"

static int local_size;
static int my_local_rank;
static MPIDU_shm_seg_t memory;
static MPIDU_shm_barrier_t *barrier;
static void *baseaddr;

int MPIDU_Init_shm_init(int rank, int size, int *nodemap)
{
    int mpi_errno = MPI_SUCCESS;
    int local_leader;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDU_INIT_SHM_INIT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDU_INIT_SHM_INIT);

    MPIR_NODEMAP_get_local_info(rank, size, nodemap, &local_size, &my_local_rank, &local_leader);

    mpi_errno =
        MPIDU_shm_seg_alloc(local_size * sizeof(MPIDU_Init_shm_block_t), (void **) &baseaddr,
                            MPL_MEM_SHM);
    MPIR_ERR_CHECK(mpi_errno);

    mpi_errno =
        MPIDU_shm_seg_commit(&memory, &barrier, local_size, my_local_rank, local_leader, rank,
                             MPL_MEM_SHM);
    MPIR_ERR_CHECK(mpi_errno);

    mpi_errno = MPIDU_shm_barrier(barrier, local_size);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDU_INIT_SHM_INIT);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIDU_Init_shm_finalize(void)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDU_INIT_SHM_FINALIZE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDU_INIT_SHM_FINALIZE);

    mpi_errno = MPIDU_shm_barrier(barrier, local_size);
    MPIR_ERR_CHECK(mpi_errno);

    mpi_errno = MPIDU_shm_seg_destroy(&memory, local_size);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDU_INIT_SHM_FINALIZE);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIDU_Init_shm_barrier(void)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDU_INIT_SHM_BARRIER);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDU_INIT_SHM_BARRIER);

    mpi_errno = MPIDU_shm_barrier(barrier, local_size);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDU_INIT_SHM_BARRIER);

    return mpi_errno;
}

int MPIDU_Init_shm_put(void *orig, size_t len)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDU_INIT_SHM_PUT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDU_INIT_SHM_PUT);

    MPIR_Assert(len <= sizeof(MPIDU_Init_shm_block_t));
    MPIR_Memcpy((char *) baseaddr + my_local_rank * sizeof(MPIDU_Init_shm_block_t), orig, len);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDU_INIT_SHM_PUT);

    return mpi_errno;
}

int MPIDU_Init_shm_get(int local_rank, void **target)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDU_INIT_SHM_GET);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDU_INIT_SHM_GET);

    MPIR_Assert(local_rank < local_size);
    *target = (char *) baseaddr + local_rank * sizeof(MPIDU_Init_shm_block_t);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDU_INIT_SHM_GET);

    return mpi_errno;
}
