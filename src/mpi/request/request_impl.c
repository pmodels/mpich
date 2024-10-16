/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"

/*
=== BEGIN_MPI_T_CVAR_INFO_BLOCK ===

categories :
    - name : REQUEST
      description : A category for requests management variables

cvars:
    - name        : MPIR_CVAR_REQUEST_ERR_FATAL
      category    : REQUEST
      type        : boolean
      default     : false
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_LOCAL
      description : >-
        By default, MPI_Waitall, MPI_Testall, MPI_Waitsome, and MPI_Testsome
        return MPI_ERR_IN_STATUS when one of the request fails. If
        MPIR_CVAR_REQUEST_ERR_FATAL is set to true, these routines will
        return the error code of the request immediately. The default
        MPI_ERRS_ARE_FATAL error handler will dump a error stack in this
        case, which maybe more convenient for debugging. This cvar will also
        make nonblocking shched return error right away as it issues
        operations.

    - name        : MPIR_CVAR_REQUEST_POLL_FREQ
      category    : REQUEST
      type        : int
      default     : 8
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_LOCAL
      description : >-
        How frequent to poll during MPI_{Waitany,Waitsome} in terms
        of number of processed requests before polling.

    - name        : MPIR_CVAR_REQUEST_BATCH_SIZE
      category    : REQUEST
      type        : int
      default     : 64
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_LOCAL
      description : >-
        The number of requests to make completion as a batch
        in MPI_Waitall and MPI_Testall implementation. A large number
        is likely to cause more cache misses.

    - name        : MPIR_CVAR_DEBUG_PROGRESS_TIMEOUT
      category    : CH4
      type        : int
      default     : 0
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_LOCAL
      description : >-
        Sets the timeout in seconds to dump outstanding requests when progress wait is not making progress for some time.

=== END_MPI_T_CVAR_INFO_BLOCK ===
*/

int MPIR_Status_set_cancelled_impl(MPI_Status * status, int flag)
{
    MPIR_STATUS_SET_CANCEL_BIT(*status, flag ? TRUE : FALSE);

    return MPI_SUCCESS;
}

int MPIR_Test_cancelled_impl(const MPI_Status * status, int *flag)
{
    *flag = MPIR_STATUS_GET_CANCEL_BIT(*status);

    return MPI_SUCCESS;
}

int MPIR_Cancel_impl(MPIR_Request * request_ptr)
{
    int mpi_errno = MPI_SUCCESS;

    switch (request_ptr->kind) {
        case MPIR_REQUEST_KIND__SEND:
            {
                mpi_errno = MPID_Cancel_send(request_ptr);
                MPIR_ERR_CHECK(mpi_errno);
                break;
            }

        case MPIR_REQUEST_KIND__RECV:
            {
                mpi_errno = MPID_Cancel_recv(request_ptr);
                MPIR_ERR_CHECK(mpi_errno);
                break;
            }

        case MPIR_REQUEST_KIND__PREQUEST_SEND:
            {
                if (request_ptr->u.persist.real_request != NULL) {
                    /* jratt@us.ibm.com: The CC stuff on the next
                     * line is *really* needed for persistent Bsend
                     * request cancels.  The CC of the parent was
                     * disconnected from the child to allow an
                     * MPI_Wait in user-level to complete immediately
                     * (mpid/dcmfd/src/persistent/mpid_startall.c).
                     * However, if the user tries to cancel the parent
                     * (and thereby cancels the child), we cannot just
                     * say the request is done.  We need to re-link
                     * the parent's cc_ptr to the child CC, thus
                     * causing an MPI_Wait on the parent to block
                     * until the child is canceled or completed.
                     */
                    request_ptr->cc_ptr = request_ptr->u.persist.real_request->cc_ptr;
                    mpi_errno = MPID_Cancel_send(request_ptr->u.persist.real_request);
                    MPIR_ERR_CHECK(mpi_errno);
                } else {
                    MPIR_ERR_SETANDJUMP(mpi_errno, MPI_ERR_REQUEST, "**cancelinactive");
                }
                break;
            }

        case MPIR_REQUEST_KIND__PREQUEST_RECV:
            {
                if (request_ptr->u.persist.real_request != NULL) {
                    mpi_errno = MPID_Cancel_recv(request_ptr->u.persist.real_request);
                    MPIR_ERR_CHECK(mpi_errno);
                } else {
                    MPIR_ERR_SETANDJUMP(mpi_errno, MPI_ERR_REQUEST, "**cancelinactive");
                }
                break;
            }

        case MPIR_REQUEST_KIND__PREQUEST_COLL:
            {
                if (request_ptr->u.persist_coll.real_request != NULL) {
                    MPIR_Assert_error("Cancel persistent collective not supported");
                } else {
                    MPIR_ERR_SETANDJUMP(mpi_errno, MPI_ERR_REQUEST, "**requestpersistactive");
                }
                break;
            }

        case MPIR_REQUEST_KIND__GREQUEST:
            {
                mpi_errno =
                    MPIR_Grequest_cancel(request_ptr, MPIR_cc_is_complete(&request_ptr->cc));
                MPIR_ERR_CHECK(mpi_errno);
                break;
            }

            /* --BEGIN ERROR HANDLING-- */
        default:
            {
                MPIR_ERR_SETANDJUMP(mpi_errno, MPI_ERR_INTERN, "**cancelunknown");
            }
            /* --END ERROR HANDLING-- */
    }

  fn_exit:
    return mpi_errno;
  fn_fail:

    goto fn_exit;
}

int MPIR_Request_free_impl(MPIR_Request * request_ptr)
{
    int mpi_errno = MPI_SUCCESS;

    switch (request_ptr->kind) {
        case MPIR_REQUEST_KIND__PREQUEST_SEND:
            /* If this is an active persistent request, we must also
             * release the partner request. */
            if (request_ptr->u.persist.real_request != NULL) {
                MPIR_Request_free(request_ptr->u.persist.real_request);
            }
            break;
        case MPIR_REQUEST_KIND__PREQUEST_RECV:
            /* If this is an active persistent request, we must also
             * release the partner request. */
            if (request_ptr->u.persist.real_request != NULL) {
                MPIR_Request_free(request_ptr->u.persist.real_request);
            }
            break;
        case MPIR_REQUEST_KIND__PREQUEST_COLL:
            /* If this is an active persistent request, we must also
             * release the partner request. */
            if (request_ptr->u.persist_coll.real_request != NULL) {
                MPIR_Request_free(request_ptr->u.persist_coll.real_request);
            }
            break;
        case MPIR_REQUEST_KIND__CONTINUE:
            break;
        default:
            break;
    }

    mpi_errno = MPIR_Request_free_return(request_ptr);

    return mpi_errno;
}

/* -- Test -- */
int MPIR_Test_state(MPIR_Request * request_ptr, int *flag, MPI_Status * status,
                    MPID_Progress_state * state)
{
    int mpi_errno = MPI_SUCCESS;

    if (!MPIR_Request_is_complete(request_ptr)) {
        mpi_errno = MPID_Progress_test(state);
        MPIR_ERR_CHECK(mpi_errno);
        MPIR_Continue_progress(request_ptr);
    }

  fn_exit:
    return mpi_errno;

  fn_fail:
    goto fn_exit;
}

int MPIR_Test_impl(MPIR_Request * request_ptr, int *flag, MPI_Status * status)
{
    return MPIR_Test_state(request_ptr, flag, status, NULL);
}

int MPIR_Test(MPIR_Request * request_ptr, int *flag, MPI_Status * status)
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = MPID_Test(request_ptr, flag, status);
    MPIR_ERR_CHECK(mpi_errno);

    if (MPIR_Request_has_poll_fn(request_ptr)) {
        mpi_errno = MPIR_Grequest_poll(request_ptr, status);
        MPIR_ERR_CHECK(mpi_errno);
    }

    if (MPIR_Request_is_complete(request_ptr)) {
        *flag = TRUE;
        mpi_errno = MPIR_Request_completion_processing(request_ptr, status);
    } else {
        *flag = FALSE;
        if (unlikely(MPIR_Request_is_anysrc_mismatched(request_ptr))) {
            MPIR_ERR_SET(mpi_errno, MPIX_ERR_PROC_FAILED_PENDING, "**failure_pending");
            if (status != MPI_STATUS_IGNORE) {
                status->MPI_ERROR = mpi_errno;
            }
            goto fn_fail;
        }
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

/* -- Testall -- */
int MPIR_Testall_state(int count, MPIR_Request * request_ptrs[], int *flag,
                       MPI_Status array_of_statuses[], int requests_property,
                       MPID_Progress_state * state)
{
    int mpi_errno = MPI_SUCCESS;
    int n_completed;
    int need_progress = 1;

  fn_check_requests:
    n_completed = 0;
    if (requests_property & MPIR_REQUESTS_PROPERTY__NO_GREQUESTS) {
        for (int i = 0; i < count; i++) {
            if (request_ptrs[i] == NULL || MPIR_Request_is_complete(request_ptrs[i])) {
                n_completed++;
            } else {
                MPIR_Continue_progress(request_ptrs[i]);
                break;
            }
        }
    } else {
        for (int i = 0; i < count; i++) {
            if (request_ptrs[i] != NULL) {
                if (MPIR_Request_has_poll_fn(request_ptrs[i])) {
                    mpi_errno = MPIR_Grequest_poll(request_ptrs[i], &array_of_statuses[i]);
                    MPIR_ERR_CHECK(mpi_errno);
                }
                if (MPIR_Request_is_complete(request_ptrs[i])) {
                    n_completed++;
                } else {
                    MPIR_Continue_progress(request_ptrs[i]);
                }
            } else {
                n_completed++;
            }
        }
    }

    if (n_completed == count) {
        *flag = TRUE;
        goto fn_exit;
    }

    if (need_progress > 0) {
        mpi_errno = MPID_Progress_test(state);
        MPIR_ERR_CHECK(mpi_errno);

        need_progress--;
        goto fn_check_requests;
    }

    *flag = FALSE;

  fn_exit:
    if (n_completed == count) {
        *flag = TRUE;
        goto fn_exit;
    }

    if (need_progress > 0) {
        mpi_errno = MPID_Progress_test(state);
        MPIR_ERR_CHECK(mpi_errno);

        need_progress--;
        goto fn_check_requests;
    }

    *flag = FALSE;

    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIR_Testall_impl(int count, MPIR_Request * request_ptrs[], int *flag,
                      MPI_Status array_of_statuses[], int requests_property)
{
    return MPIR_Testall_state(count, request_ptrs, flag, array_of_statuses,
                              requests_property, NULL);
}

int MPIR_Testall(int count, MPIR_Request * request_ptrs[], int *flag,
                 MPI_Status array_of_statuses[])
{
    int i;
    int n_completed;
    int active_flag;
    int rc = MPI_SUCCESS;
    int proc_failure = FALSE;
    int mpi_errno = MPI_SUCCESS;
    int requests_property = MPIR_REQUESTS_PROPERTY__OPT_ALL;
    int ignoring_status = (array_of_statuses == MPI_STATUSES_IGNORE);

    int ii, icount, impi_errno;
    n_completed = 0;

    for (ii = 0; ii < count; ii += MPIR_CVAR_REQUEST_BATCH_SIZE) {
        icount = count - ii > MPIR_CVAR_REQUEST_BATCH_SIZE ?
            MPIR_CVAR_REQUEST_BATCH_SIZE : count - ii;

        requests_property = MPIR_REQUESTS_PROPERTY__OPT_ALL;

        for (i = ii; i < ii + icount; i++) {
            if (request_ptrs[i]) {
                if (request_ptrs[i]->kind != MPIR_REQUEST_KIND__RECV &&
                    request_ptrs[i]->kind != MPIR_REQUEST_KIND__SEND) {
                    requests_property &= ~MPIR_REQUESTS_PROPERTY__SEND_RECV_ONLY;
                    if (request_ptrs[i]->kind == MPIR_REQUEST_KIND__GREQUEST) {
                        requests_property &= ~MPIR_REQUESTS_PROPERTY__NO_GREQUESTS;
                    }
                }
            } else {
                requests_property &= ~MPIR_REQUESTS_PROPERTY__NO_NULL;
            }
        }

        impi_errno = MPID_Testall(icount, &request_ptrs[ii], flag,
                                  ignoring_status ? MPI_STATUSES_IGNORE : &array_of_statuses[ii],
                                  requests_property);
        if (impi_errno != MPI_SUCCESS) {
            mpi_errno = impi_errno;
            goto fn_fail;
        }

        for (i = ii; i < ii + icount; i++) {
            if (request_ptrs[i] == NULL || MPIR_Request_is_complete(request_ptrs[i]))
                n_completed += 1;
#ifdef HAVE_ERROR_CHECKING
            {
                MPID_BEGIN_ERROR_CHECKS;
                {
                    if (request_ptrs[i] == NULL)
                        continue;
                    if (MPIR_Request_is_complete(request_ptrs[i])) {
                        rc = MPIR_Request_get_error(request_ptrs[i]);
                        if (rc != MPI_SUCCESS) {
                            if (MPIX_ERR_PROC_FAILED == MPIR_ERR_GET_CLASS(rc) ||
                                MPIX_ERR_PROC_FAILED_PENDING == MPIR_ERR_GET_CLASS(rc))
                                proc_failure = TRUE;
                            if (MPIR_CVAR_REQUEST_ERR_FATAL) {
                                mpi_errno = request_ptrs[i]->status.MPI_ERROR;
                                MPIR_ERR_CHECK(mpi_errno);
                            } else {
                                mpi_errno = MPI_ERR_IN_STATUS;
                            }
                        }
                    } else if (unlikely(MPIR_Request_is_anysrc_mismatched(request_ptrs[i]))) {
                        mpi_errno = MPI_ERR_IN_STATUS;
                        MPIR_ERR_SET(rc, MPIX_ERR_PROC_FAILED_PENDING, "**failure_pending");
                        if (!ignoring_status)
                            array_of_statuses[i].MPI_ERROR = rc;
                        proc_failure = TRUE;
                    }
                }
                MPID_END_ERROR_CHECKS;
            }
#endif
        }
    }

    *flag = (n_completed == count) ? TRUE : FALSE;

    /* We only process completion of requests if all are finished, or
     * there is an error. */
    if (!(*flag || mpi_errno == MPI_ERR_IN_STATUS))
        goto fn_exit;

    if (ignoring_status && (requests_property & MPIR_REQUESTS_PROPERTY__SEND_RECV_ONLY)) {
        /* nothing to do */
        goto fn_exit;
    }

    if (ignoring_status) {
        for (i = 0; i < count; i++) {
            if (request_ptrs[i] != NULL && MPIR_Request_is_complete(request_ptrs[i])) {
                MPIR_Request_completion_processing(request_ptrs[i], MPI_STATUS_IGNORE);
            }
        }
        goto fn_exit;
    }

    for (i = 0; i < count; i++) {
        if (request_ptrs[i] != NULL) {
            if (MPIR_Request_is_complete(request_ptrs[i])) {
                active_flag = MPIR_Request_is_active(request_ptrs[i]);
                rc = MPIR_Request_completion_processing(request_ptrs[i], &array_of_statuses[i]);
                if (mpi_errno == MPI_ERR_IN_STATUS) {
                    if (active_flag) {
                        array_of_statuses[i].MPI_ERROR = rc;
                    } else {
                        array_of_statuses[i].MPI_ERROR = MPI_SUCCESS;
                    }
                }
            } else {
                if (mpi_errno == MPI_ERR_IN_STATUS) {
                    if (!proc_failure)
                        array_of_statuses[i].MPI_ERROR = MPI_ERR_PENDING;
                    else
                        array_of_statuses[i].MPI_ERROR = MPIX_ERR_PROC_FAILED_PENDING;
                }
            }
        } else {
            MPIR_Status_set_empty(&array_of_statuses[i]);
        }
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

/* -- Testany -- */
int MPIR_Testany_state(int count, MPIR_Request * request_ptrs[],
                       int *indx, int *flag, MPI_Status * status, MPID_Progress_state * state)
{
    int mpi_errno = MPI_SUCCESS;
    int need_progress = 1;

  fn_check_requests:
    for (int i = 0; i < count; i++) {
        if (!request_ptrs[i]) {
            continue;
        }

        if (MPIR_Request_has_poll_fn(request_ptrs[i])) {
            mpi_errno = MPIR_Grequest_poll(request_ptrs[i], status);
            MPIR_ERR_CHECK(mpi_errno);
        }
        if (MPIR_Request_is_complete(request_ptrs[i])) {
            *flag = TRUE;
            *indx = i;
            goto fn_exit;
        }
        MPIR_Continue_progress(request_ptrs[i]);
    }

    if (need_progress > 0) {
        mpi_errno = MPID_Progress_test(state);
        MPIR_ERR_CHECK(mpi_errno);

        need_progress--;
        goto fn_check_requests;
    }

    *flag = FALSE;

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIR_Testany_impl(int count, MPIR_Request * request_ptrs[],
                      int *indx, int *flag, MPI_Status * status)
{
    return MPIR_Testany_state(count, request_ptrs, indx, flag, status, NULL);
}

int MPIR_Testany(int count, MPIR_Request * request_ptrs[],
                 int *indx, int *flag, MPI_Status * status)
{
    int mpi_errno = MPI_SUCCESS;
    int last_disabled_anysource = -1;
    int first_nonnull = count;

    *flag = FALSE;
    *indx = MPI_UNDEFINED;

    for (int i = 0; i < count; i++) {
        if (request_ptrs[i]) {
            if (!MPIR_Request_is_active(request_ptrs[i])) {
                request_ptrs[i] = NULL;
                continue;
            }

            if (first_nonnull == count) {
                first_nonnull = i;
            }

            if (unlikely(MPIR_Request_is_anysrc_mismatched(request_ptrs[i]))) {
                last_disabled_anysource = i;
            }

            if (MPIR_Request_is_complete(request_ptrs[i])) {
                *indx = i;
                *flag = TRUE;
                break;
            }
        }
    }

    if (first_nonnull == count) {
        *flag = TRUE;
        *indx = MPI_UNDEFINED;
        if (status != NULL)     /* could be null if count=0 */
            MPIR_Status_set_empty(status);
        goto fn_exit;
    }

    if (*indx == MPI_UNDEFINED) {
        mpi_errno =
            MPID_Testany(count - first_nonnull, &request_ptrs[first_nonnull], indx, flag, status);
        /* --BEGIN ERROR HANDLING-- */
        if (mpi_errno != MPI_SUCCESS) {
            goto fn_fail;
        }
        /* --END ERROR HANDLING-- */

        if (*indx != MPI_UNDEFINED) {
            *indx += first_nonnull;
        }
    }

    if (*indx != MPI_UNDEFINED) {
        mpi_errno = MPIR_Request_completion_processing(request_ptrs[*indx], status);
        MPIR_ERR_CHECK(mpi_errno);
        goto fn_exit;
    }

    /* If none of the requests completed, mark the last anysource request as
     * pending failure. */
    if (unlikely(last_disabled_anysource != -1)) {
        MPIR_ERR_SET(mpi_errno, MPIX_ERR_PROC_FAILED_PENDING, "**failure_pending");
        if (status != MPI_STATUS_IGNORE)
            status->MPI_ERROR = mpi_errno;
        *flag = TRUE;
        goto fn_fail;
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

/* -- Testsome -- */
int MPIR_Testsome_state(int incount, MPIR_Request * request_ptrs[],
                        int *outcount, int array_of_indices[], MPI_Status array_of_statuses[],
                        MPID_Progress_state * state)
{
    int i;
    int mpi_errno = MPI_SUCCESS;
    int need_progress = 1;

  fn_check_requests:
    *outcount = 0;

    for (i = 0; i < incount; i++) {
        if (!request_ptrs[i]) {
            continue;
        }

        if (MPIR_Request_has_poll_fn(request_ptrs[i])) {
            mpi_errno = MPIR_Grequest_poll(request_ptrs[i], &array_of_statuses[i]);
            MPIR_ERR_CHECK(mpi_errno);
        }
        if (MPIR_Request_is_complete(request_ptrs[i])) {
            array_of_indices[*outcount] = i;
            *outcount += 1;
            continue;
        }
        MPIR_Continue_progress(request_ptrs[i]);
    }

    if (*outcount) {
        /* if "some" are completed, skip progress */
        goto fn_exit;
    }

    if (need_progress > 0) {
        mpi_errno = MPID_Progress_test(state);
        MPIR_ERR_CHECK(mpi_errno);

        need_progress--;
        goto fn_check_requests;
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIR_Testsome_impl(int incount, MPIR_Request * request_ptrs[],
                       int *outcount, int array_of_indices[], MPI_Status array_of_statuses[])
{
    return MPIR_Testsome_state(incount, request_ptrs, outcount, array_of_indices,
                               array_of_statuses, NULL);
}

int MPIR_Testsome(int incount, MPIR_Request * request_ptrs[],
                  int *outcount, int array_of_indices[], MPI_Status array_of_statuses[])
{
    int mpi_errno = MPI_SUCCESS;
    int n_inactive;
    int proc_failure = FALSE;

    *outcount = 0;
    n_inactive = 0;
    for (int i = 0; i < incount; i++) {
        if (!request_ptrs[i]) {
            n_inactive += 1;
        } else if (!MPIR_Request_is_active(request_ptrs[i])) {
            request_ptrs[i] = NULL;
            n_inactive += 1;
        } else {
            if (unlikely(MPIR_Request_is_anysrc_mismatched(request_ptrs[i]))) {
                proc_failure = TRUE;
                int rc = MPI_SUCCESS;
                MPIR_ERR_SET(rc, MPIX_ERR_PROC_FAILED_PENDING, "**failure_pending");
                if (array_of_statuses != MPI_STATUSES_IGNORE)
                    array_of_statuses[i].MPI_ERROR = rc;
            }
        }
    }

    if (n_inactive == incount) {
        *outcount = MPI_UNDEFINED;
        goto fn_exit;
    }

    mpi_errno = MPID_Testsome(incount, request_ptrs, outcount, array_of_indices, array_of_statuses);
    /* --BEGIN ERROR HANDLING-- */
    if (mpi_errno != MPI_SUCCESS)
        goto fn_fail;
    /* --END ERROR HANDLING-- */

    if (proc_failure)
        mpi_errno = MPI_ERR_IN_STATUS;

    if (*outcount == MPI_UNDEFINED)
        goto fn_exit;

    for (int i = 0; i < *outcount; i++) {
        int idx = array_of_indices[i];
        MPI_Status *status_ptr = (array_of_statuses == MPI_STATUSES_IGNORE) ?
            MPI_STATUS_IGNORE : &array_of_statuses[i];
        int rc = MPIR_Request_completion_processing(request_ptrs[idx], status_ptr);
        if (rc != MPI_SUCCESS) {
            if (MPIR_CVAR_REQUEST_ERR_FATAL) {
                mpi_errno = request_ptrs[idx]->status.MPI_ERROR;
                MPIR_ERR_CHECK(mpi_errno);
            } else {
                mpi_errno = MPI_ERR_IN_STATUS;
            }
        }
    }

    if (mpi_errno == MPI_ERR_IN_STATUS && array_of_statuses != MPI_STATUSES_IGNORE) {
        for (int i = 0; i < *outcount; i++) {
            array_of_statuses[i].MPI_ERROR = request_ptrs[array_of_indices[i]]->status.MPI_ERROR;
        }
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

/* -- Wait -- */
/* MPID_Wait call MPIR_Wait_state with initialized progress state */
int MPIR_Wait_state(MPIR_Request * request_ptr, MPI_Status * status, MPID_Progress_state * state)
{
    int mpi_errno = MPI_SUCCESS;

    DEBUG_PROGRESS_START;
    while (!MPIR_Request_is_complete(request_ptr)) {
        mpi_errno = MPID_Progress_wait(state);
        MPIR_ERR_CHECK(mpi_errno);

        MPIR_Continue_progress(request_ptr);
        DEBUG_PROGRESS_CHECK;

        if (unlikely(MPIR_Request_is_anysrc_mismatched(request_ptr))) {
            mpi_errno = MPIR_Request_handle_proc_failed(request_ptr);
            goto fn_fail;
        }
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

/* legacy interface (for ch3) */
int MPIR_Wait_impl(MPIR_Request * request_ptr, MPI_Status * status)
{
    int mpi_errno = MPI_SUCCESS;
    MPID_Progress_state progress_state;

    MPIR_Assert(request_ptr != NULL);
    MPID_Progress_start(&progress_state);
    mpi_errno = MPIR_Wait_state(request_ptr, status, &progress_state);
    MPID_Progress_end(&progress_state);

    return mpi_errno;
}

int MPIR_Wait(MPIR_Request * request_ptr, MPI_Status * status)
{
    int mpi_errno = MPI_SUCCESS;

    if (MPIR_Request_has_poll_fn(request_ptr)) {
        while (!MPIR_Request_is_complete(request_ptr)) {
            mpi_errno = MPIR_Grequest_poll(request_ptr, status);
            MPIR_ERR_CHECK(mpi_errno);

            /* Avoid blocking other threads since I am inside an infinite loop */
            MPID_THREAD_CS_YIELD(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
        }
    } else {
        mpi_errno = MPID_Wait(request_ptr, status);
        MPIR_ERR_CHECK(mpi_errno);
    }

    mpi_errno = MPIR_Request_completion_processing(request_ptr, status);
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

/* -- Waitall -- */
/* MPID_Waitall call MPIR_Waitall_state with initialized progress state */
int MPIR_Waitall_state(int count, MPIR_Request * request_ptrs[], MPI_Status array_of_statuses[],
                       int requests_property, MPID_Progress_state * state)
{
    int mpi_errno = MPI_SUCCESS;

    if (requests_property & MPIR_REQUESTS_PROPERTY__NO_NULL) {
        for (int i = 0; i < count; ++i) {
            DEBUG_PROGRESS_START;
            while (!MPIR_Request_is_complete(request_ptrs[i])) {
                mpi_errno = MPID_Progress_wait(state);
                MPIR_ERR_CHECK(mpi_errno);
                MPIR_Continue_progress(request_ptrs[i]);
                DEBUG_PROGRESS_CHECK;
            }
        }
    } else {
        for (int i = 0; i < count; i++) {
            if (request_ptrs[i] == NULL) {
                continue;
            }
            /* wait for ith request to complete */
            DEBUG_PROGRESS_START;
            while (!MPIR_Request_is_complete(request_ptrs[i])) {
                /* generalized requests should already be finished */
                MPIR_Assert(request_ptrs[i]->kind != MPIR_REQUEST_KIND__GREQUEST);

                mpi_errno = MPID_Progress_wait(state);
                MPIR_ERR_CHECK(mpi_errno);
                MPIR_Continue_progress(request_ptrs[i]);
                DEBUG_PROGRESS_CHECK;
            }
        }
    }
  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

/* legacy interface (for ch3) */
int MPIR_Waitall_impl(int count, MPIR_Request * request_ptrs[], MPI_Status array_of_statuses[],
                      int requests_property)
{
    int mpi_errno = MPI_SUCCESS;
    MPID_Progress_state progress_state;

    MPID_Progress_start(&progress_state);
    mpi_errno = MPIR_Waitall_state(count, request_ptrs, array_of_statuses, requests_property,
                                   &progress_state);
    MPID_Progress_end(&progress_state);
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIR_Waitall(int count, MPIR_Request * request_ptrs[], MPI_Status array_of_statuses[])
{
    int mpi_errno = MPI_SUCCESS;
    int i, j, ii, icount;
    int n_completed;
    int rc = MPI_SUCCESS;
    int disabled_anysource = FALSE;
    const int ignoring_statuses = (array_of_statuses == MPI_STATUSES_IGNORE);
    int requests_property = MPIR_REQUESTS_PROPERTY__OPT_ALL;

    for (ii = 0; ii < count; ii += MPIR_CVAR_REQUEST_BATCH_SIZE) {
        icount = count - ii > MPIR_CVAR_REQUEST_BATCH_SIZE ?
            MPIR_CVAR_REQUEST_BATCH_SIZE : count - ii;

        n_completed = 0;
        requests_property = MPIR_REQUESTS_PROPERTY__OPT_ALL;

        for (i = ii; i < ii + icount; i++) {
            if (request_ptrs[i]) {
                /* If one of the requests is an anysource on a communicator that's
                 * disabled such communication, convert this operation to a testall
                 * instead to prevent getting stuck in the progress engine. */
                if (unlikely(MPIR_Request_is_anysrc_mismatched(request_ptrs[i]))) {
                    disabled_anysource = TRUE;
                }

                if (request_ptrs[i]->kind != MPIR_REQUEST_KIND__RECV &&
                    request_ptrs[i]->kind != MPIR_REQUEST_KIND__SEND) {
                    requests_property &= ~MPIR_REQUESTS_PROPERTY__SEND_RECV_ONLY;

                    /* If this is extended generalized request, we can complete it here. */
                    if (MPIR_Request_has_wait_fn(request_ptrs[i])) {
                        while (!MPIR_Request_is_complete(request_ptrs[i]))
                            MPIR_Grequest_wait(request_ptrs[i], &array_of_statuses[i]);
                    }
                }
            } else {
                if (!ignoring_statuses)
                    MPIR_Status_set_empty(&array_of_statuses[i]);
                n_completed += 1;
                requests_property &= ~MPIR_REQUESTS_PROPERTY__NO_NULL;
            }
        }

        if (n_completed == icount) {
            continue;
        }

        if (unlikely(disabled_anysource)) {
            mpi_errno = MPIR_Testall(count, request_ptrs, &disabled_anysource, array_of_statuses);
            goto fn_exit;
        }

        mpi_errno = MPID_Waitall(icount, &request_ptrs[ii], array_of_statuses, requests_property);
        MPIR_ERR_CHECK(mpi_errno);

        if (requests_property == MPIR_REQUESTS_PROPERTY__OPT_ALL && ignoring_statuses) {
            /* NOTE-O1: high-message-rate optimization.  For simple send and recv
             * operations and MPI_STATUSES_IGNORE we use a fastpath approach that strips
             * out as many unnecessary jumps and error handling as possible.
             *
             * Possible variation: permit request_ptrs[i]==NULL at the cost of an
             * additional branch inside the for-loop below. */
            for (i = ii; i < ii + icount; ++i) {
                rc = request_ptrs[i]->status.MPI_ERROR;
                if (rc != MPI_SUCCESS) {
                    if (MPIR_CVAR_REQUEST_ERR_FATAL) {
                        mpi_errno = request_ptrs[i]->status.MPI_ERROR;
                        MPIR_ERR_CHECK(mpi_errno);
                    } else {
                        MPIR_ERR_SET(mpi_errno, MPI_ERR_IN_STATUS, "**instatus");
                        goto fn_exit;
                    }
                }
            }
            continue;
        }

        if (ignoring_statuses) {
            for (i = ii; i < ii + icount; i++) {
                if (request_ptrs[i] == NULL)
                    continue;
                rc = MPIR_Request_completion_processing(request_ptrs[i], MPI_STATUS_IGNORE);
                if (rc != MPI_SUCCESS) {
                    if (MPIR_CVAR_REQUEST_ERR_FATAL) {
                        mpi_errno = request_ptrs[i]->status.MPI_ERROR;
                        MPIR_ERR_CHECK(mpi_errno);
                    } else {
                        MPIR_ERR_SET(mpi_errno, MPI_ERR_IN_STATUS, "**instatus");
                        goto fn_exit;
                    }
                }
            }
            continue;
        }

        for (i = ii; i < ii + icount; i++) {
            if (request_ptrs[i] == NULL)
                continue;
            rc = MPIR_Request_completion_processing(request_ptrs[i], &array_of_statuses[i]);
            if (rc == MPI_SUCCESS) {
                array_of_statuses[i].MPI_ERROR = MPI_SUCCESS;
            } else {
                if (MPIR_CVAR_REQUEST_ERR_FATAL) {
                    mpi_errno = request_ptrs[i]->status.MPI_ERROR;
                    MPIR_ERR_CHECK(mpi_errno);
                }

                /* default error processing */
                MPIR_ERR_SET(mpi_errno, MPI_ERR_IN_STATUS, "**instatus");

                /* set the error code for this request */
                array_of_statuses[i].MPI_ERROR = rc;

                if (unlikely(MPIX_ERR_PROC_FAILED == MPIR_ERR_GET_CLASS(rc)))
                    rc = MPIX_ERR_PROC_FAILED_PENDING;
                else
                    rc = MPI_ERR_PENDING;

                /* set the error codes for the rest of the uncompleted requests to PENDING */
                for (j = i + 1; j < count; ++j) {
                    if (request_ptrs[j] == NULL) {
                        /* either the user specified MPI_REQUEST_NULL, or this is a completed greq */
                        array_of_statuses[j].MPI_ERROR = MPI_SUCCESS;
                    } else {
                        array_of_statuses[j].MPI_ERROR = rc;
                    }
                }
                goto fn_exit;
            }
        }
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

/* -- Waitany -- */
/* MPID_Waitany call MPIR_Waitany_state with initialized progress state */
int MPIR_Waitany_state(int count, MPIR_Request * request_ptrs[], int *indx, MPI_Status * status,
                       MPID_Progress_state * state)
{
    int mpi_errno = MPI_SUCCESS;

    DEBUG_PROGRESS_START;
    for (;;) {
        for (int i = 0; i < count; i++) {
            if ((i + 1) % MPIR_CVAR_REQUEST_POLL_FREQ == 0) {
                mpi_errno = MPID_Progress_test(state);
                MPIR_ERR_CHECK(mpi_errno);
            }

            if (request_ptrs[i] == NULL) {
                continue;
            }

            if (MPIR_Request_has_poll_fn(request_ptrs[i])) {
                mpi_errno = MPIR_Grequest_poll(request_ptrs[i], status);
                MPIR_ERR_CHECK(mpi_errno);
            }
            if (MPIR_Request_is_complete(request_ptrs[i])) {
                *indx = i;
                goto fn_exit;
            }
            MPIR_Continue_progress(request_ptrs[i]);
        }

        mpi_errno = MPID_Progress_test(state);
        MPIR_ERR_CHECK(mpi_errno);
        /* Avoid blocking other threads since I am inside an infinite loop */
        MPID_THREAD_CS_YIELD(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
        DEBUG_PROGRESS_CHECK;
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

/* legacy interface (for ch3) */
int MPIR_Waitany_impl(int count, MPIR_Request * request_ptrs[], int *indx, MPI_Status * status)
{
    int mpi_errno = MPI_SUCCESS;
    MPID_Progress_state progress_state;

    MPID_Progress_start(&progress_state);
    mpi_errno = MPIR_Waitany_state(count, request_ptrs, indx, status, &progress_state);

    MPID_Progress_end(&progress_state);
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIR_Waitany(int count, MPIR_Request * request_ptrs[], int *indx, MPI_Status * status)
{
    int mpi_errno = MPI_SUCCESS;
    int last_disabled_anysource = -1;
    int first_nonnull = count;

    *indx = MPI_UNDEFINED;

    for (int i = 0; i < count; i++) {
        if (request_ptrs[i]) {
            if (!MPIR_Request_is_active(request_ptrs[i])) {
                request_ptrs[i] = NULL;
                continue;
            }

            if (first_nonnull == count) {
                first_nonnull = i;
            }

            if (unlikely(MPIR_Request_is_anysrc_mismatched(request_ptrs[i]))) {
                last_disabled_anysource = i;
            }

            /* Since waitany is likely to return as soon as a
             * completed request is found, the first loop here is
             * taking the longest. We can check for any completed
             * request here, and we can poll from the first entry
             * which is not null. */
            if (MPIR_Request_is_complete(request_ptrs[i])) {
                *indx = i;
                break;
            }
        }
    }

    if (first_nonnull == count) {
        if (status != NULL)     /* could be null if count=0 */
            MPIR_Status_set_empty(status);
        goto fn_exit;
    }

    if (*indx == MPI_UNDEFINED) {
        if (unlikely(last_disabled_anysource != -1)) {
            int flag;
            mpi_errno = MPIR_Testany(count, request_ptrs, indx, &flag, status);
            goto fn_exit;
        }

        mpi_errno = MPID_Waitany(count - first_nonnull, &request_ptrs[first_nonnull], indx, status);
        MPIR_ERR_CHECK(mpi_errno);

        MPIR_Assert(*indx != MPI_UNDEFINED);
        *indx += first_nonnull;
    }

    mpi_errno = MPIR_Request_completion_processing(request_ptrs[*indx], status);
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

/* -- Waitsome -- */
/* MPID_Waitsome call MPIR_Waitsome_state with initialized progress state */
int MPIR_Waitsome_state(int incount, MPIR_Request * request_ptrs[],
                        int *outcount, int array_of_indices[], MPI_Status array_of_statuses[],
                        MPID_Progress_state * state)
{
    int mpi_errno = MPI_SUCCESS;

    /* Bill Gropp says MPI_Waitsome() is expected to try to make
     * progress even if some requests have already completed;
     * therefore, we kick the pipes once and then fall into a loop
     * checking for completion and waiting for progress. */
    mpi_errno = MPID_Progress_test(state);
    MPIR_ERR_CHECK(mpi_errno);

    int n_active = 0;
    DEBUG_PROGRESS_START;
    for (;;) {
        for (int i = 0; i < incount; i++) {
            if ((i + 1) % MPIR_CVAR_REQUEST_POLL_FREQ == 0) {
                mpi_errno = MPID_Progress_test(state);
                MPIR_ERR_CHECK(mpi_errno);
            }

            if (request_ptrs[i] != NULL) {
                if (MPIR_Request_has_poll_fn(request_ptrs[i])) {
                    mpi_errno = MPIR_Grequest_poll(request_ptrs[i], &array_of_statuses[i]);
                    MPIR_ERR_CHECK(mpi_errno);
                }
                if (MPIR_Request_is_complete(request_ptrs[i])) {
                    array_of_indices[n_active] = i;
                    n_active += 1;
                    continue;
                }
                MPIR_Continue_progress(request_ptrs[i]);
            }
        }

        if (n_active > 0) {
            *outcount = n_active;
            break;
        }

        mpi_errno = MPID_Progress_test(state);
        MPIR_ERR_CHECK(mpi_errno);
        /* Avoid blocking other threads since I am inside an infinite loop */
        MPID_THREAD_CS_YIELD(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
        DEBUG_PROGRESS_CHECK;
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

/* legacy interface (for ch3) */
int MPIR_Waitsome_impl(int incount, MPIR_Request * request_ptrs[],
                       int *outcount, int array_of_indices[], MPI_Status array_of_statuses[])
{
    MPID_Progress_state progress_state;

    MPID_Progress_start(&progress_state);
    int mpi_errno = MPIR_Waitsome_state(incount, request_ptrs, outcount,
                                        array_of_indices, array_of_statuses, &progress_state);
    MPID_Progress_end(&progress_state);

    return mpi_errno;
}

int MPIR_Waitsome(int incount, MPIR_Request * request_ptrs[],
                  int *outcount, int array_of_indices[], MPI_Status array_of_statuses[])
{
    int mpi_errno = MPI_SUCCESS;
    int n_inactive;
    int disabled_anysource = FALSE;

    *outcount = 0;
    n_inactive = 0;
    for (int i = 0; i < incount; i++) {
        if (!request_ptrs[i]) {
            n_inactive += 1;
        } else if (!MPIR_Request_is_active(request_ptrs[i])) {
            request_ptrs[i] = NULL;
            n_inactive += 1;
        } else {
            /* If one of the requests is an anysource on a communicator that's
             * disabled such communication, convert this operation to a testall
             * instead to prevent getting stuck in the progress engine. */
            if (unlikely(MPIR_Request_is_anysrc_mismatched(request_ptrs[i]))) {
                disabled_anysource = TRUE;
            }
        }
    }

    if (n_inactive == incount) {
        *outcount = MPI_UNDEFINED;
        goto fn_exit;
    }

    if (unlikely(disabled_anysource)) {
        mpi_errno = MPIR_Testsome(incount, request_ptrs, outcount,
                                  array_of_indices, array_of_statuses);
        goto fn_exit;
    }

    mpi_errno = MPID_Waitsome(incount, request_ptrs, outcount, array_of_indices, array_of_statuses);
    if (mpi_errno != MPI_SUCCESS) {
        goto fn_fail;
    }

    if (*outcount == MPI_UNDEFINED)
        goto fn_exit;

    for (int i = 0; i < *outcount; i++) {
        int idx = array_of_indices[i];
        MPI_Status *status_ptr =
            (array_of_statuses != MPI_STATUSES_IGNORE) ? &array_of_statuses[i] : MPI_STATUS_IGNORE;
        int rc = MPIR_Request_completion_processing(request_ptrs[idx], status_ptr);
        if (rc != MPI_SUCCESS) {
            if (MPIR_CVAR_REQUEST_ERR_FATAL) {
                mpi_errno = request_ptrs[idx]->status.MPI_ERROR;
                MPIR_ERR_CHECK(mpi_errno);
            } else {
                mpi_errno = MPI_ERR_IN_STATUS;
            }
        }
    }
    if (mpi_errno == MPI_ERR_IN_STATUS && array_of_statuses != MPI_STATUSES_IGNORE) {
        for (int i = 0; i < *outcount; i++) {
            array_of_statuses[i].MPI_ERROR = request_ptrs[array_of_indices[i]]->status.MPI_ERROR;
        }
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIR_Parrived(MPIR_Request * request_ptr, int partition, int *flag)
{
    int mpi_errno = MPI_SUCCESS;

    /* For non-NULL request, query device layer */
    MPIR_Assert(request_ptr != NULL);

    mpi_errno = MPID_Parrived(request_ptr, partition, flag);
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
