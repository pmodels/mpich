/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef OFI_AM_EVENTS_H_INCLUDED
#define OFI_AM_EVENTS_H_INCLUDED

#include "ofi_am_impl.h"

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

    uo_msg_len = sizeof(*uo_msg) + am_hdr->am_hdr_sz + am_hdr->data_sz;

    /* Allocate a new memory region to store this unordered message.
     * We are doing this because the original am_hdr comes from FI_MULTI_RECV
     * buffer, which may be reused soon by OFI. */
    uo_msg = MPL_malloc(uo_msg_len, MPL_MEM_BUFFER);
    if (uo_msg == NULL)
        return MPI_ERR_NO_MEM;

    packet_len = sizeof(*am_hdr) + am_hdr->am_hdr_sz + am_hdr->data_sz;
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

static inline int MPIDI_OFI_handle_short_am(MPIDI_OFI_am_header_t * msg_hdr)
{
    int mpi_errno = MPI_SUCCESS;
    void *p_data;
    void *in_data;
    size_t data_sz;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_OFI_HANDLE_SHORT_AM);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_OFI_HANDLE_SHORT_AM);

    /* note: msg_hdr + 1 points to the payload */
    p_data = in_data = (char *) (msg_hdr + 1) + msg_hdr->am_hdr_sz;
    data_sz = msg_hdr->data_sz;

    /* note: setting is_local, is_async, req to 0, 0, NULL */
    MPIDIG_global.target_msg_cbs[msg_hdr->handler_id] (msg_hdr->handler_id, (msg_hdr + 1),
                                                       p_data, data_sz, 0, 0, NULL);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_OFI_HANDLE_SHORT_AM);
    return mpi_errno;
}

static inline int MPIDI_OFI_handle_short_am_hdr(MPIDI_OFI_am_header_t * msg_hdr, void *am_hdr)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_OFI_HANDLE_SHORT_AM_HDR);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_OFI_HANDLE_SHORT_AM_HDR);

    MPIDIG_global.target_msg_cbs[msg_hdr->handler_id] (msg_hdr->handler_id, am_hdr,
                                                       NULL, 0, 0, 0, NULL);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_OFI_HANDLE_SHORT_AM_HDR);
    return mpi_errno;
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
            .addr = MPIDI_OFI_comm_to_phys(comm, src_rank, 0, 0),
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

static inline void do_long_am_recv(MPI_Aint in_data_sz, MPIR_Request * rreq,
                                   MPIDI_OFI_lmt_msg_payload_t * lmt_msg);
static inline int MPIDI_OFI_do_handle_long_am(MPIDI_OFI_am_header_t * msg_hdr,
                                              MPIDI_OFI_lmt_msg_payload_t * lmt_msg, void *am_hdr)
{
    int c, mpi_errno = MPI_SUCCESS;
    MPIR_Request *rreq = NULL;
    size_t in_data_sz;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_OFI_DO_HANDLE_LONG_AM);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_OFI_DO_HANDLE_LONG_AM);

    in_data_sz = msg_hdr->data_sz;
    /* note: setting is_local, is_async to 0, 1 */
    MPIDIG_global.target_msg_cbs[msg_hdr->handler_id] (msg_hdr->handler_id, am_hdr,
                                                       NULL, in_data_sz, 0, 1, &rreq);

    if (!rreq)
        goto fn_exit;

    MPIDI_OFI_am_clear_request(rreq);
    mpi_errno = MPIDI_OFI_am_init_request(NULL, 0, rreq);

    MPIR_ERR_CHECK(mpi_errno);

    MPIR_cc_incr(rreq->cc_ptr, &c);

    if (!in_data_sz) {
        MPIDIG_REQUEST(rreq, req->target_cmpl_cb) (rreq);
        MPID_Request_complete(rreq);
        goto fn_exit;
    }

    MPIDI_OFI_AMREQUEST_HDR(rreq, msg_hdr) = *msg_hdr;
    MPIDI_OFI_AMREQUEST_HDR(rreq, lmt_info) = *lmt_msg;
    MPIDI_OFI_AMREQUEST_HDR(rreq, rreq_ptr) = (void *) rreq;

    do_long_am_recv(in_data_sz, rreq, lmt_msg);
    /* completion in lmt event functions */

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_OFI_DO_HANDLE_LONG_AM);
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
    MPL_atomic_fetch_sub_int(&MPIDI_OFI_global.am_inflight_rma_send_mrs, 1);

    MPL_gpu_free_host(MPIDI_OFI_AMREQUEST_HDR(sreq, pack_buffer));

    handler_id = MPIDI_OFI_AMREQUEST_HDR(sreq, msg_hdr).handler_id;
    MPID_Request_complete(sreq);        /* FIXME: Should not call MPIDI in NM ? */
    mpi_errno = MPIDIG_global.origin_cbs[handler_id] (sreq);

    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_OFI_HANDLE_LMT_ACK);
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
        = MPIDI_OFI_comm_to_phys(MPIR_Process.comm_world, MPIR_Process.comm_world->rank, 0, 0);
    msg.pyld.sreq_ptr = sreq_ptr;
    MPIDI_OFI_CALL_RETRY_AM(fi_inject(MPIDI_OFI_global.ctx[0].tx, &msg, sizeof(msg),
                                      MPIDI_OFI_comm_to_phys(comm, rank, 0, 0)), inject);
  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_OFI_DISPATCH_ACK);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

/* internal routines */
static inline void do_long_am_recv_contig(void *p_data, MPI_Aint data_sz,
                                          MPI_Aint in_data_sz, MPIR_Request * rreq,
                                          MPIDI_OFI_lmt_msg_payload_t * lmt_msg)
{
    MPIDI_OFI_AMREQUEST_HDR(rreq, lmt_type) = MPIDI_OFI_AM_LMT_IOV;
    if (in_data_sz > data_sz) {
        rreq->status.MPI_ERROR = MPIDIG_ERR_TRUNCATE(data_sz, in_data_sz);
    }
    data_sz = MPL_MIN(data_sz, in_data_sz);
    MPIDI_OFI_AMREQUEST_HDR(rreq, lmt_u.lmt_cntr) =
        ((data_sz - 1) / MPIDI_OFI_global.max_msg_size) + 1;
    MPIDI_OFI_do_rdma_read(p_data, lmt_msg->src_offset, data_sz, lmt_msg->context_id,
                           lmt_msg->src_rank, rreq);
    MPIR_STATUS_SET_COUNT(rreq->status, data_sz);
}

static inline void do_long_am_recv_iov(struct iovec *iov, MPI_Aint iov_len,
                                       MPI_Aint in_data_sz, MPIR_Request * rreq,
                                       MPIDI_OFI_lmt_msg_payload_t * lmt_msg)
{
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
        MPIDI_OFI_do_rdma_read(iov[i].iov_base, lmt_msg->src_offset + done,
                               curr_len, lmt_msg->context_id, lmt_msg->src_rank, rreq);
        rem -= curr_len;
        done += curr_len;
    }

    if (rem) {
        rreq->status.MPI_ERROR = MPIDIG_ERR_TRUNCATE(done, in_data_sz);
    }

    MPIR_STATUS_SET_COUNT(rreq->status, done);
}

static inline void do_long_am_recv_unpack(MPI_Aint in_data_sz, MPIR_Request * rreq,
                                          MPIDI_OFI_lmt_msg_payload_t * lmt_msg)
{
    MPIDI_OFI_AMREQUEST_HDR(rreq, lmt_type) = MPIDI_OFI_AM_LMT_UNPACK;
    MPIDIG_recv_setup(rreq);

    MPI_Aint pack_size = 100 * 1024;
    if (pack_size > MPIDI_OFI_global.max_msg_size) {
        pack_size = MPIDI_OFI_global.max_msg_size;
    }
    MPIDI_OFI_lmt_unpack_t *p = &MPIDI_OFI_AMREQUEST_HDR(rreq, lmt_u.unpack);
    p->lmt_msg = lmt_msg;
    p->unpack_buffer = MPL_malloc(pack_size, MPL_MEM_BUFFER);

    MPI_Aint remain = MPIDIG_REQUEST(rreq, req->async).in_data_sz;
    p->pack_size = pack_size;
    if (p->pack_size > remain) {
        p->pack_size = remain;
    }

    MPIDI_OFI_do_rdma_read(p->unpack_buffer, lmt_msg->src_offset, p->pack_size, lmt_msg->context_id,
                           lmt_msg->src_rank, rreq);
}

MPL_STATIC_INLINE_PREFIX int MPIDI_OFI_am_lmt_unpack_event(MPIR_Request * rreq)
{
    MPIDI_OFI_lmt_unpack_t *p = &MPIDI_OFI_AMREQUEST_HDR(rreq, lmt_u.unpack);
    int ret = MPIDIG_recv_copy_seg(p->unpack_buffer, p->pack_size, rreq);
    MPI_Aint remain = MPIDIG_REQUEST(rreq, req->async).in_data_sz;
    MPI_Aint offset = MPIDIG_REQUEST(rreq, req->async).offset;

    if (!ret && remain) {
        /* more to go */
        if (p->pack_size > remain) {
            p->pack_size = remain;
        }
        MPIDI_OFI_lmt_msg_payload_t *lmt_msg = p->lmt_msg;
        MPIDI_OFI_do_rdma_read(p->unpack_buffer, lmt_msg->src_offset + offset, p->pack_size,
                               lmt_msg->context_id, lmt_msg->src_rank, rreq);
        return FALSE;
    } else {
        /* all done. */
        MPL_free(p->unpack_buffer);
        return TRUE;
    }
}

static inline void do_long_am_recv(MPI_Aint in_data_sz, MPIR_Request * rreq,
                                   MPIDI_OFI_lmt_msg_payload_t * lmt_msg)
{
    int num_iov = MPIDIG_get_recv_iov_count(rreq);
    if (num_iov > 1 && in_data_sz / num_iov < MPIR_CVAR_CH4_IOV_DENSITY_MIN) {
        /* noncontig data with mostly tiny segments */
        do_long_am_recv_unpack(in_data_sz, rreq, lmt_msg);
    } else {
        int is_contig;
        void *p_data;
        MPI_Aint data_sz;
        MPIDIG_get_recv_data(&is_contig, &p_data, &data_sz, rreq);
        if (is_contig) {
            do_long_am_recv_contig(p_data, data_sz, in_data_sz, rreq, lmt_msg);
        } else {
            do_long_am_recv_iov(p_data, data_sz, in_data_sz, rreq, lmt_msg);
        }
    }
}

#endif /* OFI_AM_EVENTS_H_INCLUDED */
