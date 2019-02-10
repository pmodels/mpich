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
#ifndef CH4R_INIT_H_INCLUDED
#define CH4R_INIT_H_INCLUDED

#include "ch4_impl.h"
#include "ch4r_buf.h"
#include "ch4r_callbacks.h"
#include "ch4r_rma_target_callbacks.h"
#include "ch4r_rma_origin_callbacks.h"
#include "mpidig.h"
#include "uthash.h"

#undef FUNCNAME
#define FUNCNAME MPIDIG_init_comm
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPIDIG_init_comm(MPIR_Comm * comm)
{
    int mpi_errno = MPI_SUCCESS, comm_idx, subcomm_type, is_localcomm;
    MPIDIG_rreq_t **uelist;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIG_INIT_COMM);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIG_INIT_COMM);

    /*
     * Prevents double initialization of some special communicators.
     *
     * comm_world and comm_self may exhibit this function twice, first during MPIDIG_init
     * and the second during MPIR_Comm_commit in MPID_Init.
     * If there is an early arrival of an unexpected message before the second visit,
     * the following code will wipe out the unexpected queue andthe message is lost forever.
     */
    if (unlikely(MPIDI_CH4_Global.is_ch4u_initialized &&
                 (comm == MPIR_Process.comm_world || comm == MPIR_Process.comm_self)))
        goto fn_exit;
    if (MPIR_CONTEXT_READ_FIELD(DYNAMIC_PROC, comm->recvcontext_id))
        goto fn_exit;

    comm_idx = MPIDIG_get_context_index(comm->recvcontext_id);
    subcomm_type = MPIR_CONTEXT_READ_FIELD(SUBCOMM, comm->recvcontext_id);
    is_localcomm = MPIR_CONTEXT_READ_FIELD(IS_LOCALCOMM, comm->recvcontext_id);

    MPIR_Assert(subcomm_type <= 3);
    MPIR_Assert(is_localcomm <= 1);

    /* There is a potential race between this code (likely called by a user/main thread)
     * and an MPIDIG callback handler (called by a progress thread, when async progress
     * is turned on).
     * Thus we take a lock here to make sure the following operations are atomically done.
     * (transferring unexpected messages from a global queue to the newly created communicator) */
    MPID_THREAD_CS_ENTER(VCI, MPIDIU_THREAD_MPIDIG_GLOBAL_MUTEX);
    MPIDI_CH4_Global.comm_req_lists[comm_idx].comm[is_localcomm][subcomm_type] = comm;
    MPIDIG_COMM(comm, posted_list) = NULL;
    MPIDIG_COMM(comm, unexp_list) = NULL;

    uelist = MPIDIG_context_id_to_uelist(comm->context_id);
    if (*uelist) {
        MPIDIG_rreq_t *curr, *tmp;
        DL_FOREACH_SAFE(*uelist, curr, tmp) {
            DL_DELETE(*uelist, curr);
            MPIR_Comm_add_ref(comm);    /* +1 for each entry in unexp_list */
            DL_APPEND(MPIDIG_COMM(comm, unexp_list), curr);
        }
        *uelist = NULL;
    }
    MPID_THREAD_CS_EXIT(VCI, MPIDIU_THREAD_MPIDIG_GLOBAL_MUTEX);

    MPIDIG_COMM(comm, window_instance) = 0;
  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIG_INIT_COMM);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDIG_destroy_comm
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPIDIG_destroy_comm(MPIR_Comm * comm)
{
    int mpi_errno = MPI_SUCCESS, comm_idx, subcomm_type, is_localcomm;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIG_DESTROY_COMM);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIG_DESTROY_COMM);

    if (MPIR_CONTEXT_READ_FIELD(DYNAMIC_PROC, comm->recvcontext_id))
        goto fn_exit;
    comm_idx = MPIDIG_get_context_index(comm->recvcontext_id);
    subcomm_type = MPIR_CONTEXT_READ_FIELD(SUBCOMM, comm->recvcontext_id);
    is_localcomm = MPIR_CONTEXT_READ_FIELD(IS_LOCALCOMM, comm->recvcontext_id);

    MPIR_Assert(subcomm_type <= 3);
    MPIR_Assert(is_localcomm <= 1);

    MPID_THREAD_CS_ENTER(VCI, MPIDIU_THREAD_MPIDIG_GLOBAL_MUTEX);
    MPIR_Assert(MPIDI_CH4_Global.comm_req_lists[comm_idx].comm[is_localcomm][subcomm_type] != NULL);

    if (MPIDI_CH4_Global.comm_req_lists[comm_idx].comm[is_localcomm][subcomm_type]) {
        MPIR_Assert(MPIDIG_COMM
                    (MPIDI_CH4_Global.comm_req_lists[comm_idx].comm[is_localcomm][subcomm_type],
                     posted_list) == NULL);
        MPIR_Assert(MPIDIG_COMM
                    (MPIDI_CH4_Global.comm_req_lists[comm_idx].comm[is_localcomm][subcomm_type],
                     unexp_list) == NULL);
    }
    MPIDI_CH4_Global.comm_req_lists[comm_idx].comm[is_localcomm][subcomm_type] = NULL;
    MPID_THREAD_CS_EXIT(VCI, MPIDIU_THREAD_MPIDIG_GLOBAL_MUTEX);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIG_DESTROY_COMM);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDIG_mpi_alloc_mem
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX void *MPIDIG_mpi_alloc_mem(size_t size, MPIR_Info * info_ptr)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIG_MPI_ALLOC_MEM);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIG_MPI_ALLOC_MEM);
    void *p;
    p = MPL_malloc(size, MPL_MEM_USER);
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIG_MPI_ALLOC_MEM);
    return p;
}

#undef FUNCNAME
#define FUNCNAME MPIDIG_mpi_free_mem
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPIDIG_mpi_free_mem(void *ptr)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIG_MPI_FREE_MEM);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIG_MPI_FREE_MEM);
    MPL_free(ptr);
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIG_MPI_FREE_MEM);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDIU_update_node_map
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDIU_update_node_map(int avtid, int size, int node_map[])
{
    int i;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIU_UPDATE_NODE_MAP);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIU_UPDATE_NODE_MAP);
    for (i = 0; i < size; i++) {
        MPIDI_CH4_Global.node_map[avtid][i] = node_map[i];
    }
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIU_UPDATE_NODE_MAP);
    return MPI_SUCCESS;
}

#endif /* CH4R_INIT_H_INCLUDED */
