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

    MPIDIG_REQUEST(req, req) = NULL;
#ifndef MPIDI_CH4_DIRECT_NETMOD
    MPIDI_REQUEST_ANYSOURCE_PARTNER(req) = NULL;
#endif

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_REQUEST_CREATE_HOOK);
}

MPL_STATIC_INLINE_PREFIX void MPID_Request_free_hook(MPIR_Request * req)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_REQUEST_FREE_HOOK);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_REQUEST_FREE_HOOK);
#if MPICH_THREAD_GRANULARITY == MPICH_THREAD_GRANULARITY__GLOBAL
    OPA_incr_int(&MPIDI_global.progress_count);
#endif
    if (req->kind == MPIR_REQUEST_KIND__PREQUEST_RECV &&
        NULL != MPIDI_REQUEST_ANYSOURCE_PARTNER(req))
        MPIR_Request_free(MPIDI_REQUEST_ANYSOURCE_PARTNER(req));

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

MPL_STATIC_INLINE_PREFIX MPIR_Request *MPID_Request_create_unsafe(int kind, int vci)
{
    MPIR_Request *req;

    if (MPIDI_VCI(vci).request_cache_count > 0) {
        req = MPIDI_VCI(vci).request_cache[--(MPIDI_VCI(vci).request_cache_count)];
    } else {
        req = MPIR_Handle_obj_alloc(&MPIR_Request_mem);
    }

    if (req != NULL) {
        MPIR_Request_alloc_success(req, kind);
    } else {
        MPIR_Request_alloc_fail();
    }
    
    MPIDI_REQUEST(req, vci) = vci;

    return req;
}

MPL_STATIC_INLINE_PREFIX MPIR_Request *MPID_Request_create_safe(int kind, int vci)
{
    MPIR_Request *req;
    
    MPID_THREAD_CS_ENTER(VCI, MPIDI_VCI(vci).lock);

    req = MPID_Request_create_unsafe(kind, vci);

    MPID_THREAD_CS_EXIT(VCI, MPIDI_VCI(vci).lock);

    return req;
}

MPL_STATIC_INLINE_PREFIX void MPID_Request_free_safe(MPIR_Request * req)
{
    int vci;
    int inuse;

    vci = MPIDI_REQUEST(req, vci);

    MPID_THREAD_CS_ENTER(VCI, MPIDI_VCI(vci).lock);
    
    MPIR_Request_release(req, &inuse);
    if (inuse == 0) {
        MPIR_Request_free_not_in_use(req);
        if (MPIDI_VCI(vci).request_cache_count < MPIDI_MAX_REQUEST_CACHE_COUNT) {
            MPIDI_VCI(vci).request_cache[(MPIDI_VCI(vci).request_cache_count)++] = req;
        } else {
            MPIR_Handle_obj_free(&MPIR_Request_mem, req);
        }
    }

    MPID_THREAD_CS_EXIT(VCI, MPIDI_VCI(vci).lock);
}

MPL_STATIC_INLINE_PREFIX void MPID_Request_free_unsafe(MPIR_Request * req)
{
    int inuse;
    int vci;

    MPIR_Request_release(req, &inuse);
    vci = MPIDI_REQUEST(req, vci);

    if (inuse == 0) {
        MPIR_Request_free_not_in_use(req);
        if (MPIDI_VCI(vci).request_cache_count < MPIDI_MAX_REQUEST_CACHE_COUNT) {
            MPIDI_VCI(vci).request_cache[(MPIDI_VCI(vci).request_cache_count)++] = req;
        } else {
            MPIR_Handle_obj_free(&MPIR_Request_mem, req);
        }
    }
}

MPL_STATIC_INLINE_PREFIX MPIR_Request *MPID_Request_create_complete_safe(int kind, int vci)
{
    MPIR_Request *req;
    
    MPID_THREAD_CS_ENTER(VCI, MPIDI_VCI(vci).lock);
    
    req = MPID_Request_create_complete_unsafe(kind, vci);
    
    MPID_THREAD_CS_EXIT(VCI, MPIDI_VCI(vci).lock);
    
    return req;
}

MPL_STATIC_INLINE_PREFIX MPIR_Request *MPID_Request_create_complete_unsafe(int kind, int vci)
{
    MPIR_Request *req;
#ifdef HAVE_DEBUGGER_SUPPORT
    req = MPIR_Request_create(kind);
    MPIR_cc_set(&req->cc, 0);
#else
    req = MPIDI_VCI(vci).lw_req;
    MPIR_Request_add_ref(req);
#endif
    return req;
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
    int mpi_errno = MPI_SUCCESS;
    MPID_Progress_state progress_state;
    int i;

    if (request_properties & MPIR_REQUESTS_PROPERTY__NO_NULL) {
        MPID_Progress_start(&progress_state);
        for (i = 0; i < count; i++) {
            while (!MPIR_Request_is_complete(request_ptrs[i])) {
                mpi_errno = MPID_Progress_wait_req(request_ptrs[i]);
                /* must check and handle the error, can't guard with HAVE_ERROR_CHECKING, but it's
                 * OK for the error case to be slower */
                if (unlikely(mpi_errno)) {
                    /* --BEGIN ERROR HANDLING-- */
                    MPID_Progress_end(&progress_state);
                    MPIR_ERR_POP(mpi_errno);
                    /* --END ERROR HANDLING-- */
                }
            }
        }
        MPID_Progress_end(&progress_state);
    } else {
        MPID_Progress_start(&progress_state);
        for (i = 0; i < count; i++) {
            if (request_ptrs[i] == NULL) {
                continue;
            }
            /* wait for ith request to complete */
            while (!MPIR_Request_is_complete(request_ptrs[i])) {
                /* generalized requests should already be finished */
                MPIR_Assert(request_ptrs[i]->kind != MPIR_REQUEST_KIND__GREQUEST);

                mpi_errno = MPID_Progress_wait_req(request_ptrs[i]);
                if (mpi_errno != MPI_SUCCESS) {
                    /* --BEGIN ERROR HANDLING-- */
                    MPID_Progress_end(&progress_state);
                    MPIR_ERR_POP(mpi_errno);
                    /* --END ERROR HANDLING-- */
                }
            }
        }
        MPID_Progress_end(&progress_state);
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;

}

MPL_STATIC_INLINE_PREFIX int MPID_Wait(MPIR_Request * request_ptr, MPI_Status * status)
{
    int mpi_errno = MPI_SUCCESS;
    MPID_Progress_state progress_state;
    if (request_ptr == NULL)
        goto fn_exit;

    MPID_Progress_start(&progress_state);
    while (!MPIR_Request_is_complete(request_ptr)) {
        mpi_errno = MPID_Progress_wait_req(request_ptr);
        if (mpi_errno) {
            /* --BEGIN ERROR HANDLING-- */
            MPID_Progress_end(&progress_state);
            MPIR_ERR_POP(mpi_errno);
            /* --END ERROR HANDLING-- */
        }

        if (unlikely(MPIR_Request_is_anysrc_mismatched(request_ptr))) {
            mpi_errno = MPIR_Request_handle_proc_failed(request_ptr);
            goto fn_fail;
        }
    }
    MPID_Progress_end(&progress_state);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
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
