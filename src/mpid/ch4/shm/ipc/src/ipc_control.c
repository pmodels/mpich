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
    mpi_errno = MPID_Request_complete(sreq);
    MPIR_ERR_CHECK(mpi_errno);

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

    MPI_Aint src_data_sz = MPIDIG_recv_in_data_sz(rreq);
    MPIDI_IPC_hdr *ipc_hdr = MPIDIG_REQUEST(rreq, rndv_hdr);
    uintptr_t data_sz, recv_data_sz;
    bool is_async_copy = false;

    MPIDI_Datatype_check_size(MPIDIG_REQUEST(rreq, datatype), MPIDIG_REQUEST(rreq, count), data_sz);

    /* Data truncation checking */
    recv_data_sz = MPL_MIN(src_data_sz, data_sz);
    if (src_data_sz > data_sz)
        rreq->status.MPI_ERROR = MPI_ERR_TRUNCATE;

    /* Set receive status. NOTE: MPI_SOURCE/TAG already set at time of matching */
    MPIR_STATUS_SET_COUNT(rreq->status, recv_data_sz);

    MPIDIG_REQUEST(rreq, req->rreq.u.ipc.src_dt_ptr) = NULL;

    /* attach remote buffer */
    switch (ipc_hdr->ipc_type) {
#ifdef MPIDI_CH4_SHM_ENABLE_XPMEM
        case MPIDI_IPCI_TYPE__XPMEM:
            {
                void *src_buf = NULL;
                /* map */
                mpi_errno = MPIDI_XPMEM_ipc_handle_map(ipc_hdr->ipc_handle.xpmem, &src_buf);
                MPIR_ERR_CHECK(mpi_errno);
                /* copy */
                mpi_errno = MPIDI_IPCI_copy_data(ipc_hdr, rreq, src_buf, src_data_sz);
                MPIR_ERR_CHECK(mpi_errno);
                /* skip unmap */
            }
            break;
#endif
#ifdef MPIDI_CH4_SHM_ENABLE_CMA
        case MPIDI_IPCI_TYPE__CMA:
            mpi_errno = MPIDI_CMA_copy_data(ipc_hdr, rreq, src_data_sz);
            MPIR_ERR_CHECK(mpi_errno);
            break;
#endif
#ifdef MPIDI_CH4_SHM_ENABLE_GPU
        case MPIDI_IPCI_TYPE__GPU:
            is_async_copy = true;
            mpi_errno = MPIDI_GPU_copy_data_async(ipc_hdr, rreq, src_data_sz);
            MPIR_ERR_CHECK(mpi_errno);
            break;
#endif
        case MPIDI_IPCI_TYPE__NONE:
            break;
        default:
            MPIR_Assert(0);
    }

    IPC_TRACE("handle_lmt_recv: handle matched rreq %p [source %d, tag %d, "
              " context_id 0x%x], copy dst %p, bytes %ld\n", rreq,
              rreq->status.MPI_SOURCE, rreq->status.MPI_TAG,
              MPIDIG_REQUEST(rreq, u.recv.context_id), (char *) MPIDIG_REQUEST(rreq, buffer),
              recv_data_sz);

  fn_exit:
    if (!is_async_copy) {
        mpi_errno = MPIDI_IPC_complete(rreq, ipc_hdr->ipc_type);
    }
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    /* Need to send ack and complete the request so we don't block sender and receiver */
    if (!rreq->status.MPI_ERROR) {
        is_async_copy = false;
        rreq->status.MPI_ERROR = mpi_errno;
    }
    goto fn_exit;
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
