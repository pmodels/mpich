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
#ifdef MPIDI_BUILD_CH4_SHM
    MPIDI_CH4I_REQUEST_ANYSOURCE_PARTNER(req) = NULL;
#endif

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_REQUEST_CREATE_HOOK);
}

MPL_STATIC_INLINE_PREFIX void MPID_Request_free_hook(MPIR_Request * req)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_REQUEST_FREE_HOOK);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_REQUEST_FREE_HOOK);

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

#endif /* MPIDPOST_H_INCLUDED */
