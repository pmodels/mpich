/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 *  Portions of this code were written by Intel Corporation.
 *  Copyright (C) 2011-2012 Intel Corporation.  Intel provides this material
 *  to Argonne National Laboratory subject to Software Grant and Corporate
 *  Contributor License Agreement dated February 8, 2012.
 */
#ifndef PTL_REQUEST_H_INCLUDED
#define PTL_REQUEST_H_INCLUDED

#include "ptl_impl.h"

static inline void MPIDI_NM_am_request_init(MPIR_Request * req)
{
    req->dev.ch4.am.netmod_am.portals4.pack_buffer = NULL;
    req->dev.ch4.am.netmod_am.portals4.md = PTL_INVALID_HANDLE;
}

static inline void MPIDI_NM_am_request_finalize(MPIR_Request * req)
{
    if ((req)->dev.ch4.am.netmod_am.portals4.pack_buffer) {
        MPL_free((req)->dev.ch4.am.netmod_am.portals4.pack_buffer);
    }
    if ((req)->dev.ch4.am.netmod_am.portals4.md != PTL_INVALID_HANDLE) {
        PtlMDRelease((req)->dev.ch4.am.netmod_am.portals4.md);
    }
}

#endif /* PTL_REQUEST_H_INCLUDED */
