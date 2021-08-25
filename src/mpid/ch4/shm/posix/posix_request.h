/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef POSIX_REQUEST_H_INCLUDED
#define POSIX_REQUEST_H_INCLUDED

#include "posix_impl.h"
#include "posix_am_impl.h"
#include "posix_eager.h"

MPL_STATIC_INLINE_PREFIX void MPIDI_POSIX_am_request_init(MPIR_Request * req)
{
    MPIDI_POSIX_AMREQUEST(req, req_hdr) = NULL;

    MPIDI_POSIX_EAGER_RECV_INITIALIZE_HOOK(req);
}

MPL_STATIC_INLINE_PREFIX void MPIDI_POSIX_am_request_finalize(MPIR_Request * req)
{
    MPIDI_POSIX_am_request_header_t *req_hdr;
    MPIR_FUNC_ENTER;

    MPIDI_POSIX_EAGER_RECV_COMPLETED_HOOK(req);

    req_hdr = MPIDI_POSIX_AMREQUEST(req, req_hdr);

    if (!req_hdr)
        goto fn_exit;

    MPIDI_POSIX_am_release_req_hdr(&req_hdr);
    MPIDI_POSIX_AMREQUEST(req, req_hdr) = NULL;

  fn_exit:
    MPIR_FUNC_EXIT;
    return;
}

#endif /* POSIX_REQUEST_H_INCLUDED */
