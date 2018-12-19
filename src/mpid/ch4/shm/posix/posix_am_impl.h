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
#define FUNCNAME MPIDI_POSIX_am_release_req_hdr
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_POSIX_am_release_req_hdr(MPIDI_POSIX_am_request_header_t ** req_hdr_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_POSIX_AM_RELEASE_REQ_HDR);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_POSIX_AM_RELEASE_REQ_HDR);

    if ((*req_hdr_ptr)->am_hdr != &(*req_hdr_ptr)->am_hdr_buf[0]) {
        MPL_free((*req_hdr_ptr)->am_hdr);
    }
#ifndef POSIX_AM_REQUEST_INLINE
    MPIDI_CH4R_release_buf((*req_hdr_ptr));
#endif

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_POSIX_AM_RELEASE_REQ_HDR);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_POSIX_am_init_req_hdr
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_POSIX_am_init_req_hdr(const void *am_hdr,
                                              size_t am_hdr_sz,
                                              MPIDI_POSIX_am_request_header_t ** req_hdr_ptr,
                                              MPIR_Request * sreq)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_POSIX_am_request_header_t *req_hdr = *req_hdr_ptr;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_POSIX_AM_INIT_REQ_HDR);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_POSIX_AM_INIT_REQ_HDR);

#ifdef POSIX_AM_REQUEST_INLINE
    if (req_hdr == NULL && sreq != NULL) {
        req_hdr = &MPIDI_POSIX_AMREQUEST(sreq, req_hdr_buffer);
    }
#endif /* POSIX_AM_REQUEST_INLINE */

    if (req_hdr == NULL) {
        req_hdr = (MPIDI_POSIX_am_request_header_t *)
            MPIDI_CH4R_get_buf(MPIDI_POSIX_global.am_buf_pool);
        MPIR_ERR_CHKANDJUMP(!req_hdr, mpi_errno, MPI_ERR_OTHER, "**nomem");

        req_hdr->am_hdr = (void *) &req_hdr->am_hdr_buf[0];
        req_hdr->am_hdr_sz = MPIDI_POSIX_MAX_AM_HDR_SIZE;

        req_hdr->pack_buffer = NULL;

    }

    /* If the header is larger than what we'd preallocated, get rid of the preallocated buffer and
     * create a new one of the correct size. */
    if (am_hdr_sz > req_hdr->am_hdr_sz) {
        if (req_hdr->am_hdr != &req_hdr->am_hdr_buf[0])
            MPL_free(req_hdr->am_hdr);

        req_hdr->am_hdr = MPL_malloc(am_hdr_sz, MPL_MEM_SHM);
        MPIR_ERR_CHKANDJUMP(!(req_hdr->am_hdr), mpi_errno, MPI_ERR_OTHER, "**nomem");
    }

    if (am_hdr) {
        MPIR_Memcpy(req_hdr->am_hdr, am_hdr, am_hdr_sz);
    }

    req_hdr->am_hdr_sz = am_hdr_sz;
    *req_hdr_ptr = req_hdr;

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_POSIX_AM_INIT_REQ_HDR);
    return mpi_errno;

  fn_fail:
    goto fn_exit;
}

#endif /* POSIX_AM_IMPL_H_INCLUDED */
