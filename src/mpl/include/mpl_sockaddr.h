/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2018 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef MPL_SOCKADDR_H_INCLUDED
#define MPL_SOCKADDR_H_INCLUDED

#include <sys/socket.h>

#define MPL_SOCKADDR_ANY 0
#define MPL_SOCKADDR_LOOPBACK 1
#define MPL_LISTEN_PUSH(a,b) MPL_set_listen_attr(a, b)
#define MPL_LISTEN_POP MPL_set_listen_attr(0, SOMAXCONN)

void MPL_sockaddr_set_aftype(int type);
int MPL_get_sockaddr(const char *s_hostname, struct sockaddr_storage *p_addr);
int MPL_get_sockaddr_direct(int type, struct sockaddr_storage *p_addr);
int MPL_get_sockaddr_iface(const char *s_iface, struct sockaddr_storage *p_addr);
int MPL_socket(void);
int MPL_connect(int socket, struct sockaddr_storage *p_addr, unsigned short port);
void MPL_set_listen_attr(int use_loopback, int max_conn);
int MPL_listen(int socket, unsigned short port);
int MPL_listen_anyport(int socket, unsigned short *p_port);
int MPL_listen_portrange(int socket, unsigned short *p_port, int low_port, int high_port);
int MPL_sockaddr_to_str(struct sockaddr_storage *p_addr, char *str, int maxlen);
int MPL_sockaddr_port(struct sockaddr_storage *p_addr);

#endif /* MPL_SOCKADDR_H_INCLUDED */
