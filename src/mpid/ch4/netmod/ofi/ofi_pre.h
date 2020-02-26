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

#ifndef OFI_PRE_H_INCLUDED
#define OFI_PRE_H_INCLUDED

#include <mpi.h>
#include <rdma/fabric.h>
#include <rdma/fi_endpoint.h>
#include <rdma/fi_domain.h>
#include <rdma/fi_tagged.h>
#include <rdma/fi_rma.h>
#include <rdma/fi_atomic.h>
#include <rdma/fi_cm.h>
#include <rdma/fi_errno.h>
#include "ofi_capability_sets.h"

/* Defines */

/* configure option --enable-ofi-domain */
#ifndef MPIDI_OFI_VNI_USE_DOMAIN
#define MPIDI_OFI_VNI_USE_SEPCTX       1
#endif

#define MPIDI_OFI_MAX_AM_HDR_SIZE    128
#define MPIDI_OFI_AM_HANDLER_ID_BITS   8
#define MPIDI_OFI_AM_TYPE_BITS         8
#define MPIDI_OFI_AM_HDR_SZ_BITS       8
#define MPIDI_OFI_AM_DATA_SZ_BITS     48
#define MPIDI_OFI_AM_RANK_BITS        32
#define MPIDI_OFI_AM_MSG_HEADER_SIZE (sizeof(MPIDI_OFI_am_header_t))

/* Typedefs */

struct MPIR_Comm;
struct MPIR_Request;

typedef struct {
    void *huge_send_counters;
    void *huge_recv_counters;
    /* support for connection */
    int conn_id;
} MPIDI_OFI_comm_t;

typedef struct {
    /* context id and src rank so the target side can
     * issue RDMA read operation */
    MPIR_Context_id_t context_id;
    int src_rank;

    uint64_t src_offset;
    MPIR_Request *sreq_ptr;
    uint64_t am_hdr_src;
    uint64_t rma_key;
} MPIDI_OFI_lmt_msg_payload_t;

typedef struct {
    MPIR_Request *sreq_ptr;
} MPIDI_OFI_ack_msg_payload_t;

typedef struct MPIDI_OFI_am_header_t {
    uint64_t handler_id:MPIDI_OFI_AM_HANDLER_ID_BITS;
    uint64_t am_type:MPIDI_OFI_AM_TYPE_BITS;
    uint64_t am_hdr_sz:MPIDI_OFI_AM_HDR_SZ_BITS;
    uint64_t data_sz:MPIDI_OFI_AM_DATA_SZ_BITS;
    uint16_t seqno;             /* Sequence number of this message.
                                 * Number is unique to (fi_src_addr, fi_dest_addr) pair. */
    fi_addr_t fi_src_addr;      /* OFI address of the sender */
} MPIDI_OFI_am_header_t;

/* Represents early-arrived active messages.
 * Queued to MPIDI_OFI_global.am_unordered_msgs */
typedef struct MPIDI_OFI_am_unordered_msg {
    struct MPIDI_OFI_am_unordered_msg *next;
    struct MPIDI_OFI_am_unordered_msg *prev;
    MPIDI_OFI_am_header_t am_hdr;
    /* This is used as a variable-length structure.
     * Additional memory region may follow. */
} MPIDI_OFI_am_unordered_msg_t;

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
    MPIR_Request *rreq_ptr;
    void *am_hdr;
    uint16_t am_hdr_sz;
    uint8_t pad[6];
    MPIDI_OFI_am_header_t msg_hdr;
    uint8_t am_hdr_buf[MPIDI_OFI_MAX_AM_HDR_SIZE];
    /* FI_ASYNC_IOV requires an iov storage to be alive until a request completes */
#if MPIDI_OFI_IOVEC_ALIGN <= SIZEOF_VOID_P
    struct iovec iov[3];
#else
    /* need bigger alignment */
    struct iovec iov[3] MPL_ATTR_ALIGNED(MPIDI_OFI_IOVEC_ALIGN);
#endif
} MPIDI_OFI_am_request_header_t;

typedef struct {
    struct fi_context context[MPIDI_OFI_CONTEXT_STRUCTS];       /* fixed field, do not move */
    int event_id;               /* fixed field, do not move */
    MPIDI_OFI_am_request_header_t *req_hdr;
} MPIDI_OFI_am_request_t;


typedef struct {
    void *buf;
    size_t count;
    MPI_Datatype datatype;
    char pack_buffer[];
} MPIDI_OFI_pack_t;

typedef struct {
    struct fi_context context[MPIDI_OFI_CONTEXT_STRUCTS];       /* fixed field, do not move */
    int event_id;               /* fixed field, do not move */
    int util_id;
    MPI_Datatype datatype;
    union {
        MPIDI_OFI_pack_t *pack;
        struct iovec *nopack;
    } noncontig;
    union {
#if MPIDI_OFI_IOVEC_ALIGN <= SIZEOF_VOID_P
        struct iovec iov;
#else
        /* Enforce larger alignment. */
        /*   icc complains alignment attribute without packed struct */
        char iov_store[sizeof(struct iovec) + MPIDI_OFI_IOVEC_ALIGN - 1];
#endif
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
struct MPIDI_OFI_win_hint;

/* Stores per-rank information for RMA */
typedef struct {
    int32_t disp_unit;
    /* For MR_BASIC mode we need to store an MR key and a base address of the target window */
    /* TODO - Ideally, we'd like to not have these fields compiled in if not
     * using MR_BASIC. In practice, doing so makes the code very complex
     * elsewhere for very little payoff. */
    uint64_t mr_key;
    uintptr_t base;
} MPIDI_OFI_win_targetinfo_t;

typedef struct {
    struct fid_mr *mr;
    uint64_t mr_key;
    struct fid_ep *ep;          /* EP with counter & completion */
    int sep_tx_idx;             /* transmit context index for scalable EP,
                                 * -1 means using non scalable EP. */
    uint64_t *issued_cntr;
    uint64_t issued_cntr_v;     /* main body of an issued counter,
                                 * if we are to use per-window counter */
    struct fid_cntr *cmpl_cntr;
    uint64_t win_id;
    struct MPIDI_OFI_win_request *syncQ;
    MPIDI_OFI_win_targetinfo_t *winfo;

    /* Accumulate related info. The struct internally uses MPIDI_OFI_DT_SIZES
     * defined in ofi_types.h to allocate the max_count array. The struct
     * size is unknown when we load ofi_pre.h, thus we only set a pointer here. */
    struct MPIDI_OFI_win_acc_hint *acc_hint;

    /* Counter to track whether or not to kick the progress engine when the OFI provider does not
     * supply automatic progress. This can make a big performance difference when doing
     * large, non-contiguous RMA operations. */
    int progress_counter;
} MPIDI_OFI_win_t;

typedef struct {
    fi_addr_t dest;
#if MPIDI_OFI_ENABLE_ENDPOINTS_BITS
    unsigned ep_idx:MPIDI_OFI_MAX_ENDPOINTS_BITS_SCALABLE;
#endif
} MPIDI_OFI_addr_t;

#include "ofi_coll_params.h"
#include "ofi_coll_containers.h"
#endif /* OFI_PRE_H_INCLUDED */
