/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef OFI_AM_IMPL_H_INCLUDED
#define OFI_AM_IMPL_H_INCLUDED

#include "ofi_impl.h"
#include "mpidu_genq.h"

#define MPIDI_OFI_AM_ATTR__RDMA (1 << MPIDIG_AM_ATTR_TRANSPORT_SHIFT)

int MPIDI_OFI_am_repost_buffer(int vni, int am_idx);
MPL_STATIC_INLINE_PREFIX int MPIDI_OFI_progress_do_queue(int vni_idx);

/* active message ordering --
 * FI_MULTI_RECV doesn't guarantee the ordering of messages. Thus we have to
 * set and check the seqno and potentially enqueue the unordered messages.
 * This is necessary because CH4 message queue relies on transport observing the * message order.
 *
 * Each time we send an active message, we increment and embed a send_seqno in
 * the msg_hdr. At the receiver end we maintain a recv_seqno and match against
 * the send_seqno. If they are out-of-order, we postpone the message by
 * enqueuing to MPIDI_OFI_global.am_unordered_msgs. The matching seqno is
 * similar to how TCP protocol works.
 */

MPL_STATIC_INLINE_PREFIX uint16_t MPIDI_OFI_am_fetch_incr_send_seqno(int vni, fi_addr_t addr)
{
    uint64_t id = addr;
    uint16_t seq, old_seq;
    void *ret;

    ret = MPIDIU_map_lookup(MPIDI_OFI_global.per_vni[vni].am_send_seq_tracker, id);
    if (ret == MPIDIU_MAP_NOT_FOUND)
        old_seq = 0;
    else
        old_seq = (uint16_t) (uintptr_t) ret;

    seq = old_seq + 1;
    MPIDIU_map_update(MPIDI_OFI_global.per_vni[vni].am_send_seq_tracker, id,
                      (void *) (uintptr_t) seq, MPL_MEM_OTHER);

    return old_seq;
}

MPL_STATIC_INLINE_PREFIX uint16_t MPIDI_OFI_am_get_next_recv_seqno(int vni, fi_addr_t addr)
{
    uint64_t id = addr;
    void *r;

    r = MPIDIU_map_lookup(MPIDI_OFI_global.per_vni[vni].am_recv_seq_tracker, id);
    if (r == MPIDIU_MAP_NOT_FOUND) {
        MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_GENERAL, VERBOSE,
                        (MPL_DBG_FDEST, "First time adding recv seqno addr=%" PRIx64 "\n", addr));
        MPIDIU_map_set(MPIDI_OFI_global.per_vni[vni].am_recv_seq_tracker, id, 0, MPL_MEM_OTHER);
        return 0;
    } else {
        return (uint16_t) (uintptr_t) r;
    }
}

MPL_STATIC_INLINE_PREFIX void MPIDI_OFI_am_set_next_recv_seqno(int vni, fi_addr_t addr,
                                                               uint16_t seqno)
{
    uint64_t id = addr;

    MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_GENERAL, VERBOSE,
                    (MPL_DBG_FDEST, "Next recv seqno=%d addr=%" PRIx64 "\n", seqno, addr));

    MPIDIU_map_update(MPIDI_OFI_global.per_vni[vni].am_recv_seq_tracker, id,
                      (void *) (uintptr_t) seqno, MPL_MEM_OTHER);
}

MPL_STATIC_INLINE_PREFIX int MPIDI_OFI_am_enqueue_unordered_msg(int vni,
                                                                const MPIDI_OFI_am_header_t *
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

    DL_APPEND(MPIDI_OFI_global.per_vni[vni].am_unordered_msgs, uo_msg);

    return MPI_SUCCESS;
}

/* Find and dequeue a message that matches (comm, src_rank, seqno), then return it.
 * Caller must free the returned pointer. */
MPL_STATIC_INLINE_PREFIX MPIDI_OFI_am_unordered_msg_t
    * MPIDI_OFI_am_claim_unordered_msg(int vni, fi_addr_t addr, uint16_t seqno)
{
    MPIDI_OFI_am_unordered_msg_t *uo_msg;

    /* Future optimization note:
     * Currently we are doing linear search every time, assuming that the number of items
     * in the queue is extremely small.
     * If it's not the case, we should consider using better data structure and algorithm
     * to look up. */
    DL_FOREACH(MPIDI_OFI_global.per_vni[vni].am_unordered_msgs, uo_msg) {
        if (uo_msg->am_hdr.fi_src_addr == addr && uo_msg->am_hdr.seqno == seqno) {
            MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_GENERAL, TERSE,
                            (MPL_DBG_FDEST,
                             "Found unordered message in the queue: addr=%" PRIx64 ", seqno=%d\n",
                             addr, seqno));
            DL_DELETE(MPIDI_OFI_global.per_vni[0].am_unordered_msgs, uo_msg);
            return uo_msg;
        }
    }

    return NULL;
}

/*
  Per-object lock for OFI

  * When calling OFI function MPIDI_OFI_THREAD_FI_MUTEX must be held.
  * When being called from the MPI layer (app), we must grab the lock.
    This is the case for regular (non-reply) functions such as am_isend.
  * When being called from callback function or progress engine, we must
    not grab the lock because the progress engine is already holding the lock.
    This is the case for reply functions such as am_isend_reply.
*/
#define MPIDI_OFI_CALL_RETRY_AM(FUNC,STR)                               \
    do {                                                                \
        ssize_t _ret;                                                   \
        do {                                                            \
            _ret = FUNC;                                                \
            if (likely(_ret==0)) break;                                  \
            MPIR_ERR_##CHKANDJUMP4(_ret != -FI_EAGAIN,                  \
                                   mpi_errno,                           \
                                   MPI_ERR_OTHER,                       \
                                   "**ofid_"#STR,                        \
                                   "**ofid_"#STR" %s %d %s %s",          \
                                   __SHORT_FILE__,                      \
                                   __LINE__,                            \
                                   __func__,                              \
                                   fi_strerror(-_ret));                 \
            mpi_errno = MPIDI_OFI_progress_do_queue(0 /* vni_idx */);    \
            if (mpi_errno != MPI_SUCCESS)                                \
                MPIR_ERR_CHECK(mpi_errno);                               \
        } while (_ret == -FI_EAGAIN);                                   \
    } while (0)

#define MPIDI_OFI_AM_FREE_REQ_HDR(req_hdr, vni) \
    do { \
        if (req_hdr) { \
            MPIDU_genq_private_pool_free_cell(MPIDI_OFI_global.per_vni[vni].am_hdr_buf_pool, req_hdr); \
            req_hdr = NULL; \
        } \
    } while (0)

MPL_STATIC_INLINE_PREFIX void MPIDI_OFI_am_clear_request(MPIR_Request * req)
{
    MPIR_FUNC_ENTER;

    int vni = MPIDI_Request_get_vci(req);
    MPIDI_OFI_AM_FREE_REQ_HDR(MPIDI_OFI_AMREQUEST(req, sreq_hdr), vni);
    MPIDI_OFI_AM_FREE_REQ_HDR(MPIDI_OFI_AMREQUEST(req, rreq_hdr), vni);

    MPIR_FUNC_EXIT;
}

/* We call this at the point of sending am message, e.g.
 *     MPIDI_OFI_do_am_isend_{eager,pipeline,rdma_read}
 */
MPL_STATIC_INLINE_PREFIX int MPIDI_OFI_am_init_sreq(const void *am_hdr, size_t am_hdr_sz,
                                                    MPIR_Request * sreq)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_OFI_am_request_header_t *sreq_hdr;
    MPIR_FUNC_ENTER;

    MPIR_Assert(am_hdr_sz < (1ULL << MPIDI_OFI_AM_HDR_SZ_BITS));

    if (MPIDI_OFI_AMREQUEST(sreq, sreq_hdr) == NULL) {
        int vni = MPIDI_Request_get_vci(sreq);
        MPIDU_genq_private_pool_alloc_cell(MPIDI_OFI_global.per_vni[vni].am_hdr_buf_pool,
                                           (void **) &sreq_hdr);
        MPIR_Assert(sreq_hdr);
        MPIDI_OFI_AMREQUEST(sreq, sreq_hdr) = sreq_hdr;

        sreq_hdr->am_hdr = (void *) &sreq_hdr->am_hdr_buf[0];
        sreq_hdr->am_hdr_sz = am_hdr_sz;
        sreq_hdr->pack_buffer = NULL;
    } else {
        sreq_hdr = MPIDI_OFI_AMREQUEST(sreq, sreq_hdr);
    }

    if (am_hdr) {
        MPIR_Memcpy(sreq_hdr->am_hdr, am_hdr, am_hdr_sz);
    }

    MPIR_FUNC_EXIT;
    return mpi_errno;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_OFI_am_init_rreq(MPIR_Request * rreq)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_ENTER;

    if (MPIDI_OFI_AMREQUEST(rreq, rreq_hdr) == NULL) {
        int vni = MPIDI_Request_get_vci(rreq);
        MPIDI_OFI_am_request_header_t *rreq_hdr;
        MPIDU_genq_private_pool_alloc_cell(MPIDI_OFI_global.per_vni[vni].am_hdr_buf_pool,
                                           (void **) &rreq_hdr);
        MPIR_Assert(rreq_hdr);
        MPIDI_OFI_AMREQUEST(rreq, rreq_hdr) = rreq_hdr;
        rreq_hdr->pack_buffer = NULL;
    }

    MPIR_FUNC_EXIT;
    return mpi_errno;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_OFI_am_isend_long(int rank, MPIR_Comm * comm, int handler_id,
                                                     const void *data, MPI_Aint data_sz,
                                                     MPIR_Request * sreq, int vni_src, int vni_dst)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_OFI_am_header_t *msg_hdr;
    MPIDI_OFI_lmt_msg_payload_t *lmt_info;
    int nic = 0;
    int ctx_idx = MPIDI_OFI_get_ctx_index(comm, vni_src, nic);
    fi_addr_t dst_addr = MPIDI_OFI_comm_to_phys(comm, rank, nic, vni_src, vni_dst);

    MPIR_FUNC_ENTER;

    MPI_Aint am_hdr_sz = MPIDI_OFI_AM_SREQ_HDR(sreq, am_hdr_sz);
    MPI_Aint total_msg_sz = sizeof(*msg_hdr) + am_hdr_sz + sizeof(*lmt_info);

    MPIR_Assert(handler_id < (1 << MPIDI_OFI_AM_HANDLER_ID_BITS));
    MPIR_Assert((uint64_t) comm->rank < (1ULL << MPIDI_OFI_AM_RANK_BITS));
    MPIR_Assert(total_msg_sz <= MPIDI_OFI_AM_MAX_MSG_SIZE);

    msg_hdr = &MPIDI_OFI_AM_SREQ_HDR(sreq, msg_hdr);
    msg_hdr->handler_id = handler_id;
    msg_hdr->am_hdr_sz = am_hdr_sz;
    msg_hdr->payload_sz = 0;    /* LMT info sent as header */
    msg_hdr->am_type = MPIDI_AMTYPE_RDMA_READ;
    msg_hdr->vni_src = vni_src;
    msg_hdr->vni_dst = vni_dst;
    msg_hdr->seqno = MPIDI_OFI_am_fetch_incr_send_seqno(vni_src, dst_addr);
    msg_hdr->fi_src_addr = MPIDI_OFI_rank_to_phys(MPIR_Process.rank, nic, vni_src, vni_src);

    lmt_info = (void *) ((char *) msg_hdr + sizeof(MPIDI_OFI_am_header_t) + am_hdr_sz);
    lmt_info->context_id = comm->context_id;
    lmt_info->src_rank = comm->rank;
    lmt_info->src_offset =
        !MPIDI_OFI_ENABLE_MR_VIRT_ADDRESS ? (uint64_t) 0 : (uint64_t) (uintptr_t) data;

    lmt_info->sreq_ptr = sreq;
    if (!MPIDI_OFI_ENABLE_MR_PROV_KEY) {
        lmt_info->rma_key =
            MPIDI_OFI_mr_key_alloc(MPIDI_OFI_LOCAL_MR_KEY, MPIDI_OFI_INVALID_MR_KEY);
        MPIR_ERR_CHKANDJUMP(lmt_info->rma_key == MPIDI_OFI_INVALID_MR_KEY, mpi_errno,
                            MPI_ERR_OTHER, "**ofid_mr_key");
    } else {
        lmt_info->rma_key = 0;
    }
    lmt_info->reg_sz = data_sz;

    MPIR_cc_inc(sreq->cc_ptr);  /* send completion */
    MPIR_cc_inc(sreq->cc_ptr);  /* lmt ack handler */
    MPIDI_OFI_CALL(fi_mr_reg(MPIDI_OFI_global.ctx[ctx_idx].domain,
                             data,
                             data_sz,
                             FI_REMOTE_READ,
                             0ULL,
                             lmt_info->rma_key,
                             0ULL, &MPIDI_OFI_AM_SREQ_HDR(sreq, lmt_mr), NULL), mr_reg);
    mpi_errno = MPIDI_OFI_mr_bind(MPIDI_OFI_global.prov_use[0], MPIDI_OFI_AM_SREQ_HDR(sreq, lmt_mr),
                                  MPIDI_OFI_global.ctx[ctx_idx].ep, NULL);
    MPIR_ERR_CHECK(mpi_errno);

    MPIDI_OFI_global.per_vni[vni_src].am_inflight_rma_send_mrs += 1;

    if (MPIDI_OFI_ENABLE_MR_PROV_KEY) {
        /* MR_BASIC */
        lmt_info->rma_key = fi_mr_key(MPIDI_OFI_AM_SREQ_HDR(sreq, lmt_mr));
    }

    MPIDI_OFI_AMREQUEST(sreq, event_id) = MPIDI_OFI_EVENT_AM_SEND_RDMA;
    MPIDI_OFI_CALL_RETRY_AM(fi_send(MPIDI_OFI_global.ctx[ctx_idx].tx, msg_hdr, total_msg_sz, NULL,
                                    dst_addr, &MPIDI_OFI_AMREQUEST(sreq, context)), send);
  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_OFI_am_isend_short(int rank, MPIR_Comm * comm, int handler_id,
                                                      const void *buf, MPI_Aint count,
                                                      MPI_Datatype datatype, MPI_Aint data_sz,
                                                      bool need_packing, MPIR_Request * sreq,
                                                      int vni_src, int vni_dst)
{
    int mpi_errno = MPI_SUCCESS;
    int nic = 0;
    int ctx_idx = MPIDI_OFI_get_ctx_index(comm, vni_src, nic);
    fi_addr_t dst_addr = MPIDI_OFI_comm_to_phys(comm, rank, nic, vni_src, vni_dst);

    MPIR_FUNC_ENTER;

    MPIR_Assert(handler_id < (1 << MPIDI_OFI_AM_HANDLER_ID_BITS));
    MPIR_Assert(data_sz < (1ULL << MPIDI_OFI_AM_PAYLOAD_SZ_BITS));
    MPIR_Assert((uint64_t) comm->rank < (1ULL << MPIDI_OFI_AM_RANK_BITS));

    MPI_Aint am_hdr_sz = MPIDI_OFI_AM_SREQ_HDR(sreq, am_hdr_sz);

    MPIDI_OFI_am_header_t *msg_hdr;
    if (!MPIDI_OFI_AM_SREQ_HDR(sreq, pack_buffer)) {
        msg_hdr = &MPIDI_OFI_AM_SREQ_HDR(sreq, msg_hdr);
    } else {
        msg_hdr = MPIDI_OFI_AM_SREQ_HDR(sreq, pack_buffer);
        /* The completion callback need the handler_id in the same place
         * FIXME: maybe we should have a separate msg_hdr pointer just like isend_pipeline? */
        MPIDI_OFI_AM_SREQ_HDR(sreq, msg_hdr).handler_id = handler_id;
    }

    msg_hdr->handler_id = handler_id;
    msg_hdr->am_hdr_sz = MPIDI_OFI_AM_SREQ_HDR(sreq, am_hdr_sz);
    msg_hdr->payload_sz = data_sz;
    msg_hdr->am_type = MPIDI_AMTYPE_SHORT;
    msg_hdr->vni_src = vni_src;
    msg_hdr->vni_dst = vni_dst;
    msg_hdr->seqno = MPIDI_OFI_am_fetch_incr_send_seqno(vni_src, dst_addr);
    msg_hdr->fi_src_addr = MPIDI_OFI_rank_to_phys(MPIR_Process.rank, nic, vni_src, vni_src);

    MPIR_cc_inc(sreq->cc_ptr);
    MPIDI_OFI_AMREQUEST(sreq, event_id) = MPIDI_OFI_EVENT_AM_SEND;

    void *p_am_hdr, *p_am_data;
    p_am_hdr = (char *) msg_hdr + sizeof(MPIDI_OFI_am_header_t);
    p_am_data = (char *) p_am_hdr + am_hdr_sz;

    if (p_am_hdr != MPIDI_OFI_AM_SREQ_HDR(sreq, am_hdr)) {
        MPIR_Memcpy(p_am_hdr, MPIDI_OFI_AM_SREQ_HDR(sreq, am_hdr), am_hdr_sz);
    }

    MPI_Aint packed_size;
    mpi_errno = MPIR_Typerep_pack(buf, count, datatype, 0, p_am_data, data_sz, &packed_size,
                                  MPIR_TYPEREP_FLAG_NONE);
    MPIR_ERR_CHECK(mpi_errno);
    MPIR_Assert(packed_size == data_sz);

    MPI_Aint total_msg_sz = sizeof(MPIDI_OFI_am_header_t) + am_hdr_sz + data_sz;
    MPIDI_OFI_CALL_RETRY_AM(fi_send(MPIDI_OFI_global.ctx[ctx_idx].tx, msg_hdr, total_msg_sz,
                                    NULL, dst_addr, &MPIDI_OFI_AMREQUEST(sreq, context)), send);

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_OFI_am_isend_pipeline(int rank, MPIR_Comm * comm, int handler_id,
                                                         const void *buf, MPI_Aint count,
                                                         MPI_Datatype datatype, MPI_Aint offset,
                                                         int need_packing, MPIR_Request * sreq,
                                                         MPIDI_OFI_am_send_pipeline_request_t *
                                                         send_req, int vni_src, int vni_dst)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_OFI_am_header_t *msg_hdr;
    int nic = 0;
    int ctx_idx = MPIDI_OFI_get_ctx_index(comm, vni_src, nic);
    fi_addr_t dst_addr = MPIDI_OFI_comm_to_phys(comm, rank, nic, vni_src, vni_dst);

    MPIR_FUNC_ENTER;

    MPIR_Assert(handler_id < (1 << MPIDI_OFI_AM_HANDLER_ID_BITS));
    MPIR_Assert((uint64_t) comm->rank < (1ULL << MPIDI_OFI_AM_RANK_BITS));

    msg_hdr = send_req->msg_hdr;
    MPI_Aint am_hdr_sz = 0;
    if (MPIDIG_am_send_async_get_offset(sreq) == 0) {
        am_hdr_sz = MPIDI_OFI_AM_SREQ_HDR(sreq, am_hdr_sz);
    }
    MPI_Aint seg_sz = MPIDI_OFI_DEFAULT_SHORT_SEND_SIZE - sizeof(MPIDI_OFI_am_header_t) - am_hdr_sz;
    seg_sz = MPL_MIN(seg_sz, MPIDIG_am_send_async_get_data_sz_left(sreq));
    MPIR_Assert(seg_sz < (1ULL << MPIDI_OFI_AM_PAYLOAD_SZ_BITS));

    msg_hdr = (MPIDI_OFI_am_header_t *) send_req->msg_hdr;
    msg_hdr->handler_id = handler_id;
    msg_hdr->am_hdr_sz = am_hdr_sz;
    msg_hdr->payload_sz = seg_sz;
    msg_hdr->am_type = MPIDI_AMTYPE_PIPELINE;
    msg_hdr->vni_src = vni_src;
    msg_hdr->vni_dst = vni_dst;
    msg_hdr->seqno = MPIDI_OFI_am_fetch_incr_send_seqno(vni_src, dst_addr);
    msg_hdr->fi_src_addr = MPIDI_OFI_rank_to_phys(MPIR_Process.rank, nic, vni_src, vni_src);

    MPIR_cc_inc(sreq->cc_ptr);
    send_req->event_id = MPIDI_OFI_EVENT_AM_SEND_PIPELINE;

    MPI_Aint total_msg_sz = sizeof(*msg_hdr) + am_hdr_sz + seg_sz;
    MPIR_Memcpy(send_req->am_hdr, MPIDI_OFI_AM_SREQ_HDR(sreq, am_hdr), am_hdr_sz);
    MPI_Aint packed_size;
    mpi_errno = MPIR_Typerep_pack(buf, count, datatype, offset,
                                  send_req->am_data, seg_sz, &packed_size, MPIR_TYPEREP_FLAG_NONE);
    MPIR_ERR_CHECK(mpi_errno);
    MPIR_Assert(packed_size == seg_sz);

    MPIDI_OFI_CALL_RETRY_AM(fi_send(MPIDI_OFI_global.ctx[ctx_idx].tx, msg_hdr, total_msg_sz,
                                    NULL, dst_addr, &send_req->context), send);

    MPIDIG_am_send_async_issue_seg(sreq, seg_sz);
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#define DEFER_AM_SEND(am_op) \
    do { \
        MPIDI_OFI_AMREQUEST(sreq, deferred_req) = (MPIDI_OFI_deferred_am_isend_req_t *) MPL_malloc(sizeof(MPIDI_OFI_deferred_am_isend_req_t), MPL_MEM_OTHER); \
        MPIR_Assert(MPIDI_OFI_AMREQUEST(sreq, deferred_req)); \
        MPIDI_OFI_AMREQUEST(sreq, deferred_req)->op = am_op; \
        MPIDI_OFI_AMREQUEST(sreq, deferred_req)->rank = rank; \
        MPIDI_OFI_AMREQUEST(sreq, deferred_req)->comm = comm; \
        MPIDI_OFI_AMREQUEST(sreq, deferred_req)->handler_id = handler_id; \
        MPIDI_OFI_AMREQUEST(sreq, deferred_req)->buf = buf; \
        MPIDI_OFI_AMREQUEST(sreq, deferred_req)->count = count; \
        MPIDI_OFI_AMREQUEST(sreq, deferred_req)->datatype = datatype; \
        MPIDI_OFI_AMREQUEST(sreq, deferred_req)->sreq = sreq; \
        MPIDI_OFI_AMREQUEST(sreq, deferred_req)->data_sz = data_sz; \
        MPIDI_OFI_AMREQUEST(sreq, deferred_req)->need_packing = need_packing; \
        MPIDI_OFI_AMREQUEST(sreq, deferred_req)->vni_src = vni_src; \
        MPIDI_OFI_AMREQUEST(sreq, deferred_req)->vni_dst = vni_dst; \
        DL_APPEND(MPIDI_OFI_global.per_vni[vni_src].deferred_am_isend_q, MPIDI_OFI_AMREQUEST(sreq, deferred_req)); \
    } while (0)

#define ALLOCATE_PACK_BUFFER_OR_DEFER(pack_buffer) \
    do { \
        MPIDU_genq_private_pool_alloc_cell(MPIDI_global.per_vci[vni_src].pack_buf_pool, (void **) &(pack_buffer)); \
        if (pack_buffer == NULL) { \
            if (!issue_deferred) { \
                goto fn_deferred; \
            } else { \
                goto fn_exit; \
            } \
        } \
    } while (0)

MPL_STATIC_INLINE_PREFIX int MPIDI_OFI_do_am_isend_eager(int rank, MPIR_Comm * comm,
                                                         int handler_id, const void *am_hdr,
                                                         size_t am_hdr_sz, const void *buf,
                                                         size_t count, MPI_Datatype datatype,
                                                         MPIR_Request * sreq, bool issue_deferred,
                                                         int vni_src, int vni_dst)
{
    int mpi_errno = MPI_SUCCESS;
    MPI_Aint data_sz;
    bool need_packing = false;

    MPIR_FUNC_ENTER;

    /* NOTE: issue_deferred is set to true when progress use this function for deferred operations.
     * we need to skip some code path in the scenario. Also am_hdr and am_hdr_sz are ignored when
     * issue_deferred is set to true. They should have been saved in the request. */

    if (!issue_deferred) {
        mpi_errno = MPIDI_OFI_am_init_sreq(am_hdr, am_hdr_sz, sreq);
        MPIR_ERR_CHECK(mpi_errno);

        int dt_contig;
        MPIDI_Datatype_check_contig_size(datatype, count, dt_contig, data_sz);

        need_packing = dt_contig ? false : true;

        MPL_pointer_attr_t attr;
        MPIR_GPU_query_pointer_attr(buf, &attr);
        if (attr.type == MPL_GPU_POINTER_DEV && !MPIDI_OFI_ENABLE_HMEM) {
            /* Force packing of GPU buffer in host memory */
            need_packing = true;
        }
    } else {
        data_sz = MPIDI_OFI_AMREQUEST(sreq, deferred_req)->data_sz;
        need_packing = MPIDI_OFI_AMREQUEST(sreq, deferred_req)->need_packing;
    }

    if (!issue_deferred && MPIDI_OFI_global.per_vni[vni_src].deferred_am_isend_q) {
        /* if the deferred queue is not empty, all new ops must be deferred to maintain ordering */
        goto fn_deferred;
    }

    MPIR_Assert(data_sz <= MPIDI_OFI_DEFAULT_SHORT_SEND_SIZE);

    MPI_Aint total_msg_sz = sizeof(MPIDI_OFI_am_header_t) + am_hdr_sz + data_sz;
    if (total_msg_sz > MPIDI_OFI_AM_MAX_MSG_SIZE) {
        ALLOCATE_PACK_BUFFER_OR_DEFER(MPIDI_OFI_AM_SREQ_HDR(sreq, pack_buffer));
    }

    mpi_errno = MPIDI_OFI_am_isend_short(rank, comm, handler_id, buf, count, datatype,
                                         data_sz, need_packing, sreq, vni_src, vni_dst);
    MPIR_ERR_CHECK(mpi_errno);

    if (issue_deferred) {
        DL_DELETE(MPIDI_OFI_global.per_vni[vni_src].deferred_am_isend_q,
                  MPIDI_OFI_AMREQUEST(sreq, deferred_req));
        MPL_free(MPIDI_OFI_AMREQUEST(sreq, deferred_req));
    }

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
  fn_deferred:
    DEFER_AM_SEND(MPIDI_OFI_DEFERRED_AM_OP__ISEND_EAGER);
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_OFI_do_emulated_inject(MPIR_Comm * comm, fi_addr_t addr,
                                                          const MPIDI_OFI_am_header_t * msg_hdrp,
                                                          const void *am_hdr, size_t am_hdr_sz,
                                                          int vni_src, int vni_dst)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Request *sreq;
    char *ibuf;
    size_t len;
    int nic = 0;
    int ctx_idx = MPIDI_OFI_get_ctx_index(comm, vni_src, nic);

    MPIDI_CH4_REQUEST_CREATE(sreq, MPIR_REQUEST_KIND__SEND, vni_src, 1);
    MPIR_ERR_CHKANDSTMT((sreq) == NULL, mpi_errno, MPIX_ERR_NOREQ, goto fn_fail, "**nomemreq");
    len = am_hdr_sz + sizeof(*msg_hdrp);
    ibuf = (char *) MPL_malloc(len, MPL_MEM_BUFFER);
    MPIR_Assert(ibuf);
    memcpy(ibuf, msg_hdrp, sizeof(*msg_hdrp));
    memcpy(ibuf + sizeof(*msg_hdrp), am_hdr, am_hdr_sz);

    MPIDI_OFI_REQUEST(sreq, event_id) = MPIDI_OFI_EVENT_INJECT_EMU;
    MPIDI_OFI_REQUEST(sreq, util.inject_buf) = ibuf;
    MPIDI_OFI_global.per_vni[vni_src].am_inflight_inject_emus += 1;

    MPIDI_OFI_CALL_RETRY_AM(fi_send(MPIDI_OFI_global.ctx[ctx_idx].tx, ibuf, len,
                                    NULL /* desc */ , addr, &(MPIDI_OFI_REQUEST(sreq, context))),
                            send);
  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_OFI_do_inject(int rank,
                                                 MPIR_Comm * comm,
                                                 int handler_id, const void *am_hdr,
                                                 size_t am_hdr_sz, int vni_src, int vni_dst)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_OFI_am_header_t msg_hdr;
    char *buff;
    size_t buff_len;
    int nic = 0;
    int ctx_idx = MPIDI_OFI_get_ctx_index(comm, vni_src, nic);
    fi_addr_t dst_addr = MPIDI_OFI_comm_to_phys(comm, rank, nic, vni_src, vni_dst);
    MPIR_CHKLMEM_DECL(1);

    MPIR_FUNC_ENTER;

    MPIR_Assert(handler_id < (1 << MPIDI_OFI_AM_HANDLER_ID_BITS));
    MPIR_Assert(am_hdr_sz < (1ULL << MPIDI_OFI_AM_HDR_SZ_BITS));

    msg_hdr.handler_id = handler_id;
    msg_hdr.am_hdr_sz = am_hdr_sz;
    msg_hdr.payload_sz = 0;
    msg_hdr.am_type = MPIDI_AMTYPE_SHORT_HDR;
    msg_hdr.vni_src = vni_src;
    msg_hdr.vni_dst = vni_dst;
    msg_hdr.seqno = MPIDI_OFI_am_fetch_incr_send_seqno(vni_src, dst_addr);
    msg_hdr.fi_src_addr = MPIDI_OFI_rank_to_phys(MPIR_Process.rank, nic, vni_src, vni_src);

    MPIR_Assert((uint64_t) comm->rank < (1ULL << MPIDI_OFI_AM_RANK_BITS));

    if (unlikely(am_hdr_sz + sizeof(msg_hdr) > MPIDI_OFI_global.max_buffered_send)) {
        mpi_errno = MPIDI_OFI_do_emulated_inject(comm, dst_addr, &msg_hdr,
                                                 am_hdr, am_hdr_sz, vni_src, vni_dst);
        goto fn_exit;
    }

    buff_len = sizeof(msg_hdr) + am_hdr_sz;
    MPIR_CHKLMEM_MALLOC(buff, char *, buff_len, mpi_errno, "buff", MPL_MEM_BUFFER);
    memcpy(buff, &msg_hdr, sizeof(msg_hdr));
    memcpy(buff + sizeof(msg_hdr), am_hdr, am_hdr_sz);

    MPIDI_OFI_CALL_RETRY_AM(fi_inject(MPIDI_OFI_global.ctx[ctx_idx].tx, buff, buff_len, dst_addr),
                            inject);

  fn_exit:
    MPIR_CHKLMEM_FREEALL();
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_OFI_do_am_isend_pipeline(int rank, MPIR_Comm * comm,
                                                            int handler_id, const void *am_hdr,
                                                            size_t am_hdr_sz, const void *buf,
                                                            size_t count, MPI_Datatype datatype,
                                                            MPIR_Request * sreq, MPI_Aint data_sz,
                                                            bool issue_deferred,
                                                            int vni_src, int vni_dst)
{
    int dt_contig, mpi_errno = MPI_SUCCESS;
    MPI_Aint offset;
    bool need_packing = false;
    MPIDI_OFI_am_send_pipeline_request_t *send_req;

    MPIR_FUNC_ENTER;

    /* NOTE: issue_deferred is set to true when progress use this function for deferred operations.
     * we need to skip some code path in the scenario. Also am_hdr, am_hdr_sz and data_sz are
     * ignored when issue_deferred is set to true. They should have been saved in the request. */

    if (!issue_deferred) {
        mpi_errno = MPIDI_OFI_am_init_sreq(am_hdr, am_hdr_sz, sreq);
        MPIR_ERR_CHECK(mpi_errno);

        MPIDI_Datatype_check_contig(datatype, dt_contig);

        MPIDIG_am_send_async_init(sreq, datatype, data_sz);

        need_packing = dt_contig ? false : true;

        MPL_pointer_attr_t attr;
        MPIR_GPU_query_pointer_attr(buf, &attr);
        if (attr.type == MPL_GPU_POINTER_DEV && !MPIDI_OFI_ENABLE_HMEM) {
            /* Force packing of GPU buffer in host memory */
            need_packing = true;
        }
        offset = 0;
    } else {
        /* we are issuing deferred op. If the offset == 0, this is the first segment and we need to
         * send am_hdr */
        offset = MPIDIG_am_send_async_get_offset(sreq);
        need_packing = MPIDI_OFI_AMREQUEST(sreq, deferred_req)->need_packing;
    }

    if (!issue_deferred && MPIDI_OFI_global.per_vni[vni_src].deferred_am_isend_q) {
        /* if the deferred queue is not empty, all new ops must be deferred to maintain ordering */
        goto fn_deferred;
    }

    void *pack_buffer = NULL;
    ALLOCATE_PACK_BUFFER_OR_DEFER(pack_buffer);

    MPIDU_genq_private_pool_alloc_cell(MPIDI_OFI_global.per_vni[vni_src].am_hdr_buf_pool,
                                       (void **) &send_req);
    MPIR_Assert(send_req);
    send_req->sreq = sreq;
    send_req->pack_buffer = pack_buffer;
    if (!pack_buffer) {
        send_req->msg_hdr = &send_req->msg_hdr_data;
        send_req->am_hdr = NULL;
        send_req->am_data = NULL;
    } else {
        send_req->msg_hdr = pack_buffer;
        send_req->am_hdr = (char *) pack_buffer + sizeof(MPIDI_OFI_am_header_t);
        if (offset == 0) {
            send_req->am_data = (char *) send_req->am_hdr + am_hdr_sz;
        } else {
            send_req->am_data = (char *) send_req->am_hdr;
        }
    }

    mpi_errno = MPIDI_OFI_am_isend_pipeline(rank, comm, handler_id,
                                            buf, count, datatype, offset, need_packing,
                                            sreq, send_req, vni_src, vni_dst);

    /* if there IS MORE DATA to be sent and we ARE NOT called for issue deferred op, enqueue.
     * if there NO MORE DATA and we ARE called for issuing deferred op, pipeline is done, dequeue
     * skip for all other cases */
    if (!MPIDIG_am_send_async_is_done(sreq)) {
        if (!issue_deferred) {
            goto fn_deferred;
        }
    } else if (issue_deferred) {
        DL_DELETE(MPIDI_OFI_global.per_vni[vni_src].deferred_am_isend_q,
                  MPIDI_OFI_AMREQUEST(sreq, deferred_req));
        MPL_free(MPIDI_OFI_AMREQUEST(sreq, deferred_req));
    }

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
  fn_deferred:
    DEFER_AM_SEND(MPIDI_OFI_DEFERRED_AM_OP__ISEND_PIPELINE);
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_OFI_do_am_isend_rdma_read(int rank, MPIR_Comm * comm,
                                                             int handler_id, const void *am_hdr,
                                                             size_t am_hdr_sz, const void *buf,
                                                             size_t count, MPI_Datatype datatype,
                                                             MPIR_Request * sreq,
                                                             bool issue_deferred,
                                                             int vni_src, int vni_dst)
{
    int dt_contig, mpi_errno = MPI_SUCCESS;
    char *send_buf;
    MPI_Aint data_sz;
    MPI_Aint dt_true_lb, last;
    bool need_packing = false;

    MPIR_FUNC_ENTER;

    /* NOTE: issue_deferred is set to true when progress use this function for deferred operations.
     * we need to skip some code path in the scenario. Also am_hdr and am_hdr_sz are ignored when
     * issue_deferred is set to true. They should have been saved in the request. */

    if (!issue_deferred) {
        mpi_errno = MPIDI_OFI_am_init_sreq(am_hdr, am_hdr_sz, sreq);
        MPIR_ERR_CHECK(mpi_errno);

        MPIDI_Datatype_check_contig_size(datatype, count, dt_contig, data_sz);

        need_packing = dt_contig ? false : true;

        MPL_pointer_attr_t attr;
        MPIR_GPU_query_pointer_attr(buf, &attr);
        if (attr.type == MPL_GPU_POINTER_DEV && !MPIDI_OFI_ENABLE_HMEM) {
            /* Force packing of GPU buffer in host memory */
            need_packing = true;
        }
    } else {
        data_sz = MPIDI_OFI_AMREQUEST(sreq, deferred_req)->data_sz;
        need_packing = MPIDI_OFI_AMREQUEST(sreq, deferred_req)->need_packing;
    }

    if (!issue_deferred && MPIDI_OFI_global.per_vni[vni_src].deferred_am_isend_q) {
        /* if the deferred queue is not empty, all new ops must be deferred to maintain ordering */
        goto fn_deferred;
    }

    if (need_packing) {
        /* FIXME: currently we always do packing, also for high density types. However,
         * we should not do packing unless needed. Also, for large low-density types
         * we should not allocate the entire buffer and do the packing at once. */
        /* TODO: (1) Skip packing for high-density datatypes; */
        /* FIXME: currently always allocate pack buffer for any size. This should be removed in next
         * step when we work on ZCOPY protocol support. Basically, if the src buf and datatype needs
         * packing, we should not be doing RDMA read. */
        send_buf = MPL_malloc(data_sz, MPL_MEM_OTHER);
        mpi_errno = MPIR_Typerep_pack(buf, count, datatype, 0, send_buf, data_sz, &last,
                                      MPIR_TYPEREP_FLAG_NONE);
        MPIR_ERR_CHECK(mpi_errno);
        MPIR_Assert(data_sz == last);

        MPIDI_OFI_AM_SREQ_HDR(sreq, pack_buffer) = send_buf;
    } else {
        MPIDI_Datatype_check_lb(datatype, dt_true_lb);
        send_buf = MPIR_get_contig_ptr(buf, dt_true_lb);
    }

    MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_GENERAL, VERBOSE,
                    (MPL_DBG_FDEST,
                     "send RDMA read for req handle=0x%x send_size %ld", sreq->handle, data_sz));

    mpi_errno =
        MPIDI_OFI_am_isend_long(rank, comm, handler_id, send_buf, data_sz, sreq, vni_src, vni_dst);
    MPIR_ERR_CHECK(mpi_errno);
    if (issue_deferred) {
        DL_DELETE(MPIDI_OFI_global.per_vni[vni_src].deferred_am_isend_q,
                  MPIDI_OFI_AMREQUEST(sreq, deferred_req));
        MPL_free(MPIDI_OFI_AMREQUEST(sreq, deferred_req));
    }

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
  fn_deferred:
    DEFER_AM_SEND(MPIDI_OFI_DEFERRED_AM_OP__ISEND_RDMA_READ);
    goto fn_exit;
}

#undef DEFER_AM_SEND

#endif /* OFI_AM_IMPL_H_INCLUDED */
