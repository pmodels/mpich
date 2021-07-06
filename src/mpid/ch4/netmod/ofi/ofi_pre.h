/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
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
#include <rdma/fi_trigger.h>
#include "ofi_capability_sets.h"

/* Defines */

/* configure option --enable-ofi-domain */
#ifndef MPIDI_OFI_VNI_USE_DOMAIN
#define MPIDI_OFI_VNI_USE_SEPCTX       1
#endif

#define MPIDI_OFI_MAX_AM_HDR_SIZE      ((1 << MPIDI_OFI_AM_HDR_SZ_BITS) - 1)
#define MPIDI_OFI_AM_HANDLER_ID_BITS   8
#define MPIDI_OFI_AM_TYPE_BITS         8
#define MPIDI_OFI_AM_HDR_SZ_BITS       8
#define MPIDI_OFI_AM_PAYLOAD_SZ_BITS  24
#define MPIDI_OFI_AM_RANK_BITS        32
#define MPIDI_OFI_AM_MSG_HEADER_SIZE (sizeof(MPIDI_OFI_am_header_t))

/* Typedefs */

struct MPIR_Comm;
struct MPIR_Request;

typedef struct {
    int dummy;
} MPIDI_OFI_Global_t;

typedef struct {
    /* support for connection */
    int conn_id;
    int enable_striping;        /* Flag to enable striping per communicator. */
    int enable_hashing;         /* Flag to enable hashing per communicator. */
    int *pref_nic;              /* Array to specify the preferred NIC for each rank (if needed) */
} MPIDI_OFI_comm_t;
enum {
    MPIDI_AMTYPE_NONE = 0,
    MPIDI_AMTYPE_SHORT_HDR,
    MPIDI_AMTYPE_SHORT,
    MPIDI_AMTYPE_PIPELINE,
    MPIDI_AMTYPE_RDMA_READ
};

typedef enum {
    MPIDI_OFI_DEFERRED_AM_OP__ISEND_EAGER,
    MPIDI_OFI_DEFERRED_AM_OP__ISEND_PIPELINE,
    MPIDI_OFI_DEFERRED_AM_OP__ISEND_RDMA_READ
} MPIDI_OFI_deferred_am_op_e;

typedef struct {
    /* context id and src rank so the target side can
     * issue RDMA read operation */
    MPIR_Context_id_t context_id;
    int src_rank;

    uint64_t src_offset;
    MPIR_Request *sreq_ptr;
    uint64_t am_hdr_src;
    uint64_t rma_key;
    MPI_Aint reg_sz;
} MPIDI_OFI_lmt_msg_payload_t;

typedef struct {
    MPIR_Request *sreq_ptr;
} MPIDI_OFI_am_rdma_read_ack_msg_t;

typedef struct MPIDI_OFI_am_header_t {
    uint64_t handler_id:MPIDI_OFI_AM_HANDLER_ID_BITS;
    uint64_t am_type:MPIDI_OFI_AM_TYPE_BITS;
    uint64_t am_hdr_sz:MPIDI_OFI_AM_HDR_SZ_BITS;
    uint64_t payload_sz:MPIDI_OFI_AM_PAYLOAD_SZ_BITS;   /* data size on this OFI message. This
                                                         * could be the size of a pipeline segment
                                                         * */
    /* vcis are needed for callbacks and to reply.
     * Note: technically the vci_dst don't need be transported since the receiver
     * always know which vci it receives the message. However, having both of them
     * in the header makes the design symmetric and thus easier to maintain.
     */
    uint8_t vci_src;
    uint8_t vci_dst;
    uint16_t seqno;             /* Sequence number of this message. Number is unique between
                                 * (src_rank, src_vci) and (dst_rank, dst_vci) */
    uint64_t src_id;            /* needed for destination to track seqno, combines
                                 * communicator context_id, rank, and vci */
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
    MPIDI_OFI_lmt_msg_payload_t pyld;
} MPIDI_OFI_lmt_msg_t;

typedef struct {
    uint64_t rma_key;
    void *unpack_buffer;
    MPI_Aint pack_size;
    uint64_t src_offset;
    MPIR_Context_id_t context_id;
    int src_rank;
} MPIDI_OFI_lmt_unpack_t;

typedef enum {
    MPIDI_OFI_AM_LMT_IOV,
    MPIDI_OFI_AM_LMT_UNPACK
} MPIDI_OFI_lmt_type_t;

typedef struct {
    struct fid_mr *lmt_mr;
    MPIDI_OFI_lmt_type_t lmt_type;
    union {
        uint64_t lmt_cntr;
        MPIDI_OFI_lmt_unpack_t unpack;
    } lmt_u;
    MPIR_Request *rreq_ptr;
    void *am_hdr;
    uint16_t am_hdr_sz;
    /* used for packing non-contig data or the whole am message when payload doesn't fit */
    void *pack_buffer;
    /* FI_ASYNC_IOV requires an iov storage to be alive until a request completes */
    struct iovec iov[3];
    /* AM send buffers, must be together so we can send without sendv.
     * Note: since we allocate from genq pool, there may be some additional space
     * to pack a small payload */
    MPIDI_OFI_am_header_t msg_hdr;
    uint8_t am_hdr_buf[MPIDI_OFI_MAX_AM_HDR_SIZE];
} MPIDI_OFI_am_request_header_t;

#define MPIDI_OFI_AM_HDR_POOL_CELL_SIZE             (1024)
#define MPIDI_OFI_AM_HDR_POOL_NUM_CELLS_PER_CHUNK   (1024)
/* maximum am message size that can fit into request header buffer */
#define MPIDI_OFI_AM_MAX_MSG_SIZE \
    (MPIDI_OFI_AM_HDR_POOL_CELL_SIZE - offsetof(MPIDI_OFI_am_request_header_t, msg_hdr))

typedef struct MPIDI_OFI_deferred_am_isend_req {
    int op;
    int rank;
    MPIR_Comm *comm;
    int handler_id;
    const void *buf;
    size_t count;
    MPI_Datatype datatype;
    MPIR_Request *sreq;
    bool need_packing;
    MPI_Aint data_sz;
    int vci_src;
    int vci_dst;

    struct MPIDI_OFI_deferred_am_isend_req *prev;
    struct MPIDI_OFI_deferred_am_isend_req *next;
} MPIDI_OFI_deferred_am_isend_req_t;

typedef struct {
    struct fi_context context[MPIDI_OFI_CONTEXT_STRUCTS];       /* fixed field, do not move */
    int event_id;               /* fixed field, do not move */
    MPIDI_OFI_am_request_header_t *sreq_hdr;
    MPIDI_OFI_am_request_header_t *rreq_hdr;
    MPIDI_OFI_deferred_am_isend_req_t *deferred_req;    /* saving information when an AM isend is
                                                         * deferred */
    uint8_t am_type_choice;     /* save amtype to avoid double checking */
    MPI_Aint data_sz;           /* save data_sz to avoid double checking */
} MPIDI_OFI_am_request_t;

enum MPIDI_OFI_req_kind {
    MPIDI_OFI_req_kind__any,
    MPIDI_OFI_req_kind__probe,
    MPIDI_OFI_req_kind__mprobe,
};

typedef struct {
    struct fi_context context[MPIDI_OFI_CONTEXT_STRUCTS];       /* fixed field, do not move */
    int event_id;               /* fixed field, do not move */
    MPL_atomic_int_t util_id;
    MPI_Datatype datatype;
    int nic_num;                /* Store the nic number so we can use it to cancel a request later
                                 * if needed. */
    enum MPIDI_OFI_req_kind kind;
    union {
        struct fid_mr **send_mrs;
        void *remote_info;
    } huge;
    union {
        struct {
            void *buf;
            size_t count;
            MPI_Datatype datatype;
            char *pack_buffer;
        } pack;
        struct iovec *nopack;
    } noncontig;
    union {
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

typedef struct {
    int dummy;
} MPIDI_OFI_part_t;

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
#if defined(MPIDI_CH4_USE_MT_RUNTIME) || defined(MPIDI_CH4_USE_MT_LOCKLESS)
    MPL_atomic_uint64_t *issued_cntr;   /* atomic counter in support of lockless and runtime mt models */
#else
    uint64_t *issued_cntr;
#endif
#if defined(MPIDI_CH4_USE_MT_RUNTIME) || defined(MPIDI_CH4_USE_MT_LOCKLESS)
    MPL_atomic_uint64_t issued_cntr_v;  /* atomic counter in support of lockless and runtime mt models */
#else
    uint64_t issued_cntr_v;     /* main body of an issued counter,
                                 * if we are to use per-window counter */
#endif

    struct fid_cntr *cmpl_cntr;
    uint64_t win_id;
    struct MPIDI_OFI_win_request *syncQ;
    struct MPIDI_OFI_win_request *deferredQ;
    MPIDI_OFI_win_targetinfo_t *winfo;

    MPL_gavl_tree_t *dwin_target_mrs;   /* MR key and address pairs registered to remote processes.
                                         * One AVL tree per process. */
    MPL_gavl_tree_t dwin_mrs;   /* Single AVL tree to store locally attached MRs */

    /* Accumulate related info. The struct internally uses MPIR_DATATYPE_N_PREDEFINED
     * defined in ofi_types.h to allocate the max_count array. The struct
     * size is unknown when we load ofi_pre.h, thus we only set a pointer here. */
    struct MPIDI_OFI_win_acc_hint *acc_hint;

    /* Counter to track whether or not to kick the progress engine when the OFI provider does not
     * supply automatic progress. This can make a big performance difference when doing
     * large, non-contiguous RMA operations. */
    int progress_counter;
} MPIDI_OFI_win_t;

/* Maximum number of network interfaces CH4 can support. */
#define MPIDI_OFI_MAX_NICS 8

typedef struct {
#ifdef MPIDI_OFI_VNI_USE_DOMAIN
    fi_addr_t dest[MPIDI_OFI_MAX_NICS][MPIDI_CH4_MAX_VCIS];     /* [nic][vci] */
#else
    fi_addr_t dest[MPIDI_OFI_MAX_NICS][1];
#endif
} MPIDI_OFI_addr_t;

#endif /* OFI_PRE_H_INCLUDED */
