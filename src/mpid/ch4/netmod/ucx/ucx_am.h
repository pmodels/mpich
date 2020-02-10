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

MPL_STATIC_INLINE_PREFIX void MPIDI_UCX_am_isend_callback(void *request, ucs_status_t status)
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

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_UCX_AM_ISEND_CALLBACK);
}

MPL_STATIC_INLINE_PREFIX void MPIDI_UCX_am_send_callback(void *request, ucs_status_t status)
{
    MPIDI_UCX_ucp_request_t *ucp_request = (MPIDI_UCX_ucp_request_t *) request;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_UCX_AM_SEND_CALLBACK);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_UCX_AM_SEND_CALLBACK);

    MPL_free(ucp_request->buf);
    ucp_request->buf = NULL;
    ucp_request_release(ucp_request);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_UCX_AM_SEND_CALLBACK);
}


MPL_STATIC_INLINE_PREFIX int MPIDI_NM_am_isend(int rank, MPIR_Comm * comm, int handler_id,
                                               const void *am_hdr, size_t am_hdr_sz,
                                               const void *data, MPI_Count count,
                                               MPI_Datatype datatype, MPIR_Request * sreq,
                                               MPIDI_av_entry_t * av)
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

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_AM_ISEND);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_AM_ISEND);

    MPIDI_Datatype_get_info(count, datatype, dt_contig, data_sz, dt_ptr, dt_true_lb);
    if (handler_id == MPIDIG_SEND &&
        am_hdr_sz + sizeof(MPIDI_UCX_am_header_t) + data_sz > MPIDI_UCX_MAX_AM_EAGER_SZ) {
        MPIDIG_send_long_req_msg_t lreq_hdr;

        MPIR_Memcpy(&lreq_hdr.hdr, am_hdr, am_hdr_sz);
        lreq_hdr.data_sz = data_sz;
        lreq_hdr.sreq_ptr = sreq;
        MPIDIG_REQUEST(sreq, req->lreq).src_buf = data;
        MPIDIG_REQUEST(sreq, req->lreq).count = count;
        MPIR_Datatype_add_ref_if_not_builtin(datatype);
        MPIDIG_REQUEST(sreq, req->lreq).datatype = datatype;
        MPIDIG_REQUEST(sreq, req->lreq).tag = lreq_hdr.hdr.tag;
        MPIDIG_REQUEST(sreq, req->lreq).rank = lreq_hdr.hdr.src_rank;
        MPIDIG_REQUEST(sreq, req->lreq).context_id = lreq_hdr.hdr.context_id;
        MPIDIG_REQUEST(sreq, rank) = rank;
        mpi_errno = MPIDI_NM_am_send_hdr(rank, comm, MPIDIG_SEND_LONG_REQ,
                                         &lreq_hdr, sizeof(lreq_hdr), av);
        MPIR_ERR_CHECK(mpi_errno);
        goto fn_exit;
    }

    ep = MPIDI_UCX_AV_TO_EP(av);
    ucx_tag = MPIDI_UCX_init_tag(0, MPIR_Process.comm_world->rank, MPIDI_UCX_AM_TAG);

    /* initialize our portion of the hdr */
    ucx_hdr.handler_id = handler_id;
    ucx_hdr.data_sz = data_sz;

    if (dt_contig) {
        /* just pack and send for now */
        send_buf = MPL_malloc(data_sz + am_hdr_sz + sizeof(ucx_hdr), MPL_MEM_BUFFER);
        MPIR_Memcpy(send_buf, &ucx_hdr, sizeof(ucx_hdr));
        MPIR_Memcpy(send_buf + sizeof(ucx_hdr), am_hdr, am_hdr_sz);
        MPIR_Memcpy(send_buf + am_hdr_sz + sizeof(ucx_hdr), (char *) data + dt_true_lb, data_sz);
    } else {
        send_buf = MPL_malloc(data_sz + am_hdr_sz + sizeof(ucx_hdr), MPL_MEM_BUFFER);

        MPIR_Memcpy(send_buf, &ucx_hdr, sizeof(ucx_hdr));
        MPIR_Memcpy(send_buf + sizeof(ucx_hdr), am_hdr, am_hdr_sz);

        MPI_Aint actual_pack_bytes;
        mpi_errno =
            MPIR_Typerep_pack(data, count, datatype, 0, send_buf + am_hdr_sz + sizeof(ucx_hdr),
                              data_sz, &actual_pack_bytes);
        MPIR_ERR_CHECK(mpi_errno);
        MPIR_Assert(actual_pack_bytes == data_sz);
    }

    ucp_request = (MPIDI_UCX_ucp_request_t *) ucp_tag_send_nb(ep, send_buf,
                                                              data_sz + am_hdr_sz + sizeof(ucx_hdr),
                                                              ucp_dt_make_contig(1), ucx_tag,
                                                              &MPIDI_UCX_am_isend_callback);
    MPIDI_UCX_CHK_REQUEST(ucp_request);

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

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_am_isendv(int rank, MPIR_Comm * comm, int handler_id,
                                                struct iovec *am_hdr, size_t iov_len,
                                                const void *data, MPI_Count count,
                                                MPI_Datatype datatype, MPIR_Request * sreq,
                                                MPIDI_av_entry_t * av)
{
    int mpi_errno = MPI_SUCCESS;
    size_t am_hdr_sz = 0, i;
    MPIDI_UCX_ucp_request_t *ucp_request;
    ucp_ep_h ep;
    uint64_t ucx_tag;
    char *send_buf;
    size_t data_sz;
    MPI_Aint dt_true_lb;
    MPIR_Datatype *dt_ptr;
    int dt_contig;
    MPIDI_UCX_am_header_t ucx_hdr;


    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_AM_ISENDV);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_AM_ISENDV);

    ep = MPIDI_UCX_AV_TO_EP(av);
    ucx_tag = MPIDI_UCX_init_tag(0, MPIR_Process.comm_world->rank, MPIDI_UCX_AM_TAG);

    MPIDI_Datatype_get_info(count, datatype, dt_contig, data_sz, dt_ptr, dt_true_lb);
    for (i = 0; i < iov_len; i++) {
        am_hdr_sz += am_hdr[i].iov_len;
    }

    /* copy headers to send buffer */
    send_buf = MPL_malloc(data_sz + am_hdr_sz + sizeof(ucx_hdr), MPL_MEM_BUFFER);
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
        MPI_Aint actual_pack_bytes;
        mpi_errno =
            MPIR_Typerep_pack(data, count, datatype, 0, send_buf + sizeof(ucx_hdr) + am_hdr_sz,
                              data_sz, &actual_pack_bytes);
        MPIR_ERR_CHECK(mpi_errno);
        MPIR_Assert(actual_pack_bytes == data_sz);
    }
    ucp_request = (MPIDI_UCX_ucp_request_t *) ucp_tag_send_nb(ep, send_buf,
                                                              data_sz + am_hdr_sz + sizeof(ucx_hdr),
                                                              ucp_dt_make_contig(1), ucx_tag,
                                                              &MPIDI_UCX_am_isend_callback);
    MPIDI_UCX_CHK_REQUEST(ucp_request);

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


MPL_STATIC_INLINE_PREFIX int MPIDI_NM_am_isend_reply(int src_rank,
                                                     MPIR_Comm * comm,
                                                     int handler_id,
                                                     const void *am_hdr,
                                                     size_t am_hdr_sz,
                                                     const void *data, MPI_Count count,
                                                     MPI_Datatype datatype, MPIR_Request * sreq,
                                                     MPIDI_av_entry_t * av)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_UCX_ucp_request_t *ucp_request;
    ucp_ep_h ep;
    ucp_datatype_t dt;
    uint64_t ucx_tag;
    char *send_buf;
    void *send_buf_p;
    size_t data_sz;
    size_t total_sz;
    MPI_Aint dt_true_lb;
    MPIR_Datatype *dt_ptr;
    int dt_contig;
    MPIDI_UCX_am_header_t ucx_hdr;
    ucp_dt_iov_t *iov = sreq->dev.ch4.am.netmod_am.ucx.iov;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_AM_ISEND_REPLY);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_AM_ISEND_REPLY);

    ep = MPIDI_UCX_AV_TO_EP(av);
    ucx_tag = MPIDI_UCX_init_tag(0, MPIR_Process.comm_world->rank, MPIDI_UCX_AM_TAG);

    MPIDI_Datatype_get_info(count, datatype, dt_contig, data_sz, dt_ptr, dt_true_lb);

    /* initialize our portion of the hdr */
    ucx_hdr.handler_id = handler_id;
    ucx_hdr.data_sz = data_sz;

    if (dt_contig) {
        send_buf = MPL_malloc(sizeof(ucx_hdr) + am_hdr_sz, MPL_MEM_BUFFER);
        MPIR_Memcpy(send_buf, &ucx_hdr, sizeof(ucx_hdr));
        MPIR_Memcpy(send_buf + sizeof(ucx_hdr), am_hdr, am_hdr_sz);

        iov[0].buffer = send_buf;
        iov[0].length = sizeof(ucx_hdr) + am_hdr_sz;
        iov[1].buffer = (char *) data + dt_true_lb;
        iov[1].length = data_sz;

        send_buf_p = iov;
        dt = ucp_dt_make_iov();
        total_sz = 2;
    } else {
        send_buf = MPL_malloc(data_sz + am_hdr_sz + sizeof(ucx_hdr), MPL_MEM_BUFFER);

        MPIR_Memcpy(send_buf, &ucx_hdr, sizeof(ucx_hdr));
        MPIR_Memcpy(send_buf + sizeof(ucx_hdr), am_hdr, am_hdr_sz);

        MPI_Aint actual_pack_bytes;
        mpi_errno =
            MPIR_Typerep_pack(data, count, datatype, 0, send_buf + am_hdr_sz + sizeof(ucx_hdr),
                              data_sz, &actual_pack_bytes);
        MPIR_ERR_CHECK(mpi_errno);
        MPIR_Assert(actual_pack_bytes == data_sz);

        send_buf_p = send_buf;
        dt = ucp_dt_make_contig(1);
        total_sz = data_sz + am_hdr_sz + sizeof(ucx_hdr);
    }
    ucp_request =
        (MPIDI_UCX_ucp_request_t *) ucp_tag_send_nb(ep,
                                                    send_buf_p, total_sz, dt, ucx_tag,
                                                    &MPIDI_UCX_am_isend_callback);
    MPIDI_UCX_CHK_REQUEST(ucp_request);

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

MPL_STATIC_INLINE_PREFIX size_t MPIDI_NM_am_hdr_max_sz(void)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_AM_HDR_MAX_SZ);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_AM_HDR_MAX_SZ);

    ret = (MPIDI_UCX_MAX_AM_EAGER_SZ - sizeof(MPIDI_UCX_am_header_t));

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_AM_HDR_MAX_SZ);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_am_send_hdr(int rank, MPIR_Comm * comm, int handler_id,
                                                  const void *am_hdr, size_t am_hdr_sz,
                                                  MPIDI_av_entry_t * av)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_UCX_ucp_request_t *ucp_request;
    ucp_ep_h ep;
    uint64_t ucx_tag;
    char *send_buf;
    MPIDI_UCX_am_header_t ucx_hdr;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_AM_SEND_HDR);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_AM_SEND_HDR);

    ep = MPIDI_UCX_AV_TO_EP(av);
    ucx_tag = MPIDI_UCX_init_tag(0, MPIR_Process.comm_world->rank, MPIDI_UCX_AM_TAG);

    /* initialize our portion of the hdr */
    ucx_hdr.handler_id = handler_id;
    ucx_hdr.data_sz = 0;

    /* just pack and send for now */
    send_buf = MPL_malloc(am_hdr_sz + sizeof(ucx_hdr), MPL_MEM_BUFFER);
    MPIR_Memcpy(send_buf, &ucx_hdr, sizeof(ucx_hdr));
    MPIR_Memcpy(send_buf + sizeof(ucx_hdr), am_hdr, am_hdr_sz);

    ucp_request = (MPIDI_UCX_ucp_request_t *) ucp_tag_send_nb(ep, send_buf,
                                                              am_hdr_sz + sizeof(ucx_hdr),
                                                              ucp_dt_make_contig(1), ucx_tag,
                                                              &MPIDI_UCX_am_send_callback);
    MPIDI_UCX_CHK_REQUEST(ucp_request);

    if (ucp_request == NULL) {
        /* inject is done */
        MPL_free(send_buf);
    } else {
        ucp_request->buf = send_buf;
    }

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_AM_SEND_HDR);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_am_send_hdr_reply(int src_rank, MPIR_Comm * comm,
                                                        int handler_id, const void *am_hdr,
                                                        size_t am_hdr_sz, MPIDI_av_entry_t * av)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_UCX_ucp_request_t *ucp_request;
    ucp_ep_h ep;
    uint64_t ucx_tag;
    char *send_buf;
    MPIDI_UCX_am_header_t ucx_hdr;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_AM_SEND_HDR_REPLY);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_AM_SEND_HDR_REPLY);

    ep = MPIDI_UCX_AV_TO_EP(av);
    ucx_tag = MPIDI_UCX_init_tag(0, MPIR_Process.comm_world->rank, MPIDI_UCX_AM_TAG);

    /* initialize our portion of the hdr */
    ucx_hdr.handler_id = handler_id;

    /* just pack and send for now */
    send_buf = MPL_malloc(am_hdr_sz + sizeof(ucx_hdr), MPL_MEM_BUFFER);
    MPIR_Memcpy(send_buf, &ucx_hdr, sizeof(ucx_hdr));
    MPIR_Memcpy(send_buf + sizeof(ucx_hdr), am_hdr, am_hdr_sz);
    ucp_request = (MPIDI_UCX_ucp_request_t *) ucp_tag_send_nb(ep, send_buf,
                                                              am_hdr_sz + sizeof(ucx_hdr),
                                                              ucp_dt_make_contig(1), ucx_tag,
                                                              &MPIDI_UCX_am_send_callback);
    MPIDI_UCX_CHK_REQUEST(ucp_request);

    if (ucp_request == NULL) {
        /* inject is done */
        MPL_free(send_buf);
    } else {
        ucp_request->buf = send_buf;
    }

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_AM_SEND_HDR_REPLY);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#endif /* UCX_AM_H_INCLUDED */
