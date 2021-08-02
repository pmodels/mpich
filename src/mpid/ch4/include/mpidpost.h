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
    MPIR_FUNC_ENTER;

    MPIDIG_REQUEST(req, req) = NULL;
#ifndef MPIDI_CH4_DIRECT_NETMOD
    MPIDI_REQUEST_ANYSOURCE_PARTNER(req) = NULL;
#endif

    MPIR_FUNC_EXIT;
}

MPL_STATIC_INLINE_PREFIX void MPID_Request_free_hook(MPIR_Request * req)
{
    MPIR_FUNC_ENTER;

    int vci = MPIDI_Request_get_vci(req);
    /* Increment MPIDI_global.vci[vci].vci.progress_count. */
    int count = MPL_atomic_relaxed_load_int(&MPIDI_VCI(vci).progress_count);
    MPL_atomic_relaxed_store_int(&MPIDI_VCI(vci).progress_count, count + 1);

    /* This is tricky. I think the only solution is to expose partner
     * to the upper layer */
    if (req->kind == MPIR_REQUEST_KIND__PREQUEST_RECV &&
        NULL != MPIDI_REQUEST_ANYSOURCE_PARTNER(req))
        MPIR_Request_free(MPIDI_REQUEST_ANYSOURCE_PARTNER(req));

    MPIR_FUNC_EXIT;
    return;
}

MPL_STATIC_INLINE_PREFIX void MPID_Request_destroy_hook(MPIR_Request * req)
{
    MPIR_FUNC_ENTER;

    MPIR_FUNC_EXIT;
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
