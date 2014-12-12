/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"
#include "mpidrma.h"

MPIR_T_PVAR_DOUBLE_TIMER_DECL(RMA, rma_rmapkt_put);
MPIR_T_PVAR_DOUBLE_TIMER_DECL(RMA, rma_rmapkt_get);
MPIR_T_PVAR_DOUBLE_TIMER_DECL(RMA, rma_rmapkt_acc);
MPIR_T_PVAR_DOUBLE_TIMER_DECL(RMA, rma_rmapkt_get_accum);
MPIR_T_PVAR_DOUBLE_TIMER_DECL(RMA, rma_rmapkt_cas);
MPIR_T_PVAR_DOUBLE_TIMER_DECL(RMA, rma_rmapkt_fop);
MPIR_T_PVAR_DOUBLE_TIMER_DECL(RMA, rma_rmapkt_get_resp);
MPIR_T_PVAR_DOUBLE_TIMER_DECL(RMA, rma_rmapkt_get_accum_resp);
MPIR_T_PVAR_DOUBLE_TIMER_DECL(RMA, rma_rmapkt_cas_resp);
MPIR_T_PVAR_DOUBLE_TIMER_DECL(RMA, rma_rmapkt_fop_resp);
MPIR_T_PVAR_DOUBLE_TIMER_DECL(RMA, rma_rmapkt_lock);
MPIR_T_PVAR_DOUBLE_TIMER_DECL(RMA, rma_rmapkt_lock_ack);
MPIR_T_PVAR_DOUBLE_TIMER_DECL(RMA, rma_rmapkt_unlock);
MPIR_T_PVAR_DOUBLE_TIMER_DECL(RMA, rma_rmapkt_flush);
MPIR_T_PVAR_DOUBLE_TIMER_DECL(RMA, rma_rmapkt_flush_ack);
MPIR_T_PVAR_DOUBLE_TIMER_DECL(RMA, rma_rmapkt_decr_at_cnt);

void MPIDI_CH3_RMA_Init_pkthandler_pvars(void)
{
    /* rma_rmapkt_put */
    MPIR_T_PVAR_TIMER_REGISTER_STATIC(RMA,
                                      MPI_DOUBLE,
                                      rma_rmapkt_put,
                                      MPI_T_VERBOSITY_MPIDEV_DETAIL,
                                      MPI_T_BIND_NO_OBJECT,
                                      MPIR_T_PVAR_FLAG_READONLY,
                                      "RMA", "RMA:PKTHANDLER for Put (in seconds)");

    /* rma_rmapkt_get */
    MPIR_T_PVAR_TIMER_REGISTER_STATIC(RMA,
                                      MPI_DOUBLE,
                                      rma_rmapkt_get,
                                      MPI_T_VERBOSITY_MPIDEV_DETAIL,
                                      MPI_T_BIND_NO_OBJECT,
                                      MPIR_T_PVAR_FLAG_READONLY,
                                      "RMA", "RMA:PKTHANDLER for Get (in seconds)");

    /* rma_rmapkt_acc */
    MPIR_T_PVAR_TIMER_REGISTER_STATIC(RMA,
                                      MPI_DOUBLE,
                                      rma_rmapkt_acc,
                                      MPI_T_VERBOSITY_MPIDEV_DETAIL,
                                      MPI_T_BIND_NO_OBJECT,
                                      MPIR_T_PVAR_FLAG_READONLY,
                                      "RMA", "RMA:PKTHANDLER for Accumulate (in seconds)");

    /* rma_rmapkt_get_accum */
    MPIR_T_PVAR_TIMER_REGISTER_STATIC(RMA,
                                      MPI_DOUBLE,
                                      rma_rmapkt_get_accum,
                                      MPI_T_VERBOSITY_MPIDEV_DETAIL,
                                      MPI_T_BIND_NO_OBJECT,
                                      MPIR_T_PVAR_FLAG_READONLY,
                                      "RMA", "RMA:PKTHANDLER for Get-Accumulate (in seconds)");

    /* rma_rmapkt_cas */
    MPIR_T_PVAR_TIMER_REGISTER_STATIC(RMA,
                                      MPI_DOUBLE,
                                      rma_rmapkt_cas,
                                      MPI_T_VERBOSITY_MPIDEV_DETAIL,
                                      MPI_T_BIND_NO_OBJECT,
                                      MPIR_T_PVAR_FLAG_READONLY,
                                      "RMA", "RMA:PKTHANDLER for Compare-and-swap (in seconds)");

    /* rma_rmapkt_fop */
    MPIR_T_PVAR_TIMER_REGISTER_STATIC(RMA,
                                      MPI_DOUBLE,
                                      rma_rmapkt_fop,
                                      MPI_T_VERBOSITY_MPIDEV_DETAIL,
                                      MPI_T_BIND_NO_OBJECT,
                                      MPIR_T_PVAR_FLAG_READONLY,
                                      "RMA", "RMA:PKTHANDLER for Fetch-and-op (in seconds)");

    /* rma_rmapkt_get_resp */
    MPIR_T_PVAR_TIMER_REGISTER_STATIC(RMA,
                                      MPI_DOUBLE,
                                      rma_rmapkt_get_resp,
                                      MPI_T_VERBOSITY_MPIDEV_DETAIL,
                                      MPI_T_BIND_NO_OBJECT,
                                      MPIR_T_PVAR_FLAG_READONLY,
                                      "RMA", "RMA:PKTHANDLER for Get response (in seconds)");

    /* rma_rmapkt_get_accum_resp */
    MPIR_T_PVAR_TIMER_REGISTER_STATIC(RMA,
                                      MPI_DOUBLE,
                                      rma_rmapkt_get_accum_resp,
                                      MPI_T_VERBOSITY_MPIDEV_DETAIL,
                                      MPI_T_BIND_NO_OBJECT,
                                      MPIR_T_PVAR_FLAG_READONLY,
                                      "RMA", "RMA:PKTHANDLER for Get-Accumulate response (in seconds)");

    /* rma_rmapkt_cas_resp */
    MPIR_T_PVAR_TIMER_REGISTER_STATIC(RMA,
                                      MPI_DOUBLE,
                                      rma_rmapkt_cas_resp,
                                      MPI_T_VERBOSITY_MPIDEV_DETAIL,
                                      MPI_T_BIND_NO_OBJECT,
                                      MPIR_T_PVAR_FLAG_READONLY,
                                      "RMA", "RMA:PKTHANDLER for Compare-and-Swap response (in seconds)");

    /* rma_rmapkt_fop_resp */
    MPIR_T_PVAR_TIMER_REGISTER_STATIC(RMA,
                                      MPI_DOUBLE,
                                      rma_rmapkt_fop_resp,
                                      MPI_T_VERBOSITY_MPIDEV_DETAIL,
                                      MPI_T_BIND_NO_OBJECT,
                                      MPIR_T_PVAR_FLAG_READONLY,
                                      "RMA", "RMA:PKTHANDLER for Fetch-and-op response (in seconds)");

    /* rma_rmapkt_lock */
    MPIR_T_PVAR_TIMER_REGISTER_STATIC(RMA,
                                      MPI_DOUBLE,
                                      rma_rmapkt_lock,
                                      MPI_T_VERBOSITY_MPIDEV_DETAIL,
                                      MPI_T_BIND_NO_OBJECT,
                                      MPIR_T_PVAR_FLAG_READONLY,
                                      "RMA", "RMA:PKTHANDLER for Lock (in seconds)");

    /* rma_rmapkt_lock_granted */
    MPIR_T_PVAR_TIMER_REGISTER_STATIC(RMA,
                                      MPI_DOUBLE,
                                      rma_rmapkt_lock_ack,
                                      MPI_T_VERBOSITY_MPIDEV_DETAIL,
                                      MPI_T_BIND_NO_OBJECT,
                                      MPIR_T_PVAR_FLAG_READONLY,
                                      "RMA", "RMA:PKTHANDLER for Lock-Ack (in seconds)");

    /* rma_rmapkt_unlock */
    MPIR_T_PVAR_TIMER_REGISTER_STATIC(RMA,
                                      MPI_DOUBLE,
                                      rma_rmapkt_unlock,
                                      MPI_T_VERBOSITY_MPIDEV_DETAIL,
                                      MPI_T_BIND_NO_OBJECT,
                                      MPIR_T_PVAR_FLAG_READONLY,
                                      "RMA", "RMA:PKTHANDLER for Unlock (in seconds)");

    /* rma_rmapkt_flush */
    MPIR_T_PVAR_TIMER_REGISTER_STATIC(RMA,
                                      MPI_DOUBLE,
                                      rma_rmapkt_flush,
                                      MPI_T_VERBOSITY_MPIDEV_DETAIL,
                                      MPI_T_BIND_NO_OBJECT,
                                      MPIR_T_PVAR_FLAG_READONLY,
                                      "RMA", "RMA:PKTHANDLER for Flush (in seconds)");

    /* rma_rmapkt_flush_ack */
    MPIR_T_PVAR_TIMER_REGISTER_STATIC(RMA,
                                      MPI_DOUBLE,
                                      rma_rmapkt_flush_ack,
                                      MPI_T_VERBOSITY_MPIDEV_DETAIL,
                                      MPI_T_BIND_NO_OBJECT,
                                      MPIR_T_PVAR_FLAG_READONLY,
                                      "RMA", "RMA:PKTHANDLER for Flush-Ack (in seconds)");

    /* rma_rmapkt_decr_at_cnt */
    MPIR_T_PVAR_TIMER_REGISTER_STATIC(RMA,
                                      MPI_DOUBLE,
                                      rma_rmapkt_decr_at_cnt,
                                      MPI_T_VERBOSITY_MPIDEV_DETAIL,
                                      MPI_T_BIND_NO_OBJECT,
                                      MPIR_T_PVAR_FLAG_READONLY,
                                      "RMA", "RMA:PKTHANDLER for Decr-At-Cnt (in seconds)");
}

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

    MPIR_T_PVAR_TIMER_START(RMA, rma_rmapkt_put);

    MPIU_Assert(put_pkt->target_win_handle != MPI_WIN_NULL);
    MPID_Win_get_ptr(put_pkt->target_win_handle, win_ptr);

    mpi_errno = check_piggyback_lock(win_ptr, vc, pkt, buflen,
                                     &acquire_lock_fail, &req);
    if (mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);

    if (acquire_lock_fail) {
        (*rreqp) = req;
        goto fn_exit;
    }

    /* get start location of data and length of data */
    data_len = *buflen - sizeof(MPIDI_CH3_Pkt_t);
    data_buf = (char *) pkt + sizeof(MPIDI_CH3_Pkt_t);

    req = MPID_Request_create();
    MPIU_Object_set_ref(req, 1);

    req->dev.user_buf = put_pkt->addr;
    req->dev.user_count = put_pkt->count;
    req->dev.target_win_handle = put_pkt->target_win_handle;
    req->dev.source_win_handle = put_pkt->source_win_handle;
    req->dev.flags = put_pkt->flags;
    req->dev.OnFinal = MPIDI_CH3_ReqHandler_PutRecvComplete;

    if (MPIR_DATATYPE_IS_PREDEFINED(put_pkt->datatype)) {
        MPIDI_Request_set_type(req, MPIDI_REQUEST_TYPE_PUT_RESP);
        req->dev.datatype = put_pkt->datatype;

        MPID_Datatype_get_size_macro(put_pkt->datatype, type_size);
        req->dev.recv_data_sz = type_size * put_pkt->count;

        if (put_pkt->immed_len > 0) {
            /* See if we can receive some data from packet header. */
            MPIU_Memcpy(req->dev.user_buf, put_pkt->data, (size_t)put_pkt->immed_len);
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
    MPIR_T_PVAR_TIMER_END(RMA, rma_rmapkt_put);
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

    MPIR_T_PVAR_TIMER_START(RMA, rma_rmapkt_get);

    MPIU_Assert(get_pkt->target_win_handle != MPI_WIN_NULL);
    MPID_Win_get_ptr(get_pkt->target_win_handle, win_ptr);

    mpi_errno = check_piggyback_lock(win_ptr, vc, pkt,
                                     buflen, &acquire_lock_fail, &req);
    if (mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);

    if (acquire_lock_fail) {
        (*rreqp) = req;
        goto fn_exit;
    }

    req = MPID_Request_create();
    req->dev.target_win_handle = get_pkt->target_win_handle;
    req->dev.source_win_handle = get_pkt->source_win_handle;
    req->dev.flags = get_pkt->flags;

    /* get start location of data and length of data */
    data_len = *buflen - sizeof(MPIDI_CH3_Pkt_t);
    data_buf = (char *) pkt + sizeof(MPIDI_CH3_Pkt_t);

    /* here we increment the Active Target counter to guarantee the GET-like
       operation are completed when counter reaches zero. */
    win_ptr->at_completion_counter++;

    if (MPIR_DATATYPE_IS_PREDEFINED(get_pkt->datatype)) {
        /* basic datatype. send the data. */
        MPIDI_CH3_Pkt_t upkt;
        MPIDI_CH3_Pkt_get_resp_t *get_resp_pkt = &upkt.get_resp;
        size_t len;
        int iovcnt;

        MPIDI_Request_set_type(req, MPIDI_REQUEST_TYPE_GET_RESP);
        req->dev.OnDataAvail = MPIDI_CH3_ReqHandler_GetSendComplete;
        req->dev.OnFinal = MPIDI_CH3_ReqHandler_GetSendComplete;
        req->kind = MPID_REQUEST_SEND;

        MPIDI_Pkt_init(get_resp_pkt, MPIDI_CH3_PKT_GET_RESP);
        get_resp_pkt->request_handle = get_pkt->request_handle;
        get_resp_pkt->flags = MPIDI_CH3_PKT_FLAG_NONE;
        if (get_pkt->flags & MPIDI_CH3_PKT_FLAG_RMA_LOCK)
            get_resp_pkt->flags |= MPIDI_CH3_PKT_FLAG_RMA_LOCK_GRANTED;
        if ((get_pkt->flags & MPIDI_CH3_PKT_FLAG_RMA_FLUSH) ||
            (get_pkt->flags & MPIDI_CH3_PKT_FLAG_RMA_UNLOCK))
            get_resp_pkt->flags |= MPIDI_CH3_PKT_FLAG_RMA_FLUSH_ACK;
        get_resp_pkt->target_rank = win_ptr->comm_ptr->rank;
        get_resp_pkt->source_win_handle = get_pkt->source_win_handle;
        get_resp_pkt->immed_len = 0;

        /* length of target data */
        MPID_Datatype_get_size_macro(get_pkt->datatype, type_size);
        MPIU_Assign_trunc(len, get_pkt->count * type_size, size_t);

        /* both origin buffer and target buffer are basic datatype,
           fill IMMED data area in response packet header. */
        if (get_pkt->flags & MPIDI_CH3_PKT_FLAG_RMA_IMMED_RESP) {
            /* Try to copy target data into packet header. */
            MPIU_Assign_trunc(get_resp_pkt->immed_len,
                              MPIR_MIN(len, (MPIDI_RMA_IMMED_BYTES / type_size) * type_size),
                              int);

            if (get_resp_pkt->immed_len > 0) {
                void *src = get_pkt->addr;
                void *dest = (void*) get_resp_pkt->data;
                /* copy data from origin buffer to immed area in packet header */
                mpi_errno = immed_copy(src, dest, (size_t)get_resp_pkt->immed_len);
                if (mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);
            }
        }

        if (len == (size_t)get_resp_pkt->immed_len) {
            /* All origin data is in packet header, issue the header. */
            iov[0].MPID_IOV_BUF = (MPID_IOV_BUF_CAST) get_resp_pkt;
            iov[0].MPID_IOV_LEN = sizeof(*get_resp_pkt);
            iovcnt = 1;
        }
        else {
            iov[0].MPID_IOV_BUF = (MPID_IOV_BUF_CAST) get_resp_pkt;
            iov[0].MPID_IOV_LEN = sizeof(*get_resp_pkt);
            iov[1].MPID_IOV_BUF = (MPID_IOV_BUF_CAST) ((char *)get_pkt->addr + get_resp_pkt->immed_len);
            iov[1].MPID_IOV_LEN = get_pkt->count * type_size - get_resp_pkt->immed_len;
            iovcnt = 2;
        }

        MPIU_THREAD_CS_ENTER(CH3COMM, vc);
        mpi_errno = MPIDI_CH3_iSendv(vc, req, iov, iovcnt);
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
    MPIR_T_PVAR_TIMER_END(RMA, rma_rmapkt_get);
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

    MPIR_T_PVAR_TIMER_START(RMA, rma_rmapkt_acc);

    MPIU_Assert(accum_pkt->target_win_handle != MPI_WIN_NULL);
    MPID_Win_get_ptr(accum_pkt->target_win_handle, win_ptr);

    mpi_errno = check_piggyback_lock(win_ptr, vc, pkt, buflen,
                                     &acquire_lock_fail, &req);
    if (mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);

    if (acquire_lock_fail) {
        (*rreqp) = req;
        goto fn_exit;
    }

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
    req->dev.OnFinal = MPIDI_CH3_ReqHandler_AccumRecvComplete;

    /* get start location of data and length of data */
    data_len = *buflen - sizeof(MPIDI_CH3_Pkt_t);
    data_buf = (char *) pkt + sizeof(MPIDI_CH3_Pkt_t);

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
            MPIU_Memcpy(req->dev.user_buf, accum_pkt->data, (size_t)accum_pkt->immed_len);
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
    MPIR_T_PVAR_TIMER_END(RMA, rma_rmapkt_acc);
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

    MPIR_T_PVAR_TIMER_START(RMA, rma_rmapkt_get_accum);

    MPIU_Assert(get_accum_pkt->target_win_handle != MPI_WIN_NULL);
    MPID_Win_get_ptr(get_accum_pkt->target_win_handle, win_ptr);

    mpi_errno = check_piggyback_lock(win_ptr, vc, pkt,
                                     buflen,
                                     &acquire_lock_fail, &req);
    if (mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);

    if (acquire_lock_fail) {
        (*rreqp) = req;
        goto fn_exit;
    }

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
    req->dev.OnFinal = MPIDI_CH3_ReqHandler_GaccumRecvComplete;

    /* get start location of data and length of data */
    data_len = *buflen - sizeof(MPIDI_CH3_Pkt_t);
    data_buf = (char *) pkt + sizeof(MPIDI_CH3_Pkt_t);

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
            MPIU_Memcpy(req->dev.user_buf, get_accum_pkt->data, (size_t)get_accum_pkt->immed_len);
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
    MPIR_T_PVAR_TIMER_END(RMA, rma_rmapkt_get_accum);
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3_PKTHANDLER_GETACCUMULATE);
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
    MPID_Request *rreq = NULL;
    MPI_Aint len;
    int acquire_lock_fail = 0;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3_PKTHANDLER_CAS);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3_PKTHANDLER_CAS);

    MPIU_DBG_MSG(CH3_OTHER, VERBOSE, "received CAS pkt");

    MPIR_T_PVAR_TIMER_START(RMA, rma_rmapkt_cas);

    MPIU_Assert(cas_pkt->target_win_handle != MPI_WIN_NULL);
    MPID_Win_get_ptr(cas_pkt->target_win_handle, win_ptr);

    mpi_errno = check_piggyback_lock(win_ptr, vc, pkt, buflen,
                                     &acquire_lock_fail, &rreq);
    if (mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);
    MPIU_Assert(rreq == NULL); /* CAS should not have request because all data
                                  can fit in packet header */

    if (acquire_lock_fail) {
        (*rreqp) = rreq;
        goto fn_exit;
    }

    /* return the number of bytes processed in this function */
    /* data_len == 0 (all within packet) */
    *buflen = sizeof(MPIDI_CH3_Pkt_t);
    *rreqp = NULL;

    MPIDI_Pkt_init(cas_resp_pkt, MPIDI_CH3_PKT_CAS_RESP);
    cas_resp_pkt->request_handle = cas_pkt->request_handle;
    cas_resp_pkt->source_win_handle = cas_pkt->source_win_handle;
    cas_resp_pkt->target_rank = win_ptr->comm_ptr->rank;
    cas_resp_pkt->flags = MPIDI_CH3_PKT_FLAG_NONE;
    if (cas_pkt->flags & MPIDI_CH3_PKT_FLAG_RMA_LOCK)
        cas_resp_pkt->flags |= MPIDI_CH3_PKT_FLAG_RMA_LOCK_GRANTED;
    if ((cas_pkt->flags & MPIDI_CH3_PKT_FLAG_RMA_FLUSH) ||
        (cas_pkt->flags & MPIDI_CH3_PKT_FLAG_RMA_UNLOCK))
        cas_resp_pkt->flags |= MPIDI_CH3_PKT_FLAG_RMA_FLUSH_ACK;

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
            req->dev.OnDataAvail = MPIDI_CH3_ReqHandler_CASSendComplete;

            /* here we increment the Active Target counter to guarantee the GET-like
               operation are completed when counter reaches zero. */
            win_ptr->at_completion_counter++;

            MPID_Request_release(req);
            goto fn_exit;
        }
        else
            MPID_Request_release(req);
    }

    mpi_errno = finish_op_on_target(win_ptr, vc, MPIDI_CH3_PKT_CAS,
                                    cas_pkt->flags, cas_pkt->source_win_handle);
    if (mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);

  fn_exit:
    MPIR_T_PVAR_TIMER_END(RMA, rma_rmapkt_cas);
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

    MPIR_T_PVAR_TIMER_START(RMA, rma_rmapkt_cas_resp);

    MPID_Win_get_ptr(cas_resp_pkt->source_win_handle, win_ptr);

    /* decrement ack_counter on this target */
    if (cas_resp_pkt->flags & MPIDI_CH3_PKT_FLAG_RMA_LOCK_GRANTED) {
        mpi_errno = handle_lock_ack(win_ptr, target_rank,
                                          cas_resp_pkt->flags);
        if (mpi_errno) MPIU_ERR_POP(mpi_errno);

        mpi_errno = adjust_op_piggybacked_with_lock(win_ptr, target_rank,
                                                    cas_resp_pkt->flags);
        if (mpi_errno) MPIU_ERR_POP(mpi_errno);
    }
    if (cas_resp_pkt->flags & MPIDI_CH3_PKT_FLAG_RMA_FLUSH_ACK) {
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
    MPIR_T_PVAR_TIMER_END(RMA, rma_rmapkt_cas_resp);
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
    MPID_Request *rreq = NULL;
    int acquire_lock_fail = 0;
    MPID_Win *win_ptr = NULL;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3_PKTHANDLER_FOP);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3_PKTHANDLER_FOP);

    MPIU_DBG_MSG(CH3_OTHER, VERBOSE, "received FOP pkt");

    MPIR_T_PVAR_TIMER_START(RMA, rma_rmapkt_fop);

    MPID_Win_get_ptr(fop_pkt->target_win_handle, win_ptr);

    mpi_errno = check_piggyback_lock(win_ptr, vc, pkt, buflen,
                                     &acquire_lock_fail, &rreq);
    if (mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);

    MPIU_Assert(rreq == NULL); /* FOP should not have request because all data
                                  can fit in packet header */
    if (acquire_lock_fail) {
        (*rreqp) = rreq;
        goto fn_exit;
    }

    (*buflen) = sizeof(MPIDI_CH3_Pkt_t);
    (*rreqp) = NULL;

    MPIDI_Pkt_init(fop_resp_pkt, MPIDI_CH3_PKT_FOP_RESP);
    fop_resp_pkt->request_handle = fop_pkt->request_handle;
    fop_resp_pkt->source_win_handle = fop_pkt->source_win_handle;
    fop_resp_pkt->target_rank = win_ptr->comm_ptr->rank;
    fop_resp_pkt->flags = MPIDI_CH3_PKT_FLAG_NONE;
    if (fop_pkt->flags & MPIDI_CH3_PKT_FLAG_RMA_LOCK)
        fop_resp_pkt->flags |= MPIDI_CH3_PKT_FLAG_RMA_LOCK_GRANTED;
    if ((fop_pkt->flags & MPIDI_CH3_PKT_FLAG_RMA_FLUSH) ||
        (fop_pkt->flags & MPIDI_CH3_PKT_FLAG_RMA_UNLOCK))
        fop_resp_pkt->flags |= MPIDI_CH3_PKT_FLAG_RMA_FLUSH_ACK;
    fop_resp_pkt->immed_len = fop_pkt->immed_len;

    /* copy data to resp pkt header */
    void *src = fop_pkt->addr, *dest = fop_resp_pkt->data;
    mpi_errno = immed_copy(src, dest, (size_t)fop_resp_pkt->immed_len);
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
            resp_req->dev.OnDataAvail = MPIDI_CH3_ReqHandler_FOPSendComplete;

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

    mpi_errno = finish_op_on_target(win_ptr, vc, MPIDI_CH3_PKT_FOP,
                                    fop_pkt->flags, fop_pkt->source_win_handle);
    if (mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);

  fn_exit:
    MPIR_T_PVAR_TIMER_END(RMA, rma_rmapkt_fop);
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

    MPIR_T_PVAR_TIMER_START(RMA, rma_rmapkt_fop_resp);

    MPID_Win_get_ptr(fop_resp_pkt->source_win_handle, win_ptr);

    /* Copy data to result buffer on orgin */
    MPID_Request_get_ptr(fop_resp_pkt->request_handle, req);
    MPIU_Memcpy(req->dev.user_buf, fop_resp_pkt->data, (size_t)fop_resp_pkt->immed_len);

    /* decrement ack_counter */
    if (fop_resp_pkt->flags & MPIDI_CH3_PKT_FLAG_RMA_LOCK_GRANTED) {
        mpi_errno = handle_lock_ack(win_ptr, target_rank,
                                          fop_resp_pkt->flags);
        if (mpi_errno) MPIU_ERR_POP(mpi_errno);

        mpi_errno = adjust_op_piggybacked_with_lock(win_ptr, target_rank,
                                                    fop_resp_pkt->flags);
        if (mpi_errno) MPIU_ERR_POP(mpi_errno);
    }
    if (fop_resp_pkt->flags & MPIDI_CH3_PKT_FLAG_RMA_FLUSH_ACK) {
        mpi_errno = MPIDI_CH3I_RMA_Handle_flush_ack(win_ptr, target_rank);
        if (mpi_errno) MPIU_ERR_POP(mpi_errno);
    }

    MPIDI_CH3U_Request_complete(req);
    *buflen = sizeof(MPIDI_CH3_Pkt_t);
    *rreqp = NULL;

  fn_exit:
    MPIR_T_PVAR_TIMER_END(RMA, rma_rmapkt_fop_resp);
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

    MPIR_T_PVAR_TIMER_START(RMA, rma_rmapkt_get_accum_resp);

    MPID_Win_get_ptr(get_accum_resp_pkt->source_win_handle, win_ptr);

    /* decrement ack_counter on target */
    if (get_accum_resp_pkt->flags & MPIDI_CH3_PKT_FLAG_RMA_LOCK_GRANTED) {
        mpi_errno = handle_lock_ack(win_ptr, target_rank,
                                          get_accum_resp_pkt->flags);
        if (mpi_errno) MPIU_ERR_POP(mpi_errno);

        mpi_errno = adjust_op_piggybacked_with_lock(win_ptr, target_rank,
                                                    get_accum_resp_pkt->flags);
        if (mpi_errno) MPIU_ERR_POP(mpi_errno);
    }
    if (get_accum_resp_pkt->flags & MPIDI_CH3_PKT_FLAG_RMA_FLUSH_ACK) {
        mpi_errno = MPIDI_CH3I_RMA_Handle_flush_ack(win_ptr, target_rank);
        if (mpi_errno) MPIU_ERR_POP(mpi_errno);
    }

    data_len = *buflen - sizeof(MPIDI_CH3_Pkt_t);
    data_buf = (char *) pkt + sizeof(MPIDI_CH3_Pkt_t);

    MPID_Request_get_ptr(get_accum_resp_pkt->request_handle, req);

    MPID_Datatype_get_size_macro(req->dev.datatype, type_size);
    req->dev.recv_data_sz = type_size * req->dev.user_count;

    if (get_accum_resp_pkt->immed_len > 0) {
        /* first copy IMMED data from pkt header to origin buffer */
        MPIU_Memcpy(req->dev.user_buf, get_accum_resp_pkt->data, (size_t)get_accum_resp_pkt->immed_len);
        req->dev.user_buf = (void*)((char*)req->dev.user_buf + get_accum_resp_pkt->immed_len);
        req->dev.recv_data_sz -= get_accum_resp_pkt->immed_len;
        if (req->dev.recv_data_sz == 0)
            complete = 1;

        /* return the number of bytes processed in this function */
        *buflen = sizeof(MPIDI_CH3_Pkt_t);
    }

    if(req->dev.recv_data_sz > 0) {
    *rreqp = req;
    mpi_errno = MPIDI_CH3U_Receive_data_found(req, data_buf, &data_len, &complete);
    MPIU_ERR_CHKANDJUMP1(mpi_errno, mpi_errno, MPI_ERR_OTHER, "**ch3|postrecv",
                         "**ch3|postrecv %s", "MPIDI_CH3_PKT_GET_ACCUM_RESP");

    /* return the number of bytes processed in this function */
    *buflen = data_len + sizeof(MPIDI_CH3_Pkt_t);
    }
    if (complete) {
        /* Request-based RMA defines final actions for completing user request. */
        int (*reqFn)(MPIDI_VC_t *, MPID_Request *, int *);
        reqFn = req->dev.OnFinal;

        if (reqFn) {
            mpi_errno = reqFn(vc, req, &complete);
        } else {
            MPIDI_CH3U_Request_complete(req);
        }
        *rreqp = NULL;
    }

  fn_exit:
    MPIR_T_PVAR_TIMER_END(RMA, rma_rmapkt_get_accum_resp);
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

    MPIR_T_PVAR_TIMER_START(RMA, rma_rmapkt_lock);

    *buflen = sizeof(MPIDI_CH3_Pkt_t);

    MPID_Win_get_ptr(lock_pkt->target_win_handle, win_ptr);

    if (MPIDI_CH3I_Try_acquire_win_lock(win_ptr, lock_pkt->lock_type) == 1) {
        /* send lock granted packet. */
        mpi_errno = MPIDI_CH3I_Send_lock_ack_pkt(vc, win_ptr, MPIDI_CH3_PKT_FLAG_RMA_LOCK_GRANTED,
                                                 lock_pkt->source_win_handle);
        if (mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);
    }

    else {
        MPID_Request *req = NULL;
        mpi_errno = enqueue_lock_origin(win_ptr, vc, pkt, buflen, &req);
        if (mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);
        MPIU_Assert(req == NULL);
    }

    *rreqp = NULL;
  fn_fail:
    MPIR_T_PVAR_TIMER_END(RMA, rma_rmapkt_lock);
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3_PKTHANDLER_LOCK);
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

    MPIR_T_PVAR_TIMER_START(RMA, rma_rmapkt_get_resp);

    MPID_Win_get_ptr(get_resp_pkt->source_win_handle, win_ptr);

    /* decrement ack_counter on target */
    if (get_resp_pkt->flags & MPIDI_CH3_PKT_FLAG_RMA_LOCK_GRANTED) {
        mpi_errno = handle_lock_ack(win_ptr, target_rank,
                                          get_resp_pkt->flags);
        if (mpi_errno) MPIU_ERR_POP(mpi_errno);

        mpi_errno = adjust_op_piggybacked_with_lock(win_ptr, target_rank,
                                                    get_resp_pkt->flags);
        if (mpi_errno) MPIU_ERR_POP(mpi_errno);
    }
    if (get_resp_pkt->flags & MPIDI_CH3_PKT_FLAG_RMA_FLUSH_ACK) {
        mpi_errno = MPIDI_CH3I_RMA_Handle_flush_ack(win_ptr, target_rank);
        if (mpi_errno) MPIU_ERR_POP(mpi_errno);
    }

    data_len = *buflen - sizeof(MPIDI_CH3_Pkt_t);
    data_buf = (char *) pkt + sizeof(MPIDI_CH3_Pkt_t);

    MPID_Request_get_ptr(get_resp_pkt->request_handle, req);

    MPID_Datatype_get_size_macro(req->dev.datatype, type_size);
    req->dev.recv_data_sz = type_size * req->dev.user_count;

    if (get_resp_pkt->immed_len > 0) {
        /* first copy IMMED data from pkt header to origin buffer */
        MPIU_Memcpy(req->dev.user_buf, get_resp_pkt->data, (size_t)get_resp_pkt->immed_len);
        req->dev.user_buf = (void*)((char*)req->dev.user_buf + get_resp_pkt->immed_len);
        req->dev.recv_data_sz -= get_resp_pkt->immed_len;
        if (req->dev.recv_data_sz == 0)
            complete = 1;

        /* return the number of bytes processed in this function */
        *buflen = sizeof(MPIDI_CH3_Pkt_t);
    }

    if (req->dev.recv_data_sz > 0) {
    *rreqp = req;
    mpi_errno = MPIDI_CH3U_Receive_data_found(req, data_buf, &data_len, &complete);
    MPIU_ERR_CHKANDJUMP1(mpi_errno, mpi_errno, MPI_ERR_OTHER, "**ch3|postrecv", "**ch3|postrecv %s",
                         "MPIDI_CH3_PKT_GET_RESP");

    /* return the number of bytes processed in this function */
    *buflen = data_len + sizeof(MPIDI_CH3_Pkt_t);
    }

    if (complete) {
        /* Request-based RMA defines final actions for completing user request. */
        int (*reqFn)(MPIDI_VC_t *, MPID_Request *, int *);
        reqFn = req->dev.OnFinal;

        if (reqFn) {
            mpi_errno = reqFn(vc, req, &complete);
        } else {
            MPIDI_CH3U_Request_complete(req);
        }

        *rreqp = NULL;
    }

  fn_exit:
    MPIR_T_PVAR_TIMER_END(RMA, rma_rmapkt_get_resp);
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3_PKTHANDLER_GETRESP);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3_PktHandler_LockAck
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3_PktHandler_LockAck(MPIDI_VC_t * vc, MPIDI_CH3_Pkt_t * pkt,
                                     MPIDI_msg_sz_t * buflen, MPID_Request ** rreqp)
{
    MPIDI_CH3_Pkt_lock_ack_t *lock_ack_pkt = &pkt->lock_ack;
    MPID_Win *win_ptr = NULL;
    int target_rank = lock_ack_pkt->target_rank;
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3_PKTHANDLER_LOCKACK);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3_PKTHANDLER_LOCKACK);

    MPIU_DBG_MSG(CH3_OTHER, VERBOSE, "received lock ack pkt");

    MPIR_T_PVAR_TIMER_START(RMA, rma_rmapkt_lock_ack);

    *buflen = sizeof(MPIDI_CH3_Pkt_t);

    MPID_Win_get_ptr(lock_ack_pkt->source_win_handle, win_ptr);

    mpi_errno = handle_lock_ack(win_ptr, target_rank,
                                      lock_ack_pkt->flags);
    if (mpi_errno) MPIU_ERR_POP(mpi_errno);

    *rreqp = NULL;
    MPIDI_CH3_Progress_signal_completion();

    MPIR_T_PVAR_TIMER_END(RMA, rma_rmapkt_lock_ack);
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3_PKTHANDLER_LOCKACK);
 fn_exit:
    return MPI_SUCCESS;
 fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3_PktHandler_LockOpAck
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3_PktHandler_LockOpAck(MPIDI_VC_t * vc, MPIDI_CH3_Pkt_t * pkt,
                                   MPIDI_msg_sz_t * buflen, MPID_Request ** rreqp)
{
    MPIDI_CH3_Pkt_lock_op_ack_t *lock_op_ack_pkt = &pkt->lock_op_ack;
    MPID_Win *win_ptr = NULL;
    int target_rank = lock_op_ack_pkt->target_rank;
    MPIDI_CH3_Pkt_flags_t flags = lock_op_ack_pkt->flags;
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3_PKTHANDLER_LOCKOPACK);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3_PKTHANDLER_LOCKOPACK);

    *buflen = sizeof(MPIDI_CH3_Pkt_t);

    MPID_Win_get_ptr(lock_op_ack_pkt->source_win_handle, win_ptr);

    mpi_errno = handle_lock_ack(win_ptr, target_rank,
                                      lock_op_ack_pkt->flags);
    if (mpi_errno) MPIU_ERR_POP(mpi_errno);

    mpi_errno = adjust_op_piggybacked_with_lock(win_ptr, target_rank,
                                                lock_op_ack_pkt->flags);
    if (mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);

    if (flags & MPIDI_CH3_PKT_FLAG_RMA_FLUSH_ACK) {
        MPIU_Assert(flags & MPIDI_CH3_PKT_FLAG_RMA_LOCK_GRANTED);
        mpi_errno = MPIDI_CH3I_RMA_Handle_flush_ack(win_ptr, target_rank);
        if (mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);
    }

    *rreqp = NULL;
    MPIDI_CH3_Progress_signal_completion();

    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3_PKTHANDLER_LOCKOPACK);
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

    MPIR_T_PVAR_TIMER_START(RMA, rma_rmapkt_flush_ack);

    *buflen = sizeof(MPIDI_CH3_Pkt_t);

    MPID_Win_get_ptr(flush_ack_pkt->source_win_handle, win_ptr);

    /* decrement ack_counter on target */
    mpi_errno = MPIDI_CH3I_RMA_Handle_flush_ack(win_ptr, target_rank);
    if (mpi_errno) MPIU_ERR_POP(mpi_errno);

    *rreqp = NULL;
    MPIDI_CH3_Progress_signal_completion();

    MPIR_T_PVAR_TIMER_END(RMA, rma_rmapkt_flush_ack);
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

    MPIR_T_PVAR_TIMER_START(RMA, rma_rmapkt_decr_at_cnt);

    MPID_Win_get_ptr(decr_at_cnt_pkt->target_win_handle, win_ptr);

    win_ptr->at_completion_counter--;
    MPIU_Assert(win_ptr->at_completion_counter >= 0);

    *buflen = sizeof(MPIDI_CH3_Pkt_t);
    *rreqp = NULL;
    MPIDI_CH3_Progress_signal_completion();

 fn_exit:
    MPIR_T_PVAR_TIMER_END(RMA, rma_rmapkt_decr_at_cnt);
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

    MPIR_T_PVAR_TIMER_START(RMA, rma_rmapkt_unlock);

    *buflen = sizeof(MPIDI_CH3_Pkt_t);
    *rreqp = NULL;

    MPID_Win_get_ptr(unlock_pkt->target_win_handle, win_ptr);
    mpi_errno = MPIDI_CH3I_Release_lock(win_ptr);
    MPIU_ERR_CHKANDJUMP(mpi_errno != MPI_SUCCESS, mpi_errno, MPI_ERR_OTHER, "**ch3|rma_msg");

    if (!(unlock_pkt->flags & MPIDI_CH3_PKT_FLAG_RMA_UNLOCK_NO_ACK)) {
        mpi_errno = MPIDI_CH3I_Send_flush_ack_pkt(vc, win_ptr,
                                                  unlock_pkt->source_win_handle);
        if (mpi_errno) MPIU_ERR_POP(mpi_errno);
    }

    MPIDI_CH3_Progress_signal_completion();

  fn_exit:
    MPIR_T_PVAR_TIMER_END(RMA, rma_rmapkt_unlock);
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

    MPIR_T_PVAR_TIMER_START(RMA, rma_rmapkt_flush);

    *buflen = sizeof(MPIDI_CH3_Pkt_t);
    *rreqp = NULL;

    MPID_Win_get_ptr(flush_pkt->target_win_handle, win_ptr);

    mpi_errno = MPIDI_CH3I_Send_flush_ack_pkt(vc, win_ptr,
                                              flush_pkt->source_win_handle);
    if (mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);

  fn_exit:
    MPIR_T_PVAR_TIMER_END(RMA, rma_rmapkt_flush);
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3_PKTHANDLER_FLUSH);
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

int MPIDI_CH3_PktPrint_Lock(FILE * fp, MPIDI_CH3_Pkt_t * pkt)
{
    MPIU_DBG_PRINTF((" type ......... MPIDI_CH3_PKT_LOCK\n"));
    MPIU_DBG_PRINTF((" lock_type .... %d\n", pkt->lock.lock_type));
    MPIU_DBG_PRINTF((" target ....... 0x%08X\n", pkt->lock.target_win_handle));
    MPIU_DBG_PRINTF((" source ....... 0x%08X\n", pkt->lock.source_win_handle));
    return MPI_SUCCESS;
}

int MPIDI_CH3_PktPrint_FlushAck(FILE * fp, MPIDI_CH3_Pkt_t * pkt)
{
    MPIU_DBG_PRINTF((" type ......... MPIDI_CH3_PKT_FLUSH_ACK\n"));
    MPIU_DBG_PRINTF((" source ....... 0x%08X\n", pkt->lock_accum_unlock.source_win_handle));
    return MPI_SUCCESS;
}

int MPIDI_CH3_PktPrint_LockAck(FILE * fp, MPIDI_CH3_Pkt_t * pkt)
{
    MPIU_DBG_PRINTF((" type ......... MPIDI_CH3_PKT_LOCK_ACK\n"));
    MPIU_DBG_PRINTF((" source ....... 0x%08X\n", pkt->lock_ack.source_win_handle));
    return MPI_SUCCESS;
}
#endif
