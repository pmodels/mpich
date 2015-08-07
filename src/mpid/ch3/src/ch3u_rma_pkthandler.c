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
MPIR_T_PVAR_DOUBLE_TIMER_DECL(RMA, rma_rmapkt_ack);
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
                                      "RMA",
                                      "RMA:PKTHANDLER for Get-Accumulate response (in seconds)");

    /* rma_rmapkt_cas_resp */
    MPIR_T_PVAR_TIMER_REGISTER_STATIC(RMA,
                                      MPI_DOUBLE,
                                      rma_rmapkt_cas_resp,
                                      MPI_T_VERBOSITY_MPIDEV_DETAIL,
                                      MPI_T_BIND_NO_OBJECT,
                                      MPIR_T_PVAR_FLAG_READONLY,
                                      "RMA",
                                      "RMA:PKTHANDLER for Compare-and-Swap response (in seconds)");

    /* rma_rmapkt_fop_resp */
    MPIR_T_PVAR_TIMER_REGISTER_STATIC(RMA,
                                      MPI_DOUBLE,
                                      rma_rmapkt_fop_resp,
                                      MPI_T_VERBOSITY_MPIDEV_DETAIL,
                                      MPI_T_BIND_NO_OBJECT,
                                      MPIR_T_PVAR_FLAG_READONLY,
                                      "RMA",
                                      "RMA:PKTHANDLER for Fetch-and-op response (in seconds)");

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

    /* rma_rmapkt_ack */
    MPIR_T_PVAR_TIMER_REGISTER_STATIC(RMA,
                                      MPI_DOUBLE,
                                      rma_rmapkt_ack,
                                      MPI_T_VERBOSITY_MPIDEV_DETAIL,
                                      MPI_T_BIND_NO_OBJECT,
                                      MPIR_T_PVAR_FLAG_READONLY,
                                      "RMA", "RMA:PKTHANDLER for Ack (in seconds)");

    /* rma_rmapkt_decr_at_cnt */
    MPIR_T_PVAR_TIMER_REGISTER_STATIC(RMA,
                                      MPI_DOUBLE,
                                      rma_rmapkt_decr_at_cnt,
                                      MPI_T_VERBOSITY_MPIDEV_DETAIL,
                                      MPI_T_BIND_NO_OBJECT,
                                      MPIR_T_PVAR_FLAG_READONLY,
                                      "RMA", "RMA:PKTHANDLER for Decr-At-Cnt (in seconds)");
}

/* =========================================================== */
/*                  extended packet functions                  */
/* =========================================================== */

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3_ExtPktHandler_Accumulate
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static int MPIDI_CH3_ExtPktHandler_Accumulate(MPIDI_CH3_Pkt_flags_t flags,
                                              int is_derived_dt, void **ext_hdr_ptr,
                                              MPI_Aint * ext_hdr_sz)
{
    int mpi_errno = MPI_SUCCESS;

    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3_EXTPKTHANDLER_ACCUMULATE);
    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3_EXTPKTHANDLER_ACCUMULATE);

    if ((flags & MPIDI_CH3_PKT_FLAG_RMA_STREAM) && is_derived_dt) {
        (*ext_hdr_sz) = sizeof(MPIDI_CH3_Ext_pkt_accum_stream_derived_t);
        (*ext_hdr_ptr) = MPIU_Malloc(sizeof(MPIDI_CH3_Ext_pkt_accum_stream_derived_t));
        if ((*ext_hdr_ptr) == NULL) {
            MPIR_ERR_SETANDJUMP1(mpi_errno, MPI_ERR_OTHER, "**nomem",
                                 "**nomem %s", "MPIDI_CH3_Ext_pkt_accum_stream_derived_t");
        }
    }
    else if (flags & MPIDI_CH3_PKT_FLAG_RMA_STREAM) {
        (*ext_hdr_sz) = sizeof(MPIDI_CH3_Ext_pkt_accum_stream_t);
        (*ext_hdr_ptr) = MPIU_Malloc(sizeof(MPIDI_CH3_Ext_pkt_accum_stream_t));
        if ((*ext_hdr_ptr) == NULL) {
            MPIR_ERR_SETANDJUMP1(mpi_errno, MPI_ERR_OTHER, "**nomem",
                                 "**nomem %s", "MPIDI_CH3_Ext_pkt_accum_stream_t");
        }
    }
    else if (is_derived_dt) {
        (*ext_hdr_sz) = sizeof(MPIDI_CH3_Ext_pkt_accum_derived_t);
        (*ext_hdr_ptr) = MPIU_Malloc(sizeof(MPIDI_CH3_Ext_pkt_accum_derived_t));
        if ((*ext_hdr_ptr) == NULL) {
            MPIR_ERR_SETANDJUMP1(mpi_errno, MPI_ERR_OTHER, "**nomem",
                                 "**nomem %s", "MPIDI_CH3_Ext_pkt_accum_derived_t");
        }
    }

  fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3_EXTPKTHANDLER_ACCUMULATE);
    return mpi_errno;
  fn_fail:
    if ((*ext_hdr_ptr) != NULL)
        MPIU_Free((*ext_hdr_ptr));
    (*ext_hdr_ptr) = NULL;
    (*ext_hdr_sz) = 0;
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3_ExtPktHandler_GetAccumulate
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static int MPIDI_CH3_ExtPktHandler_GetAccumulate(MPIDI_CH3_Pkt_flags_t flags,
                                                 int is_derived_dt, void **ext_hdr_ptr,
                                                 MPI_Aint * ext_hdr_sz)
{
    /* Check if get_accum still reuses accum' extended packet header. */
    MPIU_Assert(sizeof(MPIDI_CH3_Ext_pkt_accum_stream_derived_t) ==
                sizeof(MPIDI_CH3_Ext_pkt_get_accum_stream_derived_t));
    MPIU_Assert(sizeof(MPIDI_CH3_Ext_pkt_accum_derived_t) ==
                sizeof(MPIDI_CH3_Ext_pkt_get_accum_derived_t));
    MPIU_Assert(sizeof(MPIDI_CH3_Ext_pkt_accum_stream_t) ==
                sizeof(MPIDI_CH3_Ext_pkt_get_accum_stream_t));

    return MPIDI_CH3_ExtPktHandler_Accumulate(flags, is_derived_dt, ext_hdr_ptr, ext_hdr_sz);
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
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIDI_CH3_PktHandler_Put(MPIDI_VC_t * vc, MPIDI_CH3_Pkt_t * pkt,
                             MPIDI_msg_sz_t * buflen, MPID_Request ** rreqp)
{
    MPIDI_CH3_Pkt_put_t *put_pkt = &pkt->put;
    MPID_Request *req = NULL;
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

    mpi_errno = check_piggyback_lock(win_ptr, vc, pkt, buflen, &acquire_lock_fail, &req);
    if (mpi_errno != MPI_SUCCESS)
        MPIR_ERR_POP(mpi_errno);

    if (acquire_lock_fail) {
        (*rreqp) = req;
        goto fn_exit;
    }

    if (pkt->type == MPIDI_CH3_PKT_PUT_IMMED) {
        MPI_Aint type_size;

        /* Immed packet type is used when target datatype is predefined datatype. */
        MPIU_Assert(MPIR_DATATYPE_IS_PREDEFINED(put_pkt->datatype));

        MPID_Datatype_get_size_macro(put_pkt->datatype, type_size);

        /* copy data from packet header to target buffer */
        MPIU_Memcpy(put_pkt->addr, put_pkt->info.data, put_pkt->count * type_size);

        /* trigger final action */
        mpi_errno = finish_op_on_target(win_ptr, vc, FALSE /* has no response data */ ,
                                        put_pkt->flags, put_pkt->source_win_handle);
        if (mpi_errno != MPI_SUCCESS)
            MPIR_ERR_POP(mpi_errno);

        *buflen = sizeof(MPIDI_CH3_Pkt_t);
        *rreqp = NULL;
    }
    else {
        MPIU_Assert(pkt->type == MPIDI_CH3_PKT_PUT);

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
            MPI_Aint type_size;

            MPIDI_Request_set_type(req, MPIDI_REQUEST_TYPE_PUT_RECV);
            req->dev.datatype = put_pkt->datatype;

            MPID_Datatype_get_size_macro(put_pkt->datatype, type_size);

            req->dev.recv_data_sz = type_size * put_pkt->count;
            MPIU_Assert(req->dev.recv_data_sz > 0);

            mpi_errno = MPIDI_CH3U_Receive_data_found(req, data_buf, &data_len, &complete);
            MPIR_ERR_CHKANDJUMP1(mpi_errno, mpi_errno, MPI_ERR_OTHER, "**ch3|postrecv",
                                 "**ch3|postrecv %s", "MPIDI_CH3_PKT_PUT");

            /* return the number of bytes processed in this function */
            *buflen = sizeof(MPIDI_CH3_Pkt_t) + data_len;

            if (complete) {
                mpi_errno = MPIDI_CH3_ReqHandler_PutRecvComplete(vc, req, &complete);
                if (mpi_errno)
                    MPIR_ERR_POP(mpi_errno);
                if (complete) {
                    *rreqp = NULL;
                    goto fn_exit;
                }
            }
        }
        else {
            /* derived datatype */
            MPIDI_Request_set_type(req, MPIDI_REQUEST_TYPE_PUT_RECV_DERIVED_DT);
            req->dev.datatype = MPI_DATATYPE_NULL;

            /* allocate extended header in the request,
             * only including fixed-length variables defined in packet type. */
            req->dev.ext_hdr_sz = sizeof(MPIDI_CH3_Ext_pkt_put_derived_t);
            req->dev.ext_hdr_ptr = MPIU_Malloc(req->dev.ext_hdr_sz);
            if (!req->dev.ext_hdr_ptr) {
                MPIR_ERR_SETANDJUMP1(mpi_errno, MPI_ERR_OTHER, "**nomem", "**nomem %s",
                                     "MPIDI_CH3_Ext_pkt_put_derived_t");
            }

            /* put dataloop in a separate buffer to be reused in datatype.
             * It will be freed when free datatype. */
            req->dev.dataloop = MPIU_Malloc(put_pkt->info.dataloop_size);
            if (!req->dev.dataloop) {
                MPIR_ERR_SETANDJUMP1(mpi_errno, MPI_ERR_OTHER, "**nomem", "**nomem %d",
                                     put_pkt->info.dataloop_size);
            }

            /* if we received all of the dtype_info and dataloop, copy it
             * now and call the handler, otherwise set the iov and let the
             * channel copy it */
            if (data_len >= req->dev.ext_hdr_sz + put_pkt->info.dataloop_size) {
                /* Copy extended header */
                MPIU_Memcpy(req->dev.ext_hdr_ptr, data_buf, req->dev.ext_hdr_sz);
                MPIU_Memcpy(req->dev.dataloop, data_buf + req->dev.ext_hdr_sz,
                            put_pkt->info.dataloop_size);

                *buflen = sizeof(MPIDI_CH3_Pkt_t) + req->dev.ext_hdr_sz +
                    put_pkt->info.dataloop_size;

                /* All dtype data has been received, call req handler */
                mpi_errno = MPIDI_CH3_ReqHandler_PutDerivedDTRecvComplete(vc, req, &complete);
                MPIR_ERR_CHKANDJUMP1(mpi_errno, mpi_errno, MPI_ERR_OTHER, "**ch3|postrecv",
                                     "**ch3|postrecv %s", "MPIDI_CH3_PKT_PUT");
                if (complete) {
                    *rreqp = NULL;
                    goto fn_exit;
                }
            }
            else {
                req->dev.iov[0].MPL_IOV_BUF = (MPL_IOV_BUF_CAST) ((char *) req->dev.ext_hdr_ptr);
                req->dev.iov[0].MPL_IOV_LEN = req->dev.ext_hdr_sz;
                req->dev.iov[1].MPL_IOV_BUF = (MPL_IOV_BUF_CAST) req->dev.dataloop;
                req->dev.iov[1].MPL_IOV_LEN = put_pkt->info.dataloop_size;
                req->dev.iov_count = 2;

                *buflen = sizeof(MPIDI_CH3_Pkt_t);

                req->dev.OnDataAvail = MPIDI_CH3_ReqHandler_PutDerivedDTRecvComplete;
            }

        }

        *rreqp = req;

        if (mpi_errno != MPI_SUCCESS) {
            MPIR_ERR_SET1(mpi_errno, MPI_ERR_OTHER, "**ch3|postrecv",
                          "**ch3|postrecv %s", "MPIDI_CH3_PKT_PUT");
        }
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
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIDI_CH3_PktHandler_Get(MPIDI_VC_t * vc, MPIDI_CH3_Pkt_t * pkt,
                             MPIDI_msg_sz_t * buflen, MPID_Request ** rreqp)
{
    MPIDI_CH3_Pkt_get_t *get_pkt = &pkt->get;
    MPID_Request *req = NULL;
    MPL_IOV iov[MPL_IOV_LIMIT];
    int complete = 0;
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

    mpi_errno = check_piggyback_lock(win_ptr, vc, pkt, buflen, &acquire_lock_fail, &req);
    if (mpi_errno != MPI_SUCCESS)
        MPIR_ERR_POP(mpi_errno);

    if (acquire_lock_fail) {
        (*rreqp) = req;
        goto fn_exit;
    }

    req = MPID_Request_create();
    req->dev.target_win_handle = get_pkt->target_win_handle;
    req->dev.flags = get_pkt->flags;

    /* get start location of data and length of data */
    data_len = *buflen - sizeof(MPIDI_CH3_Pkt_t);
    data_buf = (char *) pkt + sizeof(MPIDI_CH3_Pkt_t);

    /* here we increment the Active Target counter to guarantee the GET-like
     * operation are completed when counter reaches zero. */
    win_ptr->at_completion_counter++;

    if (get_pkt->flags & MPIDI_CH3_PKT_FLAG_RMA_IMMED_RESP) {
        MPIU_Assert(MPIR_DATATYPE_IS_PREDEFINED(get_pkt->datatype));
    }

    if (MPIR_DATATYPE_IS_PREDEFINED(get_pkt->datatype)) {
        /* basic datatype. send the data. */
        MPIDI_CH3_Pkt_t upkt;
        MPIDI_CH3_Pkt_get_resp_t *get_resp_pkt = &upkt.get_resp;
        size_t len;
        int iovcnt;
        int is_contig;

        MPIDI_Request_set_type(req, MPIDI_REQUEST_TYPE_GET_RESP);
        req->dev.OnDataAvail = MPIDI_CH3_ReqHandler_GetSendComplete;
        req->dev.OnFinal = MPIDI_CH3_ReqHandler_GetSendComplete;
        req->kind = MPID_REQUEST_SEND;

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

        MPID_Datatype_is_contig(get_pkt->datatype, &is_contig);

        if (get_pkt->flags & MPIDI_CH3_PKT_FLAG_RMA_IMMED_RESP) {
            MPIU_Assign_trunc(len, get_pkt->count * type_size, size_t);
            void *src = (void *) (get_pkt->addr), *dest = (void *) (get_resp_pkt->info.data);
            mpi_errno = immed_copy(src, dest, len);
            if (mpi_errno != MPI_SUCCESS)
                MPIR_ERR_POP(mpi_errno);

            iov[0].MPL_IOV_BUF = (MPL_IOV_BUF_CAST) get_resp_pkt;
            iov[0].MPL_IOV_LEN = sizeof(*get_resp_pkt);
            iovcnt = 1;

            MPID_THREAD_CS_ENTER(POBJ, vc->pobj_mutex);
            mpi_errno = MPIDI_CH3_iSendv(vc, req, iov, iovcnt);
            MPID_THREAD_CS_EXIT(POBJ, vc->pobj_mutex);
            /* --BEGIN ERROR HANDLING-- */
            if (mpi_errno != MPI_SUCCESS) {
                MPID_Request_release(req);
                MPIR_ERR_SETANDJUMP(mpi_errno, MPI_ERR_OTHER, "**ch3|rmamsg");
            }
            /* --END ERROR HANDLING-- */
        }
        else if (is_contig) {
            iov[0].MPL_IOV_BUF = (MPL_IOV_BUF_CAST) get_resp_pkt;
            iov[0].MPL_IOV_LEN = sizeof(*get_resp_pkt);
            iov[1].MPL_IOV_BUF = (MPL_IOV_BUF_CAST) ((char *) get_pkt->addr);
            iov[1].MPL_IOV_LEN = get_pkt->count * type_size;
            iovcnt = 2;

            MPID_THREAD_CS_ENTER(POBJ, vc->pobj_mutex);
            mpi_errno = MPIDI_CH3_iSendv(vc, req, iov, iovcnt);
            MPID_THREAD_CS_EXIT(POBJ, vc->pobj_mutex);
            /* --BEGIN ERROR HANDLING-- */
            if (mpi_errno != MPI_SUCCESS) {
                MPID_Request_release(req);
                MPIR_ERR_SETANDJUMP(mpi_errno, MPI_ERR_OTHER, "**ch3|rmamsg");
            }
            /* --END ERROR HANDLING-- */
        }
        else {
            iov[0].MPL_IOV_BUF = (MPL_IOV_BUF_CAST) get_resp_pkt;
            iov[0].MPL_IOV_LEN = sizeof(*get_resp_pkt);

            req->dev.segment_ptr = MPID_Segment_alloc();
            MPIR_ERR_CHKANDJUMP1(req->dev.segment_ptr == NULL, mpi_errno,
                                 MPI_ERR_OTHER, "**nomem", "**nomem %s", "MPID_Segment_alloc");

            MPID_Segment_init(get_pkt->addr, get_pkt->count,
                              get_pkt->datatype, req->dev.segment_ptr, 0);
            req->dev.segment_first = 0;
            req->dev.segment_size = get_pkt->count * type_size;

            MPID_THREAD_CS_ENTER(POBJ, vc->pobj_mutex);
            mpi_errno = vc->sendNoncontig_fn(vc, req, iov[0].MPL_IOV_BUF, iov[0].MPL_IOV_LEN);
            MPID_THREAD_CS_EXIT(POBJ, vc->pobj_mutex);
            MPIR_ERR_CHKANDJUMP(mpi_errno, mpi_errno, MPI_ERR_OTHER, "**ch3|rmamsg");
        }

        *buflen = sizeof(MPIDI_CH3_Pkt_t);
        *rreqp = NULL;
    }
    else {
        /* derived datatype. first get the dtype_info and dataloop. */

        MPIDI_Request_set_type(req, MPIDI_REQUEST_TYPE_GET_RECV_DERIVED_DT);
        req->dev.OnDataAvail = MPIDI_CH3_ReqHandler_GetDerivedDTRecvComplete;
        req->dev.OnFinal = 0;
        req->dev.user_buf = get_pkt->addr;
        req->dev.user_count = get_pkt->count;
        req->dev.datatype = MPI_DATATYPE_NULL;
        req->dev.request_handle = get_pkt->request_handle;

        /* allocate extended header in the request,
         * only including fixed-length variables defined in packet type. */
        req->dev.ext_hdr_sz = sizeof(MPIDI_CH3_Ext_pkt_get_derived_t);
        req->dev.ext_hdr_ptr = MPIU_Malloc(req->dev.ext_hdr_sz);
        if (!req->dev.ext_hdr_ptr) {
            MPIR_ERR_SETANDJUMP1(mpi_errno, MPI_ERR_OTHER, "**nomem", "**nomem %s",
                                 "MPIDI_CH3_Ext_pkt_get_derived_t");
        }

        /* put dataloop in a separate buffer to be reused in datatype.
         * It will be freed when free datatype. */
        req->dev.dataloop = MPIU_Malloc(get_pkt->info.dataloop_size);
        if (!req->dev.dataloop) {
            MPIR_ERR_SETANDJUMP1(mpi_errno, MPI_ERR_OTHER, "**nomem", "**nomem %d",
                                 get_pkt->info.dataloop_size);
        }

        /* if we received all of the dtype_info and dataloop, copy it
         * now and call the handler, otherwise set the iov and let the
         * channel copy it */
        if (data_len >= req->dev.ext_hdr_sz + get_pkt->info.dataloop_size) {
            /* Copy extended header */
            MPIU_Memcpy(req->dev.ext_hdr_ptr, data_buf, req->dev.ext_hdr_sz);
            MPIU_Memcpy(req->dev.dataloop, data_buf + req->dev.ext_hdr_sz,
                        get_pkt->info.dataloop_size);

            *buflen = sizeof(MPIDI_CH3_Pkt_t) + req->dev.ext_hdr_sz + get_pkt->info.dataloop_size;

            /* All dtype data has been received, call req handler */
            mpi_errno = MPIDI_CH3_ReqHandler_GetDerivedDTRecvComplete(vc, req, &complete);
            MPIR_ERR_CHKANDJUMP1(mpi_errno, mpi_errno, MPI_ERR_OTHER, "**ch3|postrecv",
                                 "**ch3|postrecv %s", "MPIDI_CH3_PKT_GET");
            if (complete)
                *rreqp = NULL;
        }
        else {
            req->dev.iov[0].MPL_IOV_BUF = (MPL_IOV_BUF_CAST) ((char *) req->dev.ext_hdr_ptr);
            req->dev.iov[0].MPL_IOV_LEN = req->dev.ext_hdr_sz;
            req->dev.iov[1].MPL_IOV_BUF = (MPL_IOV_BUF_CAST) req->dev.dataloop;
            req->dev.iov[1].MPL_IOV_LEN = get_pkt->info.dataloop_size;
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
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIDI_CH3_PktHandler_Accumulate(MPIDI_VC_t * vc, MPIDI_CH3_Pkt_t * pkt,
                                    MPIDI_msg_sz_t * buflen, MPID_Request ** rreqp)
{
    MPIDI_CH3_Pkt_accum_t *accum_pkt = &pkt->accum;
    MPID_Request *req = NULL;
    MPI_Aint extent;
    int complete = 0;
    char *data_buf = NULL;
    MPIDI_msg_sz_t data_len;
    MPID_Win *win_ptr;
    int acquire_lock_fail = 0;
    int mpi_errno = MPI_SUCCESS;
    MPI_Aint type_size;
    MPI_Aint stream_elem_count, total_len;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3_PKTHANDLER_ACCUMULATE);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3_PKTHANDLER_ACCUMULATE);

    MPIU_DBG_MSG(CH3_OTHER, VERBOSE, "received accumulate pkt");

    MPIR_T_PVAR_TIMER_START(RMA, rma_rmapkt_acc);

    MPIU_Assert(accum_pkt->target_win_handle != MPI_WIN_NULL);
    MPID_Win_get_ptr(accum_pkt->target_win_handle, win_ptr);

    mpi_errno = check_piggyback_lock(win_ptr, vc, pkt, buflen, &acquire_lock_fail, &req);
    if (mpi_errno != MPI_SUCCESS)
        MPIR_ERR_POP(mpi_errno);

    if (acquire_lock_fail) {
        (*rreqp) = req;
        goto fn_exit;
    }

    if (pkt->type == MPIDI_CH3_PKT_ACCUMULATE_IMMED) {
        /* Immed packet type is used when target datatype is predefined datatype. */
        MPIU_Assert(MPIR_DATATYPE_IS_PREDEFINED(accum_pkt->datatype));

        if (win_ptr->shm_allocated == TRUE)
            MPIDI_CH3I_SHM_MUTEX_LOCK(win_ptr);
        mpi_errno = do_accumulate_op(accum_pkt->info.data, accum_pkt->count, accum_pkt->datatype,
                                     accum_pkt->addr, accum_pkt->count, accum_pkt->datatype,
                                     0, accum_pkt->op);
        if (win_ptr->shm_allocated == TRUE)
            MPIDI_CH3I_SHM_MUTEX_UNLOCK(win_ptr);
        if (mpi_errno) {
            MPIR_ERR_POP(mpi_errno);
        }

        /* trigger final action */
        mpi_errno = finish_op_on_target(win_ptr, vc, FALSE /* has no response data */ ,
                                        accum_pkt->flags, accum_pkt->source_win_handle);
        if (mpi_errno != MPI_SUCCESS)
            MPIR_ERR_POP(mpi_errno);

        *buflen = sizeof(MPIDI_CH3_Pkt_t);
        *rreqp = NULL;
    }
    else {
        MPIU_Assert(pkt->type == MPIDI_CH3_PKT_ACCUMULATE);

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

        /* get start location of data and length of data */
        data_len = *buflen - sizeof(MPIDI_CH3_Pkt_t);
        data_buf = (char *) pkt + sizeof(MPIDI_CH3_Pkt_t);

        /* allocate extended header in the request,
         * only including fixed-length variables defined in packet type. */
        mpi_errno = MPIDI_CH3_ExtPktHandler_Accumulate(req->dev.flags,
                                                       (!MPIR_DATATYPE_IS_PREDEFINED
                                                        (accum_pkt->datatype)),
                                                       &req->dev.ext_hdr_ptr, &req->dev.ext_hdr_sz);
        if (mpi_errno != MPI_SUCCESS)
            MPIR_ERR_POP(mpi_errno);

        if (MPIR_DATATYPE_IS_PREDEFINED(accum_pkt->datatype)) {
            MPIDI_Request_set_type(req, MPIDI_REQUEST_TYPE_ACCUM_RECV);
            req->dev.datatype = accum_pkt->datatype;

            if (req->dev.flags & MPIDI_CH3_PKT_FLAG_RMA_STREAM) {
                req->dev.OnDataAvail = MPIDI_CH3_ReqHandler_AccumMetadataRecvComplete;

                /* if this is a streamed op pkt, set iov to receive extended pkt header. */
                req->dev.iov[0].MPL_IOV_BUF = (MPL_IOV_BUF_CAST) req->dev.ext_hdr_ptr;
                req->dev.iov[0].MPL_IOV_LEN = req->dev.ext_hdr_sz;
                req->dev.iov_count = 1;

                *buflen = sizeof(MPIDI_CH3_Pkt_t);
            }
            else {
                req->dev.OnFinal = MPIDI_CH3_ReqHandler_AccumRecvComplete;

                MPID_Datatype_get_extent_macro(accum_pkt->datatype, extent);

                MPIU_Assert(!MPIDI_Request_get_srbuf_flag(req));
                /* allocate a SRBuf for receiving stream unit */
                MPIDI_CH3U_SRBuf_alloc(req, MPIDI_CH3U_SRBuf_size);
                /* --BEGIN ERROR HANDLING-- */
                if (req->dev.tmpbuf_sz == 0) {
                    MPIU_DBG_MSG(CH3_CHANNEL, TYPICAL, "SRBuf allocation failure");
                    mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL,
                                                     FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem",
                                                     "**nomem %d", MPIDI_CH3U_SRBuf_size);
                    req->status.MPI_ERROR = mpi_errno;
                    goto fn_fail;
                }
                /* --END ERROR HANDLING-- */

                req->dev.user_buf = req->dev.tmpbuf;

                MPID_Datatype_get_size_macro(accum_pkt->datatype, type_size);

                total_len = type_size * accum_pkt->count;
                stream_elem_count = MPIDI_CH3U_SRBuf_size / extent;

                req->dev.recv_data_sz = MPIR_MIN(total_len, stream_elem_count * type_size);
                MPIU_Assert(req->dev.recv_data_sz > 0);

                mpi_errno = MPIDI_CH3U_Receive_data_found(req, data_buf, &data_len, &complete);
                MPIR_ERR_CHKANDJUMP1(mpi_errno, mpi_errno, MPI_ERR_OTHER, "**ch3|postrecv",
                                     "**ch3|postrecv %s", "MPIDI_CH3_PKT_ACCUMULATE");

                /* return the number of bytes processed in this function */
                *buflen = data_len + sizeof(MPIDI_CH3_Pkt_t);

                if (complete) {
                    mpi_errno = MPIDI_CH3_ReqHandler_AccumRecvComplete(vc, req, &complete);
                    if (mpi_errno)
                        MPIR_ERR_POP(mpi_errno);
                    if (complete) {
                        *rreqp = NULL;
                        goto fn_exit;
                    }
                }
            }
        }
        else {
            MPIDI_Request_set_type(req, MPIDI_REQUEST_TYPE_ACCUM_RECV_DERIVED_DT);
            req->dev.OnDataAvail = MPIDI_CH3_ReqHandler_AccumMetadataRecvComplete;
            req->dev.datatype = MPI_DATATYPE_NULL;

            /* Put dataloop in a separate buffer to be reused in datatype.
             * It will be freed when free datatype. */
            req->dev.dataloop = MPIU_Malloc(accum_pkt->info.dataloop_size);
            if (!req->dev.dataloop) {
                MPIR_ERR_SETANDJUMP1(mpi_errno, MPI_ERR_OTHER, "**nomem", "**nomem %d",
                                     accum_pkt->info.dataloop_size);
            }

            if (data_len >= req->dev.ext_hdr_sz + accum_pkt->info.dataloop_size) {
                /* Copy extended header */
                MPIU_Memcpy(req->dev.ext_hdr_ptr, data_buf, req->dev.ext_hdr_sz);
                MPIU_Memcpy(req->dev.dataloop, data_buf + req->dev.ext_hdr_sz,
                            accum_pkt->info.dataloop_size);

                *buflen = sizeof(MPIDI_CH3_Pkt_t) + req->dev.ext_hdr_sz +
                    accum_pkt->info.dataloop_size;

                /* All extended data has been received, call req handler */
                mpi_errno = MPIDI_CH3_ReqHandler_AccumMetadataRecvComplete(vc, req, &complete);
                MPIR_ERR_CHKANDJUMP1(mpi_errno, mpi_errno, MPI_ERR_OTHER, "**ch3|postrecv",
                                     "**ch3|postrecv %s", "MPIDI_CH3_ACCUMULATE");
                if (complete) {
                    *rreqp = NULL;
                    goto fn_exit;
                }
            }
            else {
                /* Prepare to receive extended header.
                 * All variable-length data can be received in separate iovs. */
                req->dev.iov[0].MPL_IOV_BUF = (MPL_IOV_BUF_CAST) req->dev.ext_hdr_ptr;
                req->dev.iov[0].MPL_IOV_LEN = req->dev.ext_hdr_sz;
                req->dev.iov[1].MPL_IOV_BUF = (MPL_IOV_BUF_CAST) req->dev.dataloop;
                req->dev.iov[1].MPL_IOV_LEN = accum_pkt->info.dataloop_size;
                req->dev.iov_count = 2;

                *buflen = sizeof(MPIDI_CH3_Pkt_t);
            }

        }
    }

    if (mpi_errno != MPI_SUCCESS) {
        MPIR_ERR_SETANDJUMP1(mpi_errno, MPI_ERR_OTHER, "**ch3|postrecv",
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
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIDI_CH3_PktHandler_GetAccumulate(MPIDI_VC_t * vc, MPIDI_CH3_Pkt_t * pkt,
                                       MPIDI_msg_sz_t * buflen, MPID_Request ** rreqp)
{
    MPIDI_CH3_Pkt_get_accum_t *get_accum_pkt = &pkt->get_accum;
    MPID_Request *req = NULL;
    MPI_Aint extent;
    int complete = 0;
    char *data_buf = NULL;
    MPIDI_msg_sz_t data_len;
    MPID_Win *win_ptr;
    int acquire_lock_fail = 0;
    int mpi_errno = MPI_SUCCESS;
    MPI_Aint stream_elem_count, total_len;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3_PKTHANDLER_GETACCUMULATE);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3_PKTHANDLER_GETACCUMULATE);

    MPIU_DBG_MSG(CH3_OTHER, VERBOSE, "received accumulate pkt");

    MPIR_T_PVAR_TIMER_START(RMA, rma_rmapkt_get_accum);

    MPIU_Assert(get_accum_pkt->target_win_handle != MPI_WIN_NULL);
    MPID_Win_get_ptr(get_accum_pkt->target_win_handle, win_ptr);

    mpi_errno = check_piggyback_lock(win_ptr, vc, pkt, buflen, &acquire_lock_fail, &req);
    if (mpi_errno != MPI_SUCCESS)
        MPIR_ERR_POP(mpi_errno);

    if (acquire_lock_fail) {
        (*rreqp) = req;
        goto fn_exit;
    }

    if (pkt->type == MPIDI_CH3_PKT_GET_ACCUM_IMMED) {
        size_t len;
        void *src = NULL, *dest = NULL;
        MPID_Request *resp_req = NULL;
        MPIDI_CH3_Pkt_t upkt;
        MPIDI_CH3_Pkt_get_accum_resp_t *get_accum_resp_pkt = &upkt.get_accum_resp;
        MPL_IOV iov[MPL_IOV_LIMIT];
        int iovcnt;
        MPI_Aint type_size;

        *buflen = sizeof(MPIDI_CH3_Pkt_t);
        *rreqp = NULL;

        /* Immed packet type is used when target datatype is predefined datatype. */
        MPIU_Assert(MPIR_DATATYPE_IS_PREDEFINED(get_accum_pkt->datatype));

        resp_req = MPID_Request_create();
        resp_req->dev.target_win_handle = get_accum_pkt->target_win_handle;
        resp_req->dev.flags = get_accum_pkt->flags;

        MPIDI_Request_set_type(resp_req, MPIDI_REQUEST_TYPE_GET_ACCUM_RESP);
        resp_req->dev.OnDataAvail = MPIDI_CH3_ReqHandler_GaccumSendComplete;
        resp_req->dev.OnFinal = MPIDI_CH3_ReqHandler_GaccumSendComplete;
        resp_req->kind = MPID_REQUEST_SEND;

        /* here we increment the Active Target counter to guarantee the GET-like
         * operation are completed when counter reaches zero. */
        win_ptr->at_completion_counter++;

        /* Calculate the length of reponse data, ensure that it fits into immed packet. */
        MPID_Datatype_get_size_macro(get_accum_pkt->datatype, type_size);
        MPIU_Assign_trunc(len, get_accum_pkt->count * type_size, size_t);

        MPIDI_Pkt_init(get_accum_resp_pkt, MPIDI_CH3_PKT_GET_ACCUM_RESP_IMMED);
        get_accum_resp_pkt->request_handle = get_accum_pkt->request_handle;
        get_accum_resp_pkt->target_rank = win_ptr->comm_ptr->rank;
        get_accum_resp_pkt->flags = MPIDI_CH3_PKT_FLAG_NONE;
        if (get_accum_pkt->flags & MPIDI_CH3_PKT_FLAG_RMA_LOCK_SHARED ||
            get_accum_pkt->flags & MPIDI_CH3_PKT_FLAG_RMA_LOCK_EXCLUSIVE)
            get_accum_resp_pkt->flags |= MPIDI_CH3_PKT_FLAG_RMA_LOCK_GRANTED;
        if ((get_accum_pkt->flags & MPIDI_CH3_PKT_FLAG_RMA_FLUSH) ||
            (get_accum_pkt->flags & MPIDI_CH3_PKT_FLAG_RMA_UNLOCK))
            get_accum_resp_pkt->flags |= MPIDI_CH3_PKT_FLAG_RMA_ACK;

        /* NOTE: 'copy data + ACC' needs to be atomic */

        if (win_ptr->shm_allocated == TRUE)
            MPIDI_CH3I_SHM_MUTEX_LOCK(win_ptr);

        /* copy data from target buffer to response packet header */
        src = (void *) (get_accum_pkt->addr), dest = (void *) (get_accum_resp_pkt->info.data);
        mpi_errno = immed_copy(src, dest, len);
        if (mpi_errno != MPI_SUCCESS) {
            if (win_ptr->shm_allocated == TRUE)
                MPIDI_CH3I_SHM_MUTEX_UNLOCK(win_ptr);
            MPIR_ERR_POP(mpi_errno);
        }

        /* perform accumulate operation. */
        mpi_errno =
            do_accumulate_op(get_accum_pkt->info.data, get_accum_pkt->count,
                             get_accum_pkt->datatype, get_accum_pkt->addr, get_accum_pkt->count,
                             get_accum_pkt->datatype, 0, get_accum_pkt->op);

        if (win_ptr->shm_allocated == TRUE)
            MPIDI_CH3I_SHM_MUTEX_UNLOCK(win_ptr);

        if (mpi_errno)
            MPIR_ERR_POP(mpi_errno);

        iov[0].MPL_IOV_BUF = (MPL_IOV_BUF_CAST) get_accum_resp_pkt;
        iov[0].MPL_IOV_LEN = sizeof(*get_accum_resp_pkt);
        iovcnt = 1;

        MPID_THREAD_CS_ENTER(POBJ, vc->pobj_mutex);
        mpi_errno = MPIDI_CH3_iSendv(vc, resp_req, iov, iovcnt);
        MPID_THREAD_CS_EXIT(POBJ, vc->pobj_mutex);
        /* --BEGIN ERROR HANDLING-- */
        if (mpi_errno != MPI_SUCCESS) {
            MPID_Request_release(resp_req);
            MPIR_ERR_SETANDJUMP(mpi_errno, MPI_ERR_OTHER, "**ch3|rmamsg");
        }
        /* --END ERROR HANDLING-- */
    }
    else {
        int is_derived_dt = 0;

        MPIU_Assert(pkt->type == MPIDI_CH3_PKT_GET_ACCUM);

        req = MPID_Request_create();
        MPIU_Object_set_ref(req, 1);
        *rreqp = req;

        req->dev.user_count = get_accum_pkt->count;
        req->dev.op = get_accum_pkt->op;
        req->dev.real_user_buf = get_accum_pkt->addr;
        req->dev.target_win_handle = get_accum_pkt->target_win_handle;
        req->dev.flags = get_accum_pkt->flags;

        req->dev.resp_request_handle = get_accum_pkt->request_handle;

        /* get start location of data and length of data */
        data_len = *buflen - sizeof(MPIDI_CH3_Pkt_t);
        data_buf = (char *) pkt + sizeof(MPIDI_CH3_Pkt_t);

        /* allocate extended header in the request,
         * only including fixed-length variables defined in packet type. */
        is_derived_dt = !MPIR_DATATYPE_IS_PREDEFINED(get_accum_pkt->datatype);
        mpi_errno = MPIDI_CH3_ExtPktHandler_GetAccumulate(req->dev.flags, is_derived_dt,
                                                          &req->dev.ext_hdr_ptr,
                                                          &req->dev.ext_hdr_sz);
        if (mpi_errno != MPI_SUCCESS)
            MPIR_ERR_POP(mpi_errno);

        if (MPIR_DATATYPE_IS_PREDEFINED(get_accum_pkt->datatype)) {
            MPI_Aint type_size;

            MPIDI_Request_set_type(req, MPIDI_REQUEST_TYPE_GET_ACCUM_RECV);
            req->dev.datatype = get_accum_pkt->datatype;

            if (req->dev.flags & MPIDI_CH3_PKT_FLAG_RMA_STREAM) {
                req->dev.OnDataAvail = MPIDI_CH3_ReqHandler_GaccumMetadataRecvComplete;

                /* if this is a streamed op pkt, set iov to receive extended pkt header. */
                req->dev.iov[0].MPL_IOV_BUF = (MPL_IOV_BUF_CAST) req->dev.ext_hdr_ptr;
                req->dev.iov[0].MPL_IOV_LEN = req->dev.ext_hdr_sz;
                req->dev.iov_count = 1;

                *buflen = sizeof(MPIDI_CH3_Pkt_t);
            }
            else {
                int is_empty_origin = FALSE;

                /* Judge if origin data is zero. */
                if (get_accum_pkt->op == MPI_NO_OP)
                    is_empty_origin = TRUE;

                req->dev.OnFinal = MPIDI_CH3_ReqHandler_GaccumRecvComplete;

                if (is_empty_origin == TRUE) {
                    req->dev.recv_data_sz = 0;

                    *buflen = sizeof(MPIDI_CH3_Pkt_t);
                    complete = 1;
                }
                else {
                    MPID_Datatype_get_extent_macro(get_accum_pkt->datatype, extent);

                    MPIU_Assert(!MPIDI_Request_get_srbuf_flag(req));
                    /* allocate a SRBuf for receiving stream unit */
                    MPIDI_CH3U_SRBuf_alloc(req, MPIDI_CH3U_SRBuf_size);
                    /* --BEGIN ERROR HANDLING-- */
                    if (req->dev.tmpbuf_sz == 0) {
                        MPIU_DBG_MSG(CH3_CHANNEL, TYPICAL, "SRBuf allocation failure");
                        mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL,
                                                         FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem",
                                                         "**nomem %d", MPIDI_CH3U_SRBuf_size);
                        req->status.MPI_ERROR = mpi_errno;
                        goto fn_fail;
                    }
                    /* --END ERROR HANDLING-- */

                    req->dev.user_buf = req->dev.tmpbuf;

                    MPID_Datatype_get_size_macro(get_accum_pkt->datatype, type_size);
                    total_len = type_size * get_accum_pkt->count;
                    stream_elem_count = MPIDI_CH3U_SRBuf_size / extent;

                    req->dev.recv_data_sz = MPIR_MIN(total_len, stream_elem_count * type_size);
                    MPIU_Assert(req->dev.recv_data_sz > 0);

                    mpi_errno = MPIDI_CH3U_Receive_data_found(req, data_buf, &data_len, &complete);
                    MPIR_ERR_CHKANDJUMP1(mpi_errno, mpi_errno, MPI_ERR_OTHER, "**ch3|postrecv",
                                         "**ch3|postrecv %s", "MPIDI_CH3_PKT_ACCUMULATE");

                    /* return the number of bytes processed in this function */
                    *buflen = data_len + sizeof(MPIDI_CH3_Pkt_t);
                }

                if (complete) {
                    mpi_errno = MPIDI_CH3_ReqHandler_GaccumRecvComplete(vc, req, &complete);
                    if (mpi_errno)
                        MPIR_ERR_POP(mpi_errno);
                    if (complete) {
                        *rreqp = NULL;
                        goto fn_exit;
                    }
                }
            }
        }
        else {
            MPIDI_Request_set_type(req, MPIDI_REQUEST_TYPE_GET_ACCUM_RECV_DERIVED_DT);
            req->dev.OnDataAvail = MPIDI_CH3_ReqHandler_GaccumMetadataRecvComplete;
            req->dev.datatype = MPI_DATATYPE_NULL;

            /* Put dataloop in a separate buffer to be reused in datatype.
             * It will be freed when free datatype. */
            req->dev.dataloop = MPIU_Malloc(get_accum_pkt->info.dataloop_size);
            if (!req->dev.dataloop) {
                MPIR_ERR_SETANDJUMP1(mpi_errno, MPI_ERR_OTHER, "**nomem", "**nomem %d",
                                     get_accum_pkt->info.dataloop_size);
            }

            if (data_len >= req->dev.ext_hdr_sz + get_accum_pkt->info.dataloop_size) {
                /* Copy extended header */
                MPIU_Memcpy(req->dev.ext_hdr_ptr, data_buf, req->dev.ext_hdr_sz);
                MPIU_Memcpy(req->dev.dataloop, data_buf + req->dev.ext_hdr_sz,
                            get_accum_pkt->info.dataloop_size);

                *buflen = sizeof(MPIDI_CH3_Pkt_t) + req->dev.ext_hdr_sz +
                    get_accum_pkt->info.dataloop_size;

                /* All dtype data has been received, call req handler */
                mpi_errno = MPIDI_CH3_ReqHandler_GaccumMetadataRecvComplete(vc, req, &complete);
                MPIR_ERR_CHKANDJUMP1(mpi_errno, mpi_errno, MPI_ERR_OTHER, "**ch3|postrecv",
                                     "**ch3|postrecv %s", "MPIDI_CH3_ACCUMULATE");
                if (complete) {
                    *rreqp = NULL;
                    goto fn_exit;
                }
            }
            else {
                /* Prepare to receive extended header.
                 * All variable-length data can be received in separate iovs. */
                req->dev.iov[0].MPL_IOV_BUF = (MPL_IOV_BUF_CAST) req->dev.ext_hdr_ptr;
                req->dev.iov[0].MPL_IOV_LEN = req->dev.ext_hdr_sz;
                req->dev.iov[1].MPL_IOV_BUF = (MPL_IOV_BUF_CAST) req->dev.dataloop;
                req->dev.iov[1].MPL_IOV_LEN = get_accum_pkt->info.dataloop_size;
                req->dev.iov_count = 2;

                *buflen = sizeof(MPIDI_CH3_Pkt_t);
            }

        }

        if (mpi_errno != MPI_SUCCESS) {
            MPIR_ERR_SETANDJUMP1(mpi_errno, MPI_ERR_OTHER, "**ch3|postrecv",
                                 "**ch3|postrecv %s", "MPIDI_CH3_PKT_ACCUMULATE");
        }
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
#define FCNAME MPL_QUOTE(FUNCNAME)
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

    mpi_errno = check_piggyback_lock(win_ptr, vc, pkt, buflen, &acquire_lock_fail, &rreq);
    if (mpi_errno != MPI_SUCCESS)
        MPIR_ERR_POP(mpi_errno);
    MPIU_Assert(rreq == NULL);  /* CAS should not have request because all data
                                 * can fit in packet header */

    if (acquire_lock_fail) {
        (*rreqp) = rreq;
        goto fn_exit;
    }

    /* return the number of bytes processed in this function */
    /* data_len == 0 (all within packet) */
    *buflen = sizeof(MPIDI_CH3_Pkt_t);
    *rreqp = NULL;

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
    MPID_THREAD_CS_ENTER(POBJ, vc->pobj_mutex);
    mpi_errno = MPIDI_CH3_iStartMsg(vc, cas_resp_pkt, sizeof(*cas_resp_pkt), &req);
    MPID_THREAD_CS_EXIT(POBJ, vc->pobj_mutex);

    MPIR_ERR_CHKANDJUMP(mpi_errno != MPI_SUCCESS, mpi_errno, MPI_ERR_OTHER, "**ch3|rmamsg");

    if (req != NULL) {
        if (!MPID_Request_is_complete(req)) {
            /* sending process is not completed, set proper OnDataAvail
             * (it is initialized to NULL by lower layer) */
            req->dev.target_win_handle = cas_pkt->target_win_handle;
            req->dev.flags = cas_pkt->flags;
            req->dev.OnDataAvail = MPIDI_CH3_ReqHandler_CASSendComplete;

            /* here we increment the Active Target counter to guarantee the GET-like
             * operation are completed when counter reaches zero. */
            win_ptr->at_completion_counter++;

            MPID_Request_release(req);
            goto fn_exit;
        }
        else
            MPID_Request_release(req);
    }

    mpi_errno = finish_op_on_target(win_ptr, vc, TRUE /* has response data */ ,
                                    cas_pkt->flags, MPI_WIN_NULL);
    if (mpi_errno != MPI_SUCCESS)
        MPIR_ERR_POP(mpi_errno);

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
#define FCNAME MPL_QUOTE(FUNCNAME)
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

    MPID_Request_get_ptr(cas_resp_pkt->request_handle, req);
    MPID_Win_get_ptr(req->dev.source_win_handle, win_ptr);

    /* decrement ack_counter on this target */
    if (cas_resp_pkt->flags & MPIDI_CH3_PKT_FLAG_RMA_LOCK_GRANTED) {
        mpi_errno = handle_lock_ack_with_op(win_ptr, target_rank, cas_resp_pkt->flags);
        if (mpi_errno)
            MPIR_ERR_POP(mpi_errno);

        mpi_errno = handle_lock_ack(win_ptr, target_rank, cas_resp_pkt->flags);
        if (mpi_errno)
            MPIR_ERR_POP(mpi_errno);
    }
    if (cas_resp_pkt->flags & MPIDI_CH3_PKT_FLAG_RMA_ACK) {
        mpi_errno = MPIDI_CH3I_RMA_Handle_ack(win_ptr, target_rank);
        if (mpi_errno != MPI_SUCCESS)
            MPIR_ERR_POP(mpi_errno);
    }

    MPID_Datatype_get_size_macro(req->dev.datatype, len);

    MPIU_Memcpy(req->dev.user_buf, (void *) &cas_resp_pkt->info.data, len);

    mpi_errno = MPID_Request_complete(req);
    if (mpi_errno != MPI_SUCCESS) {
        MPIR_ERR_POP(mpi_errno);
    }

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
#define FCNAME MPL_QUOTE(FUNCNAME)
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
    MPI_Aint type_size;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3_PKTHANDLER_FOP);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3_PKTHANDLER_FOP);

    MPIU_DBG_MSG(CH3_OTHER, VERBOSE, "received FOP pkt");

    MPIR_T_PVAR_TIMER_START(RMA, rma_rmapkt_fop);

    MPID_Win_get_ptr(fop_pkt->target_win_handle, win_ptr);

    mpi_errno = check_piggyback_lock(win_ptr, vc, pkt, buflen, &acquire_lock_fail, &rreq);
    if (mpi_errno != MPI_SUCCESS)
        MPIR_ERR_POP(mpi_errno);

    if (acquire_lock_fail) {
        (*rreqp) = rreq;
        goto fn_exit;
    }

    (*buflen) = sizeof(MPIDI_CH3_Pkt_t);
    (*rreqp) = NULL;

    MPID_Datatype_get_size_macro(fop_pkt->datatype, type_size);

    if (pkt->type == MPIDI_CH3_PKT_FOP_IMMED) {

        MPIDI_Pkt_init(fop_resp_pkt, MPIDI_CH3_PKT_FOP_RESP_IMMED);
        fop_resp_pkt->request_handle = fop_pkt->request_handle;
        fop_resp_pkt->target_rank = win_ptr->comm_ptr->rank;
        fop_resp_pkt->flags = MPIDI_CH3_PKT_FLAG_NONE;
        if (fop_pkt->flags & MPIDI_CH3_PKT_FLAG_RMA_LOCK_SHARED ||
            fop_pkt->flags & MPIDI_CH3_PKT_FLAG_RMA_LOCK_EXCLUSIVE)
            fop_resp_pkt->flags |= MPIDI_CH3_PKT_FLAG_RMA_LOCK_GRANTED;
        if ((fop_pkt->flags & MPIDI_CH3_PKT_FLAG_RMA_FLUSH) ||
            (fop_pkt->flags & MPIDI_CH3_PKT_FLAG_RMA_UNLOCK))
            fop_resp_pkt->flags |= MPIDI_CH3_PKT_FLAG_RMA_ACK;

        /* NOTE: 'copy data + ACC' needs to be atomic */

        if (win_ptr->shm_allocated == TRUE)
            MPIDI_CH3I_SHM_MUTEX_LOCK(win_ptr);

        /* copy data to resp pkt header */
        void *src = fop_pkt->addr, *dest = fop_resp_pkt->info.data;
        mpi_errno = immed_copy(src, dest, type_size);
        if (mpi_errno != MPI_SUCCESS) {
            if (win_ptr->shm_allocated == TRUE)
                MPIDI_CH3I_SHM_MUTEX_UNLOCK(win_ptr);
            MPIR_ERR_POP(mpi_errno);
        }

        /* Apply the op */
        mpi_errno = do_accumulate_op(fop_pkt->info.data, 1, fop_pkt->datatype,
                                     fop_pkt->addr, 1, fop_pkt->datatype, 0, fop_pkt->op);

        if (win_ptr->shm_allocated == TRUE)
            MPIDI_CH3I_SHM_MUTEX_UNLOCK(win_ptr);

        if (mpi_errno != MPI_SUCCESS)
            MPIR_ERR_POP(mpi_errno);

        /* send back the original data */
        MPID_THREAD_CS_ENTER(POBJ, vc->pobj_mutex);
        mpi_errno = MPIDI_CH3_iStartMsg(vc, fop_resp_pkt, sizeof(*fop_resp_pkt), &resp_req);
        MPID_THREAD_CS_EXIT(POBJ, vc->pobj_mutex);
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

        mpi_errno = finish_op_on_target(win_ptr, vc, TRUE /* has response data */ ,
                                        fop_pkt->flags, MPI_WIN_NULL);
        if (mpi_errno != MPI_SUCCESS)
            MPIR_ERR_POP(mpi_errno);
    }
    else {
        MPIU_Assert(pkt->type == MPIDI_CH3_PKT_FOP);

        MPID_Request *req = NULL;
        char *data_buf = NULL;
        MPIDI_msg_sz_t data_len;
        MPI_Aint extent;
        int complete = 0;
        int is_empty_origin = FALSE;

        /* Judge if origin data is zero. */
        if (fop_pkt->op == MPI_NO_OP)
            is_empty_origin = TRUE;

        req = MPID_Request_create();
        MPIU_Object_set_ref(req, 1);
        MPIDI_Request_set_type(req, MPIDI_REQUEST_TYPE_FOP_RECV);
        *rreqp = req;

        req->dev.op = fop_pkt->op;
        req->dev.real_user_buf = fop_pkt->addr;
        req->dev.target_win_handle = fop_pkt->target_win_handle;
        req->dev.flags = fop_pkt->flags;
        req->dev.resp_request_handle = fop_pkt->request_handle;
        req->dev.OnDataAvail = MPIDI_CH3_ReqHandler_FOPRecvComplete;
        req->dev.OnFinal = MPIDI_CH3_ReqHandler_FOPRecvComplete;
        req->dev.datatype = fop_pkt->datatype;
        req->dev.user_count = 1;

        if (is_empty_origin == TRUE) {
            req->dev.recv_data_sz = 0;

            *buflen = sizeof(MPIDI_CH3_Pkt_t);
            complete = 1;
        }
        else {
            /* get start location of data and length of data */
            data_len = *buflen - sizeof(MPIDI_CH3_Pkt_t);
            data_buf = (char *) pkt + sizeof(MPIDI_CH3_Pkt_t);

            MPID_Datatype_get_extent_macro(fop_pkt->datatype, extent);

            req->dev.user_buf = MPIU_Malloc(extent);
            if (!req->dev.user_buf) {
                MPIR_ERR_SETANDJUMP1(mpi_errno, MPI_ERR_OTHER, "**nomem", "**nomem %d", extent);
            }

            req->dev.recv_data_sz = type_size;
            MPIU_Assert(req->dev.recv_data_sz > 0);

            mpi_errno = MPIDI_CH3U_Receive_data_found(req, data_buf, &data_len, &complete);
            MPIR_ERR_CHKANDJUMP1(mpi_errno, mpi_errno, MPI_ERR_OTHER, "**ch3|postrecv",
                                 "**ch3|postrecv %s", "MPIDI_CH3_PKT_ACCUMULATE");

            /* return the number of bytes processed in this function */
            *buflen = data_len + sizeof(MPIDI_CH3_Pkt_t);
        }

        if (complete) {
            mpi_errno = MPIDI_CH3_ReqHandler_FOPRecvComplete(vc, req, &complete);
            if (mpi_errno)
                MPIR_ERR_POP(mpi_errno);
            if (complete) {
                *rreqp = NULL;
                goto fn_exit;
            }
        }
    }

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
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIDI_CH3_PktHandler_FOPResp(MPIDI_VC_t * vc ATTRIBUTE((unused)),
                                 MPIDI_CH3_Pkt_t * pkt,
                                 MPIDI_msg_sz_t * buflen, MPID_Request ** rreqp)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_CH3_Pkt_fop_resp_t *fop_resp_pkt = &pkt->fop_resp;
    MPID_Request *req = NULL;
    MPID_Win *win_ptr = NULL;
    MPI_Aint type_size;
    MPIDI_msg_sz_t data_len;
    char *data_buf = NULL;
    int complete = 0;
    int target_rank = fop_resp_pkt->target_rank;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3_PKTHANDLER_FOPRESP);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3_PKTHANDLER_FOPRESP);

    MPIU_DBG_MSG(CH3_OTHER, VERBOSE, "received FOP response pkt");

    MPIR_T_PVAR_TIMER_START(RMA, rma_rmapkt_fop_resp);

    MPID_Request_get_ptr(fop_resp_pkt->request_handle, req);
    MPID_Win_get_ptr(req->dev.source_win_handle, win_ptr);

    /* decrement ack_counter */
    if (fop_resp_pkt->flags & MPIDI_CH3_PKT_FLAG_RMA_LOCK_GRANTED) {
        mpi_errno = handle_lock_ack_with_op(win_ptr, target_rank, fop_resp_pkt->flags);
        if (mpi_errno)
            MPIR_ERR_POP(mpi_errno);

        mpi_errno = handle_lock_ack(win_ptr, target_rank, fop_resp_pkt->flags);
        if (mpi_errno)
            MPIR_ERR_POP(mpi_errno);
    }
    if (fop_resp_pkt->flags & MPIDI_CH3_PKT_FLAG_RMA_ACK) {
        mpi_errno = MPIDI_CH3I_RMA_Handle_ack(win_ptr, target_rank);
        if (mpi_errno)
            MPIR_ERR_POP(mpi_errno);
    }

    data_len = *buflen - sizeof(MPIDI_CH3_Pkt_t);
    data_buf = (char *) pkt + sizeof(MPIDI_CH3_Pkt_t);

    MPID_Datatype_get_size_macro(req->dev.datatype, type_size);
    req->dev.recv_data_sz = type_size;
    req->dev.user_count = 1;

    *rreqp = req;

    if (fop_resp_pkt->type == MPIDI_CH3_PKT_FOP_RESP_IMMED) {
        MPIU_Memcpy(req->dev.user_buf, fop_resp_pkt->info.data, req->dev.recv_data_sz);

        /* return the number of bytes processed in this function */
        *buflen = sizeof(MPIDI_CH3_Pkt_t);
        complete = 1;
    }
    else {
        MPIU_Assert(fop_resp_pkt->type == MPIDI_CH3_PKT_FOP_RESP);

        mpi_errno = MPIDI_CH3U_Receive_data_found(req, data_buf, &data_len, &complete);
        MPIR_ERR_CHKANDJUMP1(mpi_errno, mpi_errno, MPI_ERR_OTHER, "**ch3|postrecv",
                             "**ch3|postrecv %s", "MPIDI_CH3_PKT_FOP_RESP");

        /* return the number of bytes processed in this function */
        *buflen = data_len + sizeof(MPIDI_CH3_Pkt_t);
    }

    if (complete) {
        mpi_errno = MPID_Request_complete(req);
        if (mpi_errno != MPI_SUCCESS) {
            MPIR_ERR_POP(mpi_errno);
        }
        *rreqp = NULL;
    }

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
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIDI_CH3_PktHandler_Get_AccumResp(MPIDI_VC_t * vc, MPIDI_CH3_Pkt_t * pkt,
                                       MPIDI_msg_sz_t * buflen, MPID_Request ** rreqp)
{
    MPIDI_CH3_Pkt_get_accum_resp_t *get_accum_resp_pkt = &pkt->get_accum_resp;
    MPID_Request *req;
    int complete = 0;
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

    MPID_Request_get_ptr(get_accum_resp_pkt->request_handle, req);
    MPID_Win_get_ptr(req->dev.source_win_handle, win_ptr);

    /* decrement ack_counter on target */
    if (get_accum_resp_pkt->flags & MPIDI_CH3_PKT_FLAG_RMA_LOCK_GRANTED) {
        mpi_errno = handle_lock_ack_with_op(win_ptr, target_rank, get_accum_resp_pkt->flags);
        if (mpi_errno)
            MPIR_ERR_POP(mpi_errno);
        mpi_errno = handle_lock_ack(win_ptr, target_rank, get_accum_resp_pkt->flags);
        if (mpi_errno)
            MPIR_ERR_POP(mpi_errno);
    }
    if (get_accum_resp_pkt->flags & MPIDI_CH3_PKT_FLAG_RMA_ACK) {
        mpi_errno = MPIDI_CH3I_RMA_Handle_ack(win_ptr, target_rank);
        if (mpi_errno)
            MPIR_ERR_POP(mpi_errno);
    }

    data_len = *buflen - sizeof(MPIDI_CH3_Pkt_t);
    data_buf = (char *) pkt + sizeof(MPIDI_CH3_Pkt_t);

    MPID_Datatype_get_size_macro(req->dev.datatype, type_size);

    *rreqp = req;

    if (get_accum_resp_pkt->type == MPIDI_CH3_PKT_GET_ACCUM_RESP_IMMED) {
        req->dev.recv_data_sz = type_size * req->dev.user_count;

        MPIU_Memcpy(req->dev.user_buf, get_accum_resp_pkt->info.data, req->dev.recv_data_sz);
        /* return the number of bytes processed in this function */
        *buflen = sizeof(MPIDI_CH3_Pkt_t);
        complete = 1;
    }
    else {
        MPIU_Assert(pkt->type == MPIDI_CH3_PKT_GET_ACCUM_RESP);

        MPI_Datatype basic_type;
        MPI_Aint basic_type_extent, basic_type_size;
        MPI_Aint stream_elem_count;
        MPI_Aint total_len, rest_len;
        MPI_Aint real_stream_offset;
        MPI_Aint contig_stream_offset = 0;

        if (MPIR_DATATYPE_IS_PREDEFINED(req->dev.datatype)) {
            basic_type = req->dev.datatype;
        }
        else {
            MPIU_Assert(req->dev.datatype_ptr != NULL);
            basic_type = req->dev.datatype_ptr->basic_type;
        }

        MPID_Datatype_get_extent_macro(basic_type, basic_type_extent);
        MPID_Datatype_get_size_macro(basic_type, basic_type_size);

        /* Note: here we get the stream_offset from the extended packet header
         * in the response request, which is set in issue_get_acc_op() funcion.
         * Note that this extended packet header only contains stream_offset and
         * does not contain datatype info, so here we pass 0 to is_derived_dt
         * flag. */
        MPIDI_CH3_ExtPkt_Gaccum_get_stream(req->dev.flags, 0 /* is_derived_dt */ ,
                                           req->dev.ext_hdr_ptr, &contig_stream_offset);

        total_len = type_size * req->dev.user_count;
        rest_len = total_len - contig_stream_offset;
        stream_elem_count = MPIDI_CH3U_SRBuf_size / basic_type_extent;

        req->dev.recv_data_sz = MPIR_MIN(rest_len, stream_elem_count * basic_type_size);
        real_stream_offset = (contig_stream_offset / basic_type_size) * basic_type_extent;

        if (MPIR_DATATYPE_IS_PREDEFINED(req->dev.datatype)) {
            req->dev.user_buf = (void *) ((char *) req->dev.user_buf + real_stream_offset);
            mpi_errno = MPIDI_CH3U_Receive_data_found(req, data_buf, &data_len, &complete);
            MPIR_ERR_CHKANDJUMP1(mpi_errno, mpi_errno, MPI_ERR_OTHER, "**ch3|postrecv",
                                 "**ch3|postrecv %s", "MPIDI_CH3_PKT_GET_ACCUM_RESP");

            /* return the number of bytes processed in this function */
            *buflen = data_len + sizeof(MPIDI_CH3_Pkt_t);
        }
        else {
            *buflen = sizeof(MPIDI_CH3_Pkt_t);

            req->dev.segment_ptr = MPID_Segment_alloc();
            MPID_Segment_init(req->dev.user_buf, req->dev.user_count, req->dev.datatype,
                              req->dev.segment_ptr, 0);
            req->dev.segment_first = contig_stream_offset;
            req->dev.segment_size = contig_stream_offset + req->dev.recv_data_sz;

            mpi_errno = MPIDI_CH3U_Request_load_recv_iov(req);
            if (mpi_errno != MPI_SUCCESS) {
                MPIR_ERR_SETANDJUMP(mpi_errno, MPI_ERR_OTHER, "**ch3|loadrecviov");
            }
            if (req->dev.OnDataAvail == NULL) {
                req->dev.OnDataAvail = req->dev.OnFinal;
            }
        }
    }
    if (complete) {
        mpi_errno = MPID_Request_complete(req);
        if (mpi_errno != MPI_SUCCESS) {
            MPIR_ERR_POP(mpi_errno);
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
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIDI_CH3_PktHandler_Lock(MPIDI_VC_t * vc, MPIDI_CH3_Pkt_t * pkt,
                              MPIDI_msg_sz_t * buflen, MPID_Request ** rreqp)
{
    MPIDI_CH3_Pkt_lock_t *lock_pkt = &pkt->lock;
    MPID_Win *win_ptr = NULL;
    int lock_type;
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3_PKTHANDLER_LOCK);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3_PKTHANDLER_LOCK);

    MPIU_DBG_MSG(CH3_OTHER, VERBOSE, "received lock pkt");

    MPIR_T_PVAR_TIMER_START(RMA, rma_rmapkt_lock);

    *buflen = sizeof(MPIDI_CH3_Pkt_t);

    MPID_Win_get_ptr(lock_pkt->target_win_handle, win_ptr);

    if (lock_pkt->flags & MPIDI_CH3_PKT_FLAG_RMA_LOCK_SHARED)
        lock_type = MPI_LOCK_SHARED;
    else {
        MPIU_Assert(lock_pkt->flags & MPIDI_CH3_PKT_FLAG_RMA_LOCK_EXCLUSIVE);
        lock_type = MPI_LOCK_EXCLUSIVE;
    }

    if (MPIDI_CH3I_Try_acquire_win_lock(win_ptr, lock_type) == 1) {
        /* send lock granted packet. */
        mpi_errno = MPIDI_CH3I_Send_lock_ack_pkt(vc, win_ptr, MPIDI_CH3_PKT_FLAG_RMA_LOCK_GRANTED,
                                                 lock_pkt->source_win_handle,
                                                 lock_pkt->request_handle);
        if (mpi_errno != MPI_SUCCESS)
            MPIR_ERR_POP(mpi_errno);
    }

    else {
        MPID_Request *req = NULL;
        mpi_errno = enqueue_lock_origin(win_ptr, vc, pkt, buflen, &req);
        if (mpi_errno != MPI_SUCCESS)
            MPIR_ERR_POP(mpi_errno);
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
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIDI_CH3_PktHandler_GetResp(MPIDI_VC_t * vc ATTRIBUTE((unused)),
                                 MPIDI_CH3_Pkt_t * pkt,
                                 MPIDI_msg_sz_t * buflen, MPID_Request ** rreqp)
{
    MPIDI_CH3_Pkt_get_resp_t *get_resp_pkt = &pkt->get_resp;
    MPID_Request *req;
    int complete = 0;
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

    MPID_Request_get_ptr(get_resp_pkt->request_handle, req);
    MPID_Win_get_ptr(req->dev.source_win_handle, win_ptr);

    /* decrement ack_counter on target */
    if (get_resp_pkt->flags & MPIDI_CH3_PKT_FLAG_RMA_LOCK_GRANTED) {
        mpi_errno = handle_lock_ack_with_op(win_ptr, target_rank, get_resp_pkt->flags);
        if (mpi_errno)
            MPIR_ERR_POP(mpi_errno);

        mpi_errno = handle_lock_ack(win_ptr, target_rank, get_resp_pkt->flags);
        if (mpi_errno)
            MPIR_ERR_POP(mpi_errno);
    }
    if (get_resp_pkt->flags & MPIDI_CH3_PKT_FLAG_RMA_ACK) {
        mpi_errno = MPIDI_CH3I_RMA_Handle_ack(win_ptr, target_rank);
        if (mpi_errno)
            MPIR_ERR_POP(mpi_errno);
    }

    data_len = *buflen - sizeof(MPIDI_CH3_Pkt_t);
    data_buf = (char *) pkt + sizeof(MPIDI_CH3_Pkt_t);

    MPID_Datatype_get_size_macro(req->dev.datatype, type_size);
    req->dev.recv_data_sz = type_size * req->dev.user_count;

    *rreqp = req;

    if (get_resp_pkt->type == MPIDI_CH3_PKT_GET_RESP_IMMED) {
        MPIU_Memcpy(req->dev.user_buf, get_resp_pkt->info.data, req->dev.recv_data_sz);

        /* return the number of bytes processed in this function */
        *buflen = sizeof(MPIDI_CH3_Pkt_t);
        complete = 1;
    }
    else {
        MPIU_Assert(get_resp_pkt->type == MPIDI_CH3_PKT_GET_RESP);

        mpi_errno = MPIDI_CH3U_Receive_data_found(req, data_buf, &data_len, &complete);
        MPIR_ERR_CHKANDJUMP1(mpi_errno, mpi_errno, MPI_ERR_OTHER, "**ch3|postrecv",
                             "**ch3|postrecv %s", "MPIDI_CH3_PKT_GET_RESP");

        /* return the number of bytes processed in this function */
        *buflen = data_len + sizeof(MPIDI_CH3_Pkt_t);
    }

    if (complete) {
        mpi_errno = MPID_Request_complete(req);
        if (mpi_errno != MPI_SUCCESS) {
            MPIR_ERR_POP(mpi_errno);
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
#define FCNAME MPL_QUOTE(FUNCNAME)
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

    if (lock_ack_pkt->source_win_handle != MPI_WIN_NULL) {
        MPID_Win_get_ptr(lock_ack_pkt->source_win_handle, win_ptr);
    }
    else {
        MPIU_Assert(lock_ack_pkt->request_handle != MPI_REQUEST_NULL);

        MPID_Request *req_ptr = NULL;
        MPID_Request_get_ptr(lock_ack_pkt->request_handle, req_ptr);
        MPIU_Assert(req_ptr->dev.source_win_handle != MPI_REQUEST_NULL);
        MPID_Win_get_ptr(req_ptr->dev.source_win_handle, win_ptr);
    }

    mpi_errno = handle_lock_ack(win_ptr, target_rank, lock_ack_pkt->flags);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

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
#define FCNAME MPL_QUOTE(FUNCNAME)
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

    if (lock_op_ack_pkt->source_win_handle != MPI_WIN_NULL) {
        MPID_Win_get_ptr(lock_op_ack_pkt->source_win_handle, win_ptr);
    }
    else {
        MPIU_Assert(lock_op_ack_pkt->request_handle != MPI_REQUEST_NULL);

        MPID_Request *req_ptr = NULL;
        MPID_Request_get_ptr(lock_op_ack_pkt->request_handle, req_ptr);
        MPIU_Assert(req_ptr->dev.source_win_handle != MPI_REQUEST_NULL);
        MPID_Win_get_ptr(req_ptr->dev.source_win_handle, win_ptr);
    }

    mpi_errno = handle_lock_ack_with_op(win_ptr, target_rank, lock_op_ack_pkt->flags);
    if (mpi_errno != MPI_SUCCESS)
        MPIR_ERR_POP(mpi_errno);

    mpi_errno = handle_lock_ack(win_ptr, target_rank, lock_op_ack_pkt->flags);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

    if (flags & MPIDI_CH3_PKT_FLAG_RMA_ACK) {
        MPIU_Assert(flags & MPIDI_CH3_PKT_FLAG_RMA_LOCK_GRANTED);
        mpi_errno = MPIDI_CH3I_RMA_Handle_ack(win_ptr, target_rank);
        if (mpi_errno != MPI_SUCCESS)
            MPIR_ERR_POP(mpi_errno);
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
#define FUNCNAME MPIDI_CH3_PktHandler_Ack
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIDI_CH3_PktHandler_Ack(MPIDI_VC_t * vc, MPIDI_CH3_Pkt_t * pkt,
                             MPIDI_msg_sz_t * buflen, MPID_Request ** rreqp)
{
    MPIDI_CH3_Pkt_ack_t *ack_pkt = &pkt->ack;
    MPID_Win *win_ptr = NULL;
    int mpi_errno = MPI_SUCCESS;
    int target_rank = ack_pkt->target_rank;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3_PKTHANDLER_ACK);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3_PKTHANDLER_ACK);

    MPIU_DBG_MSG(CH3_OTHER, VERBOSE, "received shared lock ops done pkt");

    MPIR_T_PVAR_TIMER_START(RMA, rma_rmapkt_ack);

    *buflen = sizeof(MPIDI_CH3_Pkt_t);

    MPID_Win_get_ptr(ack_pkt->source_win_handle, win_ptr);

    /* decrement ack_counter on target */
    mpi_errno = MPIDI_CH3I_RMA_Handle_ack(win_ptr, target_rank);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

    *rreqp = NULL;
    MPIDI_CH3_Progress_signal_completion();

    MPIR_T_PVAR_TIMER_END(RMA, rma_rmapkt_ack);
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3_PKTHANDLER_ACK);
  fn_exit:
    return MPI_SUCCESS;
  fn_fail:
    goto fn_exit;
}


#undef FUNCNAME
#define FUNCNAME MPIDI_CH3_PktHandler_DecrAtCnt
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
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

    if (decr_at_cnt_pkt->flags & MPIDI_CH3_PKT_FLAG_RMA_FLUSH) {
        mpi_errno = MPIDI_CH3I_Send_ack_pkt(vc, win_ptr, decr_at_cnt_pkt->source_win_handle);
        if (mpi_errno)
            MPIR_ERR_POP(mpi_errno);
    }

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
#define FCNAME MPL_QUOTE(FUNCNAME)
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
    MPIR_ERR_CHKANDJUMP(mpi_errno != MPI_SUCCESS, mpi_errno, MPI_ERR_OTHER, "**ch3|rma_msg");

    if (!(unlock_pkt->flags & MPIDI_CH3_PKT_FLAG_RMA_UNLOCK_NO_ACK)) {
        mpi_errno = MPIDI_CH3I_Send_ack_pkt(vc, win_ptr, unlock_pkt->source_win_handle);
        if (mpi_errno)
            MPIR_ERR_POP(mpi_errno);
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
#define FCNAME MPL_QUOTE(FUNCNAME)
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

    mpi_errno = MPIDI_CH3I_Send_ack_pkt(vc, win_ptr, flush_pkt->source_win_handle);
    if (mpi_errno != MPI_SUCCESS)
        MPIR_ERR_POP(mpi_errno);

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
    MPIU_DBG_PRINTF((" dataloop_size. 0x%08X\n", pkt->put.info.dataloop_size));
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
    MPIU_DBG_PRINTF((" dataloop_size. %d\n", pkt->get.info.dataloop_size));
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
    MPIU_DBG_PRINTF((" dataloop_size. %d\n", pkt->accum.info.dataloop_size));
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

int MPIDI_CH3_PktPrint_Ack(FILE * fp, MPIDI_CH3_Pkt_t * pkt)
{
    MPIU_DBG_PRINTF((" type ......... MPIDI_CH3_PKT_ACK\n"));
    MPIU_DBG_PRINTF((" source ....... 0x%08X\n", pkt->ack.source_win_handle));
    return MPI_SUCCESS;
}

int MPIDI_CH3_PktPrint_LockAck(FILE * fp, MPIDI_CH3_Pkt_t * pkt)
{
    MPIU_DBG_PRINTF((" type ......... MPIDI_CH3_PKT_LOCK_ACK\n"));
    MPIU_DBG_PRINTF((" source ....... 0x%08X\n", pkt->lock_ack.source_win_handle));
    return MPI_SUCCESS;
}
#endif
