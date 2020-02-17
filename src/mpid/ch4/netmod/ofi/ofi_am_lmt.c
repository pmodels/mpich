/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2020 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 *  Portions of this code were written by Intel Corporation.
 *  Copyright (C) 2011-2016 Intel Corporation.  Intel provides this material
 *  to Argonne National Laboratory subject to Software Grant and Corporate
 *  Contributor License Agreement dated February 8, 2012.
 */

#include "mpidimpl.h"
#include "ofi_am_lmt.h"

static int ofi_handle_lmt_req(int handler_id, void *am_hdr, void **data, size_t * data_sz,
                              int is_local, int *is_contig, MPIR_Request ** req);
static int ofi_handle_lmt_ack(int handler_id, void *am_hdr, void **data, size_t * data_sz,
                              int is_local, int *is_contig, MPIR_Request ** req);

void MPIDI_OFI_am_lmt_init(void)
{
    MPIDIG_am_reg_cb(MPIDI_OFI_AM_LMT_REQ, NULL, &ofi_handle_lmt_req);
    MPIDIG_am_reg_cb(MPIDI_OFI_AM_LMT_ACK, NULL, &ofi_handle_lmt_ack);
}

/* MPIDI_OFI_AM_LMT_REQ */

int MPIDI_OFI_am_lmt_send(int rank, MPIR_Comm * comm, int handler_id,
                          const void *am_hdr, size_t am_hdr_sz,
                          const void *data, MPI_Aint data_sz, MPIR_Request * sreq)
{
    int mpi_errno = MPI_SUCCESS;

    MPIDI_OFI_am_header_t *msg_hdr;
    MPIDI_OFI_lmt_msg_payload_t *lmt_info;
    struct iovec *iov;

    MPIR_Assert(handler_id < (1 << MPIDI_OFI_AM_HANDLER_ID_BITS));
    MPIR_Assert(am_hdr_sz < (1ULL << MPIDI_OFI_AM_HDR_SZ_BITS));
    MPIR_Assert(data_sz < (1ULL << MPIDI_OFI_AM_DATA_SZ_BITS));
    MPIR_Assert((uint64_t) comm->rank < (1ULL << MPIDI_OFI_AM_RANK_BITS));

    msg_hdr = &MPIDI_OFI_AMREQUEST_HDR(sreq, msg_hdr);
    msg_hdr->handler_id = handler_id;
    msg_hdr->am_hdr_sz = am_hdr_sz;
    msg_hdr->data_sz = data_sz;
    msg_hdr->am_type = MPIDI_AMTYPE_LMT_REQ;
    msg_hdr->seqno = MPIDI_OFI_am_fetch_incr_send_seqno(comm, rank);
    msg_hdr->fi_src_addr
        = MPIDI_OFI_comm_to_phys(MPIR_Process.comm_world, MPIR_Process.comm_world->rank);

    lmt_info = &MPIDI_OFI_AMREQUEST_HDR(sreq, lmt_info);
    lmt_info->context_id = comm->context_id;
    lmt_info->src_rank = comm->rank;
    lmt_info->src_offset =
        !MPIDI_OFI_ENABLE_MR_VIRT_ADDRESS ? (uint64_t) 0 : (uint64_t) (uintptr_t) data;

    lmt_info->sreq_ptr = sreq;
    if (!MPIDI_OFI_ENABLE_MR_PROV_KEY) {
        lmt_info->rma_key = MPIDI_OFI_mr_key_alloc();
    } else {
        lmt_info->rma_key = 0;
    }

    MPIR_Assert((sizeof(*msg_hdr) + sizeof(*lmt_info) + am_hdr_sz) <=
                MPIDI_OFI_DEFAULT_SHORT_SEND_SIZE);
    MPIDI_OFI_CALL(fi_mr_reg(MPIDI_OFI_global.ctx[0].domain,
                             data,
                             data_sz,
                             FI_REMOTE_READ,
                             0ULL,
                             lmt_info->rma_key,
                             0ULL, &MPIDI_OFI_AMREQUEST_HDR(sreq, lmt_mr), NULL), mr_reg);
    OPA_incr_int(&MPIDI_OFI_global.am_inflight_rma_send_mrs);

    if (MPIDI_OFI_ENABLE_MR_PROV_KEY) {
        /* MR_BASIC */
        lmt_info->rma_key = fi_mr_key(MPIDI_OFI_AMREQUEST_HDR(sreq, lmt_mr));
    }

    iov = MPIDI_OFI_AMREQUEST_HDR(sreq, iov);

    iov[0].iov_base = msg_hdr;
    iov[0].iov_len = sizeof(*msg_hdr);

    iov[1].iov_base = MPIDI_OFI_AMREQUEST_HDR(sreq, am_hdr);
    iov[1].iov_len = am_hdr_sz;

    iov[2].iov_base = lmt_info;
    iov[2].iov_len = sizeof(*lmt_info);
    MPIDI_OFI_AMREQUEST(sreq, event_id) = MPIDI_OFI_EVENT_AM_SEND;
    MPIDI_OFI_ASSERT_IOVEC_ALIGN(iov);
    MPIDI_OFI_CALL_RETRY_AM(fi_sendv(MPIDI_OFI_global.ctx[0].tx, iov, NULL, 3,
                                     MPIDI_OFI_comm_to_phys(comm, rank),
                                     &MPIDI_OFI_AMREQUEST(sreq, context)), sendv);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static int ofi_handle_lmt_req(int handler_id, void *am_hdr, void **data, size_t * data_sz,
                              int is_local, int *is_contig, MPIR_Request ** req)
{
    int mpi_errno = MPI_SUCCESS;

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static inline int MPIDI_OFI_handle_long_am(MPIDI_OFI_am_header_t * msg_hdr)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_OFI_lmt_msg_payload_t *lmt_msg;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_OFI_HANDLE_LONG_AM);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_OFI_HANDLE_LONG_AM);

    /* note: msg_hdr + 1 points to the payload */
    lmt_msg = (MPIDI_OFI_lmt_msg_payload_t *) ((char *) (msg_hdr + 1) + msg_hdr->am_hdr_sz);
    mpi_errno = MPIDI_OFI_do_handle_long_am(msg_hdr, lmt_msg, msg_hdr + 1);

    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_OFI_HANDLE_LONG_AM);
    return mpi_errno;

  fn_fail:
    goto fn_exit;
}

static inline int MPIDI_OFI_do_handle_long_am(MPIDI_OFI_am_header_t * msg_hdr,
                                              MPIDI_OFI_lmt_msg_payload_t * lmt_msg, void *am_hdr)
{
    int num_reads, i, iov_len, c, mpi_errno = MPI_SUCCESS, is_contig = 0;
    MPIR_Request *rreq = NULL;
    void *p_data;
    size_t data_sz, rem, done, curr_len, in_data_sz;
    struct iovec *iov;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_OFI_DO_HANDLE_LONG_AM);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_OFI_DO_HANDLE_LONG_AM);

    in_data_sz = data_sz = msg_hdr->data_sz;
    MPIDIG_global.target_msg_cbs[msg_hdr->handler_id] (msg_hdr->handler_id, am_hdr,
                                                       &p_data, &data_sz, 0 /* is_local */ ,
                                                       &is_contig, &rreq);

    if (!rreq)
        goto fn_exit;

    MPIDI_OFI_am_clear_request(rreq);
    mpi_errno = MPIDI_OFI_am_init_request(NULL, 0, rreq);

    MPIR_ERR_CHECK(mpi_errno);

    MPIR_cc_incr(rreq->cc_ptr, &c);

    if (!p_data || !data_sz) {
        MPIDIG_REQUEST(rreq, req->target_cmpl_cb) (rreq);
        goto fn_exit;
    }

    MPIDI_OFI_AMREQUEST_HDR(rreq, msg_hdr) = *msg_hdr;
    MPIDI_OFI_AMREQUEST_HDR(rreq, lmt_info) = *lmt_msg;
    MPIDI_OFI_AMREQUEST_HDR(rreq, rreq_ptr) = (void *) rreq;

    if (is_contig) {
        if (in_data_sz > data_sz) {
            rreq->status.MPI_ERROR = MPIR_Err_create_code(rreq->status.MPI_ERROR,
                                                          MPIR_ERR_RECOVERABLE, __func__,
                                                          __LINE__, MPI_ERR_TRUNCATE, "**truncate",
                                                          "**truncate %d %d %d %d",
                                                          rreq->status.MPI_SOURCE,
                                                          rreq->status.MPI_TAG, data_sz,
                                                          in_data_sz);
        }

        data_sz = MPL_MIN(data_sz, in_data_sz);
        MPIDI_OFI_AMREQUEST_HDR(rreq, lmt_cntr) =
            ((data_sz - 1) / MPIDI_OFI_global.max_msg_size) + 1;
        MPIDI_OFI_do_rdma_read(p_data, lmt_msg->src_offset, data_sz, lmt_msg->context_id,
                               lmt_msg->src_rank, rreq);
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
            num_reads = ((curr_len - 1) / MPIDI_OFI_global.max_msg_size) + 1;
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
            rreq->status.MPI_ERROR = MPIR_Err_create_code(rreq->status.MPI_ERROR,
                                                          MPIR_ERR_RECOVERABLE, __func__, __LINE__,
                                                          MPI_ERR_TRUNCATE, "**truncate",
                                                          "**truncate %d %d %d %d",
                                                          rreq->status.MPI_SOURCE,
                                                          rreq->status.MPI_TAG, data_sz,
                                                          in_data_sz);
        }

        MPIR_STATUS_SET_COUNT(rreq->status, done);
    }

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_OFI_DO_HANDLE_LONG_AM);
    return mpi_errno;

  fn_fail:
    goto fn_exit;
}

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
        curr_len = MPL_MIN(rem, MPIDI_OFI_global.max_msg_size);

        MPIR_Assert(sizeof(MPIDI_OFI_am_request_t) <= MPIDI_OFI_BUF_POOL_SIZE);
        am_req = (MPIDI_OFI_am_request_t *) MPIDIU_get_buf(MPIDI_OFI_global.am_buf_pool);
        MPIR_Assert(am_req);

        am_req->req_hdr = MPIDI_OFI_AMREQUEST(rreq, req_hdr);
        am_req->event_id = MPIDI_OFI_EVENT_AM_READ;
        comm = MPIDIG_context_id_to_comm(context_id);
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

        MPIDI_OFI_CALL_RETRY_AM(fi_readmsg(MPIDI_OFI_global.ctx[0].tx, &msg, FI_COMPLETION), read);

        done += curr_len;
        rem -= curr_len;
    }

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_OFI_DO_RDMA_READ);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIDI_OFI_lmt_read_event(void *context)
{
    int mpi_errno = MPI_SUCCESS;

    MPIDI_OFI_am_request_t *ofi_req;
    ofi_req = MPL_container_of(context, MPIDI_OFI_am_request_t, context);
    ofi_req->req_hdr->lmt_cntr--;

    if (ofi_req->req_hdr->lmt_cntr)
        goto fn_exit;

    MPIR_Request *rreq;
    rreq = (MPIR_Request *) ofi_req->req_hdr->rreq_ptr;
    mpi_errno = MPIDI_OFI_dispatch_ack(MPIDI_OFI_AMREQUEST_HDR(rreq, lmt_info).src_rank,
                                       MPIDI_OFI_AMREQUEST_HDR(rreq, lmt_info).context_id,
                                       MPIDI_OFI_AMREQUEST_HDR(rreq, lmt_info).sreq_ptr,
                                       MPIDI_AMTYPE_LMT_ACK);

    MPIR_ERR_CHECK(mpi_errno);

    MPIDIG_REQUEST(rreq, req->target_cmpl_cb) (rreq);

  fn_exit:
    MPIDIU_release_buf((void *) ofi_req);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

/* MPIDI_OFI_AM_LMT_ACK */

static int ofi_do_lmt_ack(int rank, int context_id, MPIR_Request * sreq_ptr)
{
    int mpi_errno = MPI_SUCCESS;

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static inline int MPIDI_OFI_dispatch_ack(int rank, int context_id, MPIR_Request * sreq_ptr,
                                         int am_type)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_OFI_ack_msg_t msg;
    MPIR_Comm *comm;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_OFI_DISPATCH_ACK);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_OFI_DISPATCH_ACK);

    comm = MPIDIG_context_id_to_comm(context_id);

    msg.hdr.am_hdr_sz = sizeof(msg.pyld);
    msg.hdr.data_sz = 0;
    msg.hdr.am_type = am_type;
    msg.hdr.seqno = MPIDI_OFI_am_fetch_incr_send_seqno(comm, rank);
    msg.hdr.fi_src_addr
        = MPIDI_OFI_comm_to_phys(MPIR_Process.comm_world, MPIR_Process.comm_world->rank);
    msg.pyld.sreq_ptr = sreq_ptr;
    MPIDI_OFI_CALL_RETRY_AM(fi_inject(MPIDI_OFI_global.ctx[0].tx, &msg, sizeof(msg),
                                      MPIDI_OFI_comm_to_phys(comm, rank)), inject);
  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_OFI_DISPATCH_ACK);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static int ofi_handle_lmt_ack(int handler_id, void *am_hdr, void **data, size_t * data_sz,
                              int is_local, int *is_contig, MPIR_Request ** req)
{
    int mpi_errno = MPI_SUCCESS;

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static inline int MPIDI_OFI_handle_lmt_ack(MPIDI_OFI_am_header_t * msg_hdr)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Request *sreq;
    MPIDI_OFI_ack_msg_payload_t *ack_msg;
    int handler_id;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_OFI_HANDLE_LMT_ACK);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_OFI_HANDLE_LMT_ACK);

    ack_msg = (MPIDI_OFI_ack_msg_payload_t *) (msg_hdr + 1);
    sreq = ack_msg->sreq_ptr;

    if (!MPIDI_OFI_ENABLE_MR_PROV_KEY) {
        uint64_t mr_key = fi_mr_key(MPIDI_OFI_AMREQUEST_HDR(sreq, lmt_mr));
        MPIDI_OFI_mr_key_free(mr_key);
    }
    MPIDI_OFI_CALL(fi_close(&MPIDI_OFI_AMREQUEST_HDR(sreq, lmt_mr)->fid), mr_unreg);
    OPA_decr_int(&MPIDI_OFI_global.am_inflight_rma_send_mrs);

    MPL_free(MPIDI_OFI_AMREQUEST_HDR(sreq, pack_buffer));

    handler_id = MPIDI_OFI_AMREQUEST_HDR(sreq, msg_hdr).handler_id;
    mpi_errno = MPIDIG_global.origin_cbs[handler_id] (sreq);

    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_OFI_HANDLE_LMT_ACK);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
