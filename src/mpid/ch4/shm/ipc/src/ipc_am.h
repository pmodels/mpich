/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef IPC_AM_H_INCLUDED
#define IPC_AM_H_INCLUDED

#include "ipc_p2p.h"

MPL_STATIC_INLINE_PREFIX int MPIDI_IPC_am_recv_rdma_read(void *lmt_msg, size_t recv_data_sz,
                                                         MPIR_Request * rreq)
{
    void *flattened_type;
    MPIDI_IPC_ctrl_send_lmt_rts_t *slmt_rts_hdr = (MPIDI_IPC_ctrl_send_lmt_rts_t *) lmt_msg;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_IPC_AM_RECV_RDMA);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_IPC_AM_RECV_RDMA);

    MPIDIG_REQUEST(rreq, rank) = slmt_rts_hdr->src_rank;
    MPIDIG_REQUEST(rreq, tag) = slmt_rts_hdr->tag;
    MPIDIG_REQUEST(rreq, context_id) = slmt_rts_hdr->context_id;

    if (slmt_rts_hdr->flattened_type_size > 0) {
        flattened_type = slmt_rts_hdr->flattened_type;
    } else {
        flattened_type = NULL;
    }

    /* Complete IPC receive */
    mpi_errno = MPIDI_IPCI_handle_lmt_recv(slmt_rts_hdr->ipc_type,
                                           slmt_rts_hdr->ipc_handle,
                                           slmt_rts_hdr->data_sz, slmt_rts_hdr->sreq_ptr,
                                           flattened_type, rreq);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_IPC_AM_RECV_RDMA);
    return mpi_errno;
}

#endif /* IPC_AM_H_INCLUDED */
