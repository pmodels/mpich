/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 *  Portions of this code were written by Intel Corporation.
 *  Copyright (C) 2011-2017 Intel Corporation.  Intel provides this material
 *  to Argonne National Laboratory subject to Software Grant and Corporate
 *  Contributor License Agreement dated February 8, 2012.
 */
#ifndef POSIX_REQUEST_H_INCLUDED
#define POSIX_REQUEST_H_INCLUDED

#include "posix_impl.h"
#include "posix_am_impl.h"

MPL_STATIC_INLINE_PREFIX void MPIDI_POSIX_am_request_init(MPIR_Request * req)
{
    MPIDI_POSIX_AMREQUEST(req, req_hdr) = NULL;

    MPIDI_POSIX_EAGER_RECV_INITIALIZE_HOOK(req);

    POSIX_TRACE("Created request %d\n", req->kind);
}

MPL_STATIC_INLINE_PREFIX void MPIDI_POSIX_am_request_finalize(MPIR_Request * req)
{
    MPIDI_POSIX_am_request_header_t *req_hdr;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_POSIX_AM_OFI_CLEAR_REQ);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_POSIX_AM_OFI_CLEAR_REQ);

    MPIDI_POSIX_EAGER_RECV_COMPLETED_HOOK(req);

    req_hdr = MPIDI_POSIX_AMREQUEST(req, req_hdr);

    if (!req_hdr)
        return;

    POSIX_TRACE("Completed request %d (%d)\n", req->kind, req_hdr->dst_grank);

    MPIDI_POSIX_am_release_req_hdr(&req_hdr);
    MPIDI_POSIX_AMREQUEST(req, req_hdr) = NULL;

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_NETMOD_AM_OFI_CLEAR_REQ);
    return;
}

#endif /* POSIX_REQUEST_H_INCLUDED */
