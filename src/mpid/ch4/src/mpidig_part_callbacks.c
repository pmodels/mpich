/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpidimpl.h"
#include "mpidch4r.h"
#include "mpidig_part_callbacks.h"
#include "mpidig_part_utils.h"
#include "mpidpre.h"

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

    int vci_id;
    if (posted_req) {
        /* update and match the received request */
        MPIDIG_part_rreq_update_sinfo(posted_req, msg_hdr);
        MPIDIG_part_rreq_matched(posted_req);

        /* If rreq matches and local start has been called, notify sender CTS */
        vci_id = get_vci_wrapper(posted_req);
        if (MPIR_Part_request_is_active(posted_req)) {
            /* allocate the arrays, the request has been started (cannot be done if not started) */
            MPIDIG_Part_rreq_allocate(posted_req);

            /* set the cc value to the number of partitions */
            const int msg_part = MPIDIG_PART_REQUEST(posted_req, msg_part);
            MPIR_Assert(msg_part >= 0);
            MPIR_cc_set(posted_req->cc_ptr, msg_part);

            if (MPIDIG_PART_DO_TAG(posted_req)) {
                /* in tag matching we issue the recv requests we need to remove the lock from the
                 * callback */
                MPID_THREAD_CS_EXIT(VCI, MPIDI_VCI(vci_id).lock);
                mpi_errno = MPIDIG_part_issue_recv(posted_req);
                MPID_THREAD_CS_ENTER(VCI, MPIDI_VCI(vci_id).lock);
                MPIR_ERR_CHECK(mpi_errno);
            } else {
                /* reset the counter per partition */
                MPIDIG_part_rreq_reset_cc_part(posted_req);
            }

            /* issue the CTS is last to ensure we are fully ready */
            MPIR_Assert(!MPIDIG_Part_rreq_status_has_first_cts(posted_req));
            mpi_errno = MPIDIG_part_issue_cts(posted_req);
            MPIR_ERR_CHECK(mpi_errno);

            MPIDIG_Part_rreq_status_first_cts(posted_req);
        }

        /* release handshake reference */
        // TG: why?? is it because we get a copy?
        MPIR_Request_free_unsafe(posted_req);
    } else {
        vci_id = 0;
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

    /* we must notify progress as the send rqst might not have done it in case of
     * lightweight sends. The progress count must be incremented on the VCI which has
     * been entered from, not the one from the child request */
    // TODO: check this statement!!
    MPL_atomic_fetch_add_int(&MPIDI_VCI(vci_id).progress_count, 1);

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
    const bool is_first_cts = MPIDIG_PART_REQUEST(part_sreq, msg_part) < 0;
    if (unlikely(is_first_cts)) {
        MPIDIG_PART_REQUEST(part_sreq, msg_part) = msg_hdr->msg_part;
        MPIDIG_PART_REQUEST(part_sreq, peer_req_ptr) = msg_hdr->rreq_ptr;

        /* we need to allocate the data associated to the number of actual msgs */
        const int send_part = part_sreq->u.part.partitions;
        const int msg_part = MPIDIG_PART_REQUEST(part_sreq, msg_part);
        MPIDIG_PART_SREQUEST(part_sreq, cc_msg) =
            MPL_malloc(sizeof(MPIR_cc_t) * msg_part, MPL_MEM_OTHER);
        /* each msg can be updated by multiple partitions, but only once! */
        MPIR_cc_t *cc_msg = MPIDIG_PART_SREQUEST(part_sreq, cc_msg);
        for (int i = 0; i < msg_part; ++i) {
            const int ip_lb = MPIDIG_part_idx_lb(i, msg_part, send_part);
            const int ip_ub = MPIDIG_part_idx_ub(i, msg_part, send_part);
            MPIR_cc_set(cc_msg + i, ip_ub - ip_lb);
        }

#ifndef NDEBUG
        /* make sure we don't split up a datatype on the sender side */
        MPI_Aint count;
        MPIR_Datatype_get_size_macro(MPIDI_PART_REQUEST(part_sreq, datatype), count);
        count *= part_sreq->u.part.partitions * MPIDI_PART_REQUEST(part_sreq, count);
        MPIR_Assert(count % MPIDIG_PART_REQUEST(part_sreq, msg_part) == 0);
#endif
    }
    MPIR_Assert(MPIDIG_PART_REQUEST(part_sreq, peer_req_ptr) == msg_hdr->rreq_ptr);
    MPIR_Assert(MPIDIG_PART_REQUEST(part_sreq, msg_part) == msg_hdr->msg_part);

    /* reset the correct cc value for the number of actually sent msgs */
    // FIXME this is NOT the best option as we reset it twice (once at the start and once here)
    MPIR_cc_set(&MPIDIG_PART_SREQUEST(part_sreq, cc_send), msg_hdr->msg_part);

    const int vci_id = get_vci_wrapper(part_sreq);
    const int msg_part = MPIDIG_PART_REQUEST(part_sreq, msg_part);
    const bool is_active = MPIR_Part_request_is_active(part_sreq);
    if (is_active) {
        /* if the request is active then the correct cc value was unknown when activating the
         * request and we have to set it we don't want to reset the value if the request is not
         * active as we would prevent wait to complete if start is never called */
        MPIR_cc_set(part_sreq->cc_ptr, msg_part);
    }

    /* need to decrement the partition counter and send the partitions if ready */
    const int n_part = part_sreq->u.part.partitions;
    MPIR_cc_t *cc_part = MPIDIG_PART_REQUEST(part_sreq, u.send.cc_part);
    MPIR_Assert(n_part >= 0);

    /* we exit the lock only once and come back to it only once as well, cc_part is atomic so
     * it's okay to do it */
    MPID_THREAD_CS_EXIT(VCI, MPIDI_VCI(vci_id).lock);
    for (int i = 0; i < n_part; ++i) {
        int incomplete;
        MPIR_cc_decr(&cc_part[i], &incomplete);

        /* we could makesure that the request is active BUT if the request is not active we will
         * no be able to send anything because the pready counter has not been decreased */
        if (!incomplete) {
            const int msg_lb = MPIDIG_part_idx_lb(i, n_part, msg_part);
#ifndef NDEBUG
            const int msg_ub = MPIDIG_part_idx_ub(i, n_part, msg_part);
            MPIR_Assert(msg_ub - msg_lb == 1);
#endif
            mpi_errno = MPIDIG_part_issue_msg_if_ready(msg_lb, part_sreq, MPIDIG_PART_REPLY);
            MPIR_ERR_CHECK(mpi_errno);
        }
    }
    MPID_THREAD_CS_ENTER(VCI, MPIDI_VCI(vci_id).lock);

    /* we must notify progress as the send rqst might not have done it in case of lightweight
     * sends. The progress count must be incremented on the VCI which has been entered from, not
     * the one from the child request */
    // TODO: check this statement!!
    MPL_atomic_fetch_add_int(&MPIDI_VCI(vci_id).progress_count, 1);

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
    const int msg_part = MPIDIG_PART_REQUEST(part_rreq, msg_part);
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
    MPIR_cc_t *cc_part = MPIDIG_PART_RREQUEST(part_rreq, cc_part);
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
