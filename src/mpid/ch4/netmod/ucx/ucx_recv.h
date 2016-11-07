/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2016 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 *  Portions of this code were written by Mellanox Technologies Ltd.
 *  Copyright (C) Mellanox Technologies Ltd. 2016. ALL RIGHTS RESERVED
 */
#ifndef NETMOD_UCX_RECV_H_INCLUDED
#define NETMOD_UCX_RECV_H_INCLUDED

#include "ucx_impl.h"

MPL_STATIC_INLINE_PREFIX int ucx_irecv_continous(void *buf,
                                                 size_t data_sz,
                                                 int rank,
                                                 int tag,
                                                 MPIR_Comm * comm,
                                                 int context_offset, MPIR_Request ** request)
{

    int mpi_errno = MPI_SUCCESS;
    uint64_t ucp_tag, tag_mask;
    MPIR_Request *req;
    MPIDI_UCX_ucp_request_t *ucp_request;
//    MPID_THREAD_CS_ENTER(POBJ,MPIDI_THREAD_WORKER_MUTEX);
    tag_mask = MPIDI_UCX_tag_mask(tag, rank);
    ucp_tag = MPIDI_UCX_recv_tag(tag, rank, comm->recvcontext_id + context_offset);

    ucp_request = (MPIDI_UCX_ucp_request_t *) ucp_tag_recv_nb(MPIDI_UCX_global.worker,
                                                              buf, data_sz, ucp_dt_make_contig(1),
                                                              ucp_tag, tag_mask,
                                                              &MPIDI_UCX_Handle_recv_callback);


    MPIDI_CH4_UCX_REQUEST(ucp_request);


    if (ucp_request->req == NULL) {
        req = MPIR_Request_create(MPIR_REQUEST_KIND__RECV);
        MPIR_Request_add_ref(req);
        ucp_request->req = req;
        ucp_request_release(ucp_request);
    }
    else {
        req = ucp_request->req;
        ucp_request->req = NULL;
        ucp_request_release(ucp_request);
    }
  fn_exit:
    *request = req;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int ucx_irecv_non_continous(void *buf,
                                                     size_t count,
                                                     int rank,
                                                     int tag,
                                                     MPIR_Comm * comm,
                                                     int context_offset, MPIR_Request ** request,
                                                     MPIR_Datatype * datatype)
{

    int mpi_errno = MPI_SUCCESS;
    uint64_t ucp_tag, tag_mask;
    MPIR_Request *req;
    MPIDI_UCX_ucp_request_t *ucp_request;
//    MPID_THREAD_CS_ENTER(POBJ,MPIDI_THREAD_WORKER_MUTEX);
    tag_mask = MPIDI_UCX_tag_mask(tag, rank);
    ucp_tag = MPIDI_UCX_recv_tag(tag, rank, comm->recvcontext_id + context_offset);

    ucp_request = (MPIDI_UCX_ucp_request_t *) ucp_tag_recv_nb(MPIDI_UCX_global.worker,
                                                              buf, count,
                                                              datatype->dev.netmod.ucx.ucp_datatype,
                                                              ucp_tag, tag_mask,
                                                              &MPIDI_UCX_Handle_recv_callback);


    MPIDI_CH4_UCX_REQUEST(ucp_request);


    if (ucp_request->req == NULL) {
        req = MPIR_Request_create(MPIR_REQUEST_KIND__RECV);
        MPIR_Request_add_ref(req);
        ucp_request->req = req;
        MPIDI_UCX_REQ(req).a.ucp_request = ucp_request;
        ucp_request_release(ucp_request);
    }
    else {
        req = ucp_request->req;

        MPIDI_UCX_REQ(req).a.ucp_request = NULL;
        ucp_request->req = NULL;
        ucp_request_release(ucp_request);
    }
    (req)->kind = MPIR_REQUEST_KIND__RECV;
  fn_exit:
    *request = req;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static inline int do_irecv(void *buf,
                           int count,
                           MPI_Datatype datatype,
                           int rank,
                           int tag, MPIR_Comm * comm, int context_offset, MPIR_Request ** request)
{
    int mpi_errno = MPI_SUCCESS;
    size_t data_sz;
    int dt_contig;
    MPI_Aint dt_true_lb;

    MPIR_Datatype *dt_ptr;

    MPIDI_Datatype_get_info(count, datatype, dt_contig, data_sz, dt_ptr, dt_true_lb);
    if (dt_contig)
        mpi_errno =
            ucx_irecv_continous((char *) buf + dt_true_lb, data_sz, rank, tag, comm, context_offset,
                                request);
    else
        mpi_errno =
            ucx_irecv_non_continous(buf, count, rank, tag, comm, context_offset, request, dt_ptr);
  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;

}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_recv(void *buf,
                                               int count,
                                               MPI_Datatype datatype,
                                               int rank,
                                               int tag,
                                               MPIR_Comm * comm,
                                               int context_offset,
                                               MPI_Status * status, MPIR_Request ** request)
{

    return do_irecv(buf, count, datatype, rank, tag, comm, context_offset, request);
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_recv_init(void *buf,
                                                    int count,
                                                    MPI_Datatype datatype,
                                                    int rank,
                                                    int tag,
                                                    MPIR_Comm * comm,
                                                    int context_offset, MPIR_Request ** request)
{
    return MPIDIG_mpi_recv_init(buf, count, datatype, rank, tag, comm, context_offset, request);
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_imrecv(void *buf,
                                                 int count,
                                                 MPI_Datatype datatype,
                                                 MPIR_Request * message, MPIR_Request ** rreqp)
{
    ucp_tag_message_h message_handler;
    int mpi_errno = MPI_SUCCESS;
    size_t data_sz;
    int dt_contig;
    MPIR_Request *req;
    MPI_Aint dt_true_lb;
    MPIDI_UCX_ucp_request_t *ucp_request;

    MPIR_Datatype *dt_ptr;
    MPIDI_Datatype_get_info(count, datatype, dt_contig, data_sz, dt_ptr, dt_true_lb);
    if (message == NULL) {
        mpi_errno = MPI_SUCCESS;
        MPIDI_CH4U_request_complete(req);
        *rreqp = req;

        goto fn_exit;
    }

    message_handler = MPIDI_UCX_REQ(message).a.message_handler;
    if (dt_contig)
        ucp_request = (MPIDI_UCX_ucp_request_t *) ucp_tag_msg_recv_nb(MPIDI_UCX_global.worker,
                                                                      (char *) buf + dt_true_lb, data_sz,
                                                                      ucp_dt_make_contig(1),
                                                                      message_handler,
                                                                      &MPIDI_UCX_Handle_recv_callback);
    else
        ucp_request = (MPIDI_UCX_ucp_request_t *) ucp_tag_msg_recv_nb(MPIDI_UCX_global.worker,
                                                                      buf, count,
                                                                      dt_ptr->dev.netmod.ucx.
                                                                      ucp_datatype, message_handler,
                                                                      &MPIDI_UCX_Handle_recv_callback);


    MPIDI_CH4_UCX_REQUEST(ucp_request);

    if (ucp_request->req == NULL) {
        req = MPIR_Request_create(MPIR_REQUEST_KIND__RECV);
        MPIR_Request_add_ref(req);
        ucp_request->req = req;
        ucp_request_release(ucp_request);
    }
    else {
        req = ucp_request->req;
        ucp_request->req = NULL;
        ucp_request_release(ucp_request);
    }


  fn_exit:
    *rreqp = req;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_irecv(void *buf,
                                                int count,
                                                MPI_Datatype datatype,
                                                int rank,
                                                int tag,
                                                MPIR_Comm * comm, int context_offset,
                                                MPIR_Request ** request)
{



    return do_irecv(buf, count, datatype, rank, tag, comm, context_offset, request);

}

static inline int MPIDI_NM_mpi_cancel_recv(MPIR_Request * rreq)
{

    if (MPIDI_UCX_REQ(rreq).a.ucp_request) {
        ucp_request_cancel(MPIDI_UCX_global.worker, MPIDI_UCX_REQ(rreq).a.ucp_request);
    }

}

#endif /* NETMOD_UCX_RECV_H_INCLUDED */
