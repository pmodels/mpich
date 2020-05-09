/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpidimpl.h"
#include "ofi_impl.h"
#include "ofi_noinline.h"

static int dynproc_send_disconnect(int conn_id);

static MPID_Thread_mutex_t conn_manager_lock;

int MPIDI_OFI_conn_manager_init()
{
    int mpi_errno = MPI_SUCCESS, i;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_CONN_MANAGER_INIT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_CONN_MANAGER_INIT);

    int err;
    MPID_Thread_mutex_create(&conn_manager_lock, &err);
    MPIR_Assert(err == 0);

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

int MPIDI_OFI_conn_manager_destroy()
{
    int mpi_errno = MPI_SUCCESS, i, j;
    MPIDI_OFI_dynamic_process_request_t *req;
    fi_addr_t *conn;
    int max_n_conn = MPIDI_OFI_global.conn_mgr.max_n_conn;
    int *close_msg;
    uint64_t match_bits = 0;
    uint64_t mask_bits = 0;
    MPIR_Context_id_t context_id = 0xF000;
    MPIR_CHKLMEM_DECL(3);

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_CONN_MANAGER_DESTROY);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_CONN_MANAGER_DESTROY);

    int err;
    MPID_Thread_mutex_destroy(&conn_manager_lock, &err);
    MPIR_Assert(err == 0);

    match_bits = MPIDI_OFI_init_recvtag(&mask_bits, context_id, 1);
    match_bits |= MPIDI_OFI_DYNPROC_SEND;

    if (max_n_conn > 0) {
        /* try wait/close connections */
        MPIR_CHKLMEM_MALLOC(req, MPIDI_OFI_dynamic_process_request_t *,
                            max_n_conn * sizeof(MPIDI_OFI_dynamic_process_request_t), mpi_errno,
                            "req", MPL_MEM_BUFFER);
        MPIR_CHKLMEM_MALLOC(conn, fi_addr_t *, max_n_conn * sizeof(fi_addr_t), mpi_errno, "conn",
                            MPL_MEM_BUFFER);
        MPIR_CHKLMEM_MALLOC(close_msg, int *, max_n_conn * sizeof(int), mpi_errno, "int",
                            MPL_MEM_BUFFER);

        j = 0;
        for (i = 0; i < max_n_conn; ++i) {
            switch (MPIDI_OFI_global.conn_mgr.conn_list[i].state) {
                case MPIDI_OFI_DYNPROC_CONNECTED_CHILD:
                    mpi_errno = dynproc_send_disconnect(i);
                    MPIR_ERR_CHECK(mpi_errno);
                    break;
                case MPIDI_OFI_DYNPROC_LOCAL_DISCONNECTED_PARENT:
                case MPIDI_OFI_DYNPROC_CONNECTED_PARENT:
                    MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_GENERAL, VERBOSE,
                                    (MPL_DBG_FDEST, "Wait for close of conn_id=%d", i));
                    conn[j] = MPIDI_OFI_global.conn_mgr.conn_list[i].dest;
                    req[j].done = 0;
                    req[j].event_id = MPIDI_OFI_EVENT_DYNPROC_DONE;
                    MPIDI_OFI_CALL_RETRY(fi_trecv(MPIDI_OFI_global.ctx[0].rx,
                                                  &close_msg[j],
                                                  sizeof(int),
                                                  NULL,
                                                  conn[j],
                                                  match_bits,
                                                  mask_bits, &req[j].context), trecv, FALSE);
                    j++;
                    break;
                default:
                    break;
            }
        }

        for (i = 0; i < j; ++i) {
            MPIDI_OFI_PROGRESS_WHILE(!req[i].done);
            MPIDI_OFI_global.conn_mgr.conn_list[i].state = MPIDI_OFI_DYNPROC_DISCONNECTED;
            MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_GENERAL, VERBOSE,
                            (MPL_DBG_FDEST, "conn_id=%d closed", i));
        }

        MPIR_CHKLMEM_FREEALL();
    }

    MPL_munmap((void *) MPIDI_OFI_global.conn_mgr.conn_list, MPIDI_OFI_global.conn_mgr.mmapped_size,
               MPL_MEM_ADDRESS);
    MPL_free(MPIDI_OFI_global.conn_mgr.free_conn_id);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_CONN_MANAGER_DESTROY);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIDI_OFI_conn_manager_insert_conn(fi_addr_t conn, int rank, int state)
{
    int conn_id = -1;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_CONN_MANAGER_INSERT_CONN);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_CONN_MANAGER_INSERT_CONN);
    MPID_THREAD_CS_ENTER(VCI, conn_manager_lock);

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

    MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_GENERAL, VERBOSE,
                    (MPL_DBG_FDEST, " new_conn_id=%d for conn=%" PRIu64 " rank=%d state=%d",
                     conn_id, conn, rank, MPIDI_OFI_global.conn_mgr.conn_list[conn_id].state));

    MPID_THREAD_CS_EXIT(VCI, conn_manager_lock);
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_CONN_MANAGER_INSERT_CONN);
    return conn_id;
}

static int dynproc_send_disconnect(int conn_id)
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

    if (MPIDI_OFI_global.conn_mgr.conn_list[conn_id].state == MPIDI_OFI_DYNPROC_CONNECTED_CHILD) {
        MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_GENERAL, VERBOSE,
                        (MPL_DBG_FDEST, " send disconnect msg conn_id=%d from child side",
                         conn_id));
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
        msg.addr = MPIDI_OFI_global.conn_mgr.conn_list[conn_id].dest;
        msg.tag = match_bits;
        msg.ignore = context_id;
        msg.context = (void *) &req.context;
        msg.data = 0;
        MPIDI_OFI_CALL_RETRY(fi_tsendmsg(MPIDI_OFI_global.ctx[0].tx, &msg,
                                         FI_COMPLETION | FI_TRANSMIT_COMPLETE | FI_REMOTE_CQ_DATA),
                             tsendmsg, FALSE);
        MPIDI_OFI_PROGRESS_WHILE(!req.done);
    }

    switch (MPIDI_OFI_global.conn_mgr.conn_list[conn_id].state) {
        case MPIDI_OFI_DYNPROC_CONNECTED_CHILD:
            MPIDI_OFI_global.conn_mgr.conn_list[conn_id].state =
                MPIDI_OFI_DYNPROC_LOCAL_DISCONNECTED_CHILD;
            break;
        case MPIDI_OFI_DYNPROC_CONNECTED_PARENT:
            MPIDI_OFI_global.conn_mgr.conn_list[conn_id].state =
                MPIDI_OFI_DYNPROC_LOCAL_DISCONNECTED_PARENT;
            break;
        default:
            break;
    }

    MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_GENERAL, VERBOSE,
                    (MPL_DBG_FDEST, " local_disconnected conn_id=%d state=%d",
                     conn_id, MPIDI_OFI_global.conn_mgr.conn_list[conn_id].state));

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_DYNPROC_SEND_DISCONNECT);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
