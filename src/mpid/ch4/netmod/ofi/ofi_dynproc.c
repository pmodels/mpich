/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpidimpl.h"
#include "ofi_impl.h"

/* dynamic process messages are send with MPIDI_OFI_DYNPROC_SEND bit masked, so it
 * does not collide with normal messages with a regular communicator. The following
 * DISCONNECT_CONTEXT_ID is arbitrary chosen so the disconnect messages won't collide
 * with other dynamic process messages (just the initial handshake messages).
 */
#define DISCONNECT_CONTEXT_ID 0xF000

/* The connection table will dynamically grow. The INITIAL_NUM_CONN sets the first
 * capacity. A larger value may reduce the initial overhead */
#define INITIAL_NUM_CONN 10

static int dynproc_send_disconnect(int conn_id);
static int dynproc_accept_disconnect(int conn_id);
static int dynproc_grow_conn_table(int new_max);
static int dynproc_get_next_conn_id(int *conn_id_out);

int MPIDI_OFI_dynproc_init(void)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_CONN_MANAGER_INIT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_CONN_MANAGER_INIT);

    MPIDI_OFI_global.conn_mgr.n_conn = 0;
    MPIDI_OFI_global.conn_mgr.max_n_conn = 0;
    MPIDI_OFI_global.conn_mgr.conn_table = NULL;
    MPIDI_OFI_global.conn_mgr.n_free = 0;
    MPIDI_OFI_global.conn_mgr.free_stack = NULL;

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_CONN_MANAGER_INIT);
    return mpi_errno;
}

int MPIDI_OFI_dynproc_finalize(void)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_CONN_MANAGER_DESTROY);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_CONN_MANAGER_DESTROY);

    if (MPIDI_OFI_global.conn_mgr.n_conn > 0) {
        for (int i = 0; i < MPIDI_OFI_global.conn_mgr.max_n_conn; i++) {
            switch (MPIDI_OFI_global.conn_mgr.conn_table[i].state) {
                case MPIDI_OFI_DYNPROC_CONNECTED_CHILD:
                    mpi_errno = dynproc_send_disconnect(i);
                    MPIR_ERR_CHECK(mpi_errno);
                    break;
                case MPIDI_OFI_DYNPROC_CONNECTED_PARENT:
                    mpi_errno = dynproc_accept_disconnect(i);
                    MPIR_ERR_CHECK(mpi_errno);
                    break;
                default:
                    break;
            }
        }
    }

    MPL_free(MPIDI_OFI_global.conn_mgr.conn_table);
    MPL_free(MPIDI_OFI_global.conn_mgr.free_stack);
  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_CONN_MANAGER_DESTROY);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIDI_OFI_dynproc_insert_conn(fi_addr_t conn, int rank, int state, int *conn_id_out)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_CONN_MANAGER_INSERT_CONN);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_CONN_MANAGER_INSERT_CONN);
    MPID_THREAD_CS_ENTER(VCI, MPIDIU_THREAD_DYNPROC_MUTEX);

    int conn_id;
    mpi_errno = dynproc_get_next_conn_id(&conn_id);
    MPIR_ERR_CHECK(mpi_errno);

    MPIDI_OFI_global.conn_mgr.conn_table[conn_id].dest = conn;
    MPIDI_OFI_global.conn_mgr.conn_table[conn_id].rank = rank;
    MPIDI_OFI_global.conn_mgr.conn_table[conn_id].state = state;

    MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_GENERAL, VERBOSE,
                    (MPL_DBG_FDEST, " new_conn_id=%d for conn=%" PRIu64 " rank=%d state=%d",
                     conn_id, conn, rank, MPIDI_OFI_global.conn_mgr.conn_table[conn_id].state));

    *conn_id_out = conn_id;

  fn_exit:
    MPID_THREAD_CS_EXIT(VCI, MPIDIU_THREAD_DYNPROC_MUTEX);
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_CONN_MANAGER_INSERT_CONN);

    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

/* static routines */

static int dynproc_send_disconnect(int conn_id)
{
    int mpi_errno = MPI_SUCCESS;

    MPIDI_OFI_dynamic_process_request_t req;
    uint64_t match_bits = 0;
    unsigned int close_msg = 0xcccccccc;
    struct fi_msg_tagged msg;
    struct iovec msg_iov;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_DYNPROC_SEND_DISCONNECT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_DYNPROC_SEND_DISCONNECT);

    MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_GENERAL, VERBOSE,
                    (MPL_DBG_FDEST, " send disconnect msg conn_id=%d from child side", conn_id));
    match_bits = MPIDI_OFI_init_sendtag(DISCONNECT_CONTEXT_ID, 1, MPIDI_OFI_DYNPROC_SEND);

    /* fi_av_map here is not quite right for some providers */
    /* we need to get this connection from the sockname     */
    req.done = 0;
    req.event_id = MPIDI_OFI_EVENT_DYNPROC_DONE;
    msg_iov.iov_base = &close_msg;
    msg_iov.iov_len = sizeof(close_msg);
    msg.msg_iov = &msg_iov;
    msg.desc = NULL;
    msg.iov_count = 0;
    msg.addr = MPIDI_OFI_global.conn_mgr.conn_table[conn_id].dest;
    msg.tag = match_bits;
    msg.ignore = DISCONNECT_CONTEXT_ID;
    msg.context = (void *) &req.context;
    msg.data = 0;
    int nic = 0;
    int ctx_idx = MPIDI_OFI_get_ctx_index(0, nic);
    MPIDI_OFI_CALL_RETRY(fi_tsendmsg(MPIDI_OFI_global.ctx[ctx_idx].tx, &msg,
                                     FI_COMPLETION | FI_TRANSMIT_COMPLETE | FI_REMOTE_CQ_DATA),
                         0, tsendmsg, FALSE);
    MPIDI_OFI_PROGRESS_WHILE(!req.done, 0);

    MPIDI_OFI_global.conn_mgr.conn_table[conn_id].state = MPIDI_OFI_DYNPROC_DISCONNECTED;

    MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_GENERAL, VERBOSE,
                    (MPL_DBG_FDEST, " local_disconnected conn_id=%d state=%d",
                     conn_id, MPIDI_OFI_global.conn_mgr.conn_table[conn_id].state));

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_DYNPROC_SEND_DISCONNECT);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static int dynproc_accept_disconnect(int conn_id)
{
    int mpi_errno = MPI_SUCCESS;

    MPIDI_OFI_dynamic_process_request_t req;
    fi_addr_t addr = MPIDI_OFI_global.conn_mgr.conn_table[conn_id].dest;
    int close_msg;

    req.done = 0;
    req.event_id = MPIDI_OFI_EVENT_DYNPROC_DONE;

    uint64_t mask_bits = 0;
    uint64_t match_bits = 0;
    match_bits = MPIDI_OFI_init_recvtag(&mask_bits, DISCONNECT_CONTEXT_ID, 1);
    match_bits |= MPIDI_OFI_DYNPROC_SEND;

    int nic = 0;
    int ctx_idx = MPIDI_OFI_get_ctx_index(0, nic);
    MPIDI_OFI_CALL_RETRY(fi_trecv(MPIDI_OFI_global.ctx[ctx_idx].rx,
                                  &close_msg, sizeof(int),
                                  NULL, addr, match_bits, mask_bits, &req.context),
                         0, trecv, FALSE);
    MPIDI_OFI_PROGRESS_WHILE(!req.done, 0);
    MPIDI_OFI_global.conn_mgr.conn_table[conn_id].state = MPIDI_OFI_DYNPROC_DISCONNECTED;
    MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_GENERAL, VERBOSE, (MPL_DBG_FDEST, "conn_id=%d closed", conn_id));

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static int dynproc_grow_conn_table(int new_max)
{
    int mpi_errno = MPI_SUCCESS;

    if (MPIDI_OFI_global.conn_mgr.max_n_conn >= new_max) {
        goto fn_exit;
    }

    int old_max = MPIDI_OFI_global.conn_mgr.max_n_conn;
    MPIDI_OFI_global.conn_mgr.conn_table = MPL_realloc(MPIDI_OFI_global.conn_mgr.conn_table,
                                                       new_max * sizeof(MPIDI_OFI_conn_t),
                                                       MPL_MEM_OTHER);
    MPIR_ERR_CHKANDSTMT(MPIDI_OFI_global.conn_mgr.conn_table == NULL, mpi_errno, MPI_ERR_NO_MEM,
                        goto fn_fail, "**nomem");
    MPIDI_OFI_global.conn_mgr.free_stack = MPL_realloc(MPIDI_OFI_global.conn_mgr.free_stack,
                                                       new_max * sizeof(int), MPL_MEM_OTHER);
    MPIR_ERR_CHKANDSTMT(MPIDI_OFI_global.conn_mgr.free_stack == NULL, mpi_errno, MPI_ERR_NO_MEM,
                        goto fn_fail, "**nomem");

    MPIDI_OFI_global.conn_mgr.max_n_conn = new_max;
    for (int i = old_max; i < new_max; i++) {
        MPIDI_OFI_global.conn_mgr.conn_table[i].state = MPIDI_OFI_DYNPROC_DISCONNECTED;
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static int dynproc_get_next_conn_id(int *conn_id_out)
{
    int mpi_errno = MPI_SUCCESS;
    int conn_id;

    if (MPIDI_OFI_global.conn_mgr.n_free > 0) {
        MPIDI_OFI_global.conn_mgr.n_free--;
        conn_id = MPIDI_OFI_global.conn_mgr.free_stack[MPIDI_OFI_global.conn_mgr.n_free];
    } else {
        conn_id = MPIDI_OFI_global.conn_mgr.n_conn;
        MPIDI_OFI_global.conn_mgr.n_conn++;

        if (MPIDI_OFI_global.conn_mgr.n_conn > MPIDI_OFI_global.conn_mgr.max_n_conn) {
            int new_max;
            if (MPIDI_OFI_global.conn_mgr.max_n_conn == 0) {
                new_max = INITIAL_NUM_CONN;
            } else {
                new_max = MPIDI_OFI_global.conn_mgr.max_n_conn * 2;
            }
            mpi_errno = dynproc_grow_conn_table(new_max);
            MPIR_ERR_CHECK(mpi_errno);
        }
    }
    *conn_id_out = conn_id;

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
