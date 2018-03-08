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
#ifndef POSIX_AM_IMPL_H_INCLUDED
#define POSIX_AM_IMPL_H_INCLUDED

#include "posix_impl.h"

#undef FUNCNAME
#define FUNCNAME MPIDI_POSIX_am_clear_request
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline void MPIDI_POSIX_am_clear_request(MPIR_Request * sreq)
{
    MPIDI_POSIX_am_request_header_t *req_hdr;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_POSIX_AM_CLEAR_REQ);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_POSIX_AM_CLEAR_REQ);

    MPIDI_POSIX_EAGER_RECV_COMPLETED_HOOK(sreq);

    req_hdr = MPIDI_POSIX_AMREQUEST(sreq, req_hdr);

    if (!req_hdr)
        return;

    if (req_hdr->am_hdr != &req_hdr->am_hdr_buf[0]) {
        MPL_free(req_hdr->am_hdr);
    }
#ifndef POSIX_AM_REQUEST_INLINE
    MPIDI_CH4R_release_buf(req_hdr);
#endif /* POSIX_AM_REQUEST_INLINE */
    MPIDI_POSIX_AMREQUEST(sreq, req_hdr) = NULL;

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_POSIX_AM_CLEAR_REQ);
    return;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_POSIX_am_init_request
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_POSIX_am_init_request(const void *am_hdr,
                                              size_t am_hdr_sz, MPIR_Request * sreq)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_POSIX_am_request_header_t *req_hdr;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_POSIX_AM_INIT_REQ);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_POSIX_AM_INIT_REQ);

    if (MPIDI_POSIX_AMREQUEST(sreq, req_hdr) == NULL) {
#ifndef POSIX_AM_REQUEST_INLINE
        req_hdr = (MPIDI_POSIX_am_request_header_t *)
            MPIDI_CH4R_get_buf(MPIDI_POSIX_global.am_buf_pool);
#else /* POSIX_AM_REQUEST_INLINE */
        req_hdr = &MPIDI_POSIX_AMREQUEST(sreq, req_hdr_buffer);
#endif /* POSIX_AM_REQUEST_INLINE */
        MPIR_Assert(req_hdr);
        MPIDI_POSIX_AMREQUEST(sreq, req_hdr) = req_hdr;

        req_hdr->am_hdr = (void *) &req_hdr->am_hdr_buf[0];
        req_hdr->am_hdr_sz = MPIDI_POSIX_MAX_AM_HDR_SIZE;
    } else {
        req_hdr = MPIDI_POSIX_AMREQUEST(sreq, req_hdr);
    }

    if (am_hdr_sz > req_hdr->am_hdr_sz) {
        if (req_hdr->am_hdr != &req_hdr->am_hdr_buf[0])
            MPL_free(req_hdr->am_hdr);

        req_hdr->am_hdr = MPL_malloc(am_hdr_sz, MPL_MEM_SHM);
        MPIR_Assert(req_hdr->am_hdr);
    }

    if (am_hdr) {
        MPIR_Memcpy(req_hdr->am_hdr, am_hdr, am_hdr_sz);
    }

    req_hdr->am_hdr_sz = am_hdr_sz;

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_POSIX_AM_INIT_REQ);
    return mpi_errno;
}

#endif /* POSIX_AM_IMPL_H_INCLUDED */
