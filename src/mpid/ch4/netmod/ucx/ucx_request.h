/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2016 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 *  Portions of this code were written by Mellanox Technologies Ltd.
 *  Copyright (C) Mellanox Technologies Ltd. 2016. ALL RIGHTS RESERVED
 */
#ifndef UCX_REQUEST_H_INCLUDED
#define UCX_REQUEST_H_INCLUDED

#include "ucx_impl.h"
#include "mpidch4.h"
#include <ucp/api/ucp.h>
#include "mpidch4r.h"

#undef FUNCNAME
#define FUNCNAME MPIDI_NM_am_request_init
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline void MPIDI_NM_am_request_init(MPIR_Request * req)
{
    req->dev.ch4.am.netmod_am.ucx.pack_buffer = NULL;
}

static inline void MPIDI_NM_am_request_finalize(MPIR_Request * req)
{
    if ((req)->dev.ch4.am.netmod_am.ucx.pack_buffer) {
        MPL_free((req)->dev.ch4.am.netmod_am.ucx.pack_buffer);
    }
    /* MPIR_Request_free(req); */
}

static inline void MPIDI_UCX_Request_init_callback(void *request)
{

    MPIDI_UCX_ucp_request_t *ucp_request = (MPIDI_UCX_ucp_request_t *) request;
    ucp_request->req = NULL;

}

static inline void MPIDI_UCX_Handle_send_callback(void *request, ucs_status_t status)
{
    int c;
    int mpi_errno;
    MPIDI_UCX_ucp_request_t *ucp_request = (MPIDI_UCX_ucp_request_t *) request;
    MPIR_Request *req = ucp_request->req;
    if (unlikely(status == UCS_ERR_CANCELED)) {
        MPIR_STATUS_SET_CANCEL_BIT(req->status, TRUE);
        goto fn_exit;
    }

    MPIDI_CH4U_request_complete(req);
    ucp_request->req = NULL;
  fn_exit:
    return;
  fn_fail:
    req->status.MPI_ERROR = mpi_errno;
}

static inline void MPIDI_UCX_Handle_recv_callback(void *request, ucs_status_t status,
        ucp_tag_recv_info_t * info)
{
    MPI_Aint count;
    int mpi_errno;
    MPIDI_UCX_ucp_request_t *ucp_request = (MPIDI_UCX_ucp_request_t *) request;
    MPIR_Request *rreq = NULL;
    if (!ucp_request->req) {
        rreq = MPIR_Request_create(MPIR_REQUEST_KIND__RECV);
        MPIR_Request_add_ref(rreq);
        ucp_request->req = rreq;
    }
    else {
        rreq = ucp_request->req;
        ucp_request->req = NULL;
    }

    rreq->status.MPI_ERROR = MPI_SUCCESS;
    rreq->status.MPI_SOURCE = MPIDI_UCX_get_source(info->sender_tag);
    rreq->status.MPI_TAG = MPIDI_UCX_get_tag(info->sender_tag);
    count = info->length;
    MPIR_STATUS_SET_COUNT(rreq->status, count);
    MPIDI_CH4U_request_complete(rreq);
 
    if (unlikely(status == UCS_ERR_CANCELED)) {
        MPIDI_CH4U_request_complete(rreq);
        MPIR_STATUS_SET_CANCEL_BIT(rreq->status, TRUE);
        ucp_request->req = NULL;
        goto fn_exit;
    }

  fn_exit:
    return;
  fn_fail:
    rreq->status.MPI_ERROR = mpi_errno;
}

#endif /* UCX_REQUEST_H_INCLUDED */
