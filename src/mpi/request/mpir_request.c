/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* style:PMPIuse:PMPI_Status_f2c:2 sig:0 */

MPIR_Request MPIR_Request_direct[MPIR_REQUEST_PREALLOC];

MPIR_Object_alloc_t MPIR_Request_mem = {
    0, 0, 0, 0, MPIR_REQUEST, sizeof(MPIR_Request), MPIR_Request_direct,
    MPIR_REQUEST_PREALLOC
};

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
        case MPIR_REQUEST_KIND__RECV:
        case MPIR_REQUEST_KIND__COLL:
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
                        /* This is needed for persistent Bsend requests */
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
            rc = (request_ptr->u.ureq.greq_fns->cancel_fn) (request_ptr->u.ureq.
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

                ((MPIR_Grequest_f77_cancel_function *) (request_ptr->u.ureq.
                                                        greq_fns->cancel_fn)) (request_ptr->u.ureq.
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
            rc = (request_ptr->u.ureq.greq_fns->query_fn) (request_ptr->u.ureq.
                                                           greq_fns->grequest_extra_state,
                                                           &request_ptr->status);
            MPIR_ERR_CHKANDSTMT1((rc != MPI_SUCCESS), mpi_errno, MPI_ERR_OTHER, {;}
                                 , "**user", "**userquery %d", rc);
            break;
#ifdef HAVE_FORTRAN_BINDING
        case MPIR_LANG__FORTRAN:
        case MPIR_LANG__FORTRAN90:
            {
                MPI_Fint ierr;
                MPI_Fint is[sizeof(MPI_Status) / sizeof(int)];
                ((MPIR_Grequest_f77_query_function *) (request_ptr->u.ureq.
                                                       greq_fns->query_fn)) (request_ptr->u.ureq.
                                                                             greq_fns->grequest_extra_state,
                                                                             is, &ierr);
                rc = (int) ierr;
                if (rc == MPI_SUCCESS)
                    PMPI_Status_f2c(is, &request_ptr->status);
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
            rc = (request_ptr->u.ureq.greq_fns->free_fn) (request_ptr->u.ureq.
                                                          greq_fns->grequest_extra_state);
            MPIR_ERR_CHKANDSTMT1((rc != MPI_SUCCESS), mpi_errno, MPI_ERR_OTHER, {;}
                                 , "**user", "**userfree %d", rc);
            break;
#ifdef HAVE_FORTRAN_BINDING
        case MPIR_LANG__FORTRAN:
        case MPIR_LANG__FORTRAN90:
            {
                MPI_Fint ierr;

                ((MPIR_Grequest_f77_free_function *) (request_ptr->u.ureq.
                                                      greq_fns->free_fn)) (request_ptr->u.ureq.
                                                                           greq_fns->grequest_extra_state,
                                                                           &ierr);
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
