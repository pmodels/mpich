/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* style:PMPIuse:PMPI_Status_f2c:2 sig:0 */

MPIR_Request MPIR_Request_direct[MPIR_REQUEST_PREALLOC] = { {0}
};

MPIR_Object_alloc_t MPIR_Request_mem = {
    0, 0, 0, 0, MPIR_REQUEST, sizeof(MPIR_Request), MPIR_Request_direct,
    MPIR_REQUEST_PREALLOC
};

#undef FUNCNAME
#define FUNCNAME MPIR_Progress_wait_request
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
/*@
  MPIR_Progress_wait_request

  A helper routine that implements the very common case of running the progress
  engine until the given request is complete.
  @*/
int MPIR_Progress_wait_request(MPIR_Request * req)
{
    int mpi_errno = MPI_SUCCESS;

    if (!MPIR_Request_is_complete(req)) {
        MPID_Progress_state progress_state;

        MPID_Progress_start(&progress_state);
        while (!MPIR_Request_is_complete(req)) {
            mpi_errno = MPID_Progress_wait(&progress_state);
            if (mpi_errno != MPI_SUCCESS) {
                /* --BEGIN ERROR HANDLING-- */
                MPID_Progress_end(&progress_state);
                if (mpi_errno)
                    MPIR_ERR_POP(mpi_errno);
                /* --END ERROR HANDLING-- */
            }
        }
        MPID_Progress_end(&progress_state);
    }

  fn_fail:
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIR_Request_completion_processing
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
/* Complete a request, saving the status data if necessary.
   "active" has meaning only if the request is a persistent request; this
   allows the completion routines to indicate that a persistent request
   was inactive and did not require any extra completion operation.

   If debugger information is being provided for pending (user-initiated)
   send operations, the macros MPII_SENDQ_FORGET will be defined to
   call the routine MPII_Sendq_forget; otherwise that macro will be a no-op.
   The implementation of the MPIR_Sendq_xxx is in src/mpi/debugger/dbginit.c .
*/
int MPIR_Request_completion_processing(MPI_Request * request, MPIR_Request * request_ptr,
                                       MPI_Status * status, int *active)
{
    int mpi_errno = MPI_SUCCESS;

    *active = TRUE;
    switch (request_ptr->kind) {
        case MPIR_REQUEST_KIND__SEND:
            {
                if (status != MPI_STATUS_IGNORE) {
                    MPIR_STATUS_SET_CANCEL_BIT(*status,
                                               MPIR_STATUS_GET_CANCEL_BIT(request_ptr->status));
                }
                mpi_errno = request_ptr->status.MPI_ERROR;
                MPII_SENDQ_FORGET(request_ptr);
                MPIR_Request_free(request_ptr);
                if (NULL != request)
                    *request = MPI_REQUEST_NULL;
                break;
            }
        case MPIR_REQUEST_KIND__RECV:
            {
                MPIR_Request_extract_status(request_ptr, status);
                mpi_errno = request_ptr->status.MPI_ERROR;
                MPIR_Request_free(request_ptr);
                if (NULL != request)
                    *request = MPI_REQUEST_NULL;
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
                        if (status != MPI_STATUS_IGNORE) {
                            MPIR_STATUS_SET_CANCEL_BIT(*status,
                                                       MPIR_STATUS_GET_CANCEL_BIT
                                                       (prequest_ptr->status));
                        }
                        mpi_errno = prequest_ptr->status.MPI_ERROR;
                    } else {
                        /* This is needed for persistent Bsend requests */
                        int rc;

                        rc = MPIR_Grequest_query(prequest_ptr);
                        if (mpi_errno == MPI_SUCCESS) {
                            mpi_errno = rc;
                        }
                        if (status != MPI_STATUS_IGNORE) {
                            MPIR_STATUS_SET_CANCEL_BIT(*status,
                                                       MPIR_STATUS_GET_CANCEL_BIT
                                                       (prequest_ptr->status));
                        }
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
                        if (status != MPI_STATUS_IGNORE) {
                            MPIR_STATUS_SET_CANCEL_BIT(*status, FALSE);
                        }
                        mpi_errno = request_ptr->status.MPI_ERROR;
                    } else {
                        MPIR_Status_set_empty(status);
                        *active = FALSE;
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
                    } else {
                        *active = FALSE;
                    }
                    /* --END ERROR HANDLING-- */
                }

                break;
            }

        case MPIR_REQUEST_KIND__GREQUEST:
            {
                int rc;

                rc = MPIR_Grequest_query(request_ptr);
                if (mpi_errno == MPI_SUCCESS) {
                    mpi_errno = rc;
                }
                MPIR_Request_extract_status(request_ptr, status);
                rc = MPIR_Grequest_free(request_ptr);
                if (mpi_errno == MPI_SUCCESS) {
                    mpi_errno = rc;
                }

                MPIR_Request_free(request_ptr);
                if (NULL != request)
                    *request = MPI_REQUEST_NULL;

                break;
            }

        case MPIR_REQUEST_KIND__COLL:
        case MPIR_REQUEST_KIND__RMA:
            {
                mpi_errno = request_ptr->status.MPI_ERROR;
                MPIR_Request_extract_status(request_ptr, status);
                MPIR_Request_free(request_ptr);
                if (NULL != request)
                    *request = MPI_REQUEST_NULL;
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


#undef FUNCNAME
#define FUNCNAME MPIR_Request_get_error
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
/* FIXME: What is this routine for?
 *
 * [BRT] it is used by testall, although looking at testall now, I think the
 * algorithm can be change slightly and eliminate the need for this routine
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
                int rc;

                /* Note that we've acquired the thread private storage above */

                switch (request_ptr->u.ureq.greq_fns->greq_lang) {
                    case MPIR_LANG__C:
#ifdef HAVE_CXX_BINDING
                    case MPIR_LANG__CXX:
#endif
                        rc = (request_ptr->u.ureq.greq_fns->query_fn) (request_ptr->u.
                                                                       ureq.greq_fns->grequest_extra_state,
                                                                       &request_ptr->status);
                        MPIR_ERR_CHKANDSTMT1((rc != MPI_SUCCESS), mpi_errno, MPI_ERR_OTHER,;,
                                             "**user", "**userquery %d", rc);
                        break;
#ifdef HAVE_FORTRAN_BINDING
                    case MPIR_LANG__FORTRAN:
                    case MPIR_LANG__FORTRAN90:
                        {
                            MPI_Fint ierr;
                            MPI_Fint is[sizeof(MPI_Status) / sizeof(int)];
                            ((MPIR_Grequest_f77_query_function *) (request_ptr->u.ureq.
                                                                   greq_fns->query_fn))
                                (request_ptr->u.ureq.greq_fns->grequest_extra_state, is, &ierr);
                            rc = (int) ierr;
                            if (rc == MPI_SUCCESS)
                                PMPI_Status_f2c(is, &request_ptr->status);
                            MPIR_ERR_CHKANDSTMT1((rc != MPI_SUCCESS), mpi_errno, MPI_ERR_OTHER,;,
                                                 "**user", "**userquery %d", rc);
                            break;
                        }
#endif

                    default:
                        {
                            /* --BEGIN ERROR HANDLING-- */
                            /* This should not happen */
                            MPIR_ERR_SETANDSTMT1(mpi_errno, MPI_ERR_INTERN,;,
                                                 "**badcase",
                                                 "**badcase %d",
                                                 request_ptr->u.ureq.greq_fns->greq_lang);
                            break;
                            /* --END ERROR HANDLING-- */
                        }
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

#ifdef HAVE_FORTRAN_BINDING
/* Set the language type to Fortran for this (generalized) request */
void MPII_Grequest_set_lang_f77(MPI_Request greq)
{
    MPIR_Request *greq_ptr;

    MPIR_Request_get_ptr(greq, greq_ptr);

    greq_ptr->u.ureq.greq_fns->greq_lang = MPIR_LANG__FORTRAN;
}
#endif


#undef FUNCNAME
#define FUNCNAME MPIR_Grequest_cancel
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
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


#undef FUNCNAME
#define FUNCNAME MPIR_Grequest_query
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
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


#undef FUNCNAME
#define FUNCNAME MPIR_Grequest_free
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
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

/* MPIR_Grequest_progress_poke:  Drives progess on generalized requests.
 * Invokes poll_fn for each request in request_ptrs.  Waits for completion of
 * multiple requests if possible (all outstanding generalized requests are of
 * same greq class) */
#undef FUNCNAME
#define FUNCNAME MPIR_Grequest_progress_poke
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Grequest_progress_poke(int count,
                                MPIR_Request ** request_ptrs, MPI_Status array_of_statuses[])
{
    void **state_ptrs;
    int i;
    int mpi_errno = MPI_SUCCESS;
    int made_progress = 0;
    MPIR_CHKLMEM_DECL(1);

    MPIR_CHKLMEM_MALLOC(state_ptrs, void **, sizeof(void *) * count, mpi_errno, "state_ptrs",
                        MPL_MEM_GREQ);

    /* TODO: Implement the class-wise completion optimization described in:
     * Latham, Robert, et al. "Extending the MPI-2 generalized request interface."
     * PVM/MPI 4757 (2007): 223-232.
     * This was implemented previously using a class counter and doing a class-wise completion
     * with wait_fn, but it made progress_poke potentially blocking and might hang, especially
     * in multithreading environments. This optimization would be benificial but needs to be
     * implemented correctly. */

    for (i = 0; i < count; i++) {
        if (request_ptrs[i] != NULL &&
            request_ptrs[i]->kind == MPIR_REQUEST_KIND__GREQUEST &&
            !MPIR_Request_is_complete(request_ptrs[i]) &&
            request_ptrs[i]->u.ureq.greq_fns->poll_fn != NULL) {
            mpi_errno =
                (request_ptrs[i]->u.ureq.greq_fns->poll_fn) (request_ptrs[i]->u.ureq.
                                                             greq_fns->grequest_extra_state,
                                                             &(array_of_statuses[i]));
            if (mpi_errno)
                MPIR_ERR_POP(mpi_errno);
            if (MPIR_Request_is_complete(request_ptrs[i]))
                made_progress = 1;
        }
    }

    if (!made_progress) {
        MPID_THREAD_CS_YIELD(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
        MPID_THREAD_CS_YIELD(POBJ, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    }

  fn_exit:
    MPIR_CHKLMEM_FREEALL();
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

/* MPIR_Grequest_wait: Waits until all generalized requests have
   completed.  This routine groups grequests by class and calls the
   wait_fn on the whole class. */
#undef FUNCNAME
#define FUNCNAME MPIR_Grequest_waitall
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Grequest_waitall(int count, MPIR_Request * const *request_ptrs)
{
    void **state_ptrs;
    int i;
    int mpi_error = MPI_SUCCESS;
    MPID_Progress_state progress_state;
    MPIR_CHKLMEM_DECL(1);

    MPIR_CHKLMEM_MALLOC(state_ptrs, void *, sizeof(void *) * count, mpi_error, "state_ptrs",
                        MPL_MEM_GREQ);

    /* DISABLED CODE: The greq wait_fn function returns when ANY
     * of the requests completes, rather than all.  Also, once a
     * greq has completed, you can't call wait on it again.  So in
     * order to implement wait-all, we would need to rebuild the
     * state_ptrs array every time wait_fn completed.  This would
     * then be an O(n^2) algorithm.
     *
     * Until a waitall_fn is added for greqs, we'll call wait on
     * each greq individually. */
#if 0
    MPIX_Grequest_wait_function *wait_fn = NULL;
    int n_greq;
    MPIX_Grequest_class curr_class;
    /* loop over all requests, group greqs with the same class and
     * call wait_fn on the groups.  (Only consecutive greqs with the
     * same class are being grouped) */
    n_greq = 0;
    for (i = 0; i < count; ++i) {
        /* skip over requests we're not interested in */
        if (request_ptrs[i] == NULL || *request_ptrs[i]->cc_ptr == 0 ||
            request_ptrs[i]->kind != MPIR_REQUEST_KIND__GREQUEST)
            continue;

        if (n_greq == 0 || request_ptrs[i]->u.ureq.greq_fns->greq_class == curr_class) {
            /* if this is the first grequest of a group, or if it's the
             * same class as the last one, add its state to the list  */
            curr_class = request_ptrs[i]->u.ureq.greq_fns->greq_class;
            wait_fn = request_ptrs[i]->u.ureq.greq_fns->wait_fn;
            state_ptrs[n_greq] = request_ptrs[i]->u.ureq.greq_fns->grequest_extra_state;
            ++n_greq;
        } else {
            /* greq with a new class: wait on the list of greqs we've
             * created, then start a new list */
            mpi_error = (wait_fn) (n_greq, state_ptrs, 0, NULL);
            if (mpi_error)
                MPIR_ERR_POP(mpi_error);

            curr_class = request_ptrs[i]->u.ureq.greq_fns->greq_class;
            wait_fn = request_ptrs[i]->u.ureq.greq_fns->wait_fn;
            state_ptrs[0] = request_ptrs[i]->u.ureq.greq_fns->grequest_extra_state;
            n_greq = 1;
        }
    }

    if (n_greq) {
        /* wait on the last group of greqs */
        mpi_error = (wait_fn) (n_greq, state_ptrs, 0, NULL);
        if (mpi_error)
            MPIR_ERR_POP(mpi_error);

    }
#else
    for (i = 0; i < count; ++i) {
        /* skip over requests we're not interested in */
        if (request_ptrs[i] == NULL ||
            MPIR_Request_is_complete(request_ptrs[i]) ||
            request_ptrs[i]->kind != MPIR_REQUEST_KIND__GREQUEST ||
            request_ptrs[i]->u.ureq.greq_fns->wait_fn == NULL) {
            continue;
        }

        mpi_error =
            (request_ptrs[i]->u.ureq.greq_fns->wait_fn) (1,
                                                         &request_ptrs[i]->u.ureq.
                                                         greq_fns->grequest_extra_state, 0, NULL);
        if (mpi_error)
            MPIR_ERR_POP(mpi_error);
        MPIR_Assert(MPIR_Request_is_complete(request_ptrs[i]));
    }

    MPID_Progress_start(&progress_state);
    for (i = 0; i < count; ++i) {
        if (request_ptrs[i] == NULL ||
            MPIR_Request_is_complete(request_ptrs[i]) ||
            request_ptrs[i]->kind != MPIR_REQUEST_KIND__GREQUEST) {
            continue;
        }
        /* We have a greq that doesn't have a wait function; some other
         * thread will cause completion via MPI_Grequest_complete().  Rather
         * than waste the time by simply yielding the processor to the
         * other thread, we'll make progress on regular requests too.  The
         * progress engine should permit the other thread to run at some
         * point. */
        while (!MPIR_Request_is_complete(request_ptrs[i])) {
            mpi_error = MPID_Progress_wait(&progress_state);
            if (mpi_error != MPI_SUCCESS) {
                /* --BEGIN ERROR HANDLING-- */
                MPID_Progress_end(&progress_state);
                goto fn_fail;
                /* --END ERROR HANDLING-- */
            }
        }
    }
    MPID_Progress_end(&progress_state);
#endif

  fn_exit:
    MPIR_CHKLMEM_FREEALL();
    return mpi_error;
  fn_fail:
    goto fn_exit;
}
