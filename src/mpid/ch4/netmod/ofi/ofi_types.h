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
#ifndef OFI_TYPES_H_INCLUDED
#define OFI_TYPES_H_INCLUDED

#include <netdb.h>
#include <stddef.h>
#include <inttypes.h>
#ifdef HAVE_STDINT_H
#include <stdint.h>
#endif
#include "ofi_pre.h"
#include "ch4_types.h"
#include "mpidch4r.h"

#define __SHORT_FILE__                          \
    (strrchr(__FILE__,'/')                      \
     ? strrchr(__FILE__,'/')+1                  \
     : __FILE__                                 \
)
#define MPIDI_OFI_MAP_NOT_FOUND            ((void*)(-1UL))
#define MPIDI_OFI_DEFAULT_SHORT_SEND_SIZE  (16 * 1024)
#define MPIDI_OFI_MAX_NUM_AM_BUFFERS       (8)
#define MPIDI_OFI_AM_BUFF_SZ               (1 * 1024 * 1024)
#define MPIDI_OFI_CACHELINE_SIZE           (64)
#define MPIDI_OFI_IOV_MAX                  (32)
#define MPIDI_OFI_BUF_POOL_SIZE            (1024)
#define MPIDI_OFI_BUF_POOL_NUM             (1024)
#define MPIDI_OFI_NUM_CQ_BUFFERED          (1024)
#define MPIDI_OFI_INTERNAL_HANDLER_CONTROL (MPIDI_AM_HANDLERS_MAX-1)
#define MPIDI_OFI_INTERNAL_HANDLER_NEXT    (MPIDI_AM_HANDLERS_MAX-2)

#define MPIDI_OFI_PROTOCOL_BITS (2)     /* This is set to 2 event though we actually use 3. The ssend
                                         * ack bit needs to live outside the protocol bit space to avoid
                                         * accidentally matching unintended messages. Because of this,
                                         * we shift the PROTOCOL_MASK one extra bit to the left to take
                                         * the place of the empty SSEND_ACK bit. */

#if MPIDI_OFI_ENABLE_RUNTIME_CHECKS == MPIDI_OFI_ON
#define MPIDI_OFI_SYNC_SEND_ACK      (1ULL << (MPIDI_OFI_CONTEXT_BITS + MPIDI_OFI_SOURCE_BITS + MPIDI_OFI_TAG_BITS))
#define MPIDI_OFI_SYNC_SEND          (2ULL << (MPIDI_OFI_CONTEXT_BITS + MPIDI_OFI_SOURCE_BITS + MPIDI_OFI_TAG_BITS))
#define MPIDI_OFI_DYNPROC_SEND       (4ULL << (MPIDI_OFI_CONTEXT_BITS + MPIDI_OFI_SOURCE_BITS + MPIDI_OFI_TAG_BITS))
#define MPIDI_OFI_PROTOCOL_MASK      (((1ULL << MPIDI_OFI_PROTOCOL_BITS) - 1) << 1 << (MPIDI_OFI_CONTEXT_BITS + MPIDI_OFI_SOURCE_BITS + MPIDI_OFI_TAG_BITS))
#define MPIDI_OFI_CONTEXT_MASK       (((1ULL << MPIDI_OFI_CONTEXT_BITS) - 1) << (MPIDI_OFI_SOURCE_BITS + MPIDI_OFI_TAG_BITS))
#define MPIDI_OFI_SOURCE_MASK        (((1ULL << MPIDI_OFI_SOURCE_BITS) - 1) << MPIDI_OFI_TAG_BITS)
#define MPIDI_OFI_TAG_MASK           ((1ULL << MPIDI_OFI_TAG_BITS) - 1)
/* This value comes from the fact that we use a uint32_t in
 * MPIDI_OFI_send_handler to define the dest and that is the size we expect
 * from the OFI provider for its immediate data field. */
#define MPIDI_OFI_MAX_RANK_BITS      (MPIDI_OFI_SOURCE_BITS > 0 ? MPIDI_OFI_SOURCE_BITS : 32)
#else
#if MPIDI_OFI_SOURCE_BITS == 0
/* This value comes from the fact that we use a uint32_t in
 * MPIDI_OFI_send_handler to define the dest and that is the size we expect
 * from the OFI provider for its immediate data field. */
#define MPIDI_OFI_MAX_RANK_BITS      32
#else
#define MPIDI_OFI_MAX_RANK_BITS      MPIDI_OFI_SOURCE_BITS
#endif
#endif

/* RMA Key Space division
 *    |                  |                   |                        |
 *    ...     Context ID | key type flag bit |  Key/window instance   |
 *    |                  |                   |                        |
 */
/* 64-bit key space                         */
/* 2147483648 window instances per comm           */
/* 2147483648 outstanding huge RMAS per comm      */
/* 4294967296 communicators                       */
#define MPIDI_OFI_MAX_KEY_TYPE_BITS_64 (1)
#define MPIDI_OFI_MAX_CONTEXT_BITS_64  (32)
#define MPIDI_OFI_MAX_KEY_BITS_64      (64-(MPIDI_OFI_MAX_KEY_TYPE_BITS_64+MPIDI_OFI_MAX_CONTEXT_BITS_64))
#define MPIDI_OFI_MAX_HUGE_RMAS_64     (1<<(MPIDI_OFI_MAX_KEY_BITS_64))
#define MPIDI_OFI_MAX_WINDOWS_64       MPIDI_OFI_MAX_HUGE_RMAS_64

#define MPIDI_OFI_CONTEXT_SHIFT_64     (MPIDI_OFI_MAX_KEY_BITS_64+MPIDI_OFI_MAX_KEY_TYPE_BITS_64)
#define MPIDI_OFI_MAX_KEY_TYPE_SHIFT_64 (MPIDI_OFI_MAX_KEY_BITS_64)

/* 32-bit key space                         */
/* 32K window instances per comm           */
/* 32K outstanding huge RMAS per comm      */
/* 64K communicators                       */
#define MPIDI_OFI_MAX_KEY_TYPE_BITS_32 (1)
#define MPIDI_OFI_MAX_CONTEXT_BITS_32  (16)
#define MPIDI_OFI_MAX_KEY_BITS_32      (32-(MPIDI_OFI_MAX_KEY_TYPE_BITS_32+MPIDI_OFI_MAX_CONTEXT_BITS_32))
#define MPIDI_OFI_MAX_HUGE_RMAS_32     (1<<(MPIDI_OFI_MAX_KEY_BITS_32))
#define MPIDI_OFI_MAX_WINDOWS_32       MPIDI_OFI_MAX_HUGE_RMAS_32

#define MPIDI_OFI_CONTEXT_SHIFT_32     (MPIDI_OFI_MAX_KEY_BITS_32+MPIDI_OFI_MAX_KEY_TYPE_BITS_32)
#define MPIDI_OFI_MAX_KEY_TYPE_SHIFT_32 (MPIDI_OFI_MAX_KEY_BITS_32)

/* 16-bit key space                         */
/* 128 window instances per comm             */
/* 128 outstanding huge RMAS per comm        */
/* 256 communicators                          */
#define MPIDI_OFI_MAX_KEY_TYPE_BITS_16 (1)
#define MPIDI_OFI_MAX_CONTEXT_BITS_16  (8)
#define MPIDI_OFI_MAX_KEY_BITS_16      (16-(MPIDI_OFI_MAX_KEY_TYPE_BITS_16+MPIDI_OFI_MAX_CONTEXT_BITS_16))
#define MPIDI_OFI_MAX_HUGE_RMAS_16     (1<<(MPIDI_OFI_MAX_KEY_BITS_16))
#define MPIDI_OFI_MAX_WINDOWS_16       MPIDI_OFI_MAX_HUGE_RMAS_16

#define MPIDI_OFI_CONTEXT_SHIFT_16     (MPIDI_OFI_MAX_KEY_BITS_16+MPIDI_OFI_MAX_KEY_TYPE_BITS_16)
#define MPIDI_OFI_MAX_KEY_TYPE_SHIFT_16 (MPIDI_OFI_MAX_KEY_BITS_16)

#define MPIDI_OFI_KEY_TYPE_HUGE_RMA (0)
#define MPIDI_OFI_KEY_TYPE_WINDOW (1)


#ifdef HAVE_FORTRAN_BINDING
#ifdef MPICH_DEFINE_2COMPLEX
#define MPIDI_OFI_DT_SIZES 63
#else
#define MPIDI_OFI_DT_SIZES 61
#endif
#else
#define MPIDI_OFI_DT_SIZES 40
#endif
#define MPIDI_OFI_OP_SIZES 15

#define MPIDI_OFI_THREAD_UTIL_MUTEX     MPIDI_Global.mutexes[0].m
#define MPIDI_OFI_THREAD_PROGRESS_MUTEX MPIDI_Global.mutexes[1].m
#define MPIDI_OFI_THREAD_FI_MUTEX       MPIDI_Global.mutexes[2].m
#define MPIDI_OFI_THREAD_SPAWN_MUTEX    MPIDI_Global.mutexes[3].m

/* Field accessor macros */
#define MPIDI_OFI_OBJECT_HEADER_SIZE       offsetof(MPIDI_OFI_offset_checker_t,  pad)
#define MPIDI_OFI_AMREQUEST(req,field)     ((req)->dev.ch4.am.netmod_am.ofi.field)
#define MPIDI_OFI_AMREQUEST_HDR(req,field) ((req)->dev.ch4.am.netmod_am.ofi.req_hdr->field)
#define MPIDI_OFI_AMREQUEST_HDR_PTR(req)   ((req)->dev.ch4.am.netmod_am.ofi.req_hdr)
#define MPIDI_OFI_REQUEST(req,field)       ((req)->dev.ch4.netmod.ofi.field)
#define MPIDI_OFI_AV(av)                   ((av)->netmod.ofi)

#define MPIDI_OFI_DATATYPE(dt)   ((dt)->dev.netmod.ofi)
#define MPIDI_OFI_COMM(comm)     ((comm)->dev.ch4.netmod.ofi)

/* Convert the address vector entry to an endpoint index.
 * This conversion depends on the data structure which could change based on
 * whether we're using scalable endpoints or not. */
static inline int MPIDI_OFI_av_to_ep(MPIDI_OFI_addr_t * av)
{
#if MPIDI_OFI_ENABLE_RUNTIME_CHECKS
    return (av)->ep_idx;
#else /* This is necessary for older GCC compilers that don't properly do this
       * detection when using elif */
#if MPIDI_OFI_ENABLE_SCALABLE_ENDPOINTS
    return (av)->ep_idx;
#else
    return 0;
#endif
#endif
}

/* Convert a communicator and rank to an endpoint index.
 * This conversion depends on the data structure which could change based on
 * whether we're using scalable endpoints or not. */
static inline int MPIDI_OFI_comm_to_ep(MPIR_Comm * comm_ptr, int rank)
{
#if MPIDI_OFI_ENABLE_RUNTIME_CHECKS
    return MPIDI_OFI_AV(MPIDIU_comm_rank_to_av(comm_ptr, rank)).ep_idx;
#else /* This is necessary for older GCC compilers that don't properly do this
       * detection when using elif */
#if MPIDI_OFI_ENABLE_SCALABLE_ENDPOINTS
    return MPIDI_OFI_AV(MPIDIU_comm_rank_to_av(comm_ptr, rank)).ep_idx;
#else
    return 0;
#endif
#endif
}

#define MPIDI_OFI_DO_SEND        0
#define MPIDI_OFI_DO_INJECT      1
#define MPIDI_OFI_NUM_CQ_ENTRIES 8

/* Typedefs */
enum {
    MPIDI_OFI_CTRL_ASSERT,    /**< Lock acknowledge      */
    MPIDI_OFI_CTRL_LOCKACK,   /**< Lock acknowledge      */
    MPIDI_OFI_CTRL_LOCKALLACK,/**< Lock all acknowledge  */
    MPIDI_OFI_CTRL_LOCKREQ,   /**< Lock window           */
    MPIDI_OFI_CTRL_LOCKALLREQ,/**< Lock all window       */
    MPIDI_OFI_CTRL_UNLOCK,    /**< Unlock window         */
    MPIDI_OFI_CTRL_UNLOCKACK, /**< Unlock window         */
    MPIDI_OFI_CTRL_UNLOCKALL, /**< Unlock window         */
    MPIDI_OFI_CTRL_UNLOCKALLACK,
    /**< Unlock window         */
    MPIDI_OFI_CTRL_COMPLETE,  /**< End a START epoch     */
    MPIDI_OFI_CTRL_POST,      /**< Begin POST epoch      */
    MPIDI_OFI_CTRL_HUGE,      /**< Huge message          */
    MPIDI_OFI_CTRL_HUGEACK    /**< Huge message ack      */
    /**< Huge message cleanup  */
};

enum {
    MPIDI_OFI_EVENT_ABORT,
    MPIDI_OFI_EVENT_SEND,
    MPIDI_OFI_EVENT_RECV,
    MPIDI_OFI_EVENT_RMA_DONE,
    MPIDI_OFI_EVENT_AM_SEND,
    MPIDI_OFI_EVENT_AM_RECV,
    MPIDI_OFI_EVENT_AM_READ,
    MPIDI_OFI_EVENT_PEEK,
    MPIDI_OFI_EVENT_RECV_HUGE,
    MPIDI_OFI_EVENT_RECV_PACK,
    MPIDI_OFI_EVENT_RECV_NOPACK,
    MPIDI_OFI_EVENT_SEND_HUGE,
    MPIDI_OFI_EVENT_SEND_PACK,
    MPIDI_OFI_EVENT_SEND_NOPACK,
    MPIDI_OFI_EVENT_SSEND_ACK,
    MPIDI_OFI_EVENT_GET_HUGE,
    MPIDI_OFI_EVENT_CHUNK_DONE,
    MPIDI_OFI_EVENT_INJECT_EMU,
    MPIDI_OFI_EVENT_DYNPROC_DONE,
    MPIDI_OFI_EVENT_ACCEPT_PROBE
};

enum {
    MPIDI_OFI_REQUEST_LOCK,
    MPIDI_OFI_REQUEST_LOCKALL
};

enum {
    MPIDI_OFI_PEEK_START,
    MPIDI_OFI_PEEK_NOT_FOUND,
    MPIDI_OFI_PEEK_FOUND
};

enum {
    MPIDI_OFI_DYNPROC_DISCONNECTED = 0,
    MPIDI_OFI_DYNPROC_LOCAL_DISCONNECTED_CHILD,
    MPIDI_OFI_DYNPROC_LOCAL_DISCONNECTED_PARENT,
    MPIDI_OFI_DYNPROC_CONNECTED_CHILD,
    MPIDI_OFI_DYNPROC_CONNECTED_PARENT
};

typedef struct {
    char pad[MPIDI_REQUEST_HDR_SIZE];
    struct fi_context context[MPIDI_OFI_CONTEXT_STRUCTS];       /* fixed field, do not move */
    int event_id;               /* fixed field, do not move */
    int index;
} MPIDI_OFI_am_repost_request_t;

typedef struct {
    char pad[MPIDI_REQUEST_HDR_SIZE];
    struct fi_context context[MPIDI_OFI_CONTEXT_STRUCTS];       /* fixed field, do not move */
    int event_id;               /* fixed field, do not move */
    MPIR_Request *signal_req;
} MPIDI_OFI_ssendack_request_t;

typedef struct {
    char pad[MPIDI_REQUEST_HDR_SIZE];
    struct fi_context context[MPIDI_OFI_CONTEXT_STRUCTS];       /* fixed field, do not move */
    int event_id;               /* fixed field, do not move */
    int done;
    uint32_t tag;
    uint32_t source;
    uint64_t msglen;
} MPIDI_OFI_dynamic_process_request_t;

typedef struct {
    uint8_t op;
    uint8_t dt;
    unsigned atomic_valid:2;
    unsigned fetch_atomic_valid:2;
    unsigned compare_atomic_valid:2;
    unsigned dtsize:10;
    uint64_t max_atomic_count;
    uint64_t max_compare_atomic_count;
    uint64_t max_fetch_atomic_count;
    bool mpi_acc_valid;
} MPIDI_OFI_atomic_valid_t;

typedef struct {
    struct fid_ep *tx;
    struct fid_ep *rx;
    struct fid_cq *cq;
} MPIDI_OFI_context_t;

typedef union {
    MPID_Thread_mutex_t m;
    char cacheline[MPIDI_OFI_CACHELINE_SIZE];
} MPIDI_OFI_cacheline_mutex_t MPL_ATTR_ALIGNED(MPIDI_OFI_CACHELINE_SIZE);

typedef struct MPIDI_OFI_cq_list_t {
    struct fi_cq_tagged_entry cq_entry;
    fi_addr_t source;
    struct MPIDI_OFI_cq_list_t *next;
} MPIDI_OFI_cq_list_t;

typedef struct {
    struct fi_cq_tagged_entry cq_entry;
} MPIDI_OFI_cq_buff_entry_t;

typedef struct {
    unsigned enable_data:1;
    unsigned enable_av_table:1;
    unsigned enable_scalable_endpoints:1;
    unsigned enable_shared_contexts:1;
    unsigned enable_mr_scalable:1;
    unsigned enable_tagged:1;
    unsigned enable_am:1;
    unsigned enable_rma:1;
    unsigned enable_atomics:1;
    unsigned enable_pt2pt_nopack:1;
    unsigned enable_data_auto_progress:1;
    unsigned enable_control_auto_progress:1;

    int max_endpoints;
    int max_endpoints_bits;

    int fetch_atomic_iovecs;

    int context_bits;
    int source_bits;
    int tag_bits;
    int major_version;
    int minor_version;
    int num_am_buffers;
} MPIDI_OFI_capabilities_t;

typedef struct {
    fi_addr_t dest;
    int rank;
    int state;
} MPIDI_OFI_conn_t;

typedef struct MPIDI_OFI_conn_manager_t {
    int mmapped_size;           /* Size of the connection list memory which is mmapped */
    int max_n_conn;             /* Maximum number of connections up to this point */
    int n_conn;                 /* Current number of open connections */
    int next_conn_id;           /* The next connection id to be used. */
    int *free_conn_id;          /* The list of the next connection id to be used so we
                                 * can garbage collect as we go. */
    MPIDI_OFI_conn_t *conn_list;        /* The list of connection structs to track the
                                         * outstanding dynamic process connections. */
} MPIDI_OFI_conn_manager_t;

/* Global state data */
#define MPIDI_KVSAPPSTRLEN 1024
typedef struct {
    /* OFI objects */
    int avtid;
    struct fi_info *prov_use;
    struct fid_domain *domain;
    struct fid_fabric *fabric;
    struct fid_av *av;
    struct fid_ep *ep;
    struct fid_cq *p2p_cq;
    struct fid_cntr *rma_cmpl_cntr;
    struct fid_stx *rma_stx_ctx;        /* shared TX context for RMA */
    struct fid_ep *rma_sep;     /* dedicated scalable EP for RMA */

    /* Queryable limits */
    uint64_t max_buffered_send;
    uint64_t max_buffered_write;
    uint64_t max_send;
    uint64_t max_write;
    uint64_t max_short_send;
    uint64_t max_mr_key_size;
    uint64_t max_rma_key_bits;
    uint64_t max_huge_rmas;
    int rma_key_type_bits;
    int context_shift;
    size_t tx_iov_limit;
    size_t rx_iov_limit;
    size_t rma_iov_limit;
    int max_ch4_vnis;
    int max_rma_sep_tx_cnt;     /* Max number of transmit context on one RMA scalable EP */
    size_t max_order_raw;
    size_t max_order_war;
    size_t max_order_waw;

    /* Mutexex and endpoints */
    MPIDI_OFI_cacheline_mutex_t mutexes[4];
#ifdef MPIDI_OFI_ENABLE_RUNTIME_CHECKS
    MPIDI_OFI_context_t ctx[MPIDI_OFI_MAX_ENDPOINTS_SCALABLE];
#else
    MPIDI_OFI_context_t ctx[MPIDI_OFI_MAX_ENDPOINTS];
#endif

    /* Window/RMA Globals */
    void *win_map;
    uint64_t rma_issued_cntr;
    /* OFI atomics limitation of each pair of <dtype, op> returned by the
     * OFI provider at MPI initialization.*/
    MPIDI_OFI_atomic_valid_t win_op_table[MPIDI_OFI_DT_SIZES][MPIDI_OFI_OP_SIZES];
    UT_array *rma_sep_idx_array;        /* Array of available indexes of transmit contexts on sep */

    /* Active Message Globals */
    struct iovec am_iov[MPIDI_OFI_MAX_NUM_AM_BUFFERS] MPL_ATTR_ALIGNED(MPIDI_OFI_IOVEC_ALIGN);
    struct fi_msg am_msg[MPIDI_OFI_MAX_NUM_AM_BUFFERS];
    void *am_bufs[MPIDI_OFI_MAX_NUM_AM_BUFFERS];
    MPIDI_OFI_am_repost_request_t am_reqs[MPIDI_OFI_MAX_NUM_AM_BUFFERS];
    MPIU_buf_pool_t *am_buf_pool;
    OPA_int_t am_inflight_inject_emus;
    OPA_int_t am_inflight_rma_send_mrs;

    /* Completion queue buffering */
    MPIDI_OFI_cq_buff_entry_t cq_buffered_static_list[MPIDI_OFI_NUM_CQ_BUFFERED];
    int cq_buffered_static_head;
    int cq_buffered_static_tail;
    MPIDI_OFI_cq_list_t *cq_buffered_dynamic_head, *cq_buffered_dynamic_tail;

    /* Process management and PMI globals */
    int pname_set;
    int pname_len;
    char addrname[FI_NAME_MAX];
    size_t addrnamelen;
    char kvsname[MPIDI_KVSAPPSTRLEN];
    char pname[MPI_MAX_PROCESSOR_NAME];
    int port_name_tag_mask[MPIR_MAX_CONTEXT_MASK];

    /* Communication info for dynamic processes */
    MPIDI_OFI_conn_manager_t conn_mgr;

    /* complete request used for lightweight sends */
    MPIR_Request *lw_send_req;

    /* Capability settings */
#ifdef MPIDI_OFI_ENABLE_RUNTIME_CHECKS
    MPIDI_OFI_capabilities_t settings;
#endif
} MPIDI_OFI_global_t;

typedef struct {
    uint32_t index;
} MPIDI_OFI_datatype_t;

typedef struct {
    int16_t type;
    int16_t seqno;
    int origin_rank;
    MPIR_Request *ackreq;
    uintptr_t send_buf;
    size_t msgsize;
    int comm_id;
    int endpoint_id;
    uint64_t rma_key;
    int tag;
} MPIDI_OFI_send_control_t;

typedef struct {
    MPIR_OBJECT_HEADER;
    void *pad;
} MPIDI_OFI_offset_checker_t;

typedef struct MPIDI_OFI_seg_state {
    DLOOP_Count buf_limit;      /* Maximum data size in bytes which a single OFI call can handle.
                                 * This value remains constant once seg_state is initialized. */
    DLOOP_Count buf_limit_left; /* Buffer length left for a single OFI call */

    MPIR_Segment origin_seg;    /* Segment structure */
    size_t origin_cursor;       /* First byte to pack */
    size_t origin_end;          /* Last byte to pack */
    DLOOP_Count origin_iov_len; /* Length of data actually packed */
    DLOOP_VECTOR origin_iov;    /* IOVEC returned after pack */
    uintptr_t origin_addr;      /* Address of data actually packed */

    MPIR_Segment target_seg;
    size_t target_cursor;
    size_t target_end;
    DLOOP_Count target_iov_len;
    DLOOP_VECTOR target_iov;
    uintptr_t target_addr;

    MPIR_Segment result_seg;
    size_t result_cursor;
    size_t result_end;
    DLOOP_Count result_iov_len;
    DLOOP_VECTOR result_iov;
    uintptr_t result_addr;
} MPIDI_OFI_seg_state_t;

typedef enum MPIDI_OFI_segment_side {
    MPIDI_OFI_SEGMENT_ORIGIN = 0,
    MPIDI_OFI_SEGMENT_TARGET,
    MPIDI_OFI_SEGMENT_RESULT,
} MPIDI_OFI_segment_side_t;

typedef struct MPIDI_OFI_win_acc_hint {
    uint64_t dtypes_max_count[MPIDI_OFI_DT_SIZES];      /* translate CH4 which_accumulate_ops hints to
                                                         * atomicity support of all OFI datatypes. A datatype
                                                         * is supported only when all enabled ops are valid atomic
                                                         * provided by the OFI provider (recored in MPIDI_Global.win_op_table).
                                                         * Invalid <dtype, op> defined in MPI standard are excluded.
                                                         * This structure is prepared at window creation time. */
} MPIDI_OFI_win_acc_hint_t;

typedef struct {
    char pad[MPIDI_REQUEST_HDR_SIZE];
    struct fi_context context[MPIDI_OFI_CONTEXT_STRUCTS];       /* fixed field, do not move */
    int event_id;               /* fixed field, do not move */
    union {
        struct {
            struct iovec *originv;
            struct fi_rma_iov *targetv;
        } put_get;
        struct {
            struct fi_ioc *originv;
            struct fi_rma_ioc *targetv;
            struct fi_ioc *resultv;
            struct fi_ioc *comparev;
        } cas;
        struct {
            struct fi_ioc *originv;
            struct fi_rma_ioc *targetv;
        } accumulate;
        struct {
            struct fi_ioc *originv;
            struct fi_rma_ioc *targetv;
            struct fi_ioc *resultv;
        } get_accumulate;
    } iov;
    char iov_store[0];          /* Flexible array, do not move */
} MPIDI_OFI_win_noncontig_t;

typedef struct MPIDI_OFI_win_request {
    MPIR_OBJECT_HEADER;
    char pad[MPIDI_REQUEST_HDR_SIZE - MPIDI_OFI_OBJECT_HEADER_SIZE];
    struct fi_context context[MPIDI_OFI_CONTEXT_STRUCTS];       /* fixed field, do not move */
    int event_id;               /* fixed field, do not move */
    struct MPIDI_OFI_win_request *next;
    int target_rank;
    MPIDI_OFI_win_noncontig_t *noncontig;
} MPIDI_OFI_win_request_t;

typedef struct {
    char pad[MPIDI_REQUEST_HDR_SIZE];
    struct fi_context context[MPIDI_OFI_CONTEXT_STRUCTS];       /* fixed field, do not move */
    int event_id;               /* fixed field, do not move */
    MPIR_Request *parent;       /* Parent request           */
} MPIDI_OFI_chunk_request;

typedef struct MPIDI_OFI_huge_recv {
    char pad[MPIDI_REQUEST_HDR_SIZE];
    struct fi_context context[MPIDI_OFI_CONTEXT_STRUCTS];       /* fixed field, do not move */
    int event_id;               /* fixed field, do not move */
    int (*done_fn) (struct fi_cq_tagged_entry * wc, MPIR_Request * req, int event_id);
    MPIDI_OFI_send_control_t remote_info;
    size_t cur_offset;
    MPIR_Comm *comm_ptr;
    MPIR_Request *localreq;
    struct fi_cq_tagged_entry wc;
    struct MPIDI_OFI_huge_recv *next;   /* Points to the next entry in the unexpected list
                                         * (when in the unexpected list) */
} MPIDI_OFI_huge_recv_t;

/* The list of posted huge receives that haven't been matched yet. These need
 * to get matched up when handling the control message that starts transfering
 * data from the remote memory region and we need a way of matching up the
 * control messages with the "real" requests. */
typedef struct MPIDI_OFI_huge_recv_list {
    int comm_id;
    int rank;
    int tag;
    MPIR_Request *rreq;
    struct MPIDI_OFI_huge_recv_list *next;
} MPIDI_OFI_huge_recv_list_t;

/* Externs */
extern MPIDI_OFI_global_t MPIDI_Global;
extern MPIDI_OFI_huge_recv_t *MPIDI_unexp_huge_recv_head;
extern MPIDI_OFI_huge_recv_t *MPIDI_unexp_huge_recv_tail;
extern MPIDI_OFI_huge_recv_list_t *MPIDI_posted_huge_recv_head;
extern MPIDI_OFI_huge_recv_list_t *MPIDI_posted_huge_recv_tail;

extern int MPIR_Datatype_init_names(void);
extern MPIDI_OFI_capabilities_t MPIDI_OFI_caps_list[MPIDI_OFI_NUM_SETS];

/* Build uniform mr_key using contextid, key type and uniq ID passed */
MPL_STATIC_INLINE_PREFIX uint64_t MPIDI_OFI_rma_key_pack(MPIR_Context_id_t ctx, int key_type,
                                                         uint64_t key_id)
{
    uint64_t rma_key = 0;

    /* place context_id into mr_key */
    rma_key = ((uint64_t) (MPIR_CONTEXT_READ_FIELD(PREFIX, ctx))) << (MPIDI_Global.context_shift);

    /* set key type bits */
    rma_key |= (((uint64_t) key_type) & (((uint64_t) 1 << MPIDI_Global.rma_key_type_bits) - 1))
        << MPIDI_Global.max_rma_key_bits;

    /* place Key/window instance-ID into mr_key */
    rma_key |= ((uint64_t) key_id) & (MPIDI_Global.max_huge_rmas - 1);

    return rma_key;
}

MPL_STATIC_INLINE_PREFIX void MPIDI_OFI_rma_key_unpack(uint64_t key, MPIR_Context_id_t * ctx,
                                                       int *key_type, uint32_t * key_id)
{
    if (ctx) {
        (*ctx) = (MPIR_Context_id_t) (key >> MPIDI_Global.context_shift);
    }

    (*key_type) =
        (int) ((key >> MPIDI_Global.max_rma_key_bits) &
               (((uint64_t) 1 << MPIDI_Global.rma_key_type_bits) - 1));

    (*key_id) = (uint32_t) (key & (MPIDI_Global.max_huge_rmas - 1));
}

#endif /* OFI_TYPES_H_INCLUDED */
