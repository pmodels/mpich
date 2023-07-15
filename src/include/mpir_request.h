/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef MPIR_REQUEST_H_INCLUDED
#define MPIR_REQUEST_H_INCLUDED

#include "mpir_process.h"

/* NOTE-R1: MPIR_REQUEST_KIND__MPROBE signifies that this is a request created by
 * MPI_Mprobe or MPI_Improbe.  Since we use MPI_Request objects as our
 * MPI_Message objects, we use this separate kind in order to provide stronger
 * error checking.  Once a message (backed by a request) is promoted to a real
 * request by calling MPI_Mrecv/MPI_Imrecv, we actually modify the kind to be
 * MPIR_REQUEST_KIND__RECV in order to keep completion logic as simple as possible. */
/*E
  MPIR_Request_kind - Kinds of MPI Requests

  Module:
  Request-DS

  E*/
typedef enum MPIR_Request_kind_t {
    MPIR_REQUEST_KIND__UNDEFINED,
    MPIR_REQUEST_KIND__SEND,
    MPIR_REQUEST_KIND__RECV,
    MPIR_REQUEST_KIND__PREQUEST_SEND,
    MPIR_REQUEST_KIND__PREQUEST_RECV,
    MPIR_REQUEST_KIND__PREQUEST_COLL,
    MPIR_REQUEST_KIND__PART_SEND,       /* Partitioned send req returned to user */
    MPIR_REQUEST_KIND__PART_RECV,       /* Partitioned recv req returned to user */
    MPIR_REQUEST_KIND__PART,    /* Partitioned pt2pt internal reqs */
    MPIR_REQUEST_KIND__ENQUEUE, /* enqueued (to gpu stream) request */
    MPIR_REQUEST_KIND__GREQUEST,
    MPIR_REQUEST_KIND__COLL,
    MPIR_REQUEST_KIND__MPROBE,  /* see NOTE-R1 */
    MPIR_REQUEST_KIND__RMA,
    MPIR_REQUEST_KIND__LAST
#ifdef MPID_REQUEST_KIND_DECL
        , MPID_REQUEST_KIND_DECL
#endif
} MPIR_Request_kind_t;

/* define built-in handles for pre-completed requests. These are internally used
 * and are not exposed to the user.
 */
#define MPIR_REQUEST_COMPLETE      (MPI_Request)0x6c000000
#define MPIR_REQUEST_COMPLETE_SEND (MPI_Request)0x6c000001
#define MPIR_REQUEST_COMPLETE_RECV (MPI_Request)0x6c000002
#define MPIR_REQUEST_COMPLETE_COLL (MPI_Request)0x6c000006
#define MPIR_REQUEST_COMPLETE_RMA  (MPI_Request)0x6c000008

#define MPIR_REQUEST_NULL_RECV     (MPI_Request)0x6c000010

/* This currently defines a single structure type for all requests.
   Eventually, we may want a union type, as used in MPICH-1 */
/* Typedefs for Fortran generalized requests */
typedef void (MPIR_Grequest_f77_cancel_function) (void *, MPI_Fint *, MPI_Fint *);
typedef void (MPIR_Grequest_f77_free_function) (void *, MPI_Fint *);
typedef void (MPIR_Grequest_f77_query_function) (void *, MPI_Fint *, MPI_Fint *);

/* vtable-ish structure holding generalized request function pointers and other
 * state.  Saves ~48 bytes in pt2pt requests on many platforms. */
struct MPIR_Grequest_fns {
    union {
        struct {
            MPI_Grequest_cancel_function *cancel_fn;
            MPI_Grequest_free_function *free_fn;
            MPI_Grequest_query_function *query_fn;
        } C;
        struct {
            MPIR_Grequest_f77_cancel_function *cancel_fn;
            MPIR_Grequest_f77_free_function *free_fn;
            MPIR_Grequest_f77_query_function *query_fn;
        } F;
    } U;
    MPIX_Grequest_poll_function *poll_fn;
    MPIX_Grequest_wait_function *wait_fn;
    void *grequest_extra_state;
    MPIX_Grequest_class greq_class;
    MPIR_Lang_t greq_lang;      /* language that defined
                                 * the generalize req */
};

typedef struct MPIR_Grequest_class {
    MPIR_OBJECT_HEADER;         /* adds handle and ref_count fields */
    MPI_Grequest_query_function *query_fn;
    MPI_Grequest_free_function *free_fn;
    MPI_Grequest_cancel_function *cancel_fn;
    MPIX_Grequest_poll_function *poll_fn;
    MPIX_Grequest_wait_function *wait_fn;
    struct MPIR_Grequest_class *next;
} MPIR_Grequest_class;

extern MPIR_Grequest_class MPIR_Grequest_class_direct[];
extern MPIR_Object_alloc_t MPIR_Grequest_class_mem;

#ifndef ENABLE_THREADCOMM
#define MPIR_Request_extract_status(request_ptr_, status_)              \
    {                                                                   \
        if ((status_) != MPI_STATUS_IGNORE)                             \
        {                                                               \
            int error__;                                                \
                                                                        \
            /* According to the MPI 1.1 standard page 22 lines 9-12,    \
             * the MPI_ERROR field may not be modified except by the    \
             * functions in section 3.7.5 which return                  \
             * MPI_ERR_IN_STATUSES (MPI_Wait{all,some} and              \
             * MPI_Test{all,some}). */                                  \
            error__ = (status_)->MPI_ERROR;                             \
            *(status_) = (request_ptr_)->status;                        \
            (status_)->MPI_ERROR = error__;                             \
        }                                                               \
    }
#else
/* Same as above but with additional threadcomm tag reset */
#define MPIR_Request_extract_status(request_ptr_, status_)              \
    do {                                                                \
        if ((status_) != MPI_STATUS_IGNORE) {                           \
            int error__;                                                \
            error__ = (status_)->MPI_ERROR;                             \
            *(status_) = (request_ptr_)->status;                        \
            (status_)->MPI_ERROR = error__;                             \
                                                                        \
            if ((request_ptr_)->comm && (request_ptr_)->comm->threadcomm) { \
                MPIR_Threadcomm_adjust_status((request_ptr_)->comm->threadcomm, status_); \
            } \
        }                                                               \
    } while (0)
#endif

#define MPIR_Request_is_complete(req_) (MPIR_cc_is_complete((req_)->cc_ptr))

/* types of sched structure used in persistent collective */
enum MPIR_sched_type {
    MPIR_SCHED_INVALID,
    MPIR_SCHED_NORMAL,
    MPIR_SCHED_GENTRAN
};

/*S
  MPIR_Request - Description of the Request data structure

  Module:
  Request-DS

  Notes:
  If it is necessary to remember the MPI datatype, this information is
  saved within the device-specific fields provided by 'MPID_DEV_REQUEST_DECL'.

  Requests come in many flavors, as stored in the 'kind' field.  It is
  expected that each kind of request will have its own structure type
  (e.g., 'MPIR_Request_send_t') that extends the 'MPIR_Request'.

  S*/

#define MPIR_REQUEST_UNION_SIZE  40
struct MPIR_Request {
    MPIR_OBJECT_HEADER;         /* adds handle and ref_count fields */

    MPIR_Request_kind_t kind;

    /* pointer to the completion counter.  This is necessary for the
     * case when an operation is described by a list of requests */
    MPIR_cc_t *cc_ptr;
    /* the actual completion counter.  Ensure cc and status are in the
     * same cache line, assuming the cache line size is a multiple of
     * 32 bytes and 32-bit integers */
    MPIR_cc_t cc;

    /* A comm is needed to find the proper error handler */
    MPIR_Comm *comm;
    /* Status is needed for wait/test/recv */
    MPI_Status status;

    union {
        struct {
            struct MPIR_Grequest_fns *greq_fns;
        } ureq;                 /* kind : MPIR_REQUEST_KIND__GREQUEST */
        struct {
            MPIR_Errflag_t errflag;
            MPII_Coll_req_t coll;
        } nbc;                  /* kind : MPIR_REQUEST_KIND__COLL */
        struct {
            /* Persistent requests have their own "real" requests */
            struct MPIR_Request *real_request;
            MPIR_TSP_sched_t sched;
        } persist;              /* kind : MPIR_REQUEST_KIND__PREQUEST_SEND or MPIR_REQUEST_KIND__PREQUEST_RECV */
        struct {
            struct MPIR_Request *real_request;
            enum MPIR_sched_type sched_type;
            void *sched;
            MPII_Coll_req_t coll;
        } persist_coll;         /* kind : MPIR_REQUEST_KIND__PREQUEST_COLL */
        struct {
            int partitions;     /* Needed for parameter error check */
            MPL_atomic_int_t active_flag;       /* flag indicating whether in a start-complete active period.
                                                 * Value is 0 or 1. */
        } part;                 /* kind : MPIR_REQUEST_KIND__PART_SEND or MPIR_REQUEST_KIND__PART_RECV */
        struct {
            MPIR_Stream *stream_ptr;
            struct MPIR_Request *real_request;
            bool is_send;
            void *data;
        } enqueue;
        struct {
            MPIR_Win *win;
        } rma;                  /* kind : MPIR_REQUEST_KIND__RMA */
        /* Reserve space for local usages. For example, threadcomm, the actual struct
         * is defined locally and is used via casting */
        char dummy[MPIR_REQUEST_UNION_SIZE];
    } u;

#if defined HAVE_DEBUGGER_SUPPORT
    struct MPIR_Debugq *send;
    struct MPIR_Debugq *recv;
    struct MPIR_Debugq *unexp;
#endif                          /* HAVE_DEBUGGER_SUPPORT */

    struct MPIR_Request *next, *prev;
    UT_hash_handle hh;

    /* Other, device-specific information */
#ifdef MPID_DEV_REQUEST_DECL
     MPID_DEV_REQUEST_DECL
#endif
};
int MPIR_Persist_coll_start(MPIR_Request * request);
void MPIR_Persist_coll_free_cb(MPIR_Request * request);

/* Multiple Request Pools
 * Request objects creation and freeing is in a hot path. Multiple pools allow different
 * threads to access different pools without incurring global locks. Because only request
 * objects benefit from this multi-pool scheme, the normal handlemem macros and functions
 * are extended here for request objects. It is separate from the other objects, and the
 * bit patterns for POOL and BLOCK sizes can be adjusted if necessary.
 *
 * MPIR_Request_create_from_pool is used to create request objects from a specific pool.
 * MPIR_Request_create is a wrapper to create request from pool 0.
 */
/* Handle Bits - 2+4+6+8+12 - Type, Kind, Pool_idx, Block_idx, Object_idx */
#define REQUEST_POOL_MASK    0x03f00000
#define REQUEST_POOL_SHIFT   20
#define REQUEST_POOL_MAX     64
#define REQUEST_BLOCK_MASK   0x000ff000
#define REQUEST_BLOCK_SHIFT  12
#define REQUEST_BLOCK_MAX    256
#define REQUEST_OBJECT_MASK  0x00000fff
#define REQUEST_OBJECT_SHIFT 0
#define REQUEST_OBJECT_MAX   4096

#define REQUEST_NUM_BLOCKS   256
#define REQUEST_NUM_INDICES  1024

#define MPIR_REQUEST_NUM_POOLS REQUEST_POOL_MAX

#define MPIR_REQUEST_POOL(req_) (((req_)->handle & REQUEST_POOL_MASK) >> REQUEST_POOL_SHIFT)

extern MPIR_Request MPIR_Request_builtin[MPIR_REQUEST_N_BUILTIN];
extern MPIR_Object_alloc_t MPIR_Request_mem[MPIR_REQUEST_NUM_POOLS];
extern MPIR_Request MPIR_Request_direct[MPIR_REQUEST_PREALLOC];

#define MPIR_Request_get_ptr(a, ptr) \
    do { \
        int pool, blk, idx; \
        pool = ((a) & REQUEST_POOL_MASK) >> REQUEST_POOL_SHIFT; \
        switch (HANDLE_GET_KIND(a)) { \
        case HANDLE_KIND_BUILTIN: \
            if (a == MPI_MESSAGE_NO_PROC) { \
                ptr = NULL; \
            } else { \
                MPIR_Assert(HANDLE_INDEX(a) < MPIR_REQUEST_N_BUILTIN); \
                ptr = MPIR_Request_builtin + HANDLE_INDEX(a); \
            } \
            break; \
        case HANDLE_KIND_DIRECT: \
            MPIR_Assert(pool == 0); \
            ptr = MPIR_Request_direct + HANDLE_INDEX(a); \
            break; \
        case HANDLE_KIND_INDIRECT: \
            blk = ((a) & REQUEST_BLOCK_MASK) >> REQUEST_BLOCK_SHIFT; \
            idx = ((a) & REQUEST_OBJECT_MASK) >> REQUEST_OBJECT_SHIFT; \
            ptr = ((MPIR_Request *) MPIR_Request_mem[pool].indirect[blk]) + idx; \
            break; \
        default: \
            ptr = NULL; \
            break; \
        } \
    } while (0)

void MPII_init_request(void);

/* To get the benefit of multiple request pool, device layer need register their per-vci lock
 * with each pool that they are going to use, typically a 1-1 vci-pool mapping.
 * NOTE: currently, only per-vci thread granularity utilizes multiple request pool.
 */
static inline void MPIR_Request_register_pool_lock(int pool, MPID_Thread_mutex_t * lock)
{
    MPIR_Request_mem[pool].lock = lock;
}

static inline int MPIR_Request_is_persistent(MPIR_Request * req_ptr)
{
    return (req_ptr->kind == MPIR_REQUEST_KIND__PREQUEST_SEND ||
            req_ptr->kind == MPIR_REQUEST_KIND__PREQUEST_RECV ||
            req_ptr->kind == MPIR_REQUEST_KIND__PREQUEST_COLL ||
            req_ptr->kind == MPIR_REQUEST_KIND__PART_SEND ||
            req_ptr->kind == MPIR_REQUEST_KIND__PART_RECV);
}

static inline int MPIR_Part_request_is_active(MPIR_Request * req_ptr)
{
    return MPL_atomic_load_int(&req_ptr->u.part.active_flag);
}

static inline void MPIR_Part_request_inactivate(MPIR_Request * req_ptr)
{
    MPL_atomic_store_int(&req_ptr->u.part.active_flag, 0);
}

static inline void MPIR_Part_request_activate(MPIR_Request * req_ptr)
{
    MPL_atomic_store_int(&req_ptr->u.part.active_flag, 1);
}

/* Return whether a request is active.
 * A persistent request and the handle to it are "inactive"
 * if the request is not associated with any ongoing communication.
 * A handle is "active" if it is neither null nor "inactive". */
static inline int MPIR_Request_is_active(MPIR_Request * req_ptr)
{
    if (req_ptr == NULL)
        return 0;
    else {
        switch (req_ptr->kind) {
            case MPIR_REQUEST_KIND__PREQUEST_SEND:
            case MPIR_REQUEST_KIND__PREQUEST_RECV:
                return (req_ptr)->u.persist.real_request != NULL;
            case MPIR_REQUEST_KIND__PREQUEST_COLL:
                return (req_ptr)->u.persist_coll.real_request != NULL;
            case MPIR_REQUEST_KIND__PART_SEND:
            case MPIR_REQUEST_KIND__PART_RECV:
                return MPIR_Part_request_is_active(req_ptr);
            default:
                return 1;       /* regular request is always active */
        }
    }
}

#define MPIR_REQUESTS_PROPERTY__NO_NULL        (1 << 1)
#define MPIR_REQUESTS_PROPERTY__NO_GREQUESTS   (1 << 2)
#define MPIR_REQUESTS_PROPERTY__SEND_RECV_ONLY (1 << 3)
#define MPIR_REQUESTS_PROPERTY__OPT_ALL (MPIR_REQUESTS_PROPERTY__NO_NULL          \
                                         | MPIR_REQUESTS_PROPERTY__NO_GREQUESTS   \
                                         | MPIR_REQUESTS_PROPERTY__SEND_RECV_ONLY)

/* NOTE: Pool-specific request creation is unsafe unless under global thread granularity.
 */
static inline MPIR_Request *MPIR_Request_create_from_pool(MPIR_Request_kind_t kind, int pool,
                                                          int ref_count)
{
    MPIR_Assert(ref_count >= 1);
    MPIR_Request *req;

#ifdef MPICH_DEBUG_MUTEX
    MPID_THREAD_ASSERT_IN_CS(VCI, (*(MPID_Thread_mutex_t *) MPIR_Request_mem[pool].lock));
#endif
    req = MPIR_Handle_obj_alloc_unsafe(&MPIR_Request_mem[pool],
                                       REQUEST_NUM_BLOCKS, REQUEST_NUM_INDICES);
    if (req == NULL)
        goto fn_fail;

    /* Patch the handle for pool index. */
    req->handle |= (pool << REQUEST_POOL_SHIFT);

    MPL_DBG_MSG_P(MPIR_DBG_REQUEST, VERBOSE, "allocated request, handle=0x%08x", req->handle);
#ifdef MPICH_DBG_OUTPUT
    /*MPIR_Assert(HANDLE_GET_MPI_KIND(req->handle) == MPIR_REQUEST); */
    if (HANDLE_GET_MPI_KIND(req->handle) != MPIR_REQUEST) {
        int mpi_errno;
        mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL,
                                         __func__, __LINE__, MPI_ERR_OTHER,
                                         "**invalid_handle", "**invalid_handle %d", req->handle);
        MPID_Abort(MPIR_Process.comm_world, mpi_errno, -1, NULL);
    }
#endif
    /* FIXME: This makes request creation expensive.  We need to
     * trim this to the basics, with additional setup for
     * special-purpose requests (think base class and
     * inheritance).  For example, do we really* want to set the
     * kind to UNDEFINED? And should the RMA values be set only
     * for RMA requests? */
    MPIR_Object_set_ref(req, ref_count);
    req->kind = kind;
    MPIR_cc_set(&req->cc, 1);
    req->cc_ptr = &req->cc;

    req->status.MPI_ERROR = MPI_SUCCESS;
    MPIR_STATUS_SET_CANCEL_BIT(req->status, FALSE);

    req->comm = NULL;

    switch (kind) {
        case MPIR_REQUEST_KIND__COLL:
            req->u.nbc.errflag = MPIR_ERR_NONE;
            req->u.nbc.coll.host_sendbuf = NULL;
            req->u.nbc.coll.host_recvbuf = NULL;
            req->u.nbc.coll.datatype = MPI_DATATYPE_NULL;
            break;
        case MPIR_REQUEST_KIND__PREQUEST_COLL:
            req->u.persist_coll.coll.host_sendbuf = NULL;
            req->u.persist_coll.coll.host_recvbuf = NULL;
            req->u.persist_coll.coll.datatype = MPI_DATATYPE_NULL;
        default:
            break;
    }
    MPII_REQUEST_CLEAR_DBG(req);

    MPID_Request_create_hook(req);

  fn_exit:
    return req;
  fn_fail:
    MPIR_Assert(req != NULL);
    /* TODO - Obviously this is a bad solution, but no one had the stomach to make the larger change
     * in the entire codebase to do a better job. */
    goto fn_exit;
}

/* Useful for lockless MT model */
static inline MPIR_Request *MPIR_Request_create_from_pool_safe(MPIR_Request_kind_t kind, int pool,
                                                               int ref_count)
{
    MPIR_Request *req;

    MPID_THREAD_CS_ENTER(VCI, (*(MPID_Thread_mutex_t *) MPIR_Request_mem[pool].lock));
    req = MPIR_Request_create_from_pool(kind, pool, ref_count);
    MPID_THREAD_CS_EXIT(VCI, (*(MPID_Thread_mutex_t *) MPIR_Request_mem[pool].lock));
    return req;
}

/* NOTE: safe under per-vci, per-obj, or global thread granularity */
static inline MPIR_Request *MPIR_Request_create(MPIR_Request_kind_t kind)
{
    MPIR_Request *req;
    MPID_THREAD_CS_ENTER(VCI, (*(MPID_Thread_mutex_t *) MPIR_Request_mem[0].lock));
    req = MPIR_Request_create_from_pool(kind, 0, 1);
    MPID_THREAD_CS_EXIT(VCI, (*(MPID_Thread_mutex_t *) MPIR_Request_mem[0].lock));
    return req;
}

int MPIR_allocate_enqueue_request(MPIR_Comm * comm_ptr, MPIR_Request ** req);

#define MPIR_Request_add_ref(req_p_) \
    do { MPIR_Object_add_ref(req_p_); } while (0)

#define MPIR_Request_release_ref(req_p_, inuse_) \
    do { MPIR_Object_release_ref(req_p_, inuse_); } while (0)

MPL_STATIC_INLINE_PREFIX MPIR_Request *get_builtin_req(int idx, MPIR_Request_kind_t kind)
{
    return MPIR_Request_builtin + (idx);
}

MPL_STATIC_INLINE_PREFIX MPIR_Request *MPIR_Request_create_complete(MPIR_Request_kind_t kind)
{
    /* pre-completed request uses kind as idx */
    return get_builtin_req(kind, kind);
}

MPL_STATIC_INLINE_PREFIX MPIR_Request *MPIR_Request_create_null_recv(void)
{
    return get_builtin_req(HANDLE_INDEX(MPIR_REQUEST_NULL_RECV), MPIR_REQUEST_KIND__RECV);
}

static inline void MPIR_Request_free_with_safety(MPIR_Request * req, int need_safety)
{
    int inuse;
    int pool = MPIR_REQUEST_POOL(req);

    if (HANDLE_IS_BUILTIN(req->handle)) {
        /* do not free builtin request objects */
        return;
    }

    MPIR_Request_release_ref(req, &inuse);

    if (need_safety) {
        MPID_THREAD_CS_ENTER(VCI, (*(MPID_Thread_mutex_t *) MPIR_Request_mem[pool].lock));
    }
#ifdef MPICH_DEBUG_MUTEX
    MPID_THREAD_ASSERT_IN_CS(VCI, (*(MPID_Thread_mutex_t *) MPIR_Request_mem[pool].lock));
#endif
    /* inform the device that we are decrementing the ref-count on
     * this request */
    MPID_Request_free_hook(req);

    if (inuse == 0) {
        MPL_DBG_MSG_P(MPIR_DBG_REQUEST, VERBOSE, "freeing request, handle=0x%08x", req->handle);

#ifdef MPICH_DBG_OUTPUT
        if (HANDLE_GET_MPI_KIND(req->handle) != MPIR_REQUEST) {
            int mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL,
                                                 __func__, __LINE__, MPI_ERR_OTHER,
                                                 "**invalid_handle", "**invalid_handle %d",
                                                 req->handle);
            MPID_Abort(MPIR_Process.comm_world, mpi_errno, -1, NULL);
        }

        if (req->ref_count != 0) {
            int mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL,
                                                 __func__, __LINE__, MPI_ERR_OTHER,
                                                 "**invalid_refcount", "**invalid_refcount %d",
                                                 req->ref_count);
            MPID_Abort(MPIR_Process.comm_world, mpi_errno, -1, NULL);
        }
#endif

        /* FIXME: We need a better way to handle these so that we do
         * not always need to initialize these fields and check them
         * when we destroy a request */
        /* FIXME: We need a way to call these routines ONLY when the
         * related ref count has become zero. */
        if (req->comm != NULL) {
            if (MPIR_Request_is_persistent(req)) {
                MPIR_Comm_delete_inactive_request(req->comm, req);
            }
            MPIR_Comm_release(req->comm);
        }

        if (req->kind == MPIR_REQUEST_KIND__GREQUEST) {
            MPL_free(req->u.ureq.greq_fns);
        }

        if (req->kind == MPIR_REQUEST_KIND__SEND) {
            MPII_SENDQ_FORGET(req);
        } else if (req->kind == MPIR_REQUEST_KIND__RECV) {
            MPII_RECVQ_FORGET(req);
        }

        MPID_Request_destroy_hook(req);

        MPIR_Handle_obj_free_unsafe(&MPIR_Request_mem[pool], req, /* not info */ FALSE);
    }
    if (need_safety) {
        MPID_THREAD_CS_EXIT(VCI, (*(MPID_Thread_mutex_t *) MPIR_Request_mem[pool].lock));
    }
}

MPL_STATIC_INLINE_PREFIX void MPIR_Request_free_safe(MPIR_Request * req)
{
    MPIR_Request_free_with_safety(req, 1);
}

MPL_STATIC_INLINE_PREFIX void MPIR_Request_free_unsafe(MPIR_Request * req)
{
    MPIR_Request_free_with_safety(req, 0);
}

MPL_STATIC_INLINE_PREFIX void MPIR_Request_free(MPIR_Request * req)
{
    /* The default is to assume we need safety unless it's global thread granularity */
    MPIR_Request_free_with_safety(req, 1);
}

/* Requests that are not created inside device (general requests, nonblocking collective
 * requests such as sched, tsp, hcoll) should call MPIR_Request_complete.
 * MPID_Request_complete are called inside device critical section, therefore, potentially
 * are unsafe to call outside the device. (NOTE: this will come into effect with ch4 multi-vci.)
 */
MPL_STATIC_INLINE_PREFIX void MPIR_Request_complete(MPIR_Request * req)
{
    MPIR_cc_set(&req->cc, 0);
    MPIR_Request_free(req);
}

/* The "fastpath" version of MPIR_Request_completion_processing.  It only handles
 * MPIR_REQUEST_KIND__SEND and MPIR_REQUEST_KIND__RECV kinds, and it does not attempt to
 * deal with status structures under the assumption that bleeding fast code will
 * pass either MPI_STATUS_IGNORE or MPI_STATUSES_IGNORE as appropriate.  This
 * routine (or some a variation of it) is an unfortunately necessary stunt to
 * get high message rates on key benchmarks for high-end systems.
 */
MPL_STATIC_INLINE_PREFIX int MPIR_Request_completion_processing_fastpath(MPI_Request * request,
                                                                         MPIR_Request * request_ptr)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_Assert(request_ptr->kind == MPIR_REQUEST_KIND__SEND ||
                request_ptr->kind == MPIR_REQUEST_KIND__RECV);

    /* the completion path for SEND and RECV is the same at this time, modulo
     * the SENDQ hook above */
    mpi_errno = request_ptr->status.MPI_ERROR;
    MPIR_Request_free(request_ptr);
    *request = MPI_REQUEST_NULL;

    return mpi_errno;
}

int MPIR_Request_completion_processing(MPIR_Request *, MPI_Status *);
int MPIR_Request_get_error(MPIR_Request *);

MPL_STATIC_INLINE_PREFIX int MPID_Request_is_anysource(MPIR_Request *);
MPL_STATIC_INLINE_PREFIX int MPID_Comm_AS_enabled(MPIR_Comm *);
extern int MPIR_CVAR_ENABLE_FT;

/* The following routines are ULFM helpers. */

/* This routine check if the request is "anysource" but the communicator is not,
 * which happens usually due to a failure of a process in the communicator. */
MPL_STATIC_INLINE_PREFIX int MPIR_Request_is_anysrc_mismatched(MPIR_Request * req_ptr)
{
    return (MPIR_CVAR_ENABLE_FT &&
            !MPIR_Request_is_complete(req_ptr) &&
            MPID_Request_is_anysource(req_ptr) && !MPID_Comm_AS_enabled((req_ptr)->comm));
}

/* This routine handle the request when its associated process failed. */
int MPIR_Request_handle_proc_failed(MPIR_Request * request_ptr);

/* The following routines perform the callouts to the user routines registered
   as part of a generalized request.  They handle any language binding issues
   that are necessary. They are used when completing, freeing, cancelling or
   extracting the status from a generalized request. */
int MPIR_Grequest_cancel(MPIR_Request * request_ptr, int complete);
int MPIR_Grequest_query(MPIR_Request * request_ptr);
int MPIR_Grequest_free(MPIR_Request * request_ptr);

/* These routines below are helpers for the Extended generalized requests. */

MPL_STATIC_INLINE_PREFIX int MPIR_Request_has_poll_fn(MPIR_Request * request_ptr)
{
    return (request_ptr->kind == MPIR_REQUEST_KIND__GREQUEST &&
            request_ptr->u.ureq.greq_fns != NULL && request_ptr->u.ureq.greq_fns->poll_fn != NULL);
}

MPL_STATIC_INLINE_PREFIX int MPIR_Request_has_wait_fn(MPIR_Request * request_ptr)
{
    return (request_ptr->kind == MPIR_REQUEST_KIND__GREQUEST &&
            request_ptr->u.ureq.greq_fns != NULL && request_ptr->u.ureq.greq_fns->wait_fn != NULL);
}

MPL_STATIC_INLINE_PREFIX int MPIR_Grequest_wait(MPIR_Request * request_ptr, MPI_Status * status)
{
    int mpi_errno;
    MPID_THREAD_CS_EXIT(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    mpi_errno = (request_ptr->u.ureq.greq_fns->wait_fn) (1,
                                                         &request_ptr->u.ureq.greq_fns->
                                                         grequest_extra_state, 0, status);
    MPID_THREAD_CS_ENTER(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    return mpi_errno;
}

MPL_STATIC_INLINE_PREFIX int MPIR_Grequest_poll(MPIR_Request * request_ptr, MPI_Status * status)
{
    int mpi_errno;
    MPID_THREAD_CS_EXIT(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    mpi_errno =
        (request_ptr->u.ureq.greq_fns->poll_fn) (request_ptr->u.ureq.greq_fns->grequest_extra_state,
                                                 status);
    MPID_THREAD_CS_ENTER(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    return mpi_errno;
}

/* local request array size in MPI_Start_all and MPI_{Test,Wait}{all,any,some} */
#define MPIR_REQUEST_PTR_ARRAY_SIZE 64

int MPIR_Test_state(MPIR_Request * request, int *flag, MPI_Status * status,
                    MPID_Progress_state * state);
int MPIR_Testall_state(int count, MPIR_Request * request_ptrs[], int *flag,
                       MPI_Status array_of_statuses[], int requests_property,
                       MPID_Progress_state * state);
int MPIR_Testany_state(int count, MPIR_Request * request_ptrs[], int *indx, int *flag,
                       MPI_Status * status, MPID_Progress_state * state);
int MPIR_Testsome_state(int incount, MPIR_Request * request_ptrs[], int *outcount,
                        int array_of_indices[], MPI_Status array_of_statuses[],
                        MPID_Progress_state * state);
int MPIR_Test_impl(MPIR_Request * request, int *flag, MPI_Status * status);
int MPIR_Testall_impl(int count, MPIR_Request * request_ptrs[], int *flag,
                      MPI_Status array_of_statuses[], int requests_property);
int MPIR_Testany_impl(int count, MPIR_Request * request_ptrs[],
                      int *indx, int *flag, MPI_Status * status);
int MPIR_Testsome_impl(int incount, MPIR_Request * request_ptrs[],
                       int *outcount, int array_of_indices[], MPI_Status array_of_statuses[]);

int MPIR_Wait_state(MPIR_Request * request_ptr, MPI_Status * status, MPID_Progress_state * state);
int MPIR_Waitall_state(int count, MPIR_Request * request_ptrs[], MPI_Status array_of_statuses[],
                       int request_properties, MPID_Progress_state * state);
int MPIR_Waitany_state(int count, MPIR_Request * request_ptrs[], int *indx, MPI_Status * status,
                       MPID_Progress_state * state);
int MPIR_Waitsome_state(int incount, MPIR_Request * request_ptrs[],
                        int *outcount, int array_of_indices[], MPI_Status array_of_statuses[],
                        MPID_Progress_state * state);
int MPIR_Wait_impl(MPIR_Request * request_ptr, MPI_Status * status);
int MPIR_Waitall_impl(int count, MPIR_Request * request_ptrs[], MPI_Status array_of_statuses[],
                      int request_properties);
int MPIR_Waitany_impl(int count, MPIR_Request * request_ptrs[], int *indx, MPI_Status * status);
int MPIR_Waitsome_impl(int incount, MPIR_Request * request_ptrs[],
                       int *outcount, int array_of_indices[], MPI_Status array_of_statuses[]);

int MPIR_Test(MPIR_Request * request_ptr, int *flag, MPI_Status * status);
int MPIR_Testall(int count, MPI_Request array_of_requests[], int *flag,
                 MPI_Status array_of_statuses[]);
int MPIR_Testany(int count, MPI_Request array_of_requests[], MPIR_Request * request_ptrs[],
                 int *indx, int *flag, MPI_Status * status);
int MPIR_Testsome(int incount, MPI_Request array_of_requests[], MPIR_Request * request_ptrs[],
                  int *outcount, int array_of_indices[], MPI_Status array_of_statuses[]);
int MPIR_Wait(MPIR_Request * request_ptr, MPI_Status * status);
int MPIR_Waitall(int count, MPI_Request array_of_requests[], MPI_Status array_of_statuses[]);
int MPIR_Waitany(int count, MPI_Request array_of_requests[], MPIR_Request * request_ptrs[],
                 int *indx, MPI_Status * status);
int MPIR_Waitsome(int incount, MPI_Request array_of_requests[], MPIR_Request * request_ptrs[],
                  int *outcount, int array_of_indices[], MPI_Status array_of_statuses[]);
int MPIR_Parrived(MPIR_Request * request_ptr, int partition, int *flag);

#endif /* MPIR_REQUEST_H_INCLUDED */
