/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpidimpl.h"
#include "ucx_impl.h"
#include "ucx_part_utils.h"

typedef struct MPIDI_UCX_part_send_init_msg_t {
    int src_rank;
    int tag;
    MPIR_Context_id_t context_id;
    MPI_Request sreq;
    MPI_Aint data_sz;           /* size of entire send data */
} MPIDI_UCX_part_send_init_msg_t;

typedef struct MPIDI_UCX_part_recv_init_msg_t {
    MPI_Request sreq;
    MPI_Request rreq;
} MPIDI_UCX_part_recv_init_msg_t;

/* Callback used on receiver, triggered when received the send_init AM.
 * It tries to match with a local posted part_rreq or store as unexpected. */
int MPIDI_UCX_part_send_init_target_msg_cb(void *am_hdr, void *data, MPI_Aint in_data_sz,
                                           uint32_t attr, MPIR_Request ** req)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_ENTER;

    MPIDI_UCX_part_send_init_msg_t *msg_hdr = am_hdr;
    MPIR_Request *posted_req =
        MPIDIG_rreq_dequeue(msg_hdr->src_rank, msg_hdr->tag, msg_hdr->context_id,
                            &MPIDI_global.part_posted_list, MPIDIG_PART);
    if (posted_req) {
        MPIDI_UCX_PART_REQ(posted_req).peer_req = msg_hdr->sreq;
        MPIDI_UCX_PART_REQ(posted_req).data_sz = msg_hdr->data_sz;

        MPIDI_UCX_precv_matched(posted_req);
        if (MPIR_Part_request_is_active(posted_req)) {
            MPIDI_UCX_part_recv(posted_req);
        }
    } else {
        MPIR_Request *unexp_req;

        /* Create temporary unexpected request, freed when matched with a precv_init. */
        MPIDI_CH4_REQUEST_CREATE(unexp_req, MPIR_REQUEST_KIND__PART_RECV, 0, 1);
        MPIR_ERR_CHKANDSTMT(unexp_req == NULL, mpi_errno, MPIX_ERR_NOREQ, goto fn_fail,
                            "**nomemreq");

        MPIDI_PART_REQUEST(unexp_req, u.recv.source) = msg_hdr->src_rank;
        MPIDI_PART_REQUEST(unexp_req, u.recv.tag) = msg_hdr->tag;
        MPIDI_PART_REQUEST(unexp_req, u.recv.context_id) = msg_hdr->context_id;
        MPIDI_UCX_PART_REQ(unexp_req).peer_req = msg_hdr->sreq;
        MPIDI_UCX_PART_REQ(unexp_req).data_sz = msg_hdr->data_sz;

        MPIDIG_enqueue_request(unexp_req, &MPIDI_global.part_unexp_list, MPIDIG_PART);
    }

    if (attr & MPIDIG_AM_ATTR__IS_ASYNC) {
        *req = NULL;
    }

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

/* Callback used on sender to set receiver information */
int MPIDI_UCX_part_recv_init_target_msg_cb(void *am_hdr, void *data, MPI_Aint in_data_sz,
                                           uint32_t attr, MPIR_Request ** req)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_ENTER;

    MPIDI_UCX_part_recv_init_msg_t *msg_hdr = am_hdr;
    MPIR_Request *part_sreq;
    MPIR_Request_get_ptr(msg_hdr->sreq, part_sreq);
    MPIR_Assert(part_sreq);

    MPIDI_UCX_PART_REQ(part_sreq).peer_req = msg_hdr->rreq;
    int incomplete;
    MPIR_cc_decr(&MPIDI_UCX_PART_REQ(part_sreq).ready_cntr, &incomplete);
    if (!incomplete) {
        MPIDI_UCX_part_send(part_sreq);
    }

    /* release handshake reference */
    MPIR_Request_free_unsafe(part_sreq);

    MPIR_FUNC_EXIT;
    return mpi_errno;
}

int MPIDI_UCX_part_send_init_hdr(MPIR_Request * request, int tag)
{
    /* Initiate handshake with receiver for message matching */
    MPIDI_UCX_part_send_init_msg_t am_hdr;
    am_hdr.src_rank = request->comm->rank;
    am_hdr.tag = tag;
    am_hdr.context_id = request->comm->context_id;
    am_hdr.sreq = request->handle;

    MPI_Aint dtype_size;
    MPIR_Datatype_get_size_macro(MPIDI_PART_REQUEST(request, datatype), dtype_size);
    am_hdr.data_sz = dtype_size * MPIDI_PART_REQUEST(request, count) * request->u.part.partitions;      /* count is per partition */

    return MPIDI_NM_am_send_hdr(MPIDI_PART_REQUEST(request, u.send.dest), request->comm,
                                MPIDI_UCX_PART_SEND_INIT, &am_hdr, sizeof(am_hdr), 0, 0);
}

int MPIDI_UCX_part_recv_init_hdr(MPIR_Request * request)
{
    /* Send matching info to sender */
    MPIDI_UCX_part_recv_init_msg_t am_hdr;
    am_hdr.sreq = MPIDI_UCX_PART_REQ(request).peer_req;
    am_hdr.rreq = request->handle;

    return MPIDI_NM_am_send_hdr(MPIDI_PART_REQUEST(request, u.recv.source), request->comm,
                                MPIDI_UCX_PART_RECV_INIT, &am_hdr, sizeof(am_hdr), 0, 0);
}

void MPIDI_UCX_part_am_init(void)
{
    MPIDIG_am_reg_cb(MPIDI_UCX_PART_SEND_INIT, NULL, &MPIDI_UCX_part_send_init_target_msg_cb);
    MPIDIG_am_reg_cb(MPIDI_UCX_PART_RECV_INIT, NULL, &MPIDI_UCX_part_recv_init_target_msg_cb);
}
