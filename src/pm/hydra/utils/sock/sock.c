/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "hydra_utils.h"

static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

HYD_Status HYDU_sock_listen(int *listen_fd, char *port_range, uint16_t * port)
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
        /* If port range is set, we always pick from there */
        *port = 0;

        port_str = strtok(port_range, ":");
        if (port_str == NULL)
            HYDU_ERR_SETANDJUMP(status, HYD_INTERNAL_ERROR, "error parsing port range\n");
        low_port = atoi(port_str);

        port_str = strtok(NULL, ":");
        if (port_str == NULL)
            HYDU_ERR_SETANDJUMP(status, HYD_INTERNAL_ERROR, "error parsing port range\n");
        high_port = atoi(port_str);

        if (high_port < low_port)
            HYDU_ERR_SETANDJUMP(status, HYD_INTERNAL_ERROR, "high port < low port\n");
    }
    else {
        /* If port range is NULL, if a port is already provided, we
         * pick that. Otherwise, we search for an available port. */
        low_port = *port;
        high_port = *port;
    }

    *listen_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (*listen_fd < 0)
        HYDU_ERR_SETANDJUMP1(status, HYD_SOCK_ERROR, "cannot open socket (%s)\n",
                             HYDU_strerror(errno));

    if (setsockopt(*listen_fd, IPPROTO_TCP, TCP_NODELAY, &one, sizeof(int)) < 0)
        HYDU_ERR_SETANDJUMP(status, HYD_SOCK_ERROR, "cannot set TCP_NODELAY\n");

    for (i = low_port; i <= high_port; i++) {
        memset((void *) &sa, 0, sizeof(sa));
        sa.sin_family = AF_INET;
        sa.sin_port = htons(i);
        sa.sin_addr.s_addr = INADDR_ANY;

        /* The sockets standard does not guarantee that a successful
         * return here means that this is set. However, REUSEADDR not
         * being set is not a fatal error, so we ignore that
         * case. However, we do check for error cases, which means
         * that something bad has happened. */
        if (setsockopt(*listen_fd, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one)) < 0)
            HYDU_ERR_SETANDJUMP(status, HYD_SOCK_ERROR, "cannot set SO_REUSEADDR\n");

        if (bind(*listen_fd, (struct sockaddr *) &sa, sizeof(sa)) < 0) {
            close(*listen_fd);
            /* If the address is in use, we should try the next
             * port. Otherwise, it's an error. */
            if (errno != EADDRINUSE)
                HYDU_ERR_SETANDJUMP1(status, HYD_SOCK_ERROR, "bind error (%s)\n",
                                     HYDU_strerror(errno));
        }
        else    /* We got a port */
            break;
    }

    *port = i;
    if (*port > high_port)
        HYDU_ERR_SETANDJUMP(status, HYD_SOCK_ERROR, "no port to bind\n");

    if (listen(*listen_fd, -1) < 0)
        HYDU_ERR_SETANDJUMP1(status, HYD_SOCK_ERROR, "listen error (%s)\n",
                             HYDU_strerror(errno));

    /* We asked for any port, so we need to find out which port we
     * actually got. */
    if (*port == 0) {
        socklen_t sinlen = sizeof(sa);

        if (getsockname(*listen_fd, (struct sockaddr *) &sa, &sinlen) < 0)
            HYDU_ERR_SETANDJUMP1(status, HYD_SOCK_ERROR, "getsockname error (%s)\n",
                                 HYDU_strerror(errno));
        *port = ntohs(sa.sin_port);
    }

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}


HYD_Status HYDU_sock_connect(const char *host, uint16_t port, int *fd)
{
    struct hostent *ht;
    struct sockaddr_in sa;
    HYD_Status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    memset((char *) &sa, 0, sizeof(struct sockaddr_in));
    sa.sin_family = AF_INET;
    sa.sin_port = htons(port);

    /* Get the remote host's IP address */
    pthread_mutex_lock(&mutex);
    ht = gethostbyname(host);
    pthread_mutex_unlock(&mutex);
    if (ht == NULL)
        HYDU_ERR_SETANDJUMP1(status, HYD_INVALID_PARAM,
                             "unable to get host address (%s)\n", HYDU_strerror(errno));
    memcpy(&sa.sin_addr, ht->h_addr_list[0], ht->h_length);

    /* Create a socket and set the required options */
    *fd = socket(AF_INET, SOCK_STREAM, 0);
    if (*fd < 0)
        HYDU_ERR_SETANDJUMP1(status, HYD_SOCK_ERROR, "cannot open socket (%s)\n",
                             HYDU_strerror(errno));

    /* Not being able to connect is not an error in all cases. So we
     * return an error, but only print a warning message. The upper
     * layer can decide what to do with the return status. */
    if (connect(*fd, (struct sockaddr *) &sa, sizeof(sa)) < 0) {
        HYDU_Error_printf("connect error (%s)\n", HYDU_strerror(errno));
        status = HYD_SOCK_ERROR;
        goto fn_fail;
    }

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}


HYD_Status HYDU_sock_accept(int listen_fd, int *fd)
{
    HYD_Status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    *fd = accept(listen_fd, 0, 0);
    if (*fd < 0)
        HYDU_ERR_SETANDJUMP1(status, HYD_SOCK_ERROR, "accept error (%s)\n",
                             HYDU_strerror(errno));

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}


/* HYD_Sock_readline: Return the next newline-terminated string of
 * maximum length maxlen.  This is a buffered version, and reads from
 * fd as necessary. */
HYD_Status HYDU_sock_readline(int fd, char *buf, int maxlen, int *linelen)
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
            HYDU_ERR_SETANDJUMP1(status, HYD_SOCK_ERROR, "read error (%s)\n",
                                 HYDU_strerror(errno));
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


HYD_Status HYDU_sock_writeline(int fd, const char *buf, int maxsize)
{
    int n;
    HYD_Status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    if (buf[maxsize - 1] != '\n')
        HYDU_ERR_SETANDJUMP(status, HYD_INTERNAL_ERROR, "line does not end in newline\n");

    do {
        n = write(fd, buf, maxsize);
    } while (n < 0 && errno == EINTR);

    if (n < maxsize)
        HYDU_ERR_SETANDJUMP1(status, HYD_SOCK_ERROR, "write error (%s)\n",
                             HYDU_strerror(errno));

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}


HYD_Status HYDU_sock_read(int fd, void *buf, int maxlen, int *count,
                          enum HYDU_sock_comm_flag flag)
{
    int tmp;
    HYD_Status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    *count = 0;
    while (1) {
        do {
            tmp = read(fd, (char *) buf + *count, maxlen - *count);
        } while (tmp < 0 && errno == EINTR);

        if (tmp < 0) {
            *count = tmp;
            break;
        }
        *count += tmp;

        if (flag != HYDU_SOCK_COMM_MSGWAIT || *count == maxlen || tmp == 0)
            break;
    };

    if (*count < 0)
        HYDU_ERR_SETANDJUMP1(status, HYD_SOCK_ERROR, "read errno (%s)\n",
                             HYDU_strerror(errno));

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}


HYD_Status HYDU_sock_write(int fd, const void *buf, int maxsize)
{
    int n;
    HYD_Status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    do {
        n = write(fd, buf, maxsize);
    } while (n < 0 && errno == EINTR);

    if (n < maxsize)
        HYDU_ERR_SETANDJUMP1(status, HYD_SOCK_ERROR, "write error (%s)\n",
                             HYDU_strerror(errno));

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}


HYD_Status HYDU_sock_trywrite(int fd, const void *buf, int maxsize)
{
    int n;
    HYD_Status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    do {
        n = write(fd, buf, maxsize);
    } while (n < 0 && errno == EINTR);

    if (n < maxsize) {
        HYDU_Warn_printf("write error (%s)\n", HYDU_strerror(errno));
        status = HYD_SOCK_ERROR;
        goto fn_fail;
    }

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}


HYD_Status HYDU_sock_set_nonblock(int fd)
{
    int flags;
    HYD_Status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    flags = fcntl(fd, F_GETFL, 0);
    if (flags < 0)
        HYDU_ERR_SETANDJUMP1(status, HYD_SOCK_ERROR, "fcntl failed on %d\n", fd);
    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) < 0)
        HYDU_ERR_SETANDJUMP1(status, HYD_SOCK_ERROR, "fcntl failed on %d\n", fd);

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}


HYD_Status HYDU_sock_set_cloexec(int fd)
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


HYD_Status HYDU_sock_stdout_cb(int fd, HYD_Event_t events, int stdout_fd, int *closed)
{
    int count, written, ret;
    char buf[HYD_TMPBUF_SIZE];
    HYD_Status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    *closed = 0;

    if (events & HYD_STDIN)
        HYDU_ERR_SETANDJUMP(status, HYD_INTERNAL_ERROR, "stdout handler got stdin event\n");

    count = read(fd, buf, HYD_TMPBUF_SIZE);
    if (count < 0) {
        HYDU_ERR_SETANDJUMP2(status, HYD_SOCK_ERROR, "read error on %d (%s)\n",
                             fd, HYDU_strerror(errno));
    }
    else if (count == 0) {
        /* The connection has closed */
        *closed = 1;
        goto fn_exit;
    }

    written = 0;
    while (written != count) {
        ret = write(stdout_fd, buf + written, count - written);
        if (ret < 0 && errno != EAGAIN)
            HYDU_ERR_SETANDJUMP2(status, HYD_SOCK_ERROR, "write error on %d (%s)\n",
                                 stdout_fd, HYDU_strerror(errno));
        if (ret > 0)
            written += ret;
    }

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}


HYD_Status HYDU_sock_stdin_cb(int fd, HYD_Event_t events, int stdin_fd, char *buf,
                              int *buf_count, int *buf_offset, int *closed)
{
    int count;
    HYD_Status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    *closed = 0;

    if (events & HYD_STDOUT)
        HYDU_ERR_SETANDJUMP(status, HYD_INTERNAL_ERROR, "stdin handler got stdout event\n");

    while (1) {
        /* If we already have buffered data, send it out */
        if (*buf_count) {
            count = write(fd, buf + *buf_offset, *buf_count);
            if (count < 0)
                HYDU_ERR_SETANDJUMP2(status, HYD_SOCK_ERROR, "write error on %d (%s)\n",
                                     fd, HYDU_strerror(errno));
            *buf_offset += count;
            *buf_count -= count;
            break;
        }

        /* If we are still here, we need to refill our temporary buffer */
        count = read(stdin_fd, buf, HYD_TMPBUF_SIZE);
        if (count < 0) {
            if (errno == EINTR || errno == EAGAIN || errno == ENOTCONN) {
                /* This call was interrupted or there was no data to read; just break out. */
                *closed = 1;
                break;
            }

            HYDU_ERR_SETANDJUMP1(status, HYD_SOCK_ERROR, "read error on 0 (%s)\n",
                                 HYDU_strerror(errno));
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
