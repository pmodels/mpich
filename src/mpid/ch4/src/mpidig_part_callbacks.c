/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpidimpl.h"
#include "mpidch4r.h"
#include "mpidig_part_callbacks.h"
#include "mpidig_part_utils.h"

static void part_rreq_update_sinfo(MPIR_Request * rreq, MPIDIG_part_send_init_msg_t * msg_hdr)
{
    MPIDIG_PART_REQUEST(rreq, u.recv).sdata_size = msg_hdr->data_sz;
    MPIDIG_PART_REQUEST(rreq, peer_req_ptr) = msg_hdr->sreq_ptr;
}

/* Called when data transfer completes on receiver */
static int part_send_data_target_cmpl_cb(MPIR_Request * rreq)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_ENTER;

    MPIR_Assert(MPIDIG_REQUEST(rreq, req->part_am_req.part_req_ptr));

    MPIDIG_recv_finish(rreq);

    /* Internally set partitioned rreq complete via completion_notification. */
    MPID_Request_complete(rreq);

    MPIR_FUNC_EXIT;
    return mpi_errno;
}

/* Callback used on sender, triggered when the data transfer AM completes.
 * It completes the local send request. */
int MPIDIG_part_send_data_origin_cb(MPIR_Request * sreq)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_ENTER;

    MPIR_Assert(MPIDIG_REQUEST(sreq, req->part_am_req.part_req_ptr));

    /* Internally set partitioned sreq complete via completion_notification. */
    MPID_Request_complete(sreq);

    MPIR_FUNC_EXIT;
    return mpi_errno;
}

/* Callback used on receiver, triggered when received the send_init AM.
 * It tries to match with a local posted part_rreq or store as unexpected. */
int MPIDIG_part_send_init_target_msg_cb(void *am_hdr, void *data,
                                        MPI_Aint in_data_sz, uint32_t attr, MPIR_Request ** req)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_ENTER;

    MPIDIG_part_send_init_msg_t *msg_hdr = (MPIDIG_part_send_init_msg_t *) am_hdr;
    MPIR_Request *posted_req = NULL;
    posted_req = MPIDIG_rreq_dequeue(msg_hdr->src_rank, msg_hdr->tag, msg_hdr->context_id,
                                     &MPIDI_global.part_posted_list, MPIDIG_PART);
    if (posted_req) {
        part_rreq_update_sinfo(posted_req, msg_hdr);
        MPIDIG_precv_matched(posted_req);

        /* If rreq matches and local start has been called, notify sender CTS */
        if (MPIR_Part_request_is_active(posted_req)) {
            mpi_errno = MPIDIG_part_issue_cts(posted_req);
        }

        /* release handshake reference */
        MPIR_Request_free_unsafe(posted_req);
    } else {
        MPIR_Request *unexp_req = NULL;

        /* Create temporary unexpected request, freed when matched with a precv_init. */
        MPIDI_CH4_REQUEST_CREATE(unexp_req, MPIR_REQUEST_KIND__PART_RECV, 0, 1);
        MPIR_ERR_CHKANDSTMT(unexp_req == NULL, mpi_errno, MPIX_ERR_NOREQ, goto fn_fail,
                            "**nomemreq");

        MPIDI_PART_REQUEST(unexp_req, u.recv.source) = msg_hdr->src_rank;
        MPIDI_PART_REQUEST(unexp_req, u.recv.tag) = msg_hdr->tag;
        MPIDI_PART_REQUEST(unexp_req, u.recv.context_id) = msg_hdr->context_id;
        part_rreq_update_sinfo(unexp_req, msg_hdr);

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

/* Callback used on sender, triggered when received CTS from receiver.
 * It stores rreq pointer, updates local status, and optionally initiates
 * data transfer if all partitions have been marked as ready.
 */
int MPIDIG_part_cts_target_msg_cb(void *am_hdr, void *data,
                                  MPI_Aint in_data_sz, uint32_t attr, MPIR_Request ** req)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_ENTER;

    MPIDIG_part_cts_msg_t *msg_hdr = (MPIDIG_part_cts_msg_t *) am_hdr;
    MPIR_Request *part_sreq = msg_hdr->sreq_ptr;
    MPIR_Assert(part_sreq);

    MPIDIG_PART_REQUEST(part_sreq, peer_req_ptr) = msg_hdr->rreq_ptr;
    int incomplete;
    MPIR_cc_decr(&MPIDIG_PART_REQUEST(part_sreq, u.send).ready_cntr, &incomplete);
    if (!incomplete) {
        mpi_errno = MPIDIG_part_issue_data(part_sreq, MPIDIG_PART_REPLY);
    }

    MPIR_FUNC_EXIT;
    return mpi_errno;
}

/* Callback on receiver, triggered when received actual data from sender.
 * It copies data into recvbuf and set local part_rreq complete. */
int MPIDIG_part_send_data_target_msg_cb(void *am_hdr, void *data,
                                        MPI_Aint in_data_sz, uint32_t attr, MPIR_Request ** req)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_ENTER;

    MPIDIG_part_send_data_msg_t *msg_hdr = (MPIDIG_part_send_data_msg_t *) am_hdr;
    MPIR_Request *part_rreq = msg_hdr->rreq_ptr;
    MPIR_Assert(part_rreq);

    /* Setup an AM rreq to receive data */
    MPIR_Request *rreq = MPIDIG_request_create(MPIR_REQUEST_KIND__PART, 1, 0, 0);
    MPIR_ERR_CHKANDSTMT(rreq == NULL, mpi_errno, MPIX_ERR_NOREQ, goto fn_fail, "**nomemreq");
    rreq->comm = part_rreq->comm;
    MPIR_Comm_add_ref(rreq->comm);

    MPIDIG_REQUEST(rreq, buffer) = MPIDI_PART_REQUEST(part_rreq, buffer);
    MPIDIG_REQUEST(rreq, datatype) = MPIDI_PART_REQUEST(part_rreq, datatype);
    /* count of entire recv buffer. */
    MPIDIG_REQUEST(rreq, count) = MPIDI_PART_REQUEST(part_rreq, count) *
        part_rreq->u.part.partitions;
    MPIDIG_REQUEST(rreq, req->target_cmpl_cb) = part_send_data_target_cmpl_cb;

    /* Set part_rreq complete when am request completes but not decrease part_rreq refcnt */
    rreq->dev.completion_notification = &part_rreq->cc;
    /* Will update part_sreq status when the AM request completes.
     * TODO: can we get rid of the pointer? */
    MPIDIG_REQUEST(rreq, req->part_am_req.part_req_ptr) = part_rreq;

    /* Data may be segmented in pipeline AM type; initialize with total send size */
    MPIDIG_recv_type_init(MPIDIG_PART_REQUEST(part_rreq, u.recv).sdata_size, rreq);

    if (attr & MPIDIG_AM_ATTR__IS_ASYNC) {
        *req = rreq;
    } else {
        MPIDIG_recv_copy(data, rreq);
        MPIDIG_REQUEST(rreq, req->target_cmpl_cb) (rreq);
    }

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
