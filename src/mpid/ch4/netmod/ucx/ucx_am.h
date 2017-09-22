/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2016 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 *  Portions of this code were written by Mellanox Technologies Ltd.
 *  Copyright (C) Mellanox Technologies Ltd. 2016. ALL RIGHTS RESERVED
 */
#ifndef UCX_AM_H_INCLUDED
#define UCX_AM_H_INCLUDED

#include "ucx_impl.h"

static inline void MPIDI_UCX_am_isend_callback(void *request, ucs_status_t status)
{
    MPIDI_UCX_ucp_request_t *ucp_request = (MPIDI_UCX_ucp_request_t *) request;
    MPIR_Request *req = ucp_request->req;
    int handler_id = req->dev.ch4.am.netmod_am.ucx.handler_id;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_UCX_AM_ISEND_CALLBACK);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_UCX_AM_ISEND_CALLBACK);

    MPL_free(req->dev.ch4.am.netmod_am.ucx.pack_buffer);
    req->dev.ch4.am.netmod_am.ucx.pack_buffer = NULL;
    MPIDIG_global.origin_cbs[handler_id] (req);
    ucp_request->req = NULL;

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_UCX_AM_ISEND_CALLBACK);
    return;
  fn_fail:
    goto fn_exit;
}

static inline void MPIDI_UCX_am_send_callback(void *request, ucs_status_t status)
{
    MPIDI_UCX_ucp_request_t *ucp_request = (MPIDI_UCX_ucp_request_t *) request;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_UCX_AM_SEND_CALLBACK);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_UCX_AM_SEND_CALLBACK);

    MPL_free(ucp_request->req);
    ucp_request->req = NULL;
    ucp_request_release(ucp_request);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_UCX_AM_SEND_CALLBACK);
    return;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_NM_am_isend
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)

static inline int MPIDI_NM_am_isend(int rank,
                                    MPIR_Comm * comm,
                                    int handler_id,
                                    const void *am_hdr,
                                    size_t am_hdr_sz,
                                    const void *data,
                                    MPI_Count count,
                                    MPI_Datatype datatype, MPIR_Request * sreq)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_UCX_ucp_request_t *ucp_request;
    ucp_ep_h ep;
    uint64_t ucx_tag;
    char *send_buf;
    size_t data_sz;
    MPI_Aint dt_true_lb, last;
    MPIR_Datatype *dt_ptr;
    int dt_contig;
    MPIDI_UCX_am_header_t ucx_hdr;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_AM_ISEND);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_AM_ISEND);

    MPIDI_Datatype_get_info(count, datatype, dt_contig, data_sz, dt_ptr, dt_true_lb);
    if (handler_id == MPIDI_CH4U_SEND &&
        am_hdr_sz + sizeof(MPIDI_UCX_am_header_t) + data_sz > MPIDI_UCX_MAX_AM_EAGER_SZ) {
        MPIDI_CH4U_send_long_req_msg_t lreq_hdr;

        MPIR_Memcpy(&lreq_hdr.hdr, am_hdr, am_hdr_sz);
        lreq_hdr.data_sz = data_sz;
        lreq_hdr.sreq_ptr = (uint64_t) sreq;
        MPIDI_CH4U_REQUEST(sreq, req->lreq).src_buf = data;
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

    ep = MPIDI_UCX_COMM_TO_EP(comm, rank);
    ucx_tag = MPIDI_UCX_init_tag(0, MPIR_Process.comm_world->rank, MPIDI_UCX_AM_TAG);

    /* initialize our portion of the hdr */
    ucx_hdr.handler_id = handler_id;
    ucx_hdr.data_sz = data_sz;

    if (dt_contig) {
        /* just pack and send for now */
        send_buf = MPL_malloc(data_sz + am_hdr_sz + sizeof(ucx_hdr));
        MPIR_Memcpy(send_buf, &ucx_hdr, sizeof(ucx_hdr));
        MPIR_Memcpy(send_buf + sizeof(ucx_hdr), am_hdr, am_hdr_sz);
        MPIR_Memcpy(send_buf + am_hdr_sz + sizeof(ucx_hdr), (char *) data + dt_true_lb, data_sz);
    }
    else {
        size_t segment_first;
        struct MPIR_Segment *segment_ptr;
        segment_ptr = MPIR_Segment_alloc();
        MPIR_ERR_CHKANDJUMP1(segment_ptr == NULL, mpi_errno,
                             MPI_ERR_OTHER, "**nomem", "**nomem %s", "Send MPIR_Segment_alloc");
        MPIR_Segment_init(data, count, datatype, segment_ptr, 0);
        segment_first = 0;
        last = data_sz;
        send_buf = MPL_malloc(data_sz + am_hdr_sz + sizeof(ucx_hdr));

        MPIR_Memcpy(send_buf, &ucx_hdr, sizeof(ucx_hdr));
        MPIR_Memcpy(send_buf + sizeof(ucx_hdr), am_hdr, am_hdr_sz);
        MPIR_Segment_pack(segment_ptr, segment_first, &last,
                           send_buf + am_hdr_sz + sizeof(ucx_hdr));
        MPIR_Segment_free(segment_ptr);
    }

    ucp_request = (MPIDI_UCX_ucp_request_t *) ucp_tag_send_nb(ep, send_buf,
                                                              data_sz + am_hdr_sz + sizeof(ucx_hdr),
                                                              ucp_dt_make_contig(1), ucx_tag,
                                                              &MPIDI_UCX_am_isend_callback);
    MPIDI_CH4_UCX_REQUEST(ucp_request);

    /* send is done. free all resources and complete the request */
    if (ucp_request == NULL) {
        MPL_free(send_buf);
        MPIDIG_global.origin_cbs[handler_id] (sreq);
        goto fn_exit;
    }

    /* set the ch4r request inside the UCP request */
    sreq->dev.ch4.am.netmod_am.ucx.pack_buffer = send_buf;
    sreq->dev.ch4.am.netmod_am.ucx.handler_id = handler_id;
    ucp_request->req = sreq;
    ucp_request_release(ucp_request);


  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_AM_ISEND);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_NM_am_isendv
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_NM_am_isendv(int rank,
                                     MPIR_Comm * comm,
                                     int handler_id,
                                     struct iovec *am_hdr,
                                     size_t iov_len,
                                     const void *data,
                                     MPI_Count count,
                                     MPI_Datatype datatype,
                                     MPIR_Request * sreq)
{
    int mpi_errno = MPI_SUCCESS;
    size_t am_hdr_sz = 0, i;
    MPIDI_UCX_ucp_request_t *ucp_request;
    ucp_ep_h ep;
    uint64_t ucx_tag;
    char *send_buf;
    size_t data_sz;
    MPI_Aint dt_true_lb, last;
    MPIR_Datatype *dt_ptr;
    int dt_contig;
    MPIDI_UCX_am_header_t ucx_hdr;


    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_AM_ISENDV);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_AM_ISENDV);

    ep = MPIDI_UCX_COMM_TO_EP(comm, rank);
    ucx_tag = MPIDI_UCX_init_tag(0, MPIR_Process.comm_world->rank, MPIDI_UCX_AM_TAG);

    MPIDI_Datatype_get_info(count, datatype, dt_contig, data_sz, dt_ptr, dt_true_lb);
    for (i = 0; i < iov_len; i++) {
        am_hdr_sz += am_hdr[i].iov_len;
    }

    /* copy headers to send buffer */
    send_buf = MPL_malloc(data_sz + am_hdr_sz + sizeof(ucx_hdr));
    ucx_hdr.handler_id = handler_id;
    ucx_hdr.data_sz = data_sz;
    MPIR_Memcpy(send_buf, &ucx_hdr, sizeof(ucx_hdr));
    am_hdr_sz = 0;
    for (i = 0; i < iov_len; i++) {
        MPIR_Memcpy(send_buf + am_hdr_sz + sizeof(ucx_hdr), am_hdr[i].iov_base, am_hdr[i].iov_len);
        am_hdr_sz += am_hdr[i].iov_len;
    }

    if (dt_contig) {
        MPIR_Memcpy(send_buf + am_hdr_sz + sizeof(ucx_hdr), (char *) data + dt_true_lb, data_sz);
    } else {
        size_t segment_first;
        struct MPIR_Segment *segment_ptr;
        segment_ptr = MPIR_Segment_alloc();
        MPIR_ERR_CHKANDJUMP1(segment_ptr == NULL, mpi_errno,
                             MPI_ERR_OTHER, "**nomem", "**nomem %s", "MPIR_Segment_alloc");
        MPIR_Segment_init(data, count, datatype, segment_ptr, 0);
        segment_first = 0;
        last = data_sz;
        MPIR_Segment_pack(segment_ptr, segment_first, &last,
                           send_buf + sizeof(ucx_hdr) + am_hdr_sz);
        MPIR_Segment_free(segment_ptr);
    }
    ucp_request = (MPIDI_UCX_ucp_request_t *) ucp_tag_send_nb(ep, send_buf,
                                                              data_sz + am_hdr_sz + sizeof(ucx_hdr),
                                                              ucp_dt_make_contig(1), ucx_tag,
                                                              &MPIDI_UCX_am_isend_callback);
    MPIDI_CH4_UCX_REQUEST(ucp_request);

    /* send is done. free all resources and complete the request */
    if (ucp_request == NULL) {
        MPL_free(send_buf);
        MPIDIG_global.origin_cbs[handler_id] (sreq);
        goto fn_exit;
    }

    /* set the ch4r request inside the UCP request */
    sreq->dev.ch4.am.netmod_am.ucx.pack_buffer = send_buf;
    sreq->dev.ch4.am.netmod_am.ucx.handler_id = handler_id;
    ucp_request->req = sreq;
    ucp_request_release(ucp_request);

fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_AM_ISENDV);
    return mpi_errno;

fn_fail:
    goto fn_exit;

}


#undef FUNCNAME
#define FUNCNAME MPIDI_NM_am_isend_reply
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_NM_am_isend_reply(MPIR_Context_id_t context_id,
                                          int src_rank,
                                          int handler_id,
                                          const void *am_hdr,
                                          size_t am_hdr_sz,
                                          const void *data, MPI_Count count,
                                          MPI_Datatype datatype, MPIR_Request * sreq)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_UCX_ucp_request_t *ucp_request;
    ucp_ep_h ep;
    uint64_t ucx_tag;
    char *send_buf;
    size_t data_sz;
    MPI_Aint dt_true_lb;
    MPIR_Datatype *dt_ptr;
    int dt_contig;
    MPIDI_UCX_am_header_t ucx_hdr;
    MPIR_Comm *use_comm;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_AM_ISEND_REPLY);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_AM_ISEND_REPLY);

    use_comm = MPIDI_CH4U_context_id_to_comm(context_id);
    ep = MPIDI_UCX_COMM_TO_EP(use_comm, src_rank);
    ucx_tag = MPIDI_UCX_init_tag(0, MPIR_Process.comm_world->rank, MPIDI_UCX_AM_TAG);

    MPIDI_Datatype_get_info(count, datatype, dt_contig, data_sz, dt_ptr, dt_true_lb);

    /* initialize our portion of the hdr */
    ucx_hdr.handler_id = handler_id;
    ucx_hdr.data_sz = data_sz;

    /* just pack and send for now */
    send_buf = MPL_malloc(data_sz + am_hdr_sz + sizeof(ucx_hdr));
    MPIR_Memcpy(send_buf, &ucx_hdr, sizeof(ucx_hdr));
    MPIR_Memcpy(send_buf + sizeof(ucx_hdr), am_hdr, am_hdr_sz);
    if (dt_contig) {
        MPIR_Memcpy(send_buf + am_hdr_sz + sizeof(ucx_hdr), (char *) data + dt_true_lb, data_sz);
    } else {
        size_t segment_first;
        struct MPIR_Segment *segment_ptr;
        MPI_Aint last;
        segment_ptr = MPIR_Segment_alloc();
        MPIR_ERR_CHKANDJUMP1(segment_ptr == NULL, mpi_errno,
                             MPI_ERR_OTHER, "**nomem", "**nomem %s", "MPIR_Segment_alloc");
        MPIR_Segment_init(data, count, datatype, segment_ptr, 0);
        segment_first = 0;
        last = data_sz;
        MPIR_Segment_pack(segment_ptr, segment_first, &last,
                           send_buf + am_hdr_sz + sizeof(ucx_hdr));
        MPIR_Segment_free(segment_ptr);
    }
    ucp_request = (MPIDI_UCX_ucp_request_t *) ucp_tag_send_nb(ep, send_buf,
                                                              data_sz + am_hdr_sz +
                                                              sizeof(ucx_hdr),
                                                              ucp_dt_make_contig(1), ucx_tag,
                                                              &MPIDI_UCX_am_isend_callback);
    MPIDI_CH4_UCX_REQUEST(ucp_request);

    /* send is done. free all resources and complete the request */
    if (ucp_request == NULL) {
        MPL_free(send_buf);
        MPIDIG_global.origin_cbs[handler_id] (sreq);
        goto fn_exit;
    }

    /* set the ch4r request inside the UCP request */
    sreq->dev.ch4.am.netmod_am.ucx.pack_buffer = send_buf;
    sreq->dev.ch4.am.netmod_am.ucx.handler_id = handler_id;
    ucp_request->req = sreq;
    ucp_request_release(ucp_request);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_AM_ISEND_REPLY);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static inline size_t MPIDI_NM_am_hdr_max_sz(void)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_AM_HDR_MAX_SZ);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_AM_HDR_MAX_SZ);

    ret = (MPIDI_UCX_MAX_AM_EAGER_SZ - sizeof(MPIDI_UCX_am_header_t));

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_AM_HDR_MAX_SZ);
    return ret;
}

static inline int MPIDI_NM_am_send_hdr(int rank,
                                       MPIR_Comm * comm,
                                       int handler_id,
                                       const void *am_hdr, size_t am_hdr_sz)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_UCX_ucp_request_t *ucp_request;
    ucp_ep_h ep;
    uint64_t ucx_tag;
    char *send_buf;
    MPIDI_UCX_am_header_t ucx_hdr;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_AM_SEND_HDR);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_AM_SEND_HDR);

    ep = MPIDI_UCX_COMM_TO_EP(comm, rank);
    ucx_tag = MPIDI_UCX_init_tag(0, MPIR_Process.comm_world->rank, MPIDI_UCX_AM_TAG);

    /* initialize our portion of the hdr */
    ucx_hdr.handler_id = handler_id;
    ucx_hdr.data_sz = 0;

    /* just pack and send for now */
    send_buf = MPL_malloc(am_hdr_sz + sizeof(ucx_hdr));
    MPIR_Memcpy(send_buf, &ucx_hdr, sizeof(ucx_hdr));
    MPIR_Memcpy(send_buf + sizeof(ucx_hdr), am_hdr, am_hdr_sz);

    ucp_request = (MPIDI_UCX_ucp_request_t *) ucp_tag_send_nb(ep, send_buf,
                                                              am_hdr_sz + sizeof(ucx_hdr),
                                                              ucp_dt_make_contig(1), ucx_tag,
                                                              &MPIDI_UCX_am_send_callback);
    MPIDI_CH4_UCX_REQUEST(ucp_request);

    if (ucp_request == NULL) {
        /* inject is done */
        MPL_free(send_buf);
    }
    else {
        ucp_request->req = send_buf;
    }

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_AM_SEND_HDR);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static inline int MPIDI_NM_am_send_hdr_reply(MPIR_Context_id_t context_id,
                                             int src_rank,
                                             int handler_id, const void *am_hdr, size_t am_hdr_sz)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_UCX_ucp_request_t *ucp_request;
    ucp_ep_h ep;
    uint64_t ucx_tag;
    char *send_buf;
    MPIDI_UCX_am_header_t ucx_hdr;
    MPIR_Comm *use_comm;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_AM_SEND_HDR_REPLY);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_AM_SEND_HDR_REPLY);

    use_comm = MPIDI_CH4U_context_id_to_comm(context_id);
    ep = MPIDI_UCX_COMM_TO_EP(use_comm, src_rank);
    ucx_tag = MPIDI_UCX_init_tag(0, MPIR_Process.comm_world->rank, MPIDI_UCX_AM_TAG);

    /* initialize our portion of the hdr */
    ucx_hdr.handler_id = handler_id;

    /* just pack and send for now */
    send_buf = MPL_malloc(am_hdr_sz + sizeof(ucx_hdr));
    MPIR_Memcpy(send_buf, &ucx_hdr, sizeof(ucx_hdr));
    MPIR_Memcpy(send_buf + sizeof(ucx_hdr), am_hdr, am_hdr_sz);
    ucp_request = (MPIDI_UCX_ucp_request_t *) ucp_tag_send_nb(ep, send_buf,
                                                              am_hdr_sz + sizeof(ucx_hdr),
                                                              ucp_dt_make_contig(1), ucx_tag,
                                                              &MPIDI_UCX_am_send_callback);
    MPIDI_CH4_UCX_REQUEST(ucp_request);

    if (ucp_request == NULL) {
        /* inject is done */
        MPL_free(send_buf);
    }
    else {
        ucp_request->req = send_buf;
    }

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_AM_SEND_HDR_REPLY);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static inline int MPIDI_NM_am_recv(MPIR_Request * req)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_CH4U_send_long_ack_msg_t msg;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_AM_RECV);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_AM_RECV);

    msg.sreq_ptr = (MPIDI_CH4U_REQUEST(req, req->rreq.peer_req_ptr));
    msg.rreq_ptr = (uint64_t) req;
    MPIR_Assert((void *) msg.sreq_ptr != NULL);
    mpi_errno = MPIDI_NM_am_send_hdr_reply(MPIDI_CH4U_get_context(MPIDI_CH4U_REQUEST(req, tag)),
                                           MPIDI_CH4U_REQUEST(req, rank),
                                           MPIDI_CH4U_SEND_LONG_ACK, &msg, sizeof(msg));
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_AM_RECV);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}


#endif /* UCX_AM_H_INCLUDED */
