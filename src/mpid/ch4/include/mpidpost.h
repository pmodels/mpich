/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
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

    int vci = MPIDI_Request_get_vci(req);
    MPIDI_global.progress_counts[vci]++;

    /* This is tricky. I think the only solution is to expose partner
     * to the upper layer */
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
