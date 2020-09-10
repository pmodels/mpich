/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef POSIX_AM_IMPL_H_INCLUDED
#define POSIX_AM_IMPL_H_INCLUDED

#include "posix_types.h"
#include "posix_impl.h"
#include "mpidu_genq.h"

MPL_STATIC_INLINE_PREFIX int MPIDI_POSIX_am_release_req_hdr(MPIDI_POSIX_am_request_header_t **
                                                            req_hdr_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_POSIX_AM_RELEASE_REQ_HDR);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_POSIX_AM_RELEASE_REQ_HDR);

#ifndef POSIX_AM_REQUEST_INLINE
    MPIDU_genq_private_pool_free_cell(MPIDI_POSIX_global.am_hdr_buf_pool, (*req_hdr_ptr));
#endif

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_POSIX_AM_RELEASE_REQ_HDR);
    return mpi_errno;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_POSIX_am_init_req_hdr(const void *am_hdr,
                                                         size_t am_hdr_sz,
                                                         MPIDI_POSIX_am_request_header_t **
                                                         req_hdr_ptr, MPIR_Request * sreq)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_POSIX_am_request_header_t *req_hdr = *req_hdr_ptr;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_POSIX_AM_INIT_REQ_HDR);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_POSIX_AM_INIT_REQ_HDR);

#ifdef POSIX_AM_REQUEST_INLINE
    if (req_hdr == NULL && sreq != NULL) {
        req_hdr = &MPIDI_POSIX_AMREQUEST(sreq, req_hdr_buffer);
    }
#endif /* POSIX_AM_REQUEST_INLINE */

    if (req_hdr == NULL) {
        MPIDU_genq_private_pool_alloc_cell(MPIDI_POSIX_global.am_hdr_buf_pool, (void **) &req_hdr);
        MPIR_ERR_CHKANDJUMP(!req_hdr, mpi_errno, MPI_ERR_OTHER, "**nomem");

        req_hdr->am_hdr = (void *) &req_hdr->am_hdr_buf[0];
        req_hdr->am_hdr_sz = am_hdr_sz;

        req_hdr->pack_buffer = NULL;
    }

    if (am_hdr) {
        MPIR_Typerep_copy(req_hdr->am_hdr, am_hdr, am_hdr_sz);
    }

    *req_hdr_ptr = req_hdr;

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_POSIX_AM_INIT_REQ_HDR);
    return mpi_errno;

  fn_fail:
    goto fn_exit;
}

#endif /* POSIX_AM_IMPL_H_INCLUDED */
