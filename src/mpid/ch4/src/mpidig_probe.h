/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef MPIDIG_PROBE_H_INCLUDED
#define MPIDIG_PROBE_H_INCLUDED

#include "ch4_impl.h"

MPL_STATIC_INLINE_PREFIX int MPIDIG_mpi_iprobe(int source, int tag, MPIR_Comm * comm,
                                               int context_offset, int vci, int *flag,
                                               bool is_local, MPI_Status * status)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Request *unexp_req;
    MPIR_FUNC_ENTER;

    MPIR_Context_id_t context_id = (MPIR_Context_id_t) (comm->recvcontext_id + context_offset);

    unexp_req =
        MPIDIG_rreq_find(source, tag, context_id, &MPIDI_global.per_vci[vci].unexp_list,
                         is_local, MPIDIG_PT2PT_UNEXP);

    if (unexp_req) {
        *flag = 1;
        /* MPI_SOURCE and MPI_TAG should be already set */
        unexp_req->status.MPI_ERROR = MPI_SUCCESS;
        MPIR_STATUS_SET_COUNT(unexp_req->status, MPIDIG_REQUEST(unexp_req, count));

        MPIR_Request_extract_status(unexp_req, status);
    } else {
        *flag = 0;
    }

    MPIR_FUNC_EXIT;
    return mpi_errno;
}

MPL_STATIC_INLINE_PREFIX int MPIDIG_mpi_improbe(int source, int tag, MPIR_Comm * comm,
                                                int context_offset, int vci, int *flag,
                                                bool is_local,
                                                MPIR_Request ** message, MPI_Status * status)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Request *unexp_req;

    MPIR_FUNC_ENTER;

    MPIR_Context_id_t context_id = (MPIR_Context_id_t) (comm->recvcontext_id + context_offset);

    unexp_req =
        MPIDIG_rreq_dequeue(source, tag, context_id, &MPIDI_global.per_vci[vci].unexp_list,
                            is_local, MPIDIG_PT2PT_UNEXP);

    if (unexp_req) {
        MPII_UNEXPQ_FORGET(unexp_req);
        *flag = 1;
        *message = unexp_req;

        (*message)->kind = MPIR_REQUEST_KIND__MPROBE;
        (*message)->comm = comm;
        MPIR_Comm_add_ref(comm);

        /* MPI_SOURCE and MPI_TAG should be already set */
        unexp_req->status.MPI_ERROR = MPI_SUCCESS;
        MPIR_STATUS_SET_COUNT(unexp_req->status, MPIDIG_REQUEST(unexp_req, count));
        MPIDIG_REQUEST(unexp_req, req->status) |= MPIDIG_REQ_UNEXP_DQUED;

        MPIR_Request_extract_status(unexp_req, status);
    } else {
        *flag = 0;
    }

    MPIR_FUNC_EXIT;
    return mpi_errno;
}

#endif /* MPIDIG_PROBE_H_INCLUDED */
