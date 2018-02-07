/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 */

#ifndef SHM_HOOKS_H_INCLUDED
#define SHM_HOOKS_H_INCLUDED

#include <shm.h>
#include "../posix/shm_direct.h"

static inline int MPIDI_SHM_mpi_comm_create_hook(MPIR_Comm * comm)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_POSIX_MPI_COMM_CREATE_HOOK);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_POSIX_MPI_COMM_CREATE_HOOK);

    ret = MPIDI_POSIX_mpi_comm_create_hook(comm);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_POSIX_MPI_COMM_CREATE_HOOK);
    return ret;
}

static inline int MPIDI_SHM_mpi_comm_free_hook(MPIR_Comm * comm)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_POSIX_MPI_COMM_FREE_HOOK);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_POSIX_MPI_COMM_FREE_HOOK);

    ret = MPIDI_POSIX_mpi_comm_free_hook(comm);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_POSIX_MPI_COMM_FREE_HOOK);
    return ret;
}

static inline int MPIDI_SHM_mpi_type_commit_hook(MPIR_Datatype * type)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_POSIX_MPI_TYPE_COMMIT_HOOK);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_POSIX_MPI_TYPE_COMMIT_HOOK);

    ret = MPIDI_POSIX_mpi_type_commit_hook(type);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_POSIX_MPI_TYPE_COMMIT_HOOK);
    return ret;
}

static inline int MPIDI_SHM_mpi_type_free_hook(MPIR_Datatype * type)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_POSIX_MPI_TYPE_FREE_HOOK);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_POSIX_MPI_TYPE_FREE_HOOK);

    ret = MPIDI_POSIX_mpi_type_free_hook(type);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_POSIX_MPI_TYPE_FREE_HOOK);
    return ret;
}

static inline int MPIDI_SHM_mpi_op_commit_hook(MPIR_Op * op)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_POSIX_MPI_OP_COMMIT_HOOK);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_POSIX_MPI_OP_COMMIT_HOOK);

    ret = MPIDI_POSIX_mpi_op_commit_hook(op);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_POSIX_MPI_OP_COMMIT_HOOK);
    return ret;
}

static inline int MPIDI_SHM_mpi_op_free_hook(MPIR_Op * op)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_POSIX_MPI_OP_FREE_HOOK);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_POSIX_MPI_OP_FREE_HOOK);

    ret = MPIDI_POSIX_mpi_op_free_hook(op);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_POSIX_MPI_OP_FREE_HOOK);
    return ret;
}

#endif /* SHM_HOOKS_H_INCLUDED */
