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
#ifndef POSIX_PROGRESS_H_INCLUDED
#define POSIX_PROGRESS_H_INCLUDED

#include "posix_impl.h"
#include "posix_am_impl.h"
#include <posix_eager.h>

#undef FUNCNAME
#define FUNCNAME MPIDI_POSIX_progress
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPIDI_POSIX_progress(int blocking)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_POSIX_PROGRESS);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_POSIX_PROGRESS);

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

    result = MPIDI_POSIX_eager_recv_begin(&transaction);

    if (MPIDI_POSIX_OK != result) {
        goto send_progress;
    }

    msg_hdr = transaction.msg_hdr;

    payload = transaction.payload;
    payload_left = transaction.payload_sz;

    if (msg_hdr) {
        am_hdr = payload;
        p_data = payload + msg_hdr->am_hdr_sz;

        in_total_data_sz = msg_hdr->data_sz;
        p_data_sz = msg_hdr->data_sz;

        MPIDIG_global.target_msg_cbs[msg_hdr->handler_id] (msg_hdr->handler_id,
                                                           am_hdr,
                                                           &p_data,
                                                           &p_data_sz,
                                                           &is_contig, &target_cmpl_cb, &rreq);
        payload += msg_hdr->am_hdr_sz;
        payload_left -= msg_hdr->am_hdr_sz;

        if (rreq) {
            if ((!p_data || !p_data_sz) && target_cmpl_cb) {
                target_cmpl_cb(rreq);

                MPIDI_POSIX_eager_recv_commit(&transaction);

                goto fn_exit;
            }

            if (is_contig && (in_total_data_sz == payload_left)) {
                /* Received immediately */

                if (in_total_data_sz > p_data_sz) {
                    rreq->status.MPI_ERROR = MPI_ERR_TRUNCATE;
                } else {
                    rreq->status.MPI_ERROR = MPI_SUCCESS;
                }

                recv_data_sz = MPL_MIN(p_data_sz, in_total_data_sz);

                MPIR_STATUS_SET_COUNT(rreq->status, recv_data_sz);

                MPIDI_POSIX_eager_recv_memcpy(&transaction, p_data, payload, recv_data_sz);
                target_cmpl_cb(rreq);

                MPIDI_POSIX_eager_recv_commit(&transaction);

                MPIDI_POSIX_EAGER_RECV_COMPLETED_HOOK(rreq);

                goto fn_exit;
            }

            /* Allocate aux data */

            MPIDI_POSIX_am_init_request(NULL, 0, rreq);

            /* Set active recv request */

            curr_rreq_hdr = MPIDI_POSIX_AMREQUEST(rreq, req_hdr);

            curr_rreq_hdr->cmpl_handler_fn = target_cmpl_cb;
            curr_rreq_hdr->dst_grank = transaction.src_grank;

            if (is_contig) {
                curr_rreq_hdr->iov[0].iov_base = p_data;
                curr_rreq_hdr->iov[0].iov_len = p_data_sz;

                curr_rreq_hdr->iov_num = 1;
                curr_rreq_hdr->iov_num_total = 1;

                recv_data_sz = p_data_sz;
            } else {
                for (i = 0; i < p_data_sz; i++) {
                    curr_rreq_hdr->iov[i] = ((struct iovec *) p_data)[i];

                    recv_data_sz += curr_rreq_hdr->iov[i].iov_len;
                }

                curr_rreq_hdr->iov_num = p_data_sz;
                curr_rreq_hdr->iov_num_total = p_data_sz;
            }

            curr_rreq_hdr->iov_ptr = curr_rreq_hdr->iov;

            /* Set final request status */

            if (in_total_data_sz > recv_data_sz) {
                rreq->status.MPI_ERROR = MPI_ERR_TRUNCATE;
            } else {
                rreq->status.MPI_ERROR = MPI_SUCCESS;
            }

            recv_data_sz = MPL_MIN(recv_data_sz, in_total_data_sz);

            MPIR_STATUS_SET_COUNT(rreq->status, recv_data_sz);

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

    /* Generic handling for contig and non contig data */

    for (i = 0; i < curr_rreq_hdr->iov_num; i++) {
        if (payload_left < curr_rreq_hdr->iov_ptr[i].iov_len) {
            MPIDI_POSIX_eager_recv_memcpy(&transaction,
                                          curr_rreq_hdr->iov[i].iov_base, payload, payload_left);

            curr_rreq_hdr->iov_ptr[i].iov_base =
                (char *) curr_rreq_hdr->iov_ptr[i].iov_base + payload_left;
            curr_rreq_hdr->iov_ptr[i].iov_len -= payload_left;

            curr_rreq_hdr->in_total_data_sz -= payload_left;

            payload_left = 0;

            break;
        }

        MPIDI_POSIX_eager_recv_memcpy(&transaction,
                                      curr_rreq_hdr->iov_ptr[i].iov_base,
                                      payload, curr_rreq_hdr->iov_ptr[i].iov_len);

        payload += curr_rreq_hdr->iov_ptr[i].iov_len;
        payload_left -= curr_rreq_hdr->iov_ptr[i].iov_len;

        curr_rreq_hdr->in_total_data_sz -= curr_rreq_hdr->iov_ptr[i].iov_len;

        iov_done++;
    }

    curr_rreq_hdr->iov_num -= iov_done;

    if (curr_rreq_hdr->in_total_data_sz) {
        curr_rreq_hdr->iov_ptr =
            &(curr_rreq_hdr->iov[curr_rreq_hdr->iov_num_total - curr_rreq_hdr->iov_num]);
    } else {
        /* All fragments have been received */

        MPIDI_POSIX_global.active_rreq[transaction.src_grank] = NULL;

        if (curr_rreq_hdr->cmpl_handler_fn) {
            curr_rreq_hdr->cmpl_handler_fn(rreq);
        }

        MPIDI_POSIX_am_clear_request(rreq);
    }

    MPIDI_POSIX_eager_recv_commit(&transaction);

    goto fn_exit;

  send_progress:

    /* Send progress */

    if (MPIDI_POSIX_global.postponed_queue) {
        /* Drain postponed queue */

        /* Get data from request */

        MPIR_Request *sreq = (MPIR_Request *) (MPIDI_POSIX_global.postponed_queue->request);

        MPIDI_POSIX_am_request_header_t *curr_sreq_hdr = MPIDI_POSIX_AMREQUEST(sreq, req_hdr);

        POSIX_TRACE("Queue OUT HDR [ POSIX AM [handler_id %" PRIu64 ", am_hdr_sz %" PRIu64
                    ", data_sz %" PRIu64 ", seq_num = %d], " "tag = %d, src_rank = %d ] to %d\n",
                    curr_sreq_hdr->msg_hdr ? curr_sreq_hdr->msg_hdr->handler_id : (uint64_t) - 1,
                    curr_sreq_hdr->msg_hdr ? curr_sreq_hdr->msg_hdr->am_hdr_sz : (uint64_t) - 1,
                    curr_sreq_hdr->msg_hdr ? curr_sreq_hdr->msg_hdr->data_sz : (uint64_t) - 1,
#ifdef POSIX_AM_DEBUG
                    curr_sreq_hdr->msg_hdr ? curr_sreq_hdr->msg_hdr->seq_num : -1,
#else /* POSIX_AM_DEBUG */
                    -1,
#endif /* POSIX_AM_DEBUG */
                    ((MPIDI_CH4U_hdr_t *) curr_sreq_hdr->am_hdr)->tag,
                    ((MPIDI_CH4U_hdr_t *) curr_sreq_hdr->am_hdr)->src_rank,
                    curr_sreq_hdr->dst_grank);

        result = MPIDI_POSIX_eager_send(curr_sreq_hdr->dst_grank,
                                        &curr_sreq_hdr->msg_hdr,
                                        &curr_sreq_hdr->iov_ptr, &curr_sreq_hdr->iov_num, 0);

        if ((MPIDI_POSIX_NOK == result) || curr_sreq_hdr->iov_num) {
            goto fn_exit;
        }

        /* Remove element from postponed queue */

        DL_DELETE(MPIDI_POSIX_global.postponed_queue, &sreq->dev.ch4.am.req->rreq);

        /* Request has been completed */

        if (MPIDI_POSIX_AMREQUEST_HDR(sreq, pack_buffer)) {
            MPL_free(MPIDI_POSIX_AMREQUEST_HDR(sreq, pack_buffer));
            MPIDI_POSIX_AMREQUEST_HDR(sreq, pack_buffer) = NULL;
        }

        MPIDIG_global.origin_cbs[curr_sreq_hdr->handler_id] (sreq);

        MPIDI_POSIX_am_clear_request(sreq);

        goto fn_exit;
    }

  fn_exit:

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_POSIX_PROGRESS);
    return mpi_errno;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_POSIX_progress_test(void)
{
    MPIR_Assert(0);
    return MPI_SUCCESS;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_POSIX_progress_poke(void)
{
    MPIR_Assert(0);
    return MPI_SUCCESS;
}

MPL_STATIC_INLINE_PREFIX void MPIDI_POSIX_progress_start(MPID_Progress_state * state)
{
    MPIR_Assert(0);
    return;
}

MPL_STATIC_INLINE_PREFIX void MPIDI_POSIX_progress_end(MPID_Progress_state * state)
{
    MPIR_Assert(0);
    return;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_POSIX_progress_wait(MPID_Progress_state * state)
{
    MPIR_Assert(0);
    return MPI_SUCCESS;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_POSIX_progress_register(int (*progress_fn) (int *))
{
    MPIR_Assert(0);
    return MPI_SUCCESS;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_POSIX_progress_deregister(int id)
{
    MPIR_Assert(0);
    return MPI_SUCCESS;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_POSIX_progress_activate(int id)
{
    MPIR_Assert(0);
    return MPI_SUCCESS;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_POSIX_progress_deactivate(int id)
{
    MPIR_Assert(0);
    return MPI_SUCCESS;
}

#endif /* POSIX_PROGRESS_H_INCLUDED */
