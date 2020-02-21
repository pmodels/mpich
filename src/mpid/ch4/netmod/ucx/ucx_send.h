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

MPL_STATIC_INLINE_PREFIX void MPIDI_UCX_send_cmpl_cb(void *request, ucs_status_t status)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_UCX_SEND_CMPL_CB);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_UCX_SEND_CMPL_CB);

    MPIDI_UCX_ucp_request_t *ucp_request = request;

    MPIR_Request *sreq = ucp_request->u.send.sreq;
    if (unlikely(status == UCS_ERR_CANCELED))
        MPIR_STATUS_SET_CANCEL_BIT(sreq->status, TRUE);
    MPIDIU_request_complete(sreq);
    MPIDI_UCX_UCP_REQUEST_RELEASE(ucp_request);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_UCX_SEND_CMPL_CB);
}

MPL_STATIC_INLINE_PREFIX int MPIDI_UCX_send(const void *buf,
                                            MPI_Aint count,
                                            MPI_Datatype datatype,
                                            int rank,
                                            int tag,
                                            MPIR_Comm * comm,
                                            int context_offset,
                                            MPIDI_av_entry_t * addr,
                                            MPIR_Request ** request, int have_request, int is_sync)
{
    int dt_contig;
    size_t data_sz;
    MPI_Aint dt_true_lb;
    MPIR_Datatype *dt_ptr;
    int mpi_errno = MPI_SUCCESS;
    ucp_ep_h ep;
    uint64_t ucx_tag;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_UCX_SEND);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_UCX_SEND);

    ep = MPIDI_UCX_AV_TO_EP(addr);
    ucx_tag = MPIDI_UCX_init_tag(comm->context_id + context_offset, comm->rank, tag);
    MPIDI_Datatype_get_info(count, datatype, dt_contig, data_sz, dt_ptr, dt_true_lb);

    MPIDI_UCX_ucp_request_t *ucp_request;

    if (is_sync) {
        if (dt_contig) {
            ucp_request =
                (MPIDI_UCX_ucp_request_t *) ucp_tag_send_sync_nb(ep, (char *) buf + dt_true_lb,
                                                                 data_sz, ucp_dt_make_contig(1),
                                                                 ucx_tag, &MPIDI_UCX_send_cmpl_cb);
        } else {
            MPIR_Datatype_ptr_add_ref(dt_ptr);
            ucp_request =
                (MPIDI_UCX_ucp_request_t *) ucp_tag_send_sync_nb(ep, buf, count,
                                                                 dt_ptr->dev.netmod.
                                                                 ucx.ucp_datatype, ucx_tag,
                                                                 &MPIDI_UCX_send_cmpl_cb);
        }
    } else {
        if (dt_contig) {
            ucp_request =
                (MPIDI_UCX_ucp_request_t *) ucp_tag_send_nb(ep, (char *) buf + dt_true_lb, data_sz,
                                                            ucp_dt_make_contig(1), ucx_tag,
                                                            &MPIDI_UCX_send_cmpl_cb);
        } else {
            MPIR_Datatype_ptr_add_ref(dt_ptr);
            ucp_request =
                (MPIDI_UCX_ucp_request_t *) ucp_tag_send_nb(ep, buf, count,
                                                            dt_ptr->dev.netmod.ucx.ucp_datatype,
                                                            ucx_tag, &MPIDI_UCX_send_cmpl_cb);
        }
    }
    MPIDI_UCX_CHK_REQUEST(ucp_request);

    if (UCS_OK == (ucs_status_ptr_t) ucp_request) {
        /* immediately completed */
        if (*request) {
            /* a send request passed down from ch4 -- e.g. workq */
            MPID_Request_complete(*request);
        } else if (have_request) {
            /* lightweight completion */
            *request = MPIR_Request_create_complete(MPIR_REQUEST_KIND__SEND);
        }
    } else {
        MPIR_Request *sreq;
        if (*request) {
            sreq = *request;
        } else {
            sreq = MPIR_Request_create(MPIR_REQUEST_KIND__SEND, 0);
            MPIR_Request_add_ref(sreq);
            *request = sreq;
        }

        /* let callback to complete sreq */
        ucp_request->u.send.sreq = sreq;

        /* in case we need call ucp_request_cancel */
        MPIDI_UCX_req_send_t *p_send = &MPIDI_UCX_REQ(sreq).send;
        p_send->ucp_request = ucp_request;
    }

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_UCX_SEND);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_send_coll(const void *buf, MPI_Aint count,
                                                MPI_Datatype datatype, int rank, int tag,
                                                MPIR_Comm * comm,
                                                int context_offset, MPIDI_av_entry_t * addr,
                                                MPIR_Request ** request, MPIR_Errflag_t * errflag)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_SEND_COLL);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_SEND_COLL);

    switch (*errflag) {
        case MPIR_ERR_NONE:
            break;
        case MPIR_ERR_PROC_FAILED:
            MPIR_TAG_SET_PROC_FAILURE_BIT(tag);
            break;
        default:
            MPIR_TAG_SET_ERROR_BIT(tag);
    }

    mpi_errno = MPIDI_UCX_send(buf, count, datatype, rank, tag, comm, context_offset, addr, request,
                               0, 0);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_SEND_COLL);

    return mpi_errno;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_send(const void *buf,
                                               MPI_Aint count,
                                               MPI_Datatype datatype,
                                               int rank,
                                               int tag,
                                               MPIR_Comm * comm, int context_offset,
                                               MPIDI_av_entry_t * addr, MPIR_Request ** request)
{
    return MPIDI_UCX_send(buf, count, datatype, rank, tag, comm, context_offset,
                          addr, request, 0, 0);
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_ssend(const void *buf,
                                                MPI_Aint count,
                                                MPI_Datatype datatype,
                                                int rank,
                                                int tag,
                                                MPIR_Comm * comm, int context_offset,
                                                MPIDI_av_entry_t * addr, MPIR_Request ** request)
{
    return MPIDI_UCX_send(buf, count, datatype, rank, tag, comm, context_offset,
                          addr, request, 0, 1);
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_isend_coll(const void *buf, MPI_Aint count,
                                                 MPI_Datatype datatype, int rank, int tag,
                                                 MPIR_Comm * comm, int context_offset,
                                                 MPIDI_av_entry_t * addr, MPIR_Request ** request,
                                                 MPIR_Errflag_t * errflag)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_ISEND_COLL);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_ISEND_COLL);

    switch (*errflag) {
        case MPIR_ERR_NONE:
            break;
        case MPIR_ERR_PROC_FAILED:
            MPIR_TAG_SET_PROC_FAILURE_BIT(tag);
            break;
        default:
            MPIR_TAG_SET_ERROR_BIT(tag);
    }

    return MPIDI_UCX_send(buf, count, datatype, rank, tag, comm, context_offset,
                          addr, request, 1, 0);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_ISEND_COLL);

    return mpi_errno;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_isend(const void *buf,
                                                MPI_Aint count,
                                                MPI_Datatype datatype,
                                                int rank,
                                                int tag,
                                                MPIR_Comm * comm, int context_offset,
                                                MPIDI_av_entry_t * addr, MPIR_Request ** request)
{
    return MPIDI_UCX_send(buf, count, datatype, rank, tag, comm, context_offset,
                          addr, request, 1, 0);
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_issend(const void *buf,
                                                 MPI_Aint count,
                                                 MPI_Datatype datatype,
                                                 int rank,
                                                 int tag,
                                                 MPIR_Comm * comm, int context_offset,
                                                 MPIDI_av_entry_t * addr, MPIR_Request ** request)
{
    return MPIDI_UCX_send(buf, count, datatype, rank, tag, comm, context_offset,
                          addr, request, 1, 1);
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_cancel_send(MPIR_Request * sreq)
{
    if (!MPIR_Request_is_complete(sreq)) {
        MPIDI_UCX_req_send_t *p_send = &MPIDI_UCX_REQ(sreq).send;
        ucp_request_cancel(MPIDI_UCX_global.worker, p_send->ucp_request);
    }

    return MPI_SUCCESS;
}

#endif /* UCX_SEND_H_INCLUDED */
