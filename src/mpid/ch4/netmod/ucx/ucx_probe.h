/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2016 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 *  Portions of this code were written by Mellanox Technologies Ltd.
 *  Copyright (C) Mellanox Technologies Ltd. 2016. ALL RIGHTS RESERVED
 */
#ifndef UCX_PROBE_H_INCLUDED
#define UCX_PROBE_H_INCLUDED

#include "ucx_impl.h"
#include "mpidch4.h"

static inline int MPIDI_NM_mpi_improbe(int source,
                                       int tag,
                                       MPIR_Comm * comm,
                                       int context_offset,
                                       MPIDI_av_entry_t *addr,
                                       int *flag, MPIR_Request ** message, MPI_Status * status)
{
    int mpi_errno = MPI_SUCCESS;
    uint64_t ucp_tag, tag_mask;
    MPI_Aint count;
    ucp_tag_recv_info_t info;
    ucp_tag_message_h message_h;
    MPIR_Request *req = NULL;

    tag_mask = MPIDI_UCX_tag_mask(tag, source);
    ucp_tag = MPIDI_UCX_recv_tag(tag, source, comm->recvcontext_id + context_offset);

    message_h = ucp_tag_probe_nb(MPIDI_UCX_global.worker, ucp_tag, tag_mask, 1, &info);

    if (message_h) {
        *flag = 1;
        req = (MPIR_Request *) MPIR_Request_create(MPIR_REQUEST_KIND__MPROBE);
        MPIR_Assert(req);
        MPIR_Request_add_ref(req);
        MPIDI_UCX_REQ(req).a.message_handler = message_h;

        if (status != MPI_STATUS_IGNORE) {
            status->MPI_SOURCE = MPIDI_UCX_get_source(info.sender_tag);
            status->MPI_TAG = MPIDI_UCX_get_tag(info.sender_tag);
            count = info.length;
            MPIR_STATUS_SET_COUNT(*status, count);
        }
    } else {
        *flag = 0;
    }
    *message = req;

    return mpi_errno;
}


static inline int MPIDI_NM_mpi_iprobe(int source,
                                      int tag,
                                      MPIR_Comm * comm,
                                      int context_offset,
                                      MPIDI_av_entry_t *addr,
                                      int *flag, MPI_Status * status)
{
    int mpi_errno = MPI_SUCCESS;
    uint64_t ucp_tag, tag_mask;
    MPI_Aint count;
    ucp_tag_recv_info_t info;
    ucp_tag_message_h message_h;

    tag_mask = MPIDI_UCX_tag_mask(tag, source);
    ucp_tag = MPIDI_UCX_recv_tag(tag, source, comm->recvcontext_id + context_offset);

    message_h = ucp_tag_probe_nb(MPIDI_UCX_global.worker, ucp_tag, tag_mask, 0, &info);

    if (message_h) {
        *flag = 1;

        if (status != MPI_STATUS_IGNORE) {
            status->MPI_ERROR = MPI_SUCCESS;
            status->MPI_SOURCE = MPIDI_UCX_get_source(info.sender_tag);
            status->MPI_TAG = MPIDI_UCX_get_tag(info.sender_tag);
            count = info.length;
            MPIR_STATUS_SET_COUNT(*status, count);
        }
    } else {
        *flag = 0;
    }

    return mpi_errno;
}

#endif /* UCX_PROBE_H_INCLUDED */
