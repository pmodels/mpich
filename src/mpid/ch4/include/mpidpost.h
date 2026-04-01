/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef MPIDPOST_H_INCLUDED
#define MPIDPOST_H_INCLUDED

#ifndef MPIDI_CH4_DIRECT_NETMOD
MPL_STATIC_INLINE_PREFIX void MPIDI_REQUEST_SET_LOCAL(struct MPIR_Request *req, bool is_local,
                                                      struct MPIR_Request *partner)
{
    req->dev.is_local = is_local;
    if (partner) {
        MPIR_Assert(!is_local);
        req->dev.anysrc_partner = partner;
        partner->dev.anysrc_partner = req;
    }
}
#else
#define MPIDI_REQUEST_SET_LOCAL(req, is_local_, partner_)  do { } while (0)
#endif

#include "mpir_datatype.h"
#include "mpidch4.h"

MPL_STATIC_INLINE_PREFIX MPIR_Request *MPID_Request_create_from_comm(MPIR_Request_kind_t kind,
                                                                     MPIR_Comm * comm)
{
    MPIR_Request *req;
    int vci = MPIDI_get_comm_vci(comm);
    MPID_THREAD_CS_ENTER(VCI, MPIDI_VCI_LOCK(vci));
    req = MPIR_Request_create_from_pool(kind, vci, 1);
    MPID_THREAD_CS_EXIT(VCI, MPIDI_VCI_LOCK(vci));
    return req;
}

MPL_STATIC_INLINE_PREFIX void MPID_Request_create_hook(MPIR_Request * req)
{
    MPIR_FUNC_ENTER;

    req->dev.completion_notification = NULL;
    req->dev.type = MPIDI_REQ_TYPE_NONE;
#ifndef MPIDI_CH4_DIRECT_NETMOD
    req->dev.anysrc_partner = NULL;
    req->dev.is_local = 0;
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
