/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef STUBNM_REQUEST_H_INCLUDED
#define STUBNM_REQUEST_H_INCLUDED

#include "stubnm_impl.h"

MPL_STATIC_INLINE_PREFIX void MPIDI_NM_am_request_init(MPIR_Request * req)
{
    MPIR_Assert(0);
}

MPL_STATIC_INLINE_PREFIX void MPIDI_NM_am_request_finalize(MPIR_Request * req)
{
    MPIR_Assert(0);
}

#endif /* STUBNM_REQUEST_H_INCLUDED */
