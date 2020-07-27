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
    void *p_data = NULL;
    size_t in_total_data_sz = 0;
    void *am_hdr = NULL;
    MPIDI_POSIX_am_header_t *msg_hdr;
    uint8_t *payload;

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

    MPIDIG_global.target_msg_cbs[msg_hdr->handler_id] (msg_hdr->handler_id, am_hdr,
                                                       p_data, in_total_data_sz, 1, 0, NULL);
    MPIDI_POSIX_eager_recv_commit(&transaction);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_PROGRESS_RECV);
    return mpi_errno;
}

static int progress_send(int blocking)
{

    int mpi_errno = MPI_SUCCESS;
    MPIDI_POSIX_deferred_am_isend_req_t *dreq = MPIDI_POSIX_global.deferred_am_isend_q;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_PROGRESS_SEND);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_PROGRESS_SEND);

    if (dreq) {
        mpi_errno = MPIDI_POSIX_deferred_am_isend_issue(dreq);
        MPIR_ERR_CHECK(mpi_errno);
    }

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_PROGRESS_SEND);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
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
