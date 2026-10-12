/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef SOCK_PRE_H_INCLUDED
#define SOCK_PRE_H_INCLUDED

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

typedef struct {
    int dummy;
} MPIDI_SOCK_addr_t;

typedef struct {
    int dummy;
} MPIDI_SOCK_part_t;

#endif /* SOCK_PRE_H_INCLUDED */
