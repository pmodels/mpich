/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2017 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef HYDRA_SOCK_H_INCLUDED
#define HYDRA_SOCK_H_INCLUDED

#include "hydra_base.h"

enum HYD_sock_comm_flag {
    HYD_SOCK_COMM_TYPE__NONBLOCKING = 0,
    HYD_SOCK_COMM_TYPE__BLOCKING = 1
};

HYD_status HYD_sock_listen_on_port(int *listen_fd, uint16_t port);
HYD_status HYD_sock_listen_on_any_port(int *listen_fd, uint16_t * port);
HYD_status HYD_sock_listen_on_port_range(int *listen_fd, const char *port_range, uint16_t * port);
HYD_status HYD_sock_connect(const char *host, uint16_t port, int *fd, int retries,
                            unsigned long delay);
HYD_status HYD_sock_accept(int listen_fd, int *fd);
HYD_status HYD_sock_read(int fd, void *buf, int maxlen, int *recvd, int *closed,
                         enum HYD_sock_comm_flag flag);
HYD_status HYD_sock_write(int fd, const void *buf, int maxlen, int *sent, int *closed,
                          enum HYD_sock_comm_flag flag);
HYD_status HYD_sock_cloexec(int fd);

#endif /* HYDRA_SOCK_H_INCLUDED */
