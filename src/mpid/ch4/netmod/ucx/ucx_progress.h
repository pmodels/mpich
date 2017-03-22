/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2016 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 *  Portions of this code were written by Mellanox Technologies Ltd.
 *  Copyright (C) Mellanox Technologies Ltd. 2016. ALL RIGHTS RESERVED
 */
#ifndef UCX_PROGRESS_H_INCLUDED
#define UCX_PROGRESS_H_INCLUDED

#include "ucx_impl.h"
//#include "events.h"

static inline int MPIDI_UCX_am_handler(void *msg, size_t msg_sz)
{
    int mpi_errno;
    MPIR_Request *rreq = NULL;
    void *p_data;
    void *in_data;
    size_t data_sz, in_data_sz;
    MPIDIG_am_target_cmpl_cb target_cmpl_cb = NULL;
    struct iovec *iov;
    int i, is_contig, iov_len;
    size_t done, curr_len, rem;
    MPIDI_UCX_am_header_t *msg_hdr = (MPIDI_UCX_am_header_t *) msg;

    p_data = in_data = (char *) msg_hdr->payload + (msg_sz - msg_hdr->data_sz - sizeof(*msg_hdr));
    in_data_sz = data_sz = msg_hdr->data_sz;

    MPIDIG_global.target_msg_cbs[msg_hdr->handler_id] (msg_hdr->handler_id, msg_hdr->payload,
                                                       &p_data, &data_sz, &is_contig,
                                                       &target_cmpl_cb, &rreq);

    if (!rreq)
        goto fn_exit;

    if ((!p_data || !data_sz) && target_cmpl_cb) {
        MPIR_STATUS_SET_COUNT(rreq->status, data_sz);
        target_cmpl_cb(rreq);
        goto fn_exit;
    }

    if (is_contig) {
        if (in_data_sz > data_sz) {
            rreq->status.MPI_ERROR = MPI_ERR_TRUNCATE;
        }
        else {
            rreq->status.MPI_ERROR = MPI_SUCCESS;
        }

        data_sz = MPL_MIN(data_sz, in_data_sz);
        MPIR_Memcpy(p_data, in_data, data_sz);
        MPIR_STATUS_SET_COUNT(rreq->status, data_sz);
    }
    else {
        done = 0;
        rem = in_data_sz;
        iov = (struct iovec *) p_data;
        iov_len = data_sz;

        for (i = 0; i < iov_len && rem > 0; i++) {
            curr_len = MPL_MIN(rem, iov[i].iov_len);
            MPIR_Memcpy(iov[i].iov_base, (char *) in_data + done, curr_len);
            rem -= curr_len;
            done += curr_len;
        }

        if (rem) {
            rreq->status.MPI_ERROR = MPI_ERR_TRUNCATE;
        }
        else {
            rreq->status.MPI_ERROR = MPI_SUCCESS;
        }

        MPIR_STATUS_SET_COUNT(rreq->status, done);
    }

    if (target_cmpl_cb) {
        target_cmpl_cb(rreq);
    }

  fn_exit:
    return mpi_errno;
}

static inline void MPIDI_UCX_Handle_am_recv(void *request, ucs_status_t status,
                                            ucp_tag_recv_info_t * info)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_UCX_ucp_request_t *ucp_request = (MPIDI_UCX_ucp_request_t *) request;

    if (status == UCS_ERR_CANCELED) {
        goto fn_exit;
    }
  fn_exit:
    return;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_NM_progress
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_NM_progress(void *netmod_context, int blocking)
{
    int mpi_errno = MPI_SUCCESS;
    ucp_tag_recv_info_t info;
    MPIDI_UCX_ucp_request_t *ucp_request;
    void *am_buf;
    ucp_tag_message_h message_handle;
    /* check for active messages */
    message_handle =
        ucp_tag_probe_nb(MPIDI_UCX_global.worker, MPIDI_UCX_AM_TAG, MPIDI_UCX_AM_TAG, 1, &info);
    while (message_handle) {
        am_buf = MPL_malloc(info.length);
        ucp_request = (MPIDI_UCX_ucp_request_t *) ucp_tag_msg_recv_nb(MPIDI_UCX_global.worker,
                                                                      am_buf,
                                                                      info.length,
                                                                      ucp_dt_make_contig(1),
                                                                      message_handle,
                                                                      &MPIDI_UCX_Handle_am_recv);
        while (!ucp_request_is_completed(ucp_request)) {
            ucp_worker_progress(MPIDI_UCX_global.worker);
        }

        ucp_request_release(ucp_request);
        MPIDI_UCX_am_handler(am_buf, info.length);
        MPL_free(am_buf);
        message_handle =
            ucp_tag_probe_nb(MPIDI_UCX_global.worker, MPIDI_UCX_AM_TAG,
                             MPIDI_UCX_AM_TAG, 1, &info);

    }

    ucp_worker_progress(MPIDI_UCX_global.worker);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#endif /* UCX_PROGRESS_H_INCLUDED */
