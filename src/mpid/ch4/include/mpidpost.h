/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 *  Portions of this code were written by Intel Corporation.
 *  Copyright (C) 2011-2016 Intel Corporation.  Intel provides this material
 *  to Argonne National Laboratory subject to Software Grant and Corporate
 *  Contributor License Agreement dated February 8, 2012.
 */
#ifndef MPIDPOST_H_INCLUDED
#define MPIDPOST_H_INCLUDED

#include "mpir_datatype.h"
#include "mpidch4.h"

MPL_STATIC_INLINE_PREFIX void MPID_Request_create_hook(MPIR_Request * req)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_REQUEST_CREATE_HOOK);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_REQUEST_CREATE_HOOK);

    MPIDI_CH4U_REQUEST(req, req) = NULL;
#ifndef MPIDI_CH4_DIRECT_NETMOD
    MPIDI_CH4I_REQUEST_ANYSOURCE_PARTNER(req) = NULL;
#endif

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_REQUEST_CREATE_HOOK);
}

MPL_STATIC_INLINE_PREFIX void MPID_Request_free_hook(MPIR_Request * req)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_REQUEST_FREE_HOOK);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_REQUEST_FREE_HOOK);

    OPA_incr_int(&MPIDI_CH4_Global.progress_count);

    if (req->kind == MPIR_REQUEST_KIND__PREQUEST_RECV &&
        NULL != MPIDI_CH4I_REQUEST_ANYSOURCE_PARTNER(req))
        MPIR_Request_free(MPIDI_CH4I_REQUEST_ANYSOURCE_PARTNER(req));

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_REQUEST_FREE_HOOK);
    return;
}

MPL_STATIC_INLINE_PREFIX void MPID_Request_destroy_hook(MPIR_Request * req)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_REQUEST_DESTROY_HOOK);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_REQUEST_DESTROY_HOOK);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_REQUEST_DESTROY_HOOK);
    return;
}

/*
  Device override hooks for asynchronous progress threads
*/
MPL_STATIC_INLINE_PREFIX int MPID_Init_async_thread(void)
{
    return MPIR_Init_async_thread();
}

MPL_STATIC_INLINE_PREFIX int MPID_Finalize_async_thread(void)
{
    return MPIR_Finalize_async_thread();
}

MPL_STATIC_INLINE_PREFIX int MPID_Test(MPIR_Request * request_ptr, int *flag, MPI_Status * status)
{
    return MPIR_Test_impl(request_ptr, flag, status);
}

MPL_STATIC_INLINE_PREFIX int MPID_Testall(int count, MPIR_Request * request_ptrs[],
                                          int *flag, MPI_Status array_of_statuses[],
                                          int requests_property)
{
    return MPIR_Testall_impl(count, request_ptrs, flag, array_of_statuses, requests_property);
}

MPL_STATIC_INLINE_PREFIX int MPID_Testany(int count, MPIR_Request * request_ptrs[],
                                          int *indx, int *flag, MPI_Status * status)
{
    return MPIR_Testany_impl(count, request_ptrs, indx, flag, status);
}

MPL_STATIC_INLINE_PREFIX int MPID_Testsome(int incount, MPIR_Request * request_ptrs[],
                                           int *outcount, int array_of_indices[],
                                           MPI_Status array_of_statuses[])
{
    return MPIR_Testsome_impl(incount, request_ptrs, outcount, array_of_indices, array_of_statuses);
}

#ifdef MPICH_THREAD_USE_MDTA

MPL_STATIC_INLINE_PREFIX int MPID_Waitall(int count, MPIR_Request * request_ptrs[],
                                          MPI_Status array_of_statuses[], int request_properties)
{
    const int single_threaded = !MPIR_ThreadInfo.isThreaded;
    int mpi_errno = MPI_SUCCESS;
    int i;
    MPIR_Thread_sync_t *sync = NULL;

    if (likely(single_threaded))
        return MPIR_Waitall_impl(count, request_ptrs, array_of_statuses, request_properties);

    MPIR_Thread_sync_alloc(&sync, count);

    /* Fix up number of pending requests and attach the sync. */
    for (i = 0; i < count; i++) {
        if (request_ptrs[i] == NULL || MPIR_Request_is_complete(request_ptrs[i])) {
            MPIR_Thread_sync_signal(sync, 0);
        } else {
            MPIR_Request_attach_sync(request_ptrs[i], sync);
        }
    }

    /* Wait on the synchronization object. */
    MPIR_Thread_sync_wait(sync);

    /* Either being signaled, or become a server, so we poll from now. */
    mpi_errno = MPIR_Waitall_impl(count, request_ptrs, array_of_statuses, request_properties);

    MPIR_Thread_sync_free(sync);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPID_Wait(MPIR_Request * request_ptr, MPI_Status * status)
{
    const int single_threaded = !MPIR_ThreadInfo.isThreaded;
    int mpi_errno = MPI_SUCCESS;
    MPIR_Thread_sync_t *sync = NULL;

    if (likely(single_threaded))
        return MPIR_Wait_impl(request_ptr, status);

    if (request_ptr == NULL || MPIR_Request_is_complete(request_ptr))
        goto fn_exit;

    /* The request cannot be completed immediately, wait on a sync. */
    MPIR_Thread_sync_alloc(&sync, 1);
    MPIR_Request_attach_sync(request_ptr, sync);
    MPIR_Thread_sync_wait(sync);

    /* Either being signaled, or become a server, so we poll from now. */
    mpi_errno = MPIR_Wait_impl(request_ptr, status);

    MPIR_Thread_sync_free(sync);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#else

MPL_STATIC_INLINE_PREFIX int MPID_Waitall(int count, MPIR_Request * request_ptrs[],
                                          MPI_Status array_of_statuses[], int request_properties)
{
    return MPIR_Waitall_impl(count, request_ptrs, array_of_statuses, request_properties);
}

MPL_STATIC_INLINE_PREFIX int MPID_Wait(MPIR_Request * request_ptr, MPI_Status * status)
{
    return MPIR_Wait_impl(request_ptr, status);
}

#endif /* MPICH_THREAD_USE_MDTA */

MPL_STATIC_INLINE_PREFIX int MPID_Waitany(int count, MPIR_Request * request_ptrs[],
                                          int *indx, MPI_Status * status)
{
    return MPIR_Waitany_impl(count, request_ptrs, indx, status);
}

MPL_STATIC_INLINE_PREFIX int MPID_Waitsome(int incount, MPIR_Request * request_ptrs[],
                                           int *outcount, int array_of_indices[],
                                           MPI_Status array_of_statuses[])
{
    return MPIR_Waitsome_impl(incount, request_ptrs, outcount, array_of_indices, array_of_statuses);
}

#endif /* MPIDPOST_H_INCLUDED */
