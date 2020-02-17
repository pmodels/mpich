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

/* declare static functions */
static int ofi_do_rdma_read(void *dst, uint64_t src, size_t data_sz,
                            MPIR_Context_id_t context_id, int src_rank, MPIR_Request * rreq);

static int ofi_do_lmt_ack(int rank, int context_id, MPIR_Request * sreq_ptr);

/* MPIDI_OFI_AM_LMT_REQ */

struct am_lmt_hdr {
    /* payload rdma info */
    MPIR_Context_id_t context_id;
    int src_rank;
    uint64_t src_offset;
    uint64_t rma_key;
    /* orig am info */
    int handler_id;
    MPI_Aint am_hdr_sz;
    MPI_Aint data_sz;
    MPIR_Request *sreq_ptr;
};

int MPIDI_OFI_am_lmt_send(int rank, MPIR_Comm * comm, int handler_id,
                          const void *am_hdr, size_t am_hdr_sz,
                          const void *data, MPI_Aint data_sz, MPIR_Request * sreq)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_Assert(handler_id < (1 << MPIDI_OFI_AM_HANDLER_ID_BITS));
    MPIR_Assert(am_hdr_sz < (1ULL << MPIDI_OFI_AM_HDR_SZ_BITS));
    MPIR_Assert(data_sz < (1ULL << MPIDI_OFI_AM_DATA_SZ_BITS));
    MPIR_Assert((uint64_t) comm->rank < (1ULL << MPIDI_OFI_AM_RANK_BITS));

    int hdr_size = sizeof(struct am_lmt_hdr) + am_hdr_sz;
    struct am_lmt_hdr *p_hdr = MPL_malloc(hdr_size, MPL_MEM_OTHER);

    /* payload rdma info */
    p_hdr->context_id = comm->context_id;
    p_hdr->src_rank = comm->rank;
    p_hdr->src_offset =
        !MPIDI_OFI_ENABLE_MR_VIRT_ADDRESS ? (uint64_t) 0 : (uint64_t) (uintptr_t) data;
    /* orig am info */
    p_hdr->handler_id = handler_id;
    p_hdr->am_hdr_sz = am_hdr_sz;
    p_hdr->data_sz = data_sz;
    p_hdr->sreq_ptr = sreq;

    if (!MPIDI_OFI_ENABLE_MR_PROV_KEY) {
        p_hdr->rma_key = MPIDI_OFI_mr_key_alloc();
    } else {
        p_hdr->rma_key = 0;
    }

    MPIR_Memcpy(p_hdr + 1, am_hdr, am_hdr_sz);

    struct fid_mr *lmt_mr;
    MPIDI_OFI_CALL(fi_mr_reg(MPIDI_OFI_global.ctx[0].domain,
                             data, data_sz, FI_REMOTE_READ, 0ULL,
                             p_hdr->rma_key, 0ULL, &lmt_mr, NULL), mr_reg);
    MPIDI_OFI_AMREQUEST_HDR(sreq, lmt_mr) = lmt_mr;

    /* skip this. The protocol should prevent this to be needed.
     * OPA_incr_int(&MPIDI_OFI_global.am_inflight_rma_send_mrs);
     */

    if (MPIDI_OFI_ENABLE_MR_PROV_KEY) {
        p_hdr->rma_key = fi_mr_key(MPIDI_OFI_AMREQUEST_HDR(sreq, lmt_mr));
    }

    mpi_errno = MPIDI_OFI_do_inject(rank, comm, MPIDI_OFI_AM_LMT_REQ, p_hdr, hdr_size);
    MPL_free(p_hdr);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static int ofi_handle_lmt_req(int handler_id, void *am_hdr, void **data, size_t * data_sz,
                              int is_local, int *is_contig, MPIR_Request ** req)
{
    int mpi_errno = MPI_SUCCESS;

    struct am_lmt_hdr *p_hdr = (struct am_lmt_hdr *) am_hdr;

    void *p_data = NULL;
    *data_sz = p_hdr->data_sz;
    MPIDIG_global.target_msg_cbs[p_hdr->handler_id] (p_hdr->handler_id, p_hdr + 1,
                                                     &p_data, data_sz, is_local, is_contig, req);
    MPIR_Request *rreq = *req;
    /* We are taking over the completion */
    *req = NULL;

    /* this should not happen */
    if (!rreq)
        goto fn_exit;

    /* this should also not happen */
    if (!p_data || !*data_sz) {
        MPIDIG_REQUEST(rreq, req->target_cmpl_cb) (rreq);
        goto fn_exit;
    }

    MPI_Aint in_data_sz = p_hdr->data_sz;
    MPI_Aint out_data_sz = *data_sz;

    /* prepare rreq for rdma-read */
    MPIDI_OFI_am_clear_request(rreq);
    mpi_errno = MPIDI_OFI_am_init_request(NULL, 0, rreq);
    MPIR_ERR_CHECK(mpi_errno);

    /* persist data for rdma completion callback */
    MPIDI_OFI_AMREQUEST_HDR(rreq, rreq_ptr) = (void *) rreq;
    MPIDI_OFI_AMREQUEST_HDR(rreq, lmt_info.src_rank) = p_hdr->src_rank;
    MPIDI_OFI_AMREQUEST_HDR(rreq, lmt_info.context_id) = p_hdr->context_id;
    MPIDI_OFI_AMREQUEST_HDR(rreq, lmt_info.sreq_ptr) = p_hdr->sreq_ptr;

    if (*is_contig) {
        if (in_data_sz > out_data_sz) {
            rreq->status.MPI_ERROR = MPIDIG_ERR_TRUNCATE(out_data_sz, in_data_sz);
        }

        out_data_sz = MPL_MIN(out_data_sz, in_data_sz);
        MPIDI_OFI_AMREQUEST_HDR(rreq, lmt_cntr) =
            ((out_data_sz - 1) / MPIDI_OFI_global.max_msg_size) + 1;
        ofi_do_rdma_read(p_data, p_hdr->src_offset, out_data_sz,
                         p_hdr->context_id, p_hdr->src_rank, rreq);
        MPIR_STATUS_SET_COUNT(rreq->status, out_data_sz);
    } else {
        int done = 0;
        MPI_Aint rem = in_data_sz;
        struct iovec *iov = p_data;
        int iov_len = out_data_sz;

        /* FIXME: optimize iov processing part */

        /* set lmt counter */
        MPIDI_OFI_AMREQUEST_HDR(rreq, lmt_cntr) = 0;

        for (int i = 0; i < iov_len && rem > 0; i++) {
            MPI_Aint curr_len = MPL_MIN(rem, iov[i].iov_len);
            int num_reads = ((curr_len - 1) / MPIDI_OFI_global.max_msg_size) + 1;
            MPIDI_OFI_AMREQUEST_HDR(rreq, lmt_cntr) += num_reads;
            rem -= curr_len;
        }

        done = 0;
        rem = in_data_sz;

        for (int i = 0; i < iov_len && rem > 0; i++) {
            MPI_Aint curr_len = MPL_MIN(rem, iov[i].iov_len);
            ofi_do_rdma_read(iov[i].iov_base, p_hdr->src_offset + done,
                             curr_len, p_hdr->context_id, p_hdr->src_rank, rreq);
            rem -= curr_len;
            done += curr_len;
        }

        if (rem) {
            rreq->status.MPI_ERROR = MPIDIG_ERR_TRUNCATE(out_data_sz, in_data_sz);
        }

        MPIR_STATUS_SET_COUNT(rreq->status, done);
    }


  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static int ofi_lmt_target_cmpl_cb(MPIDI_OFI_am_request_header_t * req_hdr)
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = ofi_do_lmt_ack(req_hdr->lmt_info.src_rank,
                               req_hdr->lmt_info.context_id, req_hdr->lmt_info.sreq_ptr);
    MPIR_ERR_CHECK(mpi_errno);

    MPIR_Request *rreq = (MPIR_Request *) req_hdr->rreq_ptr;
    MPIDIG_REQUEST(rreq, req->target_cmpl_cb) (rreq);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_OFI_HANDLE_LONG_AM);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static int ofi_do_rdma_read(void *dst, uint64_t src, size_t data_sz,
                            MPIR_Context_id_t context_id, int src_rank, MPIR_Request * rreq)
{
    int mpi_errno = MPI_SUCCESS;
    size_t done = 0, curr_len, rem = 0;
    MPIDI_OFI_am_request_t *am_req;
    MPIR_Comm *comm;

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
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIDI_OFI_am_lmt_read_event(void *context)
{
    int mpi_errno = MPI_SUCCESS;

    MPIDI_OFI_am_request_t *am_req = context;
    MPIDI_OFI_am_request_header_t *req_hdr = am_req->req_hdr;
    req_hdr->lmt_cntr--;

    if (req_hdr->lmt_cntr == 0) {
        mpi_errno = ofi_lmt_target_cmpl_cb(req_hdr);
        MPIR_ERR_CHECK(mpi_errno);
    }

    MPIDIU_release_buf(am_req);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

/* MPIDI_OFI_AM_LMT_ACK */

struct am_lmt_ack_hdr {
    MPIR_Request *sreq_ptr;
};

static int ofi_do_lmt_ack(int rank, int context_id, MPIR_Request * sreq_ptr)
{
    int mpi_errno = MPI_SUCCESS;

    struct am_lmt_ack_hdr am_hdr;
    am_hdr.sreq_ptr = sreq_ptr;

    MPIR_Comm *comm;
    comm = MPIDIG_context_id_to_comm(context_id);

    mpi_errno = MPIDI_OFI_do_inject(rank, comm, MPIDI_OFI_AM_LMT_ACK, &am_hdr, sizeof(am_hdr));

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static int ofi_handle_lmt_ack(int handler_id, void *am_hdr, void **data, size_t * data_sz,
                              int is_local, int *is_contig, MPIR_Request ** req)
{
    int mpi_errno = MPI_SUCCESS;

    struct am_lmt_ack_hdr *p_hdr = am_hdr;
    MPIR_Request *sreq = p_hdr->sreq_ptr;

    /* clean-up rdma-read */
    if (!MPIDI_OFI_ENABLE_MR_PROV_KEY) {
        uint64_t mr_key = fi_mr_key(MPIDI_OFI_AMREQUEST_HDR(sreq, lmt_mr));
        MPIDI_OFI_mr_key_free(mr_key);
    }
    MPIDI_OFI_CALL(fi_close(&MPIDI_OFI_AMREQUEST_HDR(sreq, lmt_mr)->fid), mr_unreg);
    OPA_decr_int(&MPIDI_OFI_global.am_inflight_rma_send_mrs);

    /* am send origin completion */
    void *context = &(sreq->dev.ch4.netmod.ofi);
    mpi_errno = MPIDI_OFI_am_send_event(context);
    MPIR_ERR_CHECK(mpi_errno);

    if (req) {
        *req = NULL;
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
