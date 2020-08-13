/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef OFI_AM_IMPL_H_INCLUDED
#define OFI_AM_IMPL_H_INCLUDED

#include "ofi_impl.h"
#include "mpidu_genq.h"

MPL_STATIC_INLINE_PREFIX int MPIDI_OFI_progress_do_queue(int vni_idx);

/* Acquire a sequence number to send, and record the next number */
MPL_STATIC_INLINE_PREFIX uint16_t MPIDI_OFI_am_fetch_incr_send_seqno(MPIR_Comm * comm,
                                                                     int dest_rank)
{
    int nic = 0;
    fi_addr_t addr = MPIDI_OFI_comm_to_phys(comm, dest_rank, nic, 0, 0);
    uint64_t id = addr;
    uint16_t seq, old_seq;
    void *ret;

    ret = MPIDIU_map_lookup(MPIDI_OFI_global.am_send_seq_tracker, id);
    if (ret == MPIDIU_MAP_NOT_FOUND)
        old_seq = 0;
    else
        old_seq = (uint16_t) (uintptr_t) ret;

    seq = old_seq + 1;
    MPIDIU_map_update(MPIDI_OFI_global.am_send_seq_tracker, id, (void *) (uintptr_t) seq,
                      MPL_MEM_OTHER);

    MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_GENERAL, VERBOSE,
                    (MPL_DBG_FDEST,
                     "Generated seqno=%d for dest_rank=%d "
                     "(context_id=0x%08x, src_addr=%" PRIx64 ", dest_addr=%" PRIx64 ")\n",
                     old_seq, dest_rank, comm->context_id,
                     MPIDI_OFI_comm_to_phys(MPIR_Process.comm_world, MPIR_Process.comm_world->rank,
                                            nic, 0, 0), addr));

    return old_seq;
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

MPL_STATIC_INLINE_PREFIX void MPIDI_OFI_am_clear_request(MPIR_Request * sreq)
{
    MPIDI_OFI_am_request_header_t *req_hdr;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_OFI_AM_CLEAR_REQUEST);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_OFI_AM_CLEAR_REQUEST);

    req_hdr = MPIDI_OFI_AMREQUEST(sreq, req_hdr);

    if (!req_hdr)
        goto fn_exit;

    MPIDU_genq_private_pool_free_cell(MPIDI_OFI_global.am_hdr_buf_pool, req_hdr);
    MPIDI_OFI_AMREQUEST(sreq, req_hdr) = NULL;

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_OFI_AM_CLEAR_REQUEST);
    return;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_OFI_am_init_request(const void *am_hdr,
                                                       size_t am_hdr_sz, MPIR_Request * sreq)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_OFI_am_request_header_t *req_hdr;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_OFI_AM_INIT_REQUEST);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_OFI_AM_INIT_REQUEST);

    MPIR_Assert(am_hdr_sz < (1ULL << MPIDI_OFI_AM_HDR_SZ_BITS));

    if (MPIDI_OFI_AMREQUEST(sreq, req_hdr) == NULL) {
        MPIDU_genq_private_pool_alloc_cell(MPIDI_OFI_global.am_hdr_buf_pool, (void **) &req_hdr);
        MPIR_Assert(req_hdr);
        MPIDI_OFI_AMREQUEST(sreq, req_hdr) = req_hdr;

        req_hdr->am_hdr = (void *) &req_hdr->am_hdr_buf[0];
        req_hdr->am_hdr_sz = am_hdr_sz;
    } else {
        req_hdr = MPIDI_OFI_AMREQUEST(sreq, req_hdr);
    }

    if (am_hdr) {
        MPIR_Memcpy(req_hdr->am_hdr, am_hdr, am_hdr_sz);
    }

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_OFI_AM_INIT_REQUEST);
    return mpi_errno;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_OFI_repost_buffer(void *buf, MPIR_Request * req)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_OFI_am_repost_request_t *am = (MPIDI_OFI_am_repost_request_t *) req;
    int nic = 0;
    int ctx_idx = MPIDI_OFI_get_ctx_index(0, nic);

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_OFI_REPOST_BUFFER);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_OFI_REPOST_BUFFER);
    MPIDI_OFI_CALL_RETRY_AM(fi_recvmsg(MPIDI_OFI_global.ctx[ctx_idx].rx,
                                       &MPIDI_OFI_global.am_msg[am->index],
                                       FI_MULTI_RECV | FI_COMPLETION), prepost);
  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_OFI_REPOST_BUFFER);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_OFI_progress_do_queue(int vni_idx)
{
    int mpi_errno = MPI_SUCCESS, ret = 0;
    struct fi_cq_tagged_entry cq_entry;

    /* Caller must hold MPIDI_OFI_THREAD_FI_MUTEX */

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_OFI_PROGRESS_DO_QUEUE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_OFI_PROGRESS_DO_QUEUE);

    for (int nic = 0; nic < MPIDI_OFI_global.num_nics; nic++) {
        int ctx_idx = MPIDI_OFI_get_ctx_index(vni_idx, nic);
        ret = fi_cq_read(MPIDI_OFI_global.ctx[ctx_idx].cq, &cq_entry, 1);
    }

    if (unlikely(ret == -FI_EAGAIN))
        goto fn_exit;

    if (ret < 0) {
        mpi_errno = MPIDI_OFI_handle_cq_error_util(vni_idx, ret);
        goto fn_fail;
    }

    /* If the statically allocated buffered list is full or we've already
     * started using the dynamic list, continue using it. */
    if (((MPIDI_OFI_global.cq_buffered_static_head + 1) %
         MPIDI_OFI_NUM_CQ_BUFFERED == MPIDI_OFI_global.cq_buffered_static_tail) ||
        (NULL != MPIDI_OFI_global.cq_buffered_dynamic_head)) {
        MPIDI_OFI_cq_list_t *list_entry =
            (MPIDI_OFI_cq_list_t *) MPL_malloc(sizeof(MPIDI_OFI_cq_list_t), MPL_MEM_BUFFER);
        MPIR_Assert(list_entry);
        list_entry->cq_entry = cq_entry;
        LL_APPEND(MPIDI_OFI_global.cq_buffered_dynamic_head,
                  MPIDI_OFI_global.cq_buffered_dynamic_tail, list_entry);
    } else {
        MPIDI_OFI_global.cq_buffered_static_list[MPIDI_OFI_global.
                                                 cq_buffered_static_head].cq_entry = cq_entry;
        MPIDI_OFI_global.cq_buffered_static_head =
            (MPIDI_OFI_global.cq_buffered_static_head + 1) % MPIDI_OFI_NUM_CQ_BUFFERED;
    }

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_OFI_PROGRESS_DO_QUEUE);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_OFI_am_isend_long(int rank, MPIR_Comm * comm, int handler_id,
                                                     const void *data, MPI_Aint data_sz,
                                                     MPIR_Request * sreq)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_OFI_am_header_t *msg_hdr;
    MPIDI_OFI_lmt_msg_payload_t *lmt_info;
    struct iovec *iov;
    int nic = 0;
    int ctx_idx = MPIDI_OFI_get_ctx_index(0, nic);

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_OFI_AM_ISEND_LONG);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_OFI_AM_ISEND_LONG);

    MPIR_Assert(handler_id < (1 << MPIDI_OFI_AM_HANDLER_ID_BITS));
    MPIR_Assert((uint64_t) comm->rank < (1ULL << MPIDI_OFI_AM_RANK_BITS));

    msg_hdr = &MPIDI_OFI_AMREQUEST_HDR(sreq, msg_hdr);
    msg_hdr->handler_id = handler_id;
    msg_hdr->am_hdr_sz = MPIDI_OFI_AMREQUEST_HDR(sreq, am_hdr_sz);
    msg_hdr->payload_sz = 0;    /* LMT info sent as header */
    msg_hdr->am_type = MPIDI_AMTYPE_RDMA_READ;
    msg_hdr->seqno = MPIDI_OFI_am_fetch_incr_send_seqno(comm, rank);
    msg_hdr->fi_src_addr
        = MPIDI_OFI_comm_to_phys(MPIR_Process.comm_world, MPIR_Process.comm_world->rank, nic, 0, 0);

    lmt_info = &MPIDI_OFI_AMREQUEST_HDR(sreq, lmt_info);
    lmt_info->context_id = comm->context_id;
    lmt_info->src_rank = comm->rank;
    lmt_info->src_offset =
        !MPIDI_OFI_ENABLE_MR_VIRT_ADDRESS ? (uint64_t) 0 : (uint64_t) (uintptr_t) data;

    lmt_info->sreq_ptr = sreq;
    if (!MPIDI_OFI_ENABLE_MR_PROV_KEY) {
        lmt_info->rma_key =
            MPIDI_OFI_mr_key_alloc(MPIDI_OFI_LOCAL_MR_KEY, MPIDI_OFI_INVALID_MR_KEY);
    } else {
        lmt_info->rma_key = 0;
    }
    lmt_info->reg_sz = data_sz;

    MPIR_cc_inc(sreq->cc_ptr);  /* send completion */
    MPIR_cc_inc(sreq->cc_ptr);  /* lmt ack handler */
    MPIR_Assert((sizeof(*msg_hdr) + sizeof(*lmt_info) + MPIDI_OFI_AMREQUEST_HDR(sreq, am_hdr_sz)) <=
                MPIDI_OFI_DEFAULT_SHORT_SEND_SIZE);
    MPIDI_OFI_CALL(fi_mr_reg(MPIDI_OFI_global.ctx[ctx_idx].domain,
                             data,
                             data_sz,
                             FI_REMOTE_READ,
                             0ULL,
                             lmt_info->rma_key,
                             0ULL, &MPIDI_OFI_AMREQUEST_HDR(sreq, lmt_mr), NULL), mr_reg);
    MPL_atomic_fetch_add_int(&MPIDI_OFI_global.am_inflight_rma_send_mrs, 1);

    if (MPIDI_OFI_ENABLE_MR_PROV_KEY) {
        /* MR_BASIC */
        lmt_info->rma_key = fi_mr_key(MPIDI_OFI_AMREQUEST_HDR(sreq, lmt_mr));
    }

    iov = MPIDI_OFI_AMREQUEST_HDR(sreq, iov);

    iov[0].iov_base = msg_hdr;
    iov[0].iov_len = sizeof(*msg_hdr);

    iov[1].iov_base = MPIDI_OFI_AMREQUEST_HDR(sreq, am_hdr);
    iov[1].iov_len = MPIDI_OFI_AMREQUEST_HDR(sreq, am_hdr_sz);

    iov[2].iov_base = lmt_info;
    iov[2].iov_len = sizeof(*lmt_info);
    MPIDI_OFI_AMREQUEST(sreq, event_id) = MPIDI_OFI_EVENT_AM_SEND_RDMA;
    MPIDI_OFI_CALL_RETRY_AM(fi_sendv(MPIDI_OFI_global.ctx[ctx_idx].tx, iov, NULL, 3,
                                     MPIDI_OFI_comm_to_phys(comm, rank, nic, 0, 0),
                                     &MPIDI_OFI_AMREQUEST(sreq, context)), sendv);
  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_OFI_AM_ISEND_LONG);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_OFI_am_isend_short(int rank, MPIR_Comm * comm, int handler_id,
                                                      const void *data, MPI_Aint data_sz,
                                                      MPIR_Request * sreq)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_OFI_am_header_t *msg_hdr;
    struct iovec *iov;
    int nic = 0;
    int ctx_idx = MPIDI_OFI_get_ctx_index(0, nic);

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_OFI_AM_ISEND_SHORT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_OFI_AM_ISEND_SHORT);

    MPIR_Assert(handler_id < (1 << MPIDI_OFI_AM_HANDLER_ID_BITS));
    MPIR_Assert(data_sz < (1ULL << MPIDI_OFI_AM_PAYLOAD_SZ_BITS));
    MPIR_Assert((uint64_t) comm->rank < (1ULL << MPIDI_OFI_AM_RANK_BITS));

    msg_hdr = &MPIDI_OFI_AMREQUEST_HDR(sreq, msg_hdr);
    msg_hdr->handler_id = handler_id;
    msg_hdr->am_hdr_sz = MPIDI_OFI_AMREQUEST_HDR(sreq, am_hdr_sz);
    msg_hdr->payload_sz = data_sz;
    msg_hdr->am_type = MPIDI_AMTYPE_SHORT;
    msg_hdr->seqno = MPIDI_OFI_am_fetch_incr_send_seqno(comm, rank);
    msg_hdr->fi_src_addr
        = MPIDI_OFI_comm_to_phys(MPIR_Process.comm_world, MPIR_Process.comm_world->rank, nic, 0, 0);

    iov = MPIDI_OFI_AMREQUEST_HDR(sreq, iov);

    iov[0].iov_base = msg_hdr;
    iov[0].iov_len = sizeof(*msg_hdr);

    iov[1].iov_base = MPIDI_OFI_AMREQUEST_HDR(sreq, am_hdr);
    iov[1].iov_len = MPIDI_OFI_AMREQUEST_HDR(sreq, am_hdr_sz);

    iov[2].iov_base = (void *) data;
    iov[2].iov_len = data_sz;

    MPIR_cc_inc(sreq->cc_ptr);
    MPIDI_OFI_AMREQUEST(sreq, event_id) = MPIDI_OFI_EVENT_AM_SEND;
    MPIDI_OFI_CALL_RETRY_AM(fi_sendv(MPIDI_OFI_global.ctx[ctx_idx].tx, iov, NULL, 3,
                                     MPIDI_OFI_comm_to_phys(comm, rank, nic, 0, 0),
                                     &MPIDI_OFI_AMREQUEST(sreq, context)), sendv);
  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_OFI_AM_ISEND_SHORT);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_OFI_am_isend_pipeline(int rank, MPIR_Comm * comm, int handler_id,
                                                         const void *data, MPI_Aint seg_sz,
                                                         MPI_Aint data_sz, MPIR_Request * sreq,
                                                         MPIDI_OFI_am_send_pipeline_request_t *
                                                         send_req)
{
    int mpi_errno = MPI_SUCCESS, c;
    MPIDI_OFI_am_header_t *msg_hdr;
    struct iovec *iov;
    int nic = 0;
    int ctx_idx = MPIDI_OFI_get_ctx_index(0, nic);

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_OFI_AM_ISEND_PIPELINE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_OFI_AM_ISEND_PIPELINE);

    MPIR_Assert(handler_id < (1 << MPIDI_OFI_AM_HANDLER_ID_BITS));
    MPIR_Assert(seg_sz < (1ULL << MPIDI_OFI_AM_PAYLOAD_SZ_BITS));
    MPIR_Assert((uint64_t) comm->rank < (1ULL << MPIDI_OFI_AM_RANK_BITS));

    msg_hdr = &send_req->msg_hdr;
    msg_hdr->handler_id = handler_id;
    msg_hdr->payload_sz = seg_sz;
    msg_hdr->am_type = MPIDI_AMTYPE_PIPELINE;
    msg_hdr->seqno = MPIDI_OFI_am_fetch_incr_send_seqno(comm, rank);
    msg_hdr->fi_src_addr
        = MPIDI_OFI_comm_to_phys(MPIR_Process.comm_world, MPIR_Process.comm_world->rank, nic, 0, 0);

    iov = send_req->iov;

    MPIR_cc_incr(sreq->cc_ptr, &c);
    send_req->event_id = MPIDI_OFI_EVENT_AM_SEND_PIPELINE;

    int num_iov = 0;
    iov[num_iov].iov_base = msg_hdr;
    iov[num_iov].iov_len = sizeof(*msg_hdr);
    num_iov++;

    if (MPIDIG_am_send_async_get_offset(sreq) == 0) {
        msg_hdr->am_hdr_sz = MPIDI_OFI_AMREQUEST_HDR(sreq, am_hdr_sz);
        iov[num_iov].iov_base = MPIDI_OFI_AMREQUEST_HDR(sreq, am_hdr);
        iov[num_iov].iov_len = MPIDI_OFI_AMREQUEST_HDR(sreq, am_hdr_sz);
        num_iov++;
    } else {
        msg_hdr->am_hdr_sz = 0;
    }

    iov[num_iov].iov_base = (void *) data;
    iov[num_iov].iov_len = seg_sz;
    num_iov++;

    MPIDI_OFI_CALL_RETRY_AM(fi_sendv(MPIDI_OFI_global.ctx[ctx_idx].tx, iov, NULL, num_iov,
                                     MPIDI_OFI_comm_to_phys(comm, rank, nic, 0, 0),
                                     &send_req->context), sendv);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_OFI_AM_ISEND_PIPELINE);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_OFI_do_am_isend_eager(int rank, MPIR_Comm * comm,
                                                         int handler_id, const void *am_hdr,
                                                         size_t am_hdr_sz, const void *buf,
                                                         size_t count, MPI_Datatype datatype,
                                                         MPIR_Request * sreq, bool issue_deferred)
{
    int dt_contig, mpi_errno = MPI_SUCCESS;
    char *send_buf;
    MPI_Aint data_sz;
    MPI_Aint dt_true_lb, last;
    bool need_packing = false;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_OFI_DO_AM_ISEND_EAGER);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_OFI_DO_AM_ISEND_EAGER);

    /* NOTE: issue_deferred is set to true when progress use this function for deferred operations.
     * we need to skip some code path in the scenario. Also am_hdr and am_hdr_sz are ignored when
     * issue_deferred is set to true. They should have been saved in the request. */

    if (!issue_deferred) {
        MPIDI_OFI_AMREQUEST(sreq, req_hdr) = NULL;
        mpi_errno = MPIDI_OFI_am_init_request(am_hdr, am_hdr_sz, sreq);
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

    if (!issue_deferred && MPIDI_OFI_global.deferred_am_isend_q) {
        /* if the deferred queue is not empty, all new ops must be deferred to maintain ordering */
        goto fn_deferred;
    }

    if (need_packing) {
        /* FIXME: currently we always do packing, also for high density types. However,
         * we should not do packing unless needed. Also, for large low-density types
         * we should not allocate the entire buffer and do the packing at once. */
        /* TODO: (1) Skip packing for high-density datatypes; */
        MPIR_Assert(data_sz <= MPIDI_OFI_DEFAULT_SHORT_SEND_SIZE);
        MPIDU_genq_private_pool_alloc_cell(MPIDI_OFI_global.pack_buf_pool, (void **) &send_buf);
        if (send_buf == NULL) {
            if (!issue_deferred) {
                goto fn_deferred;
            } else {
                goto fn_exit;
            }
        }
        mpi_errno = MPIR_Typerep_pack(buf, count, datatype, 0, send_buf, data_sz, &last);
        MPIR_ERR_CHECK(mpi_errno);
        MPIR_Assert(data_sz == last);

        MPIDI_OFI_AMREQUEST_HDR(sreq, pack_buffer) = send_buf;
    } else {
        MPIDI_Datatype_check_lb(datatype, dt_true_lb);
        send_buf = (char *) buf + dt_true_lb;
        MPIDI_OFI_AMREQUEST_HDR(sreq, pack_buffer) = NULL;
    }

    MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_GENERAL, VERBOSE,
                    (MPL_DBG_FDEST,
                     "issue eager seg for req handle=0x%x send_size %ld", sreq->handle, data_sz));

    mpi_errno = MPIDI_OFI_am_isend_short(rank, comm, handler_id, send_buf, data_sz, sreq);
    MPIR_ERR_CHECK(mpi_errno);
    if (issue_deferred) {
        DL_DELETE(MPIDI_OFI_global.deferred_am_isend_q, MPIDI_OFI_AMREQUEST(sreq, deferred_req));
        MPL_free(MPIDI_OFI_AMREQUEST(sreq, deferred_req));
    }

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_OFI_DO_AM_ISEND_EAGER);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
  fn_deferred:
    MPIDI_OFI_AMREQUEST(sreq, deferred_req) =
        (MPIDI_OFI_deferred_am_isend_req_t *) MPL_malloc(sizeof(MPIDI_OFI_deferred_am_isend_req_t),
                                                         MPL_MEM_OTHER);
    MPIR_Assert(MPIDI_OFI_AMREQUEST(sreq, deferred_req));
    MPIDI_OFI_AMREQUEST(sreq, deferred_req)->op = MPIDI_OFI_DEFERRED_AM_OP__ISEND_EAGER;
    MPIDI_OFI_AMREQUEST(sreq, deferred_req)->rank = rank;
    MPIDI_OFI_AMREQUEST(sreq, deferred_req)->comm = comm;
    MPIDI_OFI_AMREQUEST(sreq, deferred_req)->handler_id = handler_id;
    MPIDI_OFI_AMREQUEST(sreq, deferred_req)->buf = buf;
    MPIDI_OFI_AMREQUEST(sreq, deferred_req)->count = count;
    MPIDI_OFI_AMREQUEST(sreq, deferred_req)->datatype = datatype;
    MPIDI_OFI_AMREQUEST(sreq, deferred_req)->sreq = sreq;
    MPIDI_OFI_AMREQUEST(sreq, deferred_req)->data_sz = data_sz;
    MPIDI_OFI_AMREQUEST(sreq, deferred_req)->need_packing = need_packing;
    DL_APPEND(MPIDI_OFI_global.deferred_am_isend_q, MPIDI_OFI_AMREQUEST(sreq, deferred_req));
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_OFI_do_emulated_inject(fi_addr_t addr,
                                                          const MPIDI_OFI_am_header_t * msg_hdrp,
                                                          const void *am_hdr, size_t am_hdr_sz)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Request *sreq;
    char *ibuf;
    size_t len;
    int nic = 0;
    int ctx_idx = MPIDI_OFI_get_ctx_index(0, nic);

    sreq = MPIR_Request_create_from_pool(MPIR_REQUEST_KIND__SEND, 0, 1);
    MPIR_ERR_CHKANDSTMT((sreq) == NULL, mpi_errno, MPIX_ERR_NOREQ, goto fn_fail, "**nomemreq");
    len = am_hdr_sz + sizeof(*msg_hdrp);
    ibuf = (char *) MPL_malloc(len, MPL_MEM_BUFFER);
    MPIR_Assert(ibuf);
    memcpy(ibuf, msg_hdrp, sizeof(*msg_hdrp));
    memcpy(ibuf + sizeof(*msg_hdrp), am_hdr, am_hdr_sz);

    MPIDI_OFI_REQUEST(sreq, event_id) = MPIDI_OFI_EVENT_INJECT_EMU;
    MPIDI_OFI_REQUEST(sreq, util.inject_buf) = ibuf;
    MPL_atomic_fetch_add_int(&MPIDI_OFI_global.am_inflight_inject_emus, 1);

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
                                                 size_t am_hdr_sz)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_OFI_am_header_t msg_hdr;
    fi_addr_t addr;
    char *buff;
    size_t buff_len;
    int nic = 0;
    int ctx_idx = MPIDI_OFI_get_ctx_index(0, nic);
    MPIR_CHKLMEM_DECL(1);

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_OFI_DO_INJECT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_OFI_DO_INJECT);

    MPIR_Assert(handler_id < (1 << MPIDI_OFI_AM_HANDLER_ID_BITS));
    MPIR_Assert(am_hdr_sz < (1ULL << MPIDI_OFI_AM_HDR_SZ_BITS));

    msg_hdr.handler_id = handler_id;
    msg_hdr.am_hdr_sz = am_hdr_sz;
    msg_hdr.payload_sz = 0;
    msg_hdr.am_type = MPIDI_AMTYPE_SHORT_HDR;
    msg_hdr.seqno = MPIDI_OFI_am_fetch_incr_send_seqno(comm, rank);
    msg_hdr.fi_src_addr
        = MPIDI_OFI_comm_to_phys(MPIR_Process.comm_world, MPIR_Process.comm_world->rank, nic, 0, 0);

    MPIR_Assert((uint64_t) comm->rank < (1ULL << MPIDI_OFI_AM_RANK_BITS));

    addr = MPIDI_OFI_comm_to_phys(comm, rank, nic, 0, 0);

    if (unlikely(am_hdr_sz + sizeof(msg_hdr) > MPIDI_OFI_global.max_buffered_send)) {
        mpi_errno = MPIDI_OFI_do_emulated_inject(addr, &msg_hdr, am_hdr, am_hdr_sz);
        goto fn_exit;
    }

    buff_len = sizeof(msg_hdr) + am_hdr_sz;
    MPIR_CHKLMEM_MALLOC(buff, char *, buff_len, mpi_errno, "buff", MPL_MEM_BUFFER);
    memcpy(buff, &msg_hdr, sizeof(msg_hdr));
    memcpy(buff + sizeof(msg_hdr), am_hdr, am_hdr_sz);

    MPIDI_OFI_CALL_RETRY_AM(fi_inject(MPIDI_OFI_global.ctx[ctx_idx].tx, buff, buff_len, addr),
                            inject);

  fn_exit:
    MPIR_CHKLMEM_FREEALL();
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_OFI_DO_INJECT);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_OFI_do_am_isend_pipeline(int rank, MPIR_Comm * comm,
                                                            int handler_id, const void *am_hdr,
                                                            size_t am_hdr_sz, const void *buf,
                                                            size_t count, MPI_Datatype datatype,
                                                            MPIR_Request * sreq, MPI_Aint data_sz,
                                                            bool issue_deferred)
{
    int dt_contig, mpi_errno = MPI_SUCCESS;
    char *send_buf;
    MPI_Aint packed_size;
    MPI_Aint dt_true_lb, send_size, offset;
    bool need_packing = false;
    MPIDI_OFI_am_send_pipeline_request_t *send_req;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_OFI_DO_AM_ISEND_PIPELINE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_OFI_DO_AM_ISEND_PIPELINE);

    /* NOTE: issue_deferred is set to true when progress use this function for deferred operations.
     * we need to skip some code path in the scenario. Also am_hdr, am_hdr_sz and data_sz are
     * ignored when issue_deferred is set to true. They should have been saved in the request. */

    if (!issue_deferred) {
        MPIDI_OFI_AMREQUEST(sreq, req_hdr) = NULL;
        mpi_errno = MPIDI_OFI_am_init_request(am_hdr, am_hdr_sz, sreq);
        MPIR_ERR_CHECK(mpi_errno);

        MPIDI_Datatype_check_contig(datatype, dt_contig);

        send_size = MPIDI_OFI_DEFAULT_SHORT_SEND_SIZE - am_hdr_sz - sizeof(MPIDI_OFI_am_header_t);
        send_size = MPL_MIN(send_size, data_sz);
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
        send_size = MPIDI_OFI_DEFAULT_SHORT_SEND_SIZE -
            (offset ? 0 : MPIDI_OFI_AMREQUEST_HDR(sreq, am_hdr_sz)) - sizeof(MPIDI_OFI_am_header_t);
        send_size = MPL_MIN(send_size, MPIDIG_am_send_async_get_data_sz_left(sreq));
        need_packing = MPIDI_OFI_AMREQUEST(sreq, deferred_req)->need_packing;
    }

    if (!issue_deferred && MPIDI_OFI_global.deferred_am_isend_q) {
        /* if the deferred queue is not empty, all new ops must be deferred to maintain ordering */
        goto fn_deferred;
    }

    if (need_packing) {
        /* FIXME: currently we always do packing, also for high density types. However,
         * we should not do packing unless needed. Also, for large low-density types
         * we should not allocate the entire buffer and do the packing at once. */
        /* TODO: (1) Skip packing for high-density datatypes; */
        MPIDU_genq_private_pool_alloc_cell(MPIDI_OFI_global.pack_buf_pool, (void **) &send_buf);
        if (send_buf == NULL) {
            if (!issue_deferred) {
                goto fn_deferred;
            } else {
                /* trying to issue deferred op but still cannot get pack buffer, just exit here */
                goto fn_exit;
            }
        }
        mpi_errno = MPIR_Typerep_pack(buf, count, datatype, offset, send_buf,
                                      send_size, &packed_size);
        MPIR_ERR_CHECK(mpi_errno);
        send_size = packed_size;

        MPIDU_genq_private_pool_alloc_cell(MPIDI_OFI_global.am_hdr_buf_pool, (void **) &send_req);
        MPIR_Assert(send_req);
        send_req->sreq = sreq;
        send_req->pack_buffer = send_buf;
    } else {
        MPIDI_Datatype_check_lb(datatype, dt_true_lb);
        send_buf = (char *) buf + dt_true_lb + offset;
        MPIDU_genq_private_pool_alloc_cell(MPIDI_OFI_global.am_hdr_buf_pool, (void **) &send_req);
        MPIR_Assert(send_req);
        send_req->sreq = sreq;
        send_req->pack_buffer = NULL;
    }

    MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_GENERAL, VERBOSE,
                    (MPL_DBG_FDEST,
                     "issue pipeline seg for req handle=0x%x send_size %ld", sreq->handle,
                     send_size));

    mpi_errno = MPIDI_OFI_am_isend_pipeline(rank, comm, handler_id, send_buf, send_size,
                                            data_sz, sreq, send_req);
    MPIDIG_am_send_async_issue_seg(sreq, send_size);
    MPIR_ERR_CHECK(mpi_errno);

    /* if there IS MORE DATA to be sent and we ARE NOT called for issue deferred op, enqueue.
     * if there NO MORE DATA and we ARE called for issuing deferred op, pipeline is done, dequeue
     * skip for all other cases */
    if (!MPIDIG_am_send_async_is_done(sreq)) {
        if (!issue_deferred) {
            goto fn_deferred;
        }
    } else if (issue_deferred) {
        DL_DELETE(MPIDI_OFI_global.deferred_am_isend_q, MPIDI_OFI_AMREQUEST(sreq, deferred_req));
        MPL_free(MPIDI_OFI_AMREQUEST(sreq, deferred_req));
    }

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_OFI_DO_AM_ISEND_PIPELINE);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
  fn_deferred:
    MPIDI_OFI_AMREQUEST(sreq, deferred_req) =
        (MPIDI_OFI_deferred_am_isend_req_t *) MPL_malloc(sizeof(MPIDI_OFI_deferred_am_isend_req_t),
                                                         MPL_MEM_OTHER);
    MPIR_Assert(MPIDI_OFI_AMREQUEST(sreq, deferred_req));
    MPIDI_OFI_AMREQUEST(sreq, deferred_req)->op = MPIDI_OFI_DEFERRED_AM_OP__ISEND_PIPELINE;
    MPIDI_OFI_AMREQUEST(sreq, deferred_req)->rank = rank;
    MPIDI_OFI_AMREQUEST(sreq, deferred_req)->comm = comm;
    MPIDI_OFI_AMREQUEST(sreq, deferred_req)->handler_id = handler_id;
    MPIDI_OFI_AMREQUEST(sreq, deferred_req)->buf = buf;
    MPIDI_OFI_AMREQUEST(sreq, deferred_req)->count = count;
    MPIDI_OFI_AMREQUEST(sreq, deferred_req)->datatype = datatype;
    MPIDI_OFI_AMREQUEST(sreq, deferred_req)->sreq = sreq;
    MPIDI_OFI_AMREQUEST(sreq, deferred_req)->data_sz = data_sz;
    MPIDI_OFI_AMREQUEST(sreq, deferred_req)->need_packing = need_packing;
    DL_APPEND(MPIDI_OFI_global.deferred_am_isend_q, MPIDI_OFI_AMREQUEST(sreq, deferred_req));
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_OFI_do_am_isend_rdma_read(int rank, MPIR_Comm * comm,
                                                             int handler_id, const void *am_hdr,
                                                             size_t am_hdr_sz, const void *buf,
                                                             size_t count, MPI_Datatype datatype,
                                                             MPIR_Request * sreq,
                                                             bool issue_deferred)
{
    int dt_contig, mpi_errno = MPI_SUCCESS;
    char *send_buf;
    MPI_Aint data_sz;
    MPI_Aint dt_true_lb, last;
    bool need_packing = false;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_OFI_DO_AM_ISEND_RDMA_READ);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_OFI_DO_AM_ISEND_RDMA_READ);

    /* NOTE: issue_deferred is set to true when progress use this function for deferred operations.
     * we need to skip some code path in the scenario. Also am_hdr and am_hdr_sz are ignored when
     * issue_deferred is set to true. They should have been saved in the request. */

    if (!issue_deferred) {
        MPIDI_OFI_AMREQUEST(sreq, req_hdr) = NULL;
        mpi_errno = MPIDI_OFI_am_init_request(am_hdr, am_hdr_sz, sreq);
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

    if (!issue_deferred && MPIDI_OFI_global.deferred_am_isend_q) {
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
        MPIR_gpu_malloc_host((void **) &send_buf, data_sz);
        mpi_errno = MPIR_Typerep_pack(buf, count, datatype, 0, send_buf, data_sz, &last);
        MPIR_ERR_CHECK(mpi_errno);
        MPIR_Assert(data_sz == last);

        MPIDI_OFI_AMREQUEST_HDR(sreq, pack_buffer) = send_buf;
    } else {
        MPIDI_Datatype_check_lb(datatype, dt_true_lb);
        send_buf = (char *) buf + dt_true_lb;
        MPIDI_OFI_AMREQUEST_HDR(sreq, pack_buffer) = NULL;
    }

    MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_GENERAL, VERBOSE,
                    (MPL_DBG_FDEST,
                     "send RDMA read for req handle=0x%x send_size %ld", sreq->handle, data_sz));

    mpi_errno = MPIDI_OFI_am_isend_long(rank, comm, handler_id, send_buf, data_sz, sreq);
    MPIR_ERR_CHECK(mpi_errno);
    if (issue_deferred) {
        DL_DELETE(MPIDI_OFI_global.deferred_am_isend_q, MPIDI_OFI_AMREQUEST(sreq, deferred_req));
        MPL_free(MPIDI_OFI_AMREQUEST(sreq, deferred_req));
    }

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_OFI_DO_AM_ISEND_RDMA_READ);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
  fn_deferred:
    MPIDI_OFI_AMREQUEST(sreq, deferred_req) =
        (MPIDI_OFI_deferred_am_isend_req_t *) MPL_malloc(sizeof(MPIDI_OFI_deferred_am_isend_req_t),
                                                         MPL_MEM_OTHER);
    MPIR_Assert(MPIDI_OFI_AMREQUEST(sreq, deferred_req));
    MPIDI_OFI_AMREQUEST(sreq, deferred_req)->op = MPIDI_OFI_DEFERRED_AM_OP__ISEND_RDMA_READ;
    MPIDI_OFI_AMREQUEST(sreq, deferred_req)->rank = rank;
    MPIDI_OFI_AMREQUEST(sreq, deferred_req)->comm = comm;
    MPIDI_OFI_AMREQUEST(sreq, deferred_req)->handler_id = handler_id;
    MPIDI_OFI_AMREQUEST(sreq, deferred_req)->buf = buf;
    MPIDI_OFI_AMREQUEST(sreq, deferred_req)->count = count;
    MPIDI_OFI_AMREQUEST(sreq, deferred_req)->datatype = datatype;
    MPIDI_OFI_AMREQUEST(sreq, deferred_req)->sreq = sreq;
    MPIDI_OFI_AMREQUEST(sreq, deferred_req)->data_sz = data_sz;
    MPIDI_OFI_AMREQUEST(sreq, deferred_req)->need_packing = need_packing;
    DL_APPEND(MPIDI_OFI_global.deferred_am_isend_q, MPIDI_OFI_AMREQUEST(sreq, deferred_req));
    goto fn_exit;
}

#endif /* OFI_AM_IMPL_H_INCLUDED */
