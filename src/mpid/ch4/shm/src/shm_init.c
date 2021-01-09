/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpidimpl.h"
#include "shm_noinline.h"
#include "../posix/posix_noinline.h"
#include "../ipc/src/ipc_noinline.h"

int MPIDI_SHM_init_local(int *tag_bits)
{
    /* There is no restriction on the tag_bits from the posix shmod side */
    *tag_bits = MPIR_TAG_BITS_DEFAULT;
    return MPI_SUCCESS;
}

int MPIDI_SHM_init_world(void)
{
    int mpi_errno = MPI_SUCCESS;

    int tmp = MPIR_Process.tag_bits;
    mpi_errno = MPIDI_SHM_mpi_init_hook(MPIR_Process.rank, MPIR_Process.size, &tmp);
    /* the code updates tag_bits should be moved to MPIDI_xxx_init_local */
    MPIR_Assert(tmp == MPIR_Process.tag_bits);

    return mpi_errno;
}

int MPIDI_SHM_mpi_init_hook(int rank, int size, int *tag_bits)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_SHM_MPI_INIT_HOOK);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_SHM_MPI_INIT_HOOK);


    ret = MPIDI_POSIX_mpi_init_hook(rank, size, tag_bits);
    MPIR_ERR_CHECK(ret);

    ret = MPIDI_IPC_mpi_init_hook(rank, size, tag_bits);
    MPIR_ERR_CHECK(ret);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_SHM_MPI_INIT_HOOK);
    return ret;
  fn_fail:
    goto fn_exit;
}

int MPIDI_SHM_mpi_finalize_hook(void)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_SHM_MPI_FINALIZE_HOOK);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_SHM_MPI_FINALIZE_HOOK);

    ret = MPIDI_IPC_mpi_finalize_hook();
    MPIR_ERR_CHECK(ret);

    ret = MPIDI_POSIX_mpi_finalize_hook();
    MPIR_ERR_CHECK(ret);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_SHM_MPI_FINALIZE_HOOK);
    return ret;
  fn_fail:
    goto fn_exit;
}

int MPIDI_SHM_get_vci_attr(int vci)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_SHM_GET_VCI_ATTR);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_SHM_GET_VCI_ATTR);

    ret = MPIDI_POSIX_get_vci_attr(vci);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_SHM_GET_VCI_ATTR);
    return ret;
}

int MPIDI_SHM_progress(int vci, int blocking)
{
    int ret = MPI_SUCCESS;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_SHM_PROGRESS);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_SHM_PROGRESS);

    ret = MPIDI_POSIX_progress(vci, blocking);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_SHM_PROGRESS);
    return ret;
}
