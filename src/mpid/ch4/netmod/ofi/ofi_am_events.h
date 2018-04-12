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
#define FUNCNAME MPIDI_OFI_dispatch_ack
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_OFI_dispatch_ack(int rank, int context_id, uint64_t sreq_ptr,
                                         uint64_t rreq_ptr, int am_type)
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
    msg.pyld.rreq_ptr = rreq_ptr;
    MPIDI_OFI_CALL_RETRY_AM(fi_inject(MPIDI_Global.ctx[0].tx, &msg, sizeof(msg),
                                      MPIDI_OFI_comm_to_phys(comm, rank)),
                            FALSE /* no lock */ , inject);
  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_OFI_DISPATCH_ACK);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

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
        } else {
            rreq->status.MPI_ERROR = MPI_SUCCESS;
        }

        data_sz = MPL_MIN(data_sz, in_data_sz);
        MPIR_Memcpy(p_data, in_data, data_sz);
        MPIR_STATUS_SET_COUNT(rreq->status, data_sz);
    } else {
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
        } else {
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
        MPIR_STATUS_SET_COUNT(rreq->status, 0);
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
            .iov_base = (char *) dst + done,
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
                                           &msg, FI_COMPLETION), FALSE /* no lock */ , read);

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
        MPID_Request_complete(rreq);    /* FIXME: Should not call MPIDI in NM ? */
        goto fn_exit;
    }

    MPIDI_OFI_AMREQUEST_HDR(rreq, msg_hdr) = *msg_hdr;
    MPIDI_OFI_AMREQUEST_HDR(rreq, lmt_info) = *lmt_msg;
    MPIDI_OFI_AMREQUEST_HDR(rreq, rreq_ptr) = (void *) rreq;

    if (!MPIDI_OFI_ENABLE_RMA) {
        /* save recv buffer info */
        if (is_contig) {
            if (in_data_sz > data_sz) {
                rreq->status.MPI_ERROR = MPI_ERR_TRUNCATE;
            } else {
                rreq->status.MPI_ERROR = MPI_SUCCESS;
            }
            MPIR_STATUS_SET_COUNT(rreq->status, data_sz);
            MPIDI_OFI_AMREQUEST_HDR(rreq, lmt_msg).data = p_data;
            MPIDI_OFI_AMREQUEST_HDR(rreq, data_sz) = MPL_MIN(data_sz, in_data_sz);
            MPIDI_OFI_AMREQUEST_HDR(rreq, recv_contig) = true;
            MPIDI_OFI_AMREQUEST_HDR(rreq, target_cmpl_cb) = target_cmpl_cb;
            MPIDI_OFI_AMREQUEST_HDR(rreq, last) = 0;
        } else {
            size_t pack_sz = 0;
            iov = (struct iovec *) p_data;

            MPIDI_OFI_AMREQUEST_HDR(rreq, lmt_msg).data = p_data;
            MPIDI_OFI_AMREQUEST_HDR(rreq, lmt_msg).data_sz = data_sz;
            MPIDI_OFI_AMREQUEST_HDR(rreq, recv_contig) = false;
            for (i = 0; i < data_sz; i++)
                pack_sz += iov[i].iov_len;
            //printf("pack_sz = %lu\n", pack_sz);
            MPIDI_OFI_AMREQUEST_HDR(rreq, pack_buffer) = MPL_malloc(pack_sz, MPL_MEM_BUFFER);
            MPIDI_OFI_AMREQUEST_HDR(rreq, data_sz) = pack_sz;
            MPIDI_OFI_AMREQUEST_HDR(rreq, last) = 0;
        }

        /* send CTS messages */
        //printf("sending CTS message for sreq %p, rreq %p\n", lmt_msg->sreq_ptr, rreq);
        mpi_errno = MPIDI_OFI_dispatch_ack(lmt_msg->src_rank,
                                           lmt_msg->context_id,
                                           lmt_msg->sreq_ptr, (uint64_t) rreq,
                                           MPIDI_AMTYPE_LMT_ACK);
        if (mpi_errno)
            MPIR_ERR_POP(mpi_errno);
    } else {
        if (is_contig) {
            if (in_data_sz > data_sz) {
                rreq->status.MPI_ERROR = MPI_ERR_TRUNCATE;
            } else {
                rreq->status.MPI_ERROR = MPI_SUCCESS;
            }

            data_sz = MPL_MIN(data_sz, in_data_sz);
            MPIDI_OFI_AMREQUEST_HDR(rreq, lmt_cntr) = ((data_sz - 1) / MPIDI_Global.max_send) + 1;
            MPIDI_OFI_do_rdma_read(p_data,
                                   lmt_msg->src_offset,
                                   data_sz, lmt_msg->context_id, lmt_msg->src_rank, rreq);
            MPIR_STATUS_SET_COUNT(rreq->status, data_sz);
        } else {
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
            } else {
                rreq->status.MPI_ERROR = MPI_SUCCESS;
            }

            MPIR_STATUS_SET_COUNT(rreq->status, done);
        }
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
#define FUNCNAME MPIDI_OFI_handle_lmt_data
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_OFI_handle_lmt_data(MPIDI_OFI_am_header_t * msg_hdr)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Request *rreq;
    MPIDI_OFI_lmt_data_t *data_msg;
    int handler_id;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_OFI_HANDLE_LMT_DATA);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_OFI_HANDLE_LMT_DATA);

    data_msg = &((MPIDI_OFI_lmt_data_msg_t *) msg_hdr)->lmt_data;
    rreq = (MPIR_Request *) data_msg->rreq_ptr;

    //printf("got some data for rreq %p\n", rreq);

    char *buf;
    if (MPIDI_OFI_AMREQUEST_HDR(rreq, recv_contig)) {
        buf =
            (char *) MPIDI_OFI_AMREQUEST_HDR(rreq, lmt_msg).data + MPIDI_OFI_AMREQUEST_HDR(rreq,
                                                                                           last);
    } else {
        buf = MPIDI_OFI_AMREQUEST_HDR(rreq, pack_buffer) + MPIDI_OFI_AMREQUEST_HDR(rreq, last);
    }

    size_t copy_sz = MPL_MIN(MPIDI_OFI_AMREQUEST_HDR(rreq, data_sz),
                             MPIDI_OFI_DEFAULT_SHORT_SEND_SIZE - sizeof(MPIDI_OFI_lmt_data_msg_t));
    memcpy(buf, data_msg->data, copy_sz);
    MPIDI_OFI_AMREQUEST_HDR(rreq, data_sz) -= copy_sz;
    MPIDI_OFI_AMREQUEST_HDR(rreq, last) += copy_sz;

    //printf("copied %lu bytes, data left = %lu\n", copy_sz, MPIDI_OFI_AMREQUEST_HDR(rreq, data_sz));
    if (MPIDI_OFI_AMREQUEST_HDR(rreq, data_sz) == 0) {
        /* unpack the data if non-contig */
        if (!MPIDI_OFI_AMREQUEST_HDR(rreq, recv_contig)) {
            const struct iovec *iov = MPIDI_OFI_AMREQUEST_HDR(rreq, lmt_msg).data;
            size_t iov_len = MPIDI_OFI_AMREQUEST_HDR(rreq, lmt_msg).data_sz;
            size_t rem = MPIDI_OFI_AMREQUEST_HDR(rreq, last);
            size_t done = 0, curr_len;
            int i;

            //printf("unpacking %lu chunks for total of %lu bytes\n", iov_len, rem);

            buf = MPIDI_OFI_AMREQUEST_HDR(rreq, pack_buffer);
            for (i = 0; i < iov_len && rem > 0; i++) {
                curr_len = MPL_MIN(rem, iov[i].iov_len);
                MPIR_Memcpy(iov[i].iov_base, (char *) buf + done, curr_len);
                rem -= curr_len;
                done += curr_len;
                //printf("copied %lu bytes, %lu remaining\n", curr_len, rem);
            }

            if (rem) {
                rreq->status.MPI_ERROR = MPI_ERR_TRUNCATE;
            } else {
                rreq->status.MPI_ERROR = MPI_SUCCESS;
            }

            MPIR_STATUS_SET_COUNT(rreq->status, done);
        }

        if (MPIDI_OFI_AMREQUEST_HDR(rreq, target_cmpl_cb)) {
            //printf("calling completion callback. cc = %d\n", MPIR_cc_get(rreq->cc));
            MPIDI_OFI_AMREQUEST_HDR(rreq, target_cmpl_cb) (rreq);
        }
        MPID_Request_complete(rreq);    /* FIXME: Should not call MPIDI in NM ? */
    }

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_OFI_HANDLE_LMT_DATA);
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
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_OFI_HANDLE_LMT_ACK);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_OFI_HANDLE_LMT_ACK);

    ack_msg = (MPIDI_OFI_ack_msg_payload_t *) msg_hdr->payload;
    sreq = (MPIR_Request *) ack_msg->sreq_ptr;

    if (!MPIDI_OFI_ENABLE_RMA) {
        /* do LMT */
        MPIDI_OFI_am_header_t *msg_hdr = &MPIDI_OFI_AMREQUEST_HDR(sreq, msg_hdr);
        MPIDI_OFI_lmt_msg_t *lmt_msg = &(MPIDI_OFI_AMREQUEST_HDR(sreq, lmt_msg));

        //printf("doing LMT for sreq %p\n", sreq);
        //printf("req_hdr = %p\n", MPIDI_OFI_AMREQUEST(sreq, req_hdr));
        //printf("message size is %d\n", lmt_msg->data_sz);

        msg_hdr->am_type = MPIDI_AMTYPE_LMT_DATA;
        msg_hdr->am_hdr_sz = sizeof(MPIDI_OFI_lmt_data_t);
        ((MPIDI_OFI_lmt_data_msg_t *) msg_hdr)->lmt_data.rreq_ptr = ack_msg->rreq_ptr;

        MPIDI_OFI_AMREQUEST(sreq, event_id) = MPIDI_OFI_EVENT_AM_SEND;

        size_t send_sz = MPIDI_OFI_DEFAULT_SHORT_SEND_SIZE - sizeof(MPIDI_OFI_lmt_data_msg_t);
        size_t num_sends = lmt_msg->data_sz / send_sz;
        if (lmt_msg->data_sz % send_sz)
            num_sends++;        /* for the last packet */
        MPIDI_OFI_AMREQUEST_HDR(sreq, lmt_cntr) = num_sends;

        //printf("sending %lu packets with %lu bytes each\n", num_sends, send_sz);

        MPIDI_OFI_AMREQUEST_HDR(sreq, iovs) =
            MPL_malloc(sizeof(struct iovec) * num_sends * 2, MPL_MEM_BUFFER);
        struct iovec *iov = MPIDI_OFI_AMREQUEST_HDR(sreq, iovs);
        MPIDI_OFI_ASSERT_IOVEC_ALIGN(iov);

        int i, c;
        char *curr = (char *) lmt_msg->data;
        for (i = 0; i < num_sends * 2; i += 2) {
            iov[i].iov_base = msg_hdr;
            iov[i].iov_len = sizeof(MPIDI_OFI_lmt_data_msg_t);
            iov[i + 1].iov_base = curr;
            iov[i + 1].iov_len = MPL_MIN(lmt_msg->data_sz, send_sz);
            curr += iov[i + 1].iov_len;
            lmt_msg->data_sz -= iov[i + 1].iov_len;
            MPIR_cc_incr(sreq->cc_ptr, &c);     /* send completion */
            //printf("sending %lu bytes, %lu left to send\n", iov[i+1].iov_len, lmt_msg->data_sz);
            MPIDI_OFI_CALL_RETRY_AM(fi_sendv(MPIDI_Global.ctx[0].tx, &iov[i], NULL, 2,
                                             MPIDI_OFI_comm_to_phys(lmt_msg->comm, lmt_msg->rank),
                                             &MPIDI_OFI_AMREQUEST(sreq, context)),
                                    FALSE /* no lock */ , sendv);
        }
        //printf("doing issuing sends\n");
    } else {
        if (MPIDI_OFI_ENABLE_MR_SCALABLE) {
            uint32_t idx_back;
            int key_type;
            uint64_t mr_key = fi_mr_key(MPIDI_OFI_AMREQUEST_HDR(sreq, lmt_mr));

            MPIDI_OFI_rma_key_unpack(mr_key, NULL, &key_type, &idx_back);
            MPIR_Assert(MPIDI_OFI_KEY_TYPE_HUGE_RMA == key_type);

            MPIDI_OFI_index_allocator_free(MPIDI_OFI_COMM(MPIR_Process.comm_world).rma_id_allocator,
                                           idx_back);
        }
        MPIDI_OFI_CALL_NOLOCK(fi_close(&MPIDI_OFI_AMREQUEST_HDR(sreq, lmt_mr)->fid), mr_unreg);
        OPA_decr_int(&MPIDI_Global.am_inflight_rma_send_mrs);

        handler_id = MPIDI_OFI_AMREQUEST_HDR(sreq, msg_hdr).handler_id;
        MPID_Request_complete(sreq);    /* FIXME: Should not call MPIDI in NM ? */
        mpi_errno = MPIDIG_global.origin_cbs[handler_id] (sreq);
    }

    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_OFI_HANDLE_LMT_ACK);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#endif /* OFI_AM_EVENTS_H_INCLUDED */
