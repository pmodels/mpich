/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef MPIDPOST_H_INCLUDED
#define MPIDPOST_H_INCLUDED

#include "mpir_datatype.h"
#include "mpidch4.h"

MPL_STATIC_INLINE_PREFIX MPIR_Request *MPID_Request_create_from_comm(MPIR_Request_kind_t kind,
                                                                     MPIR_Comm * comm)
{
    MPIR_Request *req;
    int vci = MPIDI_get_comm_vci(comm);
    MPID_THREAD_CS_ENTER(VCI, MPIDI_VCI(vci).lock);
    req = MPIR_Request_create_from_pool(kind, vci, 1);
    MPID_THREAD_CS_EXIT(VCI, MPIDI_VCI(vci).lock);
    return req;
}

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
