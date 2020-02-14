/*
 *  (C) 2019 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 *  Portions of this code were written by Intel Corporation.
 *  Copyright (C) 2011-2019 Intel Corporation.  Intel provides this material
 *  to Argonne National Laboratory subject to Software Grant and Corporate
 *  Contributor License Agreement dated February 8, 2012.
 */

#include "rndv.h"

/* Progress function for the RNDV submodule. This will call the various functions needed to make
 * progress on individual wait elements. */
int MPIDI_POSIX_lmt_progress()
{
    int mpi_errno = MPI_SUCCESS;
    lmt_rndv_wait_element_t *wait_element;
    bool done;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_POSIX_LMT_PROGRESS);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_POSIX_LMT_PROGRESS);

    /* Iterate over the list of all rndv queues and make progress on each if there is a request
     * present. */
    /* TODO - Change this to use a single queue to make polling faster. */
    for (int i = 0; i < MPIDI_POSIX_global.num_local; i++) {
        wait_element = GENERIC_Q_HEAD(rndv_send_queues[i]);
        if (wait_element) {
            mpi_errno = wait_element->progress(wait_element->req, i, &done);
            if (mpi_errno)
                MPIR_ERR_POP(mpi_errno);

            if (done) {
                GENERIC_Q_DEQUEUE(&rndv_send_queues[i], &wait_element, next);
                MPL_free(wait_element);
            }
        }
        wait_element = GENERIC_Q_HEAD(rndv_recv_queues[i]);
        if (wait_element) {
            mpi_errno = wait_element->progress(wait_element->req, i, &done);
            if (mpi_errno)
                MPIR_ERR_POP(mpi_errno);

            if (done) {
                GENERIC_Q_DEQUEUE(&rndv_recv_queues[i], &wait_element, next);
                MPL_free(wait_element);
            }
        }
    }

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_POSIX_LMT_PROGRESS);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

/* Make progress on requests involved in receiving a message
 *
 * req - The request describing the message
 * local_rank - The local rank of the progress from which the message is coming (local rank is the
 *              rank of the processes only present on this node).
 * done - Return value to indicate whether the reqeust is complete
 */
int lmt_rndv_recv_progress(MPIR_Request * req, int local_rank, bool * done)
{
    int mpi_errno = MPI_SUCCESS;
    char *src_buf;
    intptr_t last, expected_last;
    lmt_rndv_copy_buf_t *copy_buf = rndv_recv_copy_bufs[local_rank];
    intptr_t msg_offset = MPIDI_POSIX_AMREQUEST(req, lmt.lmt_msg_offset);
    intptr_t data_sz = MPIDI_POSIX_AMREQUEST(req, lmt.lmt_data_sz);
    intptr_t leftover = MPIDI_POSIX_AMREQUEST(req, lmt.lmt_leftover);
    int buf_num = MPIDI_POSIX_AMREQUEST(req, lmt.lmt_buf_num);

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_LMT_RNDV_RECV_PROGRESS);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_LMT_RNDV_RECV_PROGRESS);

    /* Each time through the progres engine, the receiver will copy out one chunk of the message. */
    if (0 == copy_buf->len[buf_num].val) {
        *done = false;
        goto fn_exit;
    }

    OPA_read_barrier();

    /* Calculate correct starting position based on whether there was leftover in the previous
     * iteration. */
    src_buf = ((char *) copy_buf->buf[buf_num]) - leftover;
    last = expected_last = (data_sz - msg_offset <= leftover + copy_buf->len[buf_num].val) ?
        data_sz : (msg_offset + leftover + copy_buf->len[buf_num].val);

    MPI_Aint actual_unpack_bytes;
    MPIR_Typerep_unpack(src_buf, last - msg_offset, MPIDIG_REQUEST(req, buffer),
                        MPIDIG_REQUEST(req, user_count), MPIDIG_REQUEST(req, datatype),
                        msg_offset, &actual_unpack_bytes);
    last = msg_offset + actual_unpack_bytes;

    /* We had leftover data from the previous buffer so we should now mark that buffer empty */
    if (leftover && buf_num > 0) {
        /* Make sure that any unpacking is done before unsetting the value to prevent another
         * process overwriting. */
        OPA_read_write_barrier();
        copy_buf->len[buf_num - 1].val = 0;
    }

    if (last < expected_last) {
        /* We have leftover data in the buffer that wasn't copied out by the datatype engine */
        char *leftover_ptr = (char *) src_buf + last - msg_offset;
        leftover = expected_last - last;

        /* Ensure that we're not going to overwrite some data in the copy_buf struct that we
         * shoulnd't be touching. */
        MPIR_Assert(leftover <= MPIDU_SHM_CACHE_LINE_LEN);

        if (buf_num == MPIDI_LMT_NUM_BUFS - 1) {
            /* If wrapping back to the starting buffer, copy the data directly to that buffer */
            MPIR_Memcpy(((char *) copy_buf->buf[0]) - leftover, leftover_ptr, leftover);

            /* Make sure that any unpacking is done before unsetting the value to prevent
             * another process overwriting. */
            OPA_read_write_barrier();
            copy_buf->len[buf_num].val = 0;
        } else {
            char tmp_buf[MPIDU_SHM_CACHE_LINE_LEN];

            /* Otherwise, we need to copy to a tempbuf first to make sure the two addresses
             * don't overlap each other. */
            MPIR_Memcpy(tmp_buf, leftover_ptr, leftover);
            MPIR_Memcpy(((char *) copy_buf->buf[buf_num + 1]) - leftover, tmp_buf, leftover);
        }
    } else {
        /* Everything was unpacked so this can be marked as empty */
        leftover = 0;

        /* Make sure that any unpacking is done before unsetting the value to prevent another
         * process overwriting. */
        OPA_read_write_barrier();
        copy_buf->len[buf_num].val = 0;
    }

    msg_offset = last;
    buf_num = (buf_num + 1) % MPIDI_LMT_NUM_BUFS;

    if (last < data_sz) {
        *done = false;
        /* Keep track of the status of the operation so the next time the progress engine is
         * called, it knows where to restart. */
        MPIDI_POSIX_AMREQUEST(req, lmt.lmt_msg_offset) = msg_offset;
        MPIDI_POSIX_AMREQUEST(req, lmt.lmt_data_sz) = data_sz;
        MPIDI_POSIX_AMREQUEST(req, lmt.lmt_buf_num) = buf_num;
        MPIDI_POSIX_AMREQUEST(req, lmt.lmt_leftover) = leftover;
    } else {
        MPIR_Request *match_req = MPIDIG_REQUEST(req, req->rreq.match_req);
        *done = true;
        MPIR_Datatype_release_if_not_builtin(MPIDIG_REQUEST(req, datatype));

        if (MPIDI_POSIX_AMREQUEST(req, req_hdr)) {
            MPL_free(MPIDI_POSIX_AMREQUEST_HDR(req, pack_buffer));
            MPIDI_POSIX_AMREQUEST_HDR(req, pack_buffer) = NULL;
        }

        /* If there is a match request, it probably means that CH4 allocated a request on our
         * behalf and we matched an unexpected request. This happens when using the handoff
         * model. The match request is the one that will be returned to the user so copy over
         * the important information and clean up both requests. */
        if (match_req != NULL) {
            MPIR_Request *match_partner = MPIDI_REQUEST_ANYSOURCE_PARTNER(match_req);
            match_req->status.MPI_SOURCE = req->status.MPI_SOURCE;
            match_req->status.MPI_TAG = req->status.MPI_TAG;
            match_req->status.MPI_ERROR = req->status.MPI_ERROR;
            MPIR_STATUS_SET_COUNT(match_req->status, MPIR_STATUS_GET_COUNT(req->status));
            if (unlikely(match_partner)) {
                /* This was an ANY_SOURCE receive -- we need to cancel NM recv and free it too */
                mpi_errno = MPIDI_NM_mpi_cancel_recv(match_partner);
                if (mpi_errno)
                    MPIR_ERR_POP(mpi_errno);

                MPIDI_REQUEST_ANYSOURCE_PARTNER(match_req) = NULL;
                MPIDI_REQUEST_ANYSOURCE_PARTNER(match_partner) = NULL;
                MPIR_Request_free(match_partner);
            }
            MPIR_Request_add_ref(match_req);
            mpi_errno = MPID_Request_complete(match_req);
            if (mpi_errno)
                MPIR_ERR_POP(mpi_errno);
            /* This is the request that was created at the time we handled the unexpected
             * message. However, because a match_req was created when the handoff to the
             * progress thread was done, this request (`req`) is no longer needed and will not be
             * returned to the user. So decrement the reference counter twice (accomplished by
             * calling MPIR_Request_free twice). */
            MPIR_Request_free(req);
            MPIR_Request_free(req);
        } else {
            mpi_errno = MPID_Request_complete(req);
            if (mpi_errno)
                MPIR_ERR_POP(mpi_errno);
        }
    }

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_LMT_RNDV_RECV_PROGRESS);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}


/* Make progress on requests involved in sending a message
 *
 * req - The request describing the message
 * local_rank - The local rank of the progress from which the message is coming (local rank is the
 *              rank of the processes only present on this node).
 * done - Return value to indicate whether the request is complete
 */
int lmt_rndv_send_progress(MPIR_Request * req, int local_rank, bool * done)
{
    int mpi_errno = MPI_SUCCESS;
    size_t copy_limit, max_pack_bytes, actual_pack_bytes;
    lmt_rndv_copy_buf_t *copy_buf = rndv_send_copy_bufs[local_rank];
    intptr_t msg_offset = MPIDI_POSIX_AMREQUEST(req, lmt.lmt_msg_offset);
    intptr_t data_sz = MPIDI_POSIX_AMREQUEST(req, lmt.lmt_data_sz);
    int buf_num = MPIDI_POSIX_AMREQUEST(req, lmt.lmt_buf_num);

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_LMT_RNDV_SEND_PROGRESS);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_LMT_RNDV_SEND_PROGRESS);

    /* Each time through the progres engine, the receiver will copy out one chunk of the message. */
    if (0 != copy_buf->len[buf_num].val) {
        *done = false;
        goto fn_exit;
    }

    OPA_read_write_barrier();

    /* TODO - Add checks to see whether the sender is present to stay in this progress loop
     * longer while progress is being made. */

    if (data_sz <= MPIR_CVAR_CH4_POSIX_LMT_PIPELINE_THRESHOLD) {
        copy_limit = MPIDI_LMT_SMALL_PIPELINE_MAX;
    } else {
        copy_limit = MPIDI_LMT_COPY_BUF_LEN;
    }

    max_pack_bytes = (data_sz - msg_offset <= copy_limit) ? data_sz - msg_offset : copy_limit;
    MPIR_Typerep_pack(MPIDIG_REQUEST(req, buffer), MPIDIG_REQUEST(req, user_count),
                      MPIDIG_REQUEST(req, datatype), msg_offset,
                      (void *) copy_buf->buf[buf_num], max_pack_bytes, &actual_pack_bytes);

    OPA_write_barrier();

    MPIR_Assign_trunc(copy_buf->len[buf_num].val, actual_pack_bytes, int);
    msg_offset += actual_pack_bytes;

    buf_num = (buf_num + 1) % MPIDI_LMT_NUM_BUFS;

    if (msg_offset < data_sz) {
        *done = false;
        /* Keep track of the status of the operation so the next time the progress engine is
         * called, it knows where to restart. */
        MPIDI_POSIX_AMREQUEST(req, lmt.lmt_msg_offset) = msg_offset;
        MPIDI_POSIX_AMREQUEST(req, lmt.lmt_data_sz) = data_sz;
        MPIDI_POSIX_AMREQUEST(req, lmt.lmt_buf_num) = buf_num;
    } else {
        *done = true;
        MPIR_Datatype_release_if_not_builtin(MPIDIG_REQUEST(req, datatype));

        if (MPIDI_POSIX_AMREQUEST(req, req_hdr)) {
            MPL_free(MPIDI_POSIX_AMREQUEST_HDR(req, pack_buffer));
            MPIDI_POSIX_AMREQUEST_HDR(req, pack_buffer) = NULL;
        }

        mpi_errno = MPID_Request_complete(req);
        if (mpi_errno)
            MPIR_ERR_POP(mpi_errno);
    }

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_LMT_RNDV_SEND_PROGRESS);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
