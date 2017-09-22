/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "hydra.h"
#include "demux.h"

struct fwd_hash {
    int in;
    int out;

    char buf[HYD_TMPBUF_SIZE];
    int buf_offset;
    int buf_count;

    struct fwd_hash *next;
};

HYD_status HYDU_sock_listen(int *listen_fd, char *port_range, uint16_t * port)
{
    struct sockaddr_in sa;
    int one = 1;
    uint16_t low_port, high_port;
    char *port_str;
    uint16_t i;
    HYD_status status = HYD_SUCCESS;

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

 setup_socket:
    *listen_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (*listen_fd < 0)
        HYDU_ERR_SETANDJUMP(status, HYD_SOCK_ERROR, "cannot open socket (%s)\n",
                            MPL_strerror(errno));

    if (setsockopt(*listen_fd, IPPROTO_TCP, TCP_NODELAY, &one, sizeof(int)) < 0)
        HYDU_ERR_SETANDJUMP(status, HYD_SOCK_ERROR, "cannot set TCP_NODELAY\n");

    /* The sockets standard does not guarantee that a successful
     * return here means that this is set. However, REUSEADDR not
     * being set is not a fatal error, so we ignore that
     * case. However, we do check for error cases, which means that
     * something bad has happened. */
    if (setsockopt(*listen_fd, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(int)) < 0)
        HYDU_ERR_SETANDJUMP(status, HYD_SOCK_ERROR, "cannot set SO_REUSEADDR\n");

    for (i = low_port; i <= high_port; i++) {
        memset((void *) &sa, 0, sizeof(struct sockaddr_in));
        sa.sin_family = AF_INET;
        sa.sin_port = htons(i);
        sa.sin_addr.s_addr = INADDR_ANY;

        if (bind(*listen_fd, (struct sockaddr *) &sa, sizeof(struct sockaddr_in)) < 0) {
            /* If the address is in use, we should try the next
             * port. Otherwise, it's an error. */
            if (errno != EADDRINUSE)
                HYDU_ERR_SETANDJUMP(status, HYD_SOCK_ERROR, "bind error (%s)\n",
                                    MPL_strerror(errno));
        }
        else    /* We got a port */
            break;
    }

    *port = i;
    if (*port > high_port)
        HYDU_ERR_SETANDJUMP(status, HYD_SOCK_ERROR, "no port to bind\n");

    if (listen(*listen_fd, SOMAXCONN) < 0) {
        if (errno == EADDRINUSE) {
            /* We need to close the socket and rebind to a new port */
            close(*listen_fd);
            goto setup_socket;
        } else {
            HYDU_ERR_SETANDJUMP(status, HYD_SOCK_ERROR, "listen error (%s)\n", MPL_strerror(errno));
        }
    }

    /* We asked for any port, so we need to find out which port we
     * actually got. */
    if (*port == 0) {
        socklen_t sinlen = sizeof(struct sockaddr_in);

        if (getsockname(*listen_fd, (struct sockaddr *) &sa, &sinlen) < 0)
            HYDU_ERR_SETANDJUMP(status, HYD_SOCK_ERROR, "getsockname error (%s)\n",
                                MPL_strerror(errno));
        *port = ntohs(sa.sin_port);
    }

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}


HYD_status HYDU_sock_connect(const char *host, uint16_t port, int *fd, int retries,
                             unsigned long delay)
{
    struct hostent *ht;
    struct sockaddr_in sa;
    int one = 1, ret, retry_count;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    memset((char *) &sa, 0, sizeof(struct sockaddr_in));
    sa.sin_family = AF_INET;
    sa.sin_port = htons(port);

    /* Get the remote host's IP address. Note that this is not
     * thread-safe. Since we don't use threads right now, we don't
     * worry about locking it. */
    ht = gethostbyname(host);
    if (ht == NULL)
        HYDU_ERR_SETANDJUMP(status, HYD_INVALID_PARAM,
                            "unable to get host address for %s (%s)\n", host, HYDU_herror(h_errno));
    memcpy(&sa.sin_addr, ht->h_addr_list[0], ht->h_length);

    /* Create a socket and set the required options */
    *fd = socket(AF_INET, SOCK_STREAM, 0);
    if (*fd < 0)
        HYDU_ERR_SETANDJUMP(status, HYD_SOCK_ERROR, "cannot open socket (%s)\n",
                            MPL_strerror(errno));

    /* Not being able to connect is not an error in all cases. So we
     * return an error, but only print a warning message. The upper
     * layer can decide what to do with the return status. */
    retry_count = 0;
    do {
        ret = connect(*fd, (struct sockaddr *) &sa, sizeof(struct sockaddr_in));
        if (ret < 0 && (errno == ECONNREFUSED || errno == ETIMEDOUT)) {
            /* connection error; increase retry count and delay */
            retry_count++;
            if (retry_count > retries)
                break;
            HYDU_delay(delay);
        }
        else
            break;
    } while (1);

    if (ret < 0) {
        char localhost[MAX_HOSTNAME_LEN] = { 0 };

        if (gethostname(localhost, MAX_HOSTNAME_LEN) < 0)
            HYDU_ERR_SETANDJUMP(status, HYD_SOCK_ERROR, "unable to get local hostname\n");

        HYDU_ERR_SETANDJUMP(status, HYD_SOCK_ERROR,
                            "unable to connect from \"%s\" to \"%s\" (%s)\n",
                            localhost, host, MPL_strerror(errno));
    }

    /* Disable nagle */
    if (setsockopt(*fd, IPPROTO_TCP, TCP_NODELAY, &one, sizeof(int)) < 0)
        HYDU_ERR_SETANDJUMP(status, HYD_SOCK_ERROR, "cannot set TCP_NODELAY\n");

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}


HYD_status HYDU_sock_accept(int listen_fd, int *fd)
{
    int one = 1;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    *fd = accept(listen_fd, 0, 0);
    if (*fd < 0)
        HYDU_ERR_SETANDJUMP(status, HYD_SOCK_ERROR, "accept error (%s)\n", MPL_strerror(errno));

    /* Disable nagle */
    if (setsockopt(*fd, IPPROTO_TCP, TCP_NODELAY, &one, sizeof(int)) < 0)
        HYDU_ERR_SETANDJUMP(status, HYD_SOCK_ERROR, "cannot set TCP_NODELAY\n");

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

HYD_status HYDU_sock_read(int fd, void *buf, int maxlen, int *recvd, int *closed,
                          enum HYDU_sock_comm_flag flag)
{
    int tmp;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    HYDU_ASSERT(maxlen, status);

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
            HYDU_ERR_SETANDJUMP(status, HYD_SOCK_ERROR, "read error (%s)\n", MPL_strerror(errno));
        }
        else if (tmp == 0) {
            *closed = 1;
            goto fn_exit;
        }
        else {
            *recvd += tmp;
        }

        if (flag == HYDU_SOCK_COMM_NONE || *recvd == maxlen)
            break;
    }

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

HYD_status HYDU_sock_write(int fd, const void *buf, int maxlen, int *sent, int *closed,
                           enum HYDU_sock_comm_flag flag)
{
    int tmp;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    HYDU_ASSERT(maxlen, status);

    *sent = 0;
    *closed = 0;
    while (1) {
        tmp = write(fd, (char *) buf + *sent, maxlen - *sent);
        if (tmp <= 0) {
            if (errno == EAGAIN) {
                if (flag == HYDU_SOCK_COMM_NONE)
                    goto fn_exit;
                else
                    continue;
            }
            else if (errno == ECONNRESET) {
                *closed = 1;
                goto fn_exit;
            }
            HYDU_ERR_SETANDJUMP(status, HYD_SOCK_ERROR, "write error (%s)\n", MPL_strerror(errno));
        }
        else {
            *sent += tmp;
        }

        if (flag == HYDU_SOCK_COMM_NONE || *sent == maxlen)
            break;
    }

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

HYD_status HYDU_sock_set_nonblock(int fd)
{
    int flags;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    if ((flags = fcntl(fd, F_GETFL, 0)) == -1)
        flags = 0;
#if defined O_NONBLOCK
    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) < 0)
        HYDU_ERR_SETANDJUMP(status, HYD_SOCK_ERROR, "fcntl failed on %d\n", fd);
#endif /* O_NONBLOCK */

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

static HYD_status alloc_fwd_hash(struct fwd_hash **fwd_hash, int in, int out)
{
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    HYDU_MALLOC_OR_JUMP(*fwd_hash, struct fwd_hash *, sizeof(struct fwd_hash), status);
    (*fwd_hash)->in = in;
    (*fwd_hash)->out = out;

    (*fwd_hash)->buf_offset = 0;
    (*fwd_hash)->buf_count = 0;

    (*fwd_hash)->next = NULL;

    status = HYDU_sock_set_nonblock(in);
    HYDU_ERR_POP(status, "unable to set out-socket to non-blocking\n");

    status = HYDU_sock_set_nonblock(out);
    HYDU_ERR_POP(status, "unable to set out-socket to non-blocking\n");

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

static struct fwd_hash *fwd_hash_list = NULL;

/* This function does not provide any flow control. We just read from
 * the incoming socket as much as we can and push out to the outgoing
 * socket as much as we can. This can result in the process calling it
 * polling continuously waiting for events, but that's a rare case for
 * stdio (which is what this function is meant to provide
 * functionality for). */
HYD_status HYDU_sock_forward_stdio(int in, int out, int *closed)
{
    struct fwd_hash *fwd_hash, *tmp;
    int count;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    /* find the fwd hash */
    for (tmp = fwd_hash_list; tmp; tmp = tmp->next)
        if (out == tmp->out)
            break;

    if (tmp == NULL) {  /* No hash found; create one */
        status = alloc_fwd_hash(&fwd_hash, in, out);
        HYDU_ERR_POP(status, "unable to allocate forward hash\n");
        if (fwd_hash_list == NULL)
            fwd_hash_list = fwd_hash;
        else {
            for (tmp = fwd_hash_list; tmp->next; tmp = tmp->next);
            tmp->next = fwd_hash;
        }
    }
    else {
        fwd_hash = tmp;
    }

    *closed = 0;
    if (fwd_hash->buf_count == 0) {
        /* there is no data in the buffer, read something into it */
        status = HYDU_sock_read(in, fwd_hash->buf, HYD_TMPBUF_SIZE, &count, closed,
                                HYDU_SOCK_COMM_NONE);
        HYDU_ERR_POP(status, "read error\n");

        if (!*closed) {
            fwd_hash->buf_offset = 0;
            fwd_hash->buf_count += count;

            /* We should never get a zero count, as the upper-layer
             * should have waited for an event from the demux engine
             * before calling us. */
            HYDU_ASSERT(count, status);
        }
    }

    if (fwd_hash->buf_count) {
        /* there is data in the buffer, send it out first */
        status =
            HYDU_sock_write(out, fwd_hash->buf + fwd_hash->buf_offset, fwd_hash->buf_count,
                            &count, closed, HYDU_SOCK_COMM_MSGWAIT);
        HYDU_ERR_POP(status, "write error\n");

        if (!*closed) {
            fwd_hash->buf_offset += count;
            fwd_hash->buf_count -= count;
        }
    }

    /* If the incoming socket is closed, make sure we forward out all
     * of the buffered data */
    while (*closed && fwd_hash->buf_count) {
        status =
            HYDU_sock_write(out, fwd_hash->buf + fwd_hash->buf_offset, fwd_hash->buf_count,
                            &count, closed, HYDU_SOCK_COMM_MSGWAIT);
        HYDU_ERR_POP(status, "write error\n");

        if (!*closed) {
            fwd_hash->buf_offset += count;
            fwd_hash->buf_count -= count;
        }
    }

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

void HYDU_sock_finalize(void)
{
    struct fwd_hash *tmp, *fwd_hash;

    for (fwd_hash = fwd_hash_list; fwd_hash;) {
        tmp = fwd_hash->next;
        MPL_free(fwd_hash);
        fwd_hash = tmp;
    }
}

HYD_status HYDU_sock_get_iface_ip(char *iface, char **ip)
{
    HYD_status status = HYD_SUCCESS;

#if defined(HAVE_GETIFADDRS)
    struct ifaddrs *ifaddr, *ifa;
    char buf[MAX_HOSTNAME_LEN];
    struct sockaddr_in *sa;

    /* Got the interface name; let's query for the IP address */
    if (getifaddrs(&ifaddr) == -1)
        HYDU_ERR_SETANDJUMP(status, HYD_INTERNAL_ERROR, "getifaddrs failed\n");

    for (ifa = ifaddr; ifa; ifa = ifa->ifa_next)
        if (!strcmp(ifa->ifa_name, iface) && (ifa->ifa_addr) &&
            (ifa->ifa_addr->sa_family == AF_INET))
            break;

    if (!ifa)
        HYDU_ERR_SETANDJUMP(status, HYD_INTERNAL_ERROR, "unable to find interface %s\n", iface);

    sa = (struct sockaddr_in *) ifa->ifa_addr;
#if defined HAVE_INET_NTOP
    (*ip) = MPL_strdup((char *)
                       inet_ntop(AF_INET, (const void *) &(sa->sin_addr), buf, MAX_HOSTNAME_LEN));
#else
    (*ip) = NULL;
#endif /* HAVE_INET_NTOP */
    if (!*ip)
        HYDU_ERR_SETANDJUMP(status, HYD_INTERNAL_ERROR,
                            "unable to find IP for interface %s\n", iface);

    freeifaddrs(ifaddr);
#else
    /* For now just disable interface selection when getifaddrs isn't
     * available, such as on AIX.  In the future we can implement in MPL
     * something along the lines of MPIDI_GetIPInterface from tcp_getip.c in
     * nemesis. */
    HYDU_ERR_SETANDJUMP(status, HYD_INTERNAL_ERROR,
                        "interface selection not supported on this platform\n");
#endif

  fn_exit:
    return status;

  fn_fail:
    goto fn_exit;
}

#if defined(HAVE_GETIFADDRS) && defined (HAVE_INET_NTOP)
HYD_status HYDU_sock_is_local(char *host, int *is_local)
{
    struct hostent *ht;
    char *host_ip = NULL, *lhost_ip = NULL;
    char lhost[MAX_HOSTNAME_LEN];
    struct sockaddr_in sa;
    struct ifaddrs *ifaddr, *ifa;
    char buf[MAX_HOSTNAME_LEN];
    HYD_status status = HYD_SUCCESS;

    *is_local = 0;

    /* Algorithm used:
     *
     * 1. Find the local host name
     *    - If "host" matches the local host name, return.
     * 2. Find the IP address associated with "host" and the IP the local host
     *    resolves to.
     *    - If these IPs match, return.
     * 3. Find all local network IP addresses
     *    - If the "host" IP address matches any of the local network IP
     *      addresses, return.
     */


    /* STEP 1: If "host" matches the local host name, return */
    if (gethostname(lhost, MAX_HOSTNAME_LEN) < 0) {
        /* We can't figure out what my localhost name is.  *sigh*.  We
         * could return an error here, but we will just punt it to the
         * upper layer saying that we don't know if it is local.  We
         * cannot try steps 2 and 3 either, since we don't have our
         * local hostname. */
        goto fn_exit;
    }
    else if (!strcmp(lhost, host)) {
        *is_local = 1;
        goto fn_exit;
    }
    else {
        /* we have our local hostname, but that does not match the
         * provided hostname.  Let's try to get our remote IP address
         * first.  If we can't get that, we can give up. */
        /* If we are unable to resolve the remote host name, it need
         * not be an error. It could mean that the user is using an
         * alias for the hostname (e.g., an ssh config alias) */
        if ((ht = gethostbyname(host)) == NULL)
            goto fn_exit;

        memset((char *) &sa, 0, sizeof(struct sockaddr_in));
        memcpy(&sa.sin_addr, ht->h_addr_list[0], ht->h_length);

        /* Find the IP address of the host */
        host_ip = MPL_strdup((char *) inet_ntop(AF_INET, (const void *) &sa.sin_addr, buf,
                                                MAX_HOSTNAME_LEN));
        HYDU_ASSERT(host_ip, status);
    }

    /* OK, if we are here, we got the remote IP.  We have two ways of
     * getting the local IP: gethostbyname or getifaddrs.  We'll try
     * both.  */

    /* STEP 2: Let's try the gethostbyname model */

    if ((ht = gethostbyname(lhost))) {
        memset((char *) &sa, 0, sizeof(struct sockaddr_in));
        memcpy(&sa.sin_addr, ht->h_addr_list[0], ht->h_length);

        /* Find the IP address of the host */
        lhost_ip = MPL_strdup((char *) inet_ntop(AF_INET, (const void *) &sa.sin_addr, buf,
                                                 MAX_HOSTNAME_LEN));
        HYDU_ASSERT(lhost_ip, status);

        /* See if the IP address of the hostname we got matches the IP
         * address to which the local host resolves */
        if (!strcmp(lhost_ip, host_ip)) {
            *is_local = 1;
            goto fn_exit;
        }
    }

    /* Either gethostbyname didn't resolve or we didn't find a match.
     * Either way, let's try the getifaddr model. */

    /* STEP 3: Let's try the getifaddr model */

    if (getifaddrs(&ifaddr) == -1)
        HYDU_ERR_SETANDJUMP(status, HYD_INTERNAL_ERROR, "getifaddrs failed\n");

    /* Find the IP addresses of all local interfaces */
    for (ifa = ifaddr; ifa; ifa = ifa->ifa_next) {
        if (ifa->ifa_addr && ifa->ifa_addr->sa_family == AF_INET) {
            struct sockaddr_in *sa_ptr = (struct sockaddr_in *) ifa->ifa_addr;

            lhost_ip = MPL_strdup((char *)
                                  inet_ntop(AF_INET, (const void *) &(sa_ptr->sin_addr), buf,
                                            MAX_HOSTNAME_LEN));
            HYDU_ASSERT(lhost_ip, status);

            /* For each local IP address, see if it matches the "host"
             * IP address */
            if (!strcmp(host_ip, lhost_ip)) {
                *is_local = 1;
                freeifaddrs(ifaddr);
                goto fn_exit;
            }

            MPL_free(lhost_ip);
            lhost_ip = NULL;
        }
    }

    freeifaddrs(ifaddr);

  fn_exit:
    if (host_ip)
        MPL_free(host_ip);
    if (lhost_ip)
        MPL_free(lhost_ip);
    return status;

  fn_fail:
    goto fn_exit;
}
#else
HYD_status HYDU_sock_is_local(char *host, int *is_local)
{
    char lhost[MAX_HOSTNAME_LEN];
    HYD_status status = HYD_SUCCESS;

    *is_local = 0;

    if (gethostname(lhost, MAX_HOSTNAME_LEN) < 0) {
        HYDU_ERR_SETANDJUMP(status, HYD_INTERNAL_ERROR, "gethostname returned an error\n");
    }
    else if (!strcmp(lhost, host)) {
        *is_local = 1;
    }

  fn_exit:
    return status;

  fn_fail:
    goto fn_exit;
}
#endif /* HAVE_GETIFADDRS && HAVE_INET_NTOP */

HYD_status
HYDU_sock_create_and_listen_portstr(char *iface, char *hostname, char *port_range,
                                    char **port_str,
                                    HYD_status(*callback) (int fd, HYD_event_t events,
                                                           void *userp), void *userp)
{
    int listenfd;
    char *sport, *real_port_range, *ip = NULL;
    uint16_t port;
    HYD_status status = HYD_SUCCESS;

    /* Listen on a port in the port range */
    port = 0;
    real_port_range = port_range ? MPL_strdup(port_range) : NULL;
    status = HYDU_sock_listen(&listenfd, real_port_range, &port);
    HYDU_ERR_POP(status, "unable to listen on port\n");

    /* Register the listening socket with the demux engine */
    status = HYDT_dmx_register_fd(1, &listenfd, HYD_POLLIN, userp, callback);
    HYDU_ERR_POP(status, "unable to register fd\n");

    /* Create a port string for MPI processes to use to connect to */
    if (iface) {
        status = HYDU_sock_get_iface_ip(iface, &ip);
        HYDU_ERR_POP(status, "unable to get network interface IP\n");
    }
    else if (hostname) {
        ip = MPL_strdup(hostname);
    }
    else {
        char localhost[MAX_HOSTNAME_LEN] = { 0 };

        if (gethostname(localhost, MAX_HOSTNAME_LEN) < 0)
            HYDU_ERR_SETANDJUMP(status, HYD_SOCK_ERROR, "unable to get local hostname\n");

        ip = MPL_strdup(localhost);
    }

    sport = HYDU_int_to_str(port);
    HYDU_MALLOC_OR_JUMP(*port_str, char *, strlen(ip) + 1 + strlen(sport) + 1, status);
    MPL_snprintf(*port_str, strlen(ip) + 1 + strlen(sport) + 1, "%s:%s", ip, sport);
    MPL_free(sport);

  fn_exit:
    if (ip)
        MPL_free(ip);
    return status;

  fn_fail:
    goto fn_exit;
}

HYD_status HYDU_sock_cloexec(int fd)
{
    int flags;
    HYD_status status = HYD_SUCCESS;

#if defined HAVE_FCNTL
    flags = fcntl(fd, F_GETFD, 0);
    if (flags < 0)
        HYDU_ERR_POP(status, "unable to get fd flags\n");
    flags |= FD_CLOEXEC;
    if (fcntl(fd, F_SETFD, flags) < 0)
        HYDU_ERR_POP(status, "unable to set fd flags\n");
#endif /* HAVE_FCNTL */

  fn_exit:
    return status;

  fn_fail:
    goto fn_exit;
}
