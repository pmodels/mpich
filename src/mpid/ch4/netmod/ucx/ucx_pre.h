/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef UCX_PRE_H_INCLUDED
#define UCX_PRE_H_INCLUDED

#include <ucp/api/ucp.h>

#define MPIDI_UCX_KVSAPPSTRLEN 4096

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

typedef struct {
    union {
        struct {
            int handler_id;
            char *pack_buffer;
            ucp_dt_iov_t iov[2];
        } send;
        struct {
            MPI_Aint data_sz;
            void *data_desc;
            char *pack_buffer;
        } recv;
    } u;
} MPIDI_UCX_am_request_t;

#define MPIDI_UCX_AM_SEND_REQUEST(req,field) ((req)->dev.ch4.am.netmod_am.ucx.u.send.field)
#define MPIDI_UCX_AM_RECV_REQUEST(req,field) ((req)->dev.ch4.am.netmod_am.ucx.u.recv.field)

typedef struct MPIDI_UCX_am_header_t {
    uint16_t handler_id;
    uint8_t src_vci;
    uint8_t dst_vci;
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

typedef struct {
    int dummy;
} MPIDI_UCX_part_t;

#endif /* UCX_PRE_H_INCLUDED */
