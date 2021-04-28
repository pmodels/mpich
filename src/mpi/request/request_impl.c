/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"

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
                    if (request_ptr->u.persist.real_request->kind != MPIR_REQUEST_KIND__GREQUEST) {
                        /* jratt@us.ibm.com: I don't know about the bsend
                         * comment below, but the CC stuff on the next
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
                        /* This is needed for persistent Bsend requests */
                        /* FIXME why do we directly access the partner request's
                         * cc field?  shouldn't our cc_ptr be set to the address
                         * of the partner req's cc field? */
                        mpi_errno = MPIR_Grequest_cancel(request_ptr->u.persist.real_request,
                                                         MPIR_cc_is_complete(&request_ptr->
                                                                             u.persist.
                                                                             real_request->cc));
                        MPIR_ERR_CHECK(mpi_errno);
                    }
                } else {
                    MPIR_ERR_SETANDJUMP(mpi_errno, MPI_ERR_REQUEST, "**requestpersistactive");
                }
                break;
            }

        case MPIR_REQUEST_KIND__PREQUEST_RECV:
            {
                if (request_ptr->u.persist.real_request != NULL) {
                    mpi_errno = MPID_Cancel_recv(request_ptr->u.persist.real_request);
                    MPIR_ERR_CHECK(mpi_errno);
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

    MPID_Progress_poke();
    switch (request_ptr->kind) {
        case MPIR_REQUEST_KIND__SEND:
            MPII_SENDQ_FORGET(request_ptr);
            break;
        case MPIR_REQUEST_KIND__RECV:
            break;
        case MPIR_REQUEST_KIND__PREQUEST_SEND:
            /* Tell the device that we are freeing a persistent request object */
            MPID_Prequest_free_hook(request_ptr);
            /* If this is an active persistent request, we must also
             * release the partner request. */
            if (request_ptr->u.persist.real_request != NULL) {
                if (request_ptr->u.persist.real_request->kind == MPIR_REQUEST_KIND__GREQUEST) {
                    /* This is needed for persistent Bsend requests */
                    mpi_errno = MPIR_Grequest_free(request_ptr->u.persist.real_request);
                }
                MPIR_Request_free(request_ptr->u.persist.real_request);
            }
            break;
        case MPIR_REQUEST_KIND__PREQUEST_RECV:
            /* Tell the device that we are freeing a persistent request object */
            MPID_Prequest_free_hook(request_ptr);
            /* If this is an active persistent request, we must also
             * release the partner request. */
            if (request_ptr->u.persist.real_request != NULL) {
                MPIR_Request_free(request_ptr->u.persist.real_request);
            }
            break;
        case MPIR_REQUEST_KIND__PREQUEST_COLL:
            MPIR_Persist_coll_free_cb(request_ptr);
            break;
        case MPIR_REQUEST_KIND__GREQUEST:
            mpi_errno = MPIR_Grequest_free(request_ptr);
            break;
        case MPIR_REQUEST_KIND__PART_SEND:
        case MPIR_REQUEST_KIND__PART_RECV:
            MPID_Part_request_free_hook(request_ptr);
            break;
        default:
            mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE,
                                             __func__, __LINE__, MPI_ERR_OTHER,
                                             "**request_invalid_kind",
                                             "**request_invalid_kind %d", request_ptr->kind);
            goto fn_fail;
    }

    MPIR_Request_free(request_ptr);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIR_Request_get_status_impl(MPIR_Request * request_ptr, int *flag, MPI_Status * status)
{
    int mpi_errno = MPI_SUCCESS;

    if (!MPIR_Request_is_complete(request_ptr)) {
        /* request not complete. poke the progress engine. Req #3130 */
        mpi_errno = MPID_Progress_test(NULL);
        MPIR_ERR_CHECK(mpi_errno);
    }

    if (MPIR_Request_is_complete(request_ptr)) {
        MPIR_Request *prequest_ptr;
        int rc;
        switch (request_ptr->kind) {
            case MPIR_REQUEST_KIND__SEND:
                MPIR_Status_set_cancel_bit(status, MPIR_STATUS_GET_CANCEL_BIT(request_ptr->status));
                mpi_errno = request_ptr->status.MPI_ERROR;
                break;
            case MPIR_REQUEST_KIND__RECV:
                MPIR_Request_extract_status(request_ptr, status);
                mpi_errno = request_ptr->status.MPI_ERROR;
                break;
            case MPIR_REQUEST_KIND__PREQUEST_SEND:
                prequest_ptr = request_ptr->u.persist.real_request;
                if (prequest_ptr != NULL) {
                    if (prequest_ptr->kind != MPIR_REQUEST_KIND__GREQUEST) {
                        MPIR_Status_set_cancel_bit(status,
                                                   MPIR_STATUS_GET_CANCEL_BIT(request_ptr->status));
                        mpi_errno = prequest_ptr->status.MPI_ERROR;
                    } else {
                        /* This is needed for persistent Bsend requests */
                        rc = MPIR_Grequest_query(prequest_ptr);
                        if (mpi_errno == MPI_SUCCESS) {
                            mpi_errno = rc;
                        }
                        MPIR_Status_set_cancel_bit(status,
                                                   MPIR_STATUS_GET_CANCEL_BIT
                                                   (prequest_ptr->status));
                        if (mpi_errno == MPI_SUCCESS) {
                            mpi_errno = prequest_ptr->status.MPI_ERROR;
                        }
                    }
                } else {
                    if (request_ptr->status.MPI_ERROR != MPI_SUCCESS) {
                        /* if the persistent request failed to start then
                         * make the error code available */
                        MPIR_Status_set_cancel_bit(status,
                                                   MPIR_STATUS_GET_CANCEL_BIT(request_ptr->status));
                        mpi_errno = request_ptr->status.MPI_ERROR;
                    } else {
                        MPIR_Status_set_empty(status);
                    }
                }
                break;
            case MPIR_REQUEST_KIND__PREQUEST_RECV:
            case MPIR_REQUEST_KIND__PREQUEST_COLL:
                prequest_ptr = request_ptr->u.persist.real_request;
                if (prequest_ptr != NULL) {
                    MPIR_Request_extract_status(prequest_ptr, status);
                    mpi_errno = prequest_ptr->status.MPI_ERROR;
                } else {
                    /* if the persistent request failed to start then
                     * make the error code available */
                    mpi_errno = request_ptr->status.MPI_ERROR;
                    MPIR_Status_set_empty(status);
                }
                break;
            case MPIR_REQUEST_KIND__GREQUEST:
                rc = MPIR_Grequest_query(request_ptr);
                if (mpi_errno == MPI_SUCCESS) {
                    mpi_errno = rc;
                }
                MPIR_Status_set_cancel_bit(status, MPIR_STATUS_GET_CANCEL_BIT(request_ptr->status));
                MPIR_Request_extract_status(request_ptr, status);
                break;

            default:
                break;
        }

        *flag = TRUE;
    } else {
        *flag = FALSE;
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

/* -- Test -- */
int MPIR_Test_state(MPIR_Request * request_ptr, int *flag, MPI_Status * status,
                    MPID_Progress_state * state)
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = MPID_Progress_test(state);
    MPIR_ERR_CHECK(mpi_errno);

    if (MPIR_Request_has_poll_fn(request_ptr)) {
        mpi_errno = MPIR_Grequest_poll(request_ptr, status);
        MPIR_ERR_CHECK(mpi_errno);
    }

    if (MPIR_Request_is_complete(request_ptr))
        *flag = TRUE;
    else
        *flag = FALSE;

  fn_exit:
    return mpi_errno;

  fn_fail:
    goto fn_exit;
}

int MPIR_Test_impl(MPIR_Request * request_ptr, int *flag, MPI_Status * status)
{
    return MPIR_Test_state(request_ptr, flag, status, NULL);
}

int MPIR_Test(MPI_Request * request, int *flag, MPI_Status * status)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Request *request_ptr = NULL;

    /* If this is a null request handle, then return an empty status */
    if (*request == MPI_REQUEST_NULL) {
        MPIR_Status_set_empty(status);
        *flag = TRUE;
        goto fn_exit;
    }

    MPIR_Request_get_ptr(*request, request_ptr);
    MPIR_Assert(request_ptr != NULL);

    mpi_errno = MPID_Test(request_ptr, flag, status);
    MPIR_ERR_CHECK(mpi_errno);

    if (*flag) {
        mpi_errno = MPIR_Request_completion_processing(request_ptr, status);
        if (!MPIR_Request_is_persistent(request_ptr) && !MPIR_Request_is_partitioned(request_ptr)) {
            MPIR_Request_free(request_ptr);
            *request = MPI_REQUEST_NULL;
        }
        MPIR_ERR_CHECK(mpi_errno);
        /* Fall through to the exit */
    } else if (unlikely(MPIR_Request_is_anysrc_mismatched(request_ptr))) {
        MPIR_ERR_SET(mpi_errno, MPIX_ERR_PROC_FAILED_PENDING, "**failure_pending");
        if (status != MPI_STATUS_IGNORE)
            status->MPI_ERROR = mpi_errno;
        goto fn_fail;
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
    int i;
    int mpi_errno = MPI_SUCCESS;
    int n_completed = 0;

    mpi_errno = MPID_Progress_test(state);
    MPIR_ERR_CHECK(mpi_errno);

    if (requests_property & MPIR_REQUESTS_PROPERTY__NO_GREQUESTS) {
        for (i = 0; i < count; i++) {
            if ((i + 1) % MPIR_CVAR_REQUEST_POLL_FREQ == 0) {
                mpi_errno = MPID_Progress_test(state);
                MPIR_ERR_CHECK(mpi_errno);
            }

            if (request_ptrs[i] == NULL || MPIR_Request_is_complete(request_ptrs[i])) {
                n_completed++;
            } else {
                break;
            }
        }
    } else {
        for (i = 0; i < count; i++) {
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
                    n_completed++;
                }
            } else {
                n_completed++;
            }
        }
    }
    *flag = (n_completed == count) ? TRUE : FALSE;

  fn_exit:
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

int MPIR_Testall(int count, MPI_Request array_of_requests[], int *flag,
                 MPI_Status array_of_statuses[])
{
    MPIR_Request *request_ptr_array[MPIR_REQUEST_PTR_ARRAY_SIZE];
    MPIR_Request **request_ptrs = request_ptr_array;
    int i;
    int n_completed;
    int active_flag;
    int rc = MPI_SUCCESS;
    int proc_failure = FALSE;
    int mpi_errno = MPI_SUCCESS;
    int requests_property = MPIR_REQUESTS_PROPERTY__OPT_ALL;
    int ignoring_status = (array_of_statuses == MPI_STATUSES_IGNORE);
    MPIR_CHKLMEM_DECL(1);

    int ii, icount, impi_errno;
    n_completed = 0;

    /* Convert MPI request handles to a request object pointers */
    if (count > MPIR_REQUEST_PTR_ARRAY_SIZE) {
        MPIR_CHKLMEM_MALLOC_ORJUMP(request_ptrs, MPIR_Request **,
                                   count * sizeof(MPIR_Request *), mpi_errno, "request pointers",
                                   MPL_MEM_OBJECT);
    }

    for (ii = 0; ii < count; ii += MPIR_CVAR_REQUEST_BATCH_SIZE) {
        icount = count - ii > MPIR_CVAR_REQUEST_BATCH_SIZE ?
            MPIR_CVAR_REQUEST_BATCH_SIZE : count - ii;

        requests_property = MPIR_REQUESTS_PROPERTY__OPT_ALL;

        for (i = ii; i < ii + icount; i++) {
            if (array_of_requests[i] != MPI_REQUEST_NULL) {
                MPIR_Request_get_ptr(array_of_requests[i], request_ptrs[i]);
                /* Validate object pointers if error checking is enabled */
#ifdef HAVE_ERROR_CHECKING
                {
                    MPID_BEGIN_ERROR_CHECKS;
                    {
                        MPIR_Request_valid_ptr(request_ptrs[i], mpi_errno);
                        if (mpi_errno)
                            goto fn_fail;
                    }
                    MPID_END_ERROR_CHECKS;
                }
#endif
                if (request_ptrs[i]->kind != MPIR_REQUEST_KIND__RECV &&
                    request_ptrs[i]->kind != MPIR_REQUEST_KIND__SEND) {
                    requests_property &= ~MPIR_REQUESTS_PROPERTY__SEND_RECV_ONLY;
                    if (request_ptrs[i]->kind == MPIR_REQUEST_KIND__GREQUEST) {
                        requests_property &= ~MPIR_REQUESTS_PROPERTY__NO_GREQUESTS;
                    }
                }
            } else {
                request_ptrs[i] = NULL;
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
                            mpi_errno = MPI_ERR_IN_STATUS;
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
        for (i = 0; i < count; i++) {
            if (request_ptrs[i] != NULL && MPIR_Request_is_complete(request_ptrs[i]))
                MPIR_Request_completion_processing_fastpath(&array_of_requests[i], request_ptrs[i]);
        }
        goto fn_exit;
    }

    if (ignoring_status) {
        for (i = 0; i < count; i++) {
            if (request_ptrs[i] != NULL && MPIR_Request_is_complete(request_ptrs[i])) {
                MPIR_Request_completion_processing(request_ptrs[i], MPI_STATUS_IGNORE);
                if (!MPIR_Request_is_persistent(request_ptrs[i]) &&
                    !MPIR_Request_is_partitioned(request_ptrs[i])) {
                    MPIR_Request_free(request_ptrs[i]);
                    array_of_requests[i] = MPI_REQUEST_NULL;
                }
            }
        }
        goto fn_exit;
    }

    for (i = 0; i < count; i++) {
        if (request_ptrs[i] != NULL) {
            if (MPIR_Request_is_complete(request_ptrs[i])) {
                active_flag = MPIR_Request_is_active(request_ptrs[i]);
                rc = MPIR_Request_completion_processing(request_ptrs[i], &array_of_statuses[i]);
                if (!MPIR_Request_is_persistent(request_ptrs[i]) &&
                    !MPIR_Request_is_partitioned(request_ptrs[i])) {
                    MPIR_Request_free(request_ptrs[i]);
                    array_of_requests[i] = MPI_REQUEST_NULL;
                }
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
            if (mpi_errno == MPI_ERR_IN_STATUS) {
                array_of_statuses[i].MPI_ERROR = MPI_SUCCESS;
            }
        }
    }

  fn_exit:
    if (count > MPIR_REQUEST_PTR_ARRAY_SIZE) {
        MPIR_CHKLMEM_FREEALL();
    }

    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

/* -- Testany -- */
int MPIR_Testany_state(int count, MPIR_Request * request_ptrs[],
                       int *indx, int *flag, MPI_Status * status, MPID_Progress_state * state)
{
    int i;
    int n_inactive = 0;
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = MPID_Progress_test(state);
    /* --BEGIN ERROR HANDLING-- */
    MPIR_ERR_CHECK(mpi_errno);
    /* --END ERROR HANDLING-- */

    for (i = 0; i < count; i++) {
        if ((i + 1) % MPIR_CVAR_REQUEST_POLL_FREQ == 0) {
            mpi_errno = MPID_Progress_test(state);
            MPIR_ERR_CHECK(mpi_errno);
        }

        if (request_ptrs[i] != NULL && MPIR_Request_has_poll_fn(request_ptrs[i])) {
            mpi_errno = MPIR_Grequest_poll(request_ptrs[i], status);
            if (mpi_errno != MPI_SUCCESS)
                goto fn_fail;
        }
        if (!MPIR_Request_is_active(request_ptrs[i])) {
            n_inactive += 1;
        } else if (MPIR_Request_is_complete(request_ptrs[i])) {
            *flag = TRUE;
            *indx = i;
            goto fn_exit;
        }
    }

    if (n_inactive == count) {
        *flag = TRUE;
        *indx = MPI_UNDEFINED;
    }

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

int MPIR_Testany(int count, MPI_Request array_of_requests[], MPIR_Request * request_ptrs[],
                 int *indx, int *flag, MPI_Status * status)
{
    int mpi_errno = MPI_SUCCESS;
    int n_inactive = 0;
    int last_disabled_anysource = -1;
    int first_nonnull = count;

    *flag = FALSE;
    *indx = MPI_UNDEFINED;

    for (int i = 0; i < count; i++) {
        if (array_of_requests[i] != MPI_REQUEST_NULL) {
            if (unlikely(MPIR_Request_is_anysrc_mismatched(request_ptrs[i]))) {
                last_disabled_anysource = i;
            }

            if (MPIR_Request_is_complete(request_ptrs[i])) {
                if (MPIR_Request_is_active(request_ptrs[i])) {
                    *indx = i;
                    *flag = TRUE;
                    break;
                } else {
                    request_ptrs[i] = NULL;
                }
            } else {
                if (first_nonnull == count)
                    first_nonnull = i;
            }
        } else {
            request_ptrs[i] = NULL;
            n_inactive += 1;
        }
    }

    if (n_inactive == count) {
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
        if (!MPIR_Request_is_persistent(request_ptrs[*indx]) &&
            !MPIR_Request_is_partitioned(request_ptrs[*indx])) {
            MPIR_Request_free(request_ptrs[*indx]);
            array_of_requests[*indx] = MPI_REQUEST_NULL;
        }
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
    int n_inactive;
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = MPID_Progress_test(state);
    /* --BEGIN ERROR HANDLING-- */
    MPIR_ERR_CHECK(mpi_errno);
    /* --END ERROR HANDLING-- */

    n_inactive = 0;
    *outcount = 0;

    for (i = 0; i < incount; i++) {
        if ((i + 1) % MPIR_CVAR_REQUEST_POLL_FREQ == 0) {
            mpi_errno = MPID_Progress_test(state);
            MPIR_ERR_CHECK(mpi_errno);
        }

        if (request_ptrs[i] != NULL && MPIR_Request_has_poll_fn(request_ptrs[i])) {
            mpi_errno = MPIR_Grequest_poll(request_ptrs[i], &array_of_statuses[i]);
            if (mpi_errno != MPI_SUCCESS)
                goto fn_fail;
        }
        if (!MPIR_Request_is_active(request_ptrs[i])) {
            n_inactive += 1;
        } else if (MPIR_Request_is_complete(request_ptrs[i])) {
            array_of_indices[*outcount] = i;
            *outcount += 1;
        }
    }

    if (n_inactive == incount)
        *outcount = MPI_UNDEFINED;

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

int MPIR_Testsome(int incount, MPI_Request array_of_requests[], MPIR_Request * request_ptrs[],
                  int *outcount, int array_of_indices[], MPI_Status array_of_statuses[])
{
    int mpi_errno = MPI_SUCCESS;
    int n_inactive;
    int proc_failure = FALSE;

    *outcount = 0;
    n_inactive = 0;
    for (int i = 0; i < incount; i++) {
        if (array_of_requests[i] != MPI_REQUEST_NULL) {
            if (unlikely(MPIR_Request_is_anysrc_mismatched(request_ptrs[i]))) {
                proc_failure = TRUE;
                int rc = MPI_SUCCESS;
                MPIR_ERR_SET(rc, MPIX_ERR_PROC_FAILED_PENDING, "**failure_pending");
                if (array_of_statuses != MPI_STATUSES_IGNORE)
                    array_of_statuses[i].MPI_ERROR = rc;
            }
        } else {
            request_ptrs[i] = NULL;
            n_inactive += 1;
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
        if (!MPIR_Request_is_persistent(request_ptrs[idx]) &&
            !MPIR_Request_is_partitioned(request_ptrs[idx])) {
            MPIR_Request_free(request_ptrs[idx]);
            array_of_requests[idx] = MPI_REQUEST_NULL;
        }
        if (rc == MPI_SUCCESS) {
            request_ptrs[idx] = NULL;
        } else {
            mpi_errno = MPI_ERR_IN_STATUS;
            if (status_ptr != MPI_STATUS_IGNORE) {
                status_ptr->MPI_ERROR = rc;
            }
        }
    }

    if (mpi_errno == MPI_ERR_IN_STATUS) {
        if (array_of_statuses != MPI_STATUSES_IGNORE) {
            for (int i = 0; i < *outcount; i++) {
                if (request_ptrs[array_of_indices[i]] == NULL) {
                    array_of_statuses[i].MPI_ERROR = MPI_SUCCESS;
                }
            }
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

    while (!MPIR_Request_is_complete(request_ptr)) {
        mpi_errno = MPID_Progress_wait(state);
        MPIR_ERR_CHECK(mpi_errno);

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

int MPIR_Wait(MPI_Request * request, MPI_Status * status)
{
    int mpi_errno = MPI_SUCCESS;
    int active_flag;
    MPIR_Request *request_ptr = NULL;

    /* If this is a null request handle, then return an empty status */
    if (*request == MPI_REQUEST_NULL) {
        MPIR_Status_set_empty(status);
        goto fn_exit;
    }

    MPIR_Request_get_ptr(*request, request_ptr);
    MPIR_Assert(request_ptr != NULL);

    if (!MPIR_Request_is_complete(request_ptr)) {
        /* If this is an anysource request including a communicator with
         * anysource disabled, convert the call to an MPI_Test instead so we
         * don't get stuck in the progress engine. */
        if (unlikely(MPIR_Request_is_anysrc_mismatched(request_ptr))) {
            mpi_errno = MPIR_Test(request, &active_flag, status);
            goto fn_exit;
        }

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
    }

    mpi_errno = MPIR_Request_completion_processing(request_ptr, status);
    if (!MPIR_Request_is_persistent(request_ptr) && !MPIR_Request_is_partitioned(request_ptr)) {
        MPIR_Request_free(request_ptr);
        *request = MPI_REQUEST_NULL;
    }
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
            while (!MPIR_Request_is_complete(request_ptrs[i])) {
                mpi_errno = MPID_Progress_wait(state);
                MPIR_ERR_CHECK(mpi_errno);
            }
        }
    } else {
        for (int i = 0; i < count; i++) {
            if (request_ptrs[i] == NULL) {
                continue;
            }
            /* wait for ith request to complete */
            while (!MPIR_Request_is_complete(request_ptrs[i])) {
                /* generalized requests should already be finished */
                MPIR_Assert(request_ptrs[i]->kind != MPIR_REQUEST_KIND__GREQUEST);

                mpi_errno = MPID_Progress_wait(state);
                MPIR_ERR_CHECK(mpi_errno);
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

int MPIR_Waitall(int count, MPI_Request array_of_requests[], MPI_Status array_of_statuses[])
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Request *request_ptr_array[MPIR_REQUEST_PTR_ARRAY_SIZE];
    MPIR_Request **request_ptrs = request_ptr_array;
    int i, j, ii, icount;
    int n_completed;
    int rc = MPI_SUCCESS;
    int disabled_anysource = FALSE;
    const int ignoring_statuses = (array_of_statuses == MPI_STATUSES_IGNORE);
    int requests_property = MPIR_REQUESTS_PROPERTY__OPT_ALL;
    MPIR_CHKLMEM_DECL(1);

    /* Convert MPI request handles to a request object pointers */
    if (count > MPIR_REQUEST_PTR_ARRAY_SIZE) {
        MPIR_CHKLMEM_MALLOC(request_ptrs, MPIR_Request **, count * sizeof(MPIR_Request *),
                            mpi_errno, "request pointers", MPL_MEM_OBJECT);
    }

    for (ii = 0; ii < count; ii += MPIR_CVAR_REQUEST_BATCH_SIZE) {
        icount = count - ii > MPIR_CVAR_REQUEST_BATCH_SIZE ?
            MPIR_CVAR_REQUEST_BATCH_SIZE : count - ii;

        n_completed = 0;
        requests_property = MPIR_REQUESTS_PROPERTY__OPT_ALL;

        for (i = ii; i < ii + icount; i++) {
            if (array_of_requests[i] != MPI_REQUEST_NULL) {
                MPIR_Request_get_ptr(array_of_requests[i], request_ptrs[i]);
                /* Validate object pointers if error checking is enabled */
#ifdef HAVE_ERROR_CHECKING
                {
                    MPID_BEGIN_ERROR_CHECKS;
                    {
                        MPIR_Request_valid_ptr(request_ptrs[i], mpi_errno);
                        MPIR_ERR_CHECK(mpi_errno);
                        MPIR_ERR_CHKANDJUMP1((request_ptrs[i]->kind == MPIR_REQUEST_KIND__MPROBE),
                                             mpi_errno, MPI_ERR_ARG, "**msgnotreq",
                                             "**msgnotreq %d", i);
                    }
                    MPID_END_ERROR_CHECKS;
                }
#endif
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
                request_ptrs[i] = NULL;
                n_completed += 1;
                requests_property &= ~MPIR_REQUESTS_PROPERTY__NO_NULL;
            }
        }

        if (n_completed == icount) {
            continue;
        }

        if (unlikely(disabled_anysource)) {
            mpi_errno =
                MPIR_Testall(count, array_of_requests, &disabled_anysource, array_of_statuses);
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
                rc = MPIR_Request_completion_processing_fastpath(&array_of_requests[i],
                                                                 request_ptrs[i]);
                if (rc != MPI_SUCCESS) {
                    MPIR_ERR_SET(mpi_errno, MPI_ERR_IN_STATUS, "**instatus");
                    goto fn_exit;
                }
            }
            continue;
        }

        if (ignoring_statuses) {
            for (i = ii; i < ii + icount; i++) {
                if (request_ptrs[i] == NULL)
                    continue;
                rc = MPIR_Request_completion_processing(request_ptrs[i], MPI_STATUS_IGNORE);
                if (!MPIR_Request_is_persistent(request_ptrs[i]) &&
                    !MPIR_Request_is_partitioned(request_ptrs[i])) {
                    MPIR_Request_free(request_ptrs[i]);
                    array_of_requests[i] = MPI_REQUEST_NULL;
                }
                if (rc != MPI_SUCCESS) {
                    MPIR_ERR_SET(mpi_errno, MPI_ERR_IN_STATUS, "**instatus");
                    goto fn_exit;
                }
            }
            continue;
        }

        for (i = ii; i < ii + icount; i++) {
            if (request_ptrs[i] == NULL)
                continue;
            rc = MPIR_Request_completion_processing(request_ptrs[i], &array_of_statuses[i]);
            if (!MPIR_Request_is_persistent(request_ptrs[i]) &&
                !MPIR_Request_is_partitioned(request_ptrs[i])) {
                MPIR_Request_free(request_ptrs[i]);
                array_of_requests[i] = MPI_REQUEST_NULL;
            }

            if (rc == MPI_SUCCESS) {
                array_of_statuses[i].MPI_ERROR = MPI_SUCCESS;
            } else {
                /* req completed with an error */
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
    if (count > MPIR_REQUEST_PTR_ARRAY_SIZE) {
        MPIR_CHKLMEM_FREEALL();
    }

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

    for (;;) {
        int n_inactive = 0;
        int found_nonnull_req = FALSE;

        for (int i = 0; i < count; i++) {
            if ((i + 1) % MPIR_CVAR_REQUEST_POLL_FREQ == 0) {
                mpi_errno = MPID_Progress_test(state);
                MPIR_ERR_CHECK(mpi_errno);
            }

            if (request_ptrs[i] == NULL) {
                ++n_inactive;
                continue;
            }
            /* we found at least one non-null request */
            found_nonnull_req = TRUE;

            if (MPIR_Request_has_poll_fn(request_ptrs[i])) {
                mpi_errno = MPIR_Grequest_poll(request_ptrs[i], status);
                MPIR_ERR_CHECK(mpi_errno);
            }
            if (MPIR_Request_is_complete(request_ptrs[i])) {
                if (MPIR_Request_is_active(request_ptrs[i])) {
                    *indx = i;
                    goto fn_exit;
                } else {
                    ++n_inactive;
                    request_ptrs[i] = NULL;

                    if (n_inactive == count) {
                        *indx = MPI_UNDEFINED;
                        /* status is set to empty by MPIR_Request_completion_processing */
                        goto fn_exit;
                    }
                }
            }
        }

        if (!found_nonnull_req) {
            /* all requests were NULL */
            *indx = MPI_UNDEFINED;
            if (status != NULL) /* could be null if count=0 */
                MPIR_Status_set_empty(status);
            goto fn_exit;
        }

        mpi_errno = MPID_Progress_test(state);
        MPIR_ERR_CHECK(mpi_errno);
        /* Avoid blocking other threads since I am inside an infinite loop */
        MPID_THREAD_CS_YIELD(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
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

int MPIR_Waitany(int count, MPI_Request array_of_requests[], MPIR_Request * request_ptrs[],
                 int *indx, MPI_Status * status)
{
    int mpi_errno = MPI_SUCCESS;
    int last_disabled_anysource = -1;
    int first_nonnull = count;

    *indx = MPI_UNDEFINED;

    for (int i = 0; i < count; i++) {
        if (array_of_requests[i] != MPI_REQUEST_NULL) {
            if (unlikely(MPIR_Request_is_anysrc_mismatched(request_ptrs[i]))) {
                last_disabled_anysource = i;
            }

            /* Since waitany is likely to return as soon as a
             * completed request is found, the first loop here is
             * taking the longest. We can check for any completed
             * request here, and we can poll from the first entry
             * which is not null. */
            if (MPIR_Request_is_complete(request_ptrs[i])) {
                if (MPIR_Request_is_active(request_ptrs[i])) {
                    *indx = i;
                    break;
                } else {
                    request_ptrs[i] = NULL;
                }
            } else {
                if (first_nonnull == count)
                    first_nonnull = i;
            }
        } else {
            request_ptrs[i] = NULL;
        }
    }

    if (*indx == MPI_UNDEFINED) {
        if (unlikely(last_disabled_anysource != -1)) {
            int flag;
            mpi_errno = MPI_Testany(count, array_of_requests, indx, &flag, status);
            goto fn_exit;
        }

        mpi_errno = MPID_Waitany(count - first_nonnull, &request_ptrs[first_nonnull], indx, status);
        MPIR_ERR_CHECK(mpi_errno);

        if (*indx != MPI_UNDEFINED) {
            *indx += first_nonnull;
        } else {
            goto fn_exit;
        }
    }

    mpi_errno = MPIR_Request_completion_processing(request_ptrs[*indx], status);
    if (!MPIR_Request_is_persistent(request_ptrs[*indx]) &&
        !MPIR_Request_is_partitioned(request_ptrs[*indx])) {
        MPIR_Request_free(request_ptrs[*indx]);
        array_of_requests[*indx] = MPI_REQUEST_NULL;
    }
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
    for (;;) {
        int n_inactive = 0;

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
                    if (MPIR_Request_is_active(request_ptrs[i])) {
                        array_of_indices[n_active] = i;
                        n_active += 1;
                    } else {
                        request_ptrs[i] = NULL;
                        n_inactive += 1;
                    }
                }
            } else {
                n_inactive += 1;
            }
        }

        if (n_active > 0) {
            *outcount = n_active;
            break;
        } else if (n_inactive == incount) {
            *outcount = MPI_UNDEFINED;
            break;
        }

        mpi_errno = MPID_Progress_test(state);
        MPIR_ERR_CHECK(mpi_errno);
        /* Avoid blocking other threads since I am inside an infinite loop */
        MPID_THREAD_CS_YIELD(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
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

int MPIR_Waitsome(int incount, MPI_Request array_of_requests[], MPIR_Request * request_ptrs[],
                  int *outcount, int array_of_indices[], MPI_Status array_of_statuses[])
{
    int mpi_errno = MPI_SUCCESS;
    int n_inactive;
    int disabled_anysource = FALSE;

    *outcount = 0;
    n_inactive = 0;
    for (int i = 0; i < incount; i++) {
        if (array_of_requests[i] != MPI_REQUEST_NULL) {
            /* If one of the requests is an anysource on a communicator that's
             * disabled such communication, convert this operation to a testall
             * instead to prevent getting stuck in the progress engine. */
            if (unlikely(MPIR_Request_is_anysrc_mismatched(request_ptrs[i]))) {
                disabled_anysource = TRUE;
            }
        } else {
            n_inactive += 1;
            request_ptrs[i] = NULL;
        }
    }

    if (n_inactive == incount) {
        *outcount = MPI_UNDEFINED;
        goto fn_exit;
    }

    if (unlikely(disabled_anysource)) {
        mpi_errno =
            MPI_Testsome(incount, array_of_requests, outcount, array_of_indices, array_of_statuses);
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
        if (!MPIR_Request_is_persistent(request_ptrs[idx]) &&
            !MPIR_Request_is_partitioned(request_ptrs[idx])) {
            MPIR_Request_free(request_ptrs[idx]);
            array_of_requests[idx] = MPI_REQUEST_NULL;
        }
        if (rc == MPI_SUCCESS) {
            request_ptrs[idx] = NULL;
        } else {
            mpi_errno = MPI_ERR_IN_STATUS;
            if (status_ptr != MPI_STATUS_IGNORE) {
                status_ptr->MPI_ERROR = rc;
            }
        }
    }

    if (mpi_errno == MPI_ERR_IN_STATUS) {
        if (array_of_statuses != MPI_STATUSES_IGNORE) {
            for (int i = 0; i < *outcount; i++) {
                if (request_ptrs[array_of_indices[i]] == NULL) {
                    array_of_statuses[i].MPI_ERROR = MPI_SUCCESS;
                }
            }
        }
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIR_Parrived(MPI_Request * request, MPIR_Request * request_ptr, int partition, int *flag)
{
    int mpi_errno = MPI_SUCCESS;

    if (*request == MPI_REQUEST_NULL) {
        *flag = TRUE;
        goto fn_exit;
    }

    /* For non-NULL request, query device layer */
    MPIR_Assert(request_ptr != NULL);

    mpi_errno = MPID_Parrived(request_ptr, partition, flag);
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
