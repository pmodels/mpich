/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"
#include "mpidrma.h"


/* ------------------------------------------------------------------------ */
/*
 * The following routines are the packet handlers for the packet types
 * used above in the implementation of the RMA operations in terms
 * of messages.
 */
/* ------------------------------------------------------------------------ */
#undef FUNCNAME
#define FUNCNAME MPIDI_CH3_PktHandler_Put
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3_PktHandler_Put(MPIDI_VC_t * vc, MPIDI_CH3_Pkt_t * pkt,
                             MPIDI_msg_sz_t * buflen, MPID_Request ** rreqp)
{
    MPIDI_CH3_Pkt_put_t *put_pkt = &pkt->put;
    MPID_Request *req = NULL;
    MPI_Aint type_size;
    int complete = 0;
    char *data_buf = NULL;
    MPIDI_msg_sz_t data_len;
    MPID_Win *win_ptr;
    int acquire_lock_fail = 0;
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3_PKTHANDLER_PUT);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3_PKTHANDLER_PUT);

    MPIU_DBG_MSG(CH3_OTHER, VERBOSE, "received put pkt");

    MPIU_Assert(put_pkt->target_win_handle != MPI_WIN_NULL);
    MPID_Win_get_ptr(put_pkt->target_win_handle, win_ptr);
    mpi_errno = MPIDI_CH3_Start_rma_op_target(win_ptr, put_pkt->flags);

    data_len = *buflen - sizeof(MPIDI_CH3_Pkt_t);
    data_buf = (char *) pkt + sizeof(MPIDI_CH3_Pkt_t);

    mpi_errno = check_piggyback_lock(win_ptr, pkt, &acquire_lock_fail);
    if (mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);

    if (acquire_lock_fail) {
        (*buflen) = sizeof(MPIDI_CH3_Pkt_t);
        (*rreqp) = NULL;
        goto fn_exit;
    }

    req = MPID_Request_create();
    MPIU_Object_set_ref(req, 1);

    req->dev.user_buf = put_pkt->addr;
    req->dev.user_count = put_pkt->count;
    req->dev.target_win_handle = put_pkt->target_win_handle;
    req->dev.source_win_handle = put_pkt->source_win_handle;
    req->dev.flags = put_pkt->flags;

    if (MPIR_DATATYPE_IS_PREDEFINED(put_pkt->datatype)) {
        MPIDI_Request_set_type(req, MPIDI_REQUEST_TYPE_PUT_RESP);
        req->dev.datatype = put_pkt->datatype;

        MPID_Datatype_get_size_macro(put_pkt->datatype, type_size);
        req->dev.recv_data_sz = type_size * put_pkt->count;

        if (put_pkt->immed_len > 0) {
            /* See if we can receive some data from packet header. */
            MPIU_Memcpy(req->dev.user_buf, put_pkt->data, put_pkt->immed_len);
            req->dev.user_buf = (void*)((char*)req->dev.user_buf + put_pkt->immed_len);
            req->dev.recv_data_sz -= put_pkt->immed_len;
        }

        if (req->dev.recv_data_sz == 0) {
            /* All data received, trigger req handler. */

            *buflen = sizeof(MPIDI_CH3_Pkt_t);
            mpi_errno = MPIDI_CH3_ReqHandler_PutRecvComplete(vc, req, &complete);
            if (mpi_errno)
                MPIU_ERR_POP(mpi_errno);
            if (complete) {
                *rreqp = NULL;
                goto fn_exit;
            }
        }

        mpi_errno = MPIDI_CH3U_Receive_data_found(req, data_buf, &data_len, &complete);
        MPIU_ERR_CHKANDJUMP1(mpi_errno, mpi_errno, MPI_ERR_OTHER, "**ch3|postrecv",
                             "**ch3|postrecv %s", "MPIDI_CH3_PKT_PUT");
        /* FIXME:  Only change the handling of completion if
         * post_data_receive reset the handler.  There should
         * be a cleaner way to do this */
        if (!req->dev.OnDataAvail) {
            req->dev.OnDataAvail = MPIDI_CH3_ReqHandler_PutRecvComplete;
        }

        /* return the number of bytes processed in this function */
        *buflen = sizeof(MPIDI_CH3_Pkt_t) + data_len;

        if (complete) {
            mpi_errno = MPIDI_CH3_ReqHandler_PutRecvComplete(vc, req, &complete);
            if (mpi_errno)
                MPIU_ERR_POP(mpi_errno);
            if (complete) {
                *rreqp = NULL;
                goto fn_exit;
            }
        }
    }
    else {
        /* derived datatype */
        MPIDI_Request_set_type(req, MPIDI_REQUEST_TYPE_PUT_RESP_DERIVED_DT);
        req->dev.datatype = MPI_DATATYPE_NULL;
        req->dev.OnFinal = MPIDI_CH3_ReqHandler_PutRecvComplete;

        req->dev.dtype_info = (MPIDI_RMA_dtype_info *)
            MPIU_Malloc(sizeof(MPIDI_RMA_dtype_info));
        if (!req->dev.dtype_info) {
            MPIU_ERR_SETANDJUMP1(mpi_errno, MPI_ERR_OTHER, "**nomem", "**nomem %s",
                                 "MPIDI_RMA_dtype_info");
        }

        req->dev.dataloop = MPIU_Malloc(put_pkt->dataloop_size);
        if (!req->dev.dataloop) {
            MPIU_ERR_SETANDJUMP1(mpi_errno, MPI_ERR_OTHER, "**nomem", "**nomem %d",
                                 put_pkt->dataloop_size);
        }

        /* if we received all of the dtype_info and dataloop, copy it
         * now and call the handler, otherwise set the iov and let the
         * channel copy it */
        if (data_len >= sizeof(MPIDI_RMA_dtype_info) + put_pkt->dataloop_size) {
            /* copy all of dtype_info and dataloop */
            MPIU_Memcpy(req->dev.dtype_info, data_buf, sizeof(MPIDI_RMA_dtype_info));
            MPIU_Memcpy(req->dev.dataloop, data_buf + sizeof(MPIDI_RMA_dtype_info),
                        put_pkt->dataloop_size);

            *buflen =
                sizeof(MPIDI_CH3_Pkt_t) + sizeof(MPIDI_RMA_dtype_info) + put_pkt->dataloop_size;

            /* All dtype data has been received, call req handler */
            mpi_errno = MPIDI_CH3_ReqHandler_PutDerivedDTRecvComplete(vc, req, &complete);
            MPIU_ERR_CHKANDJUMP1(mpi_errno, mpi_errno, MPI_ERR_OTHER, "**ch3|postrecv",
                                 "**ch3|postrecv %s", "MPIDI_CH3_PKT_PUT");
            if (complete) {
                *rreqp = NULL;
                goto fn_exit;
            }
        }
        else {
            req->dev.iov[0].MPID_IOV_BUF = (MPID_IOV_BUF_CAST) ((char *) req->dev.dtype_info);
            req->dev.iov[0].MPID_IOV_LEN = sizeof(MPIDI_RMA_dtype_info);
            req->dev.iov[1].MPID_IOV_BUF = (MPID_IOV_BUF_CAST) req->dev.dataloop;
            req->dev.iov[1].MPID_IOV_LEN = put_pkt->dataloop_size;
            req->dev.iov_count = 2;

            *buflen = sizeof(MPIDI_CH3_Pkt_t);

            req->dev.OnDataAvail = MPIDI_CH3_ReqHandler_PutDerivedDTRecvComplete;
        }

    }

    *rreqp = req;

    if (mpi_errno != MPI_SUCCESS) {
        MPIU_ERR_SET1(mpi_errno, MPI_ERR_OTHER, "**ch3|postrecv",
                      "**ch3|postrecv %s", "MPIDI_CH3_PKT_PUT");
    }


  fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3_PKTHANDLER_PUT);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3_PktHandler_Get
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3_PktHandler_Get(MPIDI_VC_t * vc, MPIDI_CH3_Pkt_t * pkt,
                             MPIDI_msg_sz_t * buflen, MPID_Request ** rreqp)
{
    MPIDI_CH3_Pkt_get_t *get_pkt = &pkt->get;
    MPID_Request *req = NULL;
    MPID_IOV iov[MPID_IOV_LIMIT];
    int complete;
    char *data_buf = NULL;
    MPIDI_msg_sz_t data_len;
    MPID_Win *win_ptr;
    int mpi_errno = MPI_SUCCESS;
    MPI_Aint type_size;
    int acquire_lock_fail = 0;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3_PKTHANDLER_GET);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3_PKTHANDLER_GET);

    MPIU_DBG_MSG(CH3_OTHER, VERBOSE, "received get pkt");

    MPIU_Assert(get_pkt->target_win_handle != MPI_WIN_NULL);
    MPID_Win_get_ptr(get_pkt->target_win_handle, win_ptr);
    mpi_errno = MPIDI_CH3_Start_rma_op_target(win_ptr, get_pkt->flags);

    mpi_errno = check_piggyback_lock(win_ptr, pkt, &acquire_lock_fail);
    if (mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);

    if (acquire_lock_fail) {
        (*buflen) = sizeof(MPIDI_CH3_Pkt_t);
        (*rreqp) = NULL;
        goto fn_exit;
    }

    data_len = *buflen - sizeof(MPIDI_CH3_Pkt_t);
    data_buf = (char *) pkt + sizeof(MPIDI_CH3_Pkt_t);

    req = MPID_Request_create();
    req->dev.target_win_handle = get_pkt->target_win_handle;
    req->dev.source_win_handle = get_pkt->source_win_handle;
    req->dev.flags = get_pkt->flags;

    /* here we increment the Active Target counter to guarantee the GET-like
       operation are completed when counter reaches zero. */
    win_ptr->at_completion_counter++;

    if (MPIR_DATATYPE_IS_PREDEFINED(get_pkt->datatype)) {
        /* basic datatype. send the data. */
        MPIDI_CH3_Pkt_t upkt;
        MPIDI_CH3_Pkt_get_resp_t *get_resp_pkt = &upkt.get_resp;

        MPIDI_Request_set_type(req, MPIDI_REQUEST_TYPE_GET_RESP);
        req->dev.OnDataAvail = MPIDI_CH3_ReqHandler_GetSendComplete;
        req->dev.OnFinal = MPIDI_CH3_ReqHandler_GetSendComplete;
        req->kind = MPID_REQUEST_SEND;

        MPIDI_Pkt_init(get_resp_pkt, MPIDI_CH3_PKT_GET_RESP);
        get_resp_pkt->request_handle = get_pkt->request_handle;
        get_resp_pkt->flags = MPIDI_CH3_PKT_FLAG_NONE;
        if (get_pkt->flags & MPIDI_CH3_PKT_FLAG_RMA_LOCK_GRANTED)
            get_resp_pkt->flags |= MPIDI_CH3_PKT_FLAG_RMA_LOCK_GRANTED;
        if (get_pkt->flags & MPIDI_CH3_PKT_FLAG_RMA_FLUSH)
            get_resp_pkt->flags |= MPIDI_CH3_PKT_FLAG_RMA_FLUSH_ACK;
        if (get_pkt->flags & MPIDI_CH3_PKT_FLAG_RMA_UNLOCK)
            get_resp_pkt->flags |= MPIDI_CH3_PKT_FLAG_RMA_UNLOCK_ACK;
        get_resp_pkt->target_rank = win_ptr->comm_ptr->rank;
        get_resp_pkt->source_win_handle = get_pkt->source_win_handle;

        iov[0].MPID_IOV_BUF = (MPID_IOV_BUF_CAST) get_resp_pkt;
        iov[0].MPID_IOV_LEN = sizeof(*get_resp_pkt);

        iov[1].MPID_IOV_BUF = (MPID_IOV_BUF_CAST) get_pkt->addr;
        MPID_Datatype_get_size_macro(get_pkt->datatype, type_size);
        iov[1].MPID_IOV_LEN = get_pkt->count * type_size;

        MPIU_THREAD_CS_ENTER(CH3COMM, vc);
        mpi_errno = MPIDI_CH3_iSendv(vc, req, iov, 2);
        MPIU_THREAD_CS_EXIT(CH3COMM, vc);
        /* --BEGIN ERROR HANDLING-- */
        if (mpi_errno != MPI_SUCCESS) {
            MPIU_Object_set_ref(req, 0);
            MPIDI_CH3_Request_destroy(req);
            MPIU_ERR_SETANDJUMP(mpi_errno, MPI_ERR_OTHER, "**ch3|rmamsg");
        }
        /* --END ERROR HANDLING-- */

        *buflen = sizeof(MPIDI_CH3_Pkt_t);
        *rreqp = NULL;
    }
    else {
        /* derived datatype. first get the dtype_info and dataloop. */

        MPIDI_Request_set_type(req, MPIDI_REQUEST_TYPE_GET_RESP_DERIVED_DT);
        req->dev.OnDataAvail = MPIDI_CH3_ReqHandler_GetDerivedDTRecvComplete;
        req->dev.OnFinal = 0;
        req->dev.user_buf = get_pkt->addr;
        req->dev.user_count = get_pkt->count;
        req->dev.datatype = MPI_DATATYPE_NULL;
        req->dev.request_handle = get_pkt->request_handle;

        req->dev.dtype_info = (MPIDI_RMA_dtype_info *)
            MPIU_Malloc(sizeof(MPIDI_RMA_dtype_info));
        if (!req->dev.dtype_info) {
            MPIU_ERR_SETANDJUMP1(mpi_errno, MPI_ERR_OTHER, "**nomem", "**nomem %s",
                                 "MPIDI_RMA_dtype_info");
        }

        req->dev.dataloop = MPIU_Malloc(get_pkt->dataloop_size);
        if (!req->dev.dataloop) {
            MPIU_ERR_SETANDJUMP1(mpi_errno, MPI_ERR_OTHER, "**nomem", "**nomem %d",
                                 get_pkt->dataloop_size);
        }

        /* if we received all of the dtype_info and dataloop, copy it
         * now and call the handler, otherwise set the iov and let the
         * channel copy it */
        if (data_len >= sizeof(MPIDI_RMA_dtype_info) + get_pkt->dataloop_size) {
            /* copy all of dtype_info and dataloop */
            MPIU_Memcpy(req->dev.dtype_info, data_buf, sizeof(MPIDI_RMA_dtype_info));
            MPIU_Memcpy(req->dev.dataloop, data_buf + sizeof(MPIDI_RMA_dtype_info),
                        get_pkt->dataloop_size);

            *buflen =
                sizeof(MPIDI_CH3_Pkt_t) + sizeof(MPIDI_RMA_dtype_info) + get_pkt->dataloop_size;

            /* All dtype data has been received, call req handler */
            mpi_errno = MPIDI_CH3_ReqHandler_GetDerivedDTRecvComplete(vc, req, &complete);
            MPIU_ERR_CHKANDJUMP1(mpi_errno, mpi_errno, MPI_ERR_OTHER, "**ch3|postrecv",
                                 "**ch3|postrecv %s", "MPIDI_CH3_PKT_GET");
            if (complete)
                *rreqp = NULL;
        }
        else {
            req->dev.iov[0].MPID_IOV_BUF = (MPID_IOV_BUF_CAST) req->dev.dtype_info;
            req->dev.iov[0].MPID_IOV_LEN = sizeof(MPIDI_RMA_dtype_info);
            req->dev.iov[1].MPID_IOV_BUF = (MPID_IOV_BUF_CAST) req->dev.dataloop;
            req->dev.iov[1].MPID_IOV_LEN = get_pkt->dataloop_size;
            req->dev.iov_count = 2;

            *buflen = sizeof(MPIDI_CH3_Pkt_t);
            *rreqp = req;
        }

    }
  fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3_PKTHANDLER_GET);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3_PktHandler_Accumulate
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3_PktHandler_Accumulate(MPIDI_VC_t * vc, MPIDI_CH3_Pkt_t * pkt,
                                    MPIDI_msg_sz_t * buflen, MPID_Request ** rreqp)
{
    MPIDI_CH3_Pkt_accum_t *accum_pkt = &pkt->accum;
    MPID_Request *req = NULL;
    MPI_Aint true_lb, true_extent, extent;
    void *tmp_buf = NULL;
    int complete = 0;
    char *data_buf = NULL;
    MPIDI_msg_sz_t data_len;
    MPID_Win *win_ptr;
    int acquire_lock_fail = 0;
    int mpi_errno = MPI_SUCCESS;
    MPI_Aint type_size;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3_PKTHANDLER_ACCUMULATE);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3_PKTHANDLER_ACCUMULATE);

    MPIU_DBG_MSG(CH3_OTHER, VERBOSE, "received accumulate pkt");

    MPIU_Assert(accum_pkt->target_win_handle != MPI_WIN_NULL);
    MPID_Win_get_ptr(accum_pkt->target_win_handle, win_ptr);
    mpi_errno = MPIDI_CH3_Start_rma_op_target(win_ptr, accum_pkt->flags);

    mpi_errno = check_piggyback_lock(win_ptr, pkt, &acquire_lock_fail);
    if (mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);

    if (acquire_lock_fail) {
        (*buflen) = sizeof(MPIDI_CH3_Pkt_t);
        (*rreqp) = NULL;
        goto fn_exit;
    }

    data_len = *buflen - sizeof(MPIDI_CH3_Pkt_t);
    data_buf = (char *) pkt + sizeof(MPIDI_CH3_Pkt_t);

    req = MPID_Request_create();
    MPIU_Object_set_ref(req, 1);
    *rreqp = req;

    req->dev.user_count = accum_pkt->count;
    req->dev.op = accum_pkt->op;
    req->dev.real_user_buf = accum_pkt->addr;
    req->dev.target_win_handle = accum_pkt->target_win_handle;
    req->dev.source_win_handle = accum_pkt->source_win_handle;
    req->dev.flags = accum_pkt->flags;

    req->dev.resp_request_handle = MPI_REQUEST_NULL;

    if (MPIR_DATATYPE_IS_PREDEFINED(accum_pkt->datatype)) {
        MPIDI_Request_set_type(req, MPIDI_REQUEST_TYPE_ACCUM_RESP);
        req->dev.datatype = accum_pkt->datatype;

        MPIR_Type_get_true_extent_impl(accum_pkt->datatype, &true_lb, &true_extent);
        MPID_Datatype_get_extent_macro(accum_pkt->datatype, extent);

        /* Predefined types should always have zero lb */
        MPIU_Assert(true_lb == 0);

        tmp_buf = MPIU_Malloc(accum_pkt->count * (MPIR_MAX(extent, true_extent)));
        if (!tmp_buf) {
            MPIU_ERR_SETANDJUMP1(mpi_errno, MPI_ERR_OTHER, "**nomem", "**nomem %d",
                                 accum_pkt->count * MPIR_MAX(extent, true_extent));
        }

        req->dev.user_buf = tmp_buf;
        req->dev.final_user_buf = req->dev.user_buf;

        MPID_Datatype_get_size_macro(accum_pkt->datatype, type_size);
        req->dev.recv_data_sz = type_size * accum_pkt->count;

        if (accum_pkt->immed_len > 0) {
            /* See if we can receive some data from packet header. */
            MPIU_Memcpy(req->dev.user_buf, accum_pkt->data, accum_pkt->immed_len);
            req->dev.user_buf = (void*)((char*)req->dev.user_buf + accum_pkt->immed_len);
            req->dev.recv_data_sz -= accum_pkt->immed_len;
        }

        if (req->dev.recv_data_sz == 0) {
            /* All data received, trigger req handler. */
            *buflen = sizeof(MPIDI_CH3_Pkt_t);
            mpi_errno = MPIDI_CH3_ReqHandler_AccumRecvComplete(vc, req, &complete);
            if (mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);
            if (complete) {
                *rreqp = NULL;
                goto fn_exit;
            }
        }

        mpi_errno = MPIDI_CH3U_Receive_data_found(req, data_buf, &data_len, &complete);
        MPIU_ERR_CHKANDJUMP1(mpi_errno, mpi_errno, MPI_ERR_OTHER, "**ch3|postrecv",
                             "**ch3|postrecv %s", "MPIDI_CH3_PKT_ACCUMULATE");
        /* FIXME:  Only change the handling of completion if
         * post_data_receive reset the handler.  There should
         * be a cleaner way to do this */
        if (!req->dev.OnDataAvail) {
            req->dev.OnDataAvail = MPIDI_CH3_ReqHandler_AccumRecvComplete;
        }
        /* return the number of bytes processed in this function */
        *buflen = data_len + sizeof(MPIDI_CH3_Pkt_t);

        if (complete) {
            mpi_errno = MPIDI_CH3_ReqHandler_AccumRecvComplete(vc, req, &complete);
            if (mpi_errno)
                MPIU_ERR_POP(mpi_errno);
            if (complete) {
                *rreqp = NULL;
                goto fn_exit;
            }
        }
    }
    else {
        MPIDI_Request_set_type(req, MPIDI_REQUEST_TYPE_ACCUM_RESP_DERIVED_DT);
        req->dev.OnDataAvail = MPIDI_CH3_ReqHandler_AccumDerivedDTRecvComplete;
        req->dev.datatype = MPI_DATATYPE_NULL;
        req->dev.OnFinal = MPIDI_CH3_ReqHandler_AccumRecvComplete;

        req->dev.dtype_info = (MPIDI_RMA_dtype_info *)
            MPIU_Malloc(sizeof(MPIDI_RMA_dtype_info));
        if (!req->dev.dtype_info) {
            MPIU_ERR_SETANDJUMP1(mpi_errno, MPI_ERR_OTHER, "**nomem", "**nomem %s",
                                 "MPIDI_RMA_dtype_info");
        }

        req->dev.dataloop = MPIU_Malloc(accum_pkt->dataloop_size);
        if (!req->dev.dataloop) {
            MPIU_ERR_SETANDJUMP1(mpi_errno, MPI_ERR_OTHER, "**nomem", "**nomem %d",
                                 accum_pkt->dataloop_size);
        }

        if (data_len >= sizeof(MPIDI_RMA_dtype_info) + accum_pkt->dataloop_size) {
            /* copy all of dtype_info and dataloop */
            MPIU_Memcpy(req->dev.dtype_info, data_buf, sizeof(MPIDI_RMA_dtype_info));
            MPIU_Memcpy(req->dev.dataloop, data_buf + sizeof(MPIDI_RMA_dtype_info),
                        accum_pkt->dataloop_size);

            *buflen =
                sizeof(MPIDI_CH3_Pkt_t) + sizeof(MPIDI_RMA_dtype_info) + accum_pkt->dataloop_size;

            /* All dtype data has been received, call req handler */
            mpi_errno = MPIDI_CH3_ReqHandler_AccumDerivedDTRecvComplete(vc, req, &complete);
            MPIU_ERR_CHKANDJUMP1(mpi_errno, mpi_errno, MPI_ERR_OTHER, "**ch3|postrecv",
                                 "**ch3|postrecv %s", "MPIDI_CH3_ACCUMULATE");
            if (complete) {
                *rreqp = NULL;
                goto fn_exit;
            }
        }
        else {
            req->dev.iov[0].MPID_IOV_BUF = (MPID_IOV_BUF_CAST) req->dev.dtype_info;
            req->dev.iov[0].MPID_IOV_LEN = sizeof(MPIDI_RMA_dtype_info);
            req->dev.iov[1].MPID_IOV_BUF = (MPID_IOV_BUF_CAST) req->dev.dataloop;
            req->dev.iov[1].MPID_IOV_LEN = accum_pkt->dataloop_size;
            req->dev.iov_count = 2;
            *buflen = sizeof(MPIDI_CH3_Pkt_t);
        }

    }

    if (mpi_errno != MPI_SUCCESS) {
        MPIU_ERR_SETANDJUMP1(mpi_errno, MPI_ERR_OTHER, "**ch3|postrecv",
                             "**ch3|postrecv %s", "MPIDI_CH3_PKT_ACCUMULATE");
    }

  fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3_PKTHANDLER_ACCUMULATE);
    return mpi_errno;
  fn_fail:
    goto fn_exit;

}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3_PktHandler_GetAccumulate
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3_PktHandler_GetAccumulate(MPIDI_VC_t * vc, MPIDI_CH3_Pkt_t * pkt,
                                       MPIDI_msg_sz_t * buflen, MPID_Request ** rreqp)
{
    MPIDI_CH3_Pkt_get_accum_t *get_accum_pkt = &pkt->get_accum;
    MPID_Request *req = NULL;
    MPI_Aint true_lb, true_extent, extent;
    void *tmp_buf = NULL;
    int complete = 0;
    char *data_buf = NULL;
    MPIDI_msg_sz_t data_len;
    MPID_Win *win_ptr;
    int acquire_lock_fail = 0;
    int mpi_errno = MPI_SUCCESS;
    MPI_Aint type_size;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3_PKTHANDLER_GETACCUMULATE);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3_PKTHANDLER_GETACCUMULATE);

    MPIU_DBG_MSG(CH3_OTHER, VERBOSE, "received accumulate pkt");

    MPIU_Assert(get_accum_pkt->target_win_handle != MPI_WIN_NULL);
    MPID_Win_get_ptr(get_accum_pkt->target_win_handle, win_ptr);
    mpi_errno = MPIDI_CH3_Start_rma_op_target(win_ptr, get_accum_pkt->flags);

    mpi_errno = check_piggyback_lock(win_ptr, pkt, &acquire_lock_fail);
    if (mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);

    if (acquire_lock_fail) {
        (*buflen) = sizeof(MPIDI_CH3_Pkt_t);
        (*rreqp) = NULL;
        goto fn_exit;
    }

    data_len = *buflen - sizeof(MPIDI_CH3_Pkt_t);
    data_buf = (char *) pkt + sizeof(MPIDI_CH3_Pkt_t);

    req = MPID_Request_create();
    MPIU_Object_set_ref(req, 1);
    *rreqp = req;

    req->dev.user_count = get_accum_pkt->count;
    req->dev.op = get_accum_pkt->op;
    req->dev.real_user_buf = get_accum_pkt->addr;
    req->dev.target_win_handle = get_accum_pkt->target_win_handle;
    req->dev.source_win_handle = get_accum_pkt->source_win_handle;
    req->dev.flags = get_accum_pkt->flags;

    req->dev.resp_request_handle = get_accum_pkt->request_handle;

    if (MPIR_DATATYPE_IS_PREDEFINED(get_accum_pkt->datatype)) {
        MPIDI_Request_set_type(req, MPIDI_REQUEST_TYPE_GET_ACCUM_RESP);
        req->dev.datatype = get_accum_pkt->datatype;

        MPIR_Type_get_true_extent_impl(get_accum_pkt->datatype, &true_lb, &true_extent);
        MPID_Datatype_get_extent_macro(get_accum_pkt->datatype, extent);

        /* Predefined types should always have zero lb */
        MPIU_Assert(true_lb == 0);

        tmp_buf = MPIU_Malloc(get_accum_pkt->count * (MPIR_MAX(extent, true_extent)));
        if (!tmp_buf) {
            MPIU_ERR_SETANDJUMP1(mpi_errno, MPI_ERR_OTHER, "**nomem", "**nomem %d",
                                 get_accum_pkt->count * MPIR_MAX(extent, true_extent));
        }

        req->dev.user_buf = tmp_buf;
        req->dev.final_user_buf = req->dev.user_buf;

        MPID_Datatype_get_size_macro(get_accum_pkt->datatype, type_size);
        req->dev.recv_data_sz = type_size * get_accum_pkt->count;

        if (get_accum_pkt->immed_len > 0) {
            /* See if we can receive some data from packet header. */
            MPIU_Memcpy(req->dev.user_buf, get_accum_pkt->data, get_accum_pkt->immed_len);
            req->dev.user_buf = (void*)((char*)req->dev.user_buf + get_accum_pkt->immed_len);
            req->dev.recv_data_sz -= get_accum_pkt->immed_len;
        }

        if (req->dev.recv_data_sz == 0) {
            /* All data received. */

            *buflen = sizeof(MPIDI_CH3_Pkt_t);

            mpi_errno = MPIDI_CH3_ReqHandler_GaccumRecvComplete(vc, req, &complete);
            if (mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);
            if (complete) {
                *rreqp = NULL;
                goto fn_exit;
            }
        }

        mpi_errno = MPIDI_CH3U_Receive_data_found(req, data_buf, &data_len, &complete);
        MPIU_ERR_CHKANDJUMP1(mpi_errno, mpi_errno, MPI_ERR_OTHER, "**ch3|postrecv",
                             "**ch3|postrecv %s", "MPIDI_CH3_PKT_ACCUMULATE");
        /* FIXME:  Only change the handling of completion if
         * post_data_receive reset the handler.  There should
         * be a cleaner way to do this */
        if (!req->dev.OnDataAvail) {
            req->dev.OnDataAvail = MPIDI_CH3_ReqHandler_GaccumRecvComplete;
        }
        /* return the number of bytes processed in this function */
        *buflen = data_len + sizeof(MPIDI_CH3_Pkt_t);

        if (complete) {
            mpi_errno = MPIDI_CH3_ReqHandler_GaccumRecvComplete(vc, req, &complete);
            if (mpi_errno)
                MPIU_ERR_POP(mpi_errno);
            if (complete) {
                *rreqp = NULL;
                goto fn_exit;
            }
        }
    }
    else {
        MPIDI_Request_set_type(req, MPIDI_REQUEST_TYPE_GET_ACCUM_RESP_DERIVED_DT);
        req->dev.OnDataAvail = MPIDI_CH3_ReqHandler_GaccumDerivedDTRecvComplete;
        req->dev.datatype = MPI_DATATYPE_NULL;
        req->dev.OnFinal = MPIDI_CH3_ReqHandler_GaccumRecvComplete;

        req->dev.dtype_info = (MPIDI_RMA_dtype_info *)
            MPIU_Malloc(sizeof(MPIDI_RMA_dtype_info));
        if (!req->dev.dtype_info) {
            MPIU_ERR_SETANDJUMP1(mpi_errno, MPI_ERR_OTHER, "**nomem", "**nomem %s",
                                 "MPIDI_RMA_dtype_info");
        }

        req->dev.dataloop = MPIU_Malloc(get_accum_pkt->dataloop_size);
        if (!req->dev.dataloop) {
            MPIU_ERR_SETANDJUMP1(mpi_errno, MPI_ERR_OTHER, "**nomem", "**nomem %d",
                                 get_accum_pkt->dataloop_size);
        }

        if (data_len >= sizeof(MPIDI_RMA_dtype_info) + get_accum_pkt->dataloop_size) {
            /* copy all of dtype_info and dataloop */
            MPIU_Memcpy(req->dev.dtype_info, data_buf, sizeof(MPIDI_RMA_dtype_info));
            MPIU_Memcpy(req->dev.dataloop, data_buf + sizeof(MPIDI_RMA_dtype_info),
                        get_accum_pkt->dataloop_size);

            *buflen =
                sizeof(MPIDI_CH3_Pkt_t) + sizeof(MPIDI_RMA_dtype_info) + get_accum_pkt->dataloop_size;

            /* All dtype data has been received, call req handler */
            mpi_errno = MPIDI_CH3_ReqHandler_GaccumDerivedDTRecvComplete(vc, req, &complete);
            MPIU_ERR_CHKANDJUMP1(mpi_errno, mpi_errno, MPI_ERR_OTHER, "**ch3|postrecv",
                                 "**ch3|postrecv %s", "MPIDI_CH3_ACCUMULATE");
            if (complete) {
                *rreqp = NULL;
                goto fn_exit;
            }
        }
        else {
            req->dev.iov[0].MPID_IOV_BUF = (MPID_IOV_BUF_CAST) req->dev.dtype_info;
            req->dev.iov[0].MPID_IOV_LEN = sizeof(MPIDI_RMA_dtype_info);
            req->dev.iov[1].MPID_IOV_BUF = (MPID_IOV_BUF_CAST) req->dev.dataloop;
            req->dev.iov[1].MPID_IOV_LEN = get_accum_pkt->dataloop_size;
            req->dev.iov_count = 2;
            *buflen = sizeof(MPIDI_CH3_Pkt_t);
        }

    }

    if (mpi_errno != MPI_SUCCESS) {
        MPIU_ERR_SETANDJUMP1(mpi_errno, MPI_ERR_OTHER, "**ch3|postrecv",
                             "**ch3|postrecv %s", "MPIDI_CH3_PKT_ACCUMULATE");
    }

  fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3_PKTHANDLER_GETACCUMULATE);
    return mpi_errno;
  fn_fail:
    goto fn_exit;

}

/* Special accumulate for short data items entirely within the packet */
#undef FUNCNAME
#define FUNCNAME MPIDI_CH3_PktHandler_Accumulate_Immed
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3_PktHandler_Accumulate_Immed(MPIDI_VC_t * vc, MPIDI_CH3_Pkt_t * pkt,
                                          MPIDI_msg_sz_t * buflen, MPID_Request ** rreqp)
{
    MPIDI_CH3_Pkt_accum_immed_t *accum_pkt = &pkt->accum_immed;
    MPID_Win *win_ptr;
    MPI_Aint extent;
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3_PKTHANDLER_ACCUMULATE_IMMED);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3_PKTHANDLER_ACCUMULATE_IMMED);

    MPIU_DBG_MSG(CH3_OTHER, VERBOSE, "received accumulate immedidate pkt");

    MPIU_Assert(accum_pkt->target_win_handle != MPI_WIN_NULL);
    MPID_Win_get_ptr(accum_pkt->target_win_handle, win_ptr);
    mpi_errno = MPIDI_CH3_Start_rma_op_target(win_ptr, accum_pkt->flags);

    /* return the number of bytes processed in this function */
    /* data_len == 0 (all within packet) */
    *buflen = sizeof(MPIDI_CH3_Pkt_t);
    *rreqp = NULL;

    MPID_Datatype_get_extent_macro(accum_pkt->datatype, extent);

    if (win_ptr->shm_allocated == TRUE)
        MPIDI_CH3I_SHM_MUTEX_LOCK(win_ptr);
    /* Data is already present */
    if (accum_pkt->op == MPI_REPLACE) {
        /* no datatypes required */
        int len;
        MPIU_Assign_trunc(len, (accum_pkt->count * extent), int);
        /* FIXME: use immediate copy because this is short */
        MPIUI_Memcpy(accum_pkt->addr, accum_pkt->data, len);
    }
    else {
        if (HANDLE_GET_KIND(accum_pkt->op) == HANDLE_KIND_BUILTIN) {
            MPI_User_function *uop;
            /* get the function by indexing into the op table */
            uop = MPIR_OP_HDL_TO_FN(accum_pkt->op);
            (*uop) (accum_pkt->data, accum_pkt->addr, &(accum_pkt->count), &(accum_pkt->datatype));
        }
        else {
            MPIU_ERR_SETANDJUMP1(mpi_errno, MPI_ERR_OP, "**opnotpredefined",
                                 "**opnotpredefined %d", accum_pkt->op);
        }
    }
    if (win_ptr->shm_allocated == TRUE)
        MPIDI_CH3I_SHM_MUTEX_UNLOCK(win_ptr);

    /* There are additional steps to take if this is a passive
     * target RMA or the last operation from the source */

    /* Here is the code executed in PutAccumRespComplete after the
     * accumulation operation */
    MPID_Win_get_ptr(accum_pkt->target_win_handle, win_ptr);

    mpi_errno = MPIDI_CH3_Finish_rma_op_target(vc, win_ptr, TRUE,
                                               accum_pkt->flags, accum_pkt->source_win_handle);
    if (mpi_errno) {
        MPIU_ERR_POP(mpi_errno);
    }

  fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3_PKTHANDLER_ACCUMULATE_IMMED);
    return mpi_errno;
  fn_fail:
    goto fn_exit;

}


#undef FUNCNAME
#define FUNCNAME MPIDI_CH3_PktHandler_CAS
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3_PktHandler_CAS(MPIDI_VC_t * vc, MPIDI_CH3_Pkt_t * pkt,
                             MPIDI_msg_sz_t * buflen, MPID_Request ** rreqp)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_CH3_Pkt_t upkt;
    MPIDI_CH3_Pkt_cas_resp_t *cas_resp_pkt = &upkt.cas_resp;
    MPIDI_CH3_Pkt_cas_t *cas_pkt = &pkt->cas;
    MPID_Win *win_ptr;
    MPID_Request *req;
    MPI_Aint len;
    int acquire_lock_fail = 0;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3_PKTHANDLER_CAS);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3_PKTHANDLER_CAS);

    MPIU_DBG_MSG(CH3_OTHER, VERBOSE, "received CAS pkt");

    MPIU_Assert(cas_pkt->target_win_handle != MPI_WIN_NULL);
    MPID_Win_get_ptr(cas_pkt->target_win_handle, win_ptr);
    mpi_errno = MPIDI_CH3_Start_rma_op_target(win_ptr, cas_pkt->flags);

    /* return the number of bytes processed in this function */
    /* data_len == 0 (all within packet) */
    *buflen = sizeof(MPIDI_CH3_Pkt_t);
    *rreqp = NULL;

    mpi_errno = check_piggyback_lock(win_ptr, pkt, &acquire_lock_fail);
    if (mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);

    if (acquire_lock_fail)
        goto fn_exit;

    MPIDI_Pkt_init(cas_resp_pkt, MPIDI_CH3_PKT_CAS_RESP);
    cas_resp_pkt->request_handle = cas_pkt->request_handle;
    cas_resp_pkt->source_win_handle = cas_pkt->source_win_handle;
    cas_resp_pkt->target_rank = win_ptr->comm_ptr->rank;
    cas_resp_pkt->flags = MPIDI_CH3_PKT_FLAG_NONE;
    if (cas_pkt->flags & MPIDI_CH3_PKT_FLAG_RMA_LOCK_GRANTED)
        cas_resp_pkt->flags |= MPIDI_CH3_PKT_FLAG_RMA_LOCK_GRANTED;
    if (cas_pkt->flags & MPIDI_CH3_PKT_FLAG_RMA_FLUSH)
        cas_resp_pkt->flags |= MPIDI_CH3_PKT_FLAG_RMA_FLUSH_ACK;
    if (cas_pkt->flags & MPIDI_CH3_PKT_FLAG_RMA_UNLOCK)
        cas_resp_pkt->flags |= MPIDI_CH3_PKT_FLAG_RMA_UNLOCK_ACK;

    /* Copy old value into the response packet */
    MPID_Datatype_get_size_macro(cas_pkt->datatype, len);
    MPIU_Assert(len <= sizeof(MPIDI_CH3_CAS_Immed_u));

    if (win_ptr->shm_allocated == TRUE)
        MPIDI_CH3I_SHM_MUTEX_LOCK(win_ptr);

    MPIU_Memcpy((void *) &cas_resp_pkt->data, cas_pkt->addr, len);

    /* Compare and replace if equal */
    if (MPIR_Compare_equal(&cas_pkt->compare_data, cas_pkt->addr, cas_pkt->datatype)) {
        MPIU_Memcpy(cas_pkt->addr, &cas_pkt->origin_data, len);
    }

    if (win_ptr->shm_allocated == TRUE)
        MPIDI_CH3I_SHM_MUTEX_UNLOCK(win_ptr);

    /* Send the response packet */
    MPIU_THREAD_CS_ENTER(CH3COMM, vc);
    mpi_errno = MPIDI_CH3_iStartMsg(vc, cas_resp_pkt, sizeof(*cas_resp_pkt), &req);
    MPIU_THREAD_CS_EXIT(CH3COMM, vc);

    MPIU_ERR_CHKANDJUMP(mpi_errno != MPI_SUCCESS, mpi_errno, MPI_ERR_OTHER, "**ch3|rmamsg");

    if (req != NULL) {
        if (!MPID_Request_is_complete(req)) {
            /* sending process is not completed, set proper OnDataAvail
               (it is initialized to NULL by lower layer) */
            req->dev.target_win_handle = cas_pkt->target_win_handle;
            req->dev.flags = cas_pkt->flags;
            req->dev.OnDataAvail = MPIDI_CH3_ReqHandler_GaccumLikeSendComplete;

            /* here we increment the Active Target counter to guarantee the GET-like
               operation are completed when counter reaches zero. */
            win_ptr->at_completion_counter++;

            MPID_Request_release(req);
            goto fn_exit;
        }
        else
            MPID_Request_release(req);
    }

    if (cas_pkt->flags & MPIDI_CH3_PKT_FLAG_RMA_UNLOCK) {
        mpi_errno = MPIDI_CH3I_Release_lock(win_ptr);
        if (mpi_errno) MPIU_ERR_POP(mpi_errno);
        MPIDI_CH3_Progress_signal_completion();
    }
    if (cas_pkt->flags & MPIDI_CH3_PKT_FLAG_RMA_DECR_AT_COUNTER) {
        win_ptr->at_completion_counter--;
        MPIU_Assert(win_ptr->at_completion_counter >= 0);
        /* Signal the local process when the op counter reaches 0. */
        if (win_ptr->at_completion_counter == 0)
            MPIDI_CH3_Progress_signal_completion();
    }

    /* There are additional steps to take if this is a passive
     * target RMA or the last operation from the source */

    mpi_errno = MPIDI_CH3_Finish_rma_op_target(NULL, win_ptr, TRUE, cas_pkt->flags, MPI_WIN_NULL);
    if (mpi_errno) {
        MPIU_ERR_POP(mpi_errno);
    }

  fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3_PKTHANDLER_CAS);
    return mpi_errno;
  fn_fail:
    goto fn_exit;

}


#undef FUNCNAME
#define FUNCNAME MPIDI_CH3_PktHandler_CASResp
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3_PktHandler_CASResp(MPIDI_VC_t * vc ATTRIBUTE((unused)),
                                 MPIDI_CH3_Pkt_t * pkt,
                                 MPIDI_msg_sz_t * buflen, MPID_Request ** rreqp)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_CH3_Pkt_cas_resp_t *cas_resp_pkt = &pkt->cas_resp;
    MPID_Request *req;
    MPI_Aint len;
    MPID_Win *win_ptr;
    int target_rank = cas_resp_pkt->target_rank;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3_PKTHANDLER_CASRESP);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3_PKTHANDLER_CASRESP);

    MPIU_DBG_MSG(CH3_OTHER, VERBOSE, "received CAS response pkt");

    MPID_Win_get_ptr(cas_resp_pkt->source_win_handle, win_ptr);

    /* decrement ack_counter on this target */
    if (cas_resp_pkt->flags & MPIDI_CH3_PKT_FLAG_RMA_LOCK_GRANTED) {
        mpi_errno = set_lock_sync_counter(win_ptr, target_rank);
        if (mpi_errno) MPIU_ERR_POP(mpi_errno);
    }
    if (cas_resp_pkt->flags & MPIDI_CH3_PKT_FLAG_RMA_FLUSH_ACK) {
        mpi_errno = MPIDI_CH3I_RMA_Handle_flush_ack(win_ptr, target_rank);
        if (mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);
    }
    if (cas_resp_pkt->flags & MPIDI_CH3_PKT_FLAG_RMA_UNLOCK_ACK) {
        mpi_errno = MPIDI_CH3I_RMA_Handle_flush_ack(win_ptr, target_rank);
        if (mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);
    }

    MPID_Request_get_ptr(cas_resp_pkt->request_handle, req);
    MPID_Datatype_get_size_macro(req->dev.datatype, len);

    MPIU_Memcpy(req->dev.user_buf, (void *) &cas_resp_pkt->data, len);

    MPIDI_CH3U_Request_complete(req);
    *buflen = sizeof(MPIDI_CH3_Pkt_t);
    *rreqp = NULL;

  fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3_PKTHANDLER_CASRESP);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}


#undef FUNCNAME
#define FUNCNAME MPIDI_CH3_PktHandler_FOP
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3_PktHandler_FOP(MPIDI_VC_t * vc, MPIDI_CH3_Pkt_t * pkt,
                             MPIDI_msg_sz_t * buflen, MPID_Request ** rreqp)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_CH3_Pkt_fop_t *fop_pkt = &pkt->fop;
    MPIDI_CH3_Pkt_t upkt;
    MPIDI_CH3_Pkt_fop_resp_t *fop_resp_pkt = &upkt.fop_resp;
    MPID_Request *resp_req = NULL;
    int acquire_lock_fail = 0;
    MPID_Win *win_ptr = NULL;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3_PKTHANDLER_FOP);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3_PKTHANDLER_FOP);

    MPIU_DBG_MSG(CH3_OTHER, VERBOSE, "received FOP pkt");

    MPID_Win_get_ptr(fop_pkt->target_win_handle, win_ptr);

    (*buflen) = sizeof(MPIDI_CH3_Pkt_t);
    (*rreqp) = NULL;

    mpi_errno = check_piggyback_lock(win_ptr, pkt, &acquire_lock_fail);
    if (mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);

    if (acquire_lock_fail) {
        goto fn_exit;
    }

    MPIDI_Pkt_init(fop_resp_pkt, MPIDI_CH3_PKT_FOP_RESP);
    fop_resp_pkt->request_handle = fop_pkt->request_handle;
    fop_resp_pkt->source_win_handle = fop_pkt->source_win_handle;
    fop_resp_pkt->target_rank = win_ptr->comm_ptr->rank;
    fop_resp_pkt->flags = MPIDI_CH3_PKT_FLAG_NONE;
    if (fop_pkt->flags & MPIDI_CH3_PKT_FLAG_RMA_LOCK_GRANTED)
        fop_resp_pkt->flags |= MPIDI_CH3_PKT_FLAG_RMA_LOCK_GRANTED;
    if (fop_pkt->flags & MPIDI_CH3_PKT_FLAG_RMA_FLUSH)
        fop_resp_pkt->flags |= MPIDI_CH3_PKT_FLAG_RMA_FLUSH_ACK;
    if (fop_pkt->flags & MPIDI_CH3_PKT_FLAG_RMA_UNLOCK)
        fop_resp_pkt->flags |= MPIDI_CH3_PKT_FLAG_RMA_UNLOCK_ACK;
    fop_resp_pkt->immed_len = fop_pkt->immed_len;

    /* copy data to resp pkt header */
    void *src = fop_pkt->addr, *dest = fop_resp_pkt->data;
    mpi_errno = immed_copy(src, dest, fop_resp_pkt->immed_len);
    if (mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);

    /* Apply the op */
    if (fop_pkt->op != MPI_NO_OP) {
        MPI_User_function *uop = MPIR_OP_HDL_TO_FN(fop_pkt->op);
        int one = 1;
        if (win_ptr->shm_allocated == TRUE)
            MPIDI_CH3I_SHM_MUTEX_LOCK(win_ptr);
        (*uop)(fop_pkt->data, fop_pkt->addr, &one, &(fop_pkt->datatype));
        if (win_ptr->shm_allocated == TRUE)
            MPIDI_CH3I_SHM_MUTEX_UNLOCK(win_ptr);
    }

    /* send back the original data */
    MPIU_THREAD_CS_ENTER(CH3COMM,vc);
    mpi_errno = MPIDI_CH3_iStartMsg(vc, fop_resp_pkt, sizeof(*fop_resp_pkt), &resp_req);
    MPIU_THREAD_CS_EXIT(CH3COMM,vc);
    MPIU_ERR_CHKANDJUMP(mpi_errno != MPI_SUCCESS, mpi_errno, MPI_ERR_OTHER, "**ch3|rmamsg");

    if (resp_req != NULL) {
        if (!MPID_Request_is_complete(resp_req)) {
            /* sending process is not completed, set proper OnDataAvail
               (it is initialized to NULL by lower layer) */
            resp_req->dev.target_win_handle = fop_pkt->target_win_handle;
            resp_req->dev.flags = fop_pkt->flags;
            resp_req->dev.OnDataAvail = MPIDI_CH3_ReqHandler_GaccumLikeSendComplete;

            /* here we increment the Active Target counter to guarantee the GET-like
               operation are completed when counter reaches zero. */
            win_ptr->at_completion_counter++;

            MPID_Request_release(resp_req);
            goto fn_exit;
        }
        else {
            MPID_Request_release(resp_req);
        }
    }

    if (fop_pkt->flags & MPIDI_CH3_PKT_FLAG_RMA_UNLOCK) {
        mpi_errno = MPIDI_CH3I_Release_lock(win_ptr);
        if (mpi_errno) MPIU_ERR_POP(mpi_errno);
        MPIDI_CH3_Progress_signal_completion();
    }

    if (fop_pkt->flags & MPIDI_CH3_PKT_FLAG_RMA_DECR_AT_COUNTER) {
        win_ptr->at_completion_counter--;
        MPIU_Assert(win_ptr->at_completion_counter >= 0);
        if (win_ptr->at_completion_counter == 0)
            MPIDI_CH3_Progress_signal_completion();
    }

  fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3_PKTHANDLER_FOP);
    return mpi_errno;
    /* --BEGIN ERROR HANDLING-- */
  fn_fail:
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}


#undef FUNCNAME
#define FUNCNAME MPIDI_CH3_PktHandler_FOPResp
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3_PktHandler_FOPResp(MPIDI_VC_t * vc ATTRIBUTE((unused)),
                                 MPIDI_CH3_Pkt_t * pkt,
                                 MPIDI_msg_sz_t * buflen, MPID_Request ** rreqp)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_CH3_Pkt_fop_resp_t *fop_resp_pkt = &pkt->fop_resp;
    MPID_Request *req = NULL;
    MPID_Win *win_ptr = NULL;
    int target_rank = fop_resp_pkt->target_rank;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3_PKTHANDLER_FOPRESP);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3_PKTHANDLER_FOPRESP);

    MPIU_DBG_MSG(CH3_OTHER, VERBOSE, "received FOP response pkt");

    MPID_Win_get_ptr(fop_resp_pkt->source_win_handle, win_ptr);

    /* Copy data to result buffer on orgin */
    MPID_Request_get_ptr(fop_resp_pkt->request_handle, req);
    MPIU_Memcpy(req->dev.user_buf, fop_resp_pkt->data, fop_resp_pkt->immed_len);

    /* decrement ack_counter */
    if (fop_resp_pkt->flags & MPIDI_CH3_PKT_FLAG_RMA_LOCK_GRANTED) {
        mpi_errno = set_lock_sync_counter(win_ptr, target_rank);
        if (mpi_errno) MPIU_ERR_POP(mpi_errno);
    }
    if (fop_resp_pkt->flags & MPIDI_CH3_PKT_FLAG_RMA_FLUSH_ACK) {
        mpi_errno = MPIDI_CH3I_RMA_Handle_flush_ack(win_ptr, target_rank);
        if (mpi_errno) MPIU_ERR_POP(mpi_errno);
    }
    if (fop_resp_pkt->flags & MPIDI_CH3_PKT_FLAG_RMA_UNLOCK_ACK) {
        mpi_errno = MPIDI_CH3I_RMA_Handle_flush_ack(win_ptr, target_rank);
        if (mpi_errno) MPIU_ERR_POP(mpi_errno);
    }

    MPIDI_CH3U_Request_complete(req);
    *buflen = sizeof(MPIDI_CH3_Pkt_t);
    *rreqp = NULL;

  fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3_PKTHANDLER_FOPRESP);
    return mpi_errno;
    /* --BEGIN ERROR HANDLING-- */
  fn_fail:
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}


#undef FUNCNAME
#define FUNCNAME MPIDI_CH3_PktHandler_Get_AccumResp
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3_PktHandler_Get_AccumResp(MPIDI_VC_t * vc, MPIDI_CH3_Pkt_t * pkt,
                                       MPIDI_msg_sz_t * buflen, MPID_Request ** rreqp)
{
    MPIDI_CH3_Pkt_get_accum_resp_t *get_accum_resp_pkt = &pkt->get_accum_resp;
    MPID_Request *req;
    int complete;
    char *data_buf = NULL;
    MPIDI_msg_sz_t data_len;
    int mpi_errno = MPI_SUCCESS;
    MPI_Aint type_size;
    MPID_Win *win_ptr;
    int target_rank = get_accum_resp_pkt->target_rank;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3_PKTHANDLER_GET_ACCUM_RESP);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3_PKTHANDLER_GET_ACCUM_RESP);

    MPIU_DBG_MSG(CH3_OTHER, VERBOSE, "received Get-Accumulate response pkt");

    MPID_Win_get_ptr(get_accum_resp_pkt->source_win_handle, win_ptr);

    /* decrement ack_counter on target */
    if (get_accum_resp_pkt->flags & MPIDI_CH3_PKT_FLAG_RMA_LOCK_GRANTED) {
        mpi_errno = set_lock_sync_counter(win_ptr, target_rank);
        if (mpi_errno) MPIU_ERR_POP(mpi_errno);
    }
    if (get_accum_resp_pkt->flags & MPIDI_CH3_PKT_FLAG_RMA_FLUSH_ACK) {
        mpi_errno = MPIDI_CH3I_RMA_Handle_flush_ack(win_ptr, target_rank);
        if (mpi_errno) MPIU_ERR_POP(mpi_errno);
    }
    if (get_accum_resp_pkt->flags & MPIDI_CH3_PKT_FLAG_RMA_UNLOCK_ACK) {
        mpi_errno = MPIDI_CH3I_RMA_Handle_flush_ack(win_ptr, target_rank);
        if (mpi_errno) MPIU_ERR_POP(mpi_errno);
    }

    data_len = *buflen - sizeof(MPIDI_CH3_Pkt_t);
    data_buf = (char *) pkt + sizeof(MPIDI_CH3_Pkt_t);

    MPID_Request_get_ptr(get_accum_resp_pkt->request_handle, req);

    MPID_Datatype_get_size_macro(req->dev.datatype, type_size);
    req->dev.recv_data_sz = type_size * req->dev.user_count;

    *rreqp = req;
    mpi_errno = MPIDI_CH3U_Receive_data_found(req, data_buf, &data_len, &complete);
    MPIU_ERR_CHKANDJUMP1(mpi_errno, mpi_errno, MPI_ERR_OTHER, "**ch3|postrecv",
                         "**ch3|postrecv %s", "MPIDI_CH3_PKT_GET_ACCUM_RESP");
    if (complete) {
        MPIDI_CH3U_Request_complete(req);
        *rreqp = NULL;
    }
    /* return the number of bytes processed in this function */
    *buflen = data_len + sizeof(MPIDI_CH3_Pkt_t);

  fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3_PKTHANDLER_GET_ACCUM_RESP);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}


#undef FUNCNAME
#define FUNCNAME MPIDI_CH3_PktHandler_Lock
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3_PktHandler_Lock(MPIDI_VC_t * vc, MPIDI_CH3_Pkt_t * pkt,
                              MPIDI_msg_sz_t * buflen, MPID_Request ** rreqp)
{
    MPIDI_CH3_Pkt_lock_t *lock_pkt = &pkt->lock;
    MPID_Win *win_ptr = NULL;
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3_PKTHANDLER_LOCK);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3_PKTHANDLER_LOCK);

    MPIU_DBG_MSG(CH3_OTHER, VERBOSE, "received lock pkt");

    *buflen = sizeof(MPIDI_CH3_Pkt_t);

    MPID_Win_get_ptr(lock_pkt->target_win_handle, win_ptr);

    if (MPIDI_CH3I_Try_acquire_win_lock(win_ptr, lock_pkt->lock_type) == 1) {
        /* send lock granted packet. */
        mpi_errno = MPIDI_CH3I_Send_lock_granted_pkt(vc, win_ptr, lock_pkt->source_win_handle);
        if (mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);
    }

    else {
        mpi_errno = enqueue_lock_origin(win_ptr, pkt);
        if (mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);
    }

    *rreqp = NULL;
  fn_fail:
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3_PKTHANDLER_LOCK);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3_PktHandler_LockPutUnlock
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3_PktHandler_LockPutUnlock(MPIDI_VC_t * vc, MPIDI_CH3_Pkt_t * pkt,
                                       MPIDI_msg_sz_t * buflen, MPID_Request ** rreqp)
{
    MPIDI_CH3_Pkt_lock_put_unlock_t *lock_put_unlock_pkt = &pkt->lock_put_unlock;
    MPID_Win *win_ptr = NULL;
    MPID_Request *req = NULL;
    MPI_Aint type_size;
    int complete;
    char *data_buf = NULL;
    MPIDI_msg_sz_t data_len;
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3_PKTHANDLER_LOCKPUTUNLOCK);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3_PKTHANDLER_LOCKPUTUNLOCK);

    MPIU_DBG_MSG(CH3_OTHER, VERBOSE, "received lock_put_unlock pkt");

    data_len = *buflen - sizeof(MPIDI_CH3_Pkt_t);
    data_buf = (char *) pkt + sizeof(MPIDI_CH3_Pkt_t);

    req = MPID_Request_create();
    MPIU_Object_set_ref(req, 1);

    req->dev.datatype = lock_put_unlock_pkt->datatype;
    MPID_Datatype_get_size_macro(lock_put_unlock_pkt->datatype, type_size);
    req->dev.recv_data_sz = type_size * lock_put_unlock_pkt->count;
    req->dev.user_count = lock_put_unlock_pkt->count;
    req->dev.target_win_handle = lock_put_unlock_pkt->target_win_handle;

    MPID_Win_get_ptr(lock_put_unlock_pkt->target_win_handle, win_ptr);

    if (MPIDI_CH3I_Try_acquire_win_lock(win_ptr, lock_put_unlock_pkt->lock_type) == 1) {
        /* do the put. for this optimization, only basic datatypes supported. */
        MPIDI_Request_set_type(req, MPIDI_REQUEST_TYPE_PUT_RESP);
        req->dev.OnDataAvail = MPIDI_CH3_ReqHandler_PutAccumRespComplete;
        req->dev.user_buf = lock_put_unlock_pkt->addr;
        req->dev.source_win_handle = lock_put_unlock_pkt->source_win_handle;
        req->dev.flags = lock_put_unlock_pkt->flags;
    }

    else {
        /* queue the information */
        MPIDI_Win_lock_queue *curr_ptr, *prev_ptr, *new_ptr;

        new_ptr = (MPIDI_Win_lock_queue *) MPIU_Malloc(sizeof(MPIDI_Win_lock_queue));
        if (!new_ptr) {
            MPIU_ERR_SETANDJUMP1(mpi_errno, MPI_ERR_OTHER, "**nomem", "**nomem %s",
                                 "MPIDI_Win_lock_queue");
        }

        new_ptr->pt_single_op = (MPIDI_PT_single_op *) MPIU_Malloc(sizeof(MPIDI_PT_single_op));
        if (new_ptr->pt_single_op == NULL) {
            MPIU_ERR_SETANDJUMP1(mpi_errno, MPI_ERR_OTHER, "**nomem", "**nomem %s",
                                 "MPIDI_PT_single_op");
        }

        /* FIXME: MT: The queuing may need to be done atomically. */

        curr_ptr = (MPIDI_Win_lock_queue *) win_ptr->lock_queue;
        prev_ptr = curr_ptr;
        while (curr_ptr != NULL) {
            prev_ptr = curr_ptr;
            curr_ptr = curr_ptr->next;
        }

        if (prev_ptr != NULL)
            prev_ptr->next = new_ptr;
        else
            win_ptr->lock_queue = new_ptr;

        new_ptr->next = NULL;
        new_ptr->lock_type = lock_put_unlock_pkt->lock_type;
        new_ptr->source_win_handle = lock_put_unlock_pkt->source_win_handle;
        new_ptr->origin_rank = lock_put_unlock_pkt->origin_rank;

        new_ptr->pt_single_op->type = MPIDI_CH3_PKT_LOCK_PUT_UNLOCK;
        new_ptr->pt_single_op->flags = lock_put_unlock_pkt->flags;
        new_ptr->pt_single_op->addr = lock_put_unlock_pkt->addr;
        new_ptr->pt_single_op->count = lock_put_unlock_pkt->count;
        new_ptr->pt_single_op->datatype = lock_put_unlock_pkt->datatype;
        /* allocate memory to receive the data */
        new_ptr->pt_single_op->data = MPIU_Malloc(req->dev.recv_data_sz);
        if (new_ptr->pt_single_op->data == NULL) {
            MPIU_ERR_SETANDJUMP1(mpi_errno, MPI_ERR_OTHER, "**nomem", "**nomem %d",
                                 req->dev.recv_data_sz);
        }

        new_ptr->pt_single_op->data_recd = 0;

        MPIDI_Request_set_type(req, MPIDI_REQUEST_TYPE_PT_SINGLE_PUT);
        req->dev.OnDataAvail = MPIDI_CH3_ReqHandler_SinglePutAccumComplete;
        req->dev.user_buf = new_ptr->pt_single_op->data;
        req->dev.lock_queue_entry = new_ptr;
    }

    int (*fcn) (MPIDI_VC_t *, struct MPID_Request *, int *);
    fcn = req->dev.OnDataAvail;
    mpi_errno = MPIDI_CH3U_Receive_data_found(req, data_buf, &data_len, &complete);
    if (mpi_errno != MPI_SUCCESS) {
        MPIU_ERR_SETFATALANDJUMP1(mpi_errno, MPI_ERR_OTHER,
                                  "**ch3|postrecv", "**ch3|postrecv %s",
                                  "MPIDI_CH3_PKT_LOCK_PUT_UNLOCK");
    }
    req->dev.OnDataAvail = fcn;
    *rreqp = req;

    if (complete) {
        mpi_errno = fcn(vc, req, &complete);
        if (complete) {
            *rreqp = NULL;
        }
    }

    /* return the number of bytes processed in this function */
    *buflen = data_len + sizeof(MPIDI_CH3_Pkt_t);

    if (mpi_errno != MPI_SUCCESS) {
        MPIU_ERR_SETFATALANDJUMP1(mpi_errno, MPI_ERR_OTHER,
                                  "**ch3|postrecv", "**ch3|postrecv %s",
                                  "MPIDI_CH3_PKT_LOCK_PUT_UNLOCK");
    }

  fn_fail:
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3_PKTHANDLER_LOCKPUTUNLOCK);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3_PktHandler_LockGetUnlock
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3_PktHandler_LockGetUnlock(MPIDI_VC_t * vc, MPIDI_CH3_Pkt_t * pkt,
                                       MPIDI_msg_sz_t * buflen, MPID_Request ** rreqp)
{
    MPIDI_CH3_Pkt_lock_get_unlock_t *lock_get_unlock_pkt = &pkt->lock_get_unlock;
    MPID_Win *win_ptr = NULL;
    MPI_Aint type_size;
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3_PKTHANDLER_LOCKGETUNLOCK);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3_PKTHANDLER_LOCKGETUNLOCK);

    MPIU_DBG_MSG(CH3_OTHER, VERBOSE, "received lock_get_unlock pkt");

    *buflen = sizeof(MPIDI_CH3_Pkt_t);

    MPID_Win_get_ptr(lock_get_unlock_pkt->target_win_handle, win_ptr);

    if (MPIDI_CH3I_Try_acquire_win_lock(win_ptr, lock_get_unlock_pkt->lock_type) == 1) {
        /* do the get. for this optimization, only basic datatypes supported. */
        MPIDI_CH3_Pkt_t upkt;
        MPIDI_CH3_Pkt_get_resp_t *get_resp_pkt = &upkt.get_resp;
        MPID_Request *req;
        MPID_IOV iov[MPID_IOV_LIMIT];

        req = MPID_Request_create();
        req->dev.target_win_handle = lock_get_unlock_pkt->target_win_handle;
        req->dev.source_win_handle = lock_get_unlock_pkt->source_win_handle;
        req->dev.flags = lock_get_unlock_pkt->flags;

        MPIDI_Request_set_type(req, MPIDI_REQUEST_TYPE_GET_RESP);
        req->dev.OnDataAvail = MPIDI_CH3_ReqHandler_GetSendRespComplete;
        req->dev.OnFinal = MPIDI_CH3_ReqHandler_GetSendRespComplete;
        req->kind = MPID_REQUEST_SEND;

        /* here we increment the Active Target counter to guarantee the GET-like
           operation are completed when counter reaches zero. */
        win_ptr->at_completion_counter++;

        MPIDI_Pkt_init(get_resp_pkt, MPIDI_CH3_PKT_GET_RESP);
        get_resp_pkt->request_handle = lock_get_unlock_pkt->request_handle;

        iov[0].MPID_IOV_BUF = (MPID_IOV_BUF_CAST) get_resp_pkt;
        iov[0].MPID_IOV_LEN = sizeof(*get_resp_pkt);

        iov[1].MPID_IOV_BUF = (MPID_IOV_BUF_CAST) lock_get_unlock_pkt->addr;
        MPID_Datatype_get_size_macro(lock_get_unlock_pkt->datatype, type_size);
        iov[1].MPID_IOV_LEN = lock_get_unlock_pkt->count * type_size;

        mpi_errno = MPIDI_CH3_iSendv(vc, req, iov, 2);
        /* --BEGIN ERROR HANDLING-- */
        if (mpi_errno != MPI_SUCCESS) {
            MPIU_Object_set_ref(req, 0);
            MPIDI_CH3_Request_destroy(req);
            MPIU_ERR_SETANDJUMP(mpi_errno, MPI_ERR_OTHER, "**ch3|rmamsg");
        }
        /* --END ERROR HANDLING-- */
    }

    else {
        /* queue the information */
        MPIDI_Win_lock_queue *curr_ptr, *prev_ptr, *new_ptr;

        /* FIXME: MT: This may need to be done atomically. */

        curr_ptr = (MPIDI_Win_lock_queue *) win_ptr->lock_queue;
        prev_ptr = curr_ptr;
        while (curr_ptr != NULL) {
            prev_ptr = curr_ptr;
            curr_ptr = curr_ptr->next;
        }

        new_ptr = (MPIDI_Win_lock_queue *) MPIU_Malloc(sizeof(MPIDI_Win_lock_queue));
        if (!new_ptr) {
            MPIU_ERR_SETANDJUMP1(mpi_errno, MPI_ERR_OTHER, "**nomem", "**nomem %s",
                                 "MPIDI_Win_lock_queue");
        }
        new_ptr->pt_single_op = (MPIDI_PT_single_op *) MPIU_Malloc(sizeof(MPIDI_PT_single_op));
        if (new_ptr->pt_single_op == NULL) {
            MPIU_ERR_SETANDJUMP1(mpi_errno, MPI_ERR_OTHER, "**nomem", "**nomem %s",
                                 "MPIDI_PT_Single_op");
        }

        if (prev_ptr != NULL)
            prev_ptr->next = new_ptr;
        else
            win_ptr->lock_queue = new_ptr;

        new_ptr->next = NULL;
        new_ptr->lock_type = lock_get_unlock_pkt->lock_type;
        new_ptr->source_win_handle = lock_get_unlock_pkt->source_win_handle;
        new_ptr->origin_rank = lock_get_unlock_pkt->origin_rank;

        new_ptr->pt_single_op->type = MPIDI_CH3_PKT_LOCK_GET_UNLOCK;
        new_ptr->pt_single_op->flags = lock_get_unlock_pkt->flags;
        new_ptr->pt_single_op->addr = lock_get_unlock_pkt->addr;
        new_ptr->pt_single_op->count = lock_get_unlock_pkt->count;
        new_ptr->pt_single_op->datatype = lock_get_unlock_pkt->datatype;
        new_ptr->pt_single_op->data = NULL;
        new_ptr->pt_single_op->request_handle = lock_get_unlock_pkt->request_handle;
        new_ptr->pt_single_op->data_recd = 1;
    }

    *rreqp = NULL;

  fn_fail:
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3_PKTHANDLER_LOCKGETUNLOCK);
    return mpi_errno;
}


#undef FUNCNAME
#define FUNCNAME MPIDI_CH3_PktHandler_LockAccumUnlock
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3_PktHandler_LockAccumUnlock(MPIDI_VC_t * vc, MPIDI_CH3_Pkt_t * pkt,
                                         MPIDI_msg_sz_t * buflen, MPID_Request ** rreqp)
{
    MPIDI_CH3_Pkt_lock_accum_unlock_t *lock_accum_unlock_pkt = &pkt->lock_accum_unlock;
    MPID_Request *req = NULL;
    MPID_Win *win_ptr = NULL;
    MPIDI_Win_lock_queue *curr_ptr = NULL, *prev_ptr = NULL, *new_ptr = NULL;
    MPI_Aint type_size;
    int complete;
    char *data_buf = NULL;
    MPIDI_msg_sz_t data_len;
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3_PKTHANDLER_LOCKACCUMUNLOCK);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3_PKTHANDLER_LOCKACCUMUNLOCK);

    MPIU_DBG_MSG(CH3_OTHER, VERBOSE, "received lock_accum_unlock pkt");

    /* no need to acquire the lock here because we need to receive the
     * data into a temporary buffer first */

    data_len = *buflen - sizeof(MPIDI_CH3_Pkt_t);
    data_buf = (char *) pkt + sizeof(MPIDI_CH3_Pkt_t);

    req = MPID_Request_create();
    MPIU_Object_set_ref(req, 1);

    req->dev.datatype = lock_accum_unlock_pkt->datatype;
    MPID_Datatype_get_size_macro(lock_accum_unlock_pkt->datatype, type_size);
    req->dev.recv_data_sz = type_size * lock_accum_unlock_pkt->count;
    req->dev.user_count = lock_accum_unlock_pkt->count;
    req->dev.target_win_handle = lock_accum_unlock_pkt->target_win_handle;
    req->dev.flags = lock_accum_unlock_pkt->flags;

    /* queue the information */

    new_ptr = (MPIDI_Win_lock_queue *) MPIU_Malloc(sizeof(MPIDI_Win_lock_queue));
    if (!new_ptr) {
        MPIU_ERR_SETANDJUMP1(mpi_errno, MPI_ERR_OTHER, "**nomem", "**nomem %s",
                             "MPIDI_Win_lock_queue");
    }

    new_ptr->pt_single_op = (MPIDI_PT_single_op *) MPIU_Malloc(sizeof(MPIDI_PT_single_op));
    if (new_ptr->pt_single_op == NULL) {
        MPIU_ERR_SETANDJUMP1(mpi_errno, MPI_ERR_OTHER, "**nomem", "**nomem %s",
                             "MPIDI_PT_single_op");
    }

    MPID_Win_get_ptr(lock_accum_unlock_pkt->target_win_handle, win_ptr);

    /* FIXME: MT: The queuing may need to be done atomically. */

    curr_ptr = (MPIDI_Win_lock_queue *) win_ptr->lock_queue;
    prev_ptr = curr_ptr;
    while (curr_ptr != NULL) {
        prev_ptr = curr_ptr;
        curr_ptr = curr_ptr->next;
    }

    if (prev_ptr != NULL)
        prev_ptr->next = new_ptr;
    else
        win_ptr->lock_queue = new_ptr;

    new_ptr->next = NULL;
    new_ptr->lock_type = lock_accum_unlock_pkt->lock_type;
    new_ptr->source_win_handle = lock_accum_unlock_pkt->source_win_handle;
    new_ptr->origin_rank = lock_accum_unlock_pkt->origin_rank;

    new_ptr->pt_single_op->type = MPIDI_CH3_PKT_LOCK_ACCUM_UNLOCK;
    new_ptr->pt_single_op->flags = lock_accum_unlock_pkt->flags;
    new_ptr->pt_single_op->addr = lock_accum_unlock_pkt->addr;
    new_ptr->pt_single_op->count = lock_accum_unlock_pkt->count;
    new_ptr->pt_single_op->datatype = lock_accum_unlock_pkt->datatype;
    new_ptr->pt_single_op->op = lock_accum_unlock_pkt->op;
    /* allocate memory to receive the data */
    new_ptr->pt_single_op->data = MPIU_Malloc(req->dev.recv_data_sz);
    if (new_ptr->pt_single_op->data == NULL) {
        MPIU_ERR_SETANDJUMP1(mpi_errno, MPI_ERR_OTHER, "**nomem", "**nomem %d",
                             req->dev.recv_data_sz);
    }

    new_ptr->pt_single_op->data_recd = 0;

    MPIDI_Request_set_type(req, MPIDI_REQUEST_TYPE_PT_SINGLE_ACCUM);
    req->dev.user_buf = new_ptr->pt_single_op->data;
    req->dev.lock_queue_entry = new_ptr;

    *rreqp = req;
    mpi_errno = MPIDI_CH3U_Receive_data_found(req, data_buf, &data_len, &complete);
    /* FIXME:  Only change the handling of completion if
     * post_data_receive reset the handler.  There should
     * be a cleaner way to do this */
    if (!req->dev.OnDataAvail) {
        req->dev.OnDataAvail = MPIDI_CH3_ReqHandler_SinglePutAccumComplete;
    }
    if (mpi_errno != MPI_SUCCESS) {
        MPIU_ERR_SET1(mpi_errno, MPI_ERR_OTHER, "**ch3|postrecv",
                      "**ch3|postrecv %s", "MPIDI_CH3_PKT_LOCK_ACCUM_UNLOCK");
    }
    /* return the number of bytes processed in this function */
    *buflen = data_len + sizeof(MPIDI_CH3_Pkt_t);

    if (complete) {
        mpi_errno = MPIDI_CH3_ReqHandler_SinglePutAccumComplete(vc, req, &complete);
        if (complete) {
            *rreqp = NULL;
        }
    }
  fn_fail:
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3_PKTHANDLER_LOCKACCUMUNLOCK);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3_PktHandler_GetResp
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3_PktHandler_GetResp(MPIDI_VC_t * vc ATTRIBUTE((unused)),
                                 MPIDI_CH3_Pkt_t * pkt,
                                 MPIDI_msg_sz_t * buflen, MPID_Request ** rreqp)
{
    MPIDI_CH3_Pkt_get_resp_t *get_resp_pkt = &pkt->get_resp;
    MPID_Request *req;
    int complete;
    char *data_buf = NULL;
    MPIDI_msg_sz_t data_len;
    int mpi_errno = MPI_SUCCESS;
    MPI_Aint type_size;
    MPID_Win *win_ptr;
    int target_rank = get_resp_pkt->target_rank;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3_PKTHANDLER_GETRESP);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3_PKTHANDLER_GETRESP);

    MPIU_DBG_MSG(CH3_OTHER, VERBOSE, "received get response pkt");

    MPID_Win_get_ptr(get_resp_pkt->source_win_handle, win_ptr);

    /* decrement ack_counter on target */
    if (get_resp_pkt->flags & MPIDI_CH3_PKT_FLAG_RMA_LOCK_GRANTED) {
        mpi_errno = set_lock_sync_counter(win_ptr, target_rank);
        if (mpi_errno) MPIU_ERR_POP(mpi_errno);
    }
    if (get_resp_pkt->flags & MPIDI_CH3_PKT_FLAG_RMA_FLUSH_ACK) {
        mpi_errno = MPIDI_CH3I_RMA_Handle_flush_ack(win_ptr, target_rank);
        if (mpi_errno) MPIU_ERR_POP(mpi_errno);
    }
    if (get_resp_pkt->flags & MPIDI_CH3_PKT_FLAG_RMA_UNLOCK_ACK) {
        mpi_errno = MPIDI_CH3I_RMA_Handle_flush_ack(win_ptr, target_rank);
        if (mpi_errno) MPIU_ERR_POP(mpi_errno);
    }

    data_len = *buflen - sizeof(MPIDI_CH3_Pkt_t);
    data_buf = (char *) pkt + sizeof(MPIDI_CH3_Pkt_t);

    MPID_Request_get_ptr(get_resp_pkt->request_handle, req);

    MPID_Datatype_get_size_macro(req->dev.datatype, type_size);
    req->dev.recv_data_sz = type_size * req->dev.user_count;

    *rreqp = req;
    mpi_errno = MPIDI_CH3U_Receive_data_found(req, data_buf, &data_len, &complete);
    MPIU_ERR_CHKANDJUMP1(mpi_errno, mpi_errno, MPI_ERR_OTHER, "**ch3|postrecv", "**ch3|postrecv %s",
                         "MPIDI_CH3_PKT_GET_RESP");
    if (complete) {
        MPIDI_CH3U_Request_complete(req);
        *rreqp = NULL;
    }
    /* return the number of bytes processed in this function */
    *buflen = data_len + sizeof(MPIDI_CH3_Pkt_t);

  fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3_PKTHANDLER_GETRESP);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3_PktHandler_LockGranted
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3_PktHandler_LockGranted(MPIDI_VC_t * vc, MPIDI_CH3_Pkt_t * pkt,
                                     MPIDI_msg_sz_t * buflen, MPID_Request ** rreqp)
{
    MPIDI_CH3_Pkt_lock_granted_t *lock_granted_pkt = &pkt->lock_granted;
    MPID_Win *win_ptr = NULL;
    int target_rank = lock_granted_pkt->target_rank;
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3_PKTHANDLER_LOCKGRANTED);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3_PKTHANDLER_LOCKGRANTED);

    MPIU_DBG_MSG(CH3_OTHER, VERBOSE, "received lock granted pkt");

    *buflen = sizeof(MPIDI_CH3_Pkt_t);

    MPID_Win_get_ptr(lock_granted_pkt->source_win_handle, win_ptr);
    /* set the remote_lock_state flag in the window */
    win_ptr->targets[lock_granted_pkt->target_rank].remote_lock_state = MPIDI_CH3_WIN_LOCK_GRANTED;

    mpi_errno = set_lock_sync_counter(win_ptr, target_rank);
    if (mpi_errno) MPIU_ERR_POP(mpi_errno);

    *rreqp = NULL;
    MPIDI_CH3_Progress_signal_completion();

    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3_PKTHANDLER_LOCKGRANTED);
 fn_exit:
    return MPI_SUCCESS;
 fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3_PktHandler_FlushAck
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3_PktHandler_FlushAck(MPIDI_VC_t * vc, MPIDI_CH3_Pkt_t * pkt,
                                   MPIDI_msg_sz_t * buflen, MPID_Request ** rreqp)
{
    MPIDI_CH3_Pkt_flush_ack_t *flush_ack_pkt = &pkt->flush_ack;
    MPID_Win *win_ptr = NULL;
    int mpi_errno = MPI_SUCCESS;
    int target_rank = flush_ack_pkt->target_rank;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3_PKTHANDLER_FLUSHACK);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3_PKTHANDLER_FLUSHACK);

    MPIU_DBG_MSG(CH3_OTHER, VERBOSE, "received shared lock ops done pkt");

    *buflen = sizeof(MPIDI_CH3_Pkt_t);

    MPID_Win_get_ptr(flush_ack_pkt->source_win_handle, win_ptr);

    if (flush_ack_pkt->flags & MPIDI_CH3_PKT_FLAG_RMA_LOCK_GRANTED) {
        mpi_errno = set_lock_sync_counter(win_ptr, target_rank);
        if (mpi_errno) MPIU_ERR_POP(mpi_errno);
    }

    /* decrement ack_counter on target */
    mpi_errno = MPIDI_CH3I_RMA_Handle_flush_ack(win_ptr, target_rank);
    if (mpi_errno) MPIU_ERR_POP(mpi_errno);

    MPIU_Assert(win_ptr->targets[flush_ack_pkt->target_rank].remote_lock_state !=
                MPIDI_CH3_WIN_LOCK_NONE);

    if (win_ptr->targets[flush_ack_pkt->target_rank].remote_lock_state ==
        MPIDI_CH3_WIN_LOCK_FLUSH)
        win_ptr->targets[flush_ack_pkt->target_rank].remote_lock_state =
            MPIDI_CH3_WIN_LOCK_GRANTED;
    else
        win_ptr->targets[flush_ack_pkt->target_rank].remote_lock_state = MPIDI_CH3_WIN_LOCK_NONE;

    *rreqp = NULL;
    MPIDI_CH3_Progress_signal_completion();

    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3_PKTHANDLER_FLUSHACK);
 fn_exit:
    return MPI_SUCCESS;
 fn_fail:
    goto fn_exit;
}


#undef FUNCNAME
#define FUNCNAME MPIDI_CH3_PktHandler_DecrAtCnt
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3_PktHandler_DecrAtCnt(MPIDI_VC_t * vc ATTRIBUTE((unused)),
                                   MPIDI_CH3_Pkt_t * pkt,
                                   MPIDI_msg_sz_t * buflen, MPID_Request ** rreqp)
{
    MPIDI_CH3_Pkt_decr_at_counter_t *decr_at_cnt_pkt = &pkt->decr_at_cnt;
    MPID_Win *win_ptr;
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3_PKTHANDLER_DECRATCNT);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3_PKTHANDLER_DECRATCNT);

    MPID_Win_get_ptr(decr_at_cnt_pkt->target_win_handle, win_ptr);

    win_ptr->at_completion_counter--;
    MPIU_Assert(win_ptr->at_completion_counter >= 0);

    *buflen = sizeof(MPIDI_CH3_Pkt_t);
    *rreqp = NULL;
    MPIDI_CH3_Progress_signal_completion();

 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3_PKTHANDLER_DECRATCNT);
    return mpi_errno;
   fn_fail:
    goto fn_exit;
}


#undef FUNCNAME
#define FUNCNAME MPIDI_CH3_PktHandler_Unlock
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3_PktHandler_Unlock(MPIDI_VC_t * vc ATTRIBUTE((unused)),
                                MPIDI_CH3_Pkt_t * pkt,
                                MPIDI_msg_sz_t * buflen, MPID_Request ** rreqp)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_CH3_Pkt_unlock_t *unlock_pkt = &pkt->unlock;
    MPID_Win *win_ptr = NULL;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3_PKTHANDLER_UNLOCK);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3_PKTHANDLER_UNLOCK);
    MPIU_DBG_MSG(CH3_OTHER, VERBOSE, "received unlock pkt");

    *buflen = sizeof(MPIDI_CH3_Pkt_t);
    *rreqp = NULL;

    MPID_Win_get_ptr(unlock_pkt->target_win_handle, win_ptr);
    mpi_errno = MPIDI_CH3I_Release_lock(win_ptr);
    MPIU_ERR_CHKANDJUMP(mpi_errno != MPI_SUCCESS, mpi_errno, MPI_ERR_OTHER, "**ch3|rma_msg");

    mpi_errno = MPIDI_CH3I_Send_flush_ack_pkt(vc, win_ptr, MPIDI_CH3_PKT_FLAG_NONE,
                                              unlock_pkt->source_win_handle);
    if (mpi_errno) MPIU_ERR_POP(mpi_errno);

    MPIDI_CH3_Progress_signal_completion();

  fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3_PKTHANDLER_UNLOCK);
    return mpi_errno;
    /* --BEGIN ERROR HANDLING-- */
  fn_fail:
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}


#undef FUNCNAME
#define FUNCNAME MPIDI_CH3_PktHandler_Flush
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3_PktHandler_Flush(MPIDI_VC_t * vc, MPIDI_CH3_Pkt_t * pkt,
                               MPIDI_msg_sz_t * buflen, MPID_Request ** rreqp)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_CH3_Pkt_flush_t *flush_pkt = &pkt->flush;
    MPID_Win *win_ptr = NULL;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3_PKTHANDLER_FLUSH);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3_PKTHANDLER_FLUSH);
    MPIU_DBG_MSG(CH3_OTHER, VERBOSE, "received flush pkt");

    *buflen = sizeof(MPIDI_CH3_Pkt_t);
    *rreqp = NULL;

    MPID_Win_get_ptr(flush_pkt->target_win_handle, win_ptr);

    mpi_errno = MPIDI_CH3I_Send_flush_ack_pkt(vc, win_ptr, MPIDI_CH3_PKT_FLAG_NONE,
                                              flush_pkt->source_win_handle);
    if (mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);

    /* This is a flush request packet */
    if (flush_pkt->target_win_handle != MPI_WIN_NULL) {
        MPID_Request *req = NULL;

        MPID_Win_get_ptr(flush_pkt->target_win_handle, win_ptr);

        flush_pkt->target_win_handle = MPI_WIN_NULL;
        flush_pkt->target_rank = win_ptr->comm_ptr->rank;

        MPIU_THREAD_CS_ENTER(CH3COMM, vc);
        mpi_errno = MPIDI_CH3_iStartMsg(vc, flush_pkt, sizeof(*flush_pkt), &req);
        MPIU_THREAD_CS_EXIT(CH3COMM, vc);
        MPIU_ERR_CHKANDJUMP(mpi_errno != MPI_SUCCESS, mpi_errno, MPI_ERR_OTHER, "**ch3|rma_msg");

        /* Release the request returned by iStartMsg */
        if (req != NULL) {
            MPID_Request_release(req);
        }
    }

    /* This is a flush response packet */
    else {
        MPID_Win_get_ptr(flush_pkt->source_win_handle, win_ptr);
        win_ptr->targets[flush_pkt->target_rank].remote_lock_state = MPIDI_CH3_WIN_LOCK_GRANTED;
        MPIDI_CH3_Progress_signal_completion();
    }

  fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3_PKTHANDLER_FLUSH);
    return mpi_errno;
    /* --BEGIN ERROR HANDLING-- */
  fn_fail:
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}


#undef FUNCNAME
#define FUNCNAME MPIDI_CH3_Start_rma_op_target
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3_Start_rma_op_target(MPID_Win * win_ptr, MPIDI_CH3_Pkt_flags_t flags)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3_START_RMA_OP_TARGET);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3_START_RMA_OP_TARGET);

    /* Lock with NOCHECK is piggybacked on this message.  We should be able to
     * immediately grab the lock.  Otherwise, there is a synchronization error. */
    if (flags & MPIDI_CH3_PKT_FLAG_RMA_LOCK && flags & MPIDI_CH3_PKT_FLAG_RMA_NOCHECK) {
        int lock_acquired;
        int lock_mode;

        if (flags & MPIDI_CH3_PKT_FLAG_RMA_SHARED) {
            lock_mode = MPI_LOCK_SHARED;
        }
        else if (flags & MPIDI_CH3_PKT_FLAG_RMA_EXCLUSIVE) {
            lock_mode = MPI_LOCK_EXCLUSIVE;
        }
        else {
            MPIU_ERR_SETANDJUMP(mpi_errno, MPI_ERR_RMA_SYNC, "**ch3|rma_flags");
        }

        lock_acquired = MPIDI_CH3I_Try_acquire_win_lock(win_ptr, lock_mode);
        MPIU_ERR_CHKANDJUMP(!lock_acquired, mpi_errno, MPI_ERR_RMA_SYNC, "**ch3|nocheck_invalid");
    }

  fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3_START_RMA_OP_TARGET);
    return mpi_errno;
    /* --BEGIN ERROR HANDLING-- */
  fn_fail:
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}


#undef FUNCNAME
#define FUNCNAME MPIDI_CH3_Finish_rma_op_target
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3_Finish_rma_op_target(MPIDI_VC_t * vc, MPID_Win * win_ptr, int is_rma_update,
                                   MPIDI_CH3_Pkt_flags_t flags, MPI_Win source_win_handle)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3_FINISH_RMA_OP_TARGET);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3_FINISH_RMA_OP_TARGET);

    /* This function should be called by the target process after each RMA
     * operation is completed, to update synchronization state. */

    /* Last RMA operation from source. If active target RMA, decrement window
     * counter. */
    if (flags & MPIDI_CH3_PKT_FLAG_RMA_AT_COMPLETE) {
        MPIU_Assert(win_ptr->current_lock_type == MPID_LOCK_NONE);

        win_ptr->at_completion_counter -= 1;
        MPIU_Assert(win_ptr->at_completion_counter >= 0);

        /* Signal the local process when the op counter reaches 0. */
        if (win_ptr->at_completion_counter == 0)
            MPIDI_CH3_Progress_signal_completion();
    }

    /* If passive target RMA, release lock on window and grant next lock in the
     * lock queue if there is any.  If requested by the origin, send an ack back
     * to indicate completion at the target. */
    else if (flags & MPIDI_CH3_PKT_FLAG_RMA_UNLOCK) {
        MPIU_Assert(win_ptr->current_lock_type != MPID_LOCK_NONE);

        if (flags & MPIDI_CH3_PKT_FLAG_RMA_REQ_ACK) {
            MPIU_Assert(source_win_handle != MPI_WIN_NULL && vc != NULL);
            mpi_errno = MPIDI_CH3I_Send_flush_ack_pkt(vc, win_ptr, source_win_handle);
            if (mpi_errno) {
                MPIU_ERR_POP(mpi_errno);
            }
        }

        mpi_errno = MPIDI_CH3I_Release_lock(win_ptr);
        if (mpi_errno) {
            MPIU_ERR_POP(mpi_errno);
        }

        /* The local process may be waiting for the lock.  Signal completion to
         * wake it up, so it can attempt to grab the lock. */
        MPIDI_CH3_Progress_signal_completion();
    }
    else if (flags & MPIDI_CH3_PKT_FLAG_RMA_FLUSH) {
        /* Ensure store instructions have been performed before flush call is
         * finished on origin process. */
        OPA_read_write_barrier();

        if (flags & MPIDI_CH3_PKT_FLAG_RMA_REQ_ACK) {
            MPIDI_CH3_Pkt_t upkt;
            MPIDI_CH3_Pkt_flush_t *flush_pkt = &upkt.flush;
            MPID_Request *req = NULL;

            MPIU_DBG_MSG(CH3_OTHER, VERBOSE, "received piggybacked flush request");
            MPIU_Assert(source_win_handle != MPI_WIN_NULL && vc != NULL);

            MPIDI_Pkt_init(flush_pkt, MPIDI_CH3_PKT_FLUSH);
            flush_pkt->source_win_handle = source_win_handle;
            flush_pkt->target_win_handle = MPI_WIN_NULL;
            flush_pkt->target_rank = win_ptr->comm_ptr->rank;

            MPIU_THREAD_CS_ENTER(CH3COMM, vc);
            mpi_errno = MPIDI_CH3_iStartMsg(vc, flush_pkt, sizeof(*flush_pkt), &req);
            MPIU_THREAD_CS_EXIT(CH3COMM, vc);
            MPIU_ERR_CHKANDJUMP(mpi_errno != MPI_SUCCESS, mpi_errno, MPI_ERR_OTHER,
                                "**ch3|rma_msg");

            /* Release the request returned by iStartMsg */
            if (req != NULL) {
                MPID_Request_release(req);
            }
        }
    }

  fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3_FINISH_RMA_OP_TARGET);
    return mpi_errno;
    /* --BEGIN ERROR HANDLING-- */
  fn_fail:
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}


/* ------------------------------------------------------------------------ */
/*
 * For debugging, we provide the following functions for printing the
 * contents of an RMA packet
 */
/* ------------------------------------------------------------------------ */
#ifdef MPICH_DBG_OUTPUT
int MPIDI_CH3_PktPrint_Put(FILE * fp, MPIDI_CH3_Pkt_t * pkt)
{
    MPIU_DBG_PRINTF((" type ......... MPIDI_CH3_PKT_PUT\n"));
    MPIU_DBG_PRINTF((" addr ......... %p\n", pkt->put.addr));
    MPIU_DBG_PRINTF((" count ........ %d\n", pkt->put.count));
    MPIU_DBG_PRINTF((" datatype ..... 0x%08X\n", pkt->put.datatype));
    MPIU_DBG_PRINTF((" dataloop_size. 0x%08X\n", pkt->put.dataloop_size));
    MPIU_DBG_PRINTF((" target ....... 0x%08X\n", pkt->put.target_win_handle));
    MPIU_DBG_PRINTF((" source ....... 0x%08X\n", pkt->put.source_win_handle));
    /*MPIU_DBG_PRINTF((" win_ptr ...... 0x%08X\n", pkt->put.win_ptr)); */
    return MPI_SUCCESS;
}

int MPIDI_CH3_PktPrint_Get(FILE * fp, MPIDI_CH3_Pkt_t * pkt)
{
    MPIU_DBG_PRINTF((" type ......... MPIDI_CH3_PKT_GET\n"));
    MPIU_DBG_PRINTF((" addr ......... %p\n", pkt->get.addr));
    MPIU_DBG_PRINTF((" count ........ %d\n", pkt->get.count));
    MPIU_DBG_PRINTF((" datatype ..... 0x%08X\n", pkt->get.datatype));
    MPIU_DBG_PRINTF((" dataloop_size. %d\n", pkt->get.dataloop_size));
    MPIU_DBG_PRINTF((" request ...... 0x%08X\n", pkt->get.request_handle));
    MPIU_DBG_PRINTF((" target ....... 0x%08X\n", pkt->get.target_win_handle));
    MPIU_DBG_PRINTF((" source ....... 0x%08X\n", pkt->get.source_win_handle));
    /*
     * MPIU_DBG_PRINTF((" request ...... 0x%08X\n", pkt->get.request));
     * MPIU_DBG_PRINTF((" win_ptr ...... 0x%08X\n", pkt->get.win_ptr));
     */
    return MPI_SUCCESS;
}

int MPIDI_CH3_PktPrint_GetResp(FILE * fp, MPIDI_CH3_Pkt_t * pkt)
{
    MPIU_DBG_PRINTF((" type ......... MPIDI_CH3_PKT_GET_RESP\n"));
    MPIU_DBG_PRINTF((" request ...... 0x%08X\n", pkt->get_resp.request_handle));
    /*MPIU_DBG_PRINTF((" request ...... 0x%08X\n", pkt->get_resp.request)); */
    return MPI_SUCCESS;
}

int MPIDI_CH3_PktPrint_Accumulate(FILE * fp, MPIDI_CH3_Pkt_t * pkt)
{
    MPIU_DBG_PRINTF((" type ......... MPIDI_CH3_PKT_ACCUMULATE\n"));
    MPIU_DBG_PRINTF((" addr ......... %p\n", pkt->accum.addr));
    MPIU_DBG_PRINTF((" count ........ %d\n", pkt->accum.count));
    MPIU_DBG_PRINTF((" datatype ..... 0x%08X\n", pkt->accum.datatype));
    MPIU_DBG_PRINTF((" dataloop_size. %d\n", pkt->accum.dataloop_size));
    MPIU_DBG_PRINTF((" op ........... 0x%08X\n", pkt->accum.op));
    MPIU_DBG_PRINTF((" target ....... 0x%08X\n", pkt->accum.target_win_handle));
    MPIU_DBG_PRINTF((" source ....... 0x%08X\n", pkt->accum.source_win_handle));
    /*MPIU_DBG_PRINTF((" win_ptr ...... 0x%08X\n", pkt->accum.win_ptr)); */
    return MPI_SUCCESS;
}

int MPIDI_CH3_PktPrint_Accum_Immed(FILE * fp, MPIDI_CH3_Pkt_t * pkt)
{
    MPIU_DBG_PRINTF((" type ......... MPIDI_CH3_PKT_ACCUM_IMMED\n"));
    MPIU_DBG_PRINTF((" addr ......... %p\n", pkt->accum_immed.addr));
    MPIU_DBG_PRINTF((" count ........ %d\n", pkt->accum_immed.count));
    MPIU_DBG_PRINTF((" datatype ..... 0x%08X\n", pkt->accum_immed.datatype));
    MPIU_DBG_PRINTF((" op ........... 0x%08X\n", pkt->accum_immed.op));
    MPIU_DBG_PRINTF((" target ....... 0x%08X\n", pkt->accum_immed.target_win_handle));
    MPIU_DBG_PRINTF((" source ....... 0x%08X\n", pkt->accum_immed.source_win_handle));
    /*MPIU_DBG_PRINTF((" win_ptr ...... 0x%08X\n", pkt->accum.win_ptr)); */
    fflush(stdout);
    return MPI_SUCCESS;
}

int MPIDI_CH3_PktPrint_Lock(FILE * fp, MPIDI_CH3_Pkt_t * pkt)
{
    MPIU_DBG_PRINTF((" type ......... MPIDI_CH3_PKT_LOCK\n"));
    MPIU_DBG_PRINTF((" lock_type .... %d\n", pkt->lock.lock_type));
    MPIU_DBG_PRINTF((" target ....... 0x%08X\n", pkt->lock.target_win_handle));
    MPIU_DBG_PRINTF((" source ....... 0x%08X\n", pkt->lock.source_win_handle));
    return MPI_SUCCESS;
}

int MPIDI_CH3_PktPrint_LockPutUnlock(FILE * fp, MPIDI_CH3_Pkt_t * pkt)
{
    MPIU_DBG_PRINTF((" type ......... MPIDI_CH3_PKT_LOCK_PUT_UNLOCK\n"));
    MPIU_DBG_PRINTF((" addr ......... %p\n", pkt->lock_put_unlock.addr));
    MPIU_DBG_PRINTF((" count ........ %d\n", pkt->lock_put_unlock.count));
    MPIU_DBG_PRINTF((" datatype ..... 0x%08X\n", pkt->lock_put_unlock.datatype));
    MPIU_DBG_PRINTF((" lock_type .... %d\n", pkt->lock_put_unlock.lock_type));
    MPIU_DBG_PRINTF((" target ....... 0x%08X\n", pkt->lock_put_unlock.target_win_handle));
    MPIU_DBG_PRINTF((" source ....... 0x%08X\n", pkt->lock_put_unlock.source_win_handle));
    return MPI_SUCCESS;
}

int MPIDI_CH3_PktPrint_LockAccumUnlock(FILE * fp, MPIDI_CH3_Pkt_t * pkt)
{
    MPIU_DBG_PRINTF((" type ......... MPIDI_CH3_PKT_LOCK_ACCUM_UNLOCK\n"));
    MPIU_DBG_PRINTF((" addr ......... %p\n", pkt->lock_accum_unlock.addr));
    MPIU_DBG_PRINTF((" count ........ %d\n", pkt->lock_accum_unlock.count));
    MPIU_DBG_PRINTF((" datatype ..... 0x%08X\n", pkt->lock_accum_unlock.datatype));
    MPIU_DBG_PRINTF((" lock_type .... %d\n", pkt->lock_accum_unlock.lock_type));
    MPIU_DBG_PRINTF((" target ....... 0x%08X\n", pkt->lock_accum_unlock.target_win_handle));
    MPIU_DBG_PRINTF((" source ....... 0x%08X\n", pkt->lock_accum_unlock.source_win_handle));
    return MPI_SUCCESS;
}

int MPIDI_CH3_PktPrint_LockGetUnlock(FILE * fp, MPIDI_CH3_Pkt_t * pkt)
{
    MPIU_DBG_PRINTF((" type ......... MPIDI_CH3_PKT_LOCK_GET_UNLOCK\n"));
    MPIU_DBG_PRINTF((" addr ......... %p\n", pkt->lock_get_unlock.addr));
    MPIU_DBG_PRINTF((" count ........ %d\n", pkt->lock_get_unlock.count));
    MPIU_DBG_PRINTF((" datatype ..... 0x%08X\n", pkt->lock_get_unlock.datatype));
    MPIU_DBG_PRINTF((" lock_type .... %d\n", pkt->lock_get_unlock.lock_type));
    MPIU_DBG_PRINTF((" target ....... 0x%08X\n", pkt->lock_get_unlock.target_win_handle));
    MPIU_DBG_PRINTF((" source ....... 0x%08X\n", pkt->lock_get_unlock.source_win_handle));
    MPIU_DBG_PRINTF((" request ...... 0x%08X\n", pkt->lock_get_unlock.request_handle));
    return MPI_SUCCESS;
}

int MPIDI_CH3_PktPrint_FlushAck(FILE * fp, MPIDI_CH3_Pkt_t * pkt)
{
    MPIU_DBG_PRINTF((" type ......... MPIDI_CH3_PKT_FLUSH_ACK\n"));
    MPIU_DBG_PRINTF((" source ....... 0x%08X\n", pkt->lock_accum_unlock.source_win_handle));
    return MPI_SUCCESS;
}

int MPIDI_CH3_PktPrint_LockGranted(FILE * fp, MPIDI_CH3_Pkt_t * pkt)
{
    MPIU_DBG_PRINTF((" type ......... MPIDI_CH3_PKT_LOCK_GRANTED\n"));
    MPIU_DBG_PRINTF((" source ....... 0x%08X\n", pkt->lock_granted.source_win_handle));
    return MPI_SUCCESS;
}
#endif
