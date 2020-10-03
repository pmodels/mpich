/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef CH4R_PROBE_H_INCLUDED
#define CH4R_PROBE_H_INCLUDED

#include "ch4_impl.h"

MPL_STATIC_INLINE_PREFIX int MPIDIG_mpi_iprobe(int source, int tag, MPIR_Comm * comm,
                                               int context_offset, int *flag, MPI_Status * status)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Comm *root_comm;
    MPIR_Request *unexp_req;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIG_MPI_IPROBE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIG_MPI_IPROBE);
    MPID_THREAD_CS_ENTER(VCI, MPIDI_VCI(0).lock);

    MPIR_Context_id_t context_id = comm->recvcontext_id + context_offset;
    root_comm = MPIDIG_context_id_to_comm(context_id);

    /* MPIDI_CS_ENTER(); */
    unexp_req = MPIDIG_find_unexp(source, tag, context_id, &MPIDIG_COMM(root_comm, unexp_list));

    if (unexp_req) {
        *flag = 1;
        unexp_req->status.MPI_ERROR = MPI_SUCCESS;
        unexp_req->status.MPI_SOURCE = MPIDIG_REQUEST(unexp_req, rank);
        unexp_req->status.MPI_TAG = MPIDIG_REQUEST(unexp_req, tag);
        MPIR_STATUS_SET_COUNT(unexp_req->status, MPIDIG_REQUEST(unexp_req, count));

        MPIR_Request_extract_status(unexp_req, status);
    } else {
        *flag = 0;
    }
    /* MPIDI_CS_EXIT(); */

    MPID_THREAD_CS_EXIT(VCI, MPIDI_VCI(0).lock);
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIG_MPI_IPROBE);
    return mpi_errno;
}

MPL_STATIC_INLINE_PREFIX int MPIDIG_mpi_improbe(int source, int tag, MPIR_Comm * comm,
                                                int context_offset, int *flag,
                                                MPIR_Request ** message, MPI_Status * status)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Comm *root_comm;
    MPIR_Request *unexp_req;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIG_MPI_IMPROBE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIG_MPI_IMPROBE);
    MPID_THREAD_CS_ENTER(VCI, MPIDI_VCI(0).lock);

    MPIR_Context_id_t context_id = comm->recvcontext_id + context_offset;
    root_comm = MPIDIG_context_id_to_comm(context_id);

    /* MPIDI_CS_ENTER(); */
    unexp_req = MPIDIG_dequeue_unexp(source, tag, context_id, &MPIDIG_COMM(root_comm, unexp_list));

    if (unexp_req) {
        *flag = 1;
        *message = unexp_req;

        (*message)->kind = MPIR_REQUEST_KIND__MPROBE;
        (*message)->comm = comm;
        /* Notes on refcounting comm:
         * We intentionally do nothing here because what we are supposed to do here
         * is -1 for dequeue(unexp_list) and +1 for (*message)->comm */

        unexp_req->status.MPI_ERROR = MPI_SUCCESS;
        unexp_req->status.MPI_SOURCE = MPIDIG_REQUEST(unexp_req, rank);
        unexp_req->status.MPI_TAG = MPIDIG_REQUEST(unexp_req, tag);
        MPIR_STATUS_SET_COUNT(unexp_req->status, MPIDIG_REQUEST(unexp_req, count));
        MPIDIG_REQUEST(unexp_req, req->status) |= MPIDIG_REQ_UNEXP_DQUED;

        MPIR_Request_extract_status(unexp_req, status);
    } else {
        *flag = 0;
    }
    /* MPIDI_CS_EXIT(); */

    MPID_THREAD_CS_EXIT(VCI, MPIDI_VCI(0).lock);
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIG_MPI_IMPROBE);
    return mpi_errno;
}

#endif /* CH4R_PROBE_H_INCLUDED */
