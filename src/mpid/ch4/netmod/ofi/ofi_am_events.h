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
#ifndef OFI_AM_EVENTS_H_INCLUDED
#define OFI_AM_EVENTS_H_INCLUDED

#include "ofi_am_impl.h"

#undef FUNCNAME
#define FUNCNAME MPIDI_OFI_handle_short_am
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_OFI_handle_short_am(MPIDI_OFI_am_header_t * msg_hdr)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Request *rreq = NULL;
    void *p_data;
    void *in_data;

    size_t data_sz, in_data_sz;
    MPIDIG_am_target_cmpl_cb target_cmpl_cb = NULL;
    struct iovec *iov;
    int i, is_contig, iov_len;
    size_t done, curr_len, rem;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_OFI_HANDLE_SHORT_AM);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_OFI_HANDLE_SHORT_AM);

    p_data = in_data = (char *) msg_hdr->payload + msg_hdr->am_hdr_sz;
    in_data_sz = data_sz = msg_hdr->data_sz;

    MPIDIG_global.target_msg_cbs[msg_hdr->handler_id] (msg_hdr->handler_id, msg_hdr->payload,
                                                       &p_data, &data_sz,
                                                       &is_contig, &target_cmpl_cb, &rreq);

    if (!rreq)
        goto fn_exit;

    if (!p_data || !data_sz) {
        if (target_cmpl_cb) {
            MPIR_STATUS_SET_COUNT(rreq->status, data_sz);
            target_cmpl_cb(rreq);
        }
        goto fn_exit;
    }

    if (is_contig) {
        if (in_data_sz > data_sz) {
            rreq->status.MPI_ERROR = MPI_ERR_TRUNCATE;
        }
        else {
            rreq->status.MPI_ERROR = MPI_SUCCESS;
        }

        data_sz = MPL_MIN(data_sz, in_data_sz);
        MPIR_Memcpy(p_data, in_data, data_sz);
        MPIR_STATUS_SET_COUNT(rreq->status, data_sz);
    }
    else {
        done = 0;
        rem = in_data_sz;
        iov = (struct iovec *) p_data;
        iov_len = data_sz;

        for (i = 0; i < iov_len && rem > 0; i++) {
            curr_len = MPL_MIN(rem, iov[i].iov_len);
            MPIR_Memcpy(iov[i].iov_base, (char *) in_data + done, curr_len);
            rem -= curr_len;
            done += curr_len;
        }

        if (rem) {
            rreq->status.MPI_ERROR = MPI_ERR_TRUNCATE;
        }
        else {
            rreq->status.MPI_ERROR = MPI_SUCCESS;
        }

        MPIR_STATUS_SET_COUNT(rreq->status, done);
    }

    if (target_cmpl_cb) {
        target_cmpl_cb(rreq);
    }

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_OFI_HANDLE_SHORT_AM);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_OFI_handle_short_am_hdr
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_OFI_handle_short_am_hdr(MPIDI_OFI_am_header_t * msg_hdr, void *am_hdr)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Request *rreq = NULL;
    MPIDIG_am_target_cmpl_cb target_cmpl_cb = NULL;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_OFI_HANDLE_SHORT_AM_HDR);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_OFI_HANDLE_SHORT_AM_HDR);

    MPIDIG_global.target_msg_cbs[msg_hdr->handler_id] (msg_hdr->handler_id, am_hdr,
                                                       NULL, NULL, NULL, &target_cmpl_cb, &rreq);

    if (!rreq)
        goto fn_exit;

    if (target_cmpl_cb) {
        target_cmpl_cb(rreq);
    }

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_OFI_HANDLE_SHORT_AM_HDR);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_OFI_do_rdma_read
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_OFI_do_rdma_read(void *dst,
                                         uint64_t src,
                                         size_t data_sz,
                                         MPIR_Context_id_t context_id,
                                         int src_rank, MPIR_Request * rreq)
{
    int mpi_errno = MPI_SUCCESS;
    size_t done = 0, curr_len, rem = 0;
    MPIDI_OFI_am_request_t *am_req;
    MPIR_Comm *comm;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_OFI_DO_RDMA_READ);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_OFI_DO_RDMA_READ);

    rem = data_sz;

    while (done != data_sz) {
        curr_len = MPL_MIN(rem, MPIDI_Global.max_send);

        MPIR_Assert(sizeof(MPIDI_OFI_am_request_t) <= MPIDI_OFI_BUF_POOL_SIZE);
        am_req = (MPIDI_OFI_am_request_t *) MPIDI_CH4R_get_buf(MPIDI_Global.am_buf_pool);
        MPIR_Assert(am_req);

        am_req->req_hdr = MPIDI_OFI_AMREQUEST(rreq, req_hdr);
        am_req->event_id = MPIDI_OFI_EVENT_AM_READ;
        comm = MPIDI_CH4U_context_id_to_comm(context_id);
        MPIR_Assert(comm);
        MPIDI_OFI_cntr_incr();

        struct iovec iov = {
            .iov_base = (char *)dst + done,
            .iov_len = curr_len
        };
        struct fi_rma_iov rma_iov = {
            .addr = src + done,
            .len = curr_len,
            .key = MPIDI_OFI_AMREQUEST_HDR(rreq, lmt_info).rma_key
        };
        struct fi_msg_rma msg = {
            .msg_iov = &iov,
            .desc = NULL,
            .iov_count = 1,
            .addr = MPIDI_OFI_comm_to_phys(comm, src_rank),
            .rma_iov = &rma_iov,
            .rma_iov_count = 1,
            .context = &am_req->context,
            .data = 0
        };

        MPIDI_OFI_CALL_RETRY_AM(fi_readmsg(MPIDI_Global.ctx[0].tx,
                                           &msg,
                                           FI_COMPLETION), FALSE /* no lock */ , read);

        done += curr_len;
        rem -= curr_len;
    }

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_OFI_DO_RDMA_READ);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_OFI_handle_long_am_hdr
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_OFI_handle_long_am_hdr(MPIDI_OFI_am_header_t * msg_hdr)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_OFI_lmt_msg_payload_t *lmt_msg;
    MPIR_Request *rreq;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_OFI_HANDLE_LONG_AM_HDR);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_OFI_HANDLE_LONG_AM_HDR);

    rreq = MPIR_Request_create(MPIR_REQUEST_KIND__RECV);

    mpi_errno = MPIDI_OFI_am_init_request(NULL, 0, rreq);

    if (mpi_errno != MPI_SUCCESS)
        MPIR_ERR_POP(mpi_errno);

    lmt_msg = (MPIDI_OFI_lmt_msg_payload_t *) msg_hdr->payload;
    MPIDI_OFI_AMREQUEST_HDR(rreq, lmt_info) = *lmt_msg;
    MPIDI_OFI_AMREQUEST_HDR(rreq, msg_hdr) = *msg_hdr;
    MPIDI_OFI_AMREQUEST_HDR(rreq, rreq_ptr) = (void *) rreq;
    MPIDI_OFI_AMREQUEST_HDR(rreq, am_hdr) = MPL_malloc(msg_hdr->am_hdr_sz);
    MPIDI_OFI_AMREQUEST_HDR(rreq, lmt_cntr) =
        ((msg_hdr->am_hdr_sz - 1) / MPIDI_Global.max_send) + 1;
    MPIDI_OFI_do_rdma_read(MPIDI_OFI_AMREQUEST_HDR(rreq, am_hdr), lmt_msg->am_hdr_src,
                           msg_hdr->am_hdr_sz, lmt_msg->context_id, lmt_msg->src_rank, rreq);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_OFI_HANDLE_LONG_AM_HDR);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_OFI_do_handle_long_am
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_OFI_do_handle_long_am(MPIDI_OFI_am_header_t * msg_hdr,
                                              MPIDI_OFI_lmt_msg_payload_t * lmt_msg, void *am_hdr)
{
    int num_reads, i, iov_len, c, mpi_errno = MPI_SUCCESS, is_contig = 0;
    MPIR_Request *rreq = NULL;
    void *p_data;
    size_t data_sz, rem, done, curr_len, in_data_sz;
    MPIDIG_am_target_cmpl_cb target_cmpl_cb = NULL;
    struct iovec *iov;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_OFI_DO_HANDLE_LONG_AM);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_OFI_DO_HANDLE_LONG_AM);

    in_data_sz = data_sz = msg_hdr->data_sz;
    MPIDIG_global.target_msg_cbs[msg_hdr->handler_id] (msg_hdr->handler_id, am_hdr,
                                                       &p_data, &data_sz, &is_contig,
                                                       &target_cmpl_cb, &rreq);

    if (!rreq)
        goto fn_exit;

    MPIDI_OFI_AMREQUEST(rreq, req_hdr) = NULL;
    mpi_errno = MPIDI_OFI_am_init_request(NULL, 0, rreq);

    if (mpi_errno != MPI_SUCCESS)
        MPIR_ERR_POP(mpi_errno);

    MPIR_cc_incr(rreq->cc_ptr, &c);

    MPIDI_OFI_AMREQUEST_HDR(rreq, target_cmpl_cb) = target_cmpl_cb;

    if ((!p_data || !data_sz) && target_cmpl_cb) {
        target_cmpl_cb(rreq);
        MPID_Request_complete(rreq);   /* FIXME: Should not call MPIDI in NM ? */
        goto fn_exit;
    }

    MPIDI_OFI_AMREQUEST_HDR(rreq, msg_hdr) = *msg_hdr;
    MPIDI_OFI_AMREQUEST_HDR(rreq, lmt_info) = *lmt_msg;
    MPIDI_OFI_AMREQUEST_HDR(rreq, rreq_ptr) = (void *) rreq;

    if (is_contig) {
        if (in_data_sz > data_sz) {
            rreq->status.MPI_ERROR = MPI_ERR_TRUNCATE;
        }
        else {
            rreq->status.MPI_ERROR = MPI_SUCCESS;
        }

        data_sz = MPL_MIN(data_sz, in_data_sz);
        MPIDI_OFI_AMREQUEST_HDR(rreq, lmt_cntr) = ((data_sz - 1) / MPIDI_Global.max_send) + 1;
        MPIDI_OFI_do_rdma_read(p_data,
                               lmt_msg->src_offset,
                               data_sz, lmt_msg->context_id, lmt_msg->src_rank, rreq);
        MPIR_STATUS_SET_COUNT(rreq->status, data_sz);
    }
    else {
        done = 0;
        rem = in_data_sz;
        iov = (struct iovec *) p_data;
        iov_len = data_sz;

        /* FIXME: optimize iov processing part */

        /* set lmt counter */
        MPIDI_OFI_AMREQUEST_HDR(rreq, lmt_cntr) = 0;

        for (i = 0; i < iov_len && rem > 0; i++) {
            curr_len = MPL_MIN(rem, iov[i].iov_len);
            num_reads = ((curr_len - 1) / MPIDI_Global.max_send) + 1;
            MPIDI_OFI_AMREQUEST_HDR(rreq, lmt_cntr) += num_reads;
            rem -= curr_len;
        }

        done = 0;
        rem = in_data_sz;

        for (i = 0; i < iov_len && rem > 0; i++) {
            curr_len = MPL_MIN(rem, iov[i].iov_len);
            MPIDI_OFI_do_rdma_read(iov[i].iov_base, lmt_msg->src_offset + done,
                                   curr_len, lmt_msg->context_id, lmt_msg->src_rank, rreq);
            rem -= curr_len;
            done += curr_len;
        }

        if (rem) {
            rreq->status.MPI_ERROR = MPI_ERR_TRUNCATE;
        }
        else {
            rreq->status.MPI_ERROR = MPI_SUCCESS;
        }

        MPIR_STATUS_SET_COUNT(rreq->status, done);
    }

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_OFI_DO_HANDLE_LONG_AM);
    return mpi_errno;

  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_OFI_handle_long_am
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_OFI_handle_long_am(MPIDI_OFI_am_header_t * msg_hdr)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_OFI_lmt_msg_payload_t *lmt_msg;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_OFI_HANDLE_LONG_AM);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_OFI_HANDLE_LONG_AM);

    lmt_msg = (MPIDI_OFI_lmt_msg_payload_t *) ((char *) msg_hdr->payload + msg_hdr->am_hdr_sz);
    mpi_errno = MPIDI_OFI_do_handle_long_am(msg_hdr, lmt_msg, msg_hdr->payload);

    if (mpi_errno != MPI_SUCCESS)
        MPIR_ERR_POP(mpi_errno);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_OFI_HANDLE_LONG_AM);
    return mpi_errno;

  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_OFI_handle_lmt_ack
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_OFI_handle_lmt_ack(MPIDI_OFI_am_header_t * msg_hdr)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Request *sreq;
    MPIDI_OFI_ack_msg_payload_t *ack_msg;
    int handler_id;
    uint64_t index;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_OFI_HANDLE_LMT_ACK);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_OFI_HANDLE_LMT_ACK);

    ack_msg = (MPIDI_OFI_ack_msg_payload_t *) msg_hdr->payload;
    sreq = (MPIR_Request *) ack_msg->sreq_ptr;

    index = fi_mr_key(MPIDI_OFI_AMREQUEST_HDR(sreq, lmt_mr)) >> MPIDI_Global.huge_rma_shift;
    MPIDI_OFI_index_allocator_free(MPIDI_OFI_COMM(MPIR_Process.comm_world).rma_id_allocator, index);
    MPIDI_OFI_CALL_NOLOCK(fi_close(&MPIDI_OFI_AMREQUEST_HDR(sreq, lmt_mr)->fid), mr_unreg);
    OPA_decr_int(&MPIDI_Global.am_inflight_rma_send_mrs);

    if (MPIDI_OFI_AMREQUEST_HDR(sreq, pack_buffer)) {
        MPL_free(MPIDI_OFI_AMREQUEST_HDR(sreq, pack_buffer));
    }

    handler_id = MPIDI_OFI_AMREQUEST_HDR(sreq, msg_hdr).handler_id;
    MPID_Request_complete(sreq);       /* FIXME: Should not call MPIDI in NM ? */
    mpi_errno = MPIDIG_global.origin_cbs[handler_id] (sreq);

    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_OFI_HANDLE_LMT_ACK);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_OFI_dispatch_ack
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_OFI_dispatch_ack(int rank,
                                         int context_id,
                                         uint64_t sreq_ptr, int am_type)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_OFI_ack_msg_t msg;
    MPIR_Comm *comm;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_OFI_DISPATCH_ACK);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_OFI_DISPATCH_ACK);

    comm = MPIDI_CH4U_context_id_to_comm(context_id);

    msg.hdr.am_hdr_sz = sizeof(msg.pyld);
    msg.hdr.data_sz = 0;
    msg.hdr.am_type = am_type;
    msg.pyld.sreq_ptr = sreq_ptr;
    MPIDI_OFI_CALL_RETRY_AM(fi_inject(MPIDI_Global.ctx[0].tx, &msg, sizeof(msg),
                                      MPIDI_OFI_comm_to_phys(comm, rank)),
                            FALSE /* no lock */ , inject);
  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_OFI_DISPATCH_ACK);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}


#endif /* OFI_AM_EVENTS_H_INCLUDED */
