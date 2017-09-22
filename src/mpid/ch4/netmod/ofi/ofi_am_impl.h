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
#ifndef OFI_AM_IMPL_H_INCLUDED
#define OFI_AM_IMPL_H_INCLUDED

#include "ofi_impl.h"

static inline int MPIDI_OFI_progress_do_queue(int vni_idx);

/*
  Per-object lock for OFI

  * When calling OFI function MPIDI_OFI_THREAD_FI_MUTEX must be held.
  * When being called from the MPI layer (app), we must grab the lock.
    This is the case for regular (non-reply) functions such as am_isend.
  * When being called from callback function or progress engine, we must
    not grab the lock because the progress engine is already holding the lock.
    This is the case for reply functions such as am_isend_reply.
*/
#define MPIDI_OFI_CALL_RETRY_AM(FUNC,LOCK,STR)                  \
    do {                                                                \
        ssize_t _ret;                                                   \
        do {                                                            \
            if (LOCK) MPID_THREAD_CS_ENTER(POBJ,MPIDI_OFI_THREAD_FI_MUTEX); \
            _ret = FUNC;                                                \
            if (LOCK) MPID_THREAD_CS_EXIT(POBJ,MPIDI_OFI_THREAD_FI_MUTEX); \
            if (likely(_ret==0)) break;                                  \
            MPIR_ERR_##CHKANDJUMP4(_ret != -FI_EAGAIN,                  \
                                   mpi_errno,                           \
                                   MPI_ERR_OTHER,                       \
                                   "**ofi_"#STR,                        \
                                   "**ofi_"#STR" %s %d %s %s",          \
                                   __SHORT_FILE__,                      \
                                   __LINE__,                            \
                                   FCNAME,                              \
                                   fi_strerror(-_ret));                 \
            if (LOCK) MPID_THREAD_CS_ENTER(POBJ,MPIDI_OFI_THREAD_FI_MUTEX); \
            mpi_errno = MPIDI_OFI_progress_do_queue(0 /* vni_idx */);    \
            if (LOCK) MPID_THREAD_CS_EXIT(POBJ,MPIDI_OFI_THREAD_FI_MUTEX); \
            if (mpi_errno != MPI_SUCCESS)                                \
                MPIR_ERR_POP(mpi_errno);                                \
        } while (_ret == -FI_EAGAIN);                                   \
    } while (0)

#undef FUNCNAME
#define FUNCNAME MPIDI_OFI_am_clear_request
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline void MPIDI_OFI_am_clear_request(MPIR_Request * sreq)
{
    MPIDI_OFI_am_request_header_t *req_hdr;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_OFI_AM_CLEAR_REQUEST);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_OFI_AM_CLEAR_REQUEST);

    req_hdr = MPIDI_OFI_AMREQUEST(sreq, req_hdr);

    if (!req_hdr)
        return;

    if (req_hdr->am_hdr != &req_hdr->am_hdr_buf[0]) {
        MPL_free(req_hdr->am_hdr);
    }

    MPIDI_CH4R_release_buf(req_hdr);
    MPIDI_OFI_AMREQUEST(sreq, req_hdr) = NULL;
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_OFI_AM_CLEAR_REQUEST);
    return;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_OFI_am_init_request
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_OFI_am_init_request(const void *am_hdr,
                                            size_t am_hdr_sz, MPIR_Request * sreq)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_OFI_am_request_header_t *req_hdr;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_OFI_AM_INIT_REQUEST);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_OFI_AM_INIT_REQUEST);

    if (MPIDI_OFI_AMREQUEST(sreq, req_hdr) == NULL) {
        req_hdr = (MPIDI_OFI_am_request_header_t *)
            MPIDI_CH4R_get_buf(MPIDI_Global.am_buf_pool);
        MPIR_Assert(req_hdr);
        MPIDI_OFI_AMREQUEST(sreq, req_hdr) = req_hdr;

        req_hdr->am_hdr = (void *) &req_hdr->am_hdr_buf[0];
        req_hdr->am_hdr_sz = MPIDI_OFI_MAX_AM_HDR_SIZE;
    }
    else {
        req_hdr = MPIDI_OFI_AMREQUEST(sreq, req_hdr);
    }

    if (am_hdr_sz > req_hdr->am_hdr_sz) {
        if (req_hdr->am_hdr != &req_hdr->am_hdr_buf[0])
            MPL_free(req_hdr->am_hdr);

        req_hdr->am_hdr = MPL_malloc(am_hdr_sz);
        MPIR_Assert(req_hdr->am_hdr);
        req_hdr->am_hdr_sz = am_hdr_sz;
    }

    if (am_hdr) {
        MPIR_Memcpy(req_hdr->am_hdr, am_hdr, am_hdr_sz);
    }

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_OFI_AM_INIT_REQUEST);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_OFI_repost_buffer
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_OFI_repost_buffer(void *buf, MPIR_Request * req)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_OFI_am_repost_request_t *am = (MPIDI_OFI_am_repost_request_t *) req;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_OFI_REPOST_BUFFER);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_OFI_REPOST_BUFFER);
    MPIDI_OFI_CALL_RETRY_AM(fi_recvmsg(MPIDI_Global.ctx[0].rx,
                                       &MPIDI_Global.am_msg[am->index],
                                       FI_MULTI_RECV | FI_COMPLETION), FALSE /* lock */ , repost);
  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_OFI_REPOST_BUFFER);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_OFI_progress_do_queue
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_OFI_progress_do_queue(int vni_idx)
{
    int mpi_errno = MPI_SUCCESS, ret;
    struct fi_cq_tagged_entry cq_entry;

    /* Caller must hold MPIDI_OFI_THREAD_FI_MUTEX */

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_OFI_PROGRESS_DO_QUEUE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_OFI_PROGRESS_DO_QUEUE);

    ret = fi_cq_read(MPIDI_Global.ctx[vni_idx].cq, &cq_entry, 1);

    if (unlikely(ret == -FI_EAGAIN))
        goto fn_exit;

    if (ret < 0) {
        mpi_errno = MPIDI_OFI_handle_cq_error_util(vni_idx, ret);
        goto fn_fail;
    }

    if (((MPIDI_Global.cq_buff_head + 1) %
         MPIDI_OFI_NUM_CQ_BUFFERED == MPIDI_Global.cq_buff_tail) ||
        !slist_empty(&MPIDI_Global.cq_buff_list)) {
        MPIDI_OFI_cq_list_t *list_entry =
            (MPIDI_OFI_cq_list_t *) MPL_malloc(sizeof(MPIDI_OFI_cq_list_t));
        MPIR_Assert(list_entry);
        list_entry->cq_entry = cq_entry;
        slist_insert_tail(&list_entry->entry, &MPIDI_Global.cq_buff_list);
    }
    else {
        MPIDI_Global.cq_buffered[MPIDI_Global.cq_buff_head].cq_entry = cq_entry;
        MPIDI_Global.cq_buff_head = (MPIDI_Global.cq_buff_head + 1) % MPIDI_OFI_NUM_CQ_BUFFERED;
    }

    if ((cq_entry.flags & FI_RECV) && (cq_entry.flags & FI_MULTI_RECV)) {
        mpi_errno = MPIDI_OFI_repost_buffer(cq_entry.op_context,
                                            MPIDI_OFI_context_to_request(cq_entry.op_context));

        if (mpi_errno)
            MPIR_ERR_POP(mpi_errno);
    }

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_OFI_PROGRESS_DO_QUEUE);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_OFI_do_am_isend_header
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_OFI_do_am_isend_header(int rank,
                                               MPIR_Comm * comm,
                                               int handler_id,
                                               const void *am_hdr,
                                               size_t am_hdr_sz, MPIR_Request * sreq, int is_reply)
{
    struct iovec *iov;
    MPIDI_OFI_am_header_t *msg_hdr;
    int mpi_errno = MPI_SUCCESS, c;
    int need_lock = !is_reply;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_OFI_DO_AM_ISEND_HEADER);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_OFI_DO_AM_ISEND_HEADER);

    MPIDI_OFI_AMREQUEST(sreq, req_hdr) = NULL;
    mpi_errno = MPIDI_OFI_am_init_request(am_hdr, am_hdr_sz, sreq);

    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

    MPIR_Assert(handler_id < (1 << MPIDI_OFI_AM_HANDLER_ID_BITS));
    MPIR_Assert(am_hdr_sz < (1ULL << MPIDI_OFI_AM_HDR_SZ_BITS));
    msg_hdr = &MPIDI_OFI_AMREQUEST_HDR(sreq, msg_hdr);
    msg_hdr->handler_id = handler_id;
    msg_hdr->am_hdr_sz = am_hdr_sz;
    msg_hdr->data_sz = 0;
    msg_hdr->am_type = MPIDI_AMTYPE_SHORT_HDR;

    MPIR_Assert((uint64_t) comm->rank < (1ULL << MPIDI_OFI_AM_RANK_BITS));

    MPIDI_OFI_AMREQUEST_HDR(sreq, pack_buffer) = NULL;
    MPIR_cc_incr(sreq->cc_ptr, &c);

    iov = MPIDI_OFI_AMREQUEST_HDR(sreq, iov);

    iov[0].iov_base = msg_hdr;
    iov[0].iov_len = sizeof(*msg_hdr);

    MPIR_Assert((sizeof(*msg_hdr) + am_hdr_sz) <= MPIDI_OFI_DEFAULT_SHORT_SEND_SIZE);
    iov[1].iov_base = MPIDI_OFI_AMREQUEST_HDR(sreq, am_hdr);
    iov[1].iov_len = am_hdr_sz;
    MPIDI_OFI_AMREQUEST(sreq, event_id) = MPIDI_OFI_EVENT_AM_SEND;
    MPIDI_OFI_CALL_RETRY_AM(fi_sendv(MPIDI_Global.ctx[0].tx, iov, NULL, 2,
                                     MPIDI_OFI_comm_to_phys(comm, rank),
                                     &MPIDI_OFI_AMREQUEST(sreq, context)), need_lock, sendv);
  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_OFI_DO_AM_ISEND_HEADER);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_OFI_am_isend_long
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_OFI_am_isend_long(int rank,
                                          MPIR_Comm * comm,
                                          int handler_id,
                                          const void *am_hdr,
                                          size_t am_hdr_sz,
                                          const void *data,
                                          size_t data_sz, MPIR_Request * sreq, int need_lock)
{
    int mpi_errno = MPI_SUCCESS, c;
    MPIDI_OFI_am_header_t *msg_hdr;
    MPIDI_OFI_lmt_msg_payload_t *lmt_info;
    struct iovec *iov;
    uint64_t index;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_OFI_AM_ISEND_LONG);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_OFI_AM_ISEND_LONG);

    MPIR_Assert(handler_id < (1 << MPIDI_OFI_AM_HANDLER_ID_BITS));
    MPIR_Assert(am_hdr_sz < (1ULL << MPIDI_OFI_AM_HDR_SZ_BITS));
    MPIR_Assert(data_sz < (1ULL << MPIDI_OFI_AM_DATA_SZ_BITS));
    MPIR_Assert((uint64_t) comm->rank < (1ULL << MPIDI_OFI_AM_RANK_BITS));

    msg_hdr = &MPIDI_OFI_AMREQUEST_HDR(sreq, msg_hdr);
    msg_hdr->handler_id = handler_id;
    msg_hdr->am_hdr_sz = am_hdr_sz;
    msg_hdr->data_sz = data_sz;
    msg_hdr->am_type = MPIDI_AMTYPE_LMT_REQ;

    lmt_info = &MPIDI_OFI_AMREQUEST_HDR(sreq, lmt_info);
    lmt_info->context_id = comm->context_id;
    lmt_info->src_rank = comm->rank;
    lmt_info->src_offset = MPIDI_OFI_ENABLE_MR_SCALABLE ? (uint64_t) 0 /* MR_SCALABLE */ : (uint64_t) data;     /* MR_BASIC */
    lmt_info->sreq_ptr = (uint64_t) sreq;
    /* Always allocates RMA ID from COMM_WORLD as the actual associated communicator
     * is not available here */
    index =
        MPIDI_OFI_index_allocator_alloc(MPIDI_OFI_COMM(MPIR_Process.comm_world).rma_id_allocator);
    MPIR_Assert(index < MPIDI_Global.max_huge_rmas);
    lmt_info->rma_key = MPIDI_OFI_ENABLE_MR_SCALABLE ? index << MPIDI_Global.huge_rma_shift : 0;

    MPIR_cc_incr(sreq->cc_ptr, &c);     /* send completion */
    MPIR_cc_incr(sreq->cc_ptr, &c);     /* lmt ack handler */
    MPIR_Assert((sizeof(*msg_hdr) + sizeof(*lmt_info) + am_hdr_sz) <=
                MPIDI_OFI_DEFAULT_SHORT_SEND_SIZE);
    if (need_lock)
        MPIDI_OFI_CALL(fi_mr_reg(MPIDI_Global.domain,
                                 data,
                                 data_sz,
                                 FI_REMOTE_READ,
                                 0ULL,
                                 lmt_info->rma_key,
                                 0ULL, &MPIDI_OFI_AMREQUEST_HDR(sreq, lmt_mr), NULL), mr_reg);
    else
        MPIDI_OFI_CALL_NOLOCK(fi_mr_reg(MPIDI_Global.domain,
                                        data,
                                        data_sz,
                                        FI_REMOTE_READ,
                                        0ULL,
                                        lmt_info->rma_key,
                                        0ULL,
                                        &MPIDI_OFI_AMREQUEST_HDR(sreq, lmt_mr), NULL), mr_reg);
    OPA_incr_int(&MPIDI_Global.am_inflight_rma_send_mrs);

    if (!MPIDI_OFI_ENABLE_MR_SCALABLE) {
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
    MPIDI_OFI_CALL_RETRY_AM(fi_sendv(MPIDI_Global.ctx[0].tx, iov, NULL, 3,
                                     MPIDI_OFI_comm_to_phys(comm, rank),
                                     &MPIDI_OFI_AMREQUEST(sreq, context)), need_lock, sendv);
  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_OFI_AM_ISEND_LONG);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_OFI_am_isend_short
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_OFI_am_isend_short(int rank,
                                           MPIR_Comm * comm,
                                           int handler_id,
                                           const void *am_hdr,
                                           size_t am_hdr_sz,
                                           const void *data,
                                           MPI_Count count, MPIR_Request * sreq, int need_lock)
{
    int mpi_errno = MPI_SUCCESS, c;
    MPIDI_OFI_am_header_t *msg_hdr;
    struct iovec *iov;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_OFI_AM_ISEND_SHORT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_OFI_AM_ISEND_SHORT);

    MPIR_Assert(handler_id < (1 << MPIDI_OFI_AM_HANDLER_ID_BITS));
    MPIR_Assert(am_hdr_sz < (1ULL << MPIDI_OFI_AM_HDR_SZ_BITS));
    MPIR_Assert((uint64_t) count < (1ULL << MPIDI_OFI_AM_DATA_SZ_BITS));
    MPIR_Assert((uint64_t) comm->rank < (1ULL << MPIDI_OFI_AM_RANK_BITS));

    msg_hdr = &MPIDI_OFI_AMREQUEST_HDR(sreq, msg_hdr);
    msg_hdr->handler_id = handler_id;
    msg_hdr->am_hdr_sz = am_hdr_sz;
    msg_hdr->data_sz = count;
    msg_hdr->am_type = MPIDI_AMTYPE_SHORT;

    iov = MPIDI_OFI_AMREQUEST_HDR(sreq, iov);

    iov[0].iov_base = msg_hdr;
    iov[0].iov_len = sizeof(*msg_hdr);

    iov[1].iov_base = MPIDI_OFI_AMREQUEST_HDR(sreq, am_hdr);
    iov[1].iov_len = am_hdr_sz;

    iov[2].iov_base = (void *) data;
    iov[2].iov_len = count;

    MPIR_cc_incr(sreq->cc_ptr, &c);
    MPIDI_OFI_AMREQUEST(sreq, event_id) = MPIDI_OFI_EVENT_AM_SEND;
    MPIDI_OFI_CALL_RETRY_AM(fi_sendv(MPIDI_Global.ctx[0].tx, iov, NULL, 3,
                                     MPIDI_OFI_comm_to_phys(comm, rank),
                                     &MPIDI_OFI_AMREQUEST(sreq, context)), need_lock, sendv);
  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_OFI_AM_ISEND_SHORT);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_OFI_do_am_isend
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_OFI_do_am_isend(int rank,
                                        MPIR_Comm * comm,
                                        int handler_id,
                                        const void *am_hdr,
                                        size_t am_hdr_sz,
                                        const void *buf,
                                        size_t count,
                                        MPI_Datatype datatype, MPIR_Request * sreq, int is_reply)
{
    int dt_contig, mpi_errno = MPI_SUCCESS;
    char *send_buf;
    size_t data_sz;
    MPI_Aint dt_true_lb, last;
    MPIR_Datatype *dt_ptr;
    int need_lock = !is_reply;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_OFI_DO_AM_ISEND);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_OFI_DO_AM_ISEND);

    MPIDI_Datatype_get_info(count, datatype, dt_contig, data_sz, dt_ptr, dt_true_lb);
    send_buf = (char *) buf + dt_true_lb;

    if (handler_id == MPIDI_CH4U_SEND &&
        am_hdr_sz + data_sz + sizeof(MPIDI_OFI_am_header_t) > MPIDI_OFI_DEFAULT_SHORT_SEND_SIZE) {
        MPIDI_CH4U_send_long_req_msg_t lreq_hdr;

        MPIR_Memcpy(&lreq_hdr.hdr, am_hdr, am_hdr_sz);
        lreq_hdr.data_sz = data_sz;
        lreq_hdr.sreq_ptr = (uint64_t) sreq;
        MPIDI_CH4U_REQUEST(sreq, req->lreq).src_buf = buf;
        MPIDI_CH4U_REQUEST(sreq, req->lreq).count = count;
        dtype_add_ref_if_not_builtin(datatype);
        MPIDI_CH4U_REQUEST(sreq, req->lreq).datatype = datatype;
        MPIDI_CH4U_REQUEST(sreq, req->lreq).msg_tag = lreq_hdr.hdr.msg_tag;
        MPIDI_CH4U_REQUEST(sreq, rank) = rank;
        mpi_errno = MPIDI_NM_am_send_hdr(rank, comm, MPIDI_CH4U_SEND_LONG_REQ,
                                         &lreq_hdr, sizeof(lreq_hdr));
        if (mpi_errno)
            MPIR_ERR_POP(mpi_errno);
        goto fn_exit;
    }

    MPIDI_OFI_AMREQUEST(sreq, req_hdr) = NULL;
    mpi_errno = MPIDI_OFI_am_init_request(am_hdr, am_hdr_sz, sreq);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

    if (!dt_contig) {
        size_t segment_first;
        struct MPIR_Segment *segment_ptr;
        segment_ptr = MPIR_Segment_alloc();
        MPIR_ERR_CHKANDJUMP1(segment_ptr == NULL, mpi_errno,
                             MPI_ERR_OTHER, "**nomem", "**nomem %s", "Send MPIR_Segment_alloc");
        MPIR_Segment_init(buf, count, datatype, segment_ptr, 0);
        segment_first = 0;
        last = data_sz;
        MPIDI_OFI_AMREQUEST_HDR(sreq, pack_buffer) = (char *) MPL_malloc(data_sz);
        MPIR_ERR_CHKANDJUMP1(MPIDI_OFI_AMREQUEST_HDR(sreq, pack_buffer) == NULL, mpi_errno,
                             MPI_ERR_OTHER, "**nomem", "**nomem %s", "Send Pack buffer alloc");
        MPIR_Segment_pack(segment_ptr, segment_first, &last,
                           MPIDI_OFI_AMREQUEST_HDR(sreq, pack_buffer));
        MPIR_Segment_free(segment_ptr);
        send_buf = (char *) MPIDI_OFI_AMREQUEST_HDR(sreq, pack_buffer);
    }
    else {
        MPIDI_OFI_AMREQUEST_HDR(sreq, pack_buffer) = NULL;
    }

    if (am_hdr_sz + data_sz + sizeof(MPIDI_OFI_am_header_t) <= MPIDI_OFI_DEFAULT_SHORT_SEND_SIZE) {
        mpi_errno =
            MPIDI_OFI_am_isend_short(rank, comm, handler_id, MPIDI_OFI_AMREQUEST_HDR(sreq, am_hdr),
                                     am_hdr_sz, send_buf, data_sz, sreq, need_lock);
    }
    else {
        mpi_errno =
            MPIDI_OFI_am_isend_long(rank, comm, handler_id, MPIDI_OFI_AMREQUEST_HDR(sreq, am_hdr),
                                    am_hdr_sz, send_buf, data_sz, sreq, need_lock);
    }
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_OFI_DO_AM_ISEND);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_OFI_do_emulated_inject
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPIDI_OFI_do_emulated_inject(fi_addr_t addr,
                                               const MPIDI_OFI_am_header_t *msg_hdrp,
                                               const void *am_hdr,
                                               size_t am_hdr_sz,
                                               int need_lock)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Request *sreq;
    char *ibuf;
    size_t len;

    sreq = MPIR_Request_create(MPIR_REQUEST_KIND__SEND);
    MPIR_Assert(sreq);
    len = am_hdr_sz + sizeof(*msg_hdrp);
    ibuf = (char *) MPL_malloc(len);
    MPIR_Assert(ibuf);
    memcpy(ibuf, msg_hdrp, sizeof(*msg_hdrp));
    memcpy(ibuf + sizeof(*msg_hdrp), am_hdr, am_hdr_sz);

    MPIDI_OFI_REQUEST(sreq, event_id) = MPIDI_OFI_EVENT_INJECT_EMU;
    MPIDI_OFI_REQUEST(sreq, util.inject_buf) = ibuf;
    OPA_incr_int(&MPIDI_Global.am_inflight_inject_emus);

    MPIDI_OFI_CALL_RETRY_AM(fi_send(MPIDI_Global.ctx[0].tx, ibuf, len,
                                    NULL /* desc*/, addr, &(MPIDI_OFI_REQUEST(sreq, context))),
                                    need_lock, send);
  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_OFI_do_inject
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_OFI_do_inject(int rank,
                                      MPIR_Comm * comm,
                                      int handler_id,
                                      const void *am_hdr,
                                      size_t am_hdr_sz,
                                      int is_reply, int use_comm_table, int need_lock)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_OFI_am_header_t msg_hdr;
    fi_addr_t addr;
    char buff[MPIDI_Global.max_buffered_send];
    size_t buff_len;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_OFI_DO_INJECT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_OFI_DO_INJECT);

    MPIR_Assert(handler_id < (1 << MPIDI_OFI_AM_HANDLER_ID_BITS));
    MPIR_Assert(am_hdr_sz < (1ULL << MPIDI_OFI_AM_HDR_SZ_BITS));

    msg_hdr.handler_id = handler_id;
    msg_hdr.am_hdr_sz = am_hdr_sz;
    msg_hdr.data_sz = 0;
    msg_hdr.am_type = MPIDI_AMTYPE_SHORT_HDR;

    MPIR_Assert((uint64_t) comm->rank < (1ULL << MPIDI_OFI_AM_RANK_BITS));

    addr = use_comm_table ?
        MPIDI_OFI_comm_to_phys(comm, rank) :
        MPIDI_OFI_to_phys(rank);

    if (unlikely(am_hdr_sz + sizeof(msg_hdr) > MPIDI_Global.max_buffered_send)) {
        mpi_errno = MPIDI_OFI_do_emulated_inject(addr, &msg_hdr, am_hdr, am_hdr_sz, need_lock);
        goto fn_exit;
    }

    memcpy(buff, &msg_hdr, sizeof(msg_hdr));
    memcpy(buff + sizeof(msg_hdr), am_hdr, am_hdr_sz);
    buff_len = sizeof(msg_hdr) + am_hdr_sz;

    MPIDI_OFI_CALL_RETRY_AM(fi_inject(MPIDI_Global.ctx[0].tx, buff, buff_len, addr),
                            need_lock, inject);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_OFI_DO_INJECT);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#endif /* OFI_AM_IMPL_H_INCLUDED */
