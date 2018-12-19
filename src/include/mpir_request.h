/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 */

#ifndef MPIR_REQUEST_H_INCLUDED
#define MPIR_REQUEST_H_INCLUDED

#include "mpir_process.h"

/*
=== BEGIN_MPI_T_CVAR_INFO_BLOCK ===

categories :
    - name : REQUEST
      description : A category for requests mangement variables

cvars:
    - name        : MPIR_CVAR_REQUEST_POLL_FREQ
      category    : REQUEST
      type        : int
      default     : 8
      class       : device
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_LOCAL
      description : >-
        How frequent to poll during completion calls (wait/test) in terms
        of number of processed requests before polling.

    - name        : MPIR_CVAR_REQUEST_BATCH_SIZE
      category    : REQUEST
      type        : int
      default     : 64
      class       : device
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_LOCAL
      description : >-
        The number of requests to make completion as a batch
        in MPI_Waitall and MPI_Testall implementation. A large number
        is likely to cause more cache misses.

=== END_MPI_T_CVAR_INFO_BLOCK ===
*/

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
    MPIR_REQUEST_KIND__GREQUEST,
    MPIR_REQUEST_KIND__COLL,
    MPIR_REQUEST_KIND__MPROBE,  /* see NOTE-R1 */
    MPIR_REQUEST_KIND__RMA,
    MPIR_REQUEST_KIND__LAST
#ifdef MPID_REQUEST_KIND_DECL
        , MPID_REQUEST_KIND_DECL
#endif
} MPIR_Request_kind_t;

/* This currently defines a single structure type for all requests.
   Eventually, we may want a union type, as used in MPICH-1 */
/* Typedefs for Fortran generalized requests */
typedef void (MPIR_Grequest_f77_cancel_function) (void *, MPI_Fint *, MPI_Fint *);
typedef void (MPIR_Grequest_f77_free_function) (void *, MPI_Fint *);
typedef void (MPIR_Grequest_f77_query_function) (void *, MPI_Fint *, MPI_Fint *);

/* vtable-ish structure holding generalized request function pointers and other
 * state.  Saves ~48 bytes in pt2pt requests on many platforms. */
struct MPIR_Grequest_fns {
    MPI_Grequest_cancel_function *cancel_fn;
    MPI_Grequest_free_function *free_fn;
    MPI_Grequest_query_function *query_fn;
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

#define MPIR_Request_is_complete(req_) (MPIR_cc_is_complete((req_)->cc_ptr))

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

#ifdef MPICH_THREAD_USE_MDTA
    /* Synchronization variable for wait/signal */
    MPIR_Thread_sync_t *sync;
#endif

    /* completion notification counter: this must be decremented by
     * the request completion routine, when the completion count hits
     * zero.  this counter allows us to keep track of the completion
     * of multiple requests in a single place. */
    MPIR_cc_t *completion_notification;

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
#if defined HAVE_DEBUGGER_SUPPORT
        struct {
            struct MPIR_Sendq *dbg_next;
        } send;                 /* kind : MPID_REQUEST_SEND */
#endif                          /* HAVE_DEBUGGER_SUPPORT */
        struct {
#if defined HAVE_DEBUGGER_SUPPORT
            struct MPIR_Sendq *dbg_next;
#endif                          /* HAVE_DEBUGGER_SUPPORT */
            /* Persistent requests have their own "real" requests */
            struct MPIR_Request *real_request;
        } persist;              /* kind : MPID_PREQUEST_SEND or MPID_PREQUEST_RECV */
    } u;

    /* Other, device-specific information */
#ifdef MPID_DEV_REQUEST_DECL
     MPID_DEV_REQUEST_DECL
#endif
};

#define MPIR_REQUEST_PREALLOC 8

extern MPIR_Object_alloc_t MPIR_Request_mem;
/* Preallocated request objects */
extern MPIR_Request MPIR_Request_direct[];

static inline int MPIR_Request_is_persistent(MPIR_Request * req_ptr)
{
    return (req_ptr->kind == MPIR_REQUEST_KIND__PREQUEST_SEND ||
            req_ptr->kind == MPIR_REQUEST_KIND__PREQUEST_RECV);
}

/* Return whether a request is active.
 * A persistent request and the handle to it are "inactive"
 * if the request is not associated with any ongoing communication.
 * A handle is "active" if it is neither null nor "inactive". */
static inline int MPIR_Request_is_active(MPIR_Request * req_ptr)
{
    if (req_ptr == NULL)
        return 0;
    else
        return (!MPIR_Request_is_persistent(req_ptr) || (req_ptr)->u.persist.real_request != NULL);
}

#define MPIR_REQUESTS_PROPERTY__NO_NULL        (1 << 1)
#define MPIR_REQUESTS_PROPERTY__NO_GREQUESTS   (1 << 2)
#define MPIR_REQUESTS_PROPERTY__SEND_RECV_ONLY (1 << 3)
#define MPIR_REQUESTS_PROPERTY__OPT_ALL (MPIR_REQUESTS_PROPERTY__NO_NULL          \
                                         | MPIR_REQUESTS_PROPERTY__NO_GREQUESTS   \
                                         | MPIR_REQUESTS_PROPERTY__SEND_RECV_ONLY)

static inline MPIR_Request *MPIR_Request_create(MPIR_Request_kind_t kind)
{
    MPIR_Request *req;

    req = MPIR_Handle_obj_alloc(&MPIR_Request_mem);
    if (req != NULL) {
        MPL_DBG_MSG_P(MPIR_DBG_REQUEST, VERBOSE, "allocated request, handle=0x%08x", req->handle);
#ifdef MPICH_DBG_OUTPUT
        /*MPIR_Assert(HANDLE_GET_MPI_KIND(req->handle) == MPIR_REQUEST); */
        if (HANDLE_GET_MPI_KIND(req->handle) != MPIR_REQUEST) {
            int mpi_errno;
            mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL,
                                             FCNAME, __LINE__, MPI_ERR_OTHER,
                                             "**invalid_handle", "**invalid_handle %d",
                                             req->handle);
            MPID_Abort(MPIR_Process.comm_world, mpi_errno, -1, NULL);
        }
#endif
        /* FIXME: This makes request creation expensive.  We need to
         * trim this to the basics, with additional setup for
         * special-purpose requests (think base class and
         * inheritance).  For example, do we really* want to set the
         * kind to UNDEFINED? And should the RMA values be set only
         * for RMA requests? */
        MPIR_Object_set_ref(req, 1);
        req->kind = kind;
        MPIR_cc_set(&req->cc, 1);
        req->cc_ptr = &req->cc;

        req->completion_notification = NULL;

        req->status.MPI_ERROR = MPI_SUCCESS;
        MPIR_STATUS_SET_CANCEL_BIT(req->status, FALSE);

        req->comm = NULL;
#ifdef MPICH_THREAD_USE_MDTA
        req->sync = NULL;
#endif

        switch (kind) {
            case MPIR_REQUEST_KIND__SEND:
                MPII_REQUEST_CLEAR_DBG(req);
                break;
            case MPIR_REQUEST_KIND__COLL:
                req->u.nbc.errflag = MPIR_ERR_NONE;
                break;
            default:
                break;
        }

        MPID_Request_create_hook(req);
    } else {
        /* FIXME: This fails to fail if debugging is turned off */
        MPL_DBG_MSG(MPIR_DBG_REQUEST, TYPICAL, "unable to allocate a request");
    }

    return req;
}

#define MPIR_Request_add_ref(req_p_) \
    do { MPIR_Object_add_ref(req_p_); } while (0)

#define MPIR_Request_release_ref(req_p_, inuse_) \
    do { MPIR_Object_release_ref(req_p_, inuse_); } while (0)

MPL_STATIC_INLINE_PREFIX MPIR_Request *MPIR_Request_create_complete(MPIR_Request_kind_t kind)
{
    MPIR_Request *req;

#ifdef HAVE_DEBUGGER_SUPPORT
    req = MPIR_Request_create(kind);
    MPIR_cc_set(&req->cc, 0);
#else
    req = MPIR_Process.lw_req;
    MPIR_Request_add_ref(req);
#endif

    return req;
}

static inline void MPIR_Request_free(MPIR_Request * req)
{
    int inuse;

    MPIR_Request_release_ref(req, &inuse);

    /* inform the device that we are decrementing the ref-count on
     * this request */
    MPID_Request_free_hook(req);

#ifdef MPICH_THREAD_USE_MDTA
    /* We signal the possible waiter to complete this request. */
    if (req->sync) {
        MPIR_Thread_sync_signal(req->sync, 0);
        req->sync = NULL;
    }
#endif

    if (inuse == 0) {
        MPL_DBG_MSG_P(MPIR_DBG_REQUEST, VERBOSE, "freeing request, handle=0x%08x", req->handle);

#ifdef MPICH_DBG_OUTPUT
        if (HANDLE_GET_MPI_KIND(req->handle) != MPIR_REQUEST) {
            int mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL,
                                                 FCNAME, __LINE__, MPI_ERR_OTHER,
                                                 "**invalid_handle", "**invalid_handle %d",
                                                 req->handle);
            MPID_Abort(MPIR_Process.comm_world, mpi_errno, -1, NULL);
        }

        if (req->ref_count != 0) {
            int mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL,
                                                 FCNAME, __LINE__, MPI_ERR_OTHER,
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
            MPIR_Comm_release(req->comm);
        }

        if (req->kind == MPIR_REQUEST_KIND__GREQUEST && req->u.ureq.greq_fns != NULL) {
            MPL_free(req->u.ureq.greq_fns);
        }

        MPID_Request_destroy_hook(req);

        MPIR_Handle_obj_free(&MPIR_Request_mem, req);
    }
}

#ifdef MPICH_THREAD_USE_MDTA
MPL_STATIC_INLINE_PREFIX void MPIR_Request_attach_sync(MPIR_Request * req_ptr,
                                                       MPIR_Thread_sync_t * sync)
{
    req_ptr->sync = sync;
    if (MPIR_Request_is_persistent(req_ptr)) {
        req_ptr->u.persist.real_request->sync = sync;
    }
}
#endif

/* The "fastpath" version of MPIR_Request_completion_processing.  It only handles
 * MPIR_REQUEST_KIND__SEND and MPIR_REQUEST_KIND__RECV kinds, and it does not attempt to
 * deal with status structures under the assumption that bleeding fast code will
 * pass either MPI_STATUS_IGNORE or MPI_STATUSES_IGNORE as appropriate.  This
 * routine (or some a variation of it) is an unfortunately necessary stunt to
 * get high message rates on key benchmarks for high-end systems.
 */
#undef FUNCNAME
#define FUNCNAME MPIR_Request_completion_processing_fastpath
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPIR_Request_completion_processing_fastpath(MPI_Request * request,
                                                                         MPIR_Request * request_ptr)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_Assert(request_ptr->kind == MPIR_REQUEST_KIND__SEND ||
                request_ptr->kind == MPIR_REQUEST_KIND__RECV);

    if (request_ptr->kind == MPIR_REQUEST_KIND__SEND) {
        /* FIXME: are Ibsend requests added to the send queue? */
        MPII_SENDQ_FORGET(request_ptr);
    }

    /* the completion path for SEND and RECV is the same at this time, modulo
     * the SENDQ hook above */
    mpi_errno = request_ptr->status.MPI_ERROR;
    MPIR_Request_free(request_ptr);
    *request = MPI_REQUEST_NULL;

    return mpi_errno;
}

int MPIR_Request_completion_processing(MPIR_Request *, MPI_Status *, int *);
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

void MPIR_Grequest_complete(MPIR_Request * request_ptr);
int MPIR_Grequest_start(MPI_Grequest_query_function * query_fn,
                        MPI_Grequest_free_function * free_fn,
                        MPI_Grequest_cancel_function * cancel_fn,
                        void *extra_state, MPIR_Request ** request_ptr);
int MPIX_Grequest_start_impl(MPI_Grequest_query_function *,
                             MPI_Grequest_free_function *,
                             MPI_Grequest_cancel_function *,
                             MPIX_Grequest_poll_function *,
                             MPIX_Grequest_wait_function *, void *, MPIR_Request **);

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
    return (request_ptr->u.ureq.greq_fns->wait_fn) (1,
                                                    &request_ptr->u.ureq.greq_fns->
                                                    grequest_extra_state, 0, status);
}

MPL_STATIC_INLINE_PREFIX int MPIR_Grequest_poll(MPIR_Request * request_ptr, MPI_Status * status)
{
    return (request_ptr->u.ureq.greq_fns->poll_fn) (request_ptr->u.ureq.
                                                    greq_fns->grequest_extra_state, status);
}

int MPIR_Test_impl(MPIR_Request * request, int *flag, MPI_Status * status);
int MPIR_Testall_impl(int count, MPIR_Request * request_ptrs[], int *flag,
                      MPI_Status array_of_statuses[], int requests_property);
int MPIR_Testany_impl(int count, MPIR_Request * request_ptrs[],
                      int *indx, int *flag, MPI_Status * status);
int MPIR_Testsome_impl(int incount, MPIR_Request * request_ptrs[],
                       int *outcount, int array_of_indices[], MPI_Status array_of_statuses[]);

int MPIR_Wait_impl(MPIR_Request * request_ptr, MPI_Status * status);
int MPIR_Waitall_impl(int count, MPIR_Request * request_ptrs[], MPI_Status array_of_statuses[],
                      int request_properties);
int MPIR_Waitany_impl(int count, MPIR_Request * request_ptrs[], int *indx, MPI_Status * status);
int MPIR_Waitsome_impl(int incount, MPIR_Request * request_ptrs[],
                       int *outcount, int array_of_indices[], MPI_Status array_of_statuses[]);

int MPIR_Test(MPI_Request * request, int *flag, MPI_Status * status);
int MPIR_Testall(int count, MPI_Request array_of_requests[], int *flag,
                 MPI_Status array_of_statuses[]);
int MPIR_Wait(MPI_Request * request, MPI_Status * status);
int MPIR_Waitall(int count, MPI_Request array_of_requests[], MPI_Status array_of_statuses[]);

#endif /* MPIR_REQUEST_H_INCLUDED */
