/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpidimpl.h"
#include "shm_noinline.h"
#include "../posix/posix_noinline.h"
#include "../ipc/src/ipc_noinline.h"

MPIDI_SHM_global_t MPIDI_SHM_global;

int MPIDI_SHM_init_local(int *tag_bits)
{
    int mpi_errno = MPI_SUCCESS;

    /* There is no restriction on the tag_bits from the posix shmod side */
    *tag_bits = MPIR_TAG_BITS_DEFAULT;

    /* Initialize local_ranks table */
    MPIDI_SHM_global.local_ranks = (int *) MPL_malloc(MPIR_Process.size * sizeof(int), MPL_MEM_SHM);
    for (int i = 0; i < MPIR_Process.size; ++i) {
        MPIDI_SHM_global.local_ranks[i] = -1;
    }
    for (int i = 0; i < MPIR_Process.local_size; i++) {
        MPIDI_SHM_global.local_ranks[MPIR_Process.node_local_map[i]] = i;
    }

    /* - */
    mpi_errno = MPIDI_POSIX_init_local(NULL);
    MPIR_ERR_CHECK(mpi_errno);

    mpi_errno = MPIDI_IPC_init_local();
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIDI_SHM_comm_bootstrap(MPIR_Comm * comm)
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = MPIDI_POSIX_comm_bootstrap(comm);
    MPIR_ERR_CHECK(mpi_errno);

    mpi_errno = MPIDI_IPC_comm_bootstrap(comm);
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIDI_SHM_post_init(void)
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = MPIDI_POSIX_post_init();
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIDI_SHM_mpi_finalize_hook(void)
{
    int ret;

    MPIR_FUNC_ENTER;

    ret = MPIDI_IPC_mpi_finalize_hook();
    MPIR_ERR_CHECK(ret);

    ret = MPIDI_POSIX_mpi_finalize_hook();
    MPIR_ERR_CHECK(ret);

    MPL_free(MPIDI_SHM_global.local_ranks);
    MPIDI_SHM_global.local_ranks = NULL;

  fn_exit:
    MPIR_FUNC_EXIT;
    return ret;
  fn_fail:
    goto fn_exit;
}
