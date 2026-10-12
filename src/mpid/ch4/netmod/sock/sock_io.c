/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpidimpl.h"
#include "sock_impl.h"
#include <sys/uio.h>

/* limit on the messages read from one connection in one progress call, so
 * that a busy connection does not starve the others */
#define MAX_RECV_PER_CONN 16

static int sock_writev(MPIDI_SOCK_conn_t * conn, struct iovec *iov, int iov_cnt,
                       MPI_Aint * p_written);
static int conn_flush(MPIDI_SOCK_conn_t * conn, int *made_progress);
static int conn_recv(MPIDI_SOCK_conn_t * conn, int *made_progress);

/* Write as much as the socket takes without blocking. *p_written is 0 if the
 * socket is full. */
static int sock_writev(MPIDI_SOCK_conn_t * conn, struct iovec *iov, int iov_cnt,
                       MPI_Aint * p_written)
{
    int mpi_errno = MPI_SUCCESS;
    char strerrbuf[MPIR_STRERROR_BUF_SIZE] ATTRIBUTE((unused));

    struct msghdr msg;
    memset(&msg, 0, sizeof(msg));
    msg.msg_iov = iov;
    msg.msg_iovlen = iov_cnt;

    ssize_t n;
    do {
        /* MSG_NOSIGNAL: get EPIPE rather than SIGPIPE if the peer is gone */
        n = sendmsg(conn->fd, &msg, MSG_NOSIGNAL);
    } while (n < 0 && errno == EINTR);

    if (n < 0) {
        MPIR_ERR_CHKANDJUMP2(errno != EAGAIN && errno != EWOULDBLOCK, mpi_errno, MPI_ERR_OTHER,
                             "**ch4_sock_write", "**ch4_sock_write %s %d",
                             MPIR_Strerror(errno, strerrbuf, MPIR_STRERROR_BUF_SIZE), errno);
        n = 0;
    }
    *p_written = n;

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

/* Send an active message. Write it right away if the connection has nothing
 * queued and the socket takes it, otherwise queue what is left.
 *
 * am_hdr is copied if the message is queued. data must stay valid until the
 * message is sent: it is either the user buffer, in which case sreq is not
 * NULL, or pack_buf, which is freed here once the message is sent. */
int MPIDI_SOCK_send(MPIDI_av_entry_t * av, int handler_id, const void *am_hdr, MPI_Aint am_hdr_sz,
                    const void *data, MPI_Aint data_sz, void *pack_buf,
                    int src_vci, int dst_vci, MPIR_Request * sreq)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_SOCK_sendq_elem_t *elem = NULL;

#ifdef MPICH_DEBUG_MUTEX
    MPID_THREAD_ASSERT_IN_CS(VCI, MPIDI_VCI_LOCK(0));
#endif
    MPIR_Assert(src_vci == 0 && dst_vci == 0);

    MPIDI_SOCK_conn_t *conn = MPIDI_SOCK_AV(av).conn;
    if (!conn) {
        mpi_errno = MPIDI_SOCK_conn_connect(av, &conn);
        MPIR_ERR_CHECK(mpi_errno);
    }

    MPIDI_SOCK_am_header_t hdr;
    hdr.handler_id = handler_id;
    hdr.src_vci = src_vci;
    hdr.dst_vci = dst_vci;
    hdr.am_hdr_sz = am_hdr_sz;
    hdr.data_sz = data_sz;

    MPI_Aint hdr_len = sizeof(hdr) + am_hdr_sz;
    MPI_Aint written = 0;
    if (!conn->sendq_head) {
        struct iovec iov[3];
        iov[0].iov_base = &hdr;
        iov[0].iov_len = sizeof(hdr);
        iov[1].iov_base = (void *) am_hdr;
        iov[1].iov_len = am_hdr_sz;
        iov[2].iov_base = (void *) data;
        iov[2].iov_len = data_sz;

        mpi_errno = sock_writev(conn, iov, 3, &written);
        MPIR_ERR_CHECK(mpi_errno);

        if (written == hdr_len + data_sz) {
            /* all sent */
            MPL_free(pack_buf);
            pack_buf = NULL;
            if (sreq) {
                mpi_errno = MPIDIG_global.origin_cbs[handler_id] (sreq);
                MPIR_ERR_CHECK(mpi_errno);
            }
            goto fn_exit;
        }
    }

    /* queue the rest */
    elem = MPL_malloc(sizeof(MPIDI_SOCK_sendq_elem_t), MPL_MEM_OTHER);
    MPIR_ERR_CHKANDJUMP(!elem, mpi_errno, MPI_ERR_OTHER, "**nomem");
    elem->hdr_buf = MPL_malloc(hdr_len, MPL_MEM_BUFFER);
    MPIR_ERR_CHKANDJUMP(!elem->hdr_buf, mpi_errno, MPI_ERR_OTHER, "**nomem");
    memcpy(elem->hdr_buf, &hdr, sizeof(hdr));
    memcpy((char *) elem->hdr_buf + sizeof(hdr), am_hdr, am_hdr_sz);

    elem->next = NULL;
    elem->sreq = sreq;
    elem->handler_id = handler_id;
    elem->hdr_len = hdr_len;
    elem->data = data;
    elem->data_len = data_sz;
    elem->pack_buf = pack_buf;
    elem->offset = written;

    if (conn->sendq_tail) {
        conn->sendq_tail->next = elem;
    } else {
        conn->sendq_head = elem;
        MPIDI_SOCK_global.pollfds[conn->poll_idx].events = POLLOUT;
    }
    conn->sendq_tail = elem;

  fn_exit:
    return mpi_errno;
  fn_fail:
    MPL_free(elem);
    MPL_free(pack_buf);
    goto fn_exit;
}

/* Write the queued messages of a connection until the socket is full */
static int conn_flush(MPIDI_SOCK_conn_t * conn, int *made_progress)
{
    int mpi_errno = MPI_SUCCESS;

    while (conn->sendq_head) {
        MPIDI_SOCK_sendq_elem_t *elem = conn->sendq_head;

        struct iovec iov[2];
        int iov_cnt = 0;
        if (elem->offset < elem->hdr_len) {
            iov[iov_cnt].iov_base = (char *) elem->hdr_buf + elem->offset;
            iov[iov_cnt].iov_len = elem->hdr_len - elem->offset;
            iov_cnt++;
            if (elem->data_len > 0) {
                iov[iov_cnt].iov_base = (void *) elem->data;
                iov[iov_cnt].iov_len = elem->data_len;
                iov_cnt++;
            }
        } else {
            MPI_Aint data_offset = elem->offset - elem->hdr_len;
            iov[iov_cnt].iov_base = (char *) elem->data + data_offset;
            iov[iov_cnt].iov_len = elem->data_len - data_offset;
            iov_cnt++;
        }

        MPI_Aint written;
        mpi_errno = sock_writev(conn, iov, iov_cnt, &written);
        MPIR_ERR_CHECK(mpi_errno);

        if (written > 0) {
            *made_progress = 1;
        }
        elem->offset += written;
        if (elem->offset < elem->hdr_len + elem->data_len) {
            /* the socket is full */
            break;
        }

        /* Remove the message from the queue before the callback, which may
         * send on this connection. */
        conn->sendq_head = elem->next;
        if (!conn->sendq_head) {
            conn->sendq_tail = NULL;
            MPIDI_SOCK_global.pollfds[conn->poll_idx].events = 0;
        }

        MPIR_Request *sreq = elem->sreq;
        int handler_id = elem->handler_id;
        MPL_free(elem->hdr_buf);
        MPL_free(elem->pack_buf);
        MPL_free(elem);

        if (sreq) {
            mpi_errno = MPIDIG_global.origin_cbs[handler_id] (sreq);
            MPIR_ERR_CHECK(mpi_errno);
        }
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

/* Read from an incoming connection and run the handler of each complete
 * message. */
static int conn_recv(MPIDI_SOCK_conn_t * conn, int *made_progress)
{
    int mpi_errno = MPI_SUCCESS;
    char strerrbuf[MPIR_STRERROR_BUF_SIZE] ATTRIBUTE((unused));

    int num_msgs = 0;
    while (num_msgs < MAX_RECV_PER_CONN) {
        if (conn->recv_got < conn->recv_need) {
            char *p;
            if (conn->recv_state == MPIDI_SOCK_RECV_HDR) {
                p = (char *) &conn->recv_hdr;
            } else {
                p = conn->recv_buf;
            }

            ssize_t n;
            do {
                n = read(conn->fd, p + conn->recv_got, conn->recv_need - conn->recv_got);
            } while (n < 0 && errno == EINTR);

            if (n < 0) {
                if (errno == EAGAIN || errno == EWOULDBLOCK) {
                    break;
                }
                MPIR_ERR_SETANDJUMP2(mpi_errno, MPI_ERR_OTHER, "**ch4_sock_read",
                                     "**ch4_sock_read %s %d",
                                     MPIR_Strerror(errno, strerrbuf, MPIR_STRERROR_BUF_SIZE),
                                     errno);
            } else if (n == 0) {
                /* The peer closed the connection. That is expected between
                 * messages, when the peer finalizes. */
                bool in_msg = (conn->recv_state != MPIDI_SOCK_RECV_HDR || conn->recv_got > 0);
                MPIDI_SOCK_conn_close(conn);
                MPIR_ERR_CHKANDJUMP(in_msg, mpi_errno, MPI_ERR_OTHER, "**ch4_sock_closed");
                break;
            }

            *made_progress = 1;
            conn->recv_got += n;
            if (conn->recv_got < conn->recv_need) {
                /* we got less than we asked for, there is no more for now */
                break;
            }
        }

        if (conn->recv_state == MPIDI_SOCK_RECV_HDR) {
            /* got the header, read the am header and data next */
            MPI_Aint body_sz = conn->recv_hdr.am_hdr_sz + conn->recv_hdr.data_sz;
            conn->recv_state = MPIDI_SOCK_RECV_BODY;
            conn->recv_need = body_sz;
            conn->recv_got = 0;
            if (body_sz > 0) {
                conn->recv_buf = MPL_malloc(body_sz, MPL_MEM_BUFFER);
                MPIR_ERR_CHKANDJUMP(!conn->recv_buf, mpi_errno, MPI_ERR_OTHER, "**nomem");
                continue;
            }
        }

        /* got a complete message */
        MPIDI_SOCK_am_header_t *hdr = &conn->recv_hdr;
        char *buf = conn->recv_buf;

        /* Reset the state before the handler. The handler does not read from
         * this connection, but keep the connection consistent anyway. */
        conn->recv_buf = NULL;
        conn->recv_state = MPIDI_SOCK_RECV_HDR;
        conn->recv_need = sizeof(MPIDI_SOCK_am_header_t);
        conn->recv_got = 0;

        if (hdr->handler_id == MPIDI_SOCK_DYNAMIC_HANDLER_ID) {
            /* the queue takes over buf */
            mpi_errno = MPIDI_SOCK_dynamic_enqueue(hdr, buf);
            MPIR_ERR_CHECK(mpi_errno);
        } else {
            uint32_t attr = 0;  /* is_local = 0, is_async = 0 */
            MPIDIG_AM_ATTR_SET_VCIS(attr, hdr->src_vci, hdr->dst_vci);
            mpi_errno = MPIDIG_global.target_msg_cbs[hdr->handler_id] (buf, buf + hdr->am_hdr_sz,
                                                                       hdr->data_sz, attr, NULL);
            MPL_free(buf);
            MPIR_ERR_CHECK(mpi_errno);
        }

        num_msgs++;
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIDI_SOCK_progress(int *made_progress)
{
    int mpi_errno = MPI_SUCCESS;
    char strerrbuf[MPIR_STRERROR_BUF_SIZE] ATTRIBUTE((unused));

    int ret;
    do {
        ret = poll(MPIDI_SOCK_global.pollfds, MPIDI_SOCK_global.num_pollfds, 0);
    } while (ret < 0 && errno == EINTR);
    MPIR_ERR_CHKANDJUMP2(ret < 0, mpi_errno, MPI_ERR_OTHER, "**ch4_sock_poll",
                         "**ch4_sock_poll %s %d",
                         MPIR_Strerror(errno, strerrbuf, MPIR_STRERROR_BUF_SIZE), errno);
    if (ret == 0) {
        goto fn_exit;
    }

    /* The handlers below may add connections, which appends to the arrays and
     * may reallocate them. Only look at the entries that were polled and
     * don't keep pointers into the arrays. */
    int num = MPIDI_SOCK_global.num_pollfds;
    for (int i = 1; i < num; i++) {
        short revents = MPIDI_SOCK_global.pollfds[i].revents;
        if (revents == 0) {
            continue;
        }
        MPIDI_SOCK_global.pollfds[i].revents = 0;

        MPIDI_SOCK_conn_t *conn = MPIDI_SOCK_global.conns[i];
        if (conn->is_outgoing) {
            /* on error, the write reports it */
            mpi_errno = conn_flush(conn, made_progress);
            MPIR_ERR_CHECK(mpi_errno);
        } else {
            /* on error or hangup, the read reports it */
            mpi_errno = conn_recv(conn, made_progress);
            MPIR_ERR_CHECK(mpi_errno);
        }
    }

    if (MPIDI_SOCK_global.pollfds[0].revents) {
        MPIDI_SOCK_global.pollfds[0].revents = 0;
        mpi_errno = MPIDI_SOCK_conn_accept();
        MPIR_ERR_CHECK(mpi_errno);
        *made_progress = 1;
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

/* Write out all queued messages. Keep receiving meanwhile, since the peers
 * may be blocked on us reading. */
int MPIDI_SOCK_flush(void)
{
    int mpi_errno = MPI_SUCCESS;

    while (true) {
        bool pending = false;
        for (int i = 1; i < MPIDI_SOCK_global.num_pollfds; i++) {
            if (MPIDI_SOCK_global.conns[i]->sendq_head) {
                pending = true;
                break;
            }
        }
        if (!pending) {
            break;
        }

        int made_progress = 0;
        mpi_errno = MPIDI_SOCK_progress(&made_progress);
        MPIR_ERR_CHECK(mpi_errno);
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
