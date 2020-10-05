/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
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

    int vni = MPIDI_UCX_vci_to_vni(vci);
    if (vni < 0) {
        /* skip if the vci is not for us */
        return MPI_SUCCESS;
    }

    if (vni == 0) {
        /* TODO: test UCX active message APIs instead of layering over tagged */
        while (true) {
            /* check for pending active messages */
            ucp_tag_message_h message_handle;
            message_handle = ucp_tag_probe_nb(MPIDI_UCX_global.ctx[0].worker,
                                              MPIDI_UCX_AM_TAG, MPIDI_UCX_AM_TAG, 1, &info);
            if (message_handle == NULL)
                break;

            /* message is available. allocate a buffer and start receiving it */
            MPL_gpu_malloc_host(&am_buf, info.length);
            ucp_request =
                (MPIDI_UCX_ucp_request_t *) ucp_tag_msg_recv_nb(MPIDI_UCX_global.ctx[0].worker,
                                                                am_buf, info.length,
                                                                ucp_dt_make_contig(1),
                                                                message_handle, &am_handler);

            /* block until receive completes and am_handler executes */
            while (!ucp_request_is_completed(ucp_request)) {
                ucp_worker_progress(MPIDI_UCX_global.ctx[0].worker);
            }

            /* free resources for handled message */
            ucp_request_release(ucp_request);
            MPL_gpu_free_host(am_buf);
        }
    }

    ucp_worker_progress(MPIDI_UCX_global.ctx[vni].worker);

    return mpi_errno;
}
