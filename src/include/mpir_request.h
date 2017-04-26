/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 */

#ifndef MPIR_REQUEST_H_INCLUDED
#define MPIR_REQUEST_H_INCLUDED

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
    MPIR_REQUEST_KIND__MPROBE, /* see NOTE-R1 */
    MPIR_REQUEST_KIND__RMA,
    MPIR_REQUEST_KIND__LAST
#ifdef MPID_REQUEST_KIND_DECL
    , MPID_REQUEST_KIND_DECL
#endif
} MPIR_Request_kind_t;

/* This currently defines a single structure type for all requests.
   Eventually, we may want a union type, as used in MPICH-1 */
/* Typedefs for Fortran generalized requests */
typedef void (MPIR_Grequest_f77_cancel_function)(void *, MPI_Fint*, MPI_Fint *);
typedef void (MPIR_Grequest_f77_free_function)(void *, MPI_Fint *);
typedef void (MPIR_Grequest_f77_query_function)(void *, MPI_Fint *, MPI_Fint *);

/* vtable-ish structure holding generalized request function pointers and other
 * state.  Saves ~48 bytes in pt2pt requests on many platforms. */
struct MPIR_Grequest_fns {
    MPI_Grequest_cancel_function *cancel_fn;
    MPI_Grequest_free_function   *free_fn;
    MPI_Grequest_query_function  *query_fn;
    MPIX_Grequest_poll_function   *poll_fn;
    MPIX_Grequest_wait_function   *wait_fn;
    void             *grequest_extra_state;
    MPIX_Grequest_class         greq_class;
    MPIR_Lang_t                  greq_lang;         /* language that defined
                                                       the generalize req */
};

typedef struct MPIR_Grequest_class {
     MPIR_OBJECT_HEADER; /* adds handle and ref_count fields */
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
    MPIR_OBJECT_HEADER; /* adds handle and ref_count fields */

    MPIR_Request_kind_t kind;

    /* pointer to the completion counter.  This is necessary for the
     * case when an operation is described by a list of requests */
    MPIR_cc_t *cc_ptr;
    /* the actual completion counter.  Ensure cc and status are in the
     * same cache line, assuming the cache line size is a multiple of
     * 32 bytes and 32-bit integers */
    MPIR_cc_t cc;

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
        } ureq; /* kind : MPIR_REQUEST_KIND__GREQUEST */
        struct {
            MPIR_Errflag_t errflag;
        } nbc;  /* kind : MPIR_REQUEST_KIND__COLL */
#if defined HAVE_DEBUGGER_SUPPORT
        struct {
            struct MPIR_Sendq *dbg_next;
        } send; /* kind : MPID_REQUEST_SEND */
#endif  /* HAVE_DEBUGGER_SUPPORT */
        struct {
#if defined HAVE_DEBUGGER_SUPPORT
            struct MPIR_Sendq *dbg_next;
#endif  /* HAVE_DEBUGGER_SUPPORT */
            /* Persistent requests have their own "real" requests */
            struct MPIR_Request *real_request;
        } persist;  /* kind : MPID_PREQUEST_SEND or MPID_PREQUEST_RECV */
    } u;

    /* Other, device-specific information */
#ifdef MPID_DEV_REQUEST_DECL
    MPID_DEV_REQUEST_DECL
#endif
    /* Collectives specific information */
#ifdef HAVE_EXT_COLL
    MPIC_REQ_DECL
#endif
};

#define MPIR_REQUEST_PREALLOC 8

extern MPIR_Object_alloc_t MPIR_Request_mem;
/* Preallocated request objects */
extern MPIR_Request MPIR_Request_direct[];

static inline MPIR_Request *MPIR_Request_create(MPIR_Request_kind_t kind)
{
    MPIR_Request *req;

    req = MPIR_Handle_obj_alloc(&MPIR_Request_mem);
    if (req != NULL) {
	MPL_DBG_MSG_P(MPIR_DBG_REQUEST,VERBOSE,
                      "allocated request, handle=0x%08x", req->handle);
#ifdef MPICH_DBG_OUTPUT
	/*MPIR_Assert(HANDLE_GET_MPI_KIND(req->handle) == MPIR_REQUEST);*/
	if (HANDLE_GET_MPI_KIND(req->handle) != MPIR_REQUEST)
	{
	    int mpi_errno;
	    mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL,
                                             FCNAME, __LINE__, MPI_ERR_OTHER,
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
	MPIR_Object_set_ref(req, 1);
	req->kind = kind;
        MPIR_cc_set(&req->cc, 1);
	req->cc_ptr		   = &req->cc;

        req->completion_notification = NULL;

	req->status.MPI_ERROR	   = MPI_SUCCESS;
        MPIR_STATUS_SET_CANCEL_BIT(req->status, FALSE);

	req->comm		   = NULL;

        switch(kind) {
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
    }
    else
    {
	/* FIXME: This fails to fail if debugging is turned off */
	MPL_DBG_MSG(MPIR_DBG_REQUEST,TYPICAL,"unable to allocate a request");
    }

    return req;
}

#define MPIR_Request_add_ref( _req ) \
    do { MPIR_Object_add_ref( _req ); } while (0)

#define MPIR_Request_release_ref( _req, _inuse ) \
    do { MPIR_Object_release_ref( _req, _inuse ); } while (0)

static inline void MPIR_Request_free(MPIR_Request *req)
{
    int inuse;

    MPIR_Request_release_ref(req, &inuse);

    /* inform the device that we are decrementing the ref-count on
     * this request */
    MPID_Request_free_hook(req);

    if (inuse == 0) {
        MPL_DBG_MSG_P(MPIR_DBG_REQUEST,VERBOSE,
                       "freeing request, handle=0x%08x", req->handle);

#ifdef MPICH_DBG_OUTPUT
        if (HANDLE_GET_MPI_KIND(req->handle) != MPIR_REQUEST)
        {
            int mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL,
                                                 FCNAME, __LINE__, MPI_ERR_OTHER,
                                                 "**invalid_handle", "**invalid_handle %d", req->handle);
            MPID_Abort(MPIR_Process.comm_world, mpi_errno, -1, NULL);
        }

        if (req->ref_count != 0)
        {
            int mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL,
                                                 FCNAME, __LINE__, MPI_ERR_OTHER,
                                                 "**invalid_refcount", "**invalid_refcount %d", req->ref_count);
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

int MPIR_Request_complete(MPI_Request *, MPIR_Request *, MPI_Status *, int *);
int MPIR_Request_get_error(MPIR_Request *);
int MPIR_Progress_wait_request(MPIR_Request *req);

/* The following routines perform the callouts to the user routines registered
   as part of a generalized request.  They handle any language binding issues
   that are necessary. They are used when completing, freeing, cancelling or
   extracting the status from a generalized request. */
int MPIR_Grequest_cancel(MPIR_Request * request_ptr, int complete);
int MPIR_Grequest_query(MPIR_Request * request_ptr);
int MPIR_Grequest_free(MPIR_Request * request_ptr);

/* this routine was added to support our extension relaxing the progress rules
 * for generalized requests */
int MPIR_Grequest_progress_poke(int count, MPIR_Request **request_ptrs,
		MPI_Status array_of_statuses[] );
int MPIR_Grequest_waitall(int count, MPIR_Request * const *  request_ptrs);

void MPIR_Grequest_complete_impl(MPIR_Request *request_ptr);
int MPIR_Grequest_start_impl(MPI_Grequest_query_function *query_fn,
                             MPI_Grequest_free_function *free_fn,
                             MPI_Grequest_cancel_function *cancel_fn,
                             void *extra_state, MPIR_Request **request_ptr);
int MPIX_Grequest_start_impl(MPI_Grequest_query_function *,
                             MPI_Grequest_free_function *,
                             MPI_Grequest_cancel_function *,
                             MPIX_Grequest_poll_function *,
                             MPIX_Grequest_wait_function *, void *,
                             MPIR_Request **);

#endif /* MPIR_REQUEST_H_INCLUDED */
