/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef UCX_PRE_H_INCLUDED
#define UCX_PRE_H_INCLUDED

#include <ucp/api/ucp.h>

#define MPIDI_UCX_KVSAPPSTRLEN 4096
#define MPIDI_UCX_MAX_AM_HDR_SIZE 256

enum {
    MPIDI_UCX_AMTYPE_NONE = 0,
    MPIDI_UCX_AMTYPE_SHORT,
    MPIDI_UCX_AMTYPE_PIPELINE,
};

typedef struct {
    int dummy;
} MPIDI_UCX_Global_t;

typedef union {
    MPIR_Request *req;
    MPI_Status *status;
    void *buf;
} MPIDI_UCX_ucp_request_t;

typedef struct {
    ucp_datatype_t ucp_datatype;
} MPIDI_UCX_dt_t;

typedef union {
    ucp_tag_message_h message_handler;
    MPIDI_UCX_ucp_request_t *ucp_request;
} MPIDI_UCX_request_t;

typedef struct MPIDI_UCX_am_header_t {
    uint32_t handler_id;
    uint32_t src_grank;
    uint64_t data_sz;
    uint64_t payload[];
} MPIDI_UCX_am_header_t;

struct MPIDI_UCX_deferred_am_isend_req;

typedef struct MPIDI_UCX_deferred_am_isend_req {
    int rank;
    MPIR_Comm *comm;
    int handler_id;
    const void *buf;
    size_t count;
    MPI_Datatype datatype;
    MPIR_Request *sreq;
    bool need_packing;

    MPI_Aint data_sz;

    int ucx_handler_id;
    ucp_ep_h ep;

    uint16_t am_hdr_sz;
    uint8_t am_hdr[MPIDI_UCX_MAX_AM_HDR_SIZE];

    struct MPIDI_UCX_deferred_am_isend_req *prev;
    struct MPIDI_UCX_deferred_am_isend_req *next;
} MPIDI_UCX_deferred_am_isend_req_t;

typedef struct {
    int handler_id;
    char *pack_buffer;
    bool is_gpu_pack_buffer;
    int am_type_choice;
    MPI_Aint data_sz;
    ucp_dt_iov_t iov[3];
    MPIR_Request *sreq;

    MPIDI_UCX_deferred_am_isend_req_t *deferred_req;
} MPIDI_UCX_am_request_t;

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
    MPIDI_UCX_win_info_t *info_table;   /* NULL indicates AM fallback for entire win */
    ucp_mem_h mem_h;
    bool mem_mapped;            /* Indicate whether mem_h has been mapped (e.g., supported mem type).
                                 * Set at win init and checked at win free for mem_unmap */

    MPIDI_UCX_win_target_sync_t *target_sync;
} MPIDI_UCX_win_t;

typedef struct {
    ucp_ep_h dest[MPIDI_CH4_MAX_VCIS][MPIDI_CH4_MAX_VCIS];
} MPIDI_UCX_addr_t;

typedef struct {
    int dummy;
} MPIDI_UCX_comm_t;

typedef struct {
    int dummy;
} MPIDI_UCX_op_t;

#endif /* UCX_PRE_H_INCLUDED */
