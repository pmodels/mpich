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

/* ---- MPIDI_UCX_ucp_request_t --------
 *
 * UCX xxx_nb functions, e.g. ucp_tag_recv_nb, will allocate (inside ucp) a ucp_request object
 * to track between call-site and callback. The object is a union of different types based on
 * operations.
 *
 * For recv, either `struct ucx_recv` or `struct ucx_recv_cb` will be used depending on whether
 * it's an immediate completion. The `is_set` flag is used to signal between the call-site
 * and the callback. Note that both call-site and callback are pretected by the same mutex, so
 * there won't be race conditions.
 */

/* async recv, set rreq and let callback to complete */
struct ucx_recv {
    MPIR_Request *rreq;
};

/* recv immediate complete, callback sets status */
struct ucx_recv_cb {
    MPI_Status status;
};

/* async send, set sreq and let callback to complete */
struct ucx_send {
    MPIR_Request *sreq;
};

/* send immediate complete, callback is skipped */

/* async am send, set sreq and let callback to complete */
struct ucx_am_send {
    MPIR_Request *sreq;
    int handler_id;
    void *pack_buffer;
};

/* am recv, use static variable and caller wait for callback completion */

/* async put/get */
struct ucx_rma {
    MPIR_Request *req;
};

/* rma put/get immediate complete, callback is skipped */

typedef struct {
    int is_set;                 /* only used for recv */
    union {
        struct ucx_recv recv;
        struct ucx_recv_cb recv_cb;
        struct ucx_send send;
        struct ucx_am_send am_send;
        struct ucx_rma rma;
    } u;
} MPIDI_UCX_ucp_request_t;

/* `ucp_request` objects are managed by ucx internally. `request_init_callback` (ref. ucx_init.c)
 * is only called at ucx memory pool init time. We need explicitly clear the object before
 * `ucp_request_release`, or we'll have a corrupted `ucp_request` object next operation.
 */
#define MPIDI_UCX_UCP_REQUEST_RELEASE(ucp_request) \
    do { \
        memset(ucp_request, 0, sizeof(MPIDI_UCX_ucp_request_t)); \
        ucp_request_release(ucp_request); \
    } while (0)

/* ---- MPIDI_UCX_request_t ---------
 *
 * For tracking data between operations, we use the request objects. We define one
 * struct for each operation/usage, and define MPIDI_UCX_request_t as a union.
 */

/* for between mprobe and mrecv */
typedef struct {
    ucp_tag_message_h message_h;
} MPIDI_UCX_req_mprobe_t;

/* for between recv and cancel_recv */
typedef struct {
    void *ucp_request;          /* needed by ucp_request_cancel */
} MPIDI_UCX_req_recv_t;

/* for between send and cancel_send */
typedef struct {
    void *ucp_request;          /* needed by ucp_request_cancel */
} MPIDI_UCX_req_send_t;

typedef union {
    MPIDI_UCX_req_mprobe_t mprobe;
    MPIDI_UCX_req_recv_t recv;
    MPIDI_UCX_req_send_t send;
} MPIDI_UCX_request_t;

/* ---- MPIDI_UCX_am_request_t ---------
 *
 * Similar to MPIDI_UCX_request_t but used for tracking data within an AM protocol.
 * We can't use MPIDI_UCX_request_t because these need co-exist with upper-layer
 * AM data.
 */

typedef struct {
    int handler_id;
    char *pack_buffer;
    ucp_dt_iov_t iov[2];
} MPIDI_UCX_am_request_t;

/* ---- Other types -------- */

typedef struct {
    ucp_datatype_t ucp_datatype;
} MPIDI_UCX_dt_t;

typedef struct MPIDI_UCX_am_header_t {
    uint64_t handler_id;
    uint64_t data_sz;
    uint64_t payload[];
} MPIDI_UCX_am_header_t;

typedef struct MPIDI_UCX_win_info {
    ucp_rkey_h rkey;
    uint64_t addr;
    uint32_t disp;
} MPIDI_UCX_win_info_t;

typedef enum {
    MPIDI_UCX_WIN_SYNC_UNSET = 0,
    MPIDI_UCX_WIN_SYNC_FLUSH_LOCAL = 1, /* need both local and remote flush */
    MPIDI_UCX_WIN_SYNC_FLUSH = 2        /* need only remote flush */
} MPIDI_UCX_win_sync_flag_t;

typedef struct MPIDI_UCX_win_target_sync {
    MPIDI_UCX_win_sync_flag_t need_sync;        /* flag for op completion */
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
