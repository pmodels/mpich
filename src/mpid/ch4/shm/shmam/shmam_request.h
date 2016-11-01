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
#ifndef SHM_SHMAM_REQUEST_H_INCLUDED
#define SHM_SHMAM_REQUEST_H_INCLUDED

#include "shmam_impl.h"

static inline void MPIDI_SHM_am_request_init(MPIR_Request * req)
{
    MPIDI_SHM_AMREQUEST(req, req_hdr) = NULL;

#ifdef SHM_AM_DEBUG
    fprintf(MPIDI_SHM_Shmam_global.logfile, "Created request %d\n", req->kind);
    fflush(MPIDI_SHM_Shmam_global.logfile);
#endif /* SHM_AM_DEBUG */
}

static inline void MPIDI_SHM_am_request_finalize(MPIR_Request * req)
{
    MPIDI_SHM_am_request_header_t *req_hdr;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_SHM_AM_OFI_CLEAR_REQ);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_SHM_AM_OFI_CLEAR_REQ);

    req_hdr = MPIDI_SHM_AMREQUEST(req, req_hdr);

    if (!req_hdr)
        return;

    MPIDI_SHM_Shmam_global.active_rreq[req_hdr->dst_grank] = NULL;

#ifdef SHM_AM_DEBUG
    fprintf(MPIDI_SHM_Shmam_global.logfile,
            "Completed request %d (%d)\n", req->kind, req_hdr->dst_grank);
    fflush(MPIDI_SHM_Shmam_global.logfile);
#endif /* SHM_AM_DEBUG */

    if (req_hdr->am_hdr != &req_hdr->am_hdr_buf[0]) {
        MPL_free(req_hdr->am_hdr);
    }

    MPIDI_CH4R_release_buf(req_hdr);
    MPIDI_SHM_AMREQUEST(req, req_hdr) = NULL;

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_NETMOD_AM_OFI_CLEAR_REQ);
    return;
}

#endif /* SHM_SHMAM_REQUEST_H_INCLUDED */
