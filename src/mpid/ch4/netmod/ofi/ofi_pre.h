/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 *  Portions of this code were written by Intel Corporation.
 *  Copyright (C) 2011-2016 Intel Corporation.  Intel provides this material
 *  to Argonne National Laboratory subject to Software Grant and Corporate
 *  Contributor License Agreement dated February 8, 2012.
 */

#ifndef NETMOD_OFI_PRE_H_INCLUDED
#define NETMOD_OFI_PRE_H_INCLUDED

#include <mpi.h>
#include <rdma/fabric.h>
#include <rdma/fi_endpoint.h>
#include <rdma/fi_domain.h>
#include <rdma/fi_tagged.h>
#include <rdma/fi_rma.h>
#include <rdma/fi_atomic.h>
#include <rdma/fi_cm.h>
#include <rdma/fi_errno.h>

/* Defines */

#define MPIDI_OFI_MAX_AM_HDR_SIZE    128
#define MPIDI_OFI_AM_HANDLER_ID_BITS   8
#define MPIDI_OFI_AM_TYPE_BITS         8
#define MPIDI_OFI_AM_HDR_SZ_BITS       8
#define MPIDI_OFI_AM_DATA_SZ_BITS     48
#define MPIDI_OFI_AM_CONTEXT_ID_BITS  16
#define MPIDI_OFI_AM_RANK_BITS        32
#define MPIDI_OFI_AM_MSG_HEADER_SIZE (sizeof(MPIDI_OFI_am_header_t))

#ifdef MPIDI_OFI_CONFIG_USE_SCALABLE_ENDPOINTS
#define MPIDI_OFI_MAX_ENDPOINTS      256
#define MPIDI_OFI_MAX_ENDPOINTS_BITS   8
#else
#define MPIDI_OFI_MAX_ENDPOINTS        1
#define MPIDI_OFI_MAX_ENDPOINTS_BITS   0
#endif

/* Typedefs */

struct MPIR_Comm;
struct MPIR_Request;

typedef struct {
    void *huge_send_counters;
    void *huge_recv_counters;
    void *win_id_allocator;
    void *rma_id_allocator;
} MPIDI_OFI_comm_t;
enum {
    MPIDI_AMTYPE_SHORT_HDR = 0,
    MPIDI_AMTYPE_SHORT,
    MPIDI_AMTYPE_LMT_REQ,
    MPIDI_AMTYPE_LMT_ACK
};

typedef struct {
    /* context id and src rank so the target side can
     * issue RDMA read operation */
    MPIR_Context_id_t context_id;
    int src_rank;

    uint64_t src_offset;
    uint64_t sreq_ptr;
    uint64_t am_hdr_src;
    uint64_t rma_key;
} MPIDI_OFI_lmt_msg_payload_t;

typedef struct {
    uint64_t sreq_ptr;
} MPIDI_OFI_ack_msg_payload_t;

typedef struct MPIDI_OFI_am_header_t {
    uint64_t handler_id:MPIDI_OFI_AM_HANDLER_ID_BITS;
    uint64_t am_type:MPIDI_OFI_AM_TYPE_BITS;
    uint64_t am_hdr_sz:MPIDI_OFI_AM_HDR_SZ_BITS;
    uint64_t data_sz:MPIDI_OFI_AM_DATA_SZ_BITS;
    uint64_t payload[0];
} MPIDI_OFI_am_header_t;

typedef struct {
    MPIDI_OFI_am_header_t hdr;
    MPIDI_OFI_ack_msg_payload_t pyld;
} MPIDI_OFI_ack_msg_t;

typedef struct {
    MPIDI_OFI_am_header_t hdr;
    MPIDI_OFI_lmt_msg_payload_t pyld;
} MPIDI_OFI_lmt_msg_t;

typedef struct {
    MPIDI_OFI_lmt_msg_payload_t lmt_info;
    uint64_t lmt_cntr;
    struct fid_mr *lmt_mr;
    void *pack_buffer;
    void *rreq_ptr;
    void *am_hdr;
    int (*cmpl_handler_fn) (struct MPIR_Request * req);
    uint16_t am_hdr_sz;
    uint8_t pad[6];
    MPIDI_OFI_am_header_t msg_hdr;
    uint8_t am_hdr_buf[MPIDI_OFI_MAX_AM_HDR_SIZE];
} MPIDI_OFI_am_request_header_t;

typedef struct {
    struct fi_context context;  /* fixed field, do not move */
    int event_id;               /* fixed field, do not move */
    MPIDI_OFI_am_request_header_t *req_hdr;
} MPIDI_OFI_am_request_t;


typedef struct MPIDI_OFI_noncontig_t {
    struct MPIDU_Segment segment;
    char pack_buffer[0];
} MPIDI_OFI_noncontig_t;

typedef struct {
    struct fi_context context;  /* fixed field, do not move */
    int event_id;               /* fixed field, do not move */
    int util_id;
    struct MPIR_Comm *util_comm;
    MPI_Datatype datatype;
    MPIDI_OFI_noncontig_t *noncontig;
    /* persistent send fields */
    union {
        struct {
            int type;
            int rank;
            int tag;
            int count;
            void *buf;
        } persist;
        struct iovec iov;
        void *inject_buf;       /* Internal buffer for inject emulation */
    } util;
} MPIDI_OFI_request_t;

typedef struct {
    int index;
} MPIDI_OFI_dt_t;

typedef struct {
    int dummy;
} MPIDI_OFI_op_t;

struct MPIDI_OFI_win_request;

/* Stores per-rank information for RMA */
typedef struct {
    int32_t disp_unit;
#ifndef USE_OFI_MR_SCALABLE
    /* For MR_BASIC mode we need to store an MR key and a base address of the target window */
    uint64_t mr_key;
    uintptr_t base;
#endif
} MPIDI_OFI_win_targetinfo_t;

typedef struct {
    struct fid_mr *mr;
    uint64_t mr_key;
    struct fid_ep *ep;          /* EP with counter & completion */
    struct fid_ep *ep_nocmpl;   /* EP with counter only (no completion) */
    uint64_t *issued_cntr;
    uint64_t issued_cntr_v;     /* main body of an issued counter,
                                 * if we are to use per-window counter */
    struct fid_cntr *cmpl_cntr;
    uint64_t win_id;
    struct MPIDI_OFI_win_request *syncQ;
    MPIDI_OFI_win_targetinfo_t *winfo;
} MPIDI_OFI_win_t;

typedef struct {
    fi_addr_t dest;
#ifdef MPIDI_OFI_CONFIG_USE_SCALABLE_ENDPOINTS
    unsigned ep_idx:MPIDI_OFI_MAX_ENDPOINTS_BITS;
#endif
} MPIDI_OFI_addr_t;

#endif
