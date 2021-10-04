/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpidimpl.h"
#include "ipc_noinline.h"
#include "ipc_types.h"

int MPIDI_IPC_init_local(void)
{
    int mpi_errno = MPI_SUCCESS;

    MPL_COMPILE_TIME_ASSERT(offsetof(MPIDI_IPC_rts_t, ipc_hdr) == sizeof(MPIDIG_hdr_t));

#ifdef MPL_USE_DBG_LOGGING
    MPIDI_IPCI_DBG_GENERAL = MPL_dbg_class_alloc("SHM_IPC", "shm_ipc");
#endif

    MPIDI_IPCI_global.node_group_ptr = NULL;

    MPIDIG_am_rndv_reg_cb(MPIDIG_RNDV_IPC, &MPIDI_IPC_rndv_cb);
    MPIDIG_am_reg_cb(MPIDI_IPC_ACK, NULL, &MPIDI_IPC_ack_target_msg_cb);

    mpi_errno = MPIDI_XPMEM_init_local();
    MPIR_ERR_CHECK(mpi_errno);

    mpi_errno = MPIDI_GPU_init_local();
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIDI_IPC_init_world(void)
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = MPIDI_XPMEM_init_world();
    MPIR_ERR_CHECK(mpi_errno);

    if (MPIR_CVAR_ENABLE_GPU) {
        mpi_errno = MPIDI_GPU_init_world();
        MPIR_ERR_CHECK(mpi_errno);
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIDI_IPC_mpi_finalize_hook(void)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_ENTER;

    mpi_errno = MPIDI_XPMEM_mpi_finalize_hook();
    MPIR_ERR_CHECK(mpi_errno);

    mpi_errno = MPIDI_GPU_mpi_finalize_hook();
    MPIR_ERR_CHECK(mpi_errno);

    if (MPIDI_IPCI_global.node_group_ptr) {
        mpi_errno = MPIR_Group_free_impl(MPIDI_IPCI_global.node_group_ptr);
        MPIR_ERR_CHECK(mpi_errno);
    }

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
