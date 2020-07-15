/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpidimpl.h"
#include "ofi_impl.h"

static int dynproc_send_disconnect(MPIDI_OFI_conn_t * conn);
static int dynproc_recv_disconnect(MPIDI_OFI_conn_t * conn);

int MPIDI_OFI_dynproc_init(void)
{
    int mpi_errno = MPI_SUCCESS, i;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_CONN_MANAGER_INIT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_CONN_MANAGER_INIT);

    MPIDI_OFI_global.conn_mgr.mmapped_size = 8 * 4 * 1024;
    MPIDI_OFI_global.conn_mgr.max_n_conn = 1;
    MPIDI_OFI_global.conn_mgr.next_conn_id = 0;
    MPIDI_OFI_global.conn_mgr.n_conn = 0;

    MPIDI_OFI_global.conn_mgr.conn_list =
        (MPIDI_OFI_conn_t *) MPL_mmap(NULL, MPIDI_OFI_global.conn_mgr.mmapped_size,
                                      PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON, -1, 0,
                                      MPL_MEM_ADDRESS);
    MPIR_ERR_CHKANDSTMT(MPIDI_OFI_global.conn_mgr.conn_list == MAP_FAILED, mpi_errno,
                        MPI_ERR_NO_MEM, goto fn_fail, "**nomem");

    MPIDI_OFI_global.conn_mgr.free_conn_id =
        (int *) MPL_malloc(MPIDI_OFI_global.conn_mgr.max_n_conn * sizeof(int), MPL_MEM_ADDRESS);
    MPIR_ERR_CHKANDSTMT(MPIDI_OFI_global.conn_mgr.free_conn_id == NULL, mpi_errno,
                        MPI_ERR_NO_MEM, goto fn_fail, "**nomem");

    for (i = 0; i < MPIDI_OFI_global.conn_mgr.max_n_conn; ++i) {
        MPIDI_OFI_global.conn_mgr.free_conn_id[i] = i + 1;
    }
    MPIDI_OFI_global.conn_mgr.free_conn_id[MPIDI_OFI_global.conn_mgr.max_n_conn - 1] = -1;

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_CONN_MANAGER_INIT);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIDI_OFI_dynproc_finalize(void)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_OFI_DYNPROC_FINALIZE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_OFI_DYNPROC_FINALIZE);

    int max_n_conn = MPIDI_OFI_global.conn_mgr.max_n_conn;
    if (max_n_conn > 0) {
        for (int i = 0; i < max_n_conn; ++i) {
            MPIDI_OFI_conn_t *conn = &MPIDI_OFI_global.conn_mgr.conn_list[i];
            if (conn->state == MPIDI_OFI_DYNPROC_CONNECTED_CHILD) {
                mpi_errno = dynproc_send_disconnect(conn);
                MPIR_ERR_CHECK(mpi_errno);
            }
        }

        for (int i = 0; i < max_n_conn; ++i) {
            MPIDI_OFI_conn_t *conn = &MPIDI_OFI_global.conn_mgr.conn_list[i];
            if (conn->state == MPIDI_OFI_DYNPROC_CONNECTED_PARENT) {
                MPIDI_OFI_PROGRESS_WHILE(!conn->close_req.done, 0);
                conn->state = MPIDI_OFI_DYNPROC_DISCONNECTED;
                MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_GENERAL, VERBOSE,
                                (MPL_DBG_FDEST, "conn_id=%d closed", i));
            }
        }
    }

    MPL_munmap((void *) MPIDI_OFI_global.conn_mgr.conn_list, MPIDI_OFI_global.conn_mgr.mmapped_size,
               MPL_MEM_ADDRESS);
    MPL_free(MPIDI_OFI_global.conn_mgr.free_conn_id);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_OFI_DYNPROC_FINALIZE);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static int dynproc_send_disconnect(MPIDI_OFI_conn_t * conn)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_Context_id_t context_id = 0xF000;
    MPIDI_OFI_dynamic_process_request_t req;
    uint64_t match_bits = 0;
    unsigned int close_msg = 0xcccccccc;
    struct fi_msg_tagged msg;
    struct iovec msg_iov;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_DYNPROC_SEND_DISCONNECT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_DYNPROC_SEND_DISCONNECT);

    if (conn->state == MPIDI_OFI_DYNPROC_CONNECTED_CHILD) {
        MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_GENERAL, VERBOSE,
                        (MPL_DBG_FDEST, " [%p] send disconnect msg from child side", conn));
        match_bits = MPIDI_OFI_init_sendtag(context_id, 1, MPIDI_OFI_DYNPROC_SEND);

        /* fi_av_map here is not quite right for some providers */
        /* we need to get this connection from the sockname     */
        req.done = 0;
        req.event_id = MPIDI_OFI_EVENT_DYNPROC_DONE;
        msg_iov.iov_base = &close_msg;
        msg_iov.iov_len = sizeof(close_msg);
        msg.msg_iov = &msg_iov;
        msg.desc = NULL;
        msg.iov_count = 0;
        msg.addr = conn->dest;
        msg.tag = match_bits;
        msg.ignore = context_id;
        msg.context = (void *) &req.context;
        msg.data = 0;
        MPIDI_OFI_VCI_CALL_RETRY(fi_tsendmsg(MPIDI_OFI_global.ctx[0].tx, &msg,
                                             FI_COMPLETION | FI_TRANSMIT_COMPLETE |
                                             FI_REMOTE_CQ_DATA), 0, tsendmsg, FALSE);
        MPIDI_OFI_VCI_PROGRESS_WHILE(0, !req.done);

        conn->state = MPIDI_OFI_DYNPROC_DISCONNECTED;
    }

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_DYNPROC_SEND_DISCONNECT);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static int dynproc_recv_disconnect(MPIDI_OFI_conn_t * conn)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_DYNPROC_RECV_DISCONNECT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_DYNPROC_RECV_DISCONNECT);

    if (conn->state == MPIDI_OFI_DYNPROC_CONNECTED_PARENT) {
        MPIR_Context_id_t context_id = 0xF000;
        uint64_t match_bits = 0;
        uint64_t mask_bits = 0;

        MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_GENERAL, VERBOSE,
                        (MPL_DBG_FDEST, " [%p] recv disconnect msg from parent side", conn));

        match_bits = MPIDI_OFI_init_recvtag(&mask_bits, context_id, 1);
        match_bits |= MPIDI_OFI_DYNPROC_SEND;

        conn->close_req.done = 0;
        conn->close_req.event_id = MPIDI_OFI_EVENT_DYNPROC_DONE;

        MPIDI_OFI_VCI_CALL_RETRY(fi_trecv(MPIDI_OFI_global.ctx[0].rx, &conn->close_msg, sizeof(int),
                                          NULL, conn->dest, match_bits, mask_bits,
                                          &conn->close_req.context), 0, trecv, FALSE);
    }

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_DYNPROC_RECV_DISCONNECT);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIDI_OFI_dynproc_insert_conn(fi_addr_t conn, int rank, int state)
{
    int conn_id = -1;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_CONN_MANAGER_INSERT_CONN);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_CONN_MANAGER_INSERT_CONN);
    MPID_THREAD_CS_ENTER(VCI, MPIDIU_THREAD_DYNPROC_MUTEX);

    /* We've run out of space in the connection table. Allocate more. */
    if (MPIDI_OFI_global.conn_mgr.next_conn_id == -1) {
        int old_max, new_max, i;
        old_max = MPIDI_OFI_global.conn_mgr.max_n_conn;
        new_max = old_max + 1;
        MPIDI_OFI_global.conn_mgr.free_conn_id =
            (int *) MPL_realloc(MPIDI_OFI_global.conn_mgr.free_conn_id, new_max * sizeof(int),
                                MPL_MEM_ADDRESS);
        for (i = old_max; i < new_max - 1; ++i) {
            MPIDI_OFI_global.conn_mgr.free_conn_id[i] = i + 1;
        }
        MPIDI_OFI_global.conn_mgr.free_conn_id[new_max - 1] = -1;
        MPIDI_OFI_global.conn_mgr.max_n_conn = new_max;
        MPIDI_OFI_global.conn_mgr.next_conn_id = old_max;
    }

    conn_id = MPIDI_OFI_global.conn_mgr.next_conn_id;
    MPIDI_OFI_global.conn_mgr.next_conn_id = MPIDI_OFI_global.conn_mgr.free_conn_id[conn_id];
    MPIDI_OFI_global.conn_mgr.free_conn_id[conn_id] = -1;
    MPIDI_OFI_global.conn_mgr.n_conn++;

    MPIR_Assert(MPIDI_OFI_global.conn_mgr.n_conn <= MPIDI_OFI_global.conn_mgr.max_n_conn);

    MPIDI_OFI_global.conn_mgr.conn_list[conn_id].dest = conn;
    MPIDI_OFI_global.conn_mgr.conn_list[conn_id].rank = rank;
    MPIDI_OFI_global.conn_mgr.conn_list[conn_id].state = state;
    if (state == MPIDI_OFI_DYNPROC_CONNECTED_PARENT) {
        int mpi_errno = dynproc_recv_disconnect(&MPIDI_OFI_global.conn_mgr.conn_list[conn_id]);
        MPIR_Assert(mpi_errno == MPI_SUCCESS);
    }

    MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_GENERAL, VERBOSE,
                    (MPL_DBG_FDEST, " new_conn_id=%d for conn=%" PRIu64 " rank=%d state=%d",
                     conn_id, conn, rank, MPIDI_OFI_global.conn_mgr.conn_list[conn_id].state));

    MPID_THREAD_CS_EXIT(VCI, MPIDIU_THREAD_DYNPROC_MUTEX);
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_CONN_MANAGER_INSERT_CONN);
    return conn_id;
}
