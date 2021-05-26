/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef OFI_AM_EVENTS_H_INCLUDED
#define OFI_AM_EVENTS_H_INCLUDED

#include "ofi_am_impl.h"
#include "mpidu_genq.h"

MPL_STATIC_INLINE_PREFIX uint16_t MPIDI_OFI_am_get_next_recv_seqno(fi_addr_t addr)
{
    uint64_t id = addr;
    void *r;

    r = MPIDIU_map_lookup(MPIDI_OFI_global.am_recv_seq_tracker, id);
    if (r == MPIDIU_MAP_NOT_FOUND) {
        MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_GENERAL, VERBOSE,
                        (MPL_DBG_FDEST, "First time adding recv seqno addr=%" PRIx64 "\n", addr));
        MPIDIU_map_set(MPIDI_OFI_global.am_recv_seq_tracker, id, 0, MPL_MEM_OTHER);
        return 0;
    } else {
        return (uint16_t) (uintptr_t) r;
    }
}

MPL_STATIC_INLINE_PREFIX void MPIDI_OFI_am_set_next_recv_seqno(fi_addr_t addr, uint16_t seqno)
{
    uint64_t id = addr;

    MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_GENERAL, VERBOSE,
                    (MPL_DBG_FDEST, "Next recv seqno=%d addr=%" PRIx64 "\n", seqno, addr));

    MPIDIU_map_update(MPIDI_OFI_global.am_recv_seq_tracker, id, (void *) (uintptr_t) seqno,
                      MPL_MEM_OTHER);
}

MPL_STATIC_INLINE_PREFIX int MPIDI_OFI_am_enqueue_unordered_msg(const MPIDI_OFI_am_header_t *
                                                                am_hdr)
{
    MPIDI_OFI_am_unordered_msg_t *uo_msg;
    size_t uo_msg_len, packet_len;
    /* Essentially, uo_msg_len == packet_len + sizeof(next,prev pointers) */

    uo_msg_len = sizeof(*uo_msg) + am_hdr->am_hdr_sz + am_hdr->payload_sz;

    /* Allocate a new memory region to store this unordered message.
     * We are doing this because the original am_hdr comes from FI_MULTI_RECV
     * buffer, which may be reused soon by OFI. */
    uo_msg = MPL_malloc(uo_msg_len, MPL_MEM_BUFFER);
    if (uo_msg == NULL)
        return MPI_ERR_NO_MEM;

    packet_len = sizeof(*am_hdr) + am_hdr->am_hdr_sz + am_hdr->payload_sz;
    MPIR_Memcpy(&uo_msg->am_hdr, am_hdr, packet_len);

    DL_APPEND(MPIDI_OFI_global.am_unordered_msgs, uo_msg);

    return MPI_SUCCESS;
}

/* Find and dequeue a message that matches (comm, src_rank, seqno), then return it.
 * Caller must free the returned pointer. */
MPL_STATIC_INLINE_PREFIX MPIDI_OFI_am_unordered_msg_t
    * MPIDI_OFI_am_claim_unordered_msg(fi_addr_t addr, uint16_t seqno)
{
    MPIDI_OFI_am_unordered_msg_t *uo_msg;

    /* Future optimization note:
     * Currently we are doing linear search every time, assuming that the number of items
     * in the queue is extremely small.
     * If it's not the case, we should consider using better data structure and algorithm
     * to look up. */
    DL_FOREACH(MPIDI_OFI_global.am_unordered_msgs, uo_msg) {
        if (uo_msg->am_hdr.fi_src_addr == addr && uo_msg->am_hdr.seqno == seqno) {
            MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_GENERAL, TERSE,
                            (MPL_DBG_FDEST,
                             "Found unordered message in the queue: addr=%" PRIx64 ", seqno=%d\n",
                             addr, seqno));
            DL_DELETE(MPIDI_OFI_global.am_unordered_msgs, uo_msg);
            return uo_msg;
        }
    }

    return NULL;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_OFI_handle_short_am(MPIDI_OFI_am_header_t * msg_hdr,
                                                       void *am_hdr, void *p_data)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_OFI_HANDLE_SHORT_AM);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_OFI_HANDLE_SHORT_AM);

    /* note: setting is_local, is_async, req to 0, 0, NULL */
    MPIDIG_global.target_msg_cbs[msg_hdr->handler_id] (msg_hdr->handler_id, am_hdr,
                                                       p_data, msg_hdr->payload_sz, 0, 0, NULL);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_OFI_HANDLE_SHORT_AM);
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

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_OFI_HANDLE_PIPELINE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_OFI_HANDLE_PIPELINE);

    cache_rreq = MPIDIG_req_cache_lookup(MPIDI_OFI_global.req_map, (uint64_t) msg_hdr->fi_src_addr);
    MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_GENERAL, VERBOSE,
                    (MPL_DBG_FDEST, "cached req %p handle=0x%x", cache_rreq,
                     cache_rreq ? cache_rreq->handle : 0));

    rreq = cache_rreq;

    if (!rreq) {
        /* no cached request, this must be the first segment */
        /* note: setting is_local, is_async, req to 0, 1, rreq */
        MPIDIG_global.target_msg_cbs[msg_hdr->handler_id] (msg_hdr->handler_id, am_hdr, p_data,
                                                           msg_hdr->payload_sz, 0, 1, &rreq);
        MPIDIG_recv_setup(rreq);
        MPIDIG_req_cache_add(MPIDI_OFI_global.req_map, (uint64_t) msg_hdr->fi_src_addr, rreq);
    }

    is_done = MPIDIG_recv_copy_seg(p_data, msg_hdr->payload_sz, rreq);
    if (is_done) {
        MPIDIG_REQUEST(rreq, req->target_cmpl_cb) (rreq);
        MPIDIG_req_cache_remove(MPIDI_OFI_global.req_map, (uint64_t) msg_hdr->fi_src_addr);
    }

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_OFI_HANDLE_PIPELINE);
    return mpi_errno;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_OFI_handle_short_am_hdr(MPIDI_OFI_am_header_t * msg_hdr,
                                                           void *am_hdr)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_OFI_HANDLE_SHORT_AM_HDR);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_OFI_HANDLE_SHORT_AM_HDR);

    MPIDIG_global.target_msg_cbs[msg_hdr->handler_id] (msg_hdr->handler_id, am_hdr,
                                                       NULL, 0, 0, 0, NULL);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_OFI_HANDLE_SHORT_AM_HDR);
    return mpi_errno;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_OFI_do_rdma_read(void *dst,
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

        MPIR_Assert(sizeof(MPIDI_OFI_am_request_t) <= MPIDI_OFI_AM_HDR_POOL_CELL_SIZE);
        MPIDU_genq_private_pool_alloc_cell(MPIDI_OFI_global.am_hdr_buf_pool, (void **) &am_req);
        MPIR_Assert(am_req);

        am_req->req_hdr = MPIDI_OFI_AMREQUEST(rreq, req_hdr);
        am_req->event_id = MPIDI_OFI_EVENT_AM_READ;
        comm = MPIDIG_context_id_to_comm(context_id);
        MPIR_Assert(comm);

        /* am uses vni 0 */
        int vni_local = 0;
        int vni_remote = 0;
        int nic = 0;
        MPIDI_OFI_cntr_incr(vni_local, nic);

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
            .addr = MPIDI_OFI_comm_to_phys(comm, src_rank, nic, vni_local, vni_remote),
            .rma_iov = &rma_iov,
            .rma_iov_count = 1,
            .context = &am_req->context,
            .data = 0
        };

        int ctx_idx = MPIDI_OFI_get_ctx_index(vni_local, nic);
        MPIDI_OFI_CALL_RETRY_AM(fi_readmsg(MPIDI_OFI_global.ctx[ctx_idx].tx, &msg, FI_COMPLETION),
                                rdma_readfrom);

        done += curr_len;
        rem -= curr_len;
    }

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_OFI_DO_RDMA_READ);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int do_long_am_recv(MPI_Aint in_data_sz, MPIR_Request * rreq,
                                             MPIDI_OFI_lmt_msg_payload_t * lmt_msg);
MPL_STATIC_INLINE_PREFIX int MPIDI_OFI_handle_rdma_read(MPIDI_OFI_am_header_t * msg_hdr,
                                                        void *am_hdr,
                                                        MPIDI_OFI_lmt_msg_payload_t * lmt_msg)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Request *rreq = NULL;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_OFI_HANDLE_RDMA_READ);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_OFI_HANDLE_RDMA_READ);

    /* note: setting is_local, is_async to 0, 1 */
    MPIDIG_global.target_msg_cbs[msg_hdr->handler_id] (msg_hdr->handler_id, am_hdr,
                                                       NULL, msg_hdr->payload_sz, 0, 1, &rreq);

    if (!rreq)
        goto fn_exit;

    MPIDI_OFI_am_clear_request(rreq);
    mpi_errno = MPIDI_OFI_am_init_request(NULL, 0, rreq);

    MPIR_ERR_CHECK(mpi_errno);

    MPIR_cc_inc(rreq->cc_ptr);

    /* FIXME: explicit check of data_sz in CH4 region of the request, will fix when adding
     * MPIDIG_am_recv */
    if (!lmt_msg->reg_sz) {
        MPIDIG_REQUEST(rreq, req->target_cmpl_cb) (rreq);
        MPID_Request_complete(rreq);
        goto fn_exit;
    }

    MPIDI_OFI_AMREQUEST_HDR(rreq, msg_hdr) = *msg_hdr;
    MPIDI_OFI_AMREQUEST_HDR(rreq, lmt_info) = *lmt_msg;
    MPIDI_OFI_AMREQUEST_HDR(rreq, rreq_ptr) = (void *) rreq;

    if (MPIDIG_IS_REQUEST_READY_FOR_RECV(rreq)) {
        mpi_errno = do_long_am_recv(lmt_msg->reg_sz, rreq, lmt_msg);
        MPIR_ERR_CHECK(mpi_errno);

        /* completion in lmt event functions */
    }

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_OFI_HANDLE_RDMA_READ);
    return mpi_errno;

  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_OFI_do_am_rdma_read_ack(int rank, int context_id,
                                                           MPIR_Request * sreq_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_OFI_am_rdma_read_ack_msg_t ack_msg;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_OFI_DO_AM_RDMA_READ_ACK);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_OFI_DO_AM_RDMA_READ_ACK);

    ack_msg.sreq_ptr = sreq_ptr;
    mpi_errno = MPIDI_NM_am_send_hdr_reply(context_id, rank, MPIDI_OFI_AM_RDMA_READ_ACK, &ack_msg,
                                           (MPI_Aint) sizeof(ack_msg));
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_OFI_DO_AM_RDMA_READ_ACK);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

/* internal routines */
MPL_STATIC_INLINE_PREFIX int do_long_am_recv_contig(void *p_data, MPI_Aint data_sz,
                                                    MPI_Aint in_data_sz, MPIR_Request * rreq,
                                                    MPIDI_OFI_lmt_msg_payload_t * lmt_msg)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_OFI_AMREQUEST_HDR(rreq, lmt_type) = MPIDI_OFI_AM_LMT_IOV;
    if (in_data_sz > data_sz) {
        rreq->status.MPI_ERROR = MPIDIG_ERR_TRUNCATE(data_sz, in_data_sz);
    }
    data_sz = MPL_MIN(data_sz, in_data_sz);
    MPIDI_OFI_AMREQUEST_HDR(rreq, lmt_u.lmt_cntr) =
        ((data_sz - 1) / MPIDI_OFI_global.max_msg_size) + 1;
    mpi_errno = MPIDI_OFI_do_rdma_read(p_data, lmt_msg->src_offset, data_sz, lmt_msg->context_id,
                                       lmt_msg->src_rank, rreq);
    MPIR_ERR_CHECK(mpi_errno);
    MPIR_STATUS_SET_COUNT(rreq->status, data_sz);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int do_long_am_recv_iov(struct iovec *iov, MPI_Aint iov_len,
                                                 MPI_Aint in_data_sz, MPIR_Request * rreq,
                                                 MPIDI_OFI_lmt_msg_payload_t * lmt_msg)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_OFI_AMREQUEST_HDR(rreq, lmt_type) = MPIDI_OFI_AM_LMT_IOV;
    MPI_Aint rem, curr_len;
    int num_reads;

    /* set lmt counter */
    MPIDI_OFI_AMREQUEST_HDR(rreq, lmt_u.lmt_cntr) = 0;

    rem = in_data_sz;
    for (int i = 0; i < iov_len && rem > 0; i++) {
        curr_len = MPL_MIN(rem, iov[i].iov_len);
        num_reads = ((curr_len - 1) / MPIDI_OFI_global.max_msg_size) + 1;
        MPIDI_OFI_AMREQUEST_HDR(rreq, lmt_u.lmt_cntr) += num_reads;
        rem -= curr_len;
    }

    int done = 0;
    rem = in_data_sz;
    for (int i = 0; i < iov_len && rem > 0; i++) {
        curr_len = MPL_MIN(rem, iov[i].iov_len);
        mpi_errno = MPIDI_OFI_do_rdma_read(iov[i].iov_base, lmt_msg->src_offset + done,
                                           curr_len, lmt_msg->context_id, lmt_msg->src_rank, rreq);
        MPIR_ERR_CHECK(mpi_errno);
        rem -= curr_len;
        done += curr_len;
    }

    if (rem) {
        rreq->status.MPI_ERROR = MPIDIG_ERR_TRUNCATE(done, in_data_sz);
    }

    MPIR_STATUS_SET_COUNT(rreq->status, done);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int do_long_am_recv_unpack(MPI_Aint in_data_sz, MPIR_Request * rreq,
                                                    MPIDI_OFI_lmt_msg_payload_t * lmt_msg)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_OFI_AMREQUEST_HDR(rreq, lmt_type) = MPIDI_OFI_AM_LMT_UNPACK;
    MPIDIG_recv_setup(rreq);

    MPI_Aint pack_size = 100 * 1024;
    if (pack_size > MPIDI_OFI_global.max_msg_size) {
        pack_size = MPIDI_OFI_global.max_msg_size;
    }
    MPIDI_OFI_lmt_unpack_t *p = &MPIDI_OFI_AMREQUEST_HDR(rreq, lmt_u.unpack);
    p->src_rank = lmt_msg->src_rank;
    p->context_id = lmt_msg->context_id;
    p->src_offset = lmt_msg->src_offset;

    mpi_errno = MPIR_gpu_malloc_host(&p->unpack_buffer, pack_size);
    MPIR_ERR_CHECK(mpi_errno);

    MPI_Aint remain = MPIDIG_REQUEST(rreq, req->recv_async).in_data_sz;
    p->pack_size = pack_size;
    if (p->pack_size > remain) {
        p->pack_size = remain;
    }

    mpi_errno = MPIDI_OFI_do_rdma_read(p->unpack_buffer, lmt_msg->src_offset, p->pack_size,
                                       lmt_msg->context_id, lmt_msg->src_rank, rreq);
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_OFI_am_lmt_unpack_event(MPIR_Request * rreq)
{
    MPIDI_OFI_lmt_unpack_t *p = &MPIDI_OFI_AMREQUEST_HDR(rreq, lmt_u.unpack);
    int ret = MPIDIG_recv_copy_seg(p->unpack_buffer, p->pack_size, rreq);
    MPI_Aint remain = MPIDIG_REQUEST(rreq, req->recv_async).in_data_sz;
    MPI_Aint offset = MPIDIG_REQUEST(rreq, req->recv_async).offset;

    if (!ret && remain) {
        /* more to go */
        if (p->pack_size > remain) {
            p->pack_size = remain;
        }
        MPIDI_OFI_do_rdma_read(p->unpack_buffer, p->src_offset + offset, p->pack_size,
                               p->context_id, p->src_rank, rreq);
        return FALSE;
    } else {
        /* all done. */
        MPIR_gpu_free_host(p->unpack_buffer);
        return TRUE;
    }
}

MPL_STATIC_INLINE_PREFIX int do_long_am_recv(MPI_Aint in_data_sz, MPIR_Request * rreq,
                                             MPIDI_OFI_lmt_msg_payload_t * lmt_msg)
{
    int mpi_errno = MPI_SUCCESS;
    int num_iov = MPIDIG_get_recv_iov_count(rreq);
    if (num_iov > 1 && in_data_sz / num_iov < MPIR_CVAR_CH4_IOV_DENSITY_MIN) {
        /* noncontig data with mostly tiny segments */
        mpi_errno = do_long_am_recv_unpack(in_data_sz, rreq, lmt_msg);
    } else {
        int is_contig, is_gpu;
        void *p_data;
        MPI_Aint data_sz;
        MPIDIG_get_recv_data(&is_contig, &is_gpu, &p_data, &data_sz, rreq);

        if (is_gpu) {
            /* Use unpack for GPU buffer */
            mpi_errno = do_long_am_recv_unpack(in_data_sz, rreq, lmt_msg);
        } else if (is_contig) {
            mpi_errno = do_long_am_recv_contig(p_data, data_sz, in_data_sz, rreq, lmt_msg);
        } else {
            mpi_errno = do_long_am_recv_iov(p_data, data_sz, in_data_sz, rreq, lmt_msg);
        }
    }
    return mpi_errno;
}

#endif /* OFI_AM_EVENTS_H_INCLUDED */
