/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef OFI_AM_EVENTS_H_INCLUDED
#define OFI_AM_EVENTS_H_INCLUDED

#include "ofi_am_impl.h"
#include "mpidu_genq.h"

int MPIDI_OFI_am_rdma_read_ack_handler(void *am_hdr, void *data,
                                       MPI_Aint in_data_sz, uint32_t attr, MPIR_Request ** req);
int MPIDI_OFI_am_rdma_read_recv_cb(MPIR_Request * rreq);
int MPIDI_OFI_am_lmt_unpack_event(MPIR_Request * rreq);

MPL_STATIC_INLINE_PREFIX int MPIDI_OFI_handle_short_am(MPIDI_OFI_am_header_t * msg_hdr,
                                                       void *am_hdr, void *p_data)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_ENTER;

    int attr = 0;               /* is_local = 0, is_async = 0 */
    MPIDIG_global.target_msg_cbs[msg_hdr->handler_id] (am_hdr,
                                                       p_data, msg_hdr->payload_sz, attr, NULL);

    MPIR_FUNC_EXIT;
    return mpi_errno;
}

/* this is called in am_recv_event in ofi_event.c on receiver side */
MPL_STATIC_INLINE_PREFIX int MPIDI_OFI_handle_pipeline(MPIDI_OFI_am_header_t * msg_hdr,
                                                       void *am_hdr, void *p_data)
{
    int mpi_errno = MPI_SUCCESS;
    int is_done = 0;
    MPIR_Request *rreq = NULL;
    MPIR_Request *cache_rreq = NULL;

    MPIR_FUNC_ENTER;

    cache_rreq = MPIDIG_req_cache_lookup(MPIDI_OFI_global.req_map, (uint64_t) msg_hdr->fi_src_addr);
    MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_GENERAL, VERBOSE,
                    (MPL_DBG_FDEST, "cached req %p handle=0x%x", cache_rreq,
                     cache_rreq ? cache_rreq->handle : 0));

    rreq = cache_rreq;

    if (!rreq) {
        int attr = MPIDIG_AM_ATTR__IS_ASYNC;
        MPIDIG_global.target_msg_cbs[msg_hdr->handler_id] (am_hdr, p_data, msg_hdr->payload_sz,
                                                           attr, &rreq);
        MPIDIG_recv_setup(rreq);
        MPIDIG_req_cache_add(MPIDI_OFI_global.req_map, (uint64_t) msg_hdr->fi_src_addr, rreq);
    }

    is_done = MPIDIG_recv_copy_seg(p_data, msg_hdr->payload_sz, rreq);
    if (is_done) {
        MPIDIG_REQUEST(rreq, req->target_cmpl_cb) (rreq);
        MPIDIG_req_cache_remove(MPIDI_OFI_global.req_map, (uint64_t) msg_hdr->fi_src_addr);
    }

    MPIR_FUNC_EXIT;
    return mpi_errno;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_OFI_handle_short_am_hdr(MPIDI_OFI_am_header_t * msg_hdr,
                                                           void *am_hdr)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_ENTER;

    int attr = 0;
    MPIDIG_global.target_msg_cbs[msg_hdr->handler_id] (am_hdr, NULL, 0, attr, NULL);

    MPIR_FUNC_EXIT;
    return mpi_errno;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_OFI_handle_rdma_read(MPIDI_OFI_am_header_t * msg_hdr,
                                                        void *am_hdr,
                                                        MPIDI_OFI_lmt_msg_payload_t * lmt_msg)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Request *rreq = NULL;

    MPIR_FUNC_ENTER;

    int attr = MPIDIG_AM_ATTR__IS_ASYNC | MPIDIG_AM_ATTR__IS_RNDV | MPIDI_OFI_AM_ATTR__RDMA;
    MPIDIG_global.target_msg_cbs[msg_hdr->handler_id] (am_hdr, NULL, 0, attr, &rreq);

    if (!rreq)
        goto fn_exit;

    mpi_errno = MPIDI_OFI_am_init_rdma_read_hdr(rreq);
    MPIR_ERR_CHECK(mpi_errno);

    MPIR_cc_inc(rreq->cc_ptr);

    if (!lmt_msg->reg_sz) {
        MPIDIG_REQUEST(rreq, req->target_cmpl_cb) (rreq);
        MPID_Request_complete(rreq);
        goto fn_exit;
    }

    MPIDI_OFI_AM_RDMA_READ_HDR(rreq, lmt_info) = *lmt_msg;

    /* only proceed with RDMA read recv when the request is initialized for recv. Otherwise, the
     * CH4 will trigger the data copy at a later time through the MPIDI_OFI_am_rdma_read_recv_cb.
     * */
    if (MPIDIG_recv_initialized(rreq)) {
        mpi_errno = MPIDI_OFI_am_rdma_read_recv_cb(rreq);
        MPIR_ERR_CHECK(mpi_errno);
        /* completion in lmt event functions */
    }

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;

  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_OFI_do_am_rdma_read_ack(int rank, MPIR_Comm * comm,
                                                           MPIR_Request * sreq_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_OFI_am_rdma_read_ack_msg_t ack_msg;

    MPIR_FUNC_ENTER;

    ack_msg.sreq_ptr = sreq_ptr;
    mpi_errno =
        MPIDI_NM_am_send_hdr_reply(comm, rank, MPIDI_OFI_AM_RDMA_READ_ACK, &ack_msg,
                                   (MPI_Aint) sizeof(ack_msg));
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#endif /* OFI_AM_EVENTS_H_INCLUDED */
