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

    if (!MPIDI_OFI_ENABLE_MR_PROV_KEY) {
        uint64_t mr_key = fi_mr_key(MPIDI_OFI_AM_SREQ_HDR(sreq, lmt_mr));
        MPIDI_OFI_mr_key_free(MPIDI_OFI_LOCAL_MR_KEY, mr_key);
    }
    MPIDI_OFI_CALL(fi_close(&MPIDI_OFI_AM_SREQ_HDR(sreq, lmt_mr)->fid), mr_unreg);
    MPL_atomic_fetch_sub_int(&MPIDI_OFI_global.am_inflight_rma_send_mrs, 1);

    MPIR_gpu_free_host(MPIDI_OFI_AM_SREQ_HDR(sreq, pack_buffer));

    /* retrieve the handler_id of the original send request for origin cb. Note the handler_id
     * parameter is MPIDI_OFI_AM_RDMA_READ_ACK and should never be called with origin_cbs */
    src_handler_id = MPIDI_OFI_AM_SREQ_HDR(sreq, msg_hdr).handler_id;

    /* cleanup in case origin_cb call am send again */
    MPIDI_OFI_AM_FREE_REQ_HDR(MPIDI_OFI_AMREQUEST(sreq, sreq_hdr));

    mpi_errno = MPIDIG_global.origin_cbs[src_handler_id] (sreq);
    MPIR_ERR_CHECK(mpi_errno);
    MPID_Request_complete(sreq);

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static int do_rdma_read(void *dst, uint64_t src, size_t data_sz, int src_rank,
                        bool is_last_read, MPIR_Request * rreq)
{
    int mpi_errno = MPI_SUCCESS;
    size_t done = 0, curr_len, rem = 0;
    MPIR_Comm *comm;
    MPIR_FUNC_ENTER;

    rem = data_sz;

    while (done != data_sz) {
        curr_len = MPL_MIN(rem, MPIDI_OFI_global.max_msg_size);

        MPIDI_OFI_am_read_req_t *read_req;
        MPIDU_genq_private_pool_alloc_cell(MPIDI_OFI_global.am_hdr_buf_pool, (void **) &read_req);
        MPIR_Assert(read_req);

        read_req->rreq_ptr = rreq;
        read_req->event_id = MPIDI_OFI_EVENT_AM_READ;
        read_req->is_last = is_last_read ? (curr_len == rem) : false;

        if (rreq->kind == MPIR_REQUEST_KIND__RMA) {
            comm = rreq->u.rma.win->comm_ptr;
        } else {
            comm = rreq->comm;
        }
        MPIR_Assert(comm);

        /* am uses vni 0 */
        int vni_local = 0;
        int vni_remote = 0;
        int nic = 0;
        MPIDI_OFI_cntr_incr(comm, vni_local, nic);

        struct iovec iov = {
            .iov_base = (char *) dst + done,
            .iov_len = curr_len
        };
        struct fi_rma_iov rma_iov = {
            .addr = src + done,
            .len = curr_len,
            .key = MPIDI_OFI_AM_RDMA_READ_HDR(rreq, lmt_info.rma_key)
        };
        struct fi_msg_rma msg = {
            .msg_iov = &iov,
            .desc = NULL,
            .iov_count = 1,
            .addr = MPIDI_OFI_comm_to_phys(comm, src_rank, nic, vni_local, vni_remote),
            .rma_iov = &rma_iov,
            .rma_iov_count = 1,
            .context = &read_req->context,
            .data = 0
        };

        int ctx_idx = MPIDI_OFI_get_ctx_index(comm, vni_local, nic);
        MPIDI_OFI_CALL_RETRY_AM(fi_readmsg(MPIDI_OFI_global.ctx[ctx_idx].tx, &msg, FI_COMPLETION),
                                rdma_readfrom);

        done += curr_len;
        rem -= curr_len;
    }

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static int do_long_am_recv_contig(void *p_data, MPI_Aint data_sz, MPIR_Request * rreq)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_ENTER;

    MPI_Aint src_rank = MPIDI_OFI_AM_RDMA_READ_HDR(rreq, lmt_info.src_rank);
    MPI_Aint in_data_sz = MPIDI_OFI_AM_RDMA_READ_HDR(rreq, lmt_info.reg_sz);
    MPI_Aint src_offset = MPIDI_OFI_AM_RDMA_READ_HDR(rreq, lmt_info.src_offset);

    MPIDI_OFI_AM_RDMA_READ_HDR(rreq, lmt_type) = MPIDI_OFI_AM_LMT_IOV;
    if (in_data_sz > data_sz) {
        rreq->status.MPI_ERROR = MPIDIG_ERR_TRUNCATE(data_sz, in_data_sz);
    }
    data_sz = MPL_MIN(data_sz, in_data_sz);

    mpi_errno = do_rdma_read(p_data, src_offset, data_sz, src_rank, true, rreq);
    MPIR_ERR_CHECK(mpi_errno);
    MPIR_STATUS_SET_COUNT(rreq->status, data_sz);

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static int do_long_am_recv_iov(struct iovec *iov, MPI_Aint iov_len, MPIR_Request * rreq)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_ENTER;

    MPI_Aint src_rank = MPIDI_OFI_AM_RDMA_READ_HDR(rreq, lmt_info.src_rank);
    MPI_Aint in_data_sz = MPIDI_OFI_AM_RDMA_READ_HDR(rreq, lmt_info.reg_sz);
    MPI_Aint src_offset = MPIDI_OFI_AM_RDMA_READ_HDR(rreq, lmt_info.src_offset);

    MPIDI_OFI_AM_RDMA_READ_HDR(rreq, lmt_type) = MPIDI_OFI_AM_LMT_IOV;
    MPI_Aint rem, curr_len;

    int done = 0;
    rem = in_data_sz;
    for (int i = 0; i < iov_len && rem > 0; i++) {
        curr_len = MPL_MIN(rem, iov[i].iov_len);
        mpi_errno = do_rdma_read(iov[i].iov_base, src_offset + done, curr_len, src_rank,
                                 (curr_len == rem), rreq);
        MPIR_ERR_CHECK(mpi_errno);
        rem -= curr_len;
        done += curr_len;
    }

    if (rem) {
        rreq->status.MPI_ERROR = MPIDIG_ERR_TRUNCATE(done, in_data_sz);
    }

    MPIR_STATUS_SET_COUNT(rreq->status, done);

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static int do_long_am_recv_unpack(MPIR_Request * rreq)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_ENTER;

    MPI_Aint src_rank = MPIDI_OFI_AM_RDMA_READ_HDR(rreq, lmt_info.src_rank);
    MPI_Aint src_offset = MPIDI_OFI_AM_RDMA_READ_HDR(rreq, lmt_info.src_offset);

    MPIDI_OFI_AM_RDMA_READ_HDR(rreq, lmt_type) = MPIDI_OFI_AM_LMT_UNPACK;
    MPIDIG_recv_setup(rreq);

    MPI_Aint pack_size = 100 * 1024;
    if (pack_size > MPIDI_OFI_global.max_msg_size) {
        pack_size = MPIDI_OFI_global.max_msg_size;
    }

    void *unpack_buffer;
    mpi_errno = MPIR_gpu_malloc_host(&unpack_buffer, pack_size);
    MPIR_ERR_CHECK(mpi_errno);

    MPI_Aint remain = MPIDIG_REQUEST(rreq, req->recv_async).in_data_sz;
    if (pack_size > remain) {
        pack_size = remain;
    }

    MPIDI_OFI_AM_RDMA_READ_HDR(rreq, lmt_u.unpack).unpack_buffer = unpack_buffer;
    MPIDI_OFI_AM_RDMA_READ_HDR(rreq, lmt_u.unpack).pack_size = pack_size;

    mpi_errno = do_rdma_read(unpack_buffer, src_offset, pack_size, src_rank,
                             (pack_size == remain), rreq);
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIDI_OFI_am_lmt_unpack_event(MPIR_Request * rreq)
{
    MPI_Aint src_rank = MPIDI_OFI_AM_RDMA_READ_HDR(rreq, lmt_info.src_rank);
    MPI_Aint src_offset = MPIDI_OFI_AM_RDMA_READ_HDR(rreq, lmt_info.src_offset);
    void *unpack_buffer = MPIDI_OFI_AM_RDMA_READ_HDR(rreq, lmt_u.unpack).unpack_buffer;
    MPI_Aint pack_size = MPIDI_OFI_AM_RDMA_READ_HDR(rreq, lmt_u.unpack).pack_size;

    int ret = MPIDIG_recv_copy_seg(unpack_buffer, pack_size, rreq);
    MPI_Aint remain = MPIDIG_REQUEST(rreq, req->recv_async).in_data_sz;
    MPI_Aint offset = MPIDIG_REQUEST(rreq, req->recv_async).offset;

    if (!ret && remain) {
        /* more to go */
        if (pack_size > remain) {
            pack_size = remain;
        }
        MPIDI_OFI_AM_RDMA_READ_HDR(rreq, lmt_u.unpack).pack_size = pack_size;
        int rc = do_rdma_read(unpack_buffer, src_offset + offset, pack_size, src_rank,
                              (pack_size == remain), rreq);
        MPIR_Assert(rc == MPI_SUCCESS);
        return FALSE;
    } else {
        /* all done. */
        MPIR_gpu_free_host(unpack_buffer);
        return TRUE;
    }
}

int MPIDI_OFI_am_rdma_read_recv_cb(MPIR_Request * rreq)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_ENTER;

    MPI_Aint in_data_sz = MPIDI_OFI_AM_RDMA_READ_HDR(rreq, lmt_info.reg_sz);
    int num_iov = MPIDIG_get_recv_iov_count(rreq);
    if (num_iov > 1 && in_data_sz / num_iov < MPIR_CVAR_CH4_IOV_DENSITY_MIN) {
        /* noncontig data with mostly tiny segments */
        mpi_errno = do_long_am_recv_unpack(rreq);
    } else {
        int is_contig, is_gpu;
        void *p_data;
        MPI_Aint data_sz;
        MPIDIG_get_recv_data(&is_contig, &is_gpu, &p_data, &data_sz, rreq);

        if (is_gpu) {
            /* Use unpack for GPU buffer */
            mpi_errno = do_long_am_recv_unpack(rreq);
        } else if (is_contig) {
            mpi_errno = do_long_am_recv_contig(p_data, data_sz, rreq);
        } else {
            mpi_errno = do_long_am_recv_iov(p_data, data_sz, rreq);
        }
    }

    MPIR_FUNC_EXIT;
    return mpi_errno;
}
