/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2016 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 *  Portions of this code were written by Mellanox Technologies Ltd.
 *  Copyright (C) Mellanox Technologies Ltd. 2016. ALL RIGHTS RESERVED
 */
#ifndef UCX_PRE_H_INCLUDED
#define UCX_PRE_H_INCLUDED

#include <ucp/api/ucp.h>

#define MPIDI_UCX_KVSAPPSTRLEN 4096

typedef struct {
    void *req;
} MPIDI_UCX_ucp_request_t;

typedef struct {
    ucp_datatype_t ucp_datatype;
} MPIDI_UCX_dt_t;

typedef struct {
    union {
        ucp_tag_message_h message_handler;
        MPIDI_UCX_ucp_request_t *ucp_request;
    } a;
} MPIDI_UCX_request_t;

typedef struct {
    int handler_id;
    char *pack_buffer;
} MPIDI_UCX_am_request_t;

typedef struct MPIDI_UCX_am_header_t {
    uint64_t handler_id;
    uint64_t data_sz;
    uint64_t payload[0];
} MPIDI_UCX_am_header_t;

typedef struct MPIDI_UCX_win_info {
    ucp_rkey_h rkey;
    uint64_t addr;
    uint32_t disp;
} __attribute__ ((packed)) MPIDI_UCX_win_info_t;

typedef enum {
    MPIDI_UCX_WIN_SYNC_UNSET = 0,
    MPIDI_UCX_WIN_SYNC_FLUSH_LOCAL = 1, /* need both local and remote flush */
    MPIDI_UCX_WIN_SYNC_FLUSH = 2        /* need only remote flush */
} MPIDI_UCX_win_sync_flag_t;

typedef struct MPIDI_UCX_win_target_sync {
    MPIDI_UCX_win_sync_flag_t need_sync;        /* flag for op completion */
    bool outstanding_atomics;   /* flag for atomics ordering and atomicity */
} MPIDI_UCX_win_target_sync_t;

typedef struct {
    MPIDI_UCX_win_info_t *info_table;
    ucp_mem_h mem_h;
    MPIDI_UCX_win_target_sync_t *target_sync;
} MPIDI_UCX_win_t;

typedef struct {
    ucp_ep_h dest;
} MPIDI_UCX_addr_t;

typedef struct {
    int dummy;
} MPIDI_UCX_comm_t;

typedef struct {
    int dummy;
} MPIDI_UCX_op_t;

#endif /* UCX_PRE_H_INCLUDED */
