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
#include "ch4i_util.h"
#include "ch4r_buf.h"
#include "ch4r_callbacks.h"
#include "ch4r_rma_target_callbacks.h"
#include "ch4r_rma_origin_callbacks.h"
#include "mpidig.h"
#include "mpl_uthash.h"

#undef FUNCNAME
#define FUNCNAME MPIDI_CH4U_init_comm
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPIDI_CH4U_init_comm(MPIR_Comm * comm)
{
    int mpi_errno = MPI_SUCCESS, comm_idx, subcomm_type, is_localcomm;
    MPIDI_CH4U_rreq_t **uelist;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_CH4U_INIT_COMM);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_CH4U_INIT_COMM);

    /*
     * Prevents double initialization of some special communicators.
     *
     * comm_world and comm_self may exhibit this function twice, first during MPIDI_CH4U_mpi_init
     * and the second during MPIR_Comm_commit in MPID_Init.
     * If there is an early arrival of an unexpected message before the second visit,
     * the following code will wipe out the unexpected queue andthe message is lost forever.
     */
    if (unlikely(MPIDI_CH4_Global.is_ch4u_initialized &&
                 (comm == MPIR_Process.comm_world || comm == MPIR_Process.comm_self)))
        goto fn_exit;
    if (MPIR_CONTEXT_READ_FIELD(DYNAMIC_PROC, comm->recvcontext_id))
        goto fn_exit;

    comm_idx = MPIDI_CH4U_get_context_index(comm->recvcontext_id);
    subcomm_type = MPIR_CONTEXT_READ_FIELD(SUBCOMM, comm->recvcontext_id);
    is_localcomm = MPIR_CONTEXT_READ_FIELD(IS_LOCALCOMM, comm->recvcontext_id);

    MPIR_Assert(subcomm_type <= 3);
    MPIR_Assert(is_localcomm <= 1);
    MPIDI_CH4_Global.comm_req_lists[comm_idx].comm[is_localcomm][subcomm_type] = comm;
    MPIDI_CH4U_COMM(comm, posted_list) = NULL;
    MPIDI_CH4U_COMM(comm, unexp_list) = NULL;

    uelist = MPIDI_CH4U_context_id_to_uelist(comm->context_id);
    if (*uelist) {
        MPIDI_CH4U_rreq_t *curr, *tmp;
        MPL_DL_FOREACH_SAFE(*uelist, curr, tmp) {
            MPL_DL_DELETE(*uelist, curr);
            MPIR_Comm_add_ref(comm);    /* +1 for each entry in unexp_list */
            MPL_DL_APPEND(MPIDI_CH4U_COMM(comm, unexp_list), curr);
        }
        *uelist = NULL;
    }

    MPIDI_CH4U_COMM(comm, window_instance) = 0;
  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_CH4U_INIT_COMM);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH4U_destroy_comm
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPIDI_CH4U_destroy_comm(MPIR_Comm * comm)
{
    int mpi_errno = MPI_SUCCESS, comm_idx, subcomm_type, is_localcomm;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_CH4U_DESTROY_COMM);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_CH4U_DESTROY_COMM);

    if (MPIR_CONTEXT_READ_FIELD(DYNAMIC_PROC, comm->recvcontext_id))
        goto fn_exit;
    comm_idx = MPIDI_CH4U_get_context_index(comm->recvcontext_id);
    subcomm_type = MPIR_CONTEXT_READ_FIELD(SUBCOMM, comm->recvcontext_id);
    is_localcomm = MPIR_CONTEXT_READ_FIELD(IS_LOCALCOMM, comm->recvcontext_id);

    MPIR_Assert(subcomm_type <= 3);
    MPIR_Assert(is_localcomm <= 1);
    MPIR_Assert(MPIDI_CH4_Global.comm_req_lists[comm_idx].comm[is_localcomm][subcomm_type] != NULL);

    if (MPIDI_CH4_Global.comm_req_lists[comm_idx].comm[subcomm_type]) {
        MPIR_Assert(MPIDI_CH4_Global.comm_req_lists[comm_idx].comm[is_localcomm][subcomm_type]->dev.
                    ch4.ch4u.posted_list == NULL);
        MPIR_Assert(MPIDI_CH4_Global.comm_req_lists[comm_idx].comm[is_localcomm][subcomm_type]->dev.
                    ch4.ch4u.unexp_list == NULL);
    }
    MPIDI_CH4_Global.comm_req_lists[comm_idx].comm[is_localcomm][subcomm_type] = NULL;

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_CH4U_DESTROY_COMM);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH4U_mpi_alloc_mem
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX void *MPIDI_CH4U_mpi_alloc_mem(size_t size, MPIR_Info * info_ptr)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_CH4U_MPI_ALLOC_MEM);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_CH4U_MPI_ALLOC_MEM);
    void *p;
    p = MPL_malloc(size);
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_CH4U_MPI_ALLOC_MEM);
    return p;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH4U_mpi_free_mem
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPIDI_CH4U_mpi_free_mem(void *ptr)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_CH4U_MPI_FREE_MEM);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_CH4U_MPI_FREE_MEM);
    MPL_free(ptr);
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_CH4U_MPI_FREE_MEM);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDIU_update_node_map
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDIU_update_node_map(int avtid, int size, MPID_Node_id_t node_map[])
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
