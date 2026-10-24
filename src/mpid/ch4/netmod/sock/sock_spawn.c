/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpidimpl.h"
#include "sock_impl.h"

/* The dynamic messages are used before two processes share a communicator,
 * e.g. in MPI_Intercomm_create and MPI_Comm_connect/accept. They are matched
 * on a context id and a tag. The receives are blocking, so there is only a
 * queue of received messages and no queue of posted receives. */

static int dynamic_send(MPIR_Lpid remote_lpid, int context_id, int tag, const void *buf, int size);
static int dynamic_recv(int context_id, int tag, void *buf, int size, int timeout);
static MPIDI_SOCK_dynamic_msg_t *dynamic_dequeue(int context_id, int tag);

/* Called from progress when a dynamic message arrives. Takes over buf. */
int MPIDI_SOCK_dynamic_enqueue(MPIDI_SOCK_am_header_t * hdr, char *buf)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_Assert(hdr->am_hdr_sz == sizeof(MPIDI_SOCK_dynamic_hdr_t));

    MPIDI_SOCK_dynamic_msg_t *msg = MPL_malloc(sizeof(MPIDI_SOCK_dynamic_msg_t), MPL_MEM_OTHER);
    if (!msg) {
        MPL_free(buf);
        MPIR_ERR_SETANDJUMP(mpi_errno, MPI_ERR_OTHER, "**nomem");
    }
    memcpy(&msg->hdr, buf, sizeof(MPIDI_SOCK_dynamic_hdr_t));
    msg->data_sz = hdr->data_sz;
    msg->buf = buf;
    msg->data = buf + hdr->am_hdr_sz;
    msg->next = NULL;

    if (MPIDI_SOCK_global.dynamic_tail) {
        MPIDI_SOCK_global.dynamic_tail->next = msg;
    } else {
        MPIDI_SOCK_global.dynamic_head = msg;
    }
    MPIDI_SOCK_global.dynamic_tail = msg;

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

void MPIDI_SOCK_dynamic_finalize(void)
{
    while (MPIDI_SOCK_global.dynamic_head) {
        MPIDI_SOCK_dynamic_msg_t *msg = MPIDI_SOCK_global.dynamic_head;
        MPIDI_SOCK_global.dynamic_head = msg->next;
        MPL_free(msg->buf);
        MPL_free(msg);
    }
    MPIDI_SOCK_global.dynamic_tail = NULL;
}

/* Remove and return the first matching message, or NULL */
static MPIDI_SOCK_dynamic_msg_t *dynamic_dequeue(int context_id, int tag)
{
    MPIDI_SOCK_dynamic_msg_t *prev = NULL;
    MPIDI_SOCK_dynamic_msg_t *msg = MPIDI_SOCK_global.dynamic_head;
    while (msg) {
        if (msg->hdr.context_id == context_id && msg->hdr.tag == tag) {
            if (prev) {
                prev->next = msg->next;
            } else {
                MPIDI_SOCK_global.dynamic_head = msg->next;
            }
            if (MPIDI_SOCK_global.dynamic_tail == msg) {
                MPIDI_SOCK_global.dynamic_tail = prev;
            }
            return msg;
        }
        prev = msg;
        msg = msg->next;
    }
    return NULL;
}

/* The message is copied, so this returns without waiting for it to be written */
static int dynamic_send(MPIR_Lpid remote_lpid, int context_id, int tag, const void *buf, int size)
{
    int mpi_errno = MPI_SUCCESS;

    MPIDI_SOCK_dynamic_hdr_t hdr;
    hdr.context_id = context_id;
    hdr.tag = tag;

    void *pack_buf = NULL;
    if (size > 0) {
        pack_buf = MPL_malloc(size, MPL_MEM_BUFFER);
        MPIR_ERR_CHKANDJUMP(!pack_buf, mpi_errno, MPI_ERR_OTHER, "**nomem");
        memcpy(pack_buf, buf, size);
    }

    /* MPIDI_SOCK_send takes over pack_buf */
    MPIDI_av_entry_t *av = MPIDIU_lpid_to_av_slow(remote_lpid);
    mpi_errno = MPIDI_SOCK_send(av, MPIDI_SOCK_DYNAMIC_HANDLER_ID, &hdr, sizeof(hdr),
                                pack_buf, size, pack_buf, 0, 0, NULL);
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

/* Called inside the critical section of vci 0. timeout is in seconds; there
 * is no timeout if it is not positive. */
static int dynamic_recv(int context_id, int tag, void *buf, int size, int timeout)
{
    int mpi_errno = MPI_SUCCESS;

    MPL_time_t time_start;
    MPL_wtime(&time_start);
    while (true) {
        MPIDI_SOCK_dynamic_msg_t *msg = dynamic_dequeue(context_id, tag);
        if (msg) {
            MPI_Aint data_sz = msg->data_sz;
            if (data_sz <= size) {
                memcpy(buf, msg->data, data_sz);
            }
            MPL_free(msg->buf);
            MPL_free(msg);
            MPIR_ERR_CHKANDJUMP(data_sz > size, mpi_errno, MPI_ERR_TRUNCATE, "**truncate");
            break;
        }

        int made_progress = 0;
        mpi_errno = MPIDI_SOCK_progress(&made_progress);
        MPIR_ERR_CHECK(mpi_errno);

        if (timeout > 0) {
            MPL_time_t time_now;
            double time_gap;
            MPL_wtime(&time_now);
            MPL_wtime_diff(&time_start, &time_now, &time_gap);
            MPIR_ERR_CHKANDJUMP(time_gap > (double) timeout, mpi_errno, MPIX_ERR_TIMEOUT,
                                "**timeout");
        }

        /* let other threads make progress */
        MPID_THREAD_CS_YIELD(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
        MPID_THREAD_CS_YIELD(VCI, MPIDI_VCI_LOCK(0));
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIDI_SOCK_dynamic_send(MPIR_Lpid remote_lpid, int tag, const void *buf, int size, int timeout)
{
#ifdef MPICH_DEBUG_MUTEX
    MPID_THREAD_ASSERT_IN_CS(VCI, MPIDI_VCI_LOCK(0));
#endif
    return dynamic_send(remote_lpid, 0, tag, buf, size);
}

int MPIDI_SOCK_dynamic_recv(int tag, void *buf, int size, int timeout)
{
#ifdef MPICH_DEBUG_MUTEX
    MPID_THREAD_ASSERT_IN_CS(VCI, MPIDI_VCI_LOCK(0));
#endif
    return dynamic_recv(0, tag, buf, size, timeout);
}

int MPIDI_SOCK_dynamic_sendrecv(MPIR_Lpid remote_lpid, MPIR_Comm * peer_comm, int tag,
                                const void *send_buf, int send_size, void *recv_buf, int recv_size,
                                int timeout)
{
    int mpi_errno = MPI_SUCCESS;

#ifdef MPICH_DEBUG_MUTEX
    MPID_THREAD_ASSERT_IN_CS(VCI, MPIDI_VCI_LOCK(0));
#endif

    if (send_size > 0) {
        mpi_errno = dynamic_send(remote_lpid, peer_comm->context_id, tag, send_buf, send_size);
        MPIR_ERR_CHECK(mpi_errno);
    }

    if (recv_size > 0) {
        mpi_errno = dynamic_recv(peer_comm->recvcontext_id, tag, recv_buf, recv_size, timeout);
        MPIR_ERR_CHECK(mpi_errno);
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIDI_SOCK_get_local_upids(MPIR_Comm * comm, int **local_upid_size, char **local_upids)
{
    int mpi_errno = MPI_SUCCESS;

#ifdef MPICH_DEBUG_MUTEX
    MPID_THREAD_ASSERT_IN_CS(VCI, MPIDI_VCI_LOCK(0));
#endif
    MPIR_CHKPMEM_DECL();
    MPIR_CHKPMEM_MALLOC((*local_upid_size), comm->local_size * sizeof(int), MPL_MEM_ADDRESS);
    MPIR_CHKPMEM_MALLOC((*local_upids), comm->local_size * sizeof(MPIDI_SOCK_name_t),
                        MPL_MEM_BUFFER);

    for (int i = 0; i < comm->local_size; i++) {
        MPIDI_av_entry_t *av = MPIDIU_comm_rank_to_av(comm, i);
        (*local_upid_size)[i] = sizeof(MPIDI_SOCK_name_t);
        memcpy((*local_upids) + i * sizeof(MPIDI_SOCK_name_t), &MPIDI_SOCK_AV(av).name,
               sizeof(MPIDI_SOCK_name_t));
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    MPIR_CHKPMEM_REAP();
    goto fn_exit;
}

int MPIDI_SOCK_insert_upid(MPIR_Lpid lpid, const char *upid, int upid_len)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_av_entry_t *av = MPIDIU_lpid_to_av_slow(lpid);

#ifdef MPICH_DEBUG_MUTEX
    MPID_THREAD_ASSERT_IN_CS(VCI, MPIDI_VCI_LOCK(0));
#endif
    MPIR_Assert(upid_len == sizeof(MPIDI_SOCK_name_t));

    bool is_dynamic = (lpid & MPIR_LPID_DYNAMIC_MASK);
    if (is_dynamic) {
        /* dynamic entry */
        memcpy(&MPIDI_SOCK_AV(av).name, upid, sizeof(MPIDI_SOCK_name_t));
        MPIDI_SOCK_AV(av).conn = NULL;
    } else if (MPIDI_SOCK_AV(av).name.port == 0) {
        /* new av entry */
        MPIDI_av_entry_t *dynamic_av = MPIDIU_find_dynamic_av(upid, upid_len);
        if (dynamic_av) {
            /* just copy it over, including the connection if there is one */
            MPIDI_SOCK_AV(av) = MPIDI_SOCK_AV(dynamic_av);
        } else {
            memcpy(&MPIDI_SOCK_AV(av).name, upid, sizeof(MPIDI_SOCK_name_t));
            MPIDI_SOCK_AV(av).conn = NULL;
        }
        MPIDIU_upidhash_add(upid, upid_len, lpid);
    } else {
        /* A known entry, nothing to do */
    }

    return mpi_errno;
}
