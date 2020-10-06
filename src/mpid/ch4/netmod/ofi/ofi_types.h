/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
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
#include "mpidu_genq.h"

#define __SHORT_FILE__                          \
    (strrchr(__FILE__,'/')                      \
     ? strrchr(__FILE__,'/')+1                  \
     : __FILE__                                 \
)

/* TODO: This should come from provider capability */
#define MPIDI_OFI_MAX_VNIS                  16

#define MPIDI_OFI_MAP_NOT_FOUND            ((void*)(-1UL))
#define MPIDI_OFI_DEFAULT_SHORT_SEND_SIZE  (16 * 1024)
#define MPIDI_OFI_MAX_NUM_AM_BUFFERS       (8)
#define MPIDI_OFI_AM_BUFF_SZ               (1 * 1024 * 1024)
#define MPIDI_OFI_CACHELINE_SIZE           (MPL_CACHELINE_SIZE)
#define MPIDI_OFI_IOV_MAX                  (32)
#define MPIDI_OFI_AM_HDR_POOL_CELL_SIZE            (1024)
#define MPIDI_OFI_AM_HDR_POOL_NUM_CELLS_PER_CHUNK   (1024)
#define MPIDI_OFI_AM_HDR_POOL_MAX_NUM_CELLS         (257 * 1024)
#define MPIDI_OFI_NUM_CQ_BUFFERED          (1024)

/* The number of bits in the immediate data field allocated to the source rank. */
#define MPIDI_OFI_IDATA_SRC_BITS (30)
/* The number of bits in the immediate data field allocated to the error propagation. */
#define MPIDI_OFI_IDATA_ERROR_BITS (2)
/* Bit mask for MPIR_ERR_OTHER */
#define MPIDI_OFI_ERR_OTHER (0x1ULL)
/* Bit mask for MPIR_PROC_FAILED */
#define MPIDI_OFI_ERR_PROC_FAILED (0x2ULL)

/* Set the error bits */
MPL_STATIC_INLINE_PREFIX void MPIDI_OFI_idata_set_error_bits(uint64_t * data_field,
                                                             MPIR_Errflag_t errflag)
{
    switch (errflag) {
        case MPIR_ERR_OTHER:
            *data_field = (*data_field) | (MPIDI_OFI_ERR_OTHER << MPIDI_OFI_IDATA_SRC_BITS);
            break;
        case MPIR_ERR_PROC_FAILED:
            *data_field = (*data_field) | (MPIDI_OFI_ERR_PROC_FAILED << MPIDI_OFI_IDATA_SRC_BITS);
            break;
        case MPIR_ERR_NONE:
            /*do nothing */
            break;
    }
}

/* Get the error flag from the OFI data field. */
MPL_STATIC_INLINE_PREFIX int MPIDI_OFI_idata_get_error_bits(uint64_t idata)
{
    if ((idata >> MPIDI_OFI_IDATA_SRC_BITS) & MPIDI_OFI_ERR_OTHER) {
        return MPIR_ERR_OTHER;
    } else if ((idata >> MPIDI_OFI_IDATA_SRC_BITS) & MPIDI_OFI_ERR_PROC_FAILED) {
        return MPIR_ERR_PROC_FAILED;
    } else {
        return MPI_SUCCESS;
    }
}


#define MPIDI_OFI_PROTOCOL_BITS (3)     /* This is set to 3 even though we actually use 4. The ssend
                                         * ack bit needs to live outside the protocol bit space to avoid
                                         * accidentally matching unintended messages. Because of this,
                                         * we shift the PROTOCOL_MASK one extra bit to the left to take
                                         * the place of the empty SSEND_ACK bit. */

#if MPIDI_OFI_ENABLE_RUNTIME_CHECKS == MPIDI_OFI_ON
#define MPIDI_OFI_SYNC_SEND_ACK      (1ULL << (MPIDI_OFI_CONTEXT_BITS + MPIDI_OFI_SOURCE_BITS + MPIDI_OFI_TAG_BITS))
#define MPIDI_OFI_SYNC_SEND          (2ULL << (MPIDI_OFI_CONTEXT_BITS + MPIDI_OFI_SOURCE_BITS + MPIDI_OFI_TAG_BITS))
#define MPIDI_OFI_DYNPROC_SEND       (4ULL << (MPIDI_OFI_CONTEXT_BITS + MPIDI_OFI_SOURCE_BITS + MPIDI_OFI_TAG_BITS))
#define MPIDI_OFI_HUGE_SEND          (8ULL << (MPIDI_OFI_CONTEXT_BITS + MPIDI_OFI_SOURCE_BITS + MPIDI_OFI_TAG_BITS))
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

#define MPIDI_OFI_THREAD_UTIL_MUTEX     MPIDI_OFI_global.mutexes[0].m
#define MPIDI_OFI_THREAD_PROGRESS_MUTEX MPIDI_OFI_global.mutexes[1].m
#define MPIDI_OFI_THREAD_FI_MUTEX       MPIDI_OFI_global.mutexes[2].m
#define MPIDI_OFI_THREAD_SPAWN_MUTEX    MPIDI_OFI_global.mutexes[3].m
#define MAX_OFI_MUTEXES 4

/* Field accessor macros */
#define MPIDI_OFI_AMREQUEST(req,field)     ((req)->dev.ch4.am.netmod_am.ofi.field)
#define MPIDI_OFI_AMREQUEST_HDR(req,field) ((req)->dev.ch4.am.netmod_am.ofi.req_hdr->field)
#define MPIDI_OFI_AMREQUEST_HDR_PTR(req)   ((req)->dev.ch4.am.netmod_am.ofi.req_hdr)
#define MPIDI_OFI_REQUEST(req,field)       ((req)->dev.ch4.netmod.ofi.field)
#define MPIDI_OFI_AV(av)                   ((av)->netmod.ofi)

#define MPIDI_OFI_COMM(comm)     ((comm)->dev.ch4.netmod.ofi)

#define MPIDI_OFI_NUM_CQ_ENTRIES 8

/* Typedefs */
enum {
    MPIDI_OFI_CTRL_HUGE,      /**< Huge message          */
    MPIDI_OFI_CTRL_HUGEACK    /**< Huge message ack      */
    /**< Huge message cleanup  */
};

enum {
    MPIDI_OFI_EVENT_ABORT,
    MPIDI_OFI_EVENT_SEND,
    MPIDI_OFI_EVENT_RECV,
    MPIDI_OFI_EVENT_AM_SEND,
    MPIDI_OFI_EVENT_AM_SEND_PIPELINE,
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

/* chunked request for AM eager/pipeline send operation */
typedef struct MPIDI_OFI_am_send_pipeline_request {
    char pad[MPIDI_REQUEST_HDR_SIZE];
    struct fi_context context[MPIDI_OFI_CONTEXT_STRUCTS];       /* fixed field, do not move */
    int event_id;               /* fixed field, do not move */
    MPIR_Request *sreq;
    void *pack_buffer;
    void *am_hdr;
    uint16_t am_hdr_sz;
    MPIDI_OFI_am_header_t msg_hdr MPL_ATTR_ALIGNED(MAX_ALIGNMENT);
    uint8_t am_hdr_buf[MPIDI_OFI_MAX_AM_HDR_SIZE] MPL_ATTR_ALIGNED(MAX_ALIGNMENT);
    /* FI_ASYNC_IOV requires an iov storage to be alive until a request completes */
    struct iovec iov[3];
} MPIDI_OFI_am_send_pipeline_request_t;

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
    struct fid_domain *domain;
    struct fid_av *av;
    struct fid_ep *ep;
    struct fid_cntr *rma_cmpl_cntr;

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
    unsigned enable_av_table:1;
    unsigned enable_scalable_endpoints:1;
    unsigned enable_shared_contexts:1;
    unsigned enable_mr_scalable:1;
    unsigned enable_mr_virt_address:1;
    unsigned enable_mr_allocated:1;
    unsigned enable_mr_prov_key:1;
    unsigned enable_tagged:1;
    unsigned enable_am:1;
    unsigned enable_rma:1;
    unsigned enable_atomics:1;
    unsigned enable_pt2pt_nopack:1;
    unsigned enable_hmem:1;
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
    struct fid_fabric *fabric;

    struct fid_stx *rma_stx_ctx;        /* shared TX context for RMA */
    struct fid_ep *rma_sep;     /* dedicated scalable EP for RMA */

    int got_named_av;

    /* Queryable limits */
    uint64_t max_buffered_send;
    uint64_t max_buffered_write;
    uint64_t max_msg_size;
    uint64_t max_short_send;
    uint64_t max_mr_key_size;
    uint64_t max_rma_key_bits;
    uint64_t max_huge_rmas;
    int rma_key_type_bits;
    int context_shift;
    size_t tx_iov_limit;
    size_t rx_iov_limit;
    size_t rma_iov_limit;
    int max_rma_sep_tx_cnt;     /* Max number of transmit context on one RMA scalable EP */
    size_t max_order_raw;
    size_t max_order_war;
    size_t max_order_waw;

    /* Mutexes and endpoints */
    MPIDI_OFI_cacheline_mutex_t mutexes[MAX_OFI_MUTEXES];
    MPIDI_OFI_context_t ctx[MPIDI_OFI_MAX_VNIS];
    int num_vnis;

    /* Window/RMA Globals */
    void *win_map;
    uint64_t rma_issued_cntr;
    /* OFI atomics limitation of each pair of <dtype, op> returned by the
     * OFI provider at MPI initialization.*/
    MPIDI_OFI_atomic_valid_t win_op_table[MPIR_DATATYPE_N_PREDEFINED][MPIDIG_ACCU_NUM_OP];
    UT_array *rma_sep_idx_array;        /* Array of available indexes of transmit contexts on sep */

    /* Active Message Globals */
    struct iovec am_iov[MPIDI_OFI_MAX_NUM_AM_BUFFERS];
    struct fi_msg am_msg[MPIDI_OFI_MAX_NUM_AM_BUFFERS];
    void *am_bufs[MPIDI_OFI_MAX_NUM_AM_BUFFERS];
    MPIDI_OFI_am_repost_request_t am_reqs[MPIDI_OFI_MAX_NUM_AM_BUFFERS];
    MPIDU_genq_private_pool_t am_hdr_buf_pool;
    MPL_atomic_int_t am_inflight_inject_emus;
    MPL_atomic_int_t am_inflight_rma_send_mrs;
    /* Sequence number trackers for active messages */
    void *am_send_seq_tracker;
    void *am_recv_seq_tracker;
    /* Queue (utlist) to store early-arrival active messages */
    MPIDI_OFI_am_unordered_msg_t *am_unordered_msgs;

    /* Pack buffers for various communication */
    MPIDU_genq_private_pool_t pack_buf_pool;

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
    char pname[MPI_MAX_PROCESSOR_NAME];
    int port_name_tag_mask[MPIR_MAX_CONTEXT_MASK];

    /* Communication info for dynamic processes */
    MPIDI_OFI_conn_manager_t conn_mgr;

    void *req_map;
    MPIDI_OFI_deferred_am_isend_req_t *deferred_am_isend_q;

    /* Capability settings */
#ifdef MPIDI_OFI_ENABLE_RUNTIME_CHECKS
    MPIDI_OFI_capabilities_t settings;
#endif
} MPIDI_OFI_global_t;

typedef struct {
    int16_t type;
    int16_t seqno;
    int origin_rank;
    MPIR_Request *ackreq;
    uintptr_t send_buf;
    size_t msgsize;
    int comm_id;
    uint64_t rma_key;
    int tag;
    int vni_src;
    int vni_dst;
} MPIDI_OFI_send_control_t;

typedef struct MPIDI_OFI_win_acc_hint {
    uint64_t dtypes_max_count[MPIR_DATATYPE_N_PREDEFINED];      /* translate CH4 which_accumulate_ops hints to
                                                                 * atomicity support of all OFI datatypes. A datatype
                                                                 * is supported only when all enabled ops are valid atomic
                                                                 * provided by the OFI provider (recored in MPIDI_OFI_global.win_op_table).
                                                                 * Invalid <dtype, op> defined in MPI standard are excluded.
                                                                 * This structure is prepared at window creation time. */
} MPIDI_OFI_win_acc_hint_t;

enum {
    MPIDI_OFI_PUT,
    MPIDI_OFI_GET,
    MPIDI_OFI_ACC,
    MPIDI_OFI_GET_ACC,
};

typedef struct MPIDI_OFI_pack_chunk {
    struct MPIDI_OFI_pack_chunk *next;
    void *pack_buffer;
    MPI_Aint unpack_size;
    MPI_Aint unpack_offset;
} MPIDI_OFI_pack_chunk;

typedef struct MPIDI_OFI_win_request {
    struct MPIDI_OFI_win_request *next;
    struct MPIDI_OFI_win_request *prev;
    int rma_type;
    MPIR_Request **sigreq;
    MPIDI_OFI_pack_chunk *chunks;
    union {
        struct {
            struct {
                const void *addr;
                int count;
                MPI_Datatype datatype;
                MPI_Aint total_bytes;
                MPI_Aint pack_offset;
            } origin;
            struct {
                void *base;
                int count;
                MPI_Datatype datatype;
                struct iovec *iov;
                MPI_Aint iov_len;
                MPI_Aint total_iov_len;
                MPI_Aint iov_offset;
                MPI_Aint iov_cur;
                MPIDI_av_entry_t *addr;
                uint64_t key;
            } target;
        } put;
        struct {
            struct {
                void *addr;
                int count;
                MPI_Datatype datatype;
                MPI_Aint total_bytes;
                MPI_Aint pack_offset;
            } origin;
            struct {
                void *base;
                int count;
                MPI_Datatype datatype;
                struct iovec *iov;
                MPI_Aint iov_len;
                MPI_Aint total_iov_len;
                MPI_Aint iov_offset;
                MPI_Aint iov_cur;
                MPIDI_av_entry_t *addr;
                uint64_t key;
            } target;
        } get;
    } noncontig;
} MPIDI_OFI_win_request_t;

typedef struct {
    char pad[MPIDI_REQUEST_HDR_SIZE];
    struct fi_context context[MPIDI_OFI_CONTEXT_STRUCTS];       /* fixed field, do not move */
    int event_id;               /* fixed field, do not move */
    MPIR_Request *parent;       /* Parent request           */
} MPIDI_OFI_chunk_request;

typedef struct MPIDI_OFI_target_mr {
    uint64_t addr;              /* Unlikely used. Needed for calculating relative offset
                                 * only when FI_MR_VIRT_ADDRESS is off but registration with MPI_BOTTOM fails
                                 * at win creation. However, we always store it for code simplicity. */
    uint64_t mr_key;
} MPIDI_OFI_target_mr_t;

typedef struct MPIDI_OFI_huge_recv {
    char pad[MPIDI_REQUEST_HDR_SIZE];
    struct fi_context context[MPIDI_OFI_CONTEXT_STRUCTS];       /* fixed field, do not move */
    int event_id;               /* fixed field, do not move */
    int (*done_fn) (struct fi_cq_tagged_entry * wc, MPIR_Request * req, int event_id);
    MPIDI_OFI_send_control_t remote_info;
    bool peek;                  /* Flag to indicate whether this struct has been created to track an uncompleted peek
                                 * operation. */
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
extern MPIDI_OFI_global_t MPIDI_OFI_global;
extern MPIDI_OFI_huge_recv_t *MPIDI_unexp_huge_recv_head;
extern MPIDI_OFI_huge_recv_t *MPIDI_unexp_huge_recv_tail;
extern MPIDI_OFI_huge_recv_list_t *MPIDI_posted_huge_recv_head;
extern MPIDI_OFI_huge_recv_list_t *MPIDI_posted_huge_recv_tail;

extern MPIDI_OFI_capabilities_t MPIDI_OFI_caps_list[MPIDI_OFI_NUM_SETS];

#endif /* OFI_TYPES_H_INCLUDED */
