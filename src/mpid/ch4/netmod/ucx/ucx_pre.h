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
    struct {
        bool is_set;
        int idx;
        int source;
    } am;
} MPIDI_UCX_ucp_request_t;

typedef struct {
    ucp_datatype_t ucp_datatype;
} MPIDI_UCX_dt_t;

typedef union {
    ucp_tag_message_h message_handler;
    MPIDI_UCX_ucp_request_t *ucp_request;
} MPIDI_UCX_request_t;

typedef struct {
    int handler_id;
    char *pack_buffer;
    ucp_dt_iov_t iov[2];
    int cmpl_cntr;              /* am message may be sent in pieces,
                                 * thus need track completion status */
} MPIDI_UCX_am_request_t;

typedef enum {
    MPIDI_UCX_AM_PACK_INVALID,
    MPIDI_UCX_AM_PACK_ONE,      /* all in one msg, if fit inside MPIDI_UCX_AM_BUF_SIZE */
    MPIDI_UCX_AM_PACK_TWO_A,    /* two msgs, msg_hdr + am_hdr and payload */
    MPIDI_UCX_AM_PACK_TWO_B,    /* two msgs, msg_hdr and am_hdr + payload, if am_hdr is too big */
} MPIDI_UCX_am_pack_type;

typedef struct MPIDI_UCX_am_header_t {
    uint8_t pack_type;
    uint8_t payload_seq;        /* needed to match payload packets */
    uint16_t handler_id;
    uint32_t am_hdr_sz;
    MPI_Aint data_sz;
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
    ucp_ep_h dest[MPIDI_CH4_MAX_VCIS][MPIDI_CH4_MAX_VCIS];
} MPIDI_UCX_addr_t;

typedef struct {
    int dummy;
} MPIDI_UCX_comm_t;

typedef struct {
    int dummy;
} MPIDI_UCX_op_t;

#endif /* UCX_PRE_H_INCLUDED */
