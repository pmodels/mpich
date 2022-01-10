/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef POSIX_PROGRESS_H_INCLUDED
#define POSIX_PROGRESS_H_INCLUDED

#include "posix_impl.h"
#include "posix_eager.h"
#include "posix_am.h"

MPL_STATIC_INLINE_PREFIX int MPIDI_POSIX_progress_recv(int vsi, int blocking)
{

    MPIDI_POSIX_eager_recv_transaction_t transaction;
    int mpi_errno = MPI_SUCCESS;
    int result = MPIDI_POSIX_OK;
    MPIR_Request *rreq = NULL;
    void *am_hdr = NULL;
    MPIDI_POSIX_am_header_t *msg_hdr;
    uint8_t *payload;
    size_t payload_left;

    MPIR_FUNC_ENTER;

    /* Check to see if any new messages are ready for processing from the eager submodule. */
    result = MPIDI_POSIX_eager_recv_begin(vsi, &transaction);

    if (MPIDI_POSIX_OK != result) {
        goto fn_exit;
    }

    /* Process the eager message */
    msg_hdr = transaction.msg_hdr;

    payload = transaction.payload;
    payload_left = transaction.payload_sz;

    if (!msg_hdr) {
        /* Fragment handling. Set currently active recv request */
        rreq = MPIDI_POSIX_global.per_vsi[vsi].active_rreq[transaction.src_local_rank];
        MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_GENERAL, VERBOSE,
                        (MPL_DBG_FDEST, "posix cached req handle=0x%x", rreq->handle));
    } else {
        /* First segment */
        am_hdr = payload;

        payload += msg_hdr->am_hdr_sz;
        payload_left -= msg_hdr->am_hdr_sz;

        uint32_t attr = MPIDIG_AM_ATTR__IS_LOCAL;
        MPIDIG_AM_ATTR_SET_VCIS(attr, transaction.src_vsi, transaction.dst_vsi);
        switch (msg_hdr->am_type) {
            case MPIDI_POSIX_AM_TYPE__HDR:
            case MPIDI_POSIX_AM_TYPE__SHORT:
                MPIDIG_global.target_msg_cbs[msg_hdr->handler_id] (am_hdr, payload, payload_left,
                                                                   attr, NULL);
                MPIDI_POSIX_eager_recv_commit(&transaction);
                goto fn_exit;
            case MPIDI_POSIX_AM_TYPE__PIPELINE:
                MPIDIG_global.target_msg_cbs[msg_hdr->handler_id] (am_hdr, NULL, payload_left,
                                                                   attr | MPIDIG_AM_ATTR__IS_ASYNC,
                                                                   &rreq);
                /* prepare for asynchronous transfer */
                MPIDIG_recv_setup(rreq);

                MPIR_Assert(MPIDI_POSIX_global.
                            per_vsi[vsi].active_rreq[transaction.src_local_rank] == NULL);
                MPIDI_POSIX_global.per_vsi[vsi].active_rreq[transaction.src_local_rank] = rreq;
                break;
            default:
                MPIR_Assert(0);
        }
    }

    int is_done = MPIDIG_recv_copy_seg(payload, payload_left, rreq);
    if (is_done) {
        MPIDI_POSIX_global.per_vsi[vsi].active_rreq[transaction.src_local_rank] = NULL;
        MPIDIG_REQUEST(rreq, req->target_cmpl_cb) (rreq);
    }

    MPIDI_POSIX_eager_recv_commit(&transaction);

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_POSIX_progress_send(int vsi, int blocking)
{

    int mpi_errno = MPI_SUCCESS;
    MPIDI_POSIX_am_request_header_t *curr_sreq_hdr = NULL;

    MPIR_FUNC_ENTER;

    if (MPIDI_POSIX_global.per_vsi[vsi].postponed_queue) {
        /* Drain postponed queue */
        curr_sreq_hdr = MPIDI_POSIX_global.per_vsi[vsi].postponed_queue;

        switch (curr_sreq_hdr->msg_hdr->am_type) {
            case MPIDI_POSIX_AM_TYPE__HDR:
                mpi_errno = MPIDI_POSIX_do_am_send_hdr(curr_sreq_hdr->dst_grank,
                                                       curr_sreq_hdr->msg_hdr,
                                                       curr_sreq_hdr->am_hdr, true,
                                                       curr_sreq_hdr->src_vsi,
                                                       curr_sreq_hdr->dst_vsi);

                break;
            case MPIDI_POSIX_AM_TYPE__SHORT:
            case MPIDI_POSIX_AM_TYPE__PIPELINE:
                mpi_errno = MPIDI_POSIX_do_am_isend(curr_sreq_hdr->dst_grank,
                                                    curr_sreq_hdr->msg_hdr,
                                                    curr_sreq_hdr->am_hdr,
                                                    curr_sreq_hdr->am_hdr_sz,
                                                    curr_sreq_hdr->buf,
                                                    curr_sreq_hdr->count,
                                                    curr_sreq_hdr->datatype, curr_sreq_hdr->request,
                                                    true, curr_sreq_hdr->src_vsi,
                                                    curr_sreq_hdr->dst_vsi);

                break;
            default:
                MPIR_Assert(0);
        }

    }

    MPIR_FUNC_EXIT;
    return mpi_errno;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_POSIX_progress(int vci, int blocking)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_ENTER;

    int vsi = vci % MPIDI_POSIX_global.num_vsis;

    mpi_errno = MPIDI_POSIX_progress_recv(vsi, blocking);
    MPIR_ERR_CHECK(mpi_errno);

    mpi_errno = MPIDI_POSIX_progress_send(vsi, blocking);
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
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
