/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 *  Portions of this code were written by Intel Corporation.
 *  Copyright (C) 2011-2017 Intel Corporation.  Intel provides this material
 *  to Argonne National Laboratory subject to Software Grant and Corporate
 *  Contributor License Agreement dated February 8, 2012.
 */

#include "mpidimpl.h"
#include "posix_types.h"
#include "posix_am_impl.h"
#include <posix_eager.h>
#include "shm_types.h"
#include "shm_control.h"

/* unused prototypes to supress -Wmissing-prototypes */
int MPIDI_POSIX_progress_test(void);
int MPIDI_POSIX_progress_poke(void);
void MPIDI_POSIX_progress_start(MPID_Progress_state * state);
void MPIDI_POSIX_progress_end(MPID_Progress_state * state);
int MPIDI_POSIX_progress_wait(MPID_Progress_state * state);
int MPIDI_POSIX_progress_register(int (*progress_fn) (int *));
int MPIDI_POSIX_progress_deregister(int id);
int MPIDI_POSIX_progress_activate(int id);
int MPIDI_POSIX_progress_deactivate(int id);

static int progress_recv(int blocking);
static int progress_send(int blocking);

static int progress_recv(int blocking)
{

    MPIDI_POSIX_eager_recv_transaction_t transaction;
    int mpi_errno = MPI_SUCCESS;
    int i;
    int result = MPIDI_POSIX_OK;
    MPIR_Request *rreq = NULL;
    MPIDIG_am_target_cmpl_cb target_cmpl_cb = NULL;
    MPIDI_POSIX_am_request_header_t *curr_rreq_hdr = NULL;
    void *p_data = NULL;
    size_t p_data_sz = 0;
    size_t recv_data_sz = 0;
    size_t in_total_data_sz = 0;
    int iov_done = 0;
    int is_contig;
    void *am_hdr = NULL;
    MPIDI_POSIX_am_header_t *msg_hdr;
    uint8_t *payload;
    size_t payload_left;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_POSIX_PROGRESS_RECV);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_POSIX_PROGRESS_RECV);

    /* Check to see if any new messages are ready for processing from the eager submodule. */
    result = MPIDI_POSIX_eager_recv_begin(&transaction);

    if (MPIDI_POSIX_OK != result) {
        goto fn_exit;
    }

    /* Process the eager message */
    msg_hdr = transaction.msg_hdr;

    payload = transaction.payload;
    payload_left = transaction.payload_sz;

    if (msg_hdr) {
        if (msg_hdr->handler_id == MPIDIG_NEW_AM) {
            /* new am message is garunteed to be within eager limit */
            progress_new_am_recv(transaction.payload, transaction.payload_sz);
            goto fn_exit;
        }
        am_hdr = payload;
        p_data = payload + msg_hdr->am_hdr_sz;

        in_total_data_sz = msg_hdr->data_sz;
        p_data_sz = msg_hdr->data_sz;

        /* This is a SHM internal control header */
        if (msg_hdr->kind == MPIDI_POSIX_AM_HDR_SHM) {
            mpi_errno = MPIDI_SHM_ctrl_dispatch(msg_hdr->handler_id, am_hdr);

            /* TODO: discard payload for now as we only handle header in
             * current internal control protocols. */
            goto recv_commit;
        }

        /* Call the MPIDIG function to handle the initial receipt of the message. This will attempt
         * to match the message (if appropriate) and return a request if the message was matched. */
        MPIDIG_global.target_msg_cbs[msg_hdr->handler_id] (msg_hdr->handler_id,
                                                           am_hdr,
                                                           &p_data,
                                                           &p_data_sz,
                                                           1 /* is_local */ , &is_contig,
                                                           &target_cmpl_cb, &rreq);
        POSIX_TRACE("POSIX AM target callback: handler_id = %d, am_hdr = %p, p_data = %p "
                    "p_data_sz = %lu, is_contig = %d, target_cmpl_cb = %p rreq = %p\n",
                    msg_hdr->handler_id, am_hdr, p_data, p_data_sz, is_contig, target_cmpl_cb,
                    rreq);
        payload += msg_hdr->am_hdr_sz;
        payload_left -= msg_hdr->am_hdr_sz;

        /* We're receiving a new message here. */
        if (rreq) {
            /* zero message size optimization */
            if ((p_data_sz == 0) && (in_total_data_sz == 0)) {

                MPIR_STATUS_SET_COUNT(rreq->status, 0);
                rreq->status.MPI_SOURCE = MPIDIG_REQUEST(rreq, rank);
                rreq->status.MPI_TAG = MPIDIG_REQUEST(rreq, tag);
                rreq->status.MPI_ERROR = MPI_SUCCESS;

                if (target_cmpl_cb) {
                    target_cmpl_cb(rreq);
                }

                MPIDI_POSIX_eager_recv_commit(&transaction);

                goto fn_exit;
            }

            /* Received immediately */
            if (is_contig && (in_total_data_sz == payload_left)) {

                if (in_total_data_sz > p_data_sz) {
                    rreq->status.MPI_ERROR =
                        MPIR_Err_create_code(rreq->status.MPI_ERROR, MPIR_ERR_RECOVERABLE, __func__,
                                             __LINE__, MPI_ERR_TRUNCATE, "**truncate",
                                             "**truncate %d %d %d %d", rreq->status.MPI_SOURCE,
                                             rreq->status.MPI_TAG, p_data_sz, in_total_data_sz);
                }

                recv_data_sz = MPL_MIN(p_data_sz, in_total_data_sz);

                MPIR_STATUS_SET_COUNT(rreq->status, recv_data_sz);
                rreq->status.MPI_SOURCE = MPIDIG_REQUEST(rreq, rank);
                rreq->status.MPI_TAG = MPIDIG_REQUEST(rreq, tag);

                MPIDI_POSIX_eager_recv_memcpy(&transaction, p_data, payload, recv_data_sz);

                /* Call the function to handle the completed receipt of the message. */
                if (target_cmpl_cb) {
                    target_cmpl_cb(rreq);
                }

                MPIDI_POSIX_eager_recv_commit(&transaction);

                MPIDI_POSIX_EAGER_RECV_COMPLETED_HOOK(rreq);

                goto fn_exit;
            }

            /* Allocate aux data */

            MPIDI_POSIX_am_init_req_hdr(NULL, 0, &MPIDI_POSIX_AMREQUEST(rreq, req_hdr), rreq);

            curr_rreq_hdr = MPIDI_POSIX_AMREQUEST(rreq, req_hdr);

            curr_rreq_hdr->cmpl_handler_fn = target_cmpl_cb;
            curr_rreq_hdr->dst_grank = transaction.src_grank;

            if (is_contig) {
                curr_rreq_hdr->iov_ptr = curr_rreq_hdr->iov;

                curr_rreq_hdr->iov_ptr[0].iov_base = p_data;
                curr_rreq_hdr->iov_ptr[0].iov_len = p_data_sz;

                curr_rreq_hdr->iov_num = 1;

                recv_data_sz = p_data_sz;
            } else {
                curr_rreq_hdr->iov_ptr = ((struct iovec *) p_data);

                for (i = 0; i < p_data_sz; i++) {
                    recv_data_sz += curr_rreq_hdr->iov_ptr[i].iov_len;
                }

                curr_rreq_hdr->iov_num = p_data_sz;
            }

            /* Set final request status */

            if (in_total_data_sz > recv_data_sz) {
                rreq->status.MPI_ERROR = MPIR_Err_create_code(rreq->status.MPI_ERROR,
                                                              MPIR_ERR_RECOVERABLE,
                                                              __func__, __LINE__, MPI_ERR_TRUNCATE,
                                                              "**truncate",
                                                              "**truncate %d %d %d %d",
                                                              rreq->status.MPI_SOURCE,
                                                              rreq->status.MPI_TAG, recv_data_sz,
                                                              in_total_data_sz);
            }

            recv_data_sz = MPL_MIN(recv_data_sz, in_total_data_sz);

            MPIR_STATUS_SET_COUNT(rreq->status, recv_data_sz);
            rreq->status.MPI_SOURCE = MPIDIG_REQUEST(rreq, rank);
            rreq->status.MPI_TAG = MPIDIG_REQUEST(rreq, tag);

            curr_rreq_hdr->is_contig = is_contig;
            curr_rreq_hdr->in_total_data_sz = in_total_data_sz;

            /* Make it active */

            MPIR_Assert(MPIDI_POSIX_global.active_rreq[transaction.src_grank] == NULL);

            MPIDI_POSIX_global.active_rreq[transaction.src_grank] = rreq;
        } else {
            MPIDI_POSIX_eager_recv_commit(&transaction);

            goto fn_exit;
        }
    } else {
        /* Fragment handling. Set currently active recv request */

        rreq = MPIDI_POSIX_global.active_rreq[transaction.src_grank];

        curr_rreq_hdr = MPIDI_POSIX_AMREQUEST(rreq, req_hdr);
    }

    curr_rreq_hdr->in_total_data_sz -= payload_left;

    /* Generic handling for contig and non contig data */
    for (i = 0; i < curr_rreq_hdr->iov_num; i++) {
        if (payload_left < curr_rreq_hdr->iov_ptr[i].iov_len) {
            MPIDI_POSIX_eager_recv_memcpy(&transaction,
                                          curr_rreq_hdr->iov_ptr[i].iov_base, payload,
                                          payload_left);

            curr_rreq_hdr->iov_ptr[i].iov_base =
                (char *) curr_rreq_hdr->iov_ptr[i].iov_base + payload_left;
            curr_rreq_hdr->iov_ptr[i].iov_len -= payload_left;

            break;
        }

        MPIDI_POSIX_eager_recv_memcpy(&transaction,
                                      curr_rreq_hdr->iov_ptr[i].iov_base,
                                      payload, curr_rreq_hdr->iov_ptr[i].iov_len);

        payload += curr_rreq_hdr->iov_ptr[i].iov_len;
        payload_left -= curr_rreq_hdr->iov_ptr[i].iov_len;

        iov_done++;
    }

    if (curr_rreq_hdr->iov_num) {
        curr_rreq_hdr->iov_num -= iov_done;
        curr_rreq_hdr->iov_ptr += iov_done;
    }

    if (curr_rreq_hdr->in_total_data_sz == 0) {
        /* All fragments have been received */

        MPIDI_POSIX_global.active_rreq[transaction.src_grank] = NULL;

        if (curr_rreq_hdr->cmpl_handler_fn) {
            curr_rreq_hdr->cmpl_handler_fn(rreq);
        }
    }

  recv_commit:
    MPIDI_POSIX_eager_recv_commit(&transaction);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_POSIX_PROGRESS_RECV);
    return mpi_errno;
}

static int progress_send(int blocking)
{

    int mpi_errno = MPI_SUCCESS;
    int result = MPIDI_POSIX_OK;
    MPIR_Request *sreq = NULL;
    MPIDI_POSIX_am_request_header_t *curr_sreq_hdr = NULL;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_POSIX_PROGRESS_SEND);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_POSIX_PROGRESS_SEND);

    progress_new_am_send();

    if (MPIDI_POSIX_global.postponed_queue) {
        /* Drain postponed queue */
        curr_sreq_hdr = MPIDI_POSIX_global.postponed_queue;

        POSIX_TRACE("Queue OUT HDR [ POSIX AM [handler_id %" PRIu64 ", am_hdr_sz %" PRIu64
                    ", data_sz %" PRIu64 ", seq_num = %d], request=%p] to %d\n",
                    curr_sreq_hdr->msg_hdr ? curr_sreq_hdr->msg_hdr->handler_id : (uint64_t) - 1,
                    curr_sreq_hdr->msg_hdr ? curr_sreq_hdr->msg_hdr->am_hdr_sz : (uint64_t) - 1,
                    curr_sreq_hdr->msg_hdr ? curr_sreq_hdr->msg_hdr->data_sz : (uint64_t) - 1,
#ifdef POSIX_AM_DEBUG
                    curr_sreq_hdr->msg_hdr ? curr_sreq_hdr->msg_hdr->seq_num : -1,
#else /* POSIX_AM_DEBUG */
                    -1,
#endif /* POSIX_AM_DEBUG */
                    curr_sreq_hdr->request, curr_sreq_hdr->dst_grank);

        result = MPIDI_POSIX_eager_send(curr_sreq_hdr->dst_grank,
                                        &curr_sreq_hdr->msg_hdr,
                                        &curr_sreq_hdr->iov_ptr, &curr_sreq_hdr->iov_num);

        if ((MPIDI_POSIX_NOK == result) || curr_sreq_hdr->iov_num) {
            goto fn_exit;
        }

        /* Remove element from postponed queue */

        DL_DELETE(MPIDI_POSIX_global.postponed_queue, curr_sreq_hdr);

        /* Request has been completed.
         * If associated with a device-layer sreq, call origin callback and cleanup.
         * Otherwise this is a POSIX internal queued sreq_hdr, simply release. */
        if (curr_sreq_hdr->request) {
            sreq = curr_sreq_hdr->request;

            MPL_free(MPIDI_POSIX_AMREQUEST_HDR(sreq, pack_buffer));
            MPIDI_POSIX_AMREQUEST_HDR(sreq, pack_buffer) = NULL;

            MPIDIG_global.origin_cbs[curr_sreq_hdr->handler_id] (sreq);
        } else {
            MPIDI_POSIX_am_release_req_hdr(&curr_sreq_hdr);
        }
    }

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_POSIX_PROGRESS_SEND);
    return mpi_errno;
}

int MPIDI_POSIX_progress(int blocking)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_POSIX_PROGRESS);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_POSIX_PROGRESS);

    int mpi_errno = MPI_SUCCESS;

    mpi_errno = progress_recv(blocking);
    MPIR_ERR_CHECK(mpi_errno);

    mpi_errno = progress_send(blocking);
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_POSIX_PROGRESS);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIDI_POSIX_progress_test(void)
{
    MPIR_Assert(0);
    return MPI_SUCCESS;
}

int MPIDI_POSIX_progress_poke(void)
{
    MPIR_Assert(0);
    return MPI_SUCCESS;
}

void MPIDI_POSIX_progress_start(MPID_Progress_state * state)
{
    MPIR_Assert(0);
    return;
}

void MPIDI_POSIX_progress_end(MPID_Progress_state * state)
{
    MPIR_Assert(0);
    return;
}

int MPIDI_POSIX_progress_wait(MPID_Progress_state * state)
{
    MPIR_Assert(0);
    return MPI_SUCCESS;
}

int MPIDI_POSIX_progress_register(int (*progress_fn) (int *))
{
    MPIR_Assert(0);
    return MPI_SUCCESS;
}

int MPIDI_POSIX_progress_deregister(int id)
{
    MPIR_Assert(0);
    return MPI_SUCCESS;
}

int MPIDI_POSIX_progress_activate(int id)
{
    MPIR_Assert(0);
    return MPI_SUCCESS;
}

int MPIDI_POSIX_progress_deactivate(int id)
{
    MPIR_Assert(0);
    return MPI_SUCCESS;
}
