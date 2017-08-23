/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2016 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 *  Portions of this code were written by Mellanox Technologies Ltd.
 *  Copyright (C) Mellanox Technologies Ltd. 2016. ALL RIGHTS RESERVED
 */
#ifndef UCX_SEND_H_INCLUDED
#define UCX_SEND_H_INCLUDED
#include <ucp/api/ucp.h>
#include "ucx_impl.h"
#include "ucx_types.h"

#undef FUNCNAME
#define FUNCNAME MPIDI_UCX_send_cmpl_cb
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline void MPIDI_UCX_send_cmpl_cb(void *request, ucs_status_t status)
{
    MPIDI_UCX_ucp_request_t *ucp_request = (MPIDI_UCX_ucp_request_t *) request;
    MPIR_Request *req = ucp_request->req;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_UCX_SEND_CMPL_CB);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_UCX_SEND_CMPL_CB);

    if (unlikely(status == UCS_ERR_CANCELED))
        MPIR_STATUS_SET_CANCEL_BIT(req->status, TRUE);
    MPIDI_CH4U_request_complete(req);
    ucp_request->req = NULL;
    ucp_request_release(ucp_request);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_UCX_SEND_CMPL_CB);
}

#undef FUNCNAME
#define FUNCNAME MPIDI_UCX_send
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_UCX_send(const void *buf,
                                 int count,
                                 MPI_Datatype datatype,
                                 int rank,
                                 int tag,
                                 MPIR_Comm * comm,
                                 int context_offset,
                                 MPIDI_av_entry_t *addr,
                                 MPIR_Request ** request,
                                 int have_request,
                                 int is_sync)
{
    int dt_contig;
    size_t data_sz;
    MPI_Aint dt_true_lb;
    MPIR_Datatype *dt_ptr;
    int mpi_errno = MPI_SUCCESS;
    MPIR_Request *req = NULL;
    MPIDI_UCX_ucp_request_t *ucp_request;
    ucp_ep_h ep;
    uint64_t ucx_tag;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_UCX_SEND);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_UCX_SEND);

    ep = MPIDI_UCX_AV_TO_EP(addr);
    ucx_tag = MPIDI_UCX_init_tag(comm->context_id + context_offset, comm->rank, tag);
    MPIDI_Datatype_get_info(count, datatype, dt_contig, data_sz, dt_ptr, dt_true_lb);

    if (is_sync) {
        if (dt_contig) {
            ucp_request =
                (MPIDI_UCX_ucp_request_t *) ucp_tag_send_sync_nb(ep, (char *)buf + dt_true_lb, data_sz,
                                                                 ucp_dt_make_contig(1),
                                                                 ucx_tag, &MPIDI_UCX_send_cmpl_cb);
        } else {
            MPIR_Datatype_add_ref(dt_ptr);
            ucp_request =
                (MPIDI_UCX_ucp_request_t *) ucp_tag_send_sync_nb(ep, buf, count,
                                                                 dt_ptr->dev.netmod.ucx.ucp_datatype,
                                                                 ucx_tag, &MPIDI_UCX_send_cmpl_cb);
        }
    } else {
        if (dt_contig) {
            ucp_request =
                (MPIDI_UCX_ucp_request_t *) ucp_tag_send_nb(ep, (char *)buf + dt_true_lb, data_sz,
                                                            ucp_dt_make_contig(1), ucx_tag,
                                                            &MPIDI_UCX_send_cmpl_cb);
        } else {
            MPIR_Datatype_add_ref(dt_ptr);
            ucp_request =
                (MPIDI_UCX_ucp_request_t *) ucp_tag_send_nb(ep, buf, count,
                                                            dt_ptr->dev.netmod.ucx.ucp_datatype, ucx_tag,
                                                            &MPIDI_UCX_send_cmpl_cb);
        }
    }
    MPIDI_CH4_UCX_REQUEST(ucp_request);

    if (ucp_request) {
        req = MPIR_Request_create(MPIR_REQUEST_KIND__SEND);
        MPIR_Request_add_ref(req);
        ucp_request->req = req;
        MPIDI_UCX_REQ(req).a.ucp_request = ucp_request;
    } else if (have_request) {
#ifndef HAVE_DEBUGGER_SUPPORT
        req = MPIDI_UCX_global.lw_send_req;
        MPIR_Request_add_ref(req);
#else
        req = MPIR_Request_create(MPIR_REQUEST_KIND__SEND);
        MPIR_cc_set(&req->cc, 0);
#endif
    }
    *request = req;

 fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_UCX_SEND);
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_NM_mpi_send
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_NM_mpi_send(const void *buf,
                                    int count,
                                    MPI_Datatype datatype,
                                    int rank,
                                    int tag,
                                    MPIR_Comm * comm, int context_offset,
                                    MPIDI_av_entry_t *addr, MPIR_Request ** request)
{
    return MPIDI_UCX_send(buf, count, datatype, rank, tag, comm, context_offset,
                          addr, request, 0, 0);
}

#undef FUNCNAME
#define FUNCNAME MPIDI_NM_mpi_ssend
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_NM_mpi_ssend(const void *buf,
                                     int count,
                                     MPI_Datatype datatype,
                                     int rank,
                                     int tag,
                                     MPIR_Comm * comm, int context_offset,
                                     MPIDI_av_entry_t *addr, MPIR_Request ** request)
{
    return MPIDI_UCX_send(buf, count, datatype, rank, tag, comm, context_offset,
                          addr, request, 0, 1);
}

#undef FUNCNAME
#define FUNCNAME MPIDI_NM_mpi_startall
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_NM_mpi_startall(int count, MPIR_Request * requests[])
{
    return MPIDIG_mpi_startall(count, requests);
}

#undef FUNCNAME
#define FUNCNAME MPIDI_NM_mpi_send_init
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_NM_mpi_send_init(const void *buf,
                                         int count,
                                         MPI_Datatype datatype,
                                         int rank,
                                         int tag,
                                         MPIR_Comm * comm, int context_offset,
                                         MPIDI_av_entry_t *addr,
                                         MPIR_Request ** request)
{
    return MPIDIG_mpi_send_init(buf, count, datatype, rank, tag, comm, context_offset, request);
}

#undef FUNCNAME
#define FUNCNAME MPIDI_NM_mpi_ssend_init
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_NM_mpi_ssend_init(const void *buf,
                                          int count,
                                          MPI_Datatype datatype,
                                          int rank,
                                          int tag,
                                          MPIR_Comm * comm, int context_offset,
                                          MPIDI_av_entry_t *addr,
                                          MPIR_Request ** request)
{
    return MPIDIG_mpi_ssend_init(buf, count, datatype, rank, tag, comm, context_offset, request);
}

#undef FUNCNAME
#define FUNCNAME MPIDI_NM_mpi_bsend_init
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_NM_mpi_bsend_init(const void *buf,
                                          int count,
                                          MPI_Datatype datatype,
                                          int rank,
                                          int tag,
                                          MPIR_Comm * comm, int context_offset,
                                          MPIDI_av_entry_t *addr,
                                          MPIR_Request ** request)
{
    return MPIDIG_mpi_bsend_init(buf, count, datatype, rank, tag, comm, context_offset, request);
}

#undef FUNCNAME
#define FUNCNAME MPIDI_NM_mpi_rsend_init
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_NM_mpi_rsend_init(const void *buf,
                                          int count,
                                          MPI_Datatype datatype,
                                          int rank,
                                          int tag,
                                          MPIR_Comm * comm, int context_offset,
                                          MPIDI_av_entry_t *addr,
                                          MPIR_Request ** request)
{
    return MPIDIG_mpi_rsend_init(buf, count, datatype, rank, tag, comm, context_offset, request);
}

#undef FUNCNAME
#define FUNCNAME MPIDI_NM_mpi_isend
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_NM_mpi_isend(const void *buf,
                                     int count,
                                     MPI_Datatype datatype,
                                     int rank,
                                     int tag,
                                     MPIR_Comm * comm, int context_offset,
                                     MPIDI_av_entry_t *addr, MPIR_Request ** request)
{
    return MPIDI_UCX_send(buf, count, datatype, rank, tag, comm, context_offset,
                          addr, request, 1, 0);
}

#undef FUNCNAME
#define FUNCNAME MPIDI_NM_mpi_issend
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_NM_mpi_issend(const void *buf,
                                      int count,
                                      MPI_Datatype datatype,
                                      int rank,
                                      int tag,
                                      MPIR_Comm * comm, int context_offset,
                                      MPIDI_av_entry_t *addr, MPIR_Request ** request)
{
    return MPIDI_UCX_send(buf, count, datatype, rank, tag, comm, context_offset,
                          addr, request, 1, 1);
}

#undef FUNCNAME
#define FUNCNAME MPIDI_NM_mpi_cancel_send
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_NM_mpi_cancel_send(MPIR_Request * sreq)
{
    if (!MPIR_Request_is_complete(sreq)) {
        ucp_request_cancel(MPIDI_UCX_global.worker, MPIDI_UCX_REQ(sreq).a.ucp_request);
    }

    return MPI_SUCCESS;
}

#endif /* UCX_SEND_H_INCLUDED */
