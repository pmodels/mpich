/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
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
    int result = MPIDI_POSIX_OK;
    MPIR_Request *rreq = NULL;
    void *p_data = NULL;
    size_t in_total_data_sz = 0;
    void *am_hdr = NULL;
    MPIDI_POSIX_am_header_t *msg_hdr;
    uint8_t *payload;
    size_t payload_left;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_PROGRESS_RECV);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_PROGRESS_RECV);

    /* Check to see if any new messages are ready for processing from the eager submodule. */
    result = MPIDI_POSIX_eager_recv_begin(&transaction);

    if (MPIDI_POSIX_OK != result) {
        goto fn_exit;
    }

    /* Process the eager message */
    msg_hdr = transaction.msg_hdr;

    payload = transaction.payload;
    payload_left = transaction.payload_sz;

    if (!msg_hdr) {
        /* Fragment handling. Set currently active recv request */
        rreq = MPIDI_POSIX_global.active_rreq[transaction.src_local_rank];
    } else {
        /* First segment */
        am_hdr = payload;
        p_data = payload + msg_hdr->am_hdr_sz;

        in_total_data_sz = msg_hdr->data_sz;

        /* This is a SHM internal control header */
        /* TODO: internal control can use the generic am interface,
         *       just need register callbacks */
        if (msg_hdr->kind == MPIDI_POSIX_AM_HDR_SHM) {
            mpi_errno = MPIDI_SHMI_ctrl_dispatch(msg_hdr->handler_id, am_hdr);

            /* TODO: discard payload for now as we only handle header in
             * current internal control protocols. */
            MPIDI_POSIX_eager_recv_commit(&transaction);
            goto fn_exit;
        }

        payload += msg_hdr->am_hdr_sz;
        payload_left -= msg_hdr->am_hdr_sz;

        /* note: setting is_local, is_async to 1, 1 */
        MPIDIG_global.target_msg_cbs[msg_hdr->handler_id] (msg_hdr->handler_id, am_hdr,
                                                           NULL, in_total_data_sz, 1, 1, &rreq);

        if (!rreq) {
            MPIDI_POSIX_eager_recv_commit(&transaction);
            goto fn_exit;
        } else if (in_total_data_sz == payload_left) {
            MPIDIG_recv_copy(p_data, rreq);
            MPIDIG_REQUEST(rreq, req->target_cmpl_cb) (rreq);
            MPIDI_POSIX_eager_recv_commit(&transaction);
            MPIDI_POSIX_EAGER_RECV_COMPLETED_HOOK(rreq);
            goto fn_exit;
        } else {
            /* prepare for asynchronous transfer */
            MPIDIG_recv_setup(rreq);

            MPIR_Assert(MPIDI_POSIX_global.active_rreq[transaction.src_local_rank] == NULL);
            MPIDI_POSIX_global.active_rreq[transaction.src_local_rank] = rreq;
        }
    }

    int is_done = MPIDIG_recv_copy_seg(payload, payload_left, rreq);
    if (is_done) {
        MPIDI_POSIX_global.active_rreq[transaction.src_local_rank] = NULL;
        MPIDIG_REQUEST(rreq, req->target_cmpl_cb) (rreq);
    }

    MPIDI_POSIX_eager_recv_commit(&transaction);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_PROGRESS_RECV);
    return mpi_errno;
}

static int progress_send(int blocking)
{

    int mpi_errno = MPI_SUCCESS;
    int result = MPIDI_POSIX_OK;
    MPIR_Request *sreq = NULL;
    MPIDI_POSIX_am_request_header_t *curr_sreq_hdr = NULL;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_PROGRESS_SEND);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_PROGRESS_SEND);

    if (MPIDI_POSIX_global.postponed_queue) {
        /* Drain postponed queue */
        curr_sreq_hdr = MPIDI_POSIX_global.postponed_queue;

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
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_PROGRESS_SEND);
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
