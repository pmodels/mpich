/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2014 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 *  Portion of this code were written by Mellanox Technologies, Inc.
 *  (C) 2014 Mellanox Technologies, Inc.
 *
 */



#include "mxm_impl.h"

static int _mxm_poll(void);
static int _mxm_handle_rreq(MPIR_Request * req);
static void _mxm_recv_completion_cb(void *context);
static int _mxm_irecv(MPID_nem_mxm_ep_t * ep, MPID_nem_mxm_req_area * req, int id, mxm_mq_h mxm_mq,
                      mxm_tag_t mxm_tag);
static int _mxm_process_rdtype(MPIR_Request ** rreq_p, MPI_Datatype datatype,
                               MPIR_Datatype* dt_ptr, intptr_t data_sz, const void *buf,
                               int count, mxm_req_buffer_t ** iov_buf, int *iov_count);

#undef FUNCNAME
#define FUNCNAME MPID_nem_mxm_poll
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPID_nem_mxm_poll(int in_blocking_progress)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Request *req = NULL;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MXM_POLL);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MXM_POLL);

    while (!MPID_nem_mxm_queue_empty(mxm_obj->sreq_queue)) {
        MPID_nem_mxm_queue_dequeue(&mxm_obj->sreq_queue, &req);
        _mxm_handle_sreq(req);
    }

    mpi_errno = _mxm_poll();
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MXM_POLL);
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
#define FCNAME MPL_QUOTE(FUNCNAME)
void MPID_nem_mxm_get_adi_msg(mxm_conn_h conn, mxm_imm_t imm, void *data,
                              size_t length, size_t offset, int last)
{
    MPIDI_VC_t *vc = NULL;

    MPL_DBG_MSG(MPIDI_CH3_DBG_CHANNEL, VERBOSE, "MPID_nem_mxm_get_adi_msg");

    vc = mxm_conn_ctx_get(conn);

    _dbg_mxm_output(5, "========> Getting ADI msg (from=%d data_size %d) \n", vc->pg_rank, length);
    _dbg_mxm_out_buf(data, (length > 16 ? 16 : length));

    MPID_nem_handle_pkt(vc, data, (intptr_t) (length));
}


#undef FUNCNAME
#define FUNCNAME MPID_nem_mxm_anysource_posted
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
void MPID_nem_mxm_anysource_posted(MPIR_Request * req)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_NEM_MXM_ANYSOURCE_POSTED);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_NEM_MXM_ANYSOURCE_POSTED);

    _dbg_mxm_output(5, "Any Source ========> Posting req %p \n", req);

    mpi_errno = MPID_nem_mxm_recv(NULL, req);
    MPIR_Assert(mpi_errno == MPI_SUCCESS);

    _dbg_mxm_out_req(req);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_NEM_MXM_ANYSOURCE_POSTED);
}


#undef FUNCNAME
#define FUNCNAME MPID_nem_mxm_anysource_matched
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPID_nem_mxm_anysource_matched(MPIR_Request * req)
{
    mxm_error_t ret = MXM_OK;
    MPID_nem_mxm_req_area *req_area = NULL;
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
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_NEM_MXM_ANYSOURCE_MATCHED);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_NEM_MXM_ANYSOURCE_MATCHED);

    _dbg_mxm_output(5, "Any Source ========> Matching req %p \n", req);

    req_area = REQ_BASE(req);
    ret = mxm_req_cancel_recv(&req_area->mxm_req->item.recv);
    mxm_req_wait(&req_area->mxm_req->item.base);
    if (req_area->mxm_req->item.base.error != MXM_ERR_CANCELED) {
        matched = TRUE;
    }

    _dbg_mxm_out_req(req);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_NEM_MXM_ANYSOURCE_MATCHED);
    return matched;
}


#undef FUNCNAME
#define FUNCNAME MPID_nem_mxm_recv
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPID_nem_mxm_recv(MPIDI_VC_t * vc, MPIR_Request * rreq)
{
    int mpi_errno = MPI_SUCCESS;
    intptr_t data_sz;
    int dt_contig;
    MPI_Aint dt_true_lb;
    MPIR_Datatype*dt_ptr;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_NEM_MXM_RECV);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_NEM_MXM_RECV);

    MPIR_Assert(rreq);
    MPIR_Assert(((rreq->dev.match.parts.rank == MPI_ANY_SOURCE) && (vc == NULL)) ||
                (vc && !vc->ch.is_local));

    MPIDI_Datatype_get_info(rreq->dev.user_count, rreq->dev.datatype, dt_contig, data_sz,
                            dt_ptr, dt_true_lb);

    {
        MPIR_Context_id_t context_id = rreq->dev.match.parts.context_id;
        int tag = rreq->dev.match.parts.tag;
        MPID_nem_mxm_vc_area *vc_area = NULL;
        MPID_nem_mxm_req_area *req_area = NULL;

        mxm_mq_h *mq_h_v = (mxm_mq_h *) rreq->comm->dev.ch.netmod_priv;
        rreq->dev.OnDataAvail = NULL;
        rreq->dev.tmpbuf = NULL;
        rreq->ch.vc = vc;
        rreq->ch.noncontig = FALSE;

        _dbg_mxm_output(5,
                        "Recv ========> Getting USER msg for req %p (context %d from %d tag %d size %d) \n",
                        rreq, context_id, rreq->dev.match.parts.rank, tag, data_sz);

        vc_area = VC_BASE(vc);
        req_area = REQ_BASE(rreq);

        req_area->ctx = rreq;
        req_area->iov_buf = req_area->tmp_buf;
        req_area->iov_count = 0;
        req_area->iov_buf[0].ptr = NULL;
        req_area->iov_buf[0].length = 0;

        if (dt_contig) {
            req_area->iov_count = 1;
            req_area->iov_buf[0].ptr = (char *) (rreq->dev.user_buf) + dt_true_lb;
            req_area->iov_buf[0].length = data_sz;
        }
        else {
            rreq->ch.noncontig = TRUE;
            mpi_errno = _mxm_process_rdtype(&rreq, rreq->dev.datatype, dt_ptr, data_sz,
                                            rreq->dev.user_buf, rreq->dev.user_count,
                                            &req_area->iov_buf, &req_area->iov_count);
            if (mpi_errno)
                MPIR_ERR_POP(mpi_errno);
        }

        mpi_errno = _mxm_irecv((vc_area ? vc_area->mxm_ep : NULL), req_area,
                               tag,
                               (rreq->comm ? mq_h_v[0] : mxm_obj->mxm_mq), _mxm_tag_mpi2mxm(tag,
                                                                                            context_id));
        if (mpi_errno)
            MPIR_ERR_POP(mpi_errno);
    }

    if (vc)
        _dbg_mxm_out_req(rreq);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_NEM_MXM_RECV);
    return mpi_errno;
  fn_fail:ATTRIBUTE((unused))
        goto fn_exit;
}


static int _mxm_handle_rreq(MPIR_Request * req)
{
    int complete = FALSE, found = FALSE;
    int dt_contig;
    MPI_Aint dt_true_lb ATTRIBUTE((unused));
    intptr_t userbuf_sz;
    MPIR_Datatype*dt_ptr;
    intptr_t data_sz;
    MPID_nem_mxm_vc_area *vc_area ATTRIBUTE((unused)) = NULL;
    MPID_nem_mxm_req_area *req_area = NULL;
    void *tmp_buf = NULL;

    MPID_THREAD_CS_ENTER(POBJ, MPIR_THREAD_POBJ_MSGQ_MUTEX);
    found = MPIDI_CH3U_Recvq_DP(req);
    MPID_THREAD_CS_EXIT(POBJ, MPIR_THREAD_POBJ_MSGQ_MUTEX);
    /* an MPI_ANY_SOURCE request may have been previously removed from the
     * CH3 queue by an FDP (find and dequeue posted) operation */
    if (req->dev.match.parts.rank != MPI_ANY_SOURCE) {
        MPIR_Assert(found);
    }

    MPIDI_Datatype_get_info(req->dev.user_count, req->dev.datatype, dt_contig, userbuf_sz, dt_ptr,
                            dt_true_lb);

    vc_area = VC_BASE(req->ch.vc);
    req_area = REQ_BASE(req);

    _dbg_mxm_out_buf(req_area->iov_buf[0].ptr,
                     (req_area->iov_buf[0].length > 16 ? 16 : req_area->iov_buf[0].length));

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
        MPL_DBG_MSG_FMT(MPIDI_CH3_DBG_OTHER, VERBOSE, (MPL_DBG_FDEST,
                                              "receive buffer too small; message truncated, msg_sz="
                                              PRIdPTR ", userbuf_sz="
                                              PRIdPTR, req->dev.recv_data_sz, userbuf_sz));
        req->status.MPI_ERROR = MPIR_Err_create_code(MPI_SUCCESS,
                                                     MPIR_ERR_RECOVERABLE, FCNAME, __LINE__,
                                                     MPI_ERR_TRUNCATE, "**truncate",
                                                     "**truncate %d %d %d %d",
                                                     req->status.MPI_SOURCE, req->status.MPI_TAG,
                                                     req->dev.recv_data_sz, userbuf_sz);
    }

    if (!dt_contig) {
        intptr_t last = 0;

        if (req->dev.tmpbuf != NULL) {
            last = req->dev.recv_data_sz;
            MPIR_Segment_unpack(req->dev.segment_ptr, 0, &last, req->dev.tmpbuf);
            tmp_buf = req->dev.tmpbuf;
        }
        else {
            mxm_req_buffer_t *iov_buf;
            MPL_IOV *iov;
            int n_iov = 0;
            int index;

            last = req->dev.recv_data_sz;
            n_iov = req_area->iov_count;
            iov_buf = req_area->iov_buf;
            if (last && n_iov > 0) {
                iov = MPL_malloc(n_iov * sizeof(*iov));
                MPIR_Assert(iov);

                for (index = 0; index < n_iov; index++) {
                    iov[index].MPL_IOV_BUF = iov_buf[index].ptr;
                    iov[index].MPL_IOV_LEN = iov_buf[index].length;
                }

                MPIR_Segment_unpack_vector(req->dev.segment_ptr, req->dev.segment_first, &last, iov,
                                           &n_iov);
                MPL_free(iov);
            }
            if (req_area->iov_count > MXM_MPICH_MAX_IOV) {
                tmp_buf = req_area->iov_buf;
                req_area->iov_buf = req_area->tmp_buf;
                req_area->iov_count = 0;
            }
        }
        if (last != data_sz) {
            MPIR_STATUS_SET_COUNT(req->status, last);
            if (req->dev.recv_data_sz <= userbuf_sz) {
                /* If the data can't be unpacked, the we have a
                 *  mismatch between the datatype and the amount of
                 *  data received.  Throw away received data.
                 */
                MPIR_ERR_SETSIMPLE(req->status.MPI_ERROR, MPI_ERR_TYPE, "**dtypemismatch");
            }
        }
    }

    MPIDI_CH3U_Handle_recv_req(req->ch.vc, req, &complete);
    MPIR_Assert(complete == TRUE);

    if (tmp_buf)
        MPL_free(tmp_buf);

    return complete;
}


static void _mxm_recv_completion_cb(void *context)
{
    MPIR_Request *req = (MPIR_Request *) context;
    mxm_recv_req_t *mxm_rreq;
    MPID_nem_mxm_req_area *req_area = NULL;

    MPIR_Assert(req);
    _dbg_mxm_out_req(req);

    req_area = REQ_BASE(req);
    _mxm_to_mpi_status(req_area->mxm_req->item.base.error, &req->status);

    mxm_rreq = &req_area->mxm_req->item.recv;
    req->status.MPI_TAG = _mxm_tag_mxm2mpi(mxm_rreq->completion.sender_tag);
    req->status.MPI_SOURCE = mxm_rreq->completion.sender_imm;
    req->dev.recv_data_sz = mxm_rreq->completion.actual_len;
    MPIR_STATUS_SET_COUNT(req->status, req->dev.recv_data_sz);

    if (req->ch.vc) {
        MPID_nem_mxm_vc_area *vc_area = VC_BASE(req->ch.vc);
        list_enqueue(&vc_area->mxm_ep->free_queue, &req_area->mxm_req->queue);
    }
    else {
        list_enqueue(&mxm_obj->free_queue, &req_area->mxm_req->queue);
    }

    _dbg_mxm_output(5, "========> %s RECV req %p status %d\n",
                    (MPIR_STATUS_GET_CANCEL_BIT(req->status) ? "Canceling" : "Completing"),
                    req, req->status.MPI_ERROR);

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

    MPIR_Assert(req);

    free_queue = (ep ? &ep->free_queue : &mxm_obj->free_queue);
    req->mxm_req = list_dequeue_mxm_req(free_queue);
    if (!req->mxm_req) {
        list_grow_mxm_req(free_queue);
        req->mxm_req = list_dequeue_mxm_req(free_queue);
        if (!req->mxm_req) {
            MPL_DBG_MSG(MPIDI_CH3_DBG_CHANNEL, VERBOSE, "empty free queue");
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
        list_enqueue(free_queue, &req->mxm_req->queue);
        mpi_errno = MPI_ERR_OTHER;
        goto fn_fail;
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}


static int _mxm_process_rdtype(MPIR_Request ** rreq_p, MPI_Datatype datatype,
                               MPIR_Datatype* dt_ptr, intptr_t data_sz, const void *buf,
                               int count, mxm_req_buffer_t ** iov_buf, int *iov_count)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Request *rreq = *rreq_p;
    intptr_t last;
    MPL_IOV *iov;
    int n_iov = 0;
    int index;

    if (rreq->dev.segment_ptr == NULL) {
        rreq->dev.segment_ptr = MPIR_Segment_alloc();
        MPIR_ERR_CHKANDJUMP1((rreq->dev.segment_ptr == NULL), mpi_errno, MPI_ERR_OTHER, "**nomem",
                             "**nomem %s", "MPIR_Segment_alloc");
    }
    MPIR_Segment_init(buf, count, datatype, rreq->dev.segment_ptr, 0);
    rreq->dev.segment_first = 0;
    rreq->dev.segment_size = data_sz;

    last = rreq->dev.segment_size;
    MPIR_Segment_count_contig_blocks(rreq->dev.segment_ptr, rreq->dev.segment_first, &last,
                                     (MPI_Aint *) & n_iov);
    MPIR_Assert(n_iov > 0);
    iov = MPL_malloc(n_iov * sizeof(*iov));
    MPIR_Assert(iov);

    last = rreq->dev.segment_size;
    MPIR_Segment_unpack_vector(rreq->dev.segment_ptr, rreq->dev.segment_first, &last, iov, &n_iov);
    MPIR_Assert(last == rreq->dev.segment_size);

#if defined(MXM_DEBUG) && (MXM_DEBUG > 0)
    _dbg_mxm_output(7, "Recv Noncontiguous data vector %i entries (free slots : %i)\n", n_iov,
                    MXM_REQ_DATA_MAX_IOV);
    for (index = 0; index < n_iov; index++) {
        _dbg_mxm_output(7, "======= Recv iov[%i] = ptr : %p, len : %i \n",
                        index, iov[index].MPL_IOV_BUF, iov[index].MPL_IOV_LEN);
    }
#endif

    if (n_iov <= MXM_REQ_DATA_MAX_IOV) {
        if (n_iov > MXM_MPICH_MAX_IOV) {
            *iov_buf = (mxm_req_buffer_t *) MPL_malloc(n_iov * sizeof(**iov_buf));
            MPIR_Assert(*iov_buf);
        }

        for (index = 0; index < n_iov; index++) {
            (*iov_buf)[index].ptr = iov[index].MPL_IOV_BUF;
            (*iov_buf)[index].length = iov[index].MPL_IOV_LEN;
        }
        rreq->dev.tmpbuf = NULL;
        rreq->dev.tmpbuf_sz = 0;
        *iov_count = n_iov;
    }
    else {
        MPI_Aint packsize = 0;
        MPIR_Pack_size_impl(rreq->dev.user_count, rreq->dev.datatype, &packsize);
        rreq->dev.tmpbuf = MPL_malloc((size_t) packsize);
        MPIR_Assert(rreq->dev.tmpbuf);
        rreq->dev.tmpbuf_sz = packsize;
        (*iov_buf)[0].ptr = rreq->dev.tmpbuf;
        (*iov_buf)[0].length = (size_t) packsize;
        *iov_count = 1;
    }
    MPL_free(iov);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
