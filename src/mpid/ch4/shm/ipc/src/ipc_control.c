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

#ifdef MPIDI_CH4_SHM_ENABLE_GPU
    /* cleanup any GPU resources created as part of the IPC */
    if (MPIDI_SHM_REQUEST(sreq, ipc.ipc_type) == MPIDI_IPCI_TYPE__GPU) {
        mpi_errno = MPIDI_GPU_send_complete(sreq);
        MPIR_ERR_CHECK(mpi_errno);
    }
#endif

    MPIR_Datatype_release_if_not_builtin(MPIDIG_REQUEST(sreq, datatype));
    MPID_Request_complete(sreq);

    if (attr & MPIDIG_AM_ATTR__IS_ASYNC) {
        *req = NULL;
    }

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
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

int MPIDI_IPC_complete(MPIR_Request * rreq, MPIDI_IPCI_type_t ipc_type)
{
    int mpi_errno = MPI_SUCCESS;

    MPIDI_IPC_ack_t am_hdr;
    am_hdr.ipc_type = ipc_type;
    am_hdr.req_ptr = MPIDIG_REQUEST(rreq, req->rreq.peer_req_ptr);

    int local_vci = MPIDIG_REQUEST(rreq, req->local_vci);
    int remote_vci = MPIDIG_REQUEST(rreq, req->remote_vci);
    CH4_CALL(am_send_hdr(rreq->status.MPI_SOURCE, rreq->comm, MPIDI_IPC_ACK,
                         &am_hdr, sizeof(am_hdr), local_vci, remote_vci), 1, mpi_errno);
    MPIR_ERR_CHECK(mpi_errno);

    if (MPIDIG_REQUEST(rreq, req->rreq.u.ipc.src_dt_ptr)) {
        MPIR_Datatype_free(MPIDIG_REQUEST(rreq, req->rreq.u.ipc.src_dt_ptr));
    }

    MPIDIG_REQUEST(rreq, req->target_cmpl_cb) (rreq);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
