/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef SOCK_PRE_H_INCLUDED
#define SOCK_PRE_H_INCLUDED

#include <sys/socket.h>

/* need define MPIDI_BUILD_CH4_UPID_HASH to support dynamic process */
#define MPIDI_BUILD_CH4_UPID_HASH

typedef struct {
    int dummy;
} MPIDI_SOCK_Global_t;

typedef struct {
    int dummy;
} MPIDI_SOCK_am_request_t;

typedef struct {
    int dummy;
} MPIDI_SOCK_request_t;

typedef struct {
    int dummy;
} MPIDI_SOCK_comm_t;

typedef struct {
    int dummy;
} MPIDI_SOCK_dt_t;

typedef struct {
    int dummy;
} MPIDI_SOCK_op_t;

typedef struct {
    int dummy;
} MPIDI_SOCK_win_t;

/* The address of a process: where its listening socket can be reached. It is
 * exchanged as the business card and used as the upid for dynamic processes.
 * It is compared as bytes, so always zero the whole struct before filling. */
typedef struct {
    struct sockaddr_storage saddr;
    int port;                   /* host byte order; 0 means the address is not set */
} MPIDI_SOCK_name_t;

struct MPIDI_SOCK_conn;

typedef struct {
    MPIDI_SOCK_name_t name;
    struct MPIDI_SOCK_conn *conn;       /* outgoing connection, NULL until the first send */
} MPIDI_SOCK_addr_t;

typedef struct {
    int dummy;
} MPIDI_SOCK_part_t;

#endif /* SOCK_PRE_H_INCLUDED */
