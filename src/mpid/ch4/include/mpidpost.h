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

MPL_STATIC_INLINE_PREFIX MPIR_Request *MPID_Request_create_complete(int kind, int vci)
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

#endif /* MPIDPOST_H_INCLUDED */
