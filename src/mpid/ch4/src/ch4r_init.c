/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2019 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 *  Portions of this code were written by Intel Corporation.
 *  Copyright (C) 2011-2016 Intel Corporation.  Intel provides this material
 *  to Argonne National Laboratory subject to Software Grant and Corporate
 *  Contributor License Agreement dated February 8, 2012.
 */

#include "mpidimpl.h"
#include "ch4r_init.h"

int MPIDIG_init_comm(MPIR_Comm * comm)
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
    if (unlikely(MPIDI_global.is_ch4u_initialized &&
                 (comm == MPIR_Process.comm_world || comm == MPIR_Process.comm_self)))
        goto fn_exit;
    if (MPIR_CONTEXT_READ_FIELD(DYNAMIC_PROC, comm->recvcontext_id))
        goto fn_exit;

    comm_idx = MPIDIG_get_context_index(comm->recvcontext_id);
    subcomm_type = MPIR_CONTEXT_READ_FIELD(SUBCOMM, comm->recvcontext_id);
    is_localcomm = MPIR_CONTEXT_READ_FIELD(IS_LOCALCOMM, comm->recvcontext_id);

    MPIR_Assert(subcomm_type <= 3);
    MPIR_Assert(is_localcomm <= 1);
    MPIDI_global.comm_req_lists[comm_idx].comm[is_localcomm][subcomm_type] = comm;
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

    MPIDIG_COMM(comm, window_instance) = 0;
  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIG_INIT_COMM);
    return mpi_errno;
}

int MPIDIG_destroy_comm(MPIR_Comm * comm)
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
    MPIR_Assert(MPIDI_global.comm_req_lists[comm_idx].comm[is_localcomm][subcomm_type] != NULL);

    if (MPIDI_global.comm_req_lists[comm_idx].comm[is_localcomm][subcomm_type]) {
        MPIR_Assert(MPIDIG_COMM
                    (MPIDI_global.comm_req_lists[comm_idx].comm[is_localcomm][subcomm_type],
                     posted_list) == NULL);
        MPIR_Assert(MPIDIG_COMM
                    (MPIDI_global.comm_req_lists[comm_idx].comm[is_localcomm][subcomm_type],
                     unexp_list) == NULL);
    }
    MPIDI_global.comm_req_lists[comm_idx].comm[is_localcomm][subcomm_type] = NULL;

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIG_DESTROY_COMM);
    return mpi_errno;
}

void *MPIDIG_mpi_alloc_mem(size_t size, MPIR_Info * info_ptr)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIG_MPI_ALLOC_MEM);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIG_MPI_ALLOC_MEM);
    void *p;
    p = MPL_malloc(size, MPL_MEM_USER);
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIG_MPI_ALLOC_MEM);
    return p;
}

int MPIDIG_mpi_free_mem(void *ptr)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIG_MPI_FREE_MEM);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIG_MPI_FREE_MEM);
    MPL_free(ptr);
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIG_MPI_FREE_MEM);
    return mpi_errno;
}
