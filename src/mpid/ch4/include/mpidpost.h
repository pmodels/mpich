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

    req->dev.completion_notification = NULL;
    req->dev.type = MPIDI_REQ_TYPE_NONE;
#ifndef MPIDI_CH4_DIRECT_NETMOD
    req->dev.anysrc_partner = NULL;
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
    return MPIR_Start_progress_thread_impl(NULL);
}

MPL_STATIC_INLINE_PREFIX int MPID_Finalize_async_thread(void)
{
    return MPIR_Stop_progress_thread_impl(NULL);
}

#endif /* MPIDPOST_H_INCLUDED */
