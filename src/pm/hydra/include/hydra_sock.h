/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef HYDRA_SOCKS_H_INCLUDED
#define HYDRA_SOCKS_H_INCLUDED

#include "hydra.h"

#include <poll.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/tcp.h>

#if !defined size_t
#define size_t unsigned int
#endif /* size_t */

HYD_Status HYDU_Sock_listen(int *listen_fd, int low_port, int high_port, int *port);
HYD_Status HYDU_Sock_connect(const char *host, int port, int *fd);
HYD_Status HYDU_Sock_accept(int listen_fd, int *fd);
HYD_Status HYDU_Sock_readline(int fd, char *buf, int maxlen, int *linelen);
HYD_Status HYDU_Sock_read(int fd, char *buf, int maxlen, int *count);
HYD_Status HYDU_Sock_writeline(int fd, char *buf, int maxsize);
HYD_Status HYDU_Sock_write(int fd, char *buf, int maxsize);
HYD_Status HYDU_Sock_set_nonblock(int fd);
HYD_Status HYDU_Sock_set_cloexec(int fd);

#endif /* HYDRA_SOCKS_H_INCLUDED */
