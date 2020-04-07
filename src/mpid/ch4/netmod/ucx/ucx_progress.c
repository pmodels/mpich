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

static void *am_buf;            /* message buffer has global scope because ucx executes am_handler */
static void am_handler(void *request, ucs_status_t status, ucp_tag_recv_info_t * info)
{
    void *p_data;
    void *in_data;
    size_t data_sz;
    MPIDI_UCX_am_header_t *msg_hdr = (MPIDI_UCX_am_header_t *) am_buf;

    p_data = in_data =
        (char *) msg_hdr->payload + (info->length - msg_hdr->data_sz - sizeof(*msg_hdr));
    data_sz = msg_hdr->data_sz;

    /* note: setting is_local, is_async to 0, 0 */
    MPIDIG_global.target_msg_cbs[msg_hdr->handler_id] (msg_hdr->handler_id, msg_hdr->payload,
                                                       p_data, data_sz, 0, 0, NULL);

}

int MPIDI_UCX_progress(int vci, int blocking)
{
    int mpi_errno = MPI_SUCCESS;
    ucp_tag_recv_info_t info;
    MPIDI_UCX_ucp_request_t *ucp_request;

    /* TODO: test UCX active message APIs instead of layering over tagged */
    while (true) {
        /* check for pending active messages */
        ucp_tag_message_h message_handle =
            ucp_tag_probe_nb(MPIDI_UCX_global.worker, MPIDI_UCX_AM_TAG, MPIDI_UCX_AM_TAG, 1,
                             &info);
        if (message_handle == NULL)
            break;

        /* message is available. allocate a buffer and start receiving it */
        am_buf = MPL_malloc(info.length, MPL_MEM_BUFFER);
        ucp_request = (MPIDI_UCX_ucp_request_t *) ucp_tag_msg_recv_nb(MPIDI_UCX_global.worker,
                                                                      am_buf,
                                                                      info.length,
                                                                      ucp_dt_make_contig(1),
                                                                      message_handle, &am_handler);

        /* block until receive completes and am_handler executes */
        while (!ucp_request_is_completed(ucp_request)) {
            ucp_worker_progress(MPIDI_UCX_global.worker);
        }

        /* free resources for handled message */
        ucp_request_release(ucp_request);
        MPL_free(am_buf);
    }

    ucp_worker_progress(MPIDI_UCX_global.worker);

    return mpi_errno;
}
