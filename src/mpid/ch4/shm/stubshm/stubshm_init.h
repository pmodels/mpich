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
#ifndef STUBSHM_INIT_H_INCLUDED
#define STUBSHM_INIT_H_INCLUDED

#include "stubshm_impl.h"

static inline int MPIDI_STUBSHM_mpi_init_hook(int rank, int size, int *n_vnis_provided)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_STUBSHM_MPI_INIT_HOOK);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_STUBSHM_MPI_INIT_HOOK);

    MPIR_Assert(0);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_STUBSHM_MPI_INIT_HOOK);
    return MPI_SUCCESS;
}

static inline int MPIDI_STUBSHM_get_vni_attr(int vni)
{
    int ret = 0;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_STUBSHM_QUERY_VNI);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_STUBSHM_QUERY_VNI);

    MPIR_Assert(0);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_STUBSHM_QUERY_VNI);
    return ret;
}

static inline int MPIDI_STUBSHM_mpi_finalize_hook(void)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_STUBSHM_MPI_FINALIZE_HOOK);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_STUBSHM_MPI_FINALIZE_HOOK);

    MPIR_Assert(0);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_STUBSHM_MPI_FINALIZE_HOOK);
    return MPI_SUCCESS;
}

static inline void *MPIDI_STUBSHM_mpi_alloc_mem(size_t size, MPIR_Info * info_ptr)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_STUBSHM_MPI_ALLOC_MEM);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_STUBSHM_MPI_ALLOC_MEM);

    MPIR_Assert(0);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_STUBSHM_MPI_ALLOC_MEM);
    return NULL;
}

static inline int MPIDI_STUBSHM_mpi_free_mem(void *ptr)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_STUBSHM_MPI_FREE_MEM);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_STUBSHM_MPI_FREE_MEM);

    MPIR_Assert(0);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_STUBSHM_MPI_FREE_MEM);
    return MPI_SUCCESS;
}

static inline int MPIDI_STUBSHM_comm_get_lpid(MPIR_Comm * comm_ptr,
                                          int idx, int *lpid_ptr, MPL_bool is_remote)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_STUBSHM_COMM_GET_LPID);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_STUBSHM_COMM_GET_LPID);

    MPIR_Assert(0);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_STUBSHM_COMM_GET_LPID);
    return MPI_SUCCESS;
}

static inline int MPIDI_STUBSHM_get_node_id(MPIR_Comm * comm, int rank, MPID_Node_id_t * id_p)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_STUBSHM_GET_NODE_ID);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_STUBSHM_GET_NODE_ID);

    *id_p = (MPID_Node_id_t) 0;

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_STUBSHM_GET_NODE_ID);
    return MPI_SUCCESS;
}

static inline int MPIDI_STUBSHM_get_max_node_id(MPIR_Comm * comm, MPID_Node_id_t * max_id_p)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_STUBSHM_GET_MAX_NODE_ID);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_STUBSHM_GET_MAX_NODE_ID);

    *max_id_p = (MPID_Node_id_t) 0;

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_STUBSHM_GET_MAX_NODE_ID);
    return MPI_SUCCESS;
}

static inline int MPIDI_STUBSHM_get_local_upids(MPIR_Comm * comm, size_t ** local_upid_size,
                                            char **local_upids)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_STUBSHM_GET_LOCAL_UPIDS);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_STUBSHM_GET_LOCAL_UPIDS);

    MPIR_Assert(0);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_STUBSHM_GET_LOCAL_UPIDS);
    return MPI_SUCCESS;
}

static inline int MPIDI_STUBSHM_upids_to_lupids(int size,
                                            size_t * remote_upid_size,
                                            char *remote_upids, int **remote_lupids)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_STUBSHM_UPIDS_TO_LUPIDS);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_STUBSHM_UPIDS_TO_LUPIDS);

    MPIR_Assert(0);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_STUBSHM_UPIDS_TO_LUPIDS);
    return MPI_SUCCESS;
}

static inline int MPIDI_STUBSHM_create_intercomm_from_lpids(MPIR_Comm * newcomm_ptr,
                                                        int size, const int lpids[])
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_STUBSHM_CREATE_INTERCOMM_FROM_LPIDS);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_STUBSHM_CREATE_INTERCOMM_FROM_LPIDS);

    MPIR_Assert(0);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_STUBSHM_CREATE_INTERCOMM_FROM_LPIDS);
    return MPI_SUCCESS;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_STUBSHM_mpi_type_commit_hook
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_STUBSHM_mpi_type_commit_hook(MPIR_Datatype * type)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_STUBSHM_MPI_TYPE_CREATE_HOOK);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_STUBSHM_MPI_TYPE_CREATE_HOOK);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_STUBSHM_MPI_TYPE_CREATE_HOOK);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_STUBSHM_mpi_type_free_hook
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_STUBSHM_mpi_type_free_hook(MPIR_Datatype * type)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_STUBSHM_MPI_TYPE_FREE_HOOK);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_STUBSHM_MPI_TYPE_FREE_HOOK);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_STUBSHM_MPI_TYPE_FREE_HOOK);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_STUBSHM_mpi_op_commit_hook
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_STUBSHM_mpi_op_commit_hook(MPIR_Op * op)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_STUBSHM_MPI_OP_CREATE_HOOK);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_STUBSHM_MPI_OP_CREATE_HOOK);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_STUBSHM_MPI_OP_CREATE_HOOK);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_STUBSHM_mpi_op_free_hook
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_STUBSHM_mpi_op_free_hook(MPIR_Op * op)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_STUBSHM_MPI_OP_FREE_HOOK);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_STUBSHM_MPI_OP_FREE_HOOK);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_STUBSHM_MPI_OP_FREE_HOOK);
    return mpi_errno;
}

#endif /* STUBSHM_INIT_H_INCLUDED */
