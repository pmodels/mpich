/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpidimpl.h"
#include "mpidch4r.h"
#include "mpidig_am_part_callbacks.h"
#include "mpidig_am_part_utils.h"

static void part_rreq_update_sinfo(MPIR_Request * rreq, MPIDIG_part_send_init_msg_t * msg_hdr)
{
    MPIDIG_PART_REQUEST(rreq, u.recv).sdata_size = msg_hdr->data_sz;
    MPIDIG_PART_REQUEST(rreq, peer_req_ptr) = msg_hdr->sreq_ptr;
}

/* Called when data transfer completes on receiver */
static int part_send_data_target_cmpl_cb(MPIR_Request * rreq)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_PUT_DT_TARGET_CMPL_CB);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_PUT_DT_TARGET_CMPL_CB);

    MPIR_Request *part_rreq = MPIDIG_REQUEST(rreq, req->part_am_req.part_req_ptr);
    MPIR_Assert(part_rreq);

    MPIDIG_recv_finish(rreq);

    /* Internally set partitioned rreq complete via completion_notification. */
    MPID_Request_complete(rreq);

    /* Reset part_rreq status to inactive */
    MPL_atomic_store_int(&MPIDIG_PART_REQUEST(part_rreq, status), MPIDIG_PART_RREQ_INACTIVE);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_PUT_DT_TARGET_CMPL_CB);
    return mpi_errno;
}

/* Callback used on sender, triggered when the data transfer AM completes.
 * It completes the local send request. */
int MPIDIG_part_send_data_origin_cb(MPIR_Request * sreq)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIG_PART_SEND_DATA_ORIGIN_CB);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIG_PART_SEND_DATA_ORIGIN_CB);

    MPIR_Request *part_sreq = MPIDIG_REQUEST(sreq, req->part_am_req.part_req_ptr);
    MPIR_Assert(part_sreq);

    /* Internally set partitioned sreq complete via completion_notification. */
    MPID_Request_complete(sreq);

    /* Reset part_sreq status to inactive */
    MPL_atomic_store_int(&MPIDIG_PART_REQUEST(part_sreq, status), MPIDIG_PART_SREQ_INACTIVE);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIG_PART_SEND_DATA_ORIGIN_CB);
    return mpi_errno;
}

/* Callback used on receiver, triggered when received the send_init AM.
 * It tries to match with a local posted part_rreq or store as unexpected. */
int MPIDIG_part_send_init_target_msg_cb(int handler_id, void *am_hdr, void *data,
                                        MPI_Aint in_data_sz, int is_local, int is_async,
                                        MPIR_Request ** req)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIG_PART_SEND_INIT_TARGET_MSG_CB);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIG_PART_SEND_INIT_TARGET_MSG_CB);

    MPIDIG_part_send_init_msg_t *msg_hdr = (MPIDIG_part_send_init_msg_t *) am_hdr;
    MPIR_Request *posted_req = NULL;
    posted_req = MPIDIG_part_dequeue(msg_hdr->src_rank, msg_hdr->tag, msg_hdr->context_id,
                                     &MPIDI_global.part_posted_list);
    if (posted_req) {
        part_rreq_update_sinfo(posted_req, msg_hdr);
        MPIDIG_part_match_rreq(posted_req);

        /* If rreq matches and local start has been called, notify sender CTS */
        if (MPIDIG_PART_REQ_INC_FETCH_STATUS(posted_req) == MPIDIG_PART_REQ_CTS) {
            mpi_errno = MPIDIG_part_issue_cts(posted_req);
        }
    } else {
        MPIR_Request *unexp_req = NULL;

        /* Create temporary unexpected request, freed when matched with a precv_init. */
        unexp_req = MPIR_Request_create_from_pool(MPIR_REQUEST_KIND__PART_RECV, 0, 1);
        MPIR_ERR_CHKANDSTMT(unexp_req == NULL, mpi_errno, MPIX_ERR_NOREQ, goto fn_fail,
                            "**nomemreq");

        MPIDI_PART_REQUEST(unexp_req, rank) = msg_hdr->src_rank;
        MPIDI_PART_REQUEST(unexp_req, tag) = msg_hdr->tag;
        MPIDI_PART_REQUEST(unexp_req, context_id) = msg_hdr->context_id;
        part_rreq_update_sinfo(unexp_req, msg_hdr);

        MPIDIG_part_enqueue(unexp_req, &MPIDI_global.part_unexp_list);
    }

    if (is_async)
        *req = NULL;

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIG_PART_SEND_INIT_TARGET_MSG_CB);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

/* Callback used on sender, triggered when received CTS from receiver.
 * It stores rreq pointer, updates local status, and optionally initiates
 * data transfer if all partitions have been marked as ready.
 */
int MPIDIG_part_cts_target_msg_cb(int handler_id, void *am_hdr, void *data,
                                  MPI_Aint in_data_sz, int is_local, int is_async,
                                  MPIR_Request ** req)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIG_PART_CTS_TARGET_MSG_CB);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIG_PART_CTS_TARGET_MSG_CB);

    MPIDIG_part_cts_msg_t *msg_hdr = (MPIDIG_part_cts_msg_t *) am_hdr;
    MPIR_Request *part_sreq = msg_hdr->sreq_ptr;
    MPIR_Assert(part_sreq);

    MPIDIG_PART_REQUEST(part_sreq, peer_req_ptr) = msg_hdr->rreq_ptr;
    MPIDIG_PART_REQ_INC_FETCH_STATUS(part_sreq);
    mpi_errno = MPIDIG_post_pready(part_sreq, is_local);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIG_PART_CTS_TARGET_MSG_CB);
    return mpi_errno;
}

/* Callback on receiver, triggered when received actual data from sender.
 * It copies data into recvbuf and set local part_rreq complete. */
int MPIDIG_part_send_data_target_msg_cb(int handler_id, void *am_hdr, void *data,
                                        MPI_Aint in_data_sz, int is_local, int is_async,
                                        MPIR_Request ** req)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIG_PART_SEND_DATA_TARGET_MSG_CB);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIG_PART_SEND_DATA_TARGET_MSG_CB);

    MPIDIG_part_send_data_msg_t *msg_hdr = (MPIDIG_part_send_data_msg_t *) am_hdr;
    MPIR_Request *part_rreq = msg_hdr->rreq_ptr;
    MPIR_Assert(part_rreq);

    /* Erroneous if received data with a non-CTS rreq */
    MPIDIG_PART_CHECK_RREQ_CTS(part_rreq);

    /* Setup an AM rreq to receive data */
    MPIR_Request *rreq = MPIDIG_request_create(MPIR_REQUEST_KIND__PART, 1);
    MPIR_ERR_CHKANDSTMT(rreq == NULL, mpi_errno, MPIX_ERR_NOREQ, goto fn_fail, "**nomemreq");

    MPIDIG_REQUEST(rreq, buffer) = MPIDI_PART_REQUEST(part_rreq, buffer);
    MPIDIG_REQUEST(rreq, datatype) = MPIDI_PART_REQUEST(part_rreq, datatype);
    /* count of entire recv buffer. */
    MPIDIG_REQUEST(rreq, count) = MPIDI_PART_REQUEST(part_rreq, count) *
        part_rreq->u.part.partitions;
    MPIDIG_REQUEST(rreq, req->target_cmpl_cb) = part_send_data_target_cmpl_cb;

    /* Set part_rreq complete when am request completes but not decrease part_rreq refcnt */
    rreq->completion_notification = &part_rreq->cc;
    /* Will update part_sreq status when the AM request completes.
     * TODO: can we get rid of the pointer? */
    MPIDIG_REQUEST(rreq, req->part_am_req.part_req_ptr) = part_rreq;

    /* Data may be segmented in pipeline AM type; initialize with total send size */
    MPIDIG_recv_type_init(MPIDIG_PART_REQUEST(part_rreq, u.recv).sdata_size, rreq);

    if (is_async) {
        *req = rreq;
    } else {
        MPIDIG_recv_copy(data, rreq);
        MPIDIG_REQUEST(rreq, req->target_cmpl_cb) (rreq);
    }

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIG_PART_SEND_DATA_TARGET_MSG_CB);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
