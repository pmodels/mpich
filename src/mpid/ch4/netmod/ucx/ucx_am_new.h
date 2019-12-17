/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2019 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 *  Portions of this code were written by Mellanox Technologies Ltd.
 *  Copyright (C) Mellanox Technologies Ltd. 2016. ALL RIGHTS RESERVED
 */
#ifndef UCX_AM_H_INCLUDED
#define UCX_AM_H_INCLUDED

#include "ucx_impl.h"

MPL_STATIC_INLINE_PREFIX void MPIDI_UCX_am_send_contig_cb(void *request, ucs_status_t status)
{
    MPIDI_UCX_ucp_request_t *ucp_request = (MPIDI_UCX_ucp_request_t *) request;
    void *context = ucp_request->context;
    int completion_id = req->dev.ch4.am.netmod_am.ucx.completion_id;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_UCX_AM_SEND_CONTIG_CB);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_UCX_AM_SEND_CONTIG_CB);

    MPL_free(req->dev.ch4.am.netmod_am.ucx.pack_buffer);
    req->dev.ch4.am.netmod_am.ucx.pack_buffer = NULL;
    MPIDIG_global.origin_cbs[completion_id] (req);
    ucp_request->req = NULL;

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_UCX_AM_SEND_CONTIG_CB);
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_am_send_contig(int target_rank,
                                                     int handler_id,
                                                     int completion_id,
                                                     const void *hdr,
                                                     size_t hdr_size,
                                                     const void *payload,
                                                     size_t payload_size, void *context)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_AM_SEND_CONTIG);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_AM_SEND_CONTIG);

    ep = MPIDI_UCX_COMM_TO_EP(MPIR_Process.comm_world, target_rank);
    ucx_tag = MPIDI_UCX_init_tag(0, MPIR_Process.comm_world->rank, MPIDI_UCX_AM_TAG);

    /* for now we always pack and send */
    char *pack_buffer = MPL_malloc(hdr_size + payload_size, MPL_MEM_BUFFER);
    MPIR_Memcpy(send_buf, hdr, hdr_size);
    MPIR_Memcpy(send_buf + hdr_size, payload, payload_size);

    ucp_request = (MPIDI_UCX_ucp_request_t *) ucp_tag_send_nb(ep, pack_buffer,
                                                              hdr_size + payload_size,
                                                              ucp_dt_make_contig(1), ucx_tag,
                                                              &MPIDI_UCX_am_send_contig_cb);
    MPIDI_UCX_CHK_REQUEST(ucp_request);

    /* send is done. free all resources and complete the request */
    if (ucp_request == NULL) {
        MPL_free(pack_buffer);
        MPIDI_am_send_complete(completion_id, context);
        goto fn_exit;
    }

    /* save the MPICH context inside the UCP request */
    sreq->dev.ch4.am.netmod_am.ucx.pack_buffer = pack_buffer;
    sreq->dev.ch4.am.netmod_am.ucx.completion_id = completion_id;
    ucp_request->context = context;
    ucp_request_release(ucp_request);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_AM_SEND_CONTIG);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#endif /* UCX_AM_H_INCLUDED */
