/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "hydra_sock.h"
#include "hydra_dbg.h"

HYD_Status HYDU_Sock_listen(int *listen_fd, char *port_range, uint16_t * port)
{
    struct sockaddr_in sa;
    int one = 1;
    uint16_t low_port, high_port;
    char *port_str;
    uint16_t i;
    HYD_Status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    low_port = 0;
    high_port = 0;
    if (port_range) {
        port_str = strtok(port_range, ":");
        if (port_str == NULL) {
            HYDU_Error_printf("error parsing port range string\n");
            status = HYD_INTERNAL_ERROR;
            goto fn_fail;
        }
        low_port = atoi(port_str);

        port_str = strtok(NULL, ":");
        if (port_str == NULL) {
            HYDU_Error_printf("error parsing port range string\n");
            status = HYD_INTERNAL_ERROR;
            goto fn_fail;
        }
        high_port = atoi(port_str);

        if (high_port < low_port) {
            HYDU_Error_printf("high port is smaller than low port\n");
            status = HYD_INTERNAL_ERROR;
            goto fn_fail;
        }
    }

    *listen_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (*listen_fd < 0) {
        HYDU_Error_printf("unable to create a stream socket (errno: %d)\n", errno);
        status = HYD_SOCK_ERROR;
        goto fn_fail;
    }

    if (setsockopt(*listen_fd, IPPROTO_TCP, TCP_NODELAY, &one, sizeof(int))
        < 0) {
        HYDU_Error_printf("unable to set the TCP_NODELAY socket option (errno: %d)\n", errno);
        status = HYD_SOCK_ERROR;
        goto fn_fail;
    }

    for (i = low_port; i <= high_port; i++) {
        memset((void *) &sa, 0, sizeof(sa));
        sa.sin_family = AF_INET;
        sa.sin_port = htons(i);
        sa.sin_addr.s_addr = INADDR_ANY;

        if (bind(*listen_fd, (struct sockaddr *) &sa, sizeof(sa)) < 0) {
            close(*listen_fd);
            /* If the address is in use, we should try the next
             * port. Otherwise, it's an error. */
            if (errno != EADDRINUSE) {
                status = HYD_SOCK_ERROR;
                HYDU_Error_printf
                    ("unable to bind listen socket %d (errno: %d)\n", *listen_fd, errno);
                goto fn_fail;
            }
        }
        else    /* We got a port */
            break;
    }
    *port = i;

    if (listen(*listen_fd, -1) < 0) {
        HYDU_Error_printf("listen error (errno: %d)\n", errno);
        status = HYD_SOCK_ERROR;
        goto fn_fail;
    }

    status = HYDU_Sock_set_cloexec(*listen_fd);
    if (status != HYD_SUCCESS) {
        HYDU_Error_printf("unable to set fd %d to close on exec\n", *listen_fd);
        goto fn_fail;
    }

    /* We asked for any port, so we need to find out which port we
     * actually got. */
    if (*port == 0) {
        socklen_t sinlen = sizeof(sa);

        if (getsockname(*listen_fd, (struct sockaddr *) &sa, &sinlen) < 0) {
            HYDU_Error_printf("getsockname error (errno: %d)\n", errno);
            status = HYD_SOCK_ERROR;
            goto fn_fail;
        }
        *port = ntohs(sa.sin_port);
    }

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}


HYD_Status HYDU_Sock_connect(const char *host, uint16_t port, int *fd)
{
    struct sockaddr_in sa;
    struct hostent *ht;
    int one = 1;
    HYD_Status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    memset((char *) &sa, 0, sizeof(sa));
    sa.sin_family = AF_INET;
    sa.sin_port = htons(port);

    /* Get the remote host's IP address */
    ht = gethostbyname(host);
    if (ht == NULL) {
        HYDU_Error_printf("unable to get host address: %s (errno: %d)\n", host, errno);
        status = HYD_INVALID_PARAM;
        goto fn_fail;
    }
    memcpy(&sa.sin_addr, ht->h_addr_list[0], ht->h_length);

    /* Create a socket and set the required options */
    *fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (*fd < 0) {
        HYDU_Error_printf("unable to create a stream socket (errno: %d)\n", errno);
        status = HYD_SOCK_ERROR;
        goto fn_fail;
    }

    if (setsockopt(*fd, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(int)) < 0) {
        HYDU_Error_printf("unable to set the SO_REUSEADDR socket option (errno: %d)\n", errno);
        status = HYD_SOCK_ERROR;
        goto fn_fail;
    }

    status = HYDU_Sock_set_nonblock(*fd);
    if (status != HYD_SUCCESS) {
        HYDU_Error_printf("unable to set fd %d as non-blocking\n", *fd);
        goto fn_fail;
    }

    status = HYDU_Sock_set_cloexec(*fd);
    if (status != HYD_SUCCESS) {
        HYDU_Error_printf("unable to set fd %d to close on exec\n", *fd);
        goto fn_fail;
    }

    if (connect(*fd, (struct sockaddr *) &sa, sizeof(sa)) < 0) {
        HYDU_Error_printf("connect error (errno: %d)\n", errno);
        status = HYD_SOCK_ERROR;
        goto fn_fail;
    }

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}


HYD_Status HYDU_Sock_accept(int listen_fd, int *fd)
{
    HYD_Status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    *fd = accept(listen_fd, 0, 0);
    if (*fd < 0) {
        HYDU_Error_printf("accept error on socket %d (errno: %d)\n", listen_fd, errno);
        status = HYD_SOCK_ERROR;
        goto fn_fail;
    }

    status = HYDU_Sock_set_nonblock(*fd);
    if (status != HYD_SUCCESS) {
        HYDU_Error_printf("unable to set fd %d as non-blocking\n", *fd);
        goto fn_fail;
    }

    status = HYDU_Sock_set_cloexec(*fd);
    if (status != HYD_SUCCESS) {
        HYDU_Error_printf("unable to set fd %d to close on exec\n", *fd);
        goto fn_fail;
    }

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}


/*
 * HYD_Sock_readline: Return the next newline-terminated string of
 * maximum length maxlen.  This is a buffered version, and reads from
 * fd as necessary.
 */
HYD_Status HYDU_Sock_readline(int fd, char *buf, int maxlen, int *linelen)
{
    int n;
    HYD_Status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    *linelen = 0;
    while (1) {
        n = read(fd, buf + *linelen, maxlen - *linelen - 1);
        if (n == 0) {   /* No more data to read */
            break;
        }
        else if (n < 0) {
            if (errno == EINTR)
                continue;

            HYDU_Error_printf("error reading from socket %d (errno: %d)\n", fd, errno);
            status = HYD_SOCK_ERROR;
            goto fn_fail;
        }

        *linelen += n;
        break;
    }

    /* Done reading; pad the last byte with a NULL */
    buf[*linelen - 1] = 0;

    /* Run through the read data and see if there are any new lines in
     * there */
    for (n = 0; n < *linelen; n++) {
        if (*(buf + n) == '\n') {
            *(buf + n) = 0;
            *linelen = n;
            break;
        }
    }

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}


/*
 * HYD_Sock_read: perform a blocking read on a non-blocking socket.
 */
HYD_Status HYDU_Sock_read(int fd, char *buf, int maxlen, int *count)
{
    HYD_Status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    do {
        *count = read(fd, buf, maxlen);
    } while (*count < 0 && errno == EINTR);

    if (*count < 0) {
        HYDU_Error_printf("error reading from socket %d (errno: %d)\n", fd, errno);
        status = HYD_SOCK_ERROR;
        goto fn_fail;
    }

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}


HYD_Status HYDU_Sock_writeline(int fd, char *buf, int maxsize)
{
    int n;
    HYD_Status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    if (buf[maxsize - 1] != '\n') {
        HYDU_Error_printf("write line does not end in newline\n");
        status = HYD_INTERNAL_ERROR;
        goto fn_fail;
    }

    do {
        n = write(fd, buf, maxsize);
    } while (n < 0 && errno == EINTR);

    if (n < maxsize) {
        HYDU_Error_printf("error writing to socket %d (errno: %d)\n", fd, errno);
        status = HYD_SOCK_ERROR;
        goto fn_fail;
    }

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}


HYD_Status HYDU_Sock_write(int fd, char *buf, int maxsize)
{
    int n;
    HYD_Status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    do {
        n = write(fd, buf, maxsize);
    } while (n < 0 && errno == EINTR);

    if (n < maxsize) {
        HYDU_Error_printf("error writing to socket %d (errno: %d)\n", fd, errno);
        status = HYD_SOCK_ERROR;
        goto fn_fail;
    }

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}


HYD_Status HYDU_Sock_set_nonblock(int fd)
{
    int flags;
    HYD_Status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    flags = fcntl(fd, F_GETFL, 0);
    if (flags < 0) {
        status = HYD_SOCK_ERROR;
        HYDU_Error_printf("unable to do fcntl on fd %d\n", fd);
        goto fn_fail;
    }
    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) < 0) {
        status = HYD_SOCK_ERROR;
        HYDU_Error_printf("unable to do fcntl on fd %d\n", fd);
        goto fn_fail;
    }

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}


HYD_Status HYDU_Sock_set_cloexec(int fd)
{
    int flags;
    HYD_Status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    flags = fcntl(fd, F_GETFL, 0);
    if (flags >= 0) {
        /* Make sure that exec'd processes don't get this fd */
        flags |= FD_CLOEXEC;
        fcntl(fd, F_SETFD, flags);
    }

    HYDU_FUNC_EXIT();
    return status;
}


HYD_Status HYDU_Sock_stdout_cb(int fd, HYD_Event_t events, int stdout_fd, int *closed)
{
    int count;
    char buf[HYD_TMPBUF_SIZE];
    HYD_Status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    *closed = 0;

    if (events & HYD_STDIN) {
        HYDU_Error_printf("stdout handler got an stdin event: %d\n", events);
        status = HYD_INTERNAL_ERROR;
        goto fn_fail;
    }

    count = read(fd, buf, HYD_TMPBUF_SIZE);
    if (count < 0) {
        HYDU_Error_printf("socket read error on fd: %d (errno: %d)\n", fd, errno);
        status = HYD_SOCK_ERROR;
        goto fn_fail;
    }
    else if (count == 0) {
        /* The connection has closed */
        *closed = 1;
        goto fn_exit;
    }

    count = write(stdout_fd, buf, count);
    if (count < 0) {
        HYDU_Error_printf("socket write error on fd: %d (errno: %d)\n", fd, errno);
        status = HYD_SOCK_ERROR;
        goto fn_fail;
    }

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}


HYD_Status HYDU_Sock_stdin_cb(int fd, HYD_Event_t events, char *buf, int *buf_count,
                              int *buf_offset, int *closed)
{
    int count;
    HYD_Status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    *closed = 0;

    if (events & HYD_STDOUT) {
        HYDU_Error_printf("stdin handler got an stdout event\n");
        status = HYD_INTERNAL_ERROR;
        goto fn_fail;
    }

    while (1) {
        /* If we already have buffered data, send it out */
        if (*buf_count) {
            count = write(fd, buf + *buf_offset, *buf_count);
            if (count < 0) {
                /* We can't get an EAGAIN as we just got out of poll */
                HYDU_Error_printf("socket write error on fd: %d (errno: %d)\n", fd, errno);
                status = HYD_SOCK_ERROR;
                goto fn_fail;
            }
            *buf_offset += count;
            *buf_count -= count;
            break;
        }

        /* If we are still here, we need to refill our temporary buffer */
        count = read(0, buf, HYD_TMPBUF_SIZE);
        if (count < 0) {
            if (errno == EINTR || errno == EAGAIN) {
                /* This call was interrupted or there was no data to read; just break out. */
                break;
            }

            HYDU_Error_printf("socket read error on fd: %d (errno: %d)\n", fd, errno);
            status = HYD_SOCK_ERROR;
            goto fn_fail;
        }
        else if (count == 0) {
            /* The connection has closed */
            *closed = 1;
            break;
        }
        *buf_count += count;
    }

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}
