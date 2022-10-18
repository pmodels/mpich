/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef MPIDPRE_H_INCLUDED
#define MPIDPRE_H_INCLUDED

#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#ifdef HAVE_HCOLL
#include "hcoll/api/hcoll_dte.h"
#endif

#include "mpid_thread.h"
#include "mpid_sched.h"
#include "netmodpre.h"
#ifndef MPIDI_CH4_DIRECT_NETMOD
#include "shmpre.h"
#endif
#include "uthash.h"
#include "ch4_csel_container.h"

enum {
    MPIDI_CH4_MT_DIRECT,
    MPIDI_CH4_MT_LOCKLESS,

    MPIDI_CH4_NUM_MT_MODELS,
};

#ifdef MPIDI_CH4_USE_MT_DIRECT
#define MPIDI_CH4_MT_MODEL MPIDI_CH4_MT_DIRECT
#elif defined MPIDI_CH4_USE_MT_LOCKLESS
#define MPIDI_CH4_MT_MODEL MPIDI_CH4_MT_LOCKLESS
#elif defined MPIDI_CH4_USE_MT_RUNTIME
#define MPIDI_CH4_MT_MODEL MPIDI_global.settings.mt_model
#else
#error "Unknown MT model or MT model not defined"
#endif

typedef struct {
#ifdef HAVE_HCOLL
    hcoll_datatype_t hcoll_datatype;
#endif
    union {
    MPIDI_NM_DT_DECL} netmod;
} MPIDI_Devdt_t;
#define MPID_DEV_DATATYPE_DECL   MPIDI_Devdt_t   dev;

typedef struct {
    int flag;
    int progress_made;
    int vci_count;              /* number of vcis that need progress */
    int progress_counts[MPIDI_CH4_MAX_VCIS];
    uint8_t vci[MPIDI_CH4_MAX_VCIS];    /* list of vcis that need progress */
} MPID_Progress_state;

typedef enum {
    MPIDI_PTYPE_RECV,
    MPIDI_PTYPE_SEND,
    MPIDI_PTYPE_BSEND,
    MPIDI_PTYPE_SSEND
} MPIDI_ptype;

#define MPIDIG_REQ_BUSY           (0x1)
#define MPIDIG_REQ_PEER_SSEND     (0x1 << 1)
#define MPIDIG_REQ_UNEXPECTED     (0x1 << 2)
#define MPIDIG_REQ_UNEXP_DQUED    (0x1 << 3)
#define MPIDIG_REQ_UNEXP_CLAIMED  (0x1 << 4)
#define MPIDIG_REQ_RCV_NON_CONTIG (0x1 << 5)
#define MPIDIG_REQ_MATCHED (0x1 << 6)
#define MPIDIG_REQ_RTS (0x1 << 7)
#define MPIDIG_REQ_IN_PROGRESS (0x1 << 8)

#define MPIDI_PARENT_PORT_KVSKEY "PARENT_ROOT_PORT_NAME"
#define MPIDI_MAX_KVS_VALUE_LEN  4096

typedef struct MPIDIG_rreq_t {
    /* mrecv fields */
    void *mrcv_buffer;
    uint64_t mrcv_count;
    MPI_Datatype mrcv_datatype;

    MPIR_Request *peer_req_ptr;
    MPIR_Request *match_req;
} MPIDIG_rreq_t;

typedef struct MPIDIG_part_am_req_t {
    MPIR_Request *part_req_ptr;
} MPIDIG_part_am_req_t;

typedef struct MPIDIG_put_req_t {
    MPIR_Request *preq_ptr;
    void *flattened_dt;
    MPI_Aint origin_data_sz;
} MPIDIG_put_req_t;

typedef struct MPIDIG_get_req_t {
    MPIR_Request *greq_ptr;
    void *flattened_dt;
} MPIDIG_get_req_t;

typedef struct MPIDIG_cswap_req_t {
    MPIR_Request *creq_ptr;
    void *data;
} MPIDIG_cswap_req_t;

typedef struct MPIDIG_acc_req_t {
    MPIR_Request *req_ptr;
    MPI_Datatype origin_datatype;
    MPI_Datatype target_datatype;
    MPI_Aint origin_count;
    MPI_Aint target_count;
    void *target_addr;
    void *flattened_dt;
    void *data;                 /* for origin_data received and result_data being sent (in GET_ACC).
                                 * Not to be confused with result_addr below which saves the
                                 * result_addr parameter. */
    MPI_Aint result_data_sz;    /* only used in GET_ACC */
    void *result_addr;
    MPI_Aint result_count;
    void *origin_addr;
    MPI_Datatype result_datatype;
    MPI_Op op;
} MPIDIG_acc_req_t;

typedef int (*MPIDIG_req_cmpl_cb) (MPIR_Request * req);

/* structure used for supporting asynchronous payload transfer */
typedef enum {
    MPIDIG_RECV_NONE,
    MPIDIG_RECV_DATATYPE,       /* use the datatype info in MPIDIG_req_t */
    MPIDIG_RECV_CONTIG,         /* set and use the contig recv-buffer info */
    MPIDIG_RECV_IOV             /* set and use the iov recv-buffer info */
} MPIDIG_recv_type;

typedef int (*MPIDIG_recv_data_copy_cb) (MPIR_Request * req);

typedef struct MPIDIG_req_async {
    MPIDIG_recv_type recv_type;
    bool is_device_buffer;
    MPI_Aint in_data_sz;
    MPI_Aint offset;
    struct iovec *iov_ptr;      /* used with MPIDIG_RECV_IOV */
    int iov_num;                /* used with MPIDIG_RECV_IOV */
    struct iovec iov_one;       /* used with MPIDIG_RECV_CONTIG */
    MPIDIG_recv_data_copy_cb data_copy_cb;      /* called in recv_init/recv_type_init for async
                                                 * data copying */
} MPIDIG_rreq_async_t;

typedef struct MPIDIG_sreq_async {
    MPI_Datatype datatype;
    MPI_Aint data_sz_left;
    MPI_Aint offset;
    int seg_issued;
    int seg_completed;
} MPIDIG_sreq_async_t;

typedef struct MPIDIG_req_ext_t {
    union {
        MPIDIG_rreq_t rreq;
        MPIDIG_put_req_t preq;
        MPIDIG_get_req_t greq;
        MPIDIG_cswap_req_t creq;
        MPIDIG_acc_req_t areq;
        MPIDIG_part_am_req_t part_am_req;
    };

    MPIDIG_rreq_async_t recv_async;
    MPIDIG_sreq_async_t send_async;
    uint8_t local_vci;
    uint8_t remote_vci;
    struct iovec *iov;
    MPIDIG_req_cmpl_cb target_cmpl_cb;
    uint64_t seq_no;
    MPIR_Request *request;
    uint64_t status;
    struct MPIDIG_req_ext_t *next, *prev;

} MPIDIG_req_ext_t;

typedef struct MPIDIG_req_t {
    union {
    MPIDI_NM_REQUEST_AM_DECL} netmod_am;
#ifndef MPIDI_CH4_DIRECT_NETMOD
    struct {
    MPIDI_SHM_REQUEST_AM_DECL} shm_am;
#endif
    MPIDIG_req_ext_t *req;
    void *rndv_hdr;
    void *buffer;
    MPI_Aint count;
    MPI_Datatype datatype;
    union {
        struct {
            int dest;
        } send;
        struct {
            int source;
            MPIR_Context_id_t context_id;
            int tag;
        } recv;
        struct {
            int target_rank;
            MPI_Datatype target_datatype;
        } origin;
        struct {
            int origin_rank;
        } target;
    } u;
} MPIDIG_req_t;

/* Structure to capture arguments for pt2pt persistent communications */
typedef struct MPIDI_prequest {
    MPIDI_ptype p_type;         /* persistent request type */
    void *buffer;
    MPI_Aint count;
    int rank;
    int tag;
    MPIR_Context_id_t context_id;
    MPI_Datatype datatype;
} MPIDI_prequest_t;

/* Structures for partitioned pt2pt request */
typedef struct MPIDIG_part_sreq {
    MPIR_cc_t ready_cntr;
} MPIDIG_part_sreq_t;

typedef struct MPIDIG_part_rreq {
    MPI_Aint sdata_size;        /* size of entire send data */
} MPIDIG_part_rreq_t;

typedef struct MPIDIG_part_request {
    MPIR_Request *peer_req_ptr;
    union {
        MPIDIG_part_sreq_t send;
        MPIDIG_part_rreq_t recv;
    } u;
} MPIDIG_part_request_t;

typedef struct MPIDI_part_request {
    MPIDIG_part_request_t am;

    /* partitioned attributes */
    void *buffer;
    MPI_Aint count;
    MPI_Datatype datatype;
    union {
        struct {
            int dest;
        } send;
        struct {
            int source;
            MPIR_Context_id_t context_id;
            int tag;
        } recv;
    } u;
    union {
    MPIDI_NM_PART_DECL} netmod;
} MPIDI_part_request_t;

/* message queue within "self"-comms, i.e. MPI_COMM_SELF and all communicators with size of 1. */

typedef struct {
    void *buf;
    MPI_Aint count;
    MPI_Datatype datatype;
    int tag;
    int context_id;
    MPIR_Request *match_req;    /* for mrecv */
} MPIDI_self_request_t;

typedef enum {
    MPIDI_REQ_TYPE_NONE = 0,
    MPIDI_REQ_TYPE_AM,
} MPIDI_REQ_TYPE;

typedef struct MPIDI_Devreq_t {
    /* completion notification counter: this must be decremented by
     * the request completion routine, when the completion count hits
     * zero.  this counter allows us to keep track of the completion
     * of multiple requests in a single place. */
    MPIR_cc_t *completion_notification;

#ifndef MPIDI_CH4_DIRECT_NETMOD
    int is_local;
    /* Anysource handling. Netmod and shm specific requests are cross
     * referenced. This must be present all of the time to avoid lots of extra
     * ifdefs in the code. */
    struct MPIR_Request *anysrc_partner;
#endif

    /* initialized to MPIDI_REQ_TYPE_NONE, set when one of the union field is set */
    MPIDI_REQ_TYPE type;
    union {
        /* The first fields are used by the MPIDIG apis */
        MPIDIG_req_t am;

        /* Used by pt2pt persistent communication */
        MPIDI_prequest_t preq;

        /* Used by partitioned communication */
        MPIDI_part_request_t part_req;

        MPIDI_self_request_t self;

        /* Used by the netmod direct apis */
        union {
        MPIDI_NM_REQUEST_DECL} netmod;

#ifndef MPIDI_CH4_DIRECT_NETMOD
        union {
        MPIDI_SHM_REQUEST_DECL} shm;
#endif
    } ch4;
} MPIDI_Devreq_t;

#define MPIDI_REQUEST_HDR_SIZE              offsetof(struct MPIR_Request, dev.ch4.netmod)
#define MPIDI_REQUEST(req,field)       (((req)->dev).field)
#define MPIDIG_REQUEST(req,field)       (((req)->dev.ch4.am).field)
#define MPIDI_PREQUEST(req,field)       (((req)->dev.ch4.preq).field)
#define MPIDI_PART_REQUEST(req,field)   (((req)->dev.ch4.part_req).field)
#define MPIDIG_PART_REQUEST(req, field)   (((req)->dev.ch4.part_req).am.field)
#define MPIDI_SELF_REQUEST(req, field)  (((req)->dev.ch4.self).field)

#define MPIDIG_REQUEST_IN_PROGRESS(r)   ((r)->dev.ch4.am.req->status & MPIDIG_REQ_IN_PROGRESS)

#ifndef MPIDI_CH4_DIRECT_NETMOD
#define MPIDI_REQUEST_SET_LOCAL(req, is_local_, partner_) \
    do { \
        (req)->dev.is_local = is_local_; \
        (req)->dev.anysrc_partner = partner_; \
    } while (0)
#else
#define MPIDI_REQUEST_SET_LOCAL(req, is_local_, partner_)  do { } while (0)
#endif

MPL_STATIC_INLINE_PREFIX void MPID_Request_create_hook(struct MPIR_Request *req);
MPL_STATIC_INLINE_PREFIX void MPID_Request_free_hook(struct MPIR_Request *req);
MPL_STATIC_INLINE_PREFIX void MPID_Request_destroy_hook(struct MPIR_Request *req);

typedef struct MPIDIG_win_shared_info {
    size_t size;
    void *shm_base_addr;
    uint32_t disp_unit;
    int ipc_mapped_device;
} MPIDIG_win_shared_info_t;

#define MPIDIG_ACCU_ORDER_RAR (1)
#define MPIDIG_ACCU_ORDER_RAW (1 << 1)
#define MPIDIG_ACCU_ORDER_WAR (1 << 2)
#define MPIDIG_ACCU_ORDER_WAW (1 << 3)

typedef enum {
    MPIDIG_ACCU_SAME_OP,
    MPIDIG_ACCU_SAME_OP_NO_OP
} MPIDIG_win_info_accumulate_ops;

#define MPIDIG_ACCU_NUM_OP (MPIR_OP_N_BUILTIN)  /* builtin reduce op + cswap */

typedef enum {
    MPIDIG_RMA_LAT_PREFERRED = 0,
    MPIDIG_RMA_MR_PREFERRED,
} MPIDIG_win_info_perf_preference;

typedef struct MPIDIG_win_info_args_t {
    int no_locks;
    int same_size;
    int same_disp_unit;
    int accumulate_ordering;
    int alloc_shared_noncontig;
    MPIDIG_win_info_accumulate_ops accumulate_ops;

    /* hints to tradeoff atomicity support */
    uint32_t which_accumulate_ops;      /* Arbitrary combination of {1<<max|1<<min|1<<sum|...}
                                         * with bit shift defined by op index (0<=index<MPIDIG_ACCU_NUM_OP).
                                         * any_op and none are two special values.
                                         * any_op by default. */
    bool accumulate_noncontig_dtype;    /* true by default. */
    MPI_Aint accumulate_max_bytes;      /* Non-negative integer, -1 (unlimited) by default.
                                         * TODO: can be set to win_size.*/
    bool disable_shm_accumulate;        /* false by default. */
    bool coll_attach;           /* false by default. Valid only for dynamic window */
    int perf_preference;        /* Arbitrary combination of MPIDIG_win_info_perf_preference.
                                 * By default MPICH/CH4 tends to optimize for low latency.
                                 * MPICH may ignore invalid combination. */

    /* alloc_shm: MPICH specific hint (same in CH3).
     * If true, MPICH will try to use shared memory routines for the window.
     * Default is true for allocate-based windows, and false for other
     * windows. Note that this hint can be also used in create-based windows,
     * and it means the user window buffer is allocated over shared memory,
     * thus RMA operation can use shm routines. */
    int alloc_shm;
    bool optimized_mr;          /* false by default. */
} MPIDIG_win_info_args_t;

struct MPIDIG_win_lock {
    struct MPIDIG_win_lock *next;
    int rank;
    uint16_t mtype;             /* MPIDIG_WIN_LOCK or MPIDIG_WIN_LOCKALL */
    uint16_t type;              /* MPI_LOCK_EXCLUSIVE or MPI_LOCK_SHARED */
};

typedef struct MPIDIG_win_lock_recvd {
    struct MPIDIG_win_lock *head;
    struct MPIDIG_win_lock *tail;
    int type;                   /* current lock's type */
    unsigned count;             /* count of granted locks (not received) */
} MPIDIG_win_lock_recvd_t;

typedef struct MPIDIG_win_target_sync_lock {
    /* NOTE: use volatile to avoid compiler optimization which keeps reading
     * register value when no dependency or function pointer is found in fully
     * inlined code.*/
    volatile unsigned locked;   /* locked == 0 or 1 */
} MPIDIG_win_target_sync_lock_t;

typedef struct MPIDIG_win_sync_lock {
    unsigned count;             /* count of lock epochs on the window */
} MPIDIG_win_sync_lock_t;

typedef struct MPIDIG_win_sync_lockall {
    /* NOTE: use volatile to avoid compiler optimization which keeps reading
     * register value when no dependency or function pointer is found in fully
     * inlined code.*/
    volatile unsigned allLocked;        /* 0 <= allLocked < size */
} MPIDIG_win_sync_lockall_t;

typedef struct MPIDIG_win_sync_pscw {
    struct MPIR_Group *group;
    /* NOTE: use volatile to avoid compiler optimization which keeps reading
     * register value when no dependency or function pointer is found in fully
     * inlined code.*/
    volatile unsigned count;
} MPIDIG_win_sync_pscw_t;

typedef struct MPIDIG_win_target_sync {
    int access_epoch_type;      /* NONE, LOCK. */
    MPIDIG_win_target_sync_lock_t lock;
    uint32_t assert_mode;       /* bit-vector OR of zero or more of the following integer constant:
                                 * MPI_MODE_NOCHECK, MPI_MODE_NOSTORE, MPI_MODE_NOPUT, MPI_MODE_NOPRECEDE, MPI_MODE_NOSUCCEED. */
} MPIDIG_win_target_sync_t;

typedef struct MPIDIG_win_sync {
    int access_epoch_type;      /* NONE, FENCE, LOCKALL, START,
                                 * LOCK (refer to target_sync). */
    int exposure_epoch_type;    /* NONE, FENCE, POST. */
    uint32_t assert_mode;       /* bit-vector OR of zero or more of the following integer constant:
                                 * MPI_MODE_NOCHECK, MPI_MODE_NOSTORE, MPI_MODE_NOPUT, MPI_MODE_NOPRECEDE, MPI_MODE_NOSUCCEED. */

    /* access epochs */
    /* TODO: Can we put access epochs in union,
     * since no concurrent epochs is allowed ? */
    MPIDIG_win_sync_pscw_t sc;
    MPIDIG_win_sync_lockall_t lockall;
    MPIDIG_win_sync_lock_t lock;

    /* exposure epochs */
    MPIDIG_win_sync_pscw_t pw;
    MPIDIG_win_lock_recvd_t lock_recvd;
} MPIDIG_win_sync_t;

typedef struct MPIDIG_win_target {
    MPIR_cc_t local_cmpl_cnts;  /* increase at OP issuing, decrease at local completion */
    MPIR_cc_t remote_cmpl_cnts; /* increase at OP issuing, decrease at remote completion */
    MPIR_cc_t remote_acc_cmpl_cnts;     /* for acc only, increase at OP issuing, decrease at remote completion */
    MPIDIG_win_target_sync_t sync;
    int rank;
    UT_hash_handle hash_handle;
} MPIDIG_win_target_t;

typedef struct MPIDIG_win_t {
    uint64_t win_id;
    void *mmap_addr;
    int64_t mmap_sz;

    /* per-window OP completion for fence */
    MPIR_cc_t local_cmpl_cnts;  /* increase at OP issuing, decrease at local completion */
    MPIR_cc_t remote_cmpl_cnts; /* increase at OP issuing, decrease at remote completion */
    MPIR_cc_t remote_acc_cmpl_cnts;     /* for acc only, increase at OP issuing, decrease at remote completion */

    MPIDIG_win_sync_t sync;
    MPIDIG_win_info_args_t info_args;
    MPIDIG_win_shared_info_t *shared_table;

    /* per-target structure for sync and OP completion. */
    MPIDIG_win_target_t *targets;
} MPIDIG_win_t;

typedef enum {
    MPIDI_WINATTR_DIRECT_INTRA_COMM = 1,
    MPIDI_WINATTR_SHM_ALLOCATED = 2,    /* shm optimized flag (0 or 1), set at shmmod win initialization time.
                                         * Equal to 1 if the window has a shared memory region associated with it
                                         * and the shmmod supports load/store based RMA operations over the window
                                         * (e.g., may rely on support of interprocess mutex). */
    MPIDI_WINATTR_ACCU_NO_SHM = 4,      /* shortcut of disable_shm_accumulate in MPIDIG_win_info_args_t. */
    MPIDI_WINATTR_ACCU_SAME_OP_NO_OP = 8,
    MPIDI_WINATTR_NM_REACHABLE = 16,    /* whether a netmod may reach the window. Set by netmod at win init.
                                         * Each netmod decides the definition of "reachable" at win_init based on
                                         * its internal optimization. */
    MPIDI_WINATTR_NM_DYNAMIC_MR = 32,   /* whether the memory region is registered dynamically. Valid only for
                                         * dynamic window. Set by netmod. */
    MPIDI_WINATTR_MR_PREFERRED = 64,    /* message rate preferred flag. Default 0, set by user hint. */
    MPIDI_WINATTR_LAST_BIT
} MPIDI_winattr_bit_t;

typedef unsigned MPIDI_winattr_t;       /* bit-vector of zero or multiple integer attributes defined in MPIDI_winattr_bit_t. */

typedef struct {
    MPIDI_winattr_t winattr;    /* attributes for performance optimization at fast path. */
    MPIDIG_win_t am;
    int am_vci;                 /* both netmod and shm have to use the same vci for am operations or
                                 * we won't be able to effectively progress at synchronization */
    int *vci_table;
    union {
    MPIDI_NM_WIN_DECL} netmod;
#ifndef MPIDI_CH4_DIRECT_NETMOD
    struct {
        /* multiple shmmods may co-exist. */
    MPIDI_SHM_WIN_DECL} shm;
#endif
} MPIDI_Devwin_t;

#define MPIDIG_WIN(win,field)        (((win)->dev.am).field)
#define MPIDI_WIN(win,field)         ((win)->dev).field

typedef unsigned MPIDI_locality_t;

typedef struct MPIDIG_comm_t {
    uint32_t window_instance;
#ifdef HAVE_DEBUGGER_SUPPORT
    MPIR_Request **posted_head_ptr;
    MPIR_Request **unexp_head_ptr;
#endif
} MPIDIG_comm_t;

#define MPIDI_CALC_STRIDE(rank, stride, blocksize, offset) \
    ((rank) / (blocksize) * ((stride) - (blocksize)) + (rank) + (offset))

#define MPIDI_CALC_STRIDE_SIMPLE(rank, stride, offset) \
    ((rank) * (stride) + (offset))

typedef enum {
    MPIDI_RANK_MAP_DIRECT,
    MPIDI_RANK_MAP_DIRECT_INTRA,
    MPIDI_RANK_MAP_OFFSET,
    MPIDI_RANK_MAP_OFFSET_INTRA,
    MPIDI_RANK_MAP_STRIDE,
    MPIDI_RANK_MAP_STRIDE_INTRA,
    MPIDI_RANK_MAP_STRIDE_BLOCK,
    MPIDI_RANK_MAP_STRIDE_BLOCK_INTRA,
    MPIDI_RANK_MAP_LUT,
    MPIDI_RANK_MAP_LUT_INTRA,
    MPIDI_RANK_MAP_MLUT,
    MPIDI_RANK_MAP_NONE
} MPIDI_rank_map_mode;

typedef int MPIDI_lpid_t;
typedef struct {
    int avtid;
    int lpid;
} MPIDI_gpid_t;

typedef struct {
    MPIR_OBJECT_HEADER;
    MPIDI_lpid_t lpid[];
} MPIDI_rank_map_lut_t;

typedef struct {
    MPIR_OBJECT_HEADER;
    MPIDI_gpid_t gpid[];
} MPIDI_rank_map_mlut_t;

typedef struct {
    MPIDI_rank_map_mode mode;
    int avtid;
    int size;

    union {
        int offset;
        struct {
            int offset;
            int stride;
            int blocksize;
        } stride;
    } reg;

    union {
        struct {
            MPIDI_rank_map_lut_t *t;
            MPIDI_lpid_t *lpid;
        } lut;
        struct {
            MPIDI_rank_map_mlut_t *t;
            MPIDI_gpid_t *gpid;
        } mlut;
    } irreg;
} MPIDI_rank_map_t;

typedef struct MPIDI_Devcomm_t {
    struct {
        /* The first fields are used by the AM(MPIDIG) apis */
        MPIDIG_comm_t am;

        /* Used by the netmod direct apis */
        union {
        MPIDI_NM_COMM_DECL} netmod;

#ifndef MPIDI_CH4_DIRECT_NETMOD
        union {
        MPIDI_SHM_COMM_DECL} shm;
#endif

        MPIDI_rank_map_t map;
        MPIDI_rank_map_t local_map;
        struct MPIR_Comm *multi_leads_comm;
        /* sub communicators related for multi-leaders based implementation */
        struct MPIR_Comm *inter_node_leads_comm, *sub_node_comm, *intra_node_leads_comm;
        int spanned_num_nodes;  /* comm spans over these number of nodes */
        /* Pointers to store info of multi-leaders based compositions */
        struct MPIDI_Multileads_comp_info_t *alltoall_comp_info, *allgather_comp_info,
            *allreduce_comp_info;
        int shm_size_per_lead;

        void *csel_comm;        /* collective selection handle */
    } ch4;
} MPIDI_Devcomm_t;

typedef struct MPIDI_Multileads_comp_info_t {
    int use_multi_leads;        /* if multi-leaders based composition can be used for comm */
    void *shm_addr;
} MPIDI_Multileads_comp_info_t;

#define MPIDIG_COMM(comm,field) ((comm)->dev.ch4.am).field
#define MPIDI_COMM(comm,field) ((comm)->dev.ch4).field
#define MPIDI_COMM_ALLTOALL(comm,field) ((comm)->dev.ch4.alltoall_comp_info)->field
#define MPIDI_COMM_ALLGATHER(comm,field) ((comm)->dev.ch4.allgather_comp_info)->field
#define MPIDI_COMM_ALLREDUCE(comm,field) ((comm)->dev.ch4.allreduce_comp_info)->field


typedef struct {
    union {
    MPIDI_NM_OP_DECL} netmod;
} MPIDI_Devop_t;

typedef struct {
    struct MPIDU_stream_workq *workq;
} MPIDI_Devstream_t;

#define MPID_DEV_REQUEST_DECL    MPIDI_Devreq_t  dev;
#define MPID_DEV_WIN_DECL        MPIDI_Devwin_t  dev;
#define MPID_DEV_COMM_DECL       MPIDI_Devcomm_t dev;
#define MPID_DEV_OP_DECL         MPIDI_Devop_t   dev;
#define MPID_DEV_STREAM_DECL     MPIDI_Devstream_t dev;

typedef struct MPIDI_av_entry {
    union {
    MPIDI_NM_ADDR_DECL} netmod;
#ifdef MPIDI_BUILD_CH4_LOCALITY_INFO
    MPIDI_locality_t is_local;
#endif
} MPIDI_av_entry_t;

#define HAVE_DEV_COMM_HOOK

/*
 * operation for (avtid, lpid) to/from gpid
 */
#define MPIDIU_LPID_BITS                     32
#define MPIDIU_LPID_MASK                     0xFFFFFFFFU
#define MPIDIU_GPID_CREATE(avtid, lpid)      (((uint64_t) (avtid) << MPIDIU_LPID_BITS) | (lpid))
#define MPIDIU_GPID_GET_AVTID(gpid)          ((gpid) >> MPIDIU_LPID_BITS)
#define MPIDIU_GPID_GET_LPID(gpid)           ((gpid) & MPIDIU_LPID_MASK)

#define MPIDI_DYNPROC_MASK                 (0x80000000U)

#define MPID_INTERCOMM_NO_DYNPROC(comm) \
    (MPIDI_COMM((comm),map).avtid == 0 && MPIDI_COMM((comm),local_map).avtid == 0)

int MPIDI_check_for_failed_procs(void);

#ifdef HAVE_SIGNAL
void MPIDI_sigusr1_handler(int sig);
#endif

#include "mpidu_pre.h"

#endif /* MPIDPRE_H_INCLUDED */
