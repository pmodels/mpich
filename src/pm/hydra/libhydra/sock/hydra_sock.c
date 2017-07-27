/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2017 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "hydra_sock.h"
#include "hydra_err.h"

HYD_status HYD_sock_listen_on_port(int *listen_fd, uint16_t port)
{
    struct sockaddr_in sa;
    int one = 1;
    HYD_status status = HYD_SUCCESS;

    HYD_FUNC_ENTER();

    *listen_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (*listen_fd < 0)
        HYD_ERR_SETANDJUMP(status, HYD_ERR_SOCK, "cannot open socket (%s)\n", MPL_strerror(errno));

    if (setsockopt(*listen_fd, IPPROTO_TCP, TCP_NODELAY, &one, sizeof(int)) < 0)
        HYD_ERR_SETANDJUMP(status, HYD_ERR_SOCK, "cannot set TCP_NODELAY\n");

    /* The sockets standard does not guarantee that a successful
     * return here means that this is set. However, REUSEADDR not
     * being set is not a fatal error, so we ignore that
     * case. However, we do check for error cases, which means that
     * something bad has happened. */
    if (setsockopt(*listen_fd, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(int)) < 0)
        HYD_ERR_SETANDJUMP(status, HYD_ERR_SOCK, "cannot set SO_REUSEADDR\n");

    memset((void *) &sa, 0, sizeof(struct sockaddr_in));
    sa.sin_family = AF_INET;
    sa.sin_port = htons(port);
    sa.sin_addr.s_addr = INADDR_ANY;

    if (bind(*listen_fd, (struct sockaddr *) &sa, sizeof(struct sockaddr_in)) < 0) {
        if (errno == EADDRINUSE) {
            status = HYD_ERR_PORT_IN_USE;
            goto fn_exit;
        }
        else {
            HYD_ERR_SETANDJUMP(status, HYD_ERR_SOCK, "bind error on port %d (%s)\n", port,
                               MPL_strerror(errno));
        }
    }

    if (listen(*listen_fd, SOMAXCONN) < 0) {
        HYD_ERR_SETANDJUMP(status, HYD_ERR_SOCK, "listen error (%s)\n", MPL_strerror(errno));
    }

  fn_exit:
    HYD_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

HYD_status HYD_sock_listen_on_any_port(int *listen_fd, uint16_t * port)
{
    struct sockaddr_in sa;
    socklen_t sinlen = sizeof(struct sockaddr_in);
    HYD_status status = HYD_SUCCESS;

    HYD_FUNC_ENTER();

    status = HYD_sock_listen_on_port(listen_fd, 0);
    HYD_ERR_POP(status, "error listening on port 0\n");

    /* figure out which port we actually got */
    if (getsockname(*listen_fd, (struct sockaddr *) &sa, &sinlen) < 0) {
        HYD_ERR_SETANDJUMP(status, HYD_ERR_SOCK, "getsockname error (%s)\n", MPL_strerror(errno));
    }
    *port = ntohs(sa.sin_port);

  fn_exit:
    HYD_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

HYD_status HYD_sock_listen_on_port_range(int *listen_fd, const char *port_range, uint16_t * port)
{
    uint16_t low_port, high_port;
    char *port_str;
    uint16_t i;
    HYD_status status = HYD_SUCCESS;

    HYD_FUNC_ENTER();

    /* strtok does not modify the first parameter, but uses "char*"
     * instead of "const char*".  We do an unsafe typecast here to
     * work around this issue with strtok. */
    port_str = strtok((char *) port_range, ":");
    if (port_str == NULL)
        HYD_ERR_SETANDJUMP(status, HYD_ERR_INTERNAL, "error parsing port range\n");
    low_port = atoi(port_str);

    port_str = strtok(NULL, ":");
    if (port_str == NULL)
        HYD_ERR_SETANDJUMP(status, HYD_ERR_INTERNAL, "error parsing port range\n");
    high_port = atoi(port_str);

    if (high_port < low_port)
        HYD_ERR_SETANDJUMP(status, HYD_ERR_INTERNAL, "high port < low port\n");

    for (i = low_port; i <= high_port; i++) {
        status = HYD_sock_listen_on_port(listen_fd, i);
        if (status == HYD_ERR_PORT_IN_USE)
            continue;
        HYD_ERR_POP(status, "error trying to listen on port %d\n", i);

        *port = i;
        break;  /* found a port, break out */
    }

    if (i > high_port)
        HYD_ERR_SETANDJUMP(status, HYD_ERR_SOCK, "no port to bind\n");

  fn_exit:
    HYD_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

static void insert_delay(unsigned long delay)
{
    struct timeval start, end;

    if (delay == 0)
        return;

    gettimeofday(&start, NULL);
    while (1) {
        gettimeofday(&end, NULL);
        if ((1000000.0 * (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec)) > delay)
            break;
    }
}

HYD_status HYD_sock_connect(const char *host, uint16_t port, int *fd, int retries,
                            unsigned long delay)
{
    struct hostent *ht;
    struct sockaddr_in sa;
    int one = 1, ret, retry_count;
    HYD_status status = HYD_SUCCESS;

    HYD_FUNC_ENTER();

    memset((char *) &sa, 0, sizeof(struct sockaddr_in));
    sa.sin_family = AF_INET;
    sa.sin_port = htons(port);

    /* Get the remote host's IP address. Note that this is not
     * thread-safe. Since we don't use threads right now, we don't
     * worry about locking it. */
    ht = gethostbyname(host);
    if (ht == NULL)
        HYD_ERR_SETANDJUMP(status, HYD_ERR_INVALID_PARAM,
                           "unable to get host address for %s (%s)\n", host, HYD_herror(h_errno));
    memcpy(&sa.sin_addr, ht->h_addr_list[0], ht->h_length);

    /* Create a socket and set the required options */
    *fd = socket(AF_INET, SOCK_STREAM, 0);
    if (*fd < 0)
        HYD_ERR_SETANDJUMP(status, HYD_ERR_SOCK, "cannot open socket (%s)\n", MPL_strerror(errno));

    /* Not being able to connect is not an error in all cases. So we
     * return an error, but only print a warning message. The upper
     * layer can decide what to do with the return status. */
    retry_count = 0;
    do {
        ret = connect(*fd, (struct sockaddr *) &sa, sizeof(struct sockaddr_in));
        if (ret < 0 && (errno == ECONNREFUSED || errno == ETIMEDOUT)) {
            /* connection error; increase retry count and delay */
            retry_count++;
            HYD_ERR_PRINT("Retrying connection, retry_count=%d, retries=%d\n", retry_count,
                          retries);
            if (retry_count > retries)
                break;
            insert_delay(delay);
        }
        else
            break;
    } while (1);

    if (ret < 0) {
        char localhost[HYD_MAX_HOSTNAME_LEN] = { 0 };

        if (gethostname(localhost, HYD_MAX_HOSTNAME_LEN) < 0)
            HYD_ERR_SETANDJUMP(status, HYD_ERR_SOCK, "unable to get local hostname\n");

        HYD_ERR_SETANDJUMP(status, HYD_ERR_SOCK, "unable to connect from \"%s\" to \"%s\" (%s)\n",
                           localhost, host, MPL_strerror(errno));
    }

    /* Disable nagle */
    if (setsockopt(*fd, IPPROTO_TCP, TCP_NODELAY, &one, sizeof(int)) < 0)
        HYD_ERR_SETANDJUMP(status, HYD_ERR_SOCK, "cannot set TCP_NODELAY\n");

  fn_exit:
    HYD_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}


HYD_status HYD_sock_accept(int listen_fd, int *fd)
{
    int one = 1;
    HYD_status status = HYD_SUCCESS;

    HYD_FUNC_ENTER();

    *fd = accept(listen_fd, 0, 0);
    if (*fd < 0)
        HYD_ERR_SETANDJUMP(status, HYD_ERR_SOCK, "accept error (%s)\n", MPL_strerror(errno));

    /* Disable nagle */
    if (setsockopt(*fd, IPPROTO_TCP, TCP_NODELAY, &one, sizeof(int)) < 0)
        HYD_ERR_SETANDJUMP(status, HYD_ERR_SOCK, "cannot set TCP_NODELAY\n");

  fn_exit:
    HYD_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

HYD_status HYD_sock_read(int fd, void *buf, int maxlen, int *recvd, int *closed,
                         enum HYD_sock_comm_flag flag)
{
    int tmp;
    HYD_status status = HYD_SUCCESS;

    HYD_FUNC_ENTER();

    HYD_ASSERT(maxlen, status);

    *recvd = 0;
    *closed = 0;
    while (1) {
        do {
            tmp = read(fd, (char *) buf + *recvd, maxlen - *recvd);
            if (tmp < 0) {
                if (errno == ECONNRESET || fd == STDIN_FILENO) {
                    /* If the remote end closed the socket or if we
                     * get an EINTR on stdin, set the socket to be
                     * closed and jump out */
                    *closed = 1;
                    status = HYD_SUCCESS;
                    goto fn_exit;
                }
            }
        } while (tmp < 0 && errno == EINTR);

        if (tmp < 0) {
            HYD_ERR_SETANDJUMP(status, HYD_ERR_SOCK, "read error (%s)\n", MPL_strerror(errno));
        }
        else if (tmp == 0) {
            *closed = 1;
            goto fn_exit;
        }
        else {
            *recvd += tmp;
        }

        if (flag == HYD_SOCK_COMM_TYPE__NONBLOCKING || *recvd == maxlen)
            break;
    }

  fn_exit:
    HYD_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

HYD_status HYD_sock_write(int fd, const void *buf, int maxlen, int *sent, int *closed,
                          enum HYD_sock_comm_flag flag)
{
    int tmp;
    HYD_status status = HYD_SUCCESS;

    HYD_FUNC_ENTER();

    HYD_ASSERT(maxlen, status);

    *sent = 0;
    *closed = 0;
    while (1) {
        tmp = write(fd, (char *) buf + *sent, maxlen - *sent);
        if (tmp <= 0) {
            if (errno == EAGAIN) {
                if (flag == HYD_SOCK_COMM_TYPE__NONBLOCKING)
                    goto fn_exit;
                else
                    continue;
            }
            else if (errno == ECONNRESET) {
                *closed = 1;
                goto fn_exit;
            }
            HYD_ERR_SETANDJUMP(status, HYD_ERR_SOCK, "write error (%s)\n", MPL_strerror(errno));
        }
        else {
            *sent += tmp;
        }

        if (flag == HYD_SOCK_COMM_TYPE__NONBLOCKING || *sent == maxlen)
            break;
    }

  fn_exit:
    HYD_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

HYD_status HYD_sock_cloexec(int fd)
{
    int flags;
    HYD_status status = HYD_SUCCESS;

#if defined HAVE_FCNTL
    flags = fcntl(fd, F_GETFD, 0);
    if (flags < 0)
        HYD_ERR_POP(status, "unable to get fd flags\n");
    flags |= FD_CLOEXEC;
    if (fcntl(fd, F_SETFD, flags) < 0)
        HYD_ERR_POP(status, "unable to set fd flags\n");
#endif /* HAVE_FCNTL */

  fn_exit:
    return status;

  fn_fail:
    goto fn_exit;
}
