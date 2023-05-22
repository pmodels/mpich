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
#define MPIDI_OFI_AM_BUFF_SZ               (2 * 1024 * 1024)
#define MPIDI_OFI_IOV_MAX                  (32)
#define MPIDI_OFI_NUM_CQ_BUFFERED          (1024)
#define MPIDI_OFI_STRIPE_CHUNK_SIZE        (2048)       /* First chunk sent through the preferred NIC during striping */

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


/* There are 4 protocol bits:
 * - MPIDI_DYNPROC_SEND
 * - MPIDI_OFI_HUGE_SEND
 * - MPIDI_OFI_SYNC_SEND
 * - MPIDI_OFI_SYNC_SEND_ACK
 * The ssend ack and dynproc send bits need to be included in matching
 * to avoid matching with user messages. Because of this, we only mask
 * the ssend and huge bits. */
#define MPIDI_OFI_PROTOCOL_BITS (4)
#define MPIDI_OFI_PROTOCOL_MASK_BITS (2)

#if MPIDI_OFI_ENABLE_RUNTIME_CHECKS == MPIDI_OFI_ON
#define MPIDI_OFI_SYNC_SEND_ACK      (1ULL << (MPIDI_OFI_CONTEXT_BITS + MPIDI_OFI_SOURCE_BITS + MPIDI_OFI_TAG_BITS))
#define MPIDI_OFI_DYNPROC_SEND       (2ULL << (MPIDI_OFI_CONTEXT_BITS + MPIDI_OFI_SOURCE_BITS + MPIDI_OFI_TAG_BITS))
#define MPIDI_OFI_SYNC_SEND          (4ULL << (MPIDI_OFI_CONTEXT_BITS + MPIDI_OFI_SOURCE_BITS + MPIDI_OFI_TAG_BITS))
#define MPIDI_OFI_HUGE_SEND          (8ULL << (MPIDI_OFI_CONTEXT_BITS + MPIDI_OFI_SOURCE_BITS + MPIDI_OFI_TAG_BITS))
#define MPIDI_OFI_PROTOCOL_MASK      (((1ULL << MPIDI_OFI_PROTOCOL_MASK_BITS) - 1) << (MPIDI_OFI_PROTOCOL_BITS - MPIDI_OFI_PROTOCOL_MASK_BITS) << (MPIDI_OFI_CONTEXT_BITS + MPIDI_OFI_SOURCE_BITS + MPIDI_OFI_TAG_BITS))
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
#define MPIDI_OFI_AM_SREQ_HDR(req,field) ((req)->dev.ch4.am.netmod_am.ofi.sreq_hdr->field)
#define MPIDI_OFI_AM_RREQ_HDR(req,field) ((req)->dev.ch4.am.netmod_am.ofi.rreq_hdr->field)
#define MPIDI_OFI_REQUEST(req,field)       ((req)->dev.ch4.netmod.ofi.field)
#define MPIDI_OFI_AV(av)                   ((av)->netmod.ofi)

#define MPIDI_OFI_COMM(comm)     ((comm)->dev.ch4.netmod.ofi)

#define MPIDI_OFI_NUM_CQ_ENTRIES 8
/* MPICH requires at least 32 bits of space in the completion queue data field. */
#define MPIDI_OFI_MIN_CQ_DATA_SIZE 4

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
    MPIDI_OFI_EVENT_AM_SEND_RDMA,
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
    MPIDI_OFI_EVENT_HUGE_CHUNK_DONE,
    MPIDI_OFI_EVENT_INJECT_EMU,
    MPIDI_OFI_EVENT_DYNPROC_DONE,
    MPIDI_OFI_EVENT_ACCEPT_PROBE
};

enum {
    MPIDI_OFI_PEEK_START,
    MPIDI_OFI_PEEK_NOT_FOUND,
    MPIDI_OFI_PEEK_FOUND
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
    void *pack_buffer;          /* always allocated from pack_buf_pool */
    void *msg_hdr;
    void *am_hdr;
    void *am_data;
    MPIDI_OFI_am_header_t msg_hdr_data MPL_ATTR_ALIGNED(MAX_ALIGNMENT);
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

typedef struct MPIDI_OFI_cq_list_t {
    struct fi_cq_tagged_entry cq_entry;
    fi_addr_t source;
    struct MPIDI_OFI_cq_list_t *next;
} MPIDI_OFI_cq_list_t;

/* global per-vci am related fields */
typedef struct {
    struct iovec am_iov[MPIDI_OFI_MAX_NUM_AM_BUFFERS];
    struct fi_msg am_msg[MPIDI_OFI_MAX_NUM_AM_BUFFERS];
    void *am_bufs;
    MPIDI_OFI_am_repost_request_t am_reqs[MPIDI_OFI_MAX_NUM_AM_BUFFERS];

    MPIDU_genq_private_pool_t am_hdr_buf_pool;

    /* Queue to store defferend am send */
    MPIDI_OFI_deferred_am_isend_req_t *deferred_am_isend_q;

    /* Sequence number trackers for active messages */
    void *am_send_seq_tracker;
    void *am_recv_seq_tracker;

    /* active message trackers */
    int am_inflight_inject_emus;
    int am_inflight_rma_send_mrs;

    /* Queue (utlist) to store early-arrival active messages */
    MPIDI_OFI_am_unordered_msg_t *am_unordered_msgs;

    /* Used by am pipeline to track rreq */
    void *req_map;

    /* Completion queue buffering */
    struct fi_cq_tagged_entry cq_buffered_static_list[MPIDI_OFI_NUM_CQ_BUFFERED];
    int cq_buffered_static_head;
    int cq_buffered_static_tail;
    MPIDI_OFI_cq_list_t *cq_buffered_dynamic_head, *cq_buffered_dynamic_tail;

    /* queues to matching huge recv and control message */
    struct MPIDI_OFI_huge_recv_list *huge_ctrl_head;
    struct MPIDI_OFI_huge_recv_list *huge_ctrl_tail;
    struct MPIDI_OFI_huge_recv_list *huge_recv_head;
    struct MPIDI_OFI_huge_recv_list *huge_recv_tail;

    char pad MPL_ATTR_ALIGNED(MPL_CACHELINE_SIZE);
} MPIDI_OFI_per_vci_t;

typedef struct {
    struct fid_domain *domain;
    struct fid_av *av;
    struct fid_ep *ep;
    struct fid_cntr *rma_cmpl_cntr;
#if defined(MPIDI_CH4_USE_MT_RUNTIME) || defined(MPIDI_CH4_USE_MT_LOCKLESS)
    MPL_atomic_uint64_t rma_issued_cntr;        /* atomic version in support of lockless mt model */
#else
    uint64_t rma_issued_cntr;
#endif
    /* rma using shared TX context */
    struct fid_stx *rma_stx_ctx;
    /* rma using dedicated scalable EP */
    struct fid_ep *rma_sep;
    UT_array *rma_sep_idx_array;        /* Array of available indexes of transmit contexts on sep */

    struct fid_ep *tx;
    struct fid_ep *rx;
    struct fid_cq *cq;
} MPIDI_OFI_context_t;

typedef union {
    MPID_Thread_mutex_t m;
    char cacheline[MPL_CACHELINE_SIZE];
} MPIDI_OFI_cacheline_mutex_t;

typedef struct {
    unsigned enable_data:1;
    unsigned enable_av_table:1;
    unsigned enable_scalable_endpoints:1;
    unsigned enable_shared_contexts:1;
    unsigned enable_mr_virt_address:1;
    unsigned enable_mr_allocated:1;
    unsigned enable_mr_prov_key:1;
    unsigned enable_tagged:1;
    unsigned enable_am:1;
    unsigned enable_rma:1;
    unsigned enable_mr_register_null:1;
    unsigned enable_atomics:1;
    unsigned enable_pt2pt_nopack:1;
    unsigned enable_hmem:1;
    unsigned enable_data_auto_progress:1;
    unsigned enable_control_auto_progress:1;
    unsigned require_rdm:1;

    int max_endpoints;
    int max_endpoints_bits;

    int fetch_atomic_iovecs;

    int context_bits;
    int source_bits;
    int tag_bits;
    int counter_wait_objects;
    int major_version;
    int minor_version;
    int num_am_buffers;
    int num_optimized_memory_regions;
} MPIDI_OFI_capabilities_t;

typedef struct {
    struct fi_info *nic;
    int id;                     /* Unique NIC ID, consistent across intranode ranks */
    int close;                  /* Boolean whether nic is close in terms of affinity */
    int prefer;                 /* Preference to order (lower is more preferred) */
    int count;                  /* Count of ranks which have this NIC as their preferred NIC */
    int num_close_ranks;        /* number of ranks which are close to this NIC */
    MPIR_hwtopo_gid_t parent;   /* Parent topology item of NIC which has affinity mask.
                                 * This is typically the socket above the NIC */
} MPIDI_OFI_nic_info_t;

/* Global state data */
#define MPIDI_KVSAPPSTRLEN 1024
typedef struct {
    /* OFI objects */
    int avtid;
    struct fi_info *prov_use[MPIDI_OFI_MAX_NICS];
    MPIDI_OFI_nic_info_t nic_info[MPIDI_OFI_MAX_NICS];
    struct fid_fabric *fabric;

    int got_named_av;

    /* Queryable limits */
    uint64_t max_buffered_send;
    uint64_t max_buffered_write;
    uint64_t max_msg_size;
    uint64_t stripe_threshold;
    uint64_t max_short_send;
    uint64_t max_mr_key_size;
    uint64_t max_rma_key_bits;
    uint64_t max_huge_rmas;
    int rma_key_type_bits;
    int context_shift;
    MPI_Aint tx_iov_limit;
    MPI_Aint rx_iov_limit;
    MPI_Aint rma_iov_limit;
    int max_rma_sep_tx_cnt;     /* Max number of transmit context on one RMA scalable EP */
    MPI_Aint max_order_raw;
    MPI_Aint max_order_war;
    MPI_Aint max_order_waw;

    /* Mutexes and endpoints */
    MPIDI_OFI_cacheline_mutex_t mutexes[MAX_OFI_MUTEXES];
    MPIDI_OFI_context_t ctx[MPIDI_OFI_MAX_VNIS * MPIDI_OFI_MAX_NICS];
    MPIDI_OFI_per_vci_t per_vci[MPIDI_OFI_MAX_VNIS];
    int num_vcis;
    int num_nics;
    int num_close_nics;
    int num_comms_enabled_striping;     /* Number of active communicators with striping enabled */
    int num_comms_enabled_hashing;      /* Number of active communicators with hashingenabled */
    bool am_bufs_registered;    /* whether active message buffers are GPU registered */

    /* Window/RMA Globals */
    void *win_map;
    /* OFI atomics limitation of each pair of <dtype, op> returned by the
     * OFI provider at MPI initialization.*/
    MPIDI_OFI_atomic_valid_t win_op_table[MPIR_DATATYPE_N_PREDEFINED][MPIDIG_ACCU_NUM_OP];

    /* stores the maximum of last recently used optimized memory region key */
    uint64_t global_max_optimized_mr_key;
    /* stores the maximum of last recently used regular memory region key */
    uint64_t global_max_regular_mr_key;

    /* Process management and PMI globals */
    int pname_set;
    int pname_len;
    char addrname[MPIDI_OFI_MAX_NICS][FI_NAME_MAX];
    size_t addrnamelen;         /* OFI uses the same name length within a provider. */
    char pname[MPI_MAX_PROCESSOR_NAME];
    int port_name_tag_mask[MPIR_MAX_CONTEXT_MASK];

    /* Capability settings */
#ifdef MPIDI_OFI_ENABLE_RUNTIME_CHECKS
    MPIDI_OFI_capabilities_t settings;
#endif
} MPIDI_OFI_global_t;

typedef struct {
    MPIR_Context_id_t comm_id;
    int origin_rank;
    int tag;
    MPIR_Request *ackreq;
    void *send_buf;
    size_t msgsize;
    uint64_t rma_keys[MPIDI_OFI_MAX_NICS];
    int vci_src;
    int vci_dst;
} MPIDI_OFI_huge_remote_info_t;

typedef struct {
    int16_t type;
    union {
        struct {
            MPIDI_OFI_huge_remote_info_t info;
        } huge;
        struct {
            MPIR_Request *ackreq;
        } huge_ack;
    } u;
} MPIDI_OFI_send_control_t;

typedef struct MPIDI_OFI_win_acc_hint {
    uint64_t dtypes_max_count[MPIR_DATATYPE_N_PREDEFINED];      /* translate CH4 which_accumulate_ops hints to
                                                                 * atomicity support of all OFI datatypes. A datatype
                                                                 * is supported only when all enabled ops are valid atomic
                                                                 * provided by the OFI provider (recorded in MPIDI_OFI_global.win_op_table).
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
    int vci_local;
    int vci_target;
    int nic_target;
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

typedef struct MPIDI_OFI_read_chunk {
    char pad[MPIDI_REQUEST_HDR_SIZE];
    struct fi_context context[MPIDI_OFI_CONTEXT_STRUCTS];       /* fixed field, do not move */
    int event_id;               /* fixed field, do not move */
    MPIR_Request *localreq;
    MPIR_cc_t *chunks_outstanding;
} MPIDI_OFI_read_chunk_t;

/* The list of posted huge receives that haven't been matched yet. These need
 * to get matched up when handling the control message that starts transferring
 * data from the remote memory region and we need a way of matching up the
 * control messages with the "real" requests. */
typedef struct MPIDI_OFI_huge_recv_list {
    MPIR_Context_id_t comm_id;
    int rank;
    int tag;
    union {
        MPIDI_OFI_huge_remote_info_t *info;     /* ctrl list */
        MPIR_Request *rreq;     /* recv list */
    } u;
    struct MPIDI_OFI_huge_recv_list *next;
} MPIDI_OFI_huge_recv_list_t;

/* Externs */
extern MPIDI_OFI_global_t MPIDI_OFI_global;

extern MPIDI_OFI_capabilities_t MPIDI_OFI_caps_list[MPIDI_OFI_NUM_SETS];

#endif /* OFI_TYPES_H_INCLUDED */
