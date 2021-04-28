/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"

/* style:PMPIuse:PMPI_Status_f2c:2 sig:0 */

MPIR_Request MPIR_Request_builtins[MPIR_REQUEST_BUILTIN_COUNT];
MPIR_Request MPIR_Request_direct[MPIR_REQUEST_PREALLOC];
MPIR_Object_alloc_t MPIR_Request_mem[MPIR_REQUEST_NUM_POOLS];

static void init_builtin_request(MPIR_Request * req, int handle, MPIR_Request_kind_t kind)
{
    req->handle = handle;
    req->kind = kind;
    MPIR_cc_set(&req->cc, 0);
    req->cc_ptr = &req->cc;
    req->completion_notification = NULL;
    req->status.MPI_ERROR = MPI_SUCCESS;
    MPIR_STATUS_SET_CANCEL_BIT(req->status, FALSE);
    req->comm = NULL;
}

void MPII_init_request(void)
{
    MPID_Thread_mutex_t *lock_ptr = NULL;
#if MPICH_THREAD_GRANULARITY == MPICH_THREAD_GRANULARITY__VCI
    lock_ptr = &MPIR_THREAD_VCI_HANDLE_MUTEX;
#endif

    /* *INDENT-OFF* */
    MPIR_Request_mem[0] = (MPIR_Object_alloc_t) { 0, 0, 0, 0, 0, 0, MPIR_REQUEST, sizeof(MPIR_Request), MPIR_Request_direct, MPIR_REQUEST_PREALLOC, lock_ptr, {0}};
    for (int i = 1; i < MPIR_REQUEST_NUM_POOLS; i++) {
        MPIR_Request_mem[i] = (MPIR_Object_alloc_t) { 0, 0, 0, 0, 0, 0, MPIR_REQUEST, sizeof(MPIR_Request), NULL, 0, lock_ptr, {0}};
    }
    /* *INDENT-ON* */

    MPIR_Request *req;

    /* lightweight request, one for each kind */
    for (int i = 0; i < MPIR_REQUEST_KIND__LAST; i++) {
        req = &MPIR_Request_builtins[i];
        init_builtin_request(req, MPIR_REQUEST_COMPLETE + i, (MPIR_Request_kind_t) i);
    }
    MPII_REQUEST_CLEAR_DBG(&MPIR_Request_builtins[MPIR_REQUEST_KIND__SEND]);
    MPIR_Request_builtins[MPIR_REQUEST_KIND__COLL].u.nbc.errflag = MPIR_ERR_NONE;
    MPIR_Request_builtins[MPIR_REQUEST_KIND__COLL].u.nbc.coll.host_sendbuf = NULL;
    MPIR_Request_builtins[MPIR_REQUEST_KIND__COLL].u.nbc.coll.host_recvbuf = NULL;
    MPIR_Request_builtins[MPIR_REQUEST_KIND__COLL].u.nbc.coll.datatype = MPI_DATATYPE_NULL;

    /* for recv from MPI_PROC_NULL */
    req = MPIR_Request_builtins + HANDLE_INDEX(MPIR_REQUEST_NULL_RECV);
    init_builtin_request(req, MPIR_REQUEST_NULL_RECV, MPIR_REQUEST_KIND__RECV);
    MPIR_Status_set_procnull(&req->status);
}

/* Complete a request, saving the status data if necessary.
   If debugger information is being provided for pending (user-initiated)
   send operations, the macros MPII_SENDQ_FORGET will be defined to
   call the routine MPII_Sendq_forget; otherwise that macro will be a no-op.
   The implementation of the MPIR_Sendq_xxx is in src/mpi/debugger/dbginit.c .
*/
int MPIR_Request_completion_processing(MPIR_Request * request_ptr, MPI_Status * status)
{
    int mpi_errno = MPI_SUCCESS;
    int rc;

    switch (request_ptr->kind) {
        case MPIR_REQUEST_KIND__SEND:
            {
                MPIR_Status_set_cancel_bit(status, MPIR_STATUS_GET_CANCEL_BIT(request_ptr->status));
                mpi_errno = request_ptr->status.MPI_ERROR;
                MPII_SENDQ_FORGET(request_ptr);
                break;
            }
        case MPIR_REQUEST_KIND__COLL:
            {
                MPII_Coll_req_t *coll = &request_ptr->u.nbc.coll;

                if (coll->host_sendbuf) {
                    MPIR_gpu_host_free(coll->host_sendbuf, coll->count, coll->datatype);
                }

                if (coll->host_recvbuf) {
                    MPIR_gpu_swap_back(coll->host_recvbuf, coll->user_recvbuf,
                                       coll->count, coll->datatype);
                    MPIR_Datatype_release_if_not_builtin(coll->datatype);
                }

                MPIR_Request_extract_status(request_ptr, status);
                mpi_errno = request_ptr->status.MPI_ERROR;
                break;
            }
        case MPIR_REQUEST_KIND__RECV:
        case MPIR_REQUEST_KIND__RMA:
            {
                MPIR_Request_extract_status(request_ptr, status);
                mpi_errno = request_ptr->status.MPI_ERROR;
                break;
            }

        case MPIR_REQUEST_KIND__PREQUEST_SEND:
            {
                if (request_ptr->u.persist.real_request != NULL) {
                    MPIR_Request *prequest_ptr = request_ptr->u.persist.real_request;

                    /* reset persistent request to inactive state */
                    MPIR_cc_set(&request_ptr->cc, 0);
                    request_ptr->cc_ptr = &request_ptr->cc;
                    request_ptr->u.persist.real_request = NULL;

                    if (prequest_ptr->kind != MPIR_REQUEST_KIND__GREQUEST) {
                        MPIR_Status_set_cancel_bit(status,
                                                   MPIR_STATUS_GET_CANCEL_BIT((prequest_ptr->status)));
                        mpi_errno = prequest_ptr->status.MPI_ERROR;
                    } else {
                        mpi_errno = MPIR_Grequest_query(prequest_ptr);
                        MPIR_Status_set_cancel_bit(status,
                                                   MPIR_STATUS_GET_CANCEL_BIT
                                                   (prequest_ptr->status));
                        if (mpi_errno == MPI_SUCCESS) {
                            mpi_errno = prequest_ptr->status.MPI_ERROR;
                        }
                        rc = MPIR_Grequest_free(prequest_ptr);
                        if (mpi_errno == MPI_SUCCESS) {
                            mpi_errno = rc;
                        }
                    }

                    MPIR_Request_free(prequest_ptr);
                } else {
                    if (request_ptr->status.MPI_ERROR != MPI_SUCCESS) {
                        /* if the persistent request failed to start then make the
                         * error code available */
                        MPIR_Status_set_cancel_bit(status, FALSE);
                        mpi_errno = request_ptr->status.MPI_ERROR;
                    } else {
                        MPIR_Status_set_empty(status);
                    }
                }

                break;
            }

        case MPIR_REQUEST_KIND__PREQUEST_RECV:
            {
                if (request_ptr->u.persist.real_request != NULL) {
                    MPIR_Request *prequest_ptr = request_ptr->u.persist.real_request;

                    /* reset persistent request to inactive state */
                    MPIR_cc_set(&request_ptr->cc, 0);
                    request_ptr->cc_ptr = &request_ptr->cc;
                    request_ptr->u.persist.real_request = NULL;

                    MPIR_Request_extract_status(prequest_ptr, status);
                    mpi_errno = prequest_ptr->status.MPI_ERROR;

                    MPIR_Request_free(prequest_ptr);
                } else {
                    MPIR_Status_set_empty(status);
                    /* --BEGIN ERROR HANDLING-- */
                    if (request_ptr->status.MPI_ERROR != MPI_SUCCESS) {
                        /* if the persistent request failed to start then make the
                         * error code available */
                        mpi_errno = request_ptr->status.MPI_ERROR;
                    }
                    /* --END ERROR HANDLING-- */
                }

                break;
            }

        case MPIR_REQUEST_KIND__PREQUEST_COLL:
            {
                if (request_ptr->u.persist.real_request != NULL) {
                    MPIR_Request *prequest_ptr = request_ptr->u.persist.real_request;

                    /* reset persistent request to inactive state */
                    MPIR_cc_set(&request_ptr->cc, 0);
                    request_ptr->cc_ptr = &request_ptr->cc;
                    request_ptr->u.persist.real_request = NULL;

                    MPIR_Request_extract_status(prequest_ptr, status);
                    mpi_errno = prequest_ptr->status.MPI_ERROR;

                    MPIR_Request_free(prequest_ptr);
                } else {
                    MPIR_Status_set_empty(status);
                    /* --BEGIN ERROR HANDLING-- */
                    if (request_ptr->status.MPI_ERROR != MPI_SUCCESS) {
                        /* if the persistent request failed to start then make the
                         * error code available */
                        mpi_errno = request_ptr->status.MPI_ERROR;
                    }
                    /* --END ERROR HANDLING-- */
                }

                break;
            }

        case MPIR_REQUEST_KIND__PART_SEND:
        case MPIR_REQUEST_KIND__PART_RECV:
            {
                MPIR_Part_request_inactivate(request_ptr);

                MPIR_Request_extract_status(request_ptr, status);
                mpi_errno = request_ptr->status.MPI_ERROR;
                break;
            }

        case MPIR_REQUEST_KIND__GREQUEST:
            {
                mpi_errno = MPIR_Grequest_query(request_ptr);
                MPIR_Request_extract_status(request_ptr, status);
                rc = MPIR_Grequest_free(request_ptr);
                if (mpi_errno == MPI_SUCCESS) {
                    mpi_errno = rc;
                }

                break;
            }

        default:
            {
                /* --BEGIN ERROR HANDLING-- */
                /* This should not happen */
                MPIR_ERR_SETANDSTMT1(mpi_errno, MPI_ERR_INTERN,;, "**badcase", "**badcase %d",
                                     request_ptr->kind);
                break;
                /* --END ERROR HANDLING-- */
            }
    }

    return mpi_errno;
}

int MPIR_Request_handle_proc_failed(MPIR_Request * request_ptr)
{
    if (request_ptr->kind == MPIR_REQUEST_KIND__RECV) {
        /* We only handle receive request at the moment. */
        MPID_Cancel_recv(request_ptr);
        MPIR_STATUS_SET_CANCEL_BIT(request_ptr->status, FALSE);
    }
    MPIR_ERR_SET(request_ptr->status.MPI_ERROR, MPIX_ERR_PROC_FAILED, "**proc_failed");
    return request_ptr->status.MPI_ERROR;
}

/* This routine is for obtaining the error code of an existing request.
 * It is similar as MPIR_Request_completion_processing without any "free" + status setting.
 *
 * It is only needed by MPI_Testall because if not all request are completed then (quote):
 * "neither the array_of_requests nor the array_of_statuses is modified.
 * If one or more of the requests completes with an error, MPI_ERR_IN_STATUS is returned".
 * Therefore, we have to get the error code without modifying the request.
 */
int MPIR_Request_get_error(MPIR_Request * request_ptr)
{
    int mpi_errno = MPI_SUCCESS;

    switch (request_ptr->kind) {
        case MPIR_REQUEST_KIND__SEND:
        case MPIR_REQUEST_KIND__RECV:
        case MPIR_REQUEST_KIND__COLL:
        case MPIR_REQUEST_KIND__RMA:
            {
                mpi_errno = request_ptr->status.MPI_ERROR;
                break;
            }

        case MPIR_REQUEST_KIND__PREQUEST_SEND:
            {
                if (request_ptr->u.persist.real_request != NULL) {
                    if (request_ptr->u.persist.real_request->kind == MPIR_REQUEST_KIND__GREQUEST) {
                        /* This is needed for persistent Bsend requests */
                        mpi_errno = MPIR_Grequest_query(request_ptr->u.persist.real_request);
                    }
                    if (mpi_errno == MPI_SUCCESS) {
                        mpi_errno = request_ptr->u.persist.real_request->status.MPI_ERROR;
                    }
                } else {
                    mpi_errno = request_ptr->status.MPI_ERROR;
                }

                break;
            }

        case MPIR_REQUEST_KIND__PREQUEST_RECV:
            {
                if (request_ptr->u.persist.real_request != NULL) {
                    mpi_errno = request_ptr->u.persist.real_request->status.MPI_ERROR;
                } else {
                    mpi_errno = request_ptr->status.MPI_ERROR;
                }

                break;
            }

        case MPIR_REQUEST_KIND__GREQUEST:
            {
                mpi_errno = MPIR_Grequest_query(request_ptr);

                break;
            }

        default:
            {
                /* --BEGIN ERROR HANDLING-- */
                /* This should not happen */
                MPIR_ERR_SETANDSTMT1(mpi_errno, MPI_ERR_INTERN,;, "**badcase", "**badcase %d",
                                     request_ptr->kind);
                break;
                /* --END ERROR HANDLING-- */
            }
    }

    return mpi_errno;
}

#ifdef HAVE_FORTRAN_BINDING
/* Set the language type to Fortran for this (generalized) request */
void MPII_Grequest_set_lang_f77(MPI_Request greq)
{
    MPIR_Request *greq_ptr;

    MPIR_Request_get_ptr(greq, greq_ptr);

    greq_ptr->u.ureq.greq_fns->greq_lang = MPIR_LANG__FORTRAN;
}
#endif


int MPIR_Grequest_cancel(MPIR_Request * request_ptr, int complete)
{
    int rc;
    int mpi_errno = MPI_SUCCESS;

    switch (request_ptr->u.ureq.greq_fns->greq_lang) {
        case MPIR_LANG__C:
#ifdef HAVE_CXX_BINDING
        case MPIR_LANG__CXX:
#endif
            rc = (request_ptr->u.ureq.greq_fns->U.C.cancel_fn) (request_ptr->u.ureq.
                                                                greq_fns->grequest_extra_state,
                                                                complete);
            MPIR_ERR_CHKANDSTMT1((rc != MPI_SUCCESS), mpi_errno, MPI_ERR_OTHER,;, "**user",
                                 "**usercancel %d", rc);
            break;
#ifdef HAVE_FORTRAN_BINDING
        case MPIR_LANG__FORTRAN:
        case MPIR_LANG__FORTRAN90:
            {
                MPI_Fint ierr;
                MPI_Fint icomplete = complete;

                (request_ptr->u.ureq.greq_fns->U.F.cancel_fn) (request_ptr->u.ureq.
                                                               greq_fns->grequest_extra_state,
                                                               &icomplete, &ierr);
                rc = (int) ierr;
                MPIR_ERR_CHKANDSTMT1((rc != MPI_SUCCESS), mpi_errno, MPI_ERR_OTHER, {;}, "**user",
                                     "**usercancel %d", rc);
                break;
            }
#endif

        default:
            {
                /* --BEGIN ERROR HANDLING-- */
                /* This should not happen */
                MPIR_ERR_SETANDSTMT1(mpi_errno, MPI_ERR_INTERN,;, "**badcase",
                                     "**badcase %d", request_ptr->u.ureq.greq_fns->greq_lang);
                break;
                /* --END ERROR HANDLING-- */
            }
    }

    return mpi_errno;
}


int MPIR_Grequest_query(MPIR_Request * request_ptr)
{
    int rc;
    int mpi_errno = MPI_SUCCESS;

    switch (request_ptr->u.ureq.greq_fns->greq_lang) {
        case MPIR_LANG__C:
#ifdef HAVE_CXX_BINDING
        case MPIR_LANG__CXX:
#endif
            /* Take off the global locks before calling user functions */
            MPID_THREAD_CS_EXIT(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
            rc = (request_ptr->u.ureq.greq_fns->U.C.query_fn) (request_ptr->u.ureq.
                                                               greq_fns->grequest_extra_state,
                                                               &request_ptr->status);
            MPID_THREAD_CS_ENTER(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
            MPIR_ERR_CHKANDSTMT1((rc != MPI_SUCCESS), mpi_errno, MPI_ERR_OTHER, {;}
                                 , "**user", "**userquery %d", rc);
            break;
#ifdef HAVE_FORTRAN_BINDING
        case MPIR_LANG__FORTRAN:
        case MPIR_LANG__FORTRAN90:
            {
                MPI_Fint ierr;
                MPI_Fint is[sizeof(MPI_Status) / sizeof(int)];
                /* Take off the global locks before calling user functions */
                MPID_THREAD_CS_EXIT(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
                (request_ptr->u.ureq.greq_fns->U.F.query_fn) (request_ptr->u.ureq.
                                                              greq_fns->grequest_extra_state, is,
                                                              &ierr);
                rc = (int) ierr;
                if (rc == MPI_SUCCESS)
                    PMPI_Status_f2c(is, &request_ptr->status);
                MPID_THREAD_CS_ENTER(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
                MPIR_ERR_CHKANDSTMT1((rc != MPI_SUCCESS), mpi_errno, MPI_ERR_OTHER, {;}
                                     , "**user", "**userquery %d", rc);
            }
            break;
#endif
        default:
            {
                /* --BEGIN ERROR HANDLING-- */
                /* This should not happen */
                MPIR_ERR_SETANDSTMT1(mpi_errno, MPI_ERR_INTERN,;, "**badcase",
                                     "**badcase %d", request_ptr->u.ureq.greq_fns->greq_lang);
                break;
                /* --END ERROR HANDLING-- */
            }
    }

    return mpi_errno;
}


int MPIR_Grequest_free(MPIR_Request * request_ptr)
{
    int rc;
    int mpi_errno = MPI_SUCCESS;

    switch (request_ptr->u.ureq.greq_fns->greq_lang) {
        case MPIR_LANG__C:
#ifdef HAVE_CXX_BINDING
        case MPIR_LANG__CXX:
#endif
            rc = (request_ptr->u.ureq.greq_fns->U.C.free_fn) (request_ptr->u.ureq.
                                                              greq_fns->grequest_extra_state);
            MPIR_ERR_CHKANDSTMT1((rc != MPI_SUCCESS), mpi_errno, MPI_ERR_OTHER, {;}
                                 , "**user", "**userfree %d", rc);
            break;
#ifdef HAVE_FORTRAN_BINDING
        case MPIR_LANG__FORTRAN:
        case MPIR_LANG__FORTRAN90:
            {
                MPI_Fint ierr;

                (request_ptr->u.ureq.greq_fns->U.F.free_fn) (request_ptr->u.ureq.
                                                             greq_fns->grequest_extra_state, &ierr);
                rc = (int) ierr;
                MPIR_ERR_CHKANDSTMT1((rc != MPI_SUCCESS), mpi_errno, MPI_ERR_OTHER, {;}, "**user",
                                     "**userfree %d", rc);
                break;
            }
#endif

        default:
            {
                /* --BEGIN ERROR HANDLING-- */
                /* This should not happen */
                MPIR_ERR_SETANDSTMT1(mpi_errno, MPI_ERR_INTERN, {;}
                                     , "**badcase",
                                     "**badcase %d", request_ptr->u.ureq.greq_fns->greq_lang);
                break;
                /* --END ERROR HANDLING-- */
            }
    }

    return mpi_errno;
}
