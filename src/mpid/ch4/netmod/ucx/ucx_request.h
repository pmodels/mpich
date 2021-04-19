/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef UCX_REQUEST_H_INCLUDED
#define UCX_REQUEST_H_INCLUDED

#include "ucx_impl.h"

MPL_STATIC_INLINE_PREFIX void MPIDI_NM_am_request_init(MPIR_Request * req)
{
    MPIDI_UCX_AMREQUEST(req, pack_buffer) = NULL;
    MPIDI_UCX_AMREQUEST(req, am_type_choice) = MPIDI_UCX_AMTYPE_NONE;
}

MPL_STATIC_INLINE_PREFIX void MPIDI_NM_am_request_finalize(MPIR_Request * req)
{
}

#endif /* UCX_REQUEST_H_INCLUDED */
