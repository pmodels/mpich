/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef MPIDPRE_H_INCLUDED
#define MPIDPRE_H_INCLUDED

#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#ifdef HAVE_LIBHCOLL
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
#include "ch4i_workq_types.h"

/* Currently, workq is a configure-time only option and guarded by macro
 * MPIDI_CH4_USE_WORK_QUEUES. If we want to enable runtime option, we will
 * need to switch everywhere from "#ifdef MPIDI_CH4_USE_WORK_QUEUES" into
 * runtime "if - else".
 */
#ifdef MPIDI_CH4_USE_MT_DIRECT
#define MPIDI_CH4_MT_MODEL MPIDI_CH4_MT_DIRECT
#elif defined MPIDI_CH4_USE_MT_HANDOFF
#define MPIDI_CH4_MT_MODEL MPIDI_CH4_MT_HANDOFF
#elif defined MPIDI_CH4_USE_MT_RUNTIME
#define MPIDI_CH4_MT_MODEL MPIDI_global.settings.mt_model
#else
#error "Unknown MT model or MT model not defined"
#endif

typedef struct {
#ifdef HAVE_LIBHCOLL
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

typedef struct MPIDIG_sreq_t {
    /* persistent send fields */
    const void *src_buf;
    MPI_Count count;
    MPI_Datatype datatype;
    int rank;
    MPIR_Context_id_t context_id;
} MPIDIG_sreq_t;

typedef struct MPIDIG_rreq_t {
    /* mrecv fields */
    void *mrcv_buffer;
    uint64_t mrcv_count;
    MPI_Datatype mrcv_datatype;

    uint64_t ignore;
    MPIR_Request *peer_req_ptr;
    MPIR_Request *match_req;
    MPIR_Request *request;

    struct MPIDIG_rreq_t *prev, *next;
} MPIDIG_rreq_t;

typedef struct MPIDIG_put_req_t {
    MPIR_Win *win_ptr;
    MPIR_Request *preq_ptr;
    void *flattened_dt;
    MPIR_Datatype *dt;
    void *origin_addr;
    int origin_count;
    MPI_Datatype origin_datatype;
    void *target_addr;
    MPI_Datatype target_datatype;
} MPIDIG_put_req_t;

typedef struct MPIDIG_get_req_t {
    MPIR_Win *win_ptr;
    MPIR_Request *greq_ptr;
    void *addr;
    MPI_Datatype datatype;
    int count;
    void *flattened_dt;
    MPIR_Datatype *dt;
    MPI_Datatype target_datatype;
} MPIDIG_get_req_t;

typedef struct MPIDIG_cswap_req_t {
    MPIR_Win *win_ptr;
    MPIR_Request *creq_ptr;
    void *addr;
    MPI_Datatype datatype;
    void *data;
    void *result_addr;
} MPIDIG_cswap_req_t;

typedef struct MPIDIG_acc_req_t {
    MPIR_Win *win_ptr;
    MPIR_Request *req_ptr;
    MPI_Datatype origin_datatype;
    MPI_Datatype target_datatype;
    int origin_count;
    int target_count;
    void *target_addr;
    void *flattened_dt;
    void *data;
    size_t data_sz;
    MPI_Op op;
    void *result_addr;
    int result_count;
    void *origin_addr;
    MPI_Datatype result_datatype;
} MPIDIG_acc_req_t;

typedef int (*MPIDIG_req_cmpl_cb) (MPIR_Request * req);

/* structure used for supporting asynchronous payload transfer */
typedef enum {
    MPIDIG_RECV_DATATYPE,       /* use the datatype info in MPIDIG_req_t */
    MPIDIG_RECV_CONTIG,         /* set and use the contig recv-buffer info */
    MPIDIG_RECV_IOV             /* set and use the iov recv-buffer info */
} MPIDIG_recv_type;

typedef struct MPIDIG_req_async {
    MPIDIG_recv_type recv_type;
    MPI_Aint in_data_sz;
    MPI_Aint offset;
    struct iovec *iov_ptr;      /* used with MPIDIG_RECV_IOV */
    int iov_num;                /* used with MPIDIG_RECV_IOV */
    struct iovec iov_one;       /* used with MPIDIG_RECV_CONTIG */
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
        MPIDIG_sreq_t sreq;
        MPIDIG_rreq_t rreq;
        MPIDIG_put_req_t preq;
        MPIDIG_get_req_t greq;
        MPIDIG_cswap_req_t creq;
        MPIDIG_acc_req_t areq;
    };

    MPIDIG_rreq_async_t recv_async;
    MPIDIG_sreq_async_t send_async;
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
    void *buffer;
    MPI_Aint count;
    int rank;
    int tag;
    MPIR_Context_id_t context_id;
    MPI_Datatype datatype;
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

typedef struct {
#ifndef MPIDI_CH4_DIRECT_NETMOD
    int is_local;
    /* Anysource handling. Netmod and shm specific requests are cross
     * referenced. This must be present all of the time to avoid lots of extra
     * ifdefs in the code. */
    struct MPIR_Request *anysource_partner_request;
#endif

    union {
        /* The first fields are used by the MPIDIG apis */
        MPIDIG_req_t am;

        /* Used by pt2pt persistent communication */
        MPIDI_prequest_t preq;

        /* Used by the netmod direct apis */
        union {
        MPIDI_NM_REQUEST_DECL} netmod;

#ifndef MPIDI_CH4_DIRECT_NETMOD
        union {
        MPIDI_SHM_REQUEST_DECL} shm;
#endif

#ifdef MPIDI_CH4_USE_WORK_QUEUES
        MPIDI_workq_elemt_t command;
#endif
    } ch4;
} MPIDI_Devreq_t;
#define MPIDI_REQUEST_HDR_SIZE              offsetof(struct MPIR_Request, dev.ch4.netmod)
#define MPIDI_REQUEST(req,field)       (((req)->dev).field)
#define MPIDIG_REQUEST(req,field)       (((req)->dev.ch4.am).field)
#define MPIDI_PREQUEST(req,field)       (((req)->dev.ch4.preq).field)

#ifdef MPIDI_CH4_USE_WORK_QUEUES
/* `(r)->dev.ch4.am.req` might not be allocated right after SHM_mpi_recv when
 * the operations are enqueued with the handoff model. */
#define MPIDIG_REQUEST_IN_PROGRESS(r)   ((r)->dev.ch4.am.req && ((r)->dev.ch4.am.req->status & MPIDIG_REQ_IN_PROGRESS))
#else
#define MPIDIG_REQUEST_IN_PROGRESS(r)   ((r)->dev.ch4.am.req->status & MPIDIG_REQ_IN_PROGRESS)
#endif /* #ifdef MPIDI_CH4_USE_WORK_QUEUES */

#ifndef MPIDI_CH4_DIRECT_NETMOD
#define MPIDI_REQUEST_ANYSOURCE_PARTNER(req)  (((req)->dev).anysource_partner_request)
#define MPIDI_REQUEST_SET_LOCAL(req, is_local_, partner_) \
    do { \
        (req)->dev.is_local = is_local_; \
        (req)->dev.anysource_partner_request = partner_; \
    } while (0)
#else
#define MPIDI_REQUEST_ANYSOURCE_PARTNER(req)  NULL
#define MPIDI_REQUEST_SET_LOCAL(req, is_local_, partner_)  do { } while (0)
#endif

MPL_STATIC_INLINE_PREFIX void MPID_Request_create_hook(struct MPIR_Request *req);
MPL_STATIC_INLINE_PREFIX void MPID_Request_free_hook(struct MPIR_Request *req);
MPL_STATIC_INLINE_PREFIX void MPID_Request_destroy_hook(struct MPIR_Request *req);

typedef struct MPIDIG_win_shared_info {
    uint32_t disp_unit;
    size_t size;
    void *shm_base_addr;
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

    /* alloc_shm: MPICH specific hint (same in CH3).
     * If true, MPICH will try to use shared memory routines for the window.
     * Default is true for allocate-based windows, and false for other
     * windows. Note that this hint can be also used in create-based windows,
     * and it means the user window buffer is allocated over shared memory,
     * thus RMA operation can use shm routines. */
    int alloc_shm;
} MPIDIG_win_info_args_t;

struct MPIDIG_win_lock {
    struct MPIDIG_win_lock *next;
    int rank;
    uint16_t mtype;
    uint16_t type;
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
    MPIDI_WINATTR_LAST_BIT
} MPIDI_winattr_bit_t;

typedef unsigned MPIDI_winattr_t;       /* bit-vector of zero or multiple integer attributes defined in MPIDI_winattr_bit_t. */

typedef struct {
    MPIDI_winattr_t winattr;    /* attributes for performance optimization at fast path. */
    MPIDIG_win_t am;
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
    MPIDIG_rreq_t *posted_list;
    MPIDIG_rreq_t *unexp_list;
    uint32_t window_instance;
#ifdef HAVE_DEBUGGER_SUPPORT
    MPIDIG_rreq_t **posted_head_ptr;
    MPIDIG_rreq_t **unexp_head_ptr;
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
        void *csel_comm;        /* collective selection handle */
    } ch4;
} MPIDI_Devcomm_t;
#define MPIDIG_COMM(comm,field) ((comm)->dev.ch4.am).field
#define MPIDI_COMM(comm,field) ((comm)->dev.ch4).field

typedef struct {
    union {
    MPIDI_NM_OP_DECL} netmod;
} MPIDI_Devop_t;

#define MPID_DEV_REQUEST_DECL    MPIDI_Devreq_t  dev;
#define MPID_DEV_WIN_DECL        MPIDI_Devwin_t  dev;
#define MPID_DEV_COMM_DECL       MPIDI_Devcomm_t dev;
#define MPID_DEV_OP_DECL         MPIDI_Devop_t   dev;

typedef struct MPIDI_av_entry {
    union {
    MPIDI_NM_ADDR_DECL} netmod;
#ifdef MPIDI_BUILD_CH4_LOCALITY_INFO
    MPIDI_locality_t is_local;
#endif
} MPIDI_av_entry_t;

typedef struct {
    MPIR_OBJECT_HEADER;
    int size;
    MPIDI_av_entry_t table[];
} MPIDI_av_table_t;

extern MPIDI_av_table_t **MPIDI_av_table;
extern MPIDI_av_table_t *MPIDI_av_table0;

#define MPIDIU_get_av_table(avtid) (MPIDI_av_table[(avtid)])
#define MPIDIU_get_av(avtid, lpid) (MPIDI_av_table[(avtid)]->table[(lpid)])

#define MPIDIU_get_node_map(avtid)   (MPIDI_global.node_map[(avtid)])

#define HAVE_DEV_COMM_HOOK

/*
 * operation for (avtid, lpid) to/from "lupid"
 * 1 bit is reserved for "new_avt_mark". It will be cleared before accessing
 * the avtid and lpid. Therefore, the avtid mask does have that bit set to 0
 */
#define MPIDIU_AVTID_BITS                    (7)
#define MPIDIU_LPID_BITS                     (8 * sizeof(int) - (MPIDIU_AVTID_BITS + 1))
#define MPIDIU_LPID_MASK                     (0xFFFFFFFFU >> (MPIDIU_AVTID_BITS + 1))
#define MPIDIU_AVTID_MASK                    (~MPIDIU_LPID_MASK)
#define MPIDIU_NEW_AVT_MARK                  (0x80000000U)
#define MPIDIU_LUPID_CREATE(avtid, lpid)      (((avtid) << MPIDIU_LPID_BITS) | (lpid))
#define MPIDIU_LUPID_GET_AVTID(lupid)          ((((lupid) & MPIDIU_AVTID_MASK) >> MPIDIU_LPID_BITS))
#define MPIDIU_LUPID_GET_LPID(lupid)           (((lupid) & MPIDIU_LPID_MASK))
#define MPIDIU_LUPID_SET_NEW_AVT_MARK(lupid)   ((lupid) |= MPIDIU_NEW_AVT_MARK)
#define MPIDIU_LUPID_CLEAR_NEW_AVT_MARK(lupid) ((lupid) &= (~MPIDIU_NEW_AVT_MARK))
#define MPIDIU_LUPID_IS_NEW_AVT(lupid)         ((lupid) & MPIDIU_NEW_AVT_MARK)

#define MPIDI_DYNPROC_MASK                 (0x80000000U)

#define MPID_INTERCOMM_NO_DYNPROC(comm) \
    (MPIDI_COMM((comm),map).avtid == 0 && MPIDI_COMM((comm),local_map).avtid == 0)

int MPIDI_check_for_failed_procs(void);

#ifdef HAVE_SIGNAL
void MPIDI_sigusr1_handler(int sig);
#endif

#include "mpidu_pre.h"

#endif /* MPIDPRE_H_INCLUDED */
