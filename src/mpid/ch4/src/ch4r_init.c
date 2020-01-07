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

    MPIR_Assert(MPIDI_global.is_ch4u_initialized);

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
    MPID_THREAD_CS_EXIT(VCI, MPIDIU_THREAD_MPIDIG_GLOBAL_MUTEX);

    MPIDIG_COMM(comm, window_instance) = 0;
  fn_exit:
    return mpi_errno;
}

int MPIDIG_destroy_comm(MPIR_Comm * comm)
{
    int mpi_errno = MPI_SUCCESS, comm_idx, subcomm_type, is_localcomm;

    if (MPIR_CONTEXT_READ_FIELD(DYNAMIC_PROC, comm->recvcontext_id))
        goto fn_exit;
    comm_idx = MPIDIG_get_context_index(comm->recvcontext_id);
    subcomm_type = MPIR_CONTEXT_READ_FIELD(SUBCOMM, comm->recvcontext_id);
    is_localcomm = MPIR_CONTEXT_READ_FIELD(IS_LOCALCOMM, comm->recvcontext_id);

    MPIR_Assert(subcomm_type <= 3);
    MPIR_Assert(is_localcomm <= 1);

    MPID_THREAD_CS_ENTER(VCI, MPIDIU_THREAD_MPIDIG_GLOBAL_MUTEX);
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
    MPID_THREAD_CS_EXIT(VCI, MPIDIU_THREAD_MPIDIG_GLOBAL_MUTEX);

  fn_exit:
    return mpi_errno;
}

void *MPIDIG_mpi_alloc_mem(size_t size, MPIR_Info * info_ptr)
{
    void *p;
    p = MPL_malloc(size, MPL_MEM_USER);

    return p;
}

int MPIDIG_mpi_free_mem(void *ptr)
{
    int mpi_errno = MPI_SUCCESS;

    MPL_free(ptr);

    return mpi_errno;
}
