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
#ifndef OFI_AM_H_INCLUDED
#define OFI_AM_H_INCLUDED
#include "ofi_impl.h"
#include "ofi_am_impl.h"
#include "ofi_am_events.h"

static inline int MPIDI_OFI_progress_do_queue(int vci_idx);

static inline void MPIDI_NM_am_request_init(MPIR_Request * req)
{
    MPIDI_OFI_AMREQUEST(req, req_hdr) = NULL;
}

static inline void MPIDI_NM_am_request_finalize(MPIR_Request * req)
{
    MPIDI_OFI_am_clear_request(req);
}

static inline int MPIDI_NM_am_isend(int rank, MPIR_Comm * comm, int handler_id, const void *am_hdr,
                                    size_t am_hdr_sz, const void *data, MPI_Count count,
                                    MPI_Datatype datatype, MPIR_Request * sreq,
                                    MPIDI_av_entry_t * av)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_NETMOD_AM_ISEND);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_NETMOD_AM_ISEND);

    mpi_errno = MPIDI_OFI_do_am_isend(rank, comm, handler_id, am_hdr, am_hdr_sz, data, count,
                                      datatype, sreq, av);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_NETMOD_AM_ISEND);
    return mpi_errno;
}

static inline int MPIDI_NM_am_isendv(int rank, MPIR_Comm * comm, int handler_id,
                                     struct iovec *am_hdr, size_t iov_len, const void *data,
                                     MPI_Count count, MPI_Datatype datatype, MPIR_Request * sreq,
                                     MPIDI_av_entry_t * av)
{
    int mpi_errno = MPI_SUCCESS, is_allocated;
    size_t am_hdr_sz = 0, i;
    char *am_hdr_buf;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_NETMOD_OFI_AM_ISENDV);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_NETMOD_OFI_AM_ISENDV);

    for (i = 0; i < iov_len; i++) {
        am_hdr_sz += am_hdr[i].iov_len;
    }

    if (am_hdr_sz > MPIDI_OFI_BUF_POOL_SIZE) {
        am_hdr_buf = (char *) MPL_malloc(am_hdr_sz, MPL_MEM_BUFFER);
        is_allocated = 1;
    } else {
        am_hdr_buf = (char *) MPIDIU_get_buf(MPIDI_OFI_global.am_buf_pool);
        is_allocated = 0;
    }

    MPIR_Assert(am_hdr_buf);
    am_hdr_sz = 0;

    for (i = 0; i < iov_len; i++) {
        MPIR_Memcpy(am_hdr_buf + am_hdr_sz, am_hdr[i].iov_base, am_hdr[i].iov_len);
        am_hdr_sz += am_hdr[i].iov_len;
    }

    mpi_errno = MPIDI_OFI_do_am_isend(rank, comm, handler_id, am_hdr_buf, am_hdr_sz, data, count,
                                      datatype, sreq, av);

    if (is_allocated)
        MPL_free(am_hdr_buf);
    else
        MPIDIU_release_buf(am_hdr_buf);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_NETMOD_OFI_AM_ISENDV);
    return mpi_errno;
}

static inline int MPIDI_NM_am_isend_reply(int src_rank, MPIR_Comm * comm,
                                          int handler_id, const void *am_hdr, size_t am_hdr_sz,
                                          const void *data, MPI_Count count, MPI_Datatype datatype,
                                          MPIR_Request * sreq, MPIDI_av_entry_t * av)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_NETMOD_AM_ISEND_REPLY);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_NETMOD_AM_ISEND_REPLY);

    mpi_errno = MPIDI_OFI_do_am_isend(src_rank, comm, handler_id, am_hdr, am_hdr_sz, data, count,
                                      datatype, sreq, av);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_NETMOD_AM_ISEND_REPLY);
    return mpi_errno;
}

static inline size_t MPIDI_NM_am_hdr_max_sz(void)
{
    /* Maximum size that fits in short send */
    size_t max_shortsend = MPIDI_OFI_DEFAULT_SHORT_SEND_SIZE -
        (sizeof(MPIDI_OFI_am_header_t) + sizeof(MPIDI_OFI_lmt_msg_payload_t));
    /* Maximum payload size representable by MPIDI_OFI_am_header_t::am_hdr_sz field */
    size_t max_representable = (1 << MPIDI_OFI_AM_HDR_SZ_BITS) - 1;

    return MPL_MIN(max_shortsend, max_representable);
}

static inline int MPIDI_NM_am_send_hdr(int rank, MPIR_Comm * comm, int handler_id,
                                       const void *am_hdr, size_t am_hdr_sz, MPIDI_av_entry_t * av)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_NETMOD_OFI_AM_SEND_HDR);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_NETMOD_OFI_AM_SEND_HDR);
    mpi_errno = MPIDI_OFI_do_inject(rank, comm, handler_id, am_hdr, am_hdr_sz, av);

    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_NETMOD_OFI_AM_SEND_HDR);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static inline int MPIDI_NM_am_send_hdr_reply(int src_rank, MPIR_Comm * comm, int handler_id,
                                             const void *am_hdr, size_t am_hdr_sz,
                                             MPIDI_av_entry_t * av)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_NETMOD_OFI_AM_SEND_HDR_REPLY);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_NETMOD_OFI_AM_SEND_HDR_REPLY);

    mpi_errno = MPIDI_OFI_do_inject(src_rank, comm, handler_id, am_hdr, am_hdr_sz, av);

    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_NETMOD_OFI_AM_SEND_HDR_REPLY);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#endif /* OFI_AM_H_INCLUDED */
