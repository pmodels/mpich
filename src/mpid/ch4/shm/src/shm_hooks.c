/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2019 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 */

#include "mpidimpl.h"
#include "shm_noinline.h"
#include "../posix/posix_noinline.h"
#ifdef MPIDI_CH4_SHM_ENABLE_XPMEM
#include "../xpmem/xpmem_noinline.h"
#endif

int MPIDI_SHMI_mpi_comm_create_hook(MPIR_Comm * comm)
{
    int ret;


    ret = MPIDI_POSIX_mpi_comm_create_hook(comm);

    return ret;
}

int MPIDI_SHMI_mpi_comm_free_hook(MPIR_Comm * comm)
{
    int ret;


    ret = MPIDI_POSIX_mpi_comm_free_hook(comm);

    return ret;
}

int MPIDI_SHMI_mpi_type_commit_hook(MPIR_Datatype * type)
{
    int ret;


    ret = MPIDI_POSIX_mpi_type_commit_hook(type);

    return ret;
}

int MPIDI_SHMI_mpi_type_free_hook(MPIR_Datatype * type)
{
    int ret;


    ret = MPIDI_POSIX_mpi_type_free_hook(type);

    return ret;
}

int MPIDI_SHMI_mpi_op_commit_hook(MPIR_Op * op)
{
    int ret;


    ret = MPIDI_POSIX_mpi_op_commit_hook(op);

    return ret;
}

int MPIDI_SHMI_mpi_op_free_hook(MPIR_Op * op)
{
    int ret;


    ret = MPIDI_POSIX_mpi_op_free_hook(op);

    return ret;
}

int MPIDI_SHMI_mpi_win_create_hook(MPIR_Win * win)
{
    int ret;


    ret = MPIDI_POSIX_mpi_win_create_hook(win);
    MPIR_ERR_CHECK(ret);

#ifdef MPIDI_CH4_SHM_ENABLE_XPMEM
    if (MPIR_CVAR_CH4_XPMEM_LMT_MSG_SIZE != -1) {
        ret = MPIDI_XPMEM_mpi_win_create_hook(win);
        MPIR_ERR_CHECK(ret);
    }
#endif

  fn_exit:
    return ret;
  fn_fail:
    goto fn_exit;
}

int MPIDI_SHMI_mpi_win_allocate_hook(MPIR_Win * win)
{
    int ret;


    ret = MPIDI_POSIX_mpi_win_allocate_hook(win);

    return ret;
}

int MPIDI_SHMI_mpi_win_allocate_shared_hook(MPIR_Win * win)
{
    int ret;


    ret = MPIDI_POSIX_mpi_win_allocate_shared_hook(win);

    return ret;
}

int MPIDI_SHMI_mpi_win_create_dynamic_hook(MPIR_Win * win)
{
    int ret;


    ret = MPIDI_POSIX_mpi_win_create_dynamic_hook(win);

    return ret;
}

int MPIDI_SHMI_mpi_win_attach_hook(MPIR_Win * win, void *base, MPI_Aint size)
{
    int ret;


    ret = MPIDI_POSIX_mpi_win_attach_hook(win, base, size);

    return ret;
}

int MPIDI_SHMI_mpi_win_detach_hook(MPIR_Win * win, const void *base)
{
    int ret;


    ret = MPIDI_POSIX_mpi_win_detach_hook(win, base);

    return ret;
}

int MPIDI_SHMI_mpi_win_free_hook(MPIR_Win * win)
{
    int ret;


#ifdef MPIDI_CH4_SHM_ENABLE_XPMEM
    if (MPIR_CVAR_CH4_XPMEM_LMT_MSG_SIZE != -1) {
        ret = MPIDI_XPMEM_mpi_win_free_hook(win);
        MPIR_ERR_CHECK(ret);
    }
#endif
    ret = MPIDI_POSIX_mpi_win_free_hook(win);
    MPIR_ERR_CHECK(ret);

  fn_exit:
    return ret;
  fn_fail:
    goto fn_exit;
}
