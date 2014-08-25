/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2014 Mellanox Technologies, Inc.
 *
 */



#include "mxm_impl.h"

static int _mxm_poll(void);
static int _mxm_handle_rreq(MPID_Request * req);
static void _mxm_recv_completion_cb(void *context);
static int _mxm_irecv(MPID_nem_mxm_ep_t * ep, MPID_nem_mxm_req_area * req, int id, mxm_mq_h mxm_mq,
                      mxm_tag_t mxm_tag);
static int _mxm_process_rdtype(MPID_Request ** rreq_p, MPI_Datatype datatype,
                               MPID_Datatype * dt_ptr, MPIDI_msg_sz_t data_sz, const void *buf,
                               int count, mxm_req_buffer_t ** iov_buf, int *iov_count);

#undef FUNCNAME
#define FUNCNAME MPID_nem_mxm_poll
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_nem_mxm_poll(int in_blocking_progress)
{
    int mpi_errno = MPI_SUCCESS;

    MPIDI_STATE_DECL(MPID_STATE_MXM_POLL);
    MPIDI_FUNC_ENTER(MPID_STATE_MXM_POLL);

    mpi_errno = _mxm_poll();
    if (mpi_errno)
        MPIU_ERR_POP(mpi_errno);

  fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MXM_POLL);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}


static int _mxm_poll(void)
{
    int mpi_errno = MPI_SUCCESS;
    mxm_error_t ret = MXM_OK;

    ret = mxm_progress(mxm_obj->mxm_context);
    if ((MXM_OK != ret) && (MXM_ERR_NO_PROGRESS != ret)) {
        mpi_errno = MPI_ERR_OTHER;
        goto fn_fail;
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}


#undef FUNCNAME
#define FUNCNAME MPID_nem_mxm_get_adi_msg
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
void MPID_nem_mxm_get_adi_msg(mxm_conn_h conn, mxm_imm_t imm, void *data,
                              size_t length, size_t offset, int last)
{
    MPIDI_VC_t *vc = NULL;

    MPIU_DBG_MSG(CH3_CHANNEL, VERBOSE, "MPID_nem_mxm_get_adi_msg");

    vc = mxm_conn_ctx_get(conn);

    _dbg_mxm_output(5, "========> Getting ADI msg (from=%d data_size %d) \n", vc->pg_rank, length);

    MPID_nem_handle_pkt(vc, data, (MPIDI_msg_sz_t) (length));
}


#undef FUNCNAME
#define FUNCNAME MPID_nem_mxm_anysource_posted
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
void MPID_nem_mxm_anysource_posted(MPID_Request * req)
{
    int mpi_errno = MPI_SUCCESS;

    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_MXM_ANYSOURCE_POSTED);
    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_MXM_ANYSOURCE_POSTED);

    _dbg_mxm_output(5, "Any Source ========> Posting req %p \n", req);

    mpi_errno = MPID_nem_mxm_recv(NULL, req);
    MPIU_Assert(mpi_errno == MPI_SUCCESS);

    _dbg_mxm_out_req(req);

    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_MXM_ANYSOURCE_POSTED);
}


#undef FUNCNAME
#define FUNCNAME MPID_nem_mxm_anysource_matched
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_nem_mxm_anysource_matched(MPID_Request * req)
{
    int mpi_errno = MPI_SUCCESS;
    mxm_error_t ret = MXM_OK;
    int matched = FALSE;

    /* This function is called when an anysource request in the posted
     * receive queue is matched and dequeued see MPIDI_POSTED_RECV_DEQUEUE_HOOK().
     * It returns 0(FALSE) if the req was not matched by mxm and  non-zero(TRUE)
     * otherwise.
     * This happens
     * when the channel supports shared-memory and network communication
     * with a network capable of matching, and the same request is matched
     * by the network and, e.g., shared-memory.
     */
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_MXM_ANYSOURCE_MATCHED);
    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_MXM_ANYSOURCE_MATCHED);

    _dbg_mxm_output(5, "Any Source ========> Matching req %p \n", req);

    ret = mxm_req_cancel_recv(&REQ_FIELD(req, mxm_req->item.recv));
    if ((MXM_OK == ret) || (MXM_ERR_NO_PROGRESS == ret)) {
        MPID_Segment_free(req->dev.segment_ptr);
    }
    else {
        _mxm_req_wait(&REQ_FIELD(req, mxm_req->item.base));
        matched = TRUE;
    }

    _dbg_mxm_out_req(req);

    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_MXM_ANYSOURCE_MATCHED);
    return matched;
}


#undef FUNCNAME
#define FUNCNAME MPID_nem_mxm_recv
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_nem_mxm_recv(MPIDI_VC_t * vc, MPID_Request * rreq)
{
    int mpi_errno = MPI_SUCCESS;
    mxm_error_t ret = MXM_OK;
    mxm_recv_req_t *mxm_rreq;

    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_MXM_RECV);
    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_MXM_RECV);

    MPIU_Assert(rreq);
    MPIU_Assert(((rreq->dev.match.parts.rank == MPI_ANY_SOURCE) && (vc == NULL)) ||
                (vc && !vc->ch.is_local));

    {
        MPIR_Rank_t source = rreq->dev.match.parts.rank;
        MPIR_Context_id_t context_id = rreq->dev.match.parts.context_id;
        int tag = rreq->dev.match.parts.tag;
        int ret;
        MPIDI_msg_sz_t data_sz;
        int dt_contig;
        MPI_Aint dt_true_lb;
        MPID_Datatype *dt_ptr;

        MPIU_Assert((rreq->kind == MPID_REQUEST_RECV) || (rreq->kind == MPID_PREQUEST_RECV));
        MPIDI_Datatype_get_info(rreq->dev.user_count, rreq->dev.datatype, dt_contig, data_sz,
                                dt_ptr, dt_true_lb);
        rreq->dev.OnDataAvail = NULL;
        rreq->dev.tmpbuf = NULL;
        rreq->ch.vc = vc;

        _dbg_mxm_output(5,
                        "Recv ========> Getting USER msg for req %p (context %d rank %d tag %d size %d) \n",
                        rreq, context_id, source, tag, data_sz);

        REQ_FIELD(rreq, ctx) = rreq;
        REQ_FIELD(rreq, iov_buf) = REQ_FIELD(rreq, tmp_buf);
        REQ_FIELD(rreq, iov_count) = 0;
        REQ_FIELD(rreq, iov_buf)[0].ptr = NULL;
        REQ_FIELD(rreq, iov_buf)[0].length = 0;

        if (dt_contig) {
            REQ_FIELD(rreq, iov_count) = 1;
            REQ_FIELD(rreq, iov_buf)[0].ptr = (char *) (rreq->dev.user_buf) + dt_true_lb;
            REQ_FIELD(rreq, iov_buf)[0].length = data_sz;
        }
        else {
            mpi_errno = _mxm_process_rdtype(&rreq, rreq->dev.datatype, dt_ptr, data_sz,
                                            rreq->dev.user_buf, rreq->dev.user_count,
                                            &REQ_FIELD(rreq, iov_buf), &REQ_FIELD(rreq, iov_count));
            if (mpi_errno)
                MPIU_ERR_POP(mpi_errno);
        }

        mpi_errno = _mxm_irecv((vc ? VC_FIELD(vc, mxm_ep) : NULL), REQ_BASE(rreq),
                               tag,
                               (rreq->comm ? (mxm_mq_h) rreq->comm->dev.ch.netmod_priv : mxm_obj->
                                mxm_mq), _mxm_tag_mpi2mxm(tag, context_id));
        if (mpi_errno)
            MPIU_ERR_POP(mpi_errno);
    }

    if (vc)
        _dbg_mxm_out_req(rreq);

  fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_MXM_RECV);
    return mpi_errno;
  fn_fail:ATTRIBUTE((unused))
        goto fn_exit;
}


static int _mxm_handle_rreq(MPID_Request * req)
{
    int mpi_errno = MPI_SUCCESS;
    int complete = FALSE;
    int dt_contig;
    MPI_Aint dt_true_lb;
    MPIDI_msg_sz_t userbuf_sz;
    MPID_Datatype *dt_ptr;
    MPIDI_msg_sz_t data_sz;
    MPIDI_VC_t *vc = NULL;

    MPIU_THREAD_CS_ENTER(MSGQUEUE, req);
    complete = MPIDI_CH3U_Recvq_DP(req)
        MPIU_THREAD_CS_EXIT(MSGQUEUE, req);
    if (!complete) {
        return TRUE;
    }

    MPIDI_Datatype_get_info(req->dev.user_count, req->dev.datatype, dt_contig, userbuf_sz, dt_ptr,
                            dt_true_lb);

    _dbg_mxm_output(5, "========> Completing RECV req %p status %d\n", req, req->status.MPI_ERROR);
    _dbg_mxm_out_buf(REQ_FIELD(req, iov_buf)[0].ptr,
                     (REQ_FIELD(req, iov_buf)[0].length >
                      16 ? 16 : REQ_FIELD(req, iov_buf)[0].length));

    if (req->dev.recv_data_sz <= userbuf_sz) {
        data_sz = req->dev.recv_data_sz;
        if (req->status.MPI_ERROR == MPI_ERR_TRUNCATE) {
            req->status.MPI_ERROR = MPIR_Err_create_code(MPI_SUCCESS,
                                                         MPIR_ERR_RECOVERABLE, FCNAME, __LINE__,
                                                         MPI_ERR_TRUNCATE, "**truncate",
                                                         "**truncate %d %d %d %d",
                                                         req->status.MPI_SOURCE,
                                                         req->status.MPI_TAG, req->dev.recv_data_sz,
                                                         userbuf_sz);
        }
    }
    else {
        data_sz = userbuf_sz;
        MPIR_STATUS_SET_COUNT(req->status, userbuf_sz);
        MPIU_DBG_MSG_FMT(CH3_OTHER, VERBOSE, (MPIU_DBG_FDEST,
                                              "receive buffer too small; message truncated, msg_sz="
                                              MPIDI_MSG_SZ_FMT ", userbuf_sz="
                                              MPIDI_MSG_SZ_FMT, req->dev.recv_data_sz, userbuf_sz));
        req->status.MPI_ERROR = MPIR_Err_create_code(MPI_SUCCESS,
                                                     MPIR_ERR_RECOVERABLE, FCNAME, __LINE__,
                                                     MPI_ERR_TRUNCATE, "**truncate",
                                                     "**truncate %d %d %d %d",
                                                     req->status.MPI_SOURCE, req->status.MPI_TAG,
                                                     req->dev.recv_data_sz, userbuf_sz);
    }

    if ((!dt_contig) && (req->dev.tmpbuf != NULL)) {
        MPIDI_msg_sz_t last;

        last = req->dev.recv_data_sz;
        MPID_Segment_unpack(req->dev.segment_ptr, 0, &last, req->dev.tmpbuf);
        MPIU_Free(req->dev.tmpbuf);
        if (last != data_sz) {
            MPIR_STATUS_SET_COUNT(req->status, last);
            if (req->dev.recv_data_sz <= userbuf_sz) {
                MPIU_ERR_SETSIMPLE(req->status.MPI_ERROR, MPI_ERR_TYPE, "**dtypemismatch");
            }
        }
    }

    if (REQ_FIELD(req, iov_count) > MXM_MPICH_MAX_IOV) {
        MPIU_Free(REQ_FIELD(req, iov_buf));
        REQ_FIELD(req, iov_buf) = REQ_FIELD(req, tmp_buf);
        REQ_FIELD(req, iov_count) = 0;
    }

    MPIDI_CH3U_Handle_recv_req(vc, req, &complete);
    MPIU_Assert(complete == TRUE);

    return complete;
}


static void _mxm_recv_completion_cb(void *context)
{
    MPID_Request *req = (MPID_Request *) context;
    mxm_recv_req_t *mxm_rreq;

    MPIU_Assert(req);
    MPIU_Assert((req->kind == MPID_REQUEST_RECV) || (req->kind == MPID_PREQUEST_RECV));
    _dbg_mxm_out_req(req);

    _mxm_to_mpi_status(REQ_FIELD(req, mxm_req->item.base.error), &req->status);

    mxm_rreq = &REQ_FIELD(req, mxm_req->item.recv);
    req->status.MPI_TAG = _mxm_tag_mxm2mpi(mxm_rreq->completion.sender_tag);
    req->status.MPI_SOURCE = mxm_rreq->completion.sender_imm;
    req->dev.recv_data_sz = mxm_rreq->completion.actual_len;
    MPIR_STATUS_SET_COUNT(req->status, req->dev.recv_data_sz);

    if (req->ch.vc) {
        list_enqueue(&VC_FIELD(req->ch.vc, mxm_ep->free_queue), &REQ_FIELD(req, mxm_req->queue));
    }
    else {
        list_enqueue(&mxm_obj->free_queue, &REQ_FIELD(req, mxm_req->queue));
    }

    if (likely(!MPIR_STATUS_GET_CANCEL_BIT(req->status))) {
        _mxm_handle_rreq(req);
    }
}


static int _mxm_irecv(MPID_nem_mxm_ep_t * ep, MPID_nem_mxm_req_area * req, int id, mxm_mq_h mxm_mq,
                      mxm_tag_t mxm_tag)
{
    int mpi_errno = MPI_SUCCESS;
    mxm_error_t ret = MXM_OK;
    mxm_recv_req_t *mxm_rreq;
    list_head_t *free_queue = NULL;

    MPIU_Assert(req);

    free_queue = (ep ? &ep->free_queue : &mxm_obj->free_queue);
    req->mxm_req = list_dequeue_mxm_req(free_queue);
    if (!req->mxm_req) {
        list_grow_mxm_req(free_queue);
        req->mxm_req = list_dequeue_mxm_req(free_queue);
        if (!req->mxm_req) {
            MPIU_DBG_MSG(CH3_CHANNEL, VERBOSE, "empty free queue");
            mpi_errno = MPI_ERR_OTHER;
            goto fn_fail;
        }
    }
    mxm_rreq = &(req->mxm_req->item.recv);

    mxm_rreq->base.state = MXM_REQ_NEW;
    mxm_rreq->base.mq = mxm_mq;
    mxm_rreq->base.conn = (ep ? ep->mxm_conn : 0);
    mxm_rreq->base.completed_cb = _mxm_recv_completion_cb;
    mxm_rreq->base.context = req->ctx;

    mxm_rreq->tag = mxm_tag;
    mxm_rreq->tag_mask = _mxm_tag_mask(id);

    if (likely(req->iov_count == 1)) {
        mxm_rreq->base.data_type = MXM_REQ_DATA_BUFFER;
        mxm_rreq->base.data.buffer.ptr = req->iov_buf[0].ptr;
        mxm_rreq->base.data.buffer.length = req->iov_buf[0].length;
    }
    else {
        mxm_rreq->base.data_type = MXM_REQ_DATA_IOV;
        mxm_rreq->base.data.iov.vector = req->iov_buf;
        mxm_rreq->base.data.iov.count = req->iov_count;
    }

    ret = mxm_req_recv(mxm_rreq);
    if (MXM_OK != ret) {
        if (ep) {
            list_enqueue(&ep->free_queue, &req->mxm_req->queue);
        }
        else {
            list_enqueue(&mxm_obj->free_queue, &req->mxm_req->queue);
        }
        mpi_errno = MPI_ERR_OTHER;
        goto fn_fail;
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}


static int _mxm_process_rdtype(MPID_Request ** rreq_p, MPI_Datatype datatype,
                               MPID_Datatype * dt_ptr, MPIDI_msg_sz_t data_sz, const void *buf,
                               int count, mxm_req_buffer_t ** iov_buf, int *iov_count)
{
    int mpi_errno = MPI_SUCCESS;
    MPID_Request *rreq = *rreq_p;
    MPIDI_msg_sz_t last;
    MPID_IOV *iov;
    int n_iov = 0;
    int index;

    if (rreq->dev.segment_ptr == NULL) {
        rreq->dev.segment_ptr = MPID_Segment_alloc();
        MPIU_ERR_CHKANDJUMP1((rreq->dev.segment_ptr == NULL), mpi_errno, MPI_ERR_OTHER, "**nomem",
                             "**nomem %s", "MPID_Segment_alloc");
    }
    MPID_Segment_init(buf, count, datatype, rreq->dev.segment_ptr, 0);
    rreq->dev.segment_first = 0;
    rreq->dev.segment_size = data_sz;

    last = rreq->dev.segment_size;
    MPID_Segment_count_contig_blocks(rreq->dev.segment_ptr, rreq->dev.segment_first, &last,
                                     (MPI_Aint *) & n_iov);
    MPIU_Assert(n_iov > 0);
    iov = MPIU_Malloc(n_iov * sizeof(*iov));
    MPIU_Assert(iov);

    last = rreq->dev.segment_size;
    MPID_Segment_unpack_vector(rreq->dev.segment_ptr, rreq->dev.segment_first, &last, iov, &n_iov);
    MPIU_Assert(last == rreq->dev.segment_size);

    if (n_iov <= MXM_REQ_DATA_MAX_IOV) {
        if (n_iov > MXM_MPICH_MAX_IOV) {
            *iov_buf = (mxm_req_buffer_t *) MPIU_Malloc(n_iov * sizeof(**iov_buf));
            MPIU_Assert(*iov_buf);
        }

        for (index = 0; index < n_iov; index++) {
            (*iov_buf)[index].ptr = iov[index].MPID_IOV_BUF;
            (*iov_buf)[index].length = iov[index].MPID_IOV_LEN;
        }
        rreq->dev.tmpbuf = NULL;
        rreq->dev.tmpbuf_sz = 0;
        *iov_count = n_iov;
    }
    else {
        int packsize = 0;
        MPIR_Pack_size_impl(rreq->dev.user_count, rreq->dev.datatype, (MPI_Aint *) & packsize);
        rreq->dev.tmpbuf = MPIU_Malloc((size_t) packsize);
        MPIU_Assert(rreq->dev.tmpbuf);
        rreq->dev.tmpbuf_sz = packsize;
        (*iov_buf)[0].ptr = rreq->dev.tmpbuf;
        (*iov_buf)[0].length = (size_t) packsize;
        *iov_count = 1;
    }
    MPIU_Free(iov);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
