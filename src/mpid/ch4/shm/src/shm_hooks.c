/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpidimpl.h"
#include "shm_noinline.h"
#include "../posix/posix_noinline.h"
#include "../ipc/src/ipc_noinline.h"

int MPIDI_SHM_mpi_comm_commit_pre_hook(MPIR_Comm * comm)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_SHM_MPI_COMM_COMMIT_PRE_HOOK);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_SHM_MPI_COMM_COMMIT_PRE_HOOK);

    ret = MPIDI_POSIX_mpi_comm_commit_pre_hook(comm);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_SHM_MPI_COMM_COMMIT_PRE_HOOK);
    return ret;
}

int MPIDI_SHM_mpi_comm_commit_post_hook(MPIR_Comm * comm)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_SHM_MPI_COMM_COMMIT_POST_HOOK);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_SHM_MPI_COMM_COMMIT_POST_HOOK);

    ret = MPIDI_POSIX_mpi_comm_commit_post_hook(comm);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_SHM_MPI_COMM_COMMIT_POST_HOOK);
    return ret;
}

int MPIDI_SHM_mpi_comm_free_hook(MPIR_Comm * comm)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_SHM_MPI_COMM_FREE_HOOK);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_SHM_MPI_COMM_FREE_HOOK);

    ret = MPIDI_POSIX_mpi_comm_free_hook(comm);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_SHM_MPI_COMM_FREE_HOOK);
    return ret;
}

int MPIDI_SHM_mpi_type_commit_hook(MPIR_Datatype * type)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_SHM_MPI_TYPE_COMMIT_HOOK);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_SHM_MPI_TYPE_COMMIT_HOOK);

    ret = MPIDI_POSIX_mpi_type_commit_hook(type);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_SHM_MPI_TYPE_COMMIT_HOOK);
    return ret;
}

int MPIDI_SHM_mpi_type_free_hook(MPIR_Datatype * type)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_SHM_MPI_TYPE_FREE_HOOK);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_SHM_MPI_TYPE_FREE_HOOK);

    ret = MPIDI_POSIX_mpi_type_free_hook(type);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_SHM_MPI_TYPE_FREE_HOOK);
    return ret;
}

int MPIDI_SHM_mpi_op_commit_hook(MPIR_Op * op)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_SHM_MPI_OP_COMMIT_HOOK);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_SHM_MPI_OP_COMMIT_HOOK);

    ret = MPIDI_POSIX_mpi_op_commit_hook(op);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_SHM_MPI_OP_COMMIT_HOOK);
    return ret;
}

int MPIDI_SHM_mpi_op_free_hook(MPIR_Op * op)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_SHM_MPI_OP_FREE_HOOK);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_SHM_MPI_OP_FREE_HOOK);

    ret = MPIDI_POSIX_mpi_op_free_hook(op);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_SHM_MPI_OP_FREE_HOOK);
    return ret;
}

int MPIDI_SHM_mpi_win_create_hook(MPIR_Win * win)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_SHM_MPI_WIN_CREATE_HOOK);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_SHM_MPI_WIN_CREATE_HOOK);

    ret = MPIDI_POSIX_mpi_win_create_hook(win);
    MPIR_ERR_CHECK(ret);

    ret = MPIDI_IPC_mpi_win_create_hook(win);
    MPIR_ERR_CHECK(ret);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_SHM_MPI_WIN_CREATE_HOOK);
    return ret;
  fn_fail:
    goto fn_exit;
}

int MPIDI_SHM_mpi_win_allocate_hook(MPIR_Win * win)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_SHM_MPI_WIN_ALLOCATE_HOOK);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_SHM_MPI_WIN_ALLOCATE_HOOK);

    ret = MPIDI_POSIX_mpi_win_allocate_hook(win);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_SHM_MPI_WIN_ALLOCATE_HOOK);
    return ret;
}

int MPIDI_SHM_mpi_win_allocate_shared_hook(MPIR_Win * win)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_SHM_MPI_WIN_ALLOCATE_SHARED_HOOK);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_SHM_MPI_WIN_ALLOCATE_SHARED_HOOK);

    ret = MPIDI_POSIX_mpi_win_allocate_shared_hook(win);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_SHM_MPI_WIN_ALLOCATE_SHARED_HOOK);
    return ret;
}

int MPIDI_SHM_mpi_win_create_dynamic_hook(MPIR_Win * win)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_SHM_MPI_WIN_CREATE_DYNAMIC_HOOK);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_SHM_MPI_WIN_CREATE_DYNAMIC_HOOK);

    ret = MPIDI_POSIX_mpi_win_create_dynamic_hook(win);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_SHM_MPI_WIN_CREATE_DYNAMIC_HOOK);
    return ret;
}

int MPIDI_SHM_mpi_win_attach_hook(MPIR_Win * win, void *base, MPI_Aint size)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_SHM_MPI_WIN_ATTACH_HOOK);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_SHM_MPI_WIN_ATTACH_HOOK);

    ret = MPIDI_POSIX_mpi_win_attach_hook(win, base, size);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_SHM_MPI_WIN_ATTACH_HOOK);
    return ret;
}

int MPIDI_SHM_mpi_win_detach_hook(MPIR_Win * win, const void *base)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_SHM_MPI_WIN_DETACH_HOOK);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_SHM_MPI_WIN_DETACH_HOOK);

    ret = MPIDI_POSIX_mpi_win_detach_hook(win, base);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_SHM_MPI_WIN_DETACH_HOOK);
    return ret;
}

int MPIDI_SHM_mpi_win_free_hook(MPIR_Win * win)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_SHM_MPI_WIN_FREE_HOOK);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_SHM_MPI_WIN_FREE_HOOK);

    ret = MPIDI_IPC_mpi_win_free_hook(win);
    MPIR_ERR_CHECK(ret);

    ret = MPIDI_POSIX_mpi_win_free_hook(win);
    MPIR_ERR_CHECK(ret);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_SHM_MPI_WIN_FREE_HOOK);
    return ret;
  fn_fail:
    goto fn_exit;
}
