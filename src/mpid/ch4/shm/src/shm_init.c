/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 */

#include "mpidimpl.h"
#include "shm_noinline.h"
#include "../posix/posix_noinline.h"

int MPIDI_SHMI_mpi_init_hook(int rank, int size, int *tag_bits)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_SHMI_MPI_INIT_HOOK);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_SHMI_MPI_INIT_HOOK);

    ret = MPIDI_POSIX_mpi_init_hook(rank, size, tag_bits);
    MPIR_ERR_CHECK(ret);

#ifdef MPIDI_CH4_SHM_ENABLE_XPMEM
    if (MPIR_CVAR_CH4_XPMEM_LMT_MSG_SIZE != -1) {
        ret = MPIDI_XPMEM_init();
        MPIR_ERR_CHECK(ret);
    }
#endif

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_SHMI_MPI_INIT_HOOK);
    return ret;
  fn_fail:
    goto fn_exit;
}

int MPIDI_SHMI_mpi_finalize_hook(void)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_SHMI_MPI_FINALIZE_HOOK);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_SHMI_MPI_FINALIZE_HOOK);

#ifdef MPIDI_CH4_SHM_ENABLE_XPMEM
    if (MPIR_CVAR_CH4_XPMEM_LMT_MSG_SIZE != -1) {
        ret = MPIDI_XPMEM_finalize();
        MPIR_ERR_CHECK(ret);
    }
#endif

    ret = MPIDI_POSIX_mpi_finalize_hook();
    MPIR_ERR_CHECK(ret);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_SHMI_MPI_FINALIZE_HOOK);
    return ret;
  fn_fail:
    goto fn_exit;
}

int MPIDI_SHMI_get_vci_attr(int vci)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_SHMI_QUERY_VCI);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_SHMI_QUERY_VCI);

    ret = MPIDI_POSIX_get_vci_attr(vci);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_SHMI_QUERY_VCI);
    return ret;
}

int MPIDI_SHMI_progress(int vci, int blocking)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_SHMI_PROGRESS);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_SHMI_PROGRESS);

    ret = MPIDI_POSIX_progress(blocking);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_SHMI_PROGRESS);
    return ret;
}
