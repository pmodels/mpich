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
#ifndef SHM_AM_IMPL_H_INCLUDED
#define SHM_AM_IMPL_H_INCLUDED

#include "shmam_impl.h"

#undef FUNCNAME
#define FUNCNAME MPIDI_SHM_am_clear_request
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline void MPIDI_SHM_am_clear_request(MPIR_Request * sreq)
{
    MPIDI_SHM_am_request_header_t *req_hdr;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_SHM_AM_SHM_CLEAR_REQ);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_SHM_AM_SHM_CLEAR_REQ);

    req_hdr = MPIDI_SHM_AMREQUEST(sreq, req_hdr);

    if (!req_hdr)
        return;

    if (req_hdr->am_hdr != &req_hdr->am_hdr_buf[0]) {
        MPL_free(req_hdr->am_hdr);
    }

#ifndef SHM_SHMAM_AM_REQUEST_INLINE
    MPIDI_CH4R_release_buf(req_hdr);
#endif /* SHM_SHMAM_AM_REQUEST_INLINE */
    MPIDI_SHM_AMREQUEST(sreq, req_hdr) = NULL;

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_SHM_AM_SHM_CLEAR_REQ);
    return;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_SHM_am_init_request
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_SHM_am_init_request(const void *am_hdr,
                                            size_t am_hdr_sz, MPIR_Request * sreq)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_SHM_am_request_header_t *req_hdr;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_SHM_AM_SHM_INIT_REQ);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_SHM_AM_SHM_INIT_REQ);

    if (MPIDI_SHM_AMREQUEST(sreq, req_hdr) == NULL) {
#ifndef SHM_SHMAM_AM_REQUEST_INLINE
        req_hdr = (MPIDI_SHM_am_request_header_t *)
            MPIDI_CH4R_get_buf(MPIDI_SHM_Shmam_global.am_buf_pool);
#else /* SHM_SHMAM_AM_REQUEST_INLINE */
        req_hdr = &MPIDI_SHM_AMREQUEST(sreq, req_hdr_buffer);
#endif /* SHM_SHMAM_AM_REQUEST_INLINE */
        MPIR_Assert(req_hdr);
        MPIDI_SHM_AMREQUEST(sreq, req_hdr) = req_hdr;

        req_hdr->am_hdr = (void *) &req_hdr->am_hdr_buf[0];
        req_hdr->am_hdr_sz = MPIDI_SHM_MAX_AM_HDR_SIZE;
    }
    else {
        req_hdr = MPIDI_SHM_AMREQUEST(sreq, req_hdr);
    }

    if (am_hdr_sz > req_hdr->am_hdr_sz) {
        if (req_hdr->am_hdr != &req_hdr->am_hdr_buf[0])
            MPL_free(req_hdr->am_hdr);

        req_hdr->am_hdr = MPL_malloc(am_hdr_sz);
        MPIR_Assert(req_hdr->am_hdr);
    }

    if (am_hdr) {
        MPIR_Memcpy(req_hdr->am_hdr, am_hdr, am_hdr_sz);
    }

    req_hdr->am_hdr_sz = am_hdr_sz;

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_SHM_AM_SHM_INIT_REQ);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_SHM_am_request_complete
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline void MPIDI_SHM_am_request_complete(MPIR_Request * req)
{
    int incomplete;
    MPIR_cc_decr(req->cc_ptr, &incomplete);

    if (!incomplete) {
        MPIDI_CH4U_request_release(req);
    }
}

#endif /*SHM_AM_IMPL_H_INCLUDED */
