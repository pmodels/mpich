/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"
#include "mpidrma.h"

static int create_derived_datatype(MPID_Request * req, MPIDI_RMA_dtype_info * dtype_info,
                                   MPID_Datatype ** dtp);

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3U_Handle_recv_req
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIDI_CH3U_Handle_recv_req(MPIDI_VC_t * vc, MPID_Request * rreq, int *complete)
{
    static int in_routine ATTRIBUTE((unused)) = FALSE;
    int mpi_errno = MPI_SUCCESS;
    int (*reqFn) (MPIDI_VC_t *, MPID_Request *, int *);
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3U_HANDLE_RECV_REQ);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3U_HANDLE_RECV_REQ);

    MPIU_Assert(in_routine == FALSE);
    in_routine = TRUE;

    reqFn = rreq->dev.OnDataAvail;
    if (!reqFn) {
        MPIU_Assert(MPIDI_Request_get_type(rreq) == MPIDI_REQUEST_TYPE_RECV);
        mpi_errno = MPID_Request_complete(rreq);
        if (mpi_errno != MPI_SUCCESS) {
            MPIR_ERR_POP(mpi_errno);
        }
        *complete = TRUE;
    }
    else {
        mpi_errno = reqFn(vc, rreq, complete);
    }

    in_routine = FALSE;

  fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3U_HANDLE_RECV_REQ);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

/* ----------------------------------------------------------------------- */
/* Here are the functions that implement the actions that are taken when
 * data is available for a receive request (or other completion operations)
 * These include "receive" requests that are part of the RMA implementation.
 *
 * The convention for the names of routines that are called when data is
 * available is
 *    MPIDI_CH3_ReqHandler_<type>(MPIDI_VC_t *, MPID_Request *, int *)
 * as in
 *    MPIDI_CH3_ReqHandler_...
 *
 * ToDo:
 *    We need a way for each of these functions to describe what they are,
 *    so that given a pointer to one of these functions, we can retrieve
 *    a description of the routine.  We may want to use a static string
 *    and require the user to maintain thread-safety, at least while
 *    accessing the string.
 */
/* ----------------------------------------------------------------------- */
int MPIDI_CH3_ReqHandler_RecvComplete(MPIDI_VC_t * vc ATTRIBUTE((unused)),
                                      MPID_Request * rreq, int *complete)
{
    int mpi_errno = MPI_SUCCESS;

    /* mark data transfer as complete and decrement CC */
    mpi_errno = MPID_Request_complete(rreq);
    if (mpi_errno != MPI_SUCCESS) {
        MPIR_ERR_POP(mpi_errno);
    }

    *complete = TRUE;

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3_ReqHandler_PutRecvComplete
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIDI_CH3_ReqHandler_PutRecvComplete(MPIDI_VC_t * vc, MPID_Request * rreq, int *complete)
{
    int mpi_errno = MPI_SUCCESS;
    MPID_Win *win_ptr;
    MPI_Win source_win_handle = rreq->dev.source_win_handle;
    MPIDI_CH3_Pkt_flags_t flags = rreq->dev.flags;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3_REQHANDLER_PUTRECVCOMPLETE);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3_REQHANDLER_PUTRECVCOMPLETE);

    /* NOTE: It is possible that this request is already completed before
     * entering this handler. This happens when this req handler is called
     * within the same req handler on the same request.
     * Consider this case: req is queued up in SHM queue with ref count of 2:
     * one is for completing the request and another is for dequeueing from
     * the queue. The first called req handler on this request completed
     * this request and decrement ref counter to 1. Request is still in the
     * queue. Within this handler, we call the req handler on the same request
     * for the second time (for example when making progress on SHM queue),
     * and the second called handler also tries to complete this request,
     * which leads to wrong execution.
     * Here we check if req is already completed to prevent processing the
     * same request twice. */
    if (MPID_Request_is_complete(rreq)) {
        *complete = FALSE;
        goto fn_exit;
    }

    MPID_Win_get_ptr(rreq->dev.target_win_handle, win_ptr);

    /* mark data transfer as complete and decrement CC */
    mpi_errno = MPID_Request_complete(rreq);
    if (mpi_errno != MPI_SUCCESS) {
        MPIR_ERR_POP(mpi_errno);
    }

    /* NOTE: finish_op_on_target() must be called after we complete this request,
     * because inside finish_op_on_target() we may call this request handler
     * on the same request again (in release_lock()). Marking this request as
     * completed will prevent us from processing the same request twice. */
    mpi_errno = finish_op_on_target(win_ptr, vc, FALSE /* has no response data */ ,
                                    flags, source_win_handle);
    if (mpi_errno != MPI_SUCCESS)
        MPIR_ERR_POP(mpi_errno);

    *complete = TRUE;

  fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3_REQHANDLER_PUTRECVCOMPLETE);
    return mpi_errno;

    /* --BEGIN ERROR HANDLING-- */
  fn_fail:
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}


#undef FUNCNAME
#define FUNCNAME MPIDI_CH3_ReqHandler_AccumRecvComplete
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIDI_CH3_ReqHandler_AccumRecvComplete(MPIDI_VC_t * vc, MPID_Request * rreq, int *complete)
{
    int mpi_errno = MPI_SUCCESS;
    MPID_Win *win_ptr;
    MPI_Win source_win_handle = rreq->dev.source_win_handle;
    MPIDI_CH3_Pkt_flags_t flags = rreq->dev.flags;
    MPI_Datatype basic_type;
    MPI_Aint predef_count, predef_dtp_size;
    MPI_Aint stream_offset;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3_REQHANDLER_ACCUMRECVCOMPLETE);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3_REQHANDLER_ACCUMRECVCOMPLETE);

    /* NOTE: It is possible that this request is already completed before
     * entering this handler. This happens when this req handler is called
     * within the same req handler on the same request.
     * Consider this case: req is queued up in SHM queue with ref count of 2:
     * one is for completing the request and another is for dequeueing from
     * the queue. The first called req handler on this request completed
     * this request and decrement ref counter to 1. Request is still in the
     * queue. Within this handler, we call the req handler on the same request
     * for the second time (for example when making progress on SHM queue),
     * and the second called handler also tries to complete this request,
     * which leads to wrong execution.
     * Here we check if req is already completed to prevent processing the
     * same request twice. */
    if (MPID_Request_is_complete(rreq)) {
        *complete = FALSE;
        goto fn_exit;
    }

    MPID_Win_get_ptr(rreq->dev.target_win_handle, win_ptr);

    MPIU_Assert(MPIDI_Request_get_type(rreq) == MPIDI_REQUEST_TYPE_ACCUM_RECV);

    if (MPIR_DATATYPE_IS_PREDEFINED(rreq->dev.datatype))
        basic_type = rreq->dev.datatype;
    else {
        basic_type = rreq->dev.datatype_ptr->basic_type;
    }
    MPIU_Assert(basic_type != MPI_DATATYPE_NULL);

    MPID_Datatype_get_size_macro(basic_type, predef_dtp_size);
    predef_count = rreq->dev.recv_data_sz / predef_dtp_size;
    MPIU_Assert(predef_count > 0);

    stream_offset = 0;
    MPIDI_CH3_ExtPkt_Accum_get_stream(flags,
                                      (!MPIR_DATATYPE_IS_PREDEFINED(rreq->dev.datatype)),
                                      rreq->dev.ext_hdr_ptr, &stream_offset);

    if (win_ptr->shm_allocated == TRUE)
        MPIDI_CH3I_SHM_MUTEX_LOCK(win_ptr);
    /* accumulate data from tmp_buf into user_buf */
    MPIU_Assert(predef_count == (int) predef_count);
    mpi_errno = do_accumulate_op(rreq->dev.user_buf, (int) predef_count, basic_type,
                                 rreq->dev.real_user_buf, rreq->dev.user_count, rreq->dev.datatype,
                                 stream_offset, rreq->dev.op);
    if (win_ptr->shm_allocated == TRUE)
        MPIDI_CH3I_SHM_MUTEX_UNLOCK(win_ptr);
    if (mpi_errno) {
        MPIR_ERR_POP(mpi_errno);
    }

    /* free the temporary buffer */
    MPIDI_CH3U_SRBuf_free(rreq);

    /* mark data transfer as complete and decrement CC */
    mpi_errno = MPID_Request_complete(rreq);
    if (mpi_errno != MPI_SUCCESS) {
        MPIR_ERR_POP(mpi_errno);
    }

    /* NOTE: finish_op_on_target() must be called after we complete this request,
     * because inside finish_op_on_target() we may call this request handler
     * on the same request again (in release_lock()). Marking this request as
     * completed will prevent us from processing the same request twice. */
    mpi_errno = finish_op_on_target(win_ptr, vc, FALSE /* has no response data */ ,
                                    flags, source_win_handle);
    if (mpi_errno != MPI_SUCCESS)
        MPIR_ERR_POP(mpi_errno);

    *complete = TRUE;

  fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3_REQHANDLER_ACCUMRECVCOMPLETE);
    return mpi_errno;

    /* --BEGIN ERROR HANDLING-- */
  fn_fail:
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}


#undef FUNCNAME
#define FUNCNAME MPIDI_CH3_ReqHandler_GaccumRecvComplete
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIDI_CH3_ReqHandler_GaccumRecvComplete(MPIDI_VC_t * vc, MPID_Request * rreq, int *complete)
{
    int mpi_errno = MPI_SUCCESS;
    MPID_Win *win_ptr;
    MPIDI_CH3_Pkt_t upkt;
    MPIDI_CH3_Pkt_get_accum_resp_t *get_accum_resp_pkt = &upkt.get_accum_resp;
    MPID_Request *resp_req;
    MPL_IOV iov[MPL_IOV_LIMIT];
    int iovcnt;
    int is_contig;
    MPI_Datatype basic_type;
    MPI_Aint predef_count, predef_dtp_size;
    MPI_Aint dt_true_lb;
    MPI_Aint stream_offset;
    int is_empty_origin = FALSE;
    MPI_Aint extent, type_size;
    MPI_Aint stream_data_len, total_len;
    MPIU_CHKPMEM_DECL(1);
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3_REQHANDLER_GACCUMRECVCOMPLETE);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3_REQHANDLER_GACCUMRECVCOMPLETE);

    /* Judge if origin buffer is empty */
    if (rreq->dev.op == MPI_NO_OP) {
        is_empty_origin = TRUE;
    }

    MPID_Win_get_ptr(rreq->dev.target_win_handle, win_ptr);

    if (MPIR_DATATYPE_IS_PREDEFINED(rreq->dev.datatype))
        basic_type = rreq->dev.datatype;
    else {
        basic_type = rreq->dev.datatype_ptr->basic_type;
    }
    MPIU_Assert(basic_type != MPI_DATATYPE_NULL);

    stream_offset = 0;
    MPIDI_CH3_ExtPkt_Gaccum_get_stream(rreq->dev.flags,
                                       (!MPIR_DATATYPE_IS_PREDEFINED(rreq->dev.datatype)),
                                       rreq->dev.ext_hdr_ptr, &stream_offset);

    /* Use target data to calculate current stream unit size */
    MPID_Datatype_get_size_macro(rreq->dev.datatype, type_size);
    total_len = type_size * rreq->dev.user_count;
    MPID_Datatype_get_size_macro(basic_type, predef_dtp_size);
    MPID_Datatype_get_extent_macro(basic_type, extent);
    stream_data_len = MPIR_MIN(total_len - (stream_offset / extent) * predef_dtp_size,
                               (MPIDI_CH3U_SRBuf_size / extent) * predef_dtp_size);

    predef_count = stream_data_len / predef_dtp_size;
    MPIU_Assert(predef_count > 0);

    MPIDI_Pkt_init(get_accum_resp_pkt, MPIDI_CH3_PKT_GET_ACCUM_RESP);
    get_accum_resp_pkt->request_handle = rreq->dev.resp_request_handle;
    get_accum_resp_pkt->target_rank = win_ptr->comm_ptr->rank;
    get_accum_resp_pkt->flags = MPIDI_CH3_PKT_FLAG_NONE;
    if (rreq->dev.flags & MPIDI_CH3_PKT_FLAG_RMA_LOCK_SHARED ||
        rreq->dev.flags & MPIDI_CH3_PKT_FLAG_RMA_LOCK_EXCLUSIVE)
        get_accum_resp_pkt->flags |= MPIDI_CH3_PKT_FLAG_RMA_LOCK_GRANTED;
    if ((rreq->dev.flags & MPIDI_CH3_PKT_FLAG_RMA_FLUSH) ||
        (rreq->dev.flags & MPIDI_CH3_PKT_FLAG_RMA_UNLOCK))
        get_accum_resp_pkt->flags |= MPIDI_CH3_PKT_FLAG_RMA_ACK;

    /* check if data is contiguous and get true lb */
    MPID_Datatype_is_contig(rreq->dev.datatype, &is_contig);
    MPID_Datatype_get_true_lb(rreq->dev.datatype, &dt_true_lb);

    resp_req = MPID_Request_create();
    MPIR_ERR_CHKANDJUMP(resp_req == NULL, mpi_errno, MPI_ERR_OTHER, "**nomemreq");
    MPIU_Object_set_ref(resp_req, 1);
    MPIDI_Request_set_type(resp_req, MPIDI_REQUEST_TYPE_GET_ACCUM_RESP);

    MPIU_CHKPMEM_MALLOC(resp_req->dev.user_buf, void *, stream_data_len,
                        mpi_errno, "GACC resp. buffer");

    /* NOTE: 'copy data + ACC' needs to be atomic */

    if (win_ptr->shm_allocated == TRUE)
        MPIDI_CH3I_SHM_MUTEX_LOCK(win_ptr);

    /* Copy data from target window to temporary buffer */

    if (is_contig) {
        MPIU_Memcpy(resp_req->dev.user_buf,
                    (void *) ((char *) rreq->dev.real_user_buf + dt_true_lb +
                              stream_offset), stream_data_len);
    }
    else {
        MPID_Segment *seg = MPID_Segment_alloc();
        MPI_Aint first = stream_offset;
        MPI_Aint last = first + stream_data_len;

        if (seg == NULL) {
            if (win_ptr->shm_allocated == TRUE)
                MPIDI_CH3I_SHM_MUTEX_UNLOCK(win_ptr);
        }
        MPIR_ERR_CHKANDJUMP1(seg == NULL, mpi_errno, MPI_ERR_OTHER, "**nomem", "**nomem %s",
                             "MPID_Segment");
        MPID_Segment_init(rreq->dev.real_user_buf, rreq->dev.user_count, rreq->dev.datatype, seg,
                          0);
        MPID_Segment_pack(seg, first, &last, resp_req->dev.user_buf);
        MPID_Segment_free(seg);
    }

    /* accumulate data from tmp_buf into user_buf */
    MPIU_Assert(predef_count == (int) predef_count);
    mpi_errno = do_accumulate_op(rreq->dev.user_buf, (int) predef_count, basic_type,
                                 rreq->dev.real_user_buf, rreq->dev.user_count, rreq->dev.datatype,
                                 stream_offset, rreq->dev.op);

    if (win_ptr->shm_allocated == TRUE)
        MPIDI_CH3I_SHM_MUTEX_UNLOCK(win_ptr);

    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

    resp_req->dev.OnFinal = MPIDI_CH3_ReqHandler_GaccumSendComplete;
    resp_req->dev.OnDataAvail = MPIDI_CH3_ReqHandler_GaccumSendComplete;
    resp_req->dev.target_win_handle = rreq->dev.target_win_handle;
    resp_req->dev.flags = rreq->dev.flags;

    /* here we increment the Active Target counter to guarantee the GET-like
     * operation are completed when counter reaches zero. */
    win_ptr->at_completion_counter++;

    iov[0].MPL_IOV_BUF = (MPL_IOV_BUF_CAST) get_accum_resp_pkt;
    iov[0].MPL_IOV_LEN = sizeof(*get_accum_resp_pkt);
    iov[1].MPL_IOV_BUF = (MPL_IOV_BUF_CAST) ((char *) resp_req->dev.user_buf);
    iov[1].MPL_IOV_LEN = stream_data_len;
    iovcnt = 2;

    MPID_THREAD_CS_ENTER(POBJ, vc->pobj_mutex);
    mpi_errno = MPIDI_CH3_iSendv(vc, resp_req, iov, iovcnt);
    MPID_THREAD_CS_EXIT(POBJ, vc->pobj_mutex);

    MPIR_ERR_CHKANDJUMP(mpi_errno != MPI_SUCCESS, mpi_errno, MPI_ERR_OTHER, "**ch3|rmamsg");

    /* Mark get portion as handled */
    rreq->dev.resp_request_handle = MPI_REQUEST_NULL;

    MPIU_Assert(MPIDI_Request_get_type(rreq) == MPIDI_REQUEST_TYPE_GET_ACCUM_RECV);

    if (is_empty_origin == FALSE) {
        /* free the temporary buffer.
         * When origin data is zero, there
         * is no temporary buffer allocated */
        MPIDI_CH3U_SRBuf_free(rreq);
    }

    /* mark data transfer as complete and decrement CC */
    mpi_errno = MPID_Request_complete(rreq);
    if (mpi_errno != MPI_SUCCESS) {
        MPIR_ERR_POP(mpi_errno);
    }

    *complete = TRUE;
  fn_exit:
    MPIU_CHKPMEM_COMMIT();
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3_REQHANDLER_GACCUMRECVCOMPLETE);
    return mpi_errno;

    /* --BEGIN ERROR HANDLING-- */
  fn_fail:
    MPIU_CHKPMEM_REAP();
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}


#undef FUNCNAME
#define FUNCNAME MPIDI_CH3_ReqHandler_FOPRecvComplete
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIDI_CH3_ReqHandler_FOPRecvComplete(MPIDI_VC_t * vc, MPID_Request * rreq, int *complete)
{
    int mpi_errno = MPI_SUCCESS;
    MPID_Win *win_ptr = NULL;
    MPI_Aint type_size;
    MPID_Request *resp_req = NULL;
    MPL_IOV iov[MPL_IOV_LIMIT];
    int iovcnt;
    MPIDI_CH3_Pkt_t upkt;
    MPIDI_CH3_Pkt_fop_resp_t *fop_resp_pkt = &upkt.fop_resp;
    int is_contig;
    int is_empty_origin = FALSE;
    MPIU_CHKPMEM_DECL(1);
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3_REQHANDLER_FOPRECVCOMPLETE);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3_REQHANDLER_FOPRECVCOMPLETE);

    /* Judge if origin buffer is empty */
    if (rreq->dev.op == MPI_NO_OP) {
        is_empty_origin = TRUE;
    }

    MPIU_Assert(MPIDI_Request_get_type(rreq) == MPIDI_REQUEST_TYPE_FOP_RECV);

    MPID_Win_get_ptr(rreq->dev.target_win_handle, win_ptr);

    MPID_Datatype_get_size_macro(rreq->dev.datatype, type_size);

    MPID_Datatype_is_contig(rreq->dev.datatype, &is_contig);

    /* Create response request */
    resp_req = MPID_Request_create();
    MPIR_ERR_CHKANDJUMP(resp_req == NULL, mpi_errno, MPI_ERR_OTHER, "**nomemreq");
    MPIDI_Request_set_type(resp_req, MPIDI_REQUEST_TYPE_FOP_RESP);
    MPIU_Object_set_ref(resp_req, 1);
    resp_req->dev.OnFinal = MPIDI_CH3_ReqHandler_FOPSendComplete;
    resp_req->dev.OnDataAvail = MPIDI_CH3_ReqHandler_FOPSendComplete;
    resp_req->dev.target_win_handle = rreq->dev.target_win_handle;
    resp_req->dev.flags = rreq->dev.flags;

    MPIU_CHKPMEM_MALLOC(resp_req->dev.user_buf, void *, type_size, mpi_errno, "FOP resp. buffer");

    /* here we increment the Active Target counter to guarantee the GET-like
     * operation are completed when counter reaches zero. */
    win_ptr->at_completion_counter++;

    /* NOTE: 'copy data + ACC' needs to be atomic */

    if (win_ptr->shm_allocated == TRUE)
        MPIDI_CH3I_SHM_MUTEX_LOCK(win_ptr);

    /* Copy data into a temporary buffer in response request */
    if (is_contig) {
        MPIU_Memcpy(resp_req->dev.user_buf, rreq->dev.real_user_buf, type_size);
    }
    else {
        MPID_Segment *seg = MPID_Segment_alloc();
        MPI_Aint last = type_size;

        if (seg == NULL) {
            if (win_ptr->shm_allocated == TRUE)
                MPIDI_CH3I_SHM_MUTEX_UNLOCK(win_ptr);
        }
        MPIR_ERR_CHKANDJUMP1(seg == NULL, mpi_errno, MPI_ERR_OTHER, "**nomem", "**nomem %s",
                             "MPID_Segment");
        MPID_Segment_init(rreq->dev.real_user_buf, 1, rreq->dev.datatype, seg, 0);
        MPID_Segment_pack(seg, 0, &last, resp_req->dev.user_buf);
        MPID_Segment_free(seg);
    }

    /* Perform accumulate computation */
    mpi_errno = do_accumulate_op(rreq->dev.user_buf, 1, rreq->dev.datatype,
                                 rreq->dev.real_user_buf, 1, rreq->dev.datatype, 0, rreq->dev.op);

    if (win_ptr->shm_allocated == TRUE)
        MPIDI_CH3I_SHM_MUTEX_UNLOCK(win_ptr);

    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

    /* Send back data */
    MPIDI_Pkt_init(fop_resp_pkt, MPIDI_CH3_PKT_FOP_RESP);
    fop_resp_pkt->request_handle = rreq->dev.resp_request_handle;
    fop_resp_pkt->target_rank = win_ptr->comm_ptr->rank;
    fop_resp_pkt->flags = MPIDI_CH3_PKT_FLAG_NONE;
    if (rreq->dev.flags & MPIDI_CH3_PKT_FLAG_RMA_LOCK_SHARED ||
        rreq->dev.flags & MPIDI_CH3_PKT_FLAG_RMA_LOCK_EXCLUSIVE)
        fop_resp_pkt->flags |= MPIDI_CH3_PKT_FLAG_RMA_LOCK_GRANTED;
    if ((rreq->dev.flags & MPIDI_CH3_PKT_FLAG_RMA_FLUSH) ||
        (rreq->dev.flags & MPIDI_CH3_PKT_FLAG_RMA_UNLOCK))
        fop_resp_pkt->flags |= MPIDI_CH3_PKT_FLAG_RMA_ACK;

    iov[0].MPL_IOV_BUF = (MPL_IOV_BUF_CAST) fop_resp_pkt;
    iov[0].MPL_IOV_LEN = sizeof(*fop_resp_pkt);
    iov[1].MPL_IOV_BUF = (MPL_IOV_BUF_CAST) ((char *) resp_req->dev.user_buf);
    iov[1].MPL_IOV_LEN = type_size;
    iovcnt = 2;

    MPID_THREAD_CS_ENTER(POBJ, vc->pobj_mutex);
    mpi_errno = MPIDI_CH3_iSendv(vc, resp_req, iov, iovcnt);
    MPID_THREAD_CS_EXIT(POBJ, vc->pobj_mutex);

    MPIR_ERR_CHKANDJUMP(mpi_errno != MPI_SUCCESS, mpi_errno, MPI_ERR_OTHER, "**ch3|rmamsg");

    if (is_empty_origin == FALSE) {
        /* free the temporary buffer.
         * When origin data is zero, there
         * is no temporary buffer allocated */
        MPIU_Free((char *) rreq->dev.user_buf);
    }

    /* mark data transfer as complete and decrement CC */
    mpi_errno = MPID_Request_complete(rreq);
    if (mpi_errno != MPI_SUCCESS) {
        MPIR_ERR_POP(mpi_errno);
    }

    *complete = TRUE;

  fn_exit:
    MPIU_CHKPMEM_COMMIT();
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3_REQHANDLER_FOPRECVCOMPLETE);
    return mpi_errno;
    /* --BEGIN ERROR HANDLING-- */
  fn_fail:
    MPIU_CHKPMEM_REAP();
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}


#undef FUNCNAME
#define FUNCNAME MPIDI_CH3_ReqHandler_PutDerivedDTRecvComplete
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIDI_CH3_ReqHandler_PutDerivedDTRecvComplete(MPIDI_VC_t * vc ATTRIBUTE((unused)),
                                                  MPID_Request * rreq, int *complete)
{
    int mpi_errno = MPI_SUCCESS;
    MPID_Datatype *new_dtp = NULL;
    MPIDI_RMA_dtype_info *dtype_info = NULL;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3_REQHANDLER_PUTDERIVEDDTRECVCOMPLETE);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3_REQHANDLER_PUTDERIVEDDTRECVCOMPLETE);

    /* get data from extended header */
    dtype_info = &((MPIDI_CH3_Ext_pkt_put_derived_t *) rreq->dev.ext_hdr_ptr)->dtype_info;
    /* create derived datatype */
    create_derived_datatype(rreq, dtype_info, &new_dtp);

    /* update request to get the data */
    MPIDI_Request_set_type(rreq, MPIDI_REQUEST_TYPE_PUT_RECV);
    rreq->dev.datatype = new_dtp->handle;
    rreq->dev.recv_data_sz = new_dtp->size * rreq->dev.user_count;

    rreq->dev.datatype_ptr = new_dtp;

    rreq->dev.segment_ptr = MPID_Segment_alloc();
    MPIR_ERR_CHKANDJUMP1((rreq->dev.segment_ptr == NULL), mpi_errno, MPI_ERR_OTHER, "**nomem",
                         "**nomem %s", "MPID_Segment_alloc");

    MPID_Segment_init(rreq->dev.user_buf,
                      rreq->dev.user_count, rreq->dev.datatype, rreq->dev.segment_ptr, 0);
    rreq->dev.segment_first = 0;
    rreq->dev.segment_size = rreq->dev.recv_data_sz;

    mpi_errno = MPIDI_CH3U_Request_load_recv_iov(rreq);
    if (mpi_errno != MPI_SUCCESS) {
        MPIR_ERR_SETANDJUMP(mpi_errno, MPI_ERR_OTHER, "**ch3|loadrecviov");
    }
    if (!rreq->dev.OnDataAvail)
        rreq->dev.OnDataAvail = MPIDI_CH3_ReqHandler_PutRecvComplete;

    *complete = FALSE;
  fn_fail:
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3_REQHANDLER_PUTDERIVEDDTRECVCOMPLETE);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3_ReqHandler_AccumMetadataRecvComplete
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIDI_CH3_ReqHandler_AccumMetadataRecvComplete(MPIDI_VC_t * vc ATTRIBUTE((unused)),
                                                   MPID_Request * rreq, int *complete)
{
    int mpi_errno = MPI_SUCCESS;
    MPID_Datatype *new_dtp = NULL;
    MPIDI_RMA_dtype_info *dtype_info = NULL;
    MPI_Aint basic_type_extent, basic_type_size;
    MPI_Aint total_len, rest_len, stream_elem_count;
    MPI_Aint stream_offset;
    MPI_Aint type_size;
    MPI_Datatype basic_dtp;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3_REQHANDLER_ACCUMMETADATARECVCOMPLETE);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3_REQHANDLER_ACCUMMETADATARECVCOMPLETE);

    stream_offset = 0;
    MPIU_Assert(rreq->dev.ext_hdr_ptr != NULL);

    if (MPIDI_Request_get_type(rreq) == MPIDI_REQUEST_TYPE_ACCUM_RECV_DERIVED_DT) {
        /* get data from extended header */
        if (rreq->dev.flags & MPIDI_CH3_PKT_FLAG_RMA_STREAM) {
            MPIDI_CH3_Ext_pkt_accum_stream_derived_t *ext_hdr = NULL;
            ext_hdr = ((MPIDI_CH3_Ext_pkt_accum_stream_derived_t *) rreq->dev.ext_hdr_ptr);
            stream_offset = ext_hdr->stream_offset;
            dtype_info = &ext_hdr->dtype_info;
        }
        else {
            MPIDI_CH3_Ext_pkt_accum_derived_t *ext_hdr = NULL;
            ext_hdr = ((MPIDI_CH3_Ext_pkt_accum_derived_t *) rreq->dev.ext_hdr_ptr);
            dtype_info = &ext_hdr->dtype_info;
        }

        /* create derived datatype */
        create_derived_datatype(rreq, dtype_info, &new_dtp);

        /* update new request to get the data */
        MPIDI_Request_set_type(rreq, MPIDI_REQUEST_TYPE_ACCUM_RECV);

        MPIU_Assert(rreq->dev.datatype == MPI_DATATYPE_NULL);
        rreq->dev.datatype = new_dtp->handle;
        rreq->dev.datatype_ptr = new_dtp;

        type_size = new_dtp->size;

        basic_dtp = new_dtp->basic_type;
    }
    else {
        MPIU_Assert(MPIDI_Request_get_type(rreq) == MPIDI_REQUEST_TYPE_ACCUM_RECV);
        MPIU_Assert(rreq->dev.datatype != MPI_DATATYPE_NULL);

        /* get data from extended header */
        if (rreq->dev.flags & MPIDI_CH3_PKT_FLAG_RMA_STREAM) {
            MPIDI_CH3_Ext_pkt_accum_stream_t *ext_hdr = NULL;
            ext_hdr = ((MPIDI_CH3_Ext_pkt_accum_stream_t *) rreq->dev.ext_hdr_ptr);
            stream_offset = ext_hdr->stream_offset;
        }

        MPID_Datatype_get_size_macro(rreq->dev.datatype, type_size);

        basic_dtp = rreq->dev.datatype;
    }

    MPID_Datatype_get_size_macro(basic_dtp, basic_type_size);
    MPID_Datatype_get_extent_macro(basic_dtp, basic_type_extent);

    MPIU_Assert(!MPIDI_Request_get_srbuf_flag(rreq));
    /* allocate a SRBuf for receiving stream unit */
    MPIDI_CH3U_SRBuf_alloc(rreq, MPIDI_CH3U_SRBuf_size);
    /* --BEGIN ERROR HANDLING-- */
    if (rreq->dev.tmpbuf_sz == 0) {
        MPIU_DBG_MSG(CH3_CHANNEL, TYPICAL, "SRBuf allocation failure");
        mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL,
                                         FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem",
                                         "**nomem %d", MPIDI_CH3U_SRBuf_size);
        rreq->status.MPI_ERROR = mpi_errno;
        goto fn_fail;
    }
    /* --END ERROR HANDLING-- */

    rreq->dev.user_buf = rreq->dev.tmpbuf;

    total_len = type_size * rreq->dev.user_count;
    rest_len = total_len - stream_offset;
    stream_elem_count = MPIDI_CH3U_SRBuf_size / basic_type_extent;

    rreq->dev.recv_data_sz = MPIR_MIN(rest_len, stream_elem_count * basic_type_size);

    rreq->dev.segment_ptr = MPID_Segment_alloc();
    MPIR_ERR_CHKANDJUMP1((rreq->dev.segment_ptr == NULL), mpi_errno, MPI_ERR_OTHER, "**nomem",
                         "**nomem %s", "MPID_Segment_alloc");

    MPID_Segment_init(rreq->dev.user_buf,
                      (rreq->dev.recv_data_sz / basic_type_size),
                      basic_dtp, rreq->dev.segment_ptr, 0);
    rreq->dev.segment_first = 0;
    rreq->dev.segment_size = rreq->dev.recv_data_sz;

    mpi_errno = MPIDI_CH3U_Request_load_recv_iov(rreq);
    if (mpi_errno != MPI_SUCCESS) {
        MPIR_ERR_SETANDJUMP(mpi_errno, MPI_ERR_OTHER, "**ch3|loadrecviov");
    }
    if (!rreq->dev.OnDataAvail)
        rreq->dev.OnDataAvail = MPIDI_CH3_ReqHandler_AccumRecvComplete;

    *complete = FALSE;
  fn_fail:
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3_REQHANDLER_ACCUMMETADATARECVCOMPLETE);
    return mpi_errno;
}


#undef FUNCNAME
#define FUNCNAME MPIDI_CH3_ReqHandler_GaccumMetadataRecvComplete
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIDI_CH3_ReqHandler_GaccumMetadataRecvComplete(MPIDI_VC_t * vc,
                                                    MPID_Request * rreq, int *complete)
{
    int mpi_errno = MPI_SUCCESS;
    MPID_Datatype *new_dtp = NULL;
    MPIDI_RMA_dtype_info *dtype_info = NULL;
    MPI_Aint basic_type_extent, basic_type_size;
    MPI_Aint total_len, rest_len, stream_elem_count;
    MPI_Aint stream_offset;
    MPI_Aint type_size;
    MPI_Datatype basic_dtp;
    int is_empty_origin = FALSE;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3_REQHANDLER_GACCUMMETADATARECVCOMPLETE);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3_REQHANDLER_GACCUMMETADATARECVCOMPLETE);

    /* Judge if origin buffer is empty */
    if (rreq->dev.op == MPI_NO_OP) {
        is_empty_origin = TRUE;
    }

    stream_offset = 0;
    MPIU_Assert(rreq->dev.ext_hdr_ptr != NULL);

    if (MPIDI_Request_get_type(rreq) == MPIDI_REQUEST_TYPE_GET_ACCUM_RECV_DERIVED_DT) {
        /* get data from extended header */
        if (rreq->dev.flags & MPIDI_CH3_PKT_FLAG_RMA_STREAM) {
            MPIDI_CH3_Ext_pkt_get_accum_stream_derived_t *ext_hdr = NULL;
            ext_hdr = ((MPIDI_CH3_Ext_pkt_get_accum_stream_derived_t *) rreq->dev.ext_hdr_ptr);
            stream_offset = ext_hdr->stream_offset;
            dtype_info = &ext_hdr->dtype_info;
        }
        else {
            MPIDI_CH3_Ext_pkt_get_accum_derived_t *ext_hdr = NULL;
            ext_hdr = ((MPIDI_CH3_Ext_pkt_get_accum_derived_t *) rreq->dev.ext_hdr_ptr);
            dtype_info = &ext_hdr->dtype_info;
        }

        /* create derived datatype */
        create_derived_datatype(rreq, dtype_info, &new_dtp);

        /* update new request to get the data */
        MPIDI_Request_set_type(rreq, MPIDI_REQUEST_TYPE_GET_ACCUM_RECV);

        MPIU_Assert(rreq->dev.datatype == MPI_DATATYPE_NULL);
        rreq->dev.datatype = new_dtp->handle;
        rreq->dev.datatype_ptr = new_dtp;

        type_size = new_dtp->size;

        basic_dtp = new_dtp->basic_type;
    }
    else {
        MPIU_Assert(MPIDI_Request_get_type(rreq) == MPIDI_REQUEST_TYPE_GET_ACCUM_RECV);
        MPIU_Assert(rreq->dev.datatype != MPI_DATATYPE_NULL);

        /* get data from extended header */
        if (rreq->dev.flags & MPIDI_CH3_PKT_FLAG_RMA_STREAM) {
            MPIDI_CH3_Ext_pkt_get_accum_stream_t *ext_hdr = NULL;
            ext_hdr = ((MPIDI_CH3_Ext_pkt_get_accum_stream_t *) rreq->dev.ext_hdr_ptr);
            stream_offset = ext_hdr->stream_offset;
        }

        MPID_Datatype_get_size_macro(rreq->dev.datatype, type_size);

        basic_dtp = rreq->dev.datatype;
    }

    if (is_empty_origin == TRUE) {
        rreq->dev.recv_data_sz = 0;

        /* There is no origin data coming, directly call final req handler. */
        mpi_errno = MPIDI_CH3_ReqHandler_GaccumRecvComplete(vc, rreq, complete);
        if (mpi_errno != MPI_SUCCESS)
            MPIR_ERR_POP(mpi_errno);
    }
    else {
        MPID_Datatype_get_size_macro(basic_dtp, basic_type_size);
        MPID_Datatype_get_extent_macro(basic_dtp, basic_type_extent);

        MPIU_Assert(!MPIDI_Request_get_srbuf_flag(rreq));
        /* allocate a SRBuf for receiving stream unit */
        MPIDI_CH3U_SRBuf_alloc(rreq, MPIDI_CH3U_SRBuf_size);
        /* --BEGIN ERROR HANDLING-- */
        if (rreq->dev.tmpbuf_sz == 0) {
            MPIU_DBG_MSG(CH3_CHANNEL, TYPICAL, "SRBuf allocation failure");
            mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL,
                                             FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem",
                                             "**nomem %d", MPIDI_CH3U_SRBuf_size);
            rreq->status.MPI_ERROR = mpi_errno;
            goto fn_fail;
        }
        /* --END ERROR HANDLING-- */

        rreq->dev.user_buf = rreq->dev.tmpbuf;

        total_len = type_size * rreq->dev.user_count;
        rest_len = total_len - stream_offset;
        stream_elem_count = MPIDI_CH3U_SRBuf_size / basic_type_extent;

        rreq->dev.recv_data_sz = MPIR_MIN(rest_len, stream_elem_count * basic_type_size);

        rreq->dev.segment_ptr = MPID_Segment_alloc();
        MPIR_ERR_CHKANDJUMP1((rreq->dev.segment_ptr == NULL), mpi_errno, MPI_ERR_OTHER, "**nomem",
                             "**nomem %s", "MPID_Segment_alloc");

        MPID_Segment_init(rreq->dev.user_buf,
                          (rreq->dev.recv_data_sz / basic_type_size),
                          basic_dtp, rreq->dev.segment_ptr, 0);
        rreq->dev.segment_first = 0;
        rreq->dev.segment_size = rreq->dev.recv_data_sz;

        mpi_errno = MPIDI_CH3U_Request_load_recv_iov(rreq);
        if (mpi_errno != MPI_SUCCESS) {
            MPIR_ERR_SETANDJUMP(mpi_errno, MPI_ERR_OTHER, "**ch3|loadrecviov");
        }
        if (!rreq->dev.OnDataAvail)
            rreq->dev.OnDataAvail = MPIDI_CH3_ReqHandler_GaccumRecvComplete;

        *complete = FALSE;
    }

  fn_fail:
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3_REQHANDLER_GACCUMMETADATARECVCOMPLETE);
    return mpi_errno;
}



#undef FUNCNAME
#define FUNCNAME MPIDI_CH3_ReqHandler_GetDerivedDTRecvComplete
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIDI_CH3_ReqHandler_GetDerivedDTRecvComplete(MPIDI_VC_t * vc,
                                                  MPID_Request * rreq, int *complete)
{
    int mpi_errno = MPI_SUCCESS;
    MPID_Datatype *new_dtp = NULL;
    MPIDI_RMA_dtype_info *dtype_info = NULL;
    MPIDI_CH3_Pkt_t upkt;
    MPIDI_CH3_Pkt_get_resp_t *get_resp_pkt = &upkt.get_resp;
    MPID_Request *sreq;
    MPID_Win *win_ptr;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3_REQHANDLER_GETDERIVEDDTRECVCOMPLETE);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3_REQHANDLER_GETDERIVEDDTRECVCOMPLETE);

    MPID_Win_get_ptr(rreq->dev.target_win_handle, win_ptr);

    MPIU_Assert(!(rreq->dev.flags & MPIDI_CH3_PKT_FLAG_RMA_IMMED_RESP));

    /* get data from extended header */
    dtype_info = &((MPIDI_CH3_Ext_pkt_get_derived_t *) rreq->dev.ext_hdr_ptr)->dtype_info;
    /* create derived datatype */
    create_derived_datatype(rreq, dtype_info, &new_dtp);

    /* create request for sending data */
    sreq = MPID_Request_create();
    MPIR_ERR_CHKANDJUMP(sreq == NULL, mpi_errno, MPI_ERR_OTHER, "**nomemreq");

    sreq->kind = MPID_REQUEST_SEND;
    MPIDI_Request_set_type(sreq, MPIDI_REQUEST_TYPE_GET_RESP);
    sreq->dev.OnDataAvail = MPIDI_CH3_ReqHandler_GetSendComplete;
    sreq->dev.OnFinal = MPIDI_CH3_ReqHandler_GetSendComplete;
    sreq->dev.user_buf = rreq->dev.user_buf;
    sreq->dev.user_count = rreq->dev.user_count;
    sreq->dev.datatype = new_dtp->handle;
    sreq->dev.datatype_ptr = new_dtp;
    sreq->dev.target_win_handle = rreq->dev.target_win_handle;
    sreq->dev.flags = rreq->dev.flags;

    MPIDI_Pkt_init(get_resp_pkt, MPIDI_CH3_PKT_GET_RESP);
    get_resp_pkt->request_handle = rreq->dev.request_handle;
    get_resp_pkt->target_rank = win_ptr->comm_ptr->rank;
    get_resp_pkt->flags = MPIDI_CH3_PKT_FLAG_NONE;
    if (rreq->dev.flags & MPIDI_CH3_PKT_FLAG_RMA_LOCK_SHARED ||
        rreq->dev.flags & MPIDI_CH3_PKT_FLAG_RMA_LOCK_EXCLUSIVE)
        get_resp_pkt->flags |= MPIDI_CH3_PKT_FLAG_RMA_LOCK_GRANTED;
    if ((rreq->dev.flags & MPIDI_CH3_PKT_FLAG_RMA_FLUSH) ||
        (rreq->dev.flags & MPIDI_CH3_PKT_FLAG_RMA_UNLOCK))
        get_resp_pkt->flags |= MPIDI_CH3_PKT_FLAG_RMA_ACK;

    sreq->dev.segment_ptr = MPID_Segment_alloc();
    MPIR_ERR_CHKANDJUMP1((sreq->dev.segment_ptr == NULL), mpi_errno, MPI_ERR_OTHER, "**nomem",
                         "**nomem %s", "MPID_Segment_alloc");

    MPID_Segment_init(sreq->dev.user_buf,
                      sreq->dev.user_count, sreq->dev.datatype, sreq->dev.segment_ptr, 0);
    sreq->dev.segment_first = 0;
    sreq->dev.segment_size = new_dtp->size * sreq->dev.user_count;

    /* Because this is in a packet handler, it is already within a critical section */
    /* MPID_THREAD_CS_ENTER(POBJ, vc->pobj_mutex); */
    mpi_errno = vc->sendNoncontig_fn(vc, sreq, get_resp_pkt, sizeof(*get_resp_pkt));
    /* MPID_THREAD_CS_EXIT(POBJ, vc->pobj_mutex); */
    /* --BEGIN ERROR HANDLING-- */
    if (mpi_errno != MPI_SUCCESS) {
        MPID_Request_release(sreq);
        sreq = NULL;
        MPIR_ERR_SETANDJUMP(mpi_errno, MPI_ERR_OTHER, "**ch3|rmamsg");
    }
    /* --END ERROR HANDLING-- */

    /* mark receive data transfer as complete and decrement CC in receive
     * request */
    mpi_errno = MPID_Request_complete(rreq);
    if (mpi_errno != MPI_SUCCESS) {
        MPIR_ERR_POP(mpi_errno);
    }

    *complete = TRUE;

  fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3_REQHANDLER_GETDERIVEDDTRECVCOMPLETE);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}


#undef FUNCNAME
#define FUNCNAME MPIDI_CH3_ReqHandler_UnpackUEBufComplete
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIDI_CH3_ReqHandler_UnpackUEBufComplete(MPIDI_VC_t * vc ATTRIBUTE((unused)),
                                             MPID_Request * rreq, int *complete)
{
    int recv_pending;
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3_REQHANDLER_UNPACKUEBUFCOMPLETE);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3_REQHANDLER_UNPACKUEBUFCOMPLETE);

    MPIDI_Request_decr_pending(rreq);
    MPIDI_Request_check_pending(rreq, &recv_pending);
    if (!recv_pending) {
        if (rreq->dev.recv_data_sz > 0) {
            MPIDI_CH3U_Request_unpack_uebuf(rreq);
            MPIU_Free(rreq->dev.tmpbuf);
        }
    }
    else {
        /* The receive has not been posted yet.  MPID_{Recv/Irecv}()
         * is responsible for unpacking the buffer. */
    }

    /* mark data transfer as complete and decrement CC */
    mpi_errno = MPID_Request_complete(rreq);
    if (mpi_errno != MPI_SUCCESS) {
        MPIR_ERR_POP(mpi_errno);
    }

    *complete = TRUE;

  fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3_REQHANDLER_UNPACKUEBUFCOMPLETE);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3_ReqHandler_UnpackSRBufComplete
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIDI_CH3_ReqHandler_UnpackSRBufComplete(MPIDI_VC_t * vc, MPID_Request * rreq, int *complete)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3_REQHANDLER_UNPACKSRBUFCOMPLETE);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3_REQHANDLER_UNPACKSRBUFCOMPLETE);

    MPIDI_CH3U_Request_unpack_srbuf(rreq);

    if (MPIDI_Request_get_type(rreq) == MPIDI_REQUEST_TYPE_PUT_RECV) {
        mpi_errno = MPIDI_CH3_ReqHandler_PutRecvComplete(vc, rreq, complete);
    }
    else if (MPIDI_Request_get_type(rreq) == MPIDI_REQUEST_TYPE_ACCUM_RECV) {
        mpi_errno = MPIDI_CH3_ReqHandler_AccumRecvComplete(vc, rreq, complete);
    }
    else if (MPIDI_Request_get_type(rreq) == MPIDI_REQUEST_TYPE_GET_ACCUM_RECV) {
        mpi_errno = MPIDI_CH3_ReqHandler_GaccumRecvComplete(vc, rreq, complete);
    }
    else if (MPIDI_Request_get_type(rreq) == MPIDI_REQUEST_TYPE_FOP_RECV) {
        mpi_errno = MPIDI_CH3_ReqHandler_FOPRecvComplete(vc, rreq, complete);
    }
    else {
        /* mark data transfer as complete and decrement CC */
        mpi_errno = MPID_Request_complete(rreq);
        if (mpi_errno != MPI_SUCCESS) {
            MPIR_ERR_POP(mpi_errno);
        }

        *complete = TRUE;
    }

  fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3_REQHANDLER_UNPACKSRBUFCOMPLETE);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3_ReqHandler_UnpackSRBufReloadIOV
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIDI_CH3_ReqHandler_UnpackSRBufReloadIOV(MPIDI_VC_t * vc ATTRIBUTE((unused)),
                                              MPID_Request * rreq, int *complete)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3_REQHANDLER_UNPACKSRBUFRELOADIOV);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3_REQHANDLER_UNPACKSRBUFRELOADIOV);

    MPIDI_CH3U_Request_unpack_srbuf(rreq);
    mpi_errno = MPIDI_CH3U_Request_load_recv_iov(rreq);
    if (mpi_errno != MPI_SUCCESS) {
        MPIR_ERR_SETFATALANDJUMP(mpi_errno, MPI_ERR_OTHER, "**ch3|loadrecviov");
    }
    *complete = FALSE;
  fn_fail:
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3_REQHANDLER_UNPACKSRBUFRELOADIOV);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3_ReqHandler_ReloadIOV
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIDI_CH3_ReqHandler_ReloadIOV(MPIDI_VC_t * vc ATTRIBUTE((unused)),
                                   MPID_Request * rreq, int *complete)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3_REQHANDLER_RELOADIOV);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3_REQHANDLER_RELOADIOV);

    mpi_errno = MPIDI_CH3U_Request_load_recv_iov(rreq);
    if (mpi_errno != MPI_SUCCESS) {
        MPIR_ERR_SETFATALANDJUMP(mpi_errno, MPI_ERR_OTHER, "**ch3|loadrecviov");
    }
    *complete = FALSE;
  fn_fail:
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3_REQHANDLER_RELOADIOV);
    return mpi_errno;
}

/* ----------------------------------------------------------------------- */
/* ----------------------------------------------------------------------- */

#undef FUNCNAME
#define FUNCNAME create_derived_datatype
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static int create_derived_datatype(MPID_Request * req, MPIDI_RMA_dtype_info * dtype_info,
                                   MPID_Datatype ** dtp)
{
    MPID_Datatype *new_dtp;
    int mpi_errno = MPI_SUCCESS;
    MPI_Aint ptrdiff;
    MPIDI_STATE_DECL(MPID_STATE_CREATE_DERIVED_DATATYPE);

    MPIDI_FUNC_ENTER(MPID_STATE_CREATE_DERIVED_DATATYPE);

    /* allocate new datatype object and handle */
    new_dtp = (MPID_Datatype *) MPIU_Handle_obj_alloc(&MPID_Datatype_mem);
    if (!new_dtp) {
        MPIR_ERR_SETANDJUMP1(mpi_errno, MPI_ERR_OTHER, "**nomem", "**nomem %s",
                             "MPID_Datatype_mem");
    }

    *dtp = new_dtp;

    /* Note: handle is filled in by MPIU_Handle_obj_alloc() */
    MPIU_Object_set_ref(new_dtp, 1);
    new_dtp->is_permanent = 0;
    new_dtp->is_committed = 1;
    new_dtp->attributes = 0;
    new_dtp->cache_id = 0;
    new_dtp->name[0] = 0;
    new_dtp->is_contig = dtype_info->is_contig;
    new_dtp->max_contig_blocks = dtype_info->max_contig_blocks;
    new_dtp->size = dtype_info->size;
    new_dtp->extent = dtype_info->extent;
    new_dtp->dataloop_size = dtype_info->dataloop_size;
    new_dtp->dataloop_depth = dtype_info->dataloop_depth;
    new_dtp->basic_type = dtype_info->basic_type;
    /* set dataloop pointer */
    new_dtp->dataloop = req->dev.dataloop;

    new_dtp->ub = dtype_info->ub;
    new_dtp->lb = dtype_info->lb;
    new_dtp->true_ub = dtype_info->true_ub;
    new_dtp->true_lb = dtype_info->true_lb;
    new_dtp->has_sticky_ub = dtype_info->has_sticky_ub;
    new_dtp->has_sticky_lb = dtype_info->has_sticky_lb;
    /* update pointers in dataloop */
    ptrdiff = (MPI_Aint) ((char *) (new_dtp->dataloop) - (char *)
                          (dtype_info->dataloop));

    /* FIXME: Temp to avoid SEGV when memory tracing */
    new_dtp->hetero_dloop = 0;

    MPID_Dataloop_update(new_dtp->dataloop, ptrdiff);

    new_dtp->contents = NULL;

  fn_fail:
    MPIDI_FUNC_EXIT(MPID_STATE_CREATE_DERIVED_DATATYPE);

    return mpi_errno;
}


static inline int perform_put_in_lock_queue(MPID_Win * win_ptr,
                                            MPIDI_RMA_Target_lock_entry_t * target_lock_entry)
{
    MPIDI_CH3_Pkt_put_t *put_pkt = &((target_lock_entry->pkt).put);
    int mpi_errno = MPI_SUCCESS;

    /* Piggyback candidate should have basic datatype for target datatype. */
    MPIU_Assert(MPIR_DATATYPE_IS_PREDEFINED(put_pkt->datatype));

    /* Make sure that all data is received for this op. */
    MPIU_Assert(target_lock_entry->all_data_recved == 1);

    if (put_pkt->type == MPIDI_CH3_PKT_PUT_IMMED) {
        /* all data fits in packet header */
        mpi_errno = MPIR_Localcopy(put_pkt->info.data, put_pkt->count, put_pkt->datatype,
                                   put_pkt->addr, put_pkt->count, put_pkt->datatype);
        if (mpi_errno != MPI_SUCCESS)
            MPIR_ERR_POP(mpi_errno);
    }
    else {
        MPIU_Assert(put_pkt->type == MPIDI_CH3_PKT_PUT);

        mpi_errno = MPIR_Localcopy(target_lock_entry->data, put_pkt->count, put_pkt->datatype,
                                   put_pkt->addr, put_pkt->count, put_pkt->datatype);
        if (mpi_errno != MPI_SUCCESS)
            MPIR_ERR_POP(mpi_errno);
    }

    /* do final action */
    mpi_errno =
        finish_op_on_target(win_ptr, target_lock_entry->vc, FALSE /* has no response data */ ,
                            put_pkt->flags, put_pkt->source_win_handle);
    if (mpi_errno != MPI_SUCCESS)
        MPIR_ERR_POP(mpi_errno);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static inline int perform_get_in_lock_queue(MPID_Win * win_ptr,
                                            MPIDI_RMA_Target_lock_entry_t * target_lock_entry)
{
    MPIDI_CH3_Pkt_t upkt;
    MPIDI_CH3_Pkt_get_resp_t *get_resp_pkt = &upkt.get_resp;
    MPIDI_CH3_Pkt_get_t *get_pkt = &((target_lock_entry->pkt).get);
    MPID_Request *sreq = NULL;
    MPI_Aint type_size;
    size_t len;
    int iovcnt;
    MPL_IOV iov[MPL_IOV_LIMIT];
    int is_contig;
    int mpi_errno = MPI_SUCCESS;

    /* Piggyback candidate should have basic datatype for target datatype. */
    MPIU_Assert(MPIR_DATATYPE_IS_PREDEFINED(get_pkt->datatype));

    /* Make sure that all data is received for this op. */
    MPIU_Assert(target_lock_entry->all_data_recved == 1);

    sreq = MPID_Request_create();
    if (sreq == NULL) {
        MPIR_ERR_SETANDJUMP(mpi_errno, MPI_ERR_OTHER, "**nomemreq");
    }
    MPIU_Object_set_ref(sreq, 1);

    MPIDI_Request_set_type(sreq, MPIDI_REQUEST_TYPE_GET_RESP);
    sreq->kind = MPID_REQUEST_SEND;
    sreq->dev.OnDataAvail = MPIDI_CH3_ReqHandler_GetSendComplete;
    sreq->dev.OnFinal = MPIDI_CH3_ReqHandler_GetSendComplete;

    sreq->dev.target_win_handle = win_ptr->handle;
    sreq->dev.flags = get_pkt->flags;

    /* here we increment the Active Target counter to guarantee the GET-like
     * operation are completed when counter reaches zero. */
    win_ptr->at_completion_counter++;

    if (get_pkt->flags & MPIDI_CH3_PKT_FLAG_RMA_IMMED_RESP) {
        MPIDI_Pkt_init(get_resp_pkt, MPIDI_CH3_PKT_GET_RESP_IMMED);
    }
    else {
        MPIDI_Pkt_init(get_resp_pkt, MPIDI_CH3_PKT_GET_RESP);
    }
    get_resp_pkt->request_handle = get_pkt->request_handle;
    get_resp_pkt->flags = MPIDI_CH3_PKT_FLAG_NONE;
    if (get_pkt->flags & MPIDI_CH3_PKT_FLAG_RMA_LOCK_SHARED ||
        get_pkt->flags & MPIDI_CH3_PKT_FLAG_RMA_LOCK_EXCLUSIVE)
        get_resp_pkt->flags |= MPIDI_CH3_PKT_FLAG_RMA_LOCK_GRANTED;
    if ((get_pkt->flags & MPIDI_CH3_PKT_FLAG_RMA_FLUSH) ||
        (get_pkt->flags & MPIDI_CH3_PKT_FLAG_RMA_UNLOCK))
        get_resp_pkt->flags |= MPIDI_CH3_PKT_FLAG_RMA_ACK;
    get_resp_pkt->target_rank = win_ptr->comm_ptr->rank;

    /* length of target data */
    MPID_Datatype_get_size_macro(get_pkt->datatype, type_size);
    MPIU_Assign_trunc(len, get_pkt->count * type_size, size_t);

    MPID_Datatype_is_contig(get_pkt->datatype, &is_contig);

    if (get_pkt->flags & MPIDI_CH3_PKT_FLAG_RMA_IMMED_RESP) {
        void *src = (void *) (get_pkt->addr), *dest = (void *) (get_resp_pkt->info.data);
        mpi_errno = immed_copy(src, dest, len);
        if (mpi_errno != MPI_SUCCESS)
            MPIR_ERR_POP(mpi_errno);

        /* All origin data is in packet header, issue the header. */
        iov[0].MPL_IOV_BUF = (MPL_IOV_BUF_CAST) get_resp_pkt;
        iov[0].MPL_IOV_LEN = sizeof(*get_resp_pkt);
        iovcnt = 1;

        mpi_errno = MPIDI_CH3_iSendv(target_lock_entry->vc, sreq, iov, iovcnt);
        if (mpi_errno != MPI_SUCCESS) {
            MPID_Request_release(sreq);
            MPIR_ERR_SETANDJUMP(mpi_errno, MPI_ERR_OTHER, "**ch3|rmamsg");
        }
    }
    else if (is_contig) {
        iov[0].MPL_IOV_BUF = (MPL_IOV_BUF_CAST) get_resp_pkt;
        iov[0].MPL_IOV_LEN = sizeof(*get_resp_pkt);
        iov[1].MPL_IOV_BUF = (MPL_IOV_BUF_CAST) (get_pkt->addr);
        iov[1].MPL_IOV_LEN = get_pkt->count * type_size;
        iovcnt = 2;

        mpi_errno = MPIDI_CH3_iSendv(target_lock_entry->vc, sreq, iov, iovcnt);
        if (mpi_errno != MPI_SUCCESS) {
            MPID_Request_release(sreq);
            MPIR_ERR_SETANDJUMP(mpi_errno, MPI_ERR_OTHER, "**ch3|rmamsg");
        }
    }
    else {
        iov[0].MPL_IOV_BUF = (MPL_IOV_BUF_CAST) get_resp_pkt;
        iov[0].MPL_IOV_LEN = sizeof(*get_resp_pkt);

        sreq->dev.segment_ptr = MPID_Segment_alloc();
        MPIR_ERR_CHKANDJUMP1(sreq->dev.segment_ptr == NULL, mpi_errno,
                             MPI_ERR_OTHER, "**nomem", "**nomem %s", "MPID_Segment_alloc");

        MPID_Segment_init(get_pkt->addr, get_pkt->count,
                          get_pkt->datatype, sreq->dev.segment_ptr, 0);
        sreq->dev.segment_first = 0;
        sreq->dev.segment_size = get_pkt->count * type_size;

        mpi_errno = target_lock_entry->vc->sendNoncontig_fn(target_lock_entry->vc, sreq,
                                                            iov[0].MPL_IOV_BUF, iov[0].MPL_IOV_LEN);
        MPIR_ERR_CHKANDJUMP(mpi_errno, mpi_errno, MPI_ERR_OTHER, "**ch3|rmamsg");
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}


static inline int perform_acc_in_lock_queue(MPID_Win * win_ptr,
                                            MPIDI_RMA_Target_lock_entry_t * target_lock_entry)
{
    MPIDI_CH3_Pkt_accum_t *acc_pkt = &((target_lock_entry->pkt).accum);
    int mpi_errno = MPI_SUCCESS;

    MPIU_Assert(target_lock_entry->all_data_recved == 1);

    /* Piggyback candidate should have basic datatype for target datatype. */
    MPIU_Assert(MPIR_DATATYPE_IS_PREDEFINED(acc_pkt->datatype));

    if (win_ptr->shm_allocated == TRUE)
        MPIDI_CH3I_SHM_MUTEX_LOCK(win_ptr);

    if (acc_pkt->type == MPIDI_CH3_PKT_ACCUMULATE_IMMED) {
        /* All data fits in packet header */
        mpi_errno = do_accumulate_op(acc_pkt->info.data, acc_pkt->count, acc_pkt->datatype,
                                     acc_pkt->addr, acc_pkt->count, acc_pkt->datatype,
                                     0, acc_pkt->op);
    }
    else {
        MPIU_Assert(acc_pkt->type == MPIDI_CH3_PKT_ACCUMULATE);
        MPI_Aint type_size, type_extent;
        MPI_Aint total_len, recv_count;

        MPID_Datatype_get_size_macro(acc_pkt->datatype, type_size);
        MPID_Datatype_get_extent_macro(acc_pkt->datatype, type_extent);

        total_len = type_size * acc_pkt->count;
        recv_count = MPIR_MIN((total_len / type_size), (MPIDI_CH3U_SRBuf_size / type_extent));
        MPIU_Assert(recv_count > 0);

        /* Note: here stream_offset is 0 because when piggybacking LOCK, we must use
         * the first stream unit. */
        MPIU_Assert(recv_count = (int) recv_count);
        mpi_errno = do_accumulate_op(target_lock_entry->data, (int) recv_count, acc_pkt->datatype,
                                     acc_pkt->addr, acc_pkt->count, acc_pkt->datatype,
                                     0, acc_pkt->op);
    }

    if (win_ptr->shm_allocated == TRUE)
        MPIDI_CH3I_SHM_MUTEX_UNLOCK(win_ptr);

    if (mpi_errno != MPI_SUCCESS)
        MPIR_ERR_POP(mpi_errno);

    mpi_errno =
        finish_op_on_target(win_ptr, target_lock_entry->vc, FALSE /* has no response data */ ,
                            acc_pkt->flags, acc_pkt->source_win_handle);
    if (mpi_errno != MPI_SUCCESS)
        MPIR_ERR_POP(mpi_errno);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}


static inline int perform_get_acc_in_lock_queue(MPID_Win * win_ptr,
                                                MPIDI_RMA_Target_lock_entry_t * target_lock_entry)
{
    MPIDI_CH3_Pkt_t upkt;
    MPIDI_CH3_Pkt_get_accum_resp_t *get_accum_resp_pkt = &upkt.get_accum_resp;
    MPIDI_CH3_Pkt_get_accum_t *get_accum_pkt = &((target_lock_entry->pkt).get_accum);
    MPID_Request *sreq = NULL;
    MPI_Aint type_size;
    size_t len;
    int iovcnt;
    MPL_IOV iov[MPL_IOV_LIMIT];
    int is_contig;
    int mpi_errno = MPI_SUCCESS;
    MPI_Aint type_extent;
    MPI_Aint total_len, recv_count;

    /* Piggyback candidate should have basic datatype for target datatype. */
    MPIU_Assert(MPIR_DATATYPE_IS_PREDEFINED(get_accum_pkt->datatype));

    /* Make sure that all data is received for this op. */
    MPIU_Assert(target_lock_entry->all_data_recved == 1);

    sreq = MPID_Request_create();
    if (sreq == NULL) {
        MPIR_ERR_SETANDJUMP(mpi_errno, MPI_ERR_OTHER, "**nomemreq");
    }
    MPIU_Object_set_ref(sreq, 1);

    MPIDI_Request_set_type(sreq, MPIDI_REQUEST_TYPE_GET_ACCUM_RESP);
    sreq->kind = MPID_REQUEST_SEND;
    sreq->dev.OnDataAvail = MPIDI_CH3_ReqHandler_GaccumSendComplete;
    sreq->dev.OnFinal = MPIDI_CH3_ReqHandler_GaccumSendComplete;

    sreq->dev.target_win_handle = win_ptr->handle;
    sreq->dev.flags = get_accum_pkt->flags;

    /* Copy data into a temporary buffer */
    MPID_Datatype_get_size_macro(get_accum_pkt->datatype, type_size);

    /* length of target data */
    MPIU_Assign_trunc(len, get_accum_pkt->count * type_size, size_t);

    if (get_accum_pkt->type == MPIDI_CH3_PKT_GET_ACCUM_IMMED) {
        MPIDI_Pkt_init(get_accum_resp_pkt, MPIDI_CH3_PKT_GET_ACCUM_RESP_IMMED);
        get_accum_resp_pkt->request_handle = get_accum_pkt->request_handle;
        get_accum_resp_pkt->flags = MPIDI_CH3_PKT_FLAG_NONE;
        if (get_accum_pkt->flags & MPIDI_CH3_PKT_FLAG_RMA_LOCK_SHARED ||
            get_accum_pkt->flags & MPIDI_CH3_PKT_FLAG_RMA_LOCK_EXCLUSIVE)
            get_accum_resp_pkt->flags |= MPIDI_CH3_PKT_FLAG_RMA_LOCK_GRANTED;
        if ((get_accum_pkt->flags & MPIDI_CH3_PKT_FLAG_RMA_FLUSH) ||
            (get_accum_pkt->flags & MPIDI_CH3_PKT_FLAG_RMA_UNLOCK))
            get_accum_resp_pkt->flags |= MPIDI_CH3_PKT_FLAG_RMA_ACK;
        get_accum_resp_pkt->target_rank = win_ptr->comm_ptr->rank;


        /* NOTE: copy 'data + ACC' needs to be atomic */

        if (win_ptr->shm_allocated == TRUE)
            MPIDI_CH3I_SHM_MUTEX_LOCK(win_ptr);

        /* Copy data from target window to response packet header */

        void *src = (void *) (get_accum_pkt->addr), *dest =
            (void *) (get_accum_resp_pkt->info.data);
        mpi_errno = immed_copy(src, dest, len);
        if (mpi_errno != MPI_SUCCESS) {
            if (win_ptr->shm_allocated == TRUE)
                MPIDI_CH3I_SHM_MUTEX_UNLOCK(win_ptr);
            MPIR_ERR_POP(mpi_errno);
        }

        /* Perform ACCUMULATE OP */

        /* All data fits in packet header */
        mpi_errno =
            do_accumulate_op(get_accum_pkt->info.data, get_accum_pkt->count,
                             get_accum_pkt->datatype, get_accum_pkt->addr, get_accum_pkt->count,
                             get_accum_pkt->datatype, 0, get_accum_pkt->op);

        if (win_ptr->shm_allocated == TRUE)
            MPIDI_CH3I_SHM_MUTEX_UNLOCK(win_ptr);

        if (mpi_errno != MPI_SUCCESS)
            MPIR_ERR_POP(mpi_errno);

        /* here we increment the Active Target counter to guarantee the GET-like
         * operation are completed when counter reaches zero. */
        win_ptr->at_completion_counter++;

        /* All origin data is in packet header, issue the header. */
        iov[0].MPL_IOV_BUF = (MPL_IOV_BUF_CAST) get_accum_resp_pkt;
        iov[0].MPL_IOV_LEN = sizeof(*get_accum_resp_pkt);
        iovcnt = 1;

        mpi_errno = MPIDI_CH3_iSendv(target_lock_entry->vc, sreq, iov, iovcnt);
        if (mpi_errno != MPI_SUCCESS) {
            MPID_Request_release(sreq);
            MPIR_ERR_SETANDJUMP(mpi_errno, MPI_ERR_OTHER, "**ch3|rmamsg");
        }

        goto fn_exit;
    }

    MPIU_Assert(get_accum_pkt->type == MPIDI_CH3_PKT_GET_ACCUM);

    MPID_Datatype_get_extent_macro(get_accum_pkt->datatype, type_extent);

    total_len = type_size * get_accum_pkt->count;
    recv_count = MPIR_MIN((total_len / type_size), (MPIDI_CH3U_SRBuf_size / type_extent));
    MPIU_Assert(recv_count > 0);

    sreq->dev.user_buf = (void *) MPIU_Malloc(recv_count * type_size);

    MPID_Datatype_is_contig(get_accum_pkt->datatype, &is_contig);

    /* NOTE: 'copy data + ACC' needs to be atomic */

    if (win_ptr->shm_allocated == TRUE)
        MPIDI_CH3I_SHM_MUTEX_LOCK(win_ptr);

    /* Copy data from target window to temporary buffer */

    /* Note: here stream_offset is 0 because when piggybacking LOCK, we must use
     * the first stream unit. */

    if (is_contig) {
        MPIU_Memcpy(sreq->dev.user_buf, get_accum_pkt->addr, recv_count * type_size);
    }
    else {
        MPID_Segment *seg = MPID_Segment_alloc();
        MPI_Aint first = 0;
        MPI_Aint last = first + type_size * recv_count;

        if (seg == NULL) {
            if (win_ptr->shm_allocated == TRUE)
                MPIDI_CH3I_SHM_MUTEX_UNLOCK(win_ptr);
        }
        MPIR_ERR_CHKANDJUMP1(seg == NULL, mpi_errno, MPI_ERR_OTHER, "**nomem", "**nomem %s",
                             "MPID_Segment");
        MPID_Segment_init(get_accum_pkt->addr, get_accum_pkt->count, get_accum_pkt->datatype, seg,
                          0);
        MPID_Segment_pack(seg, first, &last, sreq->dev.user_buf);
        MPID_Segment_free(seg);
    }

    /* Perform ACCUMULATE OP */

    MPIU_Assert(recv_count == (int) recv_count);
    mpi_errno = do_accumulate_op(target_lock_entry->data, (int) recv_count, get_accum_pkt->datatype,
                                 get_accum_pkt->addr, get_accum_pkt->count, get_accum_pkt->datatype,
                                 0, get_accum_pkt->op);

    if (win_ptr->shm_allocated == TRUE)
        MPIDI_CH3I_SHM_MUTEX_UNLOCK(win_ptr);

    if (mpi_errno != MPI_SUCCESS)
        MPIR_ERR_POP(mpi_errno);

    /* here we increment the Active Target counter to guarantee the GET-like
     * operation are completed when counter reaches zero. */
    win_ptr->at_completion_counter++;

    MPIDI_Pkt_init(get_accum_resp_pkt, MPIDI_CH3_PKT_GET_ACCUM_RESP);
    get_accum_resp_pkt->request_handle = get_accum_pkt->request_handle;
    get_accum_resp_pkt->flags = MPIDI_CH3_PKT_FLAG_NONE;
    if (get_accum_pkt->flags & MPIDI_CH3_PKT_FLAG_RMA_LOCK_SHARED ||
        get_accum_pkt->flags & MPIDI_CH3_PKT_FLAG_RMA_LOCK_EXCLUSIVE)
        get_accum_resp_pkt->flags |= MPIDI_CH3_PKT_FLAG_RMA_LOCK_GRANTED;
    if ((get_accum_pkt->flags & MPIDI_CH3_PKT_FLAG_RMA_FLUSH) ||
        (get_accum_pkt->flags & MPIDI_CH3_PKT_FLAG_RMA_UNLOCK))
        get_accum_resp_pkt->flags |= MPIDI_CH3_PKT_FLAG_RMA_ACK;
    get_accum_resp_pkt->target_rank = win_ptr->comm_ptr->rank;

    iov[0].MPL_IOV_BUF = (MPL_IOV_BUF_CAST) get_accum_resp_pkt;
    iov[0].MPL_IOV_LEN = sizeof(*get_accum_resp_pkt);
    iov[1].MPL_IOV_BUF = (MPL_IOV_BUF_CAST) ((char *) sreq->dev.user_buf);
    iov[1].MPL_IOV_LEN = recv_count * type_size;
    iovcnt = 2;

    mpi_errno = MPIDI_CH3_iSendv(target_lock_entry->vc, sreq, iov, iovcnt);
    if (mpi_errno != MPI_SUCCESS) {
        MPID_Request_release(sreq);
        MPIR_ERR_SETANDJUMP(mpi_errno, MPI_ERR_OTHER, "**ch3|rmamsg");
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}


static inline int perform_fop_in_lock_queue(MPID_Win * win_ptr,
                                            MPIDI_RMA_Target_lock_entry_t * target_lock_entry)
{
    MPIDI_CH3_Pkt_t upkt;
    MPIDI_CH3_Pkt_fop_resp_t *fop_resp_pkt = &upkt.fop_resp;
    MPIDI_CH3_Pkt_fop_t *fop_pkt = &((target_lock_entry->pkt).fop);
    MPID_Request *resp_req = NULL;
    MPI_Aint type_size;
    MPL_IOV iov[MPL_IOV_LIMIT];
    int iovcnt;
    int is_contig;
    int mpi_errno = MPI_SUCCESS;

    /* Piggyback candidate should have basic datatype for target datatype. */
    MPIU_Assert(MPIR_DATATYPE_IS_PREDEFINED(fop_pkt->datatype));

    /* Make sure that all data is received for this op. */
    MPIU_Assert(target_lock_entry->all_data_recved == 1);

    /* FIXME: this function is same with PktHandler_FOP(), should
     * do code refactoring on both of them. */

    MPID_Datatype_get_size_macro(fop_pkt->datatype, type_size);

    MPID_Datatype_is_contig(fop_pkt->datatype, &is_contig);

    if (fop_pkt->type == MPIDI_CH3_PKT_FOP_IMMED) {
        MPIDI_Pkt_init(fop_resp_pkt, MPIDI_CH3_PKT_FOP_RESP_IMMED);
    }
    else {
        MPIDI_Pkt_init(fop_resp_pkt, MPIDI_CH3_PKT_FOP_RESP);
    }

    fop_resp_pkt->request_handle = fop_pkt->request_handle;
    fop_resp_pkt->target_rank = win_ptr->comm_ptr->rank;
    fop_resp_pkt->flags = MPIDI_CH3_PKT_FLAG_NONE;
    if (fop_pkt->flags & MPIDI_CH3_PKT_FLAG_RMA_LOCK_SHARED ||
        fop_pkt->flags & MPIDI_CH3_PKT_FLAG_RMA_LOCK_EXCLUSIVE)
        fop_resp_pkt->flags |= MPIDI_CH3_PKT_FLAG_RMA_LOCK_GRANTED;
    if ((fop_pkt->flags & MPIDI_CH3_PKT_FLAG_RMA_FLUSH) ||
        (fop_pkt->flags & MPIDI_CH3_PKT_FLAG_RMA_UNLOCK))
        fop_resp_pkt->flags |= MPIDI_CH3_PKT_FLAG_RMA_ACK;

    if (fop_pkt->type == MPIDI_CH3_PKT_FOP) {
        resp_req = MPID_Request_create();
        if (resp_req == NULL) {
            MPIR_ERR_SETANDJUMP(mpi_errno, MPI_ERR_OTHER, "**nomemreq");
        }
        MPIU_Object_set_ref(resp_req, 1);

        resp_req->dev.OnDataAvail = MPIDI_CH3_ReqHandler_FOPSendComplete;
        resp_req->dev.OnFinal = MPIDI_CH3_ReqHandler_FOPSendComplete;

        resp_req->dev.target_win_handle = win_ptr->handle;
        resp_req->dev.flags = fop_pkt->flags;

        resp_req->dev.user_buf = (void *) MPIU_Malloc(type_size);

        /* here we increment the Active Target counter to guarantee the GET-like
         * operation are completed when counter reaches zero. */
        win_ptr->at_completion_counter++;
    }

    /* NOTE: 'copy data + ACC' needs to be atomic */

    if (win_ptr->shm_allocated == TRUE)
        MPIDI_CH3I_SHM_MUTEX_LOCK(win_ptr);

    /* Copy data from target window to temporary buffer / response packet header */

    if (fop_pkt->type == MPIDI_CH3_PKT_FOP_IMMED) {
        /* copy data to resp pkt header */
        void *src = fop_pkt->addr, *dest = fop_resp_pkt->info.data;
        mpi_errno = immed_copy(src, dest, type_size);
        if (mpi_errno != MPI_SUCCESS) {
            if (win_ptr->shm_allocated == TRUE)
                MPIDI_CH3I_SHM_MUTEX_UNLOCK(win_ptr);
            MPIR_ERR_POP(mpi_errno);
        }
    }
    else if (is_contig) {
        MPIU_Memcpy(resp_req->dev.user_buf, fop_pkt->addr, type_size);
    }
    else {
        MPID_Segment *seg = MPID_Segment_alloc();
        MPI_Aint last = type_size;

        if (seg == NULL) {
            if (win_ptr->shm_allocated == TRUE)
                MPIDI_CH3I_SHM_MUTEX_UNLOCK(win_ptr);
        }
        MPIR_ERR_CHKANDJUMP1(seg == NULL, mpi_errno, MPI_ERR_OTHER, "**nomem", "**nomem %s",
                             "MPID_Segment");
        MPID_Segment_init(fop_pkt->addr, 1, fop_pkt->datatype, seg, 0);
        MPID_Segment_pack(seg, 0, &last, resp_req->dev.user_buf);
        MPID_Segment_free(seg);
    }

    /* Apply the op */
    if (fop_pkt->type == MPIDI_CH3_PKT_FOP_IMMED) {
        mpi_errno = do_accumulate_op(fop_pkt->info.data, 1, fop_pkt->datatype,
                                     fop_pkt->addr, 1, fop_pkt->datatype, 0, fop_pkt->op);
    }
    else {
        mpi_errno = do_accumulate_op(target_lock_entry->data, 1, fop_pkt->datatype,
                                     fop_pkt->addr, 1, fop_pkt->datatype, 0, fop_pkt->op);
    }

    if (win_ptr->shm_allocated == TRUE)
        MPIDI_CH3I_SHM_MUTEX_UNLOCK(win_ptr);

    if (mpi_errno != MPI_SUCCESS)
        MPIR_ERR_POP(mpi_errno);

    if (fop_pkt->type == MPIDI_CH3_PKT_FOP_IMMED) {
        /* send back the original data */
        MPID_THREAD_CS_ENTER(POBJ, target_lock_entry->vc->pobj_mutex);
        mpi_errno =
            MPIDI_CH3_iStartMsg(target_lock_entry->vc, fop_resp_pkt, sizeof(*fop_resp_pkt),
                                &resp_req);
        MPID_THREAD_CS_EXIT(POBJ, target_lock_entry->vc->pobj_mutex);
        MPIR_ERR_CHKANDJUMP(mpi_errno != MPI_SUCCESS, mpi_errno, MPI_ERR_OTHER, "**ch3|rmamsg");

        if (resp_req != NULL) {
            if (!MPID_Request_is_complete(resp_req)) {
                /* sending process is not completed, set proper OnDataAvail
                 * (it is initialized to NULL by lower layer) */
                resp_req->dev.target_win_handle = fop_pkt->target_win_handle;
                resp_req->dev.flags = fop_pkt->flags;
                resp_req->dev.OnDataAvail = MPIDI_CH3_ReqHandler_FOPSendComplete;

                /* here we increment the Active Target counter to guarantee the GET-like
                 * operation are completed when counter reaches zero. */
                win_ptr->at_completion_counter++;

                MPID_Request_release(resp_req);
                goto fn_exit;
            }
            else {
                MPID_Request_release(resp_req);
            }
        }
    }
    else {
        iov[0].MPL_IOV_BUF = (MPL_IOV_BUF_CAST) fop_resp_pkt;
        iov[0].MPL_IOV_LEN = sizeof(*fop_resp_pkt);
        iov[1].MPL_IOV_BUF = (MPL_IOV_BUF_CAST) ((char *) resp_req->dev.user_buf);
        iov[1].MPL_IOV_LEN = type_size;
        iovcnt = 2;

        mpi_errno = MPIDI_CH3_iSendv(target_lock_entry->vc, resp_req, iov, iovcnt);
        if (mpi_errno != MPI_SUCCESS) {
            MPID_Request_release(resp_req);
            MPIR_ERR_SETANDJUMP(mpi_errno, MPI_ERR_OTHER, "**ch3|rmamsg");
        }
        goto fn_exit;
    }

    /* do final action */
    mpi_errno = finish_op_on_target(win_ptr, target_lock_entry->vc, TRUE /* has response data */ ,
                                    fop_pkt->flags, MPI_WIN_NULL);
    if (mpi_errno != MPI_SUCCESS)
        MPIR_ERR_POP(mpi_errno);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}


static inline int perform_cas_in_lock_queue(MPID_Win * win_ptr,
                                            MPIDI_RMA_Target_lock_entry_t * target_lock_entry)
{
    MPIDI_CH3_Pkt_t upkt;
    MPIDI_CH3_Pkt_cas_resp_t *cas_resp_pkt = &upkt.cas_resp;
    MPIDI_CH3_Pkt_cas_t *cas_pkt = &((target_lock_entry->pkt).cas);
    MPID_Request *send_req = NULL;
    MPI_Aint len;
    int mpi_errno = MPI_SUCCESS;

    /* Piggyback candidate should have basic datatype for target datatype. */
    MPIU_Assert(MPIR_DATATYPE_IS_PREDEFINED(cas_pkt->datatype));

    /* Make sure that all data is received for this op. */
    MPIU_Assert(target_lock_entry->all_data_recved == 1);

    MPIDI_Pkt_init(cas_resp_pkt, MPIDI_CH3_PKT_CAS_RESP_IMMED);
    cas_resp_pkt->request_handle = cas_pkt->request_handle;
    cas_resp_pkt->target_rank = win_ptr->comm_ptr->rank;
    cas_resp_pkt->flags = MPIDI_CH3_PKT_FLAG_NONE;
    if (cas_pkt->flags & MPIDI_CH3_PKT_FLAG_RMA_LOCK_SHARED ||
        cas_pkt->flags & MPIDI_CH3_PKT_FLAG_RMA_LOCK_EXCLUSIVE)
        cas_resp_pkt->flags |= MPIDI_CH3_PKT_FLAG_RMA_LOCK_GRANTED;
    if ((cas_pkt->flags & MPIDI_CH3_PKT_FLAG_RMA_FLUSH) ||
        (cas_pkt->flags & MPIDI_CH3_PKT_FLAG_RMA_UNLOCK))
        cas_resp_pkt->flags |= MPIDI_CH3_PKT_FLAG_RMA_ACK;

    /* Copy old value into the response packet */
    MPID_Datatype_get_size_macro(cas_pkt->datatype, len);
    MPIU_Assert(len <= sizeof(MPIDI_CH3_CAS_Immed_u));

    if (win_ptr->shm_allocated == TRUE)
        MPIDI_CH3I_SHM_MUTEX_LOCK(win_ptr);

    MPIU_Memcpy((void *) &cas_resp_pkt->info.data, cas_pkt->addr, len);

    /* Compare and replace if equal */
    if (MPIR_Compare_equal(&cas_pkt->compare_data, cas_pkt->addr, cas_pkt->datatype)) {
        MPIU_Memcpy(cas_pkt->addr, &cas_pkt->origin_data, len);
    }

    if (win_ptr->shm_allocated == TRUE)
        MPIDI_CH3I_SHM_MUTEX_UNLOCK(win_ptr);

    /* Send the response packet */
    MPID_THREAD_CS_ENTER(POBJ, target_lock_entry->vc->pobj_mutex);
    mpi_errno =
        MPIDI_CH3_iStartMsg(target_lock_entry->vc, cas_resp_pkt, sizeof(*cas_resp_pkt), &send_req);
    MPID_THREAD_CS_EXIT(POBJ, target_lock_entry->vc->pobj_mutex);

    MPIR_ERR_CHKANDJUMP(mpi_errno != MPI_SUCCESS, mpi_errno, MPI_ERR_OTHER, "**ch3|rmamsg");

    if (send_req != NULL) {
        if (!MPID_Request_is_complete(send_req)) {
            /* sending process is not completed, set proper OnDataAvail
             * (it is initialized to NULL by lower layer) */
            send_req->dev.target_win_handle = cas_pkt->target_win_handle;
            send_req->dev.flags = cas_pkt->flags;
            send_req->dev.OnDataAvail = MPIDI_CH3_ReqHandler_CASSendComplete;

            /* here we increment the Active Target counter to guarantee the GET-like
             * operation are completed when counter reaches zero. */
            win_ptr->at_completion_counter++;

            MPID_Request_release(send_req);
            goto fn_exit;
        }
        else
            MPID_Request_release(send_req);
    }

    /* do final action */
    mpi_errno = finish_op_on_target(win_ptr, target_lock_entry->vc, TRUE /* has response data */ ,
                                    cas_pkt->flags, MPI_WIN_NULL);
    if (mpi_errno != MPI_SUCCESS)
        MPIR_ERR_POP(mpi_errno);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}


static inline int perform_op_in_lock_queue(MPID_Win * win_ptr,
                                           MPIDI_RMA_Target_lock_entry_t * target_lock_entry)
{
    int mpi_errno = MPI_SUCCESS;

    if (target_lock_entry->pkt.type == MPIDI_CH3_PKT_LOCK) {

        /* single LOCK request */

        MPIDI_CH3_Pkt_lock_t *lock_pkt = &(target_lock_entry->pkt.lock);
        MPIDI_VC_t *my_vc = NULL;

        MPIDI_Comm_get_vc_set_active(win_ptr->comm_ptr, win_ptr->comm_ptr->rank, &my_vc);

        if (target_lock_entry->vc == my_vc) {
            mpi_errno = handle_lock_ack(win_ptr, win_ptr->comm_ptr->rank,
                                        MPIDI_CH3_PKT_FLAG_RMA_LOCK_GRANTED);
            if (mpi_errno)
                MPIR_ERR_POP(mpi_errno);
        }
        else {
            mpi_errno = MPIDI_CH3I_Send_lock_ack_pkt(target_lock_entry->vc, win_ptr,
                                                     MPIDI_CH3_PKT_FLAG_RMA_LOCK_GRANTED,
                                                     lock_pkt->source_win_handle,
                                                     lock_pkt->request_handle);
            if (mpi_errno != MPI_SUCCESS)
                MPIR_ERR_POP(mpi_errno);
        }
    }
    else {
        /* LOCK+OP packet */
        switch (target_lock_entry->pkt.type) {
        case (MPIDI_CH3_PKT_PUT):
        case (MPIDI_CH3_PKT_PUT_IMMED):
            mpi_errno = perform_put_in_lock_queue(win_ptr, target_lock_entry);
            if (mpi_errno)
                MPIR_ERR_POP(mpi_errno);
            break;
        case (MPIDI_CH3_PKT_GET):
            mpi_errno = perform_get_in_lock_queue(win_ptr, target_lock_entry);
            if (mpi_errno)
                MPIR_ERR_POP(mpi_errno);
            break;
        case (MPIDI_CH3_PKT_ACCUMULATE):
        case (MPIDI_CH3_PKT_ACCUMULATE_IMMED):
            mpi_errno = perform_acc_in_lock_queue(win_ptr, target_lock_entry);
            if (mpi_errno)
                MPIR_ERR_POP(mpi_errno);
            break;
        case (MPIDI_CH3_PKT_GET_ACCUM):
        case (MPIDI_CH3_PKT_GET_ACCUM_IMMED):
            mpi_errno = perform_get_acc_in_lock_queue(win_ptr, target_lock_entry);
            if (mpi_errno)
                MPIR_ERR_POP(mpi_errno);
            break;
        case (MPIDI_CH3_PKT_FOP):
        case (MPIDI_CH3_PKT_FOP_IMMED):
            mpi_errno = perform_fop_in_lock_queue(win_ptr, target_lock_entry);
            if (mpi_errno)
                MPIR_ERR_POP(mpi_errno);
            break;
        case (MPIDI_CH3_PKT_CAS_IMMED):
            mpi_errno = perform_cas_in_lock_queue(win_ptr, target_lock_entry);
            if (mpi_errno)
                MPIR_ERR_POP(mpi_errno);
            break;
        default:
            MPIR_ERR_SETANDJUMP1(mpi_errno, MPI_ERR_OTHER,
                                 "**invalidpkt", "**invalidpkt %d", target_lock_entry->pkt.type);
        }
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}


static int entered_flag = 0;
static int entered_count = 0;

/* Release the current lock on the window and grant the next lock in the
   queue if any */
#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_Release_lock
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIDI_CH3I_Release_lock(MPID_Win * win_ptr)
{
    MPIDI_RMA_Target_lock_entry_t *target_lock_entry, *target_lock_entry_next;
    int requested_lock, mpi_errno = MPI_SUCCESS, temp_entered_count;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3I_RELEASE_LOCK);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3I_RELEASE_LOCK);

    if (win_ptr->current_lock_type == MPI_LOCK_SHARED) {
        /* decr ref cnt */
        /* FIXME: MT: Must be done atomically */
        win_ptr->shared_lock_ref_cnt--;
    }

    /* If shared lock ref count is 0 (which is also true if the lock is an
     * exclusive lock), release the lock. */
    if (win_ptr->shared_lock_ref_cnt == 0) {

        /* This function needs to be reentrant even in the single-threaded case
         * because when going through the lock queue, pkt_handler() in
         * perform_op_in_lock_queue() may again call release_lock(). To handle
         * this possibility, we use an entered_flag.
         * If the flag is not 0, we simply increment the entered_count and return.
         * The loop through the lock queue is repeated if the entered_count has
         * changed while we are in the loop.
         */
        if (entered_flag != 0) {
            entered_count++;    /* Count how many times we re-enter */
            goto fn_exit;
        }

        entered_flag = 1;       /* Mark that we are now entering release_lock() */
        temp_entered_count = entered_count;

        do {
            if (temp_entered_count != entered_count)
                temp_entered_count++;

            /* FIXME: MT: The setting of the lock type must be done atomically */
            win_ptr->current_lock_type = MPID_LOCK_NONE;

            /* If there is a lock queue, try to satisfy as many lock requests as
             * possible. If the first one is a shared lock, grant it and grant all
             * other shared locks. If the first one is an exclusive lock, grant
             * only that one. */

            /* FIXME: MT: All queue accesses need to be made atomic */
            target_lock_entry = (MPIDI_RMA_Target_lock_entry_t *) win_ptr->target_lock_queue_head;
            while (target_lock_entry) {
                target_lock_entry_next = target_lock_entry->next;

                if (target_lock_entry->all_data_recved) {
                    MPIDI_CH3_Pkt_flags_t flags;
                    MPIDI_CH3_PKT_RMA_GET_FLAGS(target_lock_entry->pkt, flags, mpi_errno);
                    if (flags & MPIDI_CH3_PKT_FLAG_RMA_LOCK_SHARED)
                        requested_lock = MPI_LOCK_SHARED;
                    else {
                        MPIU_Assert(flags & MPIDI_CH3_PKT_FLAG_RMA_LOCK_EXCLUSIVE);
                        requested_lock = MPI_LOCK_EXCLUSIVE;
                    }
                    if (MPIDI_CH3I_Try_acquire_win_lock(win_ptr, requested_lock) == 1) {
                        /* dequeue entry from lock queue */
                        MPL_DL_DELETE(win_ptr->target_lock_queue_head, target_lock_entry);

                        /* perform this OP */
                        mpi_errno = perform_op_in_lock_queue(win_ptr, target_lock_entry);
                        if (mpi_errno != MPI_SUCCESS)
                            MPIR_ERR_POP(mpi_errno);

                        /* free this entry */
                        mpi_errno =
                            MPIDI_CH3I_Win_target_lock_entry_free(win_ptr, target_lock_entry);
                        if (mpi_errno != MPI_SUCCESS)
                            MPIR_ERR_POP(mpi_errno);

                        /* if the granted lock is exclusive,
                         * no need to continue */
                        if (requested_lock == MPI_LOCK_EXCLUSIVE)
                            break;
                    }
                }
                target_lock_entry = target_lock_entry_next;
            }
        } while (temp_entered_count != entered_count);

        entered_count = entered_flag = 0;
    }

  fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_RELEASE_LOCK);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}



#undef FUNCNAME
#define FUNCNAME MPIDI_CH3_ReqHandler_PiggybackLockOpRecvComplete
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIDI_CH3_ReqHandler_PiggybackLockOpRecvComplete(MPIDI_VC_t * vc,
                                                     MPID_Request * rreq, int *complete)
{
    int requested_lock;
    MPI_Win target_win_handle;
    MPID_Win *win_ptr = NULL;
    MPIDI_CH3_Pkt_flags_t flags;
    MPIDI_RMA_Target_lock_entry_t *target_lock_queue_entry = rreq->dev.target_lock_queue_entry;
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3_REQHANDLER_PIGGYBACKLOCKOPRECVCOMPLETE);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3_REQHANDLER_PIGGYBACKLOCKOPRECVCOMPLETE);

    /* This handler is triggered when we received all data of a lock queue
     * entry */

    /* Note that if we decided to drop op data, here we just need to complete this
     * request; otherwise we try to get the lock again in this handler. */
    if (rreq->dev.target_lock_queue_entry != NULL) {

        /* Mark all data received in lock queue entry */
        target_lock_queue_entry->all_data_recved = 1;

        /* try to acquire the lock here */
        MPIDI_CH3_PKT_RMA_GET_FLAGS(target_lock_queue_entry->pkt, flags, mpi_errno);
        MPIDI_CH3_PKT_RMA_GET_TARGET_WIN_HANDLE(target_lock_queue_entry->pkt, target_win_handle,
                                                mpi_errno);
        MPID_Win_get_ptr(target_win_handle, win_ptr);

        if (flags & MPIDI_CH3_PKT_FLAG_RMA_STREAM &&
            (rreq->dev.target_lock_queue_entry)->data != NULL) {

            MPIU_Assert(target_lock_queue_entry->pkt.type == MPIDI_CH3_PKT_ACCUMULATE ||
                        target_lock_queue_entry->pkt.type == MPIDI_CH3_PKT_GET_ACCUM);

            int ext_hdr_sz;

            /* only basic datatype may contain piggyback lock.
             * Thus we do not check extended header type for derived case.*/
            if (target_lock_queue_entry->pkt.type == MPIDI_CH3_PKT_ACCUMULATE)
                ext_hdr_sz = sizeof(MPIDI_CH3_Ext_pkt_accum_stream_t);
            else
                ext_hdr_sz = sizeof(MPIDI_CH3_Ext_pkt_get_accum_stream_t);

            /* here we drop the stream_offset received, because the stream unit that piggybacked with
             * LOCK must be the first stream unit, with stream_offset equals to 0. */
            rreq->dev.recv_data_sz -= ext_hdr_sz;
            memmove((rreq->dev.target_lock_queue_entry)->data,
                    (void *) ((char *) ((rreq->dev.target_lock_queue_entry)->data) + ext_hdr_sz),
                    rreq->dev.recv_data_sz);
        }

        if (flags & MPIDI_CH3_PKT_FLAG_RMA_LOCK_SHARED) {
            requested_lock = MPI_LOCK_SHARED;
        }
        else {
            MPIU_Assert(flags & MPIDI_CH3_PKT_FLAG_RMA_LOCK_EXCLUSIVE);
            requested_lock = MPI_LOCK_EXCLUSIVE;
        }

        if (MPIDI_CH3I_Try_acquire_win_lock(win_ptr, requested_lock) == 1) {
            /* dequeue entry from lock queue */
            MPL_DL_DELETE(win_ptr->target_lock_queue_head, target_lock_queue_entry);

            /* perform this OP */
            mpi_errno = perform_op_in_lock_queue(win_ptr, target_lock_queue_entry);
            if (mpi_errno != MPI_SUCCESS)
                MPIR_ERR_POP(mpi_errno);

            /* free this entry */
            mpi_errno = MPIDI_CH3I_Win_target_lock_entry_free(win_ptr, target_lock_queue_entry);
            if (mpi_errno != MPI_SUCCESS)
                MPIR_ERR_POP(mpi_errno);
        }
        /* If try acquiring lock failed, just leave the lock queue entry in the queue with
         * all_data_recved marked as 1, release_lock() function will traverse the queue
         * and find entry with all_data_recved being 1 to grant the lock. */
    }

    /* mark receive data transfer as complete and decrement CC in receive
     * request */
    mpi_errno = MPID_Request_complete(rreq);
    if (mpi_errno != MPI_SUCCESS) {
        MPIR_ERR_POP(mpi_errno);
    }

    *complete = TRUE;

  fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3_REQHANDLER_PIGGYBACKLOCKOPRECVCOMPLETE);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
