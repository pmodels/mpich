/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpidimpl.h"
#include "ipc_pre.h"
#include "ipc_types.h"

int MPIDI_IPC_ack_target_msg_cb(void *am_hdr, void *data, MPI_Aint in_data_sz,
                                uint32_t attr, MPIR_Request ** req)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_IPC_ack_t *hdr = am_hdr;
    MPIR_Request *sreq = hdr->req_ptr;

    MPIR_FUNC_ENTER;

    MPIR_Datatype_release_if_not_builtin(MPIDIG_REQUEST(sreq, datatype));
    MPID_Request_complete(sreq);

    if (attr & MPIDIG_AM_ATTR__IS_ASYNC) {
        *req = NULL;
    }

    MPIR_FUNC_EXIT;
    return mpi_errno;
}

int MPIDI_IPC_rndv_cb(MPIR_Request * rreq)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_ENTER;

    MPI_Aint in_data_sz = MPIDIG_recv_in_data_sz(rreq);
    MPIR_Request *sreq_ptr = MPIDIG_REQUEST(rreq, req->rreq.peer_req_ptr);

    mpi_errno = MPIDI_IPCI_handle_lmt_recv(MPIDIG_REQUEST(rreq, rndv_hdr),
                                           in_data_sz, sreq_ptr, rreq);

    MPIR_FUNC_EXIT;
    return mpi_errno;
}
