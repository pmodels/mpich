/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpl.h"

#if defined MPL_HAVE_SYS_UIO_H
/* Some platforms, such as Mac OSX (at least as of 10.9.1) hang when
 * attempting to send more than 2GB data, even though the writev
 * function is supposed to be able to handle large data.  This
 * function is a simple workaround for this case by attempting to send
 * lesser data, and having the upper layer retry later if needed.
 * This adds a small amount of bookkeeping overhead, but it should be
 * negligible compared to the system call overhead for small messages
 * and compared to the data transmission overhead for large
 * messages. */
ssize_t MPL_large_writev(int fd, const struct iovec *iov, int iovcnt)
{
    ssize_t total_size, tmp;
    struct iovec dummy;
    int i;

    /* If the total data fits into INT_MAX, directly use writev */
    total_size = 0;
    for (i = 0; i < iovcnt; i++)
        total_size += iov[i].iov_len;

    if (total_size <= INT_MAX) {
        do {
            tmp = writev(fd, iov, iovcnt);
        } while (tmp == -1 && errno == EINTR);
        return tmp;
    }

    /* Total data is larger than INT_MAX.  Issue writev with fewer
     * elements, so as to not exceed INT_MAX.  In this case, doing
     * multiple write calls, one for each iov segment is not a big
     * deal with respect to performance. */

    total_size = 0;
    for (i = 0; i < iovcnt; i++) {
        if (iov[i].iov_len <= INT_MAX) {
            do {
                tmp = writev(fd, &iov[i], 1);
            } while (tmp == -1 && errno == EINTR);
        } else {
            dummy.iov_base = iov[i].iov_base;
            dummy.iov_len = INT_MAX;
            do {
                tmp = writev(fd, &dummy, 1);
            } while (tmp == -1 && errno == EINTR);
        }

        if (tmp < 0)
            return tmp;
        else if (tmp < iov[i].iov_len) {
            total_size += tmp;
            return total_size;
        } else
            total_size += tmp;
    }

    return total_size;
}


ssize_t MPL_large_readv(int fd, const struct iovec * iov, int iovcnt)
{
    ssize_t total_size, tmp;
    struct iovec dummy;
    int i;

    /* If the total data fits into INT_MAX, directly use readv */
    total_size = 0;
    for (i = 0; i < iovcnt; i++)
        total_size += iov[i].iov_len;

    if (total_size <= INT_MAX) {
        do {
            tmp = readv(fd, iov, iovcnt);
        } while (tmp == -1 && errno == EINTR);
        return tmp;
    }

    /* Total data is larger than INT_MAX.  Issue readv with fewer
     * elements, so as to not exceed INT_MAX.  In this case, doing
     * multiple read calls, one for each iov segment is not a big
     * deal with respect to performance. */

    total_size = 0;
    for (i = 0; i < iovcnt; i++) {
        if (iov[i].iov_len <= INT_MAX) {
            do {
                tmp = readv(fd, &iov[i], 1);
            } while (tmp == -1 && errno == EINTR);
        } else {
            dummy.iov_base = iov[i].iov_base;
            dummy.iov_len = INT_MAX;
            do {
                tmp = readv(fd, &dummy, 1);
            } while (tmp == -1 && errno == EINTR);
        }

        if (tmp < 0)
            return tmp;
        else if (tmp < iov[i].iov_len) {
            total_size += tmp;
            return total_size;
        } else
            total_size += tmp;
    }

    return total_size;
}
#endif /* MPL_HAVE_SYS_UIO_H */

#ifdef _WIN32
/* Native Windows (Winsock) implementation of the MPL_sock_* primitives. */

#include <stdlib.h>

static volatile LONG wsa_initialized = 0;

int MPL_sock_init(void)
{
    if (InterlockedCompareExchange(&wsa_initialized, 1, 0) == 0) {
        WSADATA wsadata;
        if (WSAStartup(MAKEWORD(2, 2), &wsadata) != 0) {
            wsa_initialized = 0;
            return -1;
        }
    }
    return 0;
}

/* Make sure Winsock is up before anyone calls gethostname/getaddrinfo, which
 * happens in several places before the first socket is created. */
static void __attribute__ ((constructor)) mpli_sock_init_ctor(void)
{
    MPL_sock_init();
}

static int wsa_to_errno(int wsa_err)
{
    switch (wsa_err) {
        case WSAEINTR:
            return EINTR;
        case WSAEWOULDBLOCK:
            return EAGAIN;
        case WSAEINPROGRESS:
            return EINPROGRESS;
        case WSAEALREADY:
            return EALREADY;
        case WSAENOTSOCK:
        case WSAEBADF:
            return EBADF;
        case WSAEACCES:
            return EACCES;
        case WSAEFAULT:
            return EFAULT;
        case WSAEINVAL:
            return EINVAL;
        case WSAEMFILE:
            return EMFILE;
        case WSAEMSGSIZE:
            return EMSGSIZE;
        case WSAEAFNOSUPPORT:
            return EAFNOSUPPORT;
        case WSAEADDRINUSE:
            return EADDRINUSE;
        case WSAEADDRNOTAVAIL:
            return EADDRNOTAVAIL;
        case WSAENETDOWN:
            return ENETDOWN;
        case WSAENETUNREACH:
            return ENETUNREACH;
        case WSAENETRESET:
            return ENETRESET;
        case WSAECONNABORTED:
            return ECONNABORTED;
        case WSAECONNRESET:
            return ECONNRESET;
        case WSAENOBUFS:
            return ENOBUFS;
        case WSAEISCONN:
            return EISCONN;
        case WSAENOTCONN:
            return ENOTCONN;
        case WSAESHUTDOWN:
            return EPIPE;
        case WSAETIMEDOUT:
            return ETIMEDOUT;
        case WSAECONNREFUSED:
            return ECONNREFUSED;
        case WSAEHOSTUNREACH:
            return EHOSTUNREACH;
        default:
            return EIO;
    }
}

/* Translate the last Winsock error into errno; always returns -1 */
int MPLI_sock_set_errno(void)
{
    errno = wsa_to_errno(WSAGetLastError());
    return -1;
}

ssize_t MPL_sock_read(int fd, void *buf, size_t len)
{
    int n = recv((SOCKET) fd, buf, len > INT_MAX ? INT_MAX : (int) len, 0);
    return n == SOCKET_ERROR ? MPLI_sock_set_errno() : n;
}

ssize_t MPL_sock_write(int fd, const void *buf, size_t len)
{
    int n = send((SOCKET) fd, buf, len > INT_MAX ? INT_MAX : (int) len, 0);
    return n == SOCKET_ERROR ? MPLI_sock_set_errno() : n;
}

int MPL_sock_close(int fd)
{
    return closesocket((SOCKET) fd) == SOCKET_ERROR ? MPLI_sock_set_errno() : 0;
}

int MPL_sock_set_nonblock(int fd, int nonblock)
{
    u_long mode = nonblock ? 1 : 0;
    return ioctlsocket((SOCKET) fd, FIONBIO, &mode) == SOCKET_ERROR ? MPLI_sock_set_errno() : 0;
}

int MPL_sock_set_cloexec(int fd)
{
    /* Winsock sockets are inheritable by default */
    return SetHandleInformation((HANDLE) (intptr_t) fd, HANDLE_FLAG_INHERIT, 0) ? 0 : -1;
}

/* Like poll(): entries with a negative fd are ignored and get revents = 0.
 * WSAPoll instead reports them as POLLNVAL (SOCKET is unsigned), so pass it
 * only the valid entries. */
int MPL_sock_poll(struct pollfd *fds, unsigned long nfds, int timeout)
{
    struct pollfd stack_fds[64], *wfds = stack_fds;
    unsigned long *idx, stack_idx[64], nvalid = 0;
    int n;

    if (nfds > 64) {
        wfds = malloc(nfds * sizeof(*wfds));
        idx = malloc(nfds * sizeof(*idx));
        if (!wfds || !idx) {
            free(wfds);
            free(idx);
            errno = ENOMEM;
            return -1;
        }
    } else {
        idx = stack_idx;
    }

    for (unsigned long i = 0; i < nfds; i++) {
        fds[i].revents = 0;
        if ((int) fds[i].fd < 0)
            continue;
        wfds[nvalid] = fds[i];
        /* WSAPoll rejects anything but the normal read/write events */
        wfds[nvalid].events &= (POLLIN | POLLOUT);
        idx[nvalid++] = i;
    }

    if (nvalid == 0) {
        /* WSAPoll fails on an empty set; just wait */
        if (timeout != 0)
            Sleep(timeout < 0 ? INFINITE : (DWORD) timeout);
        n = 0;
    } else {
        n = WSAPoll(wfds, nvalid, timeout);
        if (n == SOCKET_ERROR) {
            n = MPLI_sock_set_errno();
        } else {
            for (unsigned long j = 0; j < nvalid; j++)
                fds[idx[j]].revents = wfds[j].revents;
        }
    }

    if (wfds != stack_fds) {
        free(wfds);
        free(idx);
    }
    return n;
}

int MPL_sock_get_error(int fd, int *err)
{
    int len = sizeof(*err);
    if (getsockopt((SOCKET) fd, SOL_SOCKET, SO_ERROR, (char *) err, &len) == SOCKET_ERROR)
        return MPLI_sock_set_errno();
    /* SO_ERROR holds a Winsock error code */
    if (*err)
        *err = wsa_to_errno(*err);
    return 0;
}

int MPL_sock_accept(int fd, struct sockaddr *addr, socklen_t * addrlen)
{
    SOCKET s = accept((SOCKET) fd, addr, addrlen);
    return s == INVALID_SOCKET ? MPLI_sock_set_errno() : (int) s;
}

/* A connected pair of loopback TCP sockets, standing in for pipe() */
int MPL_sock_pair(int fds[2])
{
    SOCKET listener = INVALID_SOCKET, a = INVALID_SOCKET, b = INVALID_SOCKET;
    struct sockaddr_in addr;
    int addrlen = sizeof(addr);

    MPL_sock_init();
    listener = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listener == INVALID_SOCKET)
        goto fn_fail;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    addr.sin_port = 0;
    if (bind(listener, (struct sockaddr *) &addr, sizeof(addr)) == SOCKET_ERROR ||
        getsockname(listener, (struct sockaddr *) &addr, &addrlen) == SOCKET_ERROR ||
        listen(listener, 1) == SOCKET_ERROR)
        goto fn_fail;
    a = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (a == INVALID_SOCKET || connect(a, (struct sockaddr *) &addr, sizeof(addr)) == SOCKET_ERROR)
        goto fn_fail;
    /* Any local process can connect to the listener, so only accept the
     * connection that comes from our own socket a. */
    struct sockaddr_in a_addr, peer;
    int a_len = sizeof(a_addr);
    if (getsockname(a, (struct sockaddr *) &a_addr, &a_len) == SOCKET_ERROR)
        goto fn_fail;
    for (int tries = 0;; tries++) {
        int peer_len = sizeof(peer);
        b = accept(listener, (struct sockaddr *) &peer, &peer_len);
        if (b == INVALID_SOCKET)
            goto fn_fail;
        if (peer.sin_port == a_addr.sin_port && peer.sin_addr.s_addr == a_addr.sin_addr.s_addr)
            break;
        closesocket(b);
        b = INVALID_SOCKET;
        if (tries == 16) {
            WSASetLastError(WSAECONNREFUSED);
            goto fn_fail;
        }
    }
    closesocket(listener);
    /* used for 1-byte wakeup messages, which Nagle would hold back */
    BOOL nodelay = TRUE;
    setsockopt(a, IPPROTO_TCP, TCP_NODELAY, (const char *) &nodelay, sizeof(nodelay));
    setsockopt(b, IPPROTO_TCP, TCP_NODELAY, (const char *) &nodelay, sizeof(nodelay));
    fds[0] = (int) b;
    fds[1] = (int) a;
    return 0;

  fn_fail:
    MPLI_sock_set_errno();
    if (listener != INVALID_SOCKET)
        closesocket(listener);
    if (a != INVALID_SOCKET)
        closesocket(a);
    return -1;
}

/* Use at most this many buffers per WSASend/WSARecv; a short transfer is
 * fine since callers already handle partial reads and writes. */
#define MPLI_SOCK_MAX_WSABUF 64

static int fill_wsabuf(WSABUF * bufs, const struct iovec *iov, int iovcnt)
{
    int n = iovcnt < MPLI_SOCK_MAX_WSABUF ? iovcnt : MPLI_SOCK_MAX_WSABUF;
    for (int i = 0; i < n; i++) {
        bufs[i].buf = iov[i].iov_base;
        bufs[i].len = iov[i].iov_len > INT_MAX ? INT_MAX : (ULONG) iov[i].iov_len;
        if (iov[i].iov_len > INT_MAX)
            return i + 1;
    }
    return n;
}

ssize_t MPL_large_writev(int fd, const struct iovec * iov, int iovcnt)
{
    WSABUF bufs[MPLI_SOCK_MAX_WSABUF];
    DWORD nb = 0;
    int n = fill_wsabuf(bufs, iov, iovcnt);
    if (WSASend((SOCKET) fd, bufs, n, &nb, 0, NULL, NULL) == SOCKET_ERROR)
        return MPLI_sock_set_errno();
    return nb;
}

ssize_t MPL_large_readv(int fd, const struct iovec * iov, int iovcnt)
{
    WSABUF bufs[MPLI_SOCK_MAX_WSABUF];
    DWORD nb = 0, flags = 0;
    int n = fill_wsabuf(bufs, iov, iovcnt);
    if (WSARecv((SOCKET) fd, bufs, n, &nb, &flags, NULL, NULL) == SOCKET_ERROR)
        return MPLI_sock_set_errno();
    return nb;
}

#else /* !_WIN32 */

int MPL_sock_init(void)
{
    return 0;
}

int MPL_sock_pair(int fds[2])
{
    return pipe(fds);
}

#endif /* _WIN32 */
