/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 */

#include "mpidimpl.h"
#include "shm_noinline.h"
#include "../posix/posix_noinline.h"
#ifdef MPIDI_CH4_SHM_ENABLE_XPMEM
#include "../xpmem/xpmem_noinline.h"
#endif

int MPIDI_SHMI_mpi_init_hook(int rank, int size, int *n_vcis_provided, int *tag_bits)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_SHMI_MPI_INIT_HOOK);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_SHMI_MPI_INIT_HOOK);

    ret = MPIDI_POSIX_mpi_init_hook(rank, size, n_vcis_provided, tag_bits);
    MPIR_ERR_CHECK(ret);

#ifdef MPIDI_CH4_SHM_ENABLE_XPMEM
    if (MPIR_CVAR_CH4_XPMEM_LMT_MSG_SIZE != -1) {
        ret = MPIDI_XPMEM_mpi_init_hook(rank, size, n_vcis_provided, tag_bits);
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
        ret = MPIDI_XPMEM_mpi_finalize_hook();
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

int MPIDI_SHMI_mpi_comm_set_info_mutable(MPIR_Comm * comm, MPIR_Info * info)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_SHM_MPI_COMM_SET_INFO_MUTABLE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_SHM_MPI_COMM_SET_INFO_MUTABLE);

    ret = MPIDI_POSIX_mpi_comm_set_info_mutable(comm, info);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_SHM_MPI_COMM_SET_INFO_MUTABLE);
    return ret;
}

int MPIDI_SHMI_mpi_comm_set_info_immutable(MPIR_Comm * comm, MPIR_Info * info)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_SHM_MPI_COMM_SET_INFO_IMMUTABLE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_SHM_MPI_COMM_SET_INFO_IMMUTABLE);

    ret = MPIDI_POSIX_mpi_comm_set_info_immutable(comm, info);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_SHM_MPI_COMM_SET_INFO_IMMUTABLE);
    return ret;
}

int MPIDI_SHMI_mpi_comm_get_info(MPIR_Comm * comm, MPIR_Info ** info_p_p)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_SHM_MPI_COMM_GET_INFO);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_SHM_MPI_COMM_GET_INFO);

    ret = MPIDI_POSIX_mpi_comm_get_info(comm, info_p_p);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_SHM_MPI_COMM_GET_INFO);
    return ret;
}
