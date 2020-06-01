/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpidimpl.h"
#include "ipc_noinline.h"
#include "ipc_control.h"
#include "shm_control.h"
#include "ipc_types.h"

static void register_shm_ctrl_cb(void)
{
    MPIDI_SHMI_ctrl_reg_cb(MPIDI_IPC_SEND_CONTIG_LMT_RTS, &MPIDI_IPCI_send_contig_lmt_rts_cb);
    MPIDI_SHMI_ctrl_reg_cb(MPIDI_IPC_SEND_CONTIG_LMT_FIN, &MPIDI_IPCI_send_contig_lmt_fin_cb);
}

int MPIDI_IPC_mpi_init_hook(int rank, int size, int *tag_bits)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_IPC_MPI_INIT_HOOK);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_IPC_MPI_INIT_HOOK);

#ifdef MPL_USE_DBG_LOGGING
    MPIDI_IPCI_DBG_GENERAL = MPL_dbg_class_alloc("SHM_IPC", "shm_ipc");
#endif

    register_shm_ctrl_cb();

    MPIDI_IPCI_global.node_group_ptr = NULL;

    mpi_errno = MPIDI_XPMEM_mpi_init_hook(rank, size, tag_bits);
    MPIR_ERR_CHECK(mpi_errno);

    mpi_errno = MPIDI_GPU_mpi_init_hook(rank, size, tag_bits);
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_IPC_MPI_INIT_HOOK);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIDI_IPC_mpi_finalize_hook(void)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_IPC_MPI_FINALIZE_HOOK);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_IPC_MPI_FINALIZE_HOOK);

    mpi_errno = MPIDI_XPMEM_mpi_finalize_hook();
    MPIR_ERR_CHECK(mpi_errno);

    mpi_errno = MPIDI_GPU_mpi_finalize_hook();
    MPIR_ERR_CHECK(mpi_errno);

    if (MPIDI_IPCI_global.node_group_ptr) {
        mpi_errno = MPIR_Group_free_impl(MPIDI_IPCI_global.node_group_ptr);
        MPIR_ERR_CHECK(mpi_errno);
    }

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_IPC_MPI_FINALIZE_HOOK);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
