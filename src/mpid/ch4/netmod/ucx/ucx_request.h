/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef UCX_REQUEST_H_INCLUDED
#define UCX_REQUEST_H_INCLUDED

#include "ucx_impl.h"

MPL_STATIC_INLINE_PREFIX void MPIDI_NM_am_request_init(MPIR_Request * req)
{
    req->dev.ch4.am.netmod_am.ucx.pack_buffer = NULL;
}

MPL_STATIC_INLINE_PREFIX void MPIDI_NM_am_request_finalize(MPIR_Request * req)
{
    MPL_free((req)->dev.ch4.am.netmod_am.ucx.pack_buffer);
    /* MPIR_Request_free(req); */
}

#endif /* UCX_REQUEST_H_INCLUDED */
