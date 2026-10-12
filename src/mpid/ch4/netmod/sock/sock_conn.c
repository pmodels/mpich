/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpidimpl.h"
#include "sock_impl.h"
#include <fcntl.h>
#include <unistd.h>
#include <netinet/in.h>
#include <netinet/tcp.h>

static int add_conn(int fd, bool is_outgoing, MPIDI_SOCK_conn_t ** conn_p);

int MPIDI_SOCK_set_nonblock(int fd)
{
    int mpi_errno = MPI_SUCCESS;
    char strerrbuf[MPIR_STRERROR_BUF_SIZE] ATTRIBUTE((unused));

    int flags = fcntl(fd, F_GETFL, 0);
    int ret = (flags == -1) ? -1 : fcntl(fd, F_SETFL, flags | O_NONBLOCK);
    MPIR_ERR_CHKANDJUMP2(ret == -1, mpi_errno, MPI_ERR_OTHER, "**ch4_sock_nonblock",
                         "**ch4_sock_nonblock %s %d",
                         MPIR_Strerror(errno, strerrbuf, MPIR_STRERROR_BUF_SIZE), errno);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

/* Set up the poll array with the listening socket in slot 0 */
int MPIDI_SOCK_conn_init(void)
{
    int mpi_errno = MPI_SUCCESS;

    MPIDI_SOCK_global.max_pollfds = 16;
    MPIDI_SOCK_global.pollfds = MPL_malloc(MPIDI_SOCK_global.max_pollfds * sizeof(struct pollfd),
                                           MPL_MEM_OTHER);
    MPIR_ERR_CHKANDJUMP(!MPIDI_SOCK_global.pollfds, mpi_errno, MPI_ERR_OTHER, "**nomem");
    MPIDI_SOCK_global.conns = MPL_malloc(MPIDI_SOCK_global.max_pollfds *
                                         sizeof(MPIDI_SOCK_conn_t *), MPL_MEM_OTHER);
    MPIR_ERR_CHKANDJUMP(!MPIDI_SOCK_global.conns, mpi_errno, MPI_ERR_OTHER, "**nomem");

    MPIDI_SOCK_global.pollfds[0].fd = MPIDI_SOCK_global.listen_fd;
    MPIDI_SOCK_global.pollfds[0].events = POLLIN;
    MPIDI_SOCK_global.pollfds[0].revents = 0;
    MPIDI_SOCK_global.conns[0] = NULL;
    MPIDI_SOCK_global.num_pollfds = 1;

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

/* Close all connections and free the poll array */
int MPIDI_SOCK_conn_finalize(void)
{
    for (int i = 1; i < MPIDI_SOCK_global.num_pollfds; i++) {
        MPIDI_SOCK_conn_t *conn = MPIDI_SOCK_global.conns[i];
        MPIDI_SOCK_conn_close(conn);
        MPL_free(conn);
    }
    MPL_free(MPIDI_SOCK_global.pollfds);
    MPL_free(MPIDI_SOCK_global.conns);
    MPIDI_SOCK_global.pollfds = NULL;
    MPIDI_SOCK_global.conns = NULL;
    MPIDI_SOCK_global.num_pollfds = 0;

    return MPI_SUCCESS;
}

/* Close the socket of a connection. The connection keeps its slot in the poll
 * array, where a negative fd is ignored by poll. */
void MPIDI_SOCK_conn_close(MPIDI_SOCK_conn_t * conn)
{
    if (conn->fd >= 0) {
        close(conn->fd);
        conn->fd = -1;
        MPIDI_SOCK_global.pollfds[conn->poll_idx].fd = -1;
    }
    MPL_free(conn->recv_buf);
    conn->recv_buf = NULL;
    MPL_free(conn->recv_chunk);
    conn->recv_chunk = NULL;
    while (conn->sendq_head) {
        MPIDI_SOCK_sendq_elem_t *elem = conn->sendq_head;
        conn->sendq_head = elem->next;
        MPL_free(elem->hdr_buf);
        MPL_free(elem->pack_buf);
        MPL_free(elem);
    }
    conn->sendq_tail = NULL;
}

/* Create a connection for a connected socket and add it to the poll array.
 * NOTE: this may reallocate MPIDI_SOCK_global.pollfds and conns. */
static int add_conn(int fd, bool is_outgoing, MPIDI_SOCK_conn_t ** conn_p)
{
    int mpi_errno = MPI_SUCCESS;

    MPIDI_SOCK_conn_t *conn = MPL_calloc(1, sizeof(MPIDI_SOCK_conn_t), MPL_MEM_OTHER);
    MPIR_ERR_CHKANDJUMP(!conn, mpi_errno, MPI_ERR_OTHER, "**nomem");

    if (MPIDI_SOCK_global.num_pollfds == MPIDI_SOCK_global.max_pollfds) {
        int n = MPIDI_SOCK_global.max_pollfds * 2;
        void *p = MPL_realloc(MPIDI_SOCK_global.pollfds, n * sizeof(struct pollfd), MPL_MEM_OTHER);
        MPIR_ERR_CHKANDJUMP(!p, mpi_errno, MPI_ERR_OTHER, "**nomem");
        MPIDI_SOCK_global.pollfds = p;
        p = MPL_realloc(MPIDI_SOCK_global.conns, n * sizeof(MPIDI_SOCK_conn_t *), MPL_MEM_OTHER);
        MPIR_ERR_CHKANDJUMP(!p, mpi_errno, MPI_ERR_OTHER, "**nomem");
        MPIDI_SOCK_global.conns = p;
        MPIDI_SOCK_global.max_pollfds = n;
    }

    conn->fd = fd;
    conn->is_outgoing = is_outgoing;
    conn->recv_state = MPIDI_SOCK_RECV_HDR;
    conn->recv_need = sizeof(MPIDI_SOCK_am_header_t);

    int idx = MPIDI_SOCK_global.num_pollfds++;
    conn->poll_idx = idx;
    MPIDI_SOCK_global.conns[idx] = conn;
    MPIDI_SOCK_global.pollfds[idx].fd = fd;
    /* an outgoing connection is polled for POLLOUT only while its send queue is not empty */
    MPIDI_SOCK_global.pollfds[idx].events = is_outgoing ? 0 : POLLIN;
    MPIDI_SOCK_global.pollfds[idx].revents = 0;

    *conn_p = conn;

  fn_exit:
    return mpi_errno;
  fn_fail:
    MPL_free(conn);
    goto fn_exit;
}

/* Open the outgoing connection to a process.
 *
 * The connect is blocking. It completes once the connection is in the listen
 * queue of the peer, so it does not depend on the peer calling accept. */
int MPIDI_SOCK_conn_connect(MPIDI_av_entry_t * av, MPIDI_SOCK_conn_t ** conn_p)
{
    int mpi_errno = MPI_SUCCESS;
    char strerrbuf[MPIR_STRERROR_BUF_SIZE] ATTRIBUTE((unused));
    int fd = -1;

    MPIDI_SOCK_name_t *name = &MPIDI_SOCK_AV(av).name;
    MPIR_ERR_CHKANDJUMP(name->port == 0, mpi_errno, MPI_ERR_OTHER, "**ch4_sock_noaddr");

    fd = MPL_socket();
    MPIR_ERR_CHKANDJUMP2(fd < 0, mpi_errno, MPI_ERR_OTHER, "**sock_create", "**sock_create %s %d",
                         MPIR_Strerror(errno, strerrbuf, MPIR_STRERROR_BUF_SIZE), errno);

    int ret;
    do {
        ret = MPL_connect(fd, &name->saddr, name->port);
    } while (ret != 0 && errno == EINTR);
    if (ret != 0) {
        char addr_str[100];
        MPL_sockaddr_to_str(&name->saddr, addr_str, sizeof(addr_str));
        MPIR_ERR_SETANDJUMP4(mpi_errno, MPI_ERR_OTHER, "**ch4_sock_connect",
                             "**ch4_sock_connect %s %d %s %d", addr_str, name->port,
                             MPIR_Strerror(errno, strerrbuf, MPIR_STRERROR_BUF_SIZE), errno);
    }

    mpi_errno = MPIDI_SOCK_set_nonblock(fd);
    MPIR_ERR_CHECK(mpi_errno);

    /* messages are written in one piece, don't delay them */
    int one = 1;
    ret = setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &one, sizeof(one));
    MPIR_ERR_CHKANDJUMP2(ret != 0, mpi_errno, MPI_ERR_OTHER, "**setsockopt", "**setsockopt %s %d",
                         MPIR_Strerror(errno, strerrbuf, MPIR_STRERROR_BUF_SIZE), errno);

    mpi_errno = add_conn(fd, true, conn_p);
    MPIR_ERR_CHECK(mpi_errno);

    MPIDI_SOCK_AV(av).conn = *conn_p;

  fn_exit:
    return mpi_errno;
  fn_fail:
    if (fd >= 0) {
        close(fd);
    }
    goto fn_exit;
}

/* Accept all the connections that are waiting on the listening socket */
int MPIDI_SOCK_conn_accept(void)
{
    int mpi_errno = MPI_SUCCESS;
    char strerrbuf[MPIR_STRERROR_BUF_SIZE] ATTRIBUTE((unused));

    while (true) {
        int fd = accept(MPIDI_SOCK_global.listen_fd, NULL, NULL);
        if (fd < 0) {
            if (errno == EINTR || errno == ECONNABORTED) {
                continue;
            } else if (errno == EAGAIN || errno == EWOULDBLOCK) {
                break;
            }
            MPIR_ERR_SETANDJUMP2(mpi_errno, MPI_ERR_OTHER, "**ch4_sock_accept",
                                 "**ch4_sock_accept %s %d",
                                 MPIR_Strerror(errno, strerrbuf, MPIR_STRERROR_BUF_SIZE), errno);
        }

        mpi_errno = MPIDI_SOCK_set_nonblock(fd);
        if (mpi_errno) {
            close(fd);
            MPIR_ERR_POP(mpi_errno);
        }

        MPIDI_SOCK_conn_t *conn;
        mpi_errno = add_conn(fd, false, &conn);
        if (mpi_errno) {
            close(fd);
            MPIR_ERR_POP(mpi_errno);
        }
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
