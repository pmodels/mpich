/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpidimpl.h"
#include "mpidch4r.h"
#include "mpidig_part_callbacks.h"
#include "mpidig_part_utils.h"

/* Called when data transfer completes on receiver */
static int part_send_data_target_cmpl_cb(MPIR_Request * rreq)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_ENTER;
    MPIR_Assert(rreq->kind == MPIR_REQUEST_KIND__PART);
    MPIR_Assert(MPIDIG_REQUEST(rreq, req->part_am_req.part_req_ptr));

    MPIDIG_recv_finish(rreq);

    /* need to tag the given partition as ready the counter must be 0 now */
    int incomplete;
    MPIR_cc_decr(MPIDIG_REQUEST(rreq, req->part_am_req.cc_part_ptr), &incomplete);
    MPIR_Assert(!incomplete);

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

    MPIR_Assert(sreq->kind == MPIR_REQUEST_KIND__PART);
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
    /* try to find a request with the user provided comm, rank, and tag */
    posted_req = MPIDIG_rreq_dequeue(msg_hdr->src_rank, msg_hdr->tag, msg_hdr->context_id,
                                     &MPIDI_global.part_posted_list, MPIDIG_PART);

    if (posted_req) {
        /* update and match the received request */
        MPIDIG_part_rreq_update_sinfo(posted_req, msg_hdr);
        MPIDIG_part_rreq_matched(posted_req);

        /* If rreq matches and local start has been called, notify sender CTS */
        if (MPIR_Part_request_is_active(posted_req)) {
            // reset the counter per partition
            MPIDIG_part_rreq_reset_cc_part(posted_req);

            // set the cc value to the number of partitions
            const int msg_part = MPIDIG_PART_REQUEST(posted_req, u.recv.msg_part);
            MPIR_Assert(msg_part >= 0);
            MPIR_cc_set(posted_req->cc_ptr, msg_part);

            // notify the sender we are now ready
            mpi_errno = MPIDIG_part_issue_cts(posted_req);
        }

        /* release handshake reference */
        // TG: why?? is it because we get a copy?
        MPIR_Request_free_unsafe(posted_req);
    } else {
        MPIR_Request *unexp_req = NULL;

        /* Create temporary unexpected request, freed when matched with a precv_init.
         * This request will be kept internally till the user calls MPI_Precv_init
         * */
        MPIDI_CH4_REQUEST_CREATE(unexp_req, MPIR_REQUEST_KIND__PART_RECV, 0, 1);
        MPIR_ERR_CHKANDSTMT(unexp_req == NULL, mpi_errno, MPIX_ERR_NOREQ, goto fn_fail,
                            "**nomemreq");

        MPIDI_PART_REQUEST(unexp_req, u.recv.source) = msg_hdr->src_rank;
        MPIDI_PART_REQUEST(unexp_req, u.recv.tag) = msg_hdr->tag;
        MPIDI_PART_REQUEST(unexp_req, u.recv.context_id) = msg_hdr->context_id;

        // store send_dsize, peer_req_ptr, msg_part
        MPIDIG_part_rreq_update_sinfo(unexp_req, msg_hdr);

        MPIDIG_enqueue_request(unexp_req, &MPIDI_global.part_unexp_list, MPIDIG_PART);
    }

    fflush(stdout);

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
    MPIR_Assert(part_sreq->kind == MPIR_REQUEST_KIND__PART_SEND);

    /* detect if it's the first CTS that we receive.
     * if so then we assign the value to peer_prt and msg_part*/
    const bool is_first_cts = (!MPIDIG_PART_REQUEST(part_sreq, peer_req_ptr));
    if (is_first_cts) {
        MPIDIG_PART_REQUEST(part_sreq, peer_req_ptr) = msg_hdr->rreq_ptr;
        MPIDIG_PART_REQUEST(part_sreq, u.send.msg_part) = msg_hdr->msg_part;

#ifndef NDEBUG
        /* make sure we don't split up a datatype */
        MPI_Aint count;
        MPIR_Datatype_get_size_macro(MPIDI_PART_REQUEST(part_sreq, datatype), count);
        count *= part_sreq->u.part.partitions * MPIDI_PART_REQUEST(part_sreq, count);
        MPIR_Assert(count % MPIDIG_PART_REQUEST(part_sreq, u.send.msg_part) == 0);
#endif
    }
    MPIR_Assert(MPIDIG_PART_REQUEST(part_sreq, peer_req_ptr) == msg_hdr->rreq_ptr);
    MPIR_Assert(MPIDIG_PART_REQUEST(part_sreq, u.send.msg_part) == msg_hdr->msg_part);

    /* reset the correct cc value for the number of actually sent msgs */
    MPIR_Assert(MPIR_cc_get(MPIDIG_PART_REQUEST(part_sreq, u.send.cc_send)) == 0);
    MPIR_cc_set(&MPIDIG_PART_REQUEST(part_sreq, u.send.cc_send), msg_hdr->msg_part);

    /* decrements the counter of all the partitions at once because a msg
     * might depend on multiple partitions */
    const int n_part = part_sreq->u.part.partitions;
    MPIR_cc_t *cc_part = MPIDIG_PART_REQUEST(part_sreq, u.send.cc_part);
    MPIR_Assert(n_part >= 0);
    for (int i = 0; i < n_part; ++i) {
        MPIR_cc_dec(&cc_part[i]);
    }

    /* if the request is active (i.e. has been started) then check if we can send something */
    const bool is_active = MPIR_Part_request_is_active(part_sreq);
    if (is_active) {
        if (is_first_cts) {
            /* if the request is active and it's the first CTS then the correct cc value
             * was unknown when activating the request and we have to set it */
            MPIR_cc_set(part_sreq->cc_ptr, msg_hdr->msg_part);
        }
        /* might have partitions that are ready to be sent */
        const int msg_part = MPIDIG_PART_REQUEST(part_sreq, u.send.msg_part);
        mpi_errno = MPIDIG_part_issue_msg_if_ready(0, msg_part, part_sreq, MPIDIG_PART_REPLY);
        MPIR_ERR_CHECK(mpi_errno);
    }

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

/* Callback on receiver, triggered when received actual data from sender. It copies data into recvbuf and set local part_rreq complete. */
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

    /* get the right partition location */
    const int imsg = msg_hdr->imsg;
    const int msg_part = MPIDIG_PART_REQUEST(part_rreq, u.recv.msg_part);
    MPIR_Assert(imsg >= 0);
    MPIR_Assert(imsg < msg_part);

    /* the buffer is the start address of the user's buffer, the offset is computed
     * below and depends on the GCD approach taken*/
    MPIDIG_REQUEST(rreq, buffer) = MPIDI_PART_REQUEST(part_rreq, buffer);
    MPIDIG_REQUEST(rreq, datatype) = MPIDI_PART_REQUEST(part_rreq, datatype);
    MPIDIG_REQUEST(rreq, req->target_cmpl_cb) = part_send_data_target_cmpl_cb;

    /* the start/end of the receive might be in the middle of a datatype.
     * the dsize is the only accurate measure we have and is sufficient with the offset
     * the count value contains the total number of datatype which is valid*/
    MPI_Aint msg_size;
    MPI_Aint count = MPIDI_PART_REQUEST(part_rreq, count) * part_rreq->u.part.partitions;
    MPIR_Datatype_get_size_macro(MPIDI_PART_REQUEST(part_rreq, datatype), msg_size);
    MPIR_Assert((count * msg_size) % msg_part == 0);
    msg_size = (msg_size * count) / msg_part;

    /* all the msgs are the same size = dsize */
    MPIDIG_REQUEST(rreq, offset) = imsg * msg_size;
    /* the count must be the total number of datatypes in the buffer */
    MPIDIG_REQUEST(rreq, count) = count;

    /*register the cc_part ptr to complete the partition's counter as well once the callback is called */
    MPIR_cc_t *cc_part = MPIDIG_PART_REQUEST(part_rreq, u.recv.cc_part);
    MPIR_Assert(MPIR_cc_get(cc_part[imsg]) == MPIDIG_PART_STATUS_RECV_INIT);
    MPIDIG_REQUEST(rreq, req->part_am_req.cc_part_ptr) = cc_part + imsg;

    /* Set part_rreq complete when am request completes but not decrease part_rreq refcnt */
    rreq->dev.completion_notification = &part_rreq->cc;
    /* Will update part_sreq status when the AM request completes.
     * TODO: can we get rid of the pointer? */
    MPIDIG_REQUEST(rreq, req->part_am_req.part_req_ptr) = part_rreq;


    /* Data may be segmented in pipeline AM type; initialize with total send size */
    MPIDIG_recv_type_init(msg_size, rreq);
    if (attr & MPIDIG_AM_ATTR__IS_ASYNC) {
        *req = rreq;
    } else {
        MPIDIG_recv_copy(data, rreq);
        MPIDIG_REQUEST(rreq, req->target_cmpl_cb)
            (rreq);
    }

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
