/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpidimpl.h"
#include "ofi_am_events.h"

int MPIDI_OFI_am_rdma_read_ack_handler(void *am_hdr, void *data,
                                       MPI_Aint in_data_sz, uint32_t attr, MPIR_Request ** req)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Request *sreq;
    MPIDI_OFI_am_rdma_read_ack_msg_t *ack_msg;
    int src_handler_id;

    MPIR_FUNC_ENTER;

    ack_msg = (MPIDI_OFI_am_rdma_read_ack_msg_t *) am_hdr;
    sreq = ack_msg->sreq_ptr;

    int vni_local = MPIDIG_AM_ATTR_DST_VCI(attr);
    if (!MPIDI_OFI_ENABLE_MR_PROV_KEY) {
        uint64_t mr_key = fi_mr_key(MPIDI_OFI_AM_SREQ_HDR(sreq, lmt_mr));
        MPIDI_OFI_mr_key_free(MPIDI_OFI_LOCAL_MR_KEY, mr_key);
    }
    MPIDI_OFI_CALL(fi_close(&MPIDI_OFI_AM_SREQ_HDR(sreq, lmt_mr)->fid), mr_unreg);
    MPIDI_OFI_global.per_vni[vni_local].am_inflight_rma_send_mrs -= 1;

    MPL_free(MPIDI_OFI_AM_SREQ_HDR(sreq, pack_buffer));

    /* retrieve the handler_id of the original send request for origin cb. Note the handler_id
     * parameter is MPIDI_OFI_AM_RDMA_READ_ACK and should never be called with origin_cbs */
    src_handler_id = MPIDI_OFI_AM_SREQ_HDR(sreq, msg_hdr).handler_id;
    mpi_errno = MPIDIG_global.origin_cbs[src_handler_id] (sreq);
    MPIR_ERR_CHECK(mpi_errno);
    MPID_Request_complete(sreq);

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIDI_OFI_am_rdma_read_recv_cb(MPIR_Request * rreq)
{
    MPIDI_OFI_lmt_msg_payload_t *lmt_info = (void *) MPIDI_OFI_AM_RREQ_HDR(rreq, am_hdr_buf);
    return do_long_am_recv(lmt_info->reg_sz, rreq, lmt_info);
}
