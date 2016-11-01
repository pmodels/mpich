/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2016 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 *  Portions of this code were written by Mellanox Technologies Ltd.
 *  Copyright (C) Mellanox Technologies Ltd. 2016. ALL RIGHTS RESERVED
 */
#ifndef NETMOD_UCX_SEND_H_INCLUDED
#define NETMOD_UCX_SEND_H_INCLUDED
#include <ucp/api/ucp.h>
#include "ucx_impl.h"
#include "ucx_types.h"

#undef FUNCNAME
#define FUNCNAME ucx_send_continous
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int ucx_send_continous(const void *buf,
                                                size_t data_sz,
                                                int rank,
                                                int tag,
                                                MPIR_Comm * comm, int context_offset,
                                                MPIR_Request ** request, int have_request)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Request *req;
    MPIDI_UCX_ucp_request_t *ucp_request;
    ucp_ep_h ep;
    uint64_t ucx_tag;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_SEND_CONTINOUS);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_SEND_CONTINOUS);

    ep = MPIDI_UCX_COMM_TO_EP(comm, rank);
    ucx_tag = MPIDI_UCX_init_tag(comm->context_id + context_offset, comm->rank, tag);

    ucp_request =
        (MPIDI_UCX_ucp_request_t *) ucp_tag_send_nb(ep, buf, data_sz, ucp_dt_make_contig(1),
                ucx_tag, &MPIDI_UCX_Handle_send_callback);

    MPIDI_CH4_UCX_REQUEST(ucp_request);

    if (ucp_request == NULL) {
        req = MPIR_Request_create(MPIR_REQUEST_KIND__SEND);
        MPIR_cc_set(&req->cc, 0);
        MPIDI_UCX_REQ(req).a.ucp_request = NULL;
        goto fn_exit;
    }

    req = MPIR_Request_create(MPIR_REQUEST_KIND__SEND);
    MPIR_Request_add_ref(req);
    ucp_request->req = req;
    MPIDI_UCX_REQ(req).a.ucp_request = ucp_request;
    ucp_request_release(ucp_request);

  fn_exit:
    *request = req;

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_SEND_CONTINOUS);
    return mpi_errno;
  fn_fail:
    goto fn_exit;

}

MPL_STATIC_INLINE_PREFIX int ucx_sync_send_continous(const void *buf,
                                                     size_t data_sz,
                                                     int rank,
                                                     int tag,
                                                     MPIR_Comm * comm, int context_offset,
                                                     MPIR_Request ** request, int have_request)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Request *req;
    MPIDI_UCX_ucp_request_t *ucp_request;
    ucp_ep_h ep;
    uint64_t ucx_tag;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_SEND_CONTINOUS);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_SEND_CONTINOUS);

    ep = MPIDI_UCX_COMM_TO_EP(comm, rank);
    ucx_tag = MPIDI_UCX_init_tag(comm->context_id + context_offset, comm->rank, tag);

    ucp_request =
        (MPIDI_UCX_ucp_request_t *) ucp_tag_send_sync_nb(ep, buf, data_sz, ucp_dt_make_contig(1),
                                                         ucx_tag, &MPIDI_UCX_Handle_send_callback);

    MPIDI_CH4_UCX_REQUEST(ucp_request);
    if (ucp_request->req) {
        req = ucp_request->req;
        ucp_request->req = NULL;
        MPIDI_UCX_REQ(req).a.ucp_request = NULL;
        ucp_request_release(ucp_request);
    }
    else {
        req = MPIR_Request_create(MPIR_REQUEST_KIND__SEND);
        MPIR_Request_add_ref(req);
        ucp_request->req = req;
        MPIDI_UCX_REQ(req).a.ucp_request = ucp_request;
        ucp_request_release(ucp_request);
    }


  fn_exit:
    *request = req;

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_SEND_CONTINOUS);
    return mpi_errno;
  fn_fail:
    goto fn_exit;

}

MPL_STATIC_INLINE_PREFIX int ucx_sync_send_non_continous(const void *buf,
                                                         size_t count,
                                                         int rank,
                                                         int tag,
                                                         MPIR_Comm * comm, int context_offset,
                                                         MPIR_Request ** request, int have_request,
                                                         MPIR_Datatype * datatype)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Request *req;
    MPIDI_UCX_ucp_request_t *ucp_request;
    ucp_ep_h ep;
    uint64_t ucx_tag;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_SEND_CONTINOUS);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_SEND_CONTINOUS);

    ep = MPIDI_UCX_COMM_TO_EP(comm, rank);
    ucx_tag = MPIDI_UCX_init_tag(comm->context_id + context_offset, comm->rank, tag);

    ucp_request =
        (MPIDI_UCX_ucp_request_t *) ucp_tag_send_sync_nb(ep, buf, count,
                                                         datatype->dev.netmod.ucx.ucp_datatype,
                                                         ucx_tag, &MPIDI_UCX_Handle_send_callback);

    MPIDI_CH4_UCX_REQUEST(ucp_request);

    if (ucp_request->req) {
        req = ucp_request->req;
        ucp_request->req = NULL;
        ucp_request_release(ucp_request);
        MPIDI_UCX_REQ(req).a.ucp_request = NULL;
    }
    else {
        req = MPIR_Request_create(MPIR_REQUEST_KIND__SEND);
        MPIR_Request_add_ref(req);
        ucp_request->req = req;
        ucp_request_release(ucp_request);
        MPIDI_UCX_REQ(req).a.ucp_request = ucp_request;
    }


  fn_exit:
    *request = req;

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_SEND_CONTINOUS);
    return mpi_errno;
  fn_fail:
    goto fn_exit;

}



#undef FUNCNAME
#define FUNCNAME ucx_send_continous
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)

MPL_STATIC_INLINE_PREFIX int ucx_send_non_continous(const void *buf,
                                                    size_t count,
                                                    int rank,
                                                    int tag,
                                                    MPIR_Comm * comm, int context_offset,
                                                    MPIR_Request ** request, int have_request,
                                                    MPIR_Datatype * datatype)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Request *req;
    MPIDI_UCX_ucp_request_t *ucp_request;
    ucp_ep_h ep;
    uint64_t ucx_tag;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_SEND_CONTINOUS);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_SEND_CONTINOUS);

    ep = MPIDI_UCX_COMM_TO_EP(comm, rank);
    ucx_tag = MPIDI_UCX_init_tag(comm->context_id + context_offset, comm->rank, tag);

    ucp_request =
        (MPIDI_UCX_ucp_request_t *) ucp_tag_send_nb(ep, buf, count,
                                                    datatype->dev.netmod.ucx.ucp_datatype, ucx_tag,
                                                    &MPIDI_UCX_Handle_send_callback);

    MPIDI_CH4_UCX_REQUEST(ucp_request);

    if (ucp_request == NULL) {
        req = MPIR_Request_create(MPIR_REQUEST_KIND__SEND);
        MPIDI_UCX_REQ(req).a.ucp_request = NULL;
        MPIR_cc_set(&req->cc, 0);
        goto fn_exit;
    }

    if (ucp_request->req) {
        req = ucp_request->req;
        ucp_request->req = NULL;
        MPIDI_UCX_REQ(req).a.ucp_request = NULL;
        ucp_request_release(ucp_request);
    }
    else {
        req = MPIR_Request_create(MPIR_REQUEST_KIND__SEND);
        MPIR_Request_add_ref(req);
        ucp_request->req = req;
        MPIDI_UCX_REQ(req).a.ucp_request = ucp_request;
        ucp_request_release(ucp_request);
    }


  fn_exit:
    *request = req;

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_SEND_CONTINOUS);
    return mpi_errno;
  fn_fail:
    goto fn_exit;

}

#undef FUNCNAME
#define FUNCNAME ucx_send
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int ucx_send(const void *buf,
                           int count,
                           MPI_Datatype datatype,
                           int rank,
                           int tag,
                           MPIR_Comm * comm, int context_offset, MPIR_Request ** request,
                           int have_request)
{

    int dt_contig, mpi_errno;
    size_t data_sz;
    MPI_Aint dt_true_lb;
    MPIR_Datatype *dt_ptr;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_SEND);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_SEND);

    MPIDI_Datatype_get_info(count, datatype, dt_contig, data_sz, dt_ptr, dt_true_lb);

    if (dt_contig)
        mpi_errno =
            ucx_send_continous((char *) buf + dt_true_lb, data_sz, rank, tag, comm, context_offset, request,
                               have_request);
    else
        mpi_errno =
            ucx_send_non_continous(buf, count, rank, tag, comm, context_offset, request,
                                   have_request, dt_ptr);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_SEND);
    return mpi_errno;
  fn_fail:
    goto fn_exit;

}

static inline int ucx_sync_send(const void *buf,
                                int count,
                                MPI_Datatype datatype,
                                int rank,
                                int tag,
                                MPIR_Comm * comm, int context_offset, MPIR_Request ** request,
                                int have_request)
{

    int dt_contig, mpi_errno;
    size_t data_sz;
    MPI_Aint dt_true_lb;
    MPIR_Datatype *dt_ptr;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_SEND);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_SEND);

    MPIDI_Datatype_get_info(count, datatype, dt_contig, data_sz, dt_ptr, dt_true_lb);

    if (dt_contig)
        mpi_errno =
            ucx_sync_send_continous((char *) buf + dt_true_lb, data_sz, rank, tag, comm, context_offset,
                                    request, have_request);
    else
        mpi_errno =
            ucx_sync_send_non_continous(buf, count, rank, tag, comm, context_offset, request,
                                        have_request, dt_ptr);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_SEND);
    return mpi_errno;
  fn_fail:
    goto fn_exit;

}

#undef FUNCNAME
#define FUNCNAME MPIDI_netmode_send
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_NM_mpi_send(const void *buf,
                                    int count,
                                    MPI_Datatype datatype,
                                    int rank,
                                    int tag,
                                    MPIR_Comm * comm, int context_offset, MPIR_Request ** request)
{

    int mpi_errno;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_SEND);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_SEND);
    mpi_errno = ucx_send(buf, count, datatype, rank, tag, comm, context_offset, request, 0);
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_SEND);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_netmode_rsend
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_NM_rsend(const void *buf,
                                 int count,
                                 MPI_Datatype datatype,
                                 int rank,
                                 int tag,
                                 MPIR_Comm * comm, int context_offset, MPIR_Request ** request)
{

    return ucx_send(buf, count, datatype, rank, tag, comm, context_offset, request, 0);
}


#undef FUNCNAME
#define FUNCNAME MPIDI_netmode_irsend
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_netmod_irsend(const void *buf,
                                      int count,
                                      MPI_Datatype datatype,
                                      int rank,
                                      int tag,
                                      MPIR_Comm * comm, int context_offset, MPIR_Request ** request)
{
    return ucx_send(buf, count, datatype, rank, tag, comm, context_offset, request, 1);
}

#undef FUNCNAME
#define FUNCNAME MPIDI_netmode_ssend
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_NM_mpi_ssend(const void *buf,
                                     int count,
                                     MPI_Datatype datatype,
                                     int rank,
                                     int tag,
                                     MPIR_Comm * comm, int context_offset, MPIR_Request ** request)
{
    return ucx_sync_send(buf, count, datatype, rank, tag, comm, context_offset, request, 0);
}

static inline int MPIDI_NM_mpi_startall(int count, MPIR_Request * requests[])
{
    return MPIDI_CH4U_mpi_startall(count, requests);
}

#undef FUNCNAME
#define FUNCNAME MPIDI_netmode_send_init
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_NM_mpi_send_init(const void *buf,
                                         int count,
                                         MPI_Datatype datatype,
                                         int rank,
                                         int tag,
                                         MPIR_Comm * comm, int context_offset,
                                         MPIR_Request ** request)
{
    return MPIDI_CH4U_mpi_send_init(buf, count, datatype, rank, tag, comm, context_offset, request);
}

#undef FUNCNAME
#define FUNCNAME MPIDI_netmode_ssend_init
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_NM_mpi_ssend_init(const void *buf,
                                          int count,
                                          MPI_Datatype datatype,
                                          int rank,
                                          int tag,
                                          MPIR_Comm * comm, int context_offset,
                                          MPIR_Request ** request)
{
    return MPIDI_CH4U_mpi_ssend_init(buf, count, datatype, rank, tag, comm, context_offset,
                                     request);
}

#undef FUNCNAME
#define FUNCNAME MPIDI_netmode_bsend_init
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_NM_mpi_bsend_init(const void *buf,
                                          int count,
                                          MPI_Datatype datatype,
                                          int rank,
                                          int tag,
                                          MPIR_Comm * comm, int context_offset,
                                          MPIR_Request ** request)
{
    return MPIDI_CH4U_mpi_bsend_init(buf, count, datatype, rank, tag, comm, context_offset,
                                     request);
}

#undef FUNCNAME
#define FUNCNAME MPIDI_netmode_rsend_init
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_NM_mpi_rsend_init(const void *buf,
                                          int count,
                                          MPI_Datatype datatype,
                                          int rank,
                                          int tag,
                                          MPIR_Comm * comm, int context_offset,
                                          MPIR_Request ** request)
{
    return MPIDI_CH4U_mpi_rsend_init(buf, count, datatype, rank, tag, comm, context_offset,
                                     request);
}

#undef FUNCNAME
#define FUNCNAME MPIDI_netmode_isend
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_NM_mpi_isend(const void *buf,
                                     int count,
                                     MPI_Datatype datatype,
                                     int rank,
                                     int tag,
                                     MPIR_Comm * comm, int context_offset, MPIR_Request ** request)
{

    return ucx_send(buf, count, datatype, rank, tag, comm, context_offset, request, 1);

}

#undef FUNCNAME
#define FUNCNAME MPIDI_netmode_issend
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_NM_mpi_issend(const void *buf,
                                      int count,
                                      MPI_Datatype datatype,
                                      int rank,
                                      int tag,
                                      MPIR_Comm * comm, int context_offset, MPIR_Request ** request)
{

    return ucx_sync_send(buf, count, datatype, rank, tag, comm, context_offset, request, 1);

}

#undef FUNCNAME
#define FUNCNAME MPIDI_netmode_cancel_send
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_NM_mpi_cancel_send(MPIR_Request * sreq)
{
    if (MPIDI_UCX_REQ(sreq).a.ucp_request) {
        ucp_request_cancel(MPIDI_UCX_global.worker, MPIDI_UCX_REQ(sreq).a.ucp_request);
        ucp_request_release(MPIDI_UCX_REQ(sreq).a.ucp_request);
    }
}

#endif /* NETMOD_UCX_SEND_H_INCLUDED */
