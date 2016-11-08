/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 *  Portions of this code were written by Intel Corporation.
 *  Copyright (C) 2011-2016 Intel Corporation.  Intel provides this material
 *  to Argonne National Laboratory subject to Software Grant and Corporate
 *  Contributor License Agreement dated February 8, 2012.
 */
#ifndef SHM_STUBSHM_INIT_H_INCLUDED
#define SHM_STUBSHM_INIT_H_INCLUDED

#include "stubshm_impl.h"

static inline int MPIDI_SHM_mpi_init_hook(int rank, int size)
{
    MPIR_Assert(0);
    return MPI_SUCCESS;
}

static inline int MPIDI_SHM_mpi_finalize_hook(void)
{
    MPIR_Assert(0);
    return MPI_SUCCESS;
}

static inline void *MPIDI_SHM_mpi_alloc_mem(size_t size, MPIR_Info * info_ptr)
{
    MPIR_Assert(0);
    return NULL;
}

static inline int MPIDI_SHM_mpi_free_mem(void *ptr)
{
    MPIR_Assert(0);
    return MPI_SUCCESS;
}

static inline int MPIDI_SHM_comm_get_lpid(MPIR_Comm * comm_ptr,
                                          int idx, int *lpid_ptr, MPL_bool is_remote)
{
    MPIR_Assert(0);
    return MPI_SUCCESS;
}

static inline int MPIDI_SHM_get_node_id(MPIR_Comm * comm, int rank, MPID_Node_id_t * id_p)
{
    *id_p = (MPID_Node_id_t) 0;
    return MPI_SUCCESS;
}

static inline int MPIDI_SHM_get_max_node_id(MPIR_Comm * comm, MPID_Node_id_t * max_id_p)
{
    *max_id_p = (MPID_Node_id_t) 1;
    return MPI_SUCCESS;
}

static inline int MPIDI_SHM_get_local_upids(MPIR_Comm * comm, size_t ** local_upid_size,
                                            char **local_upids)
{
    MPIR_Assert(0);
    return MPI_SUCCESS;
}

static inline int MPIDI_SHM_upids_to_lupids(int size,
                                            size_t * remote_upid_size,
                                            char *remote_upids, int **remote_lupids)
{
    MPIR_Assert(0);
    return MPI_SUCCESS;
}

static inline int MPIDI_SHM_create_intercomm_from_lpids(MPIR_Comm * newcomm_ptr,
                                                        int size, const int lpids[])
{
    MPIR_Assert(0);
    return MPI_SUCCESS;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_SHM_mpi_type_create_hook
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_SHM_mpi_type_create_hook(MPIR_Datatype * type)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_SHM_TYPE_CREATE_HOOK);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_SHM_TYPE_CREATE_HOOK);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_SHM_TYPE_CREATE_HOOK);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_SHM_mpi_type_free_hook
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_SHM_mpi_type_free_hook(MPIR_Datatype * type)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_SHM_TYPE_FREE_HOOK);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_SHM_TYPE_FREE_HOOK);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_SHM_TYPE_FREE_HOOK);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_SHM_mpi_op_create_hook
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_SHM_mpi_op_create_hook(MPIR_Op * op)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_SHM_OP_CREATE_HOOK);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_SHM_OP_CREATE_HOOK);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_SHM_OP_CREATE_HOOK);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_SHM_mpi_op_free_hook
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_SHM_mpi_op_free_hook(MPIR_Op * op)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_SHM_OP_FREE_HOOK);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_SHM_OP_FREE_HOOK);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_SHM_OP_FREE_HOOK);
    return mpi_errno;
}

#endif /* SHM_STUBSHM_INIT_H_INCLUDED */
