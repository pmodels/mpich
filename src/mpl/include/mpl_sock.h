/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef MPL_SOCK_H_INCLUDED
#define MPL_SOCK_H_INCLUDED

#include "mplconfig.h"

#ifdef _WIN32
/* winsock2.h must come before windows.h */
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <sys/types.h>
#include <errno.h>
#include <limits.h>
#ifndef SHUT_RDWR
#define SHUT_RD   SD_RECEIVE
#define SHUT_WR   SD_SEND
#define SHUT_RDWR SD_BOTH
#endif
#else
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <netinet/tcp.h>
#include <netdb.h>
#include <limits.h>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>
#endif

#include "mpl_iov.h"

/* *INDENT-ON* */
#if defined(__cplusplus)
extern "C" {
#endif
/* *INDENT-OFF* */

ssize_t MPL_large_writev(int fd, const struct iovec *iov, int iovcnt);
ssize_t MPL_large_readv(int fd, const struct iovec *iov, int iovcnt);
int MPL_host_is_local(const char *host);

/* Portable socket primitives.
 *
 * Sockets are passed around as int file descriptors.  On Windows a SOCKET
 * is a kernel handle, which is guaranteed to fit in 32 bits.  The
 * functions below return -1 on failure and set errno, also on Windows
 * where Winsock itself only reports errors through WSAGetLastError(), so
 * callers can keep testing errno against EAGAIN, EINTR, EINPROGRESS etc.
 */
int MPL_sock_init(void);
int MPL_sock_pair(int fds[2]);

#ifdef _WIN32
int MPLI_sock_set_errno(void);

ssize_t MPL_sock_read(int fd, void *buf, size_t len);
ssize_t MPL_sock_write(int fd, const void *buf, size_t len);
int MPL_sock_close(int fd);
int MPL_sock_set_nonblock(int fd, int nonblock);
int MPL_sock_set_cloexec(int fd);
int MPL_sock_poll(struct pollfd *fds, unsigned long nfds, int timeout);
int MPL_sock_accept(int fd, struct sockaddr *addr, socklen_t * addrlen);
int MPL_sock_get_error(int fd, int *err);
#else
static inline ssize_t MPL_sock_read(int fd, void *buf, size_t len)
{
    return read(fd, buf, len);
}

static inline ssize_t MPL_sock_write(int fd, const void *buf, size_t len)
{
    return write(fd, buf, len);
}

static inline int MPL_sock_close(int fd)
{
    return close(fd);
}

static inline int MPL_sock_set_nonblock(int fd, int nonblock)
{
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1)
        return -1;
    flags = nonblock ? (flags | O_NONBLOCK) : (flags & ~O_NONBLOCK);
    return fcntl(fd, F_SETFL, flags);
}

static inline int MPL_sock_set_cloexec(int fd)
{
    return fcntl(fd, F_SETFD, FD_CLOEXEC);
}

static inline int MPL_sock_poll(struct pollfd *fds, unsigned long nfds, int timeout)
{
    return poll(fds, (nfds_t) nfds, timeout);
}

static inline int MPL_sock_accept(int fd, struct sockaddr *addr, socklen_t * addrlen)
{
    return accept(fd, addr, addrlen);
}

/* pending error of a socket (SO_ERROR) as an errno value */
static inline int MPL_sock_get_error(int fd, int *err)
{
    socklen_t len = sizeof(*err);
    return getsockopt(fd, SOL_SOCKET, SO_ERROR, err, &len);
}
#endif

/* *INDENT-ON* */
#if defined(__cplusplus)
}
#endif
/* *INDENT-OFF* */

#endif /* MPL_SOCK_H_INCLUDED */
