/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2016 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 *  Portions of this code were written by Mellanox Technologies Ltd.
 *  Copyright (C) Mellanox Technologies Ltd. 2016. ALL RIGHTS RESERVED
 */

#include "mpidimpl.h"
#include "ucx_impl.h"

static void *am_buf;

static void handle_am_recv(void *request, ucs_status_t status, ucp_tag_recv_info_t * info)
{
    MPIDI_am_run_handler(am_buf, info->length);
}

int MPIDI_UCX_progress(int vci, int blocking)
{
    MPIDI_UCX_ucp_request_t *ucp_request;
    ucp_tag_message_h message_handle;
    ucp_tag_recv_info_t info;

    /* check for active messages */
    while (message_handle =
           ucp_tag_probe_nb(MPIDI_UCX_global.worker, MPIDI_UCX_AM_TAG, MPIDI_UCX_AM_TAG, 1,
                            &info)) {
        am_buf = MPL_malloc(info.length, MPL_MEM_BUFFER);
        ucp_request = (MPIDI_UCX_ucp_request_t *) ucp_tag_msg_recv_nb(MPIDI_UCX_global.worker,
                                                                      am_buf,
                                                                      info.length,
                                                                      ucp_dt_make_contig(1),
                                                                      message_handle,
                                                                      &handle_am_recv);

        /* block until message handler is called to ensure ordering */
        while (!ucp_request_is_completed(ucp_request)) {
            ucp_worker_progress(MPIDI_UCX_global.worker);
        }

        MPL_free(am_buf);
        ucp_request_release(ucp_request);
    }

    ucp_worker_progress(MPIDI_UCX_global.worker);

    return MPI_SUCCESS;
}
