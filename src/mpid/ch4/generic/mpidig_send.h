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

#ifndef MPIDIG_SEND_H_INCLUDED
#define MPIDIG_SEND_H_INCLUDED

#include "ch4_impl.h"
int MPIDIG_am_send_init(void);
int MPIDIG_do_isend(const void *buf, MPI_Aint count, MPI_Datatype datatype,
                    int rank, int tag, MPIR_Comm * comm, int context_offset,
                    MPIDI_av_entry_t * addr, MPIR_Request * sreq, MPIR_Errflag_t errflag);

int MPIDIG_am_ssend_init(void);
int MPIDIG_do_ssend(const void *buf, MPI_Aint count, MPI_Datatype datatype,
                    int rank, int tag, MPIR_Comm * comm, int context_offset,
                    MPIDI_av_entry_t * addr, MPIR_Request * sreq, MPIR_Errflag_t errflag);

int MPIDIG_am_long_init(void);
int MPIDIG_do_long(const void *buf, MPI_Aint count, MPI_Datatype datatype,
                   int rank, int tag, MPIR_Comm * comm, int context_offset,
                   MPIDI_av_entry_t * addr, MPIR_Request * sreq, MPIR_Errflag_t errflag);

#define MPIDI_NONBLOCKING 0
#define MPIDI_BLOCKING    1

#define MPIDIGI_AM_SEND_HDR_SIZE 16
static inline int get_eager_limit(int is_local)
{
#ifdef MPIDI_CH4_DIRECT_NETMOD
    return MPIDI_NM_am_hdr_max_sz() - MPIDIGI_AM_SEND_HDR_SIZE;
#else
    if (is_local) {
        return MPIDI_SHM_am_hdr_max_sz() - MPIDIGI_AM_SEND_HDR_SIZE;
    } else {
        return MPIDI_NM_am_hdr_max_sz() - MPIDIGI_AM_SEND_HDR_SIZE;
    }
#endif
}

static inline int MPIDIG_isend_impl(const void *buf, MPI_Aint count, MPI_Datatype datatype,
                                    int rank, int tag, MPIR_Comm * comm, int context_offset,
                                    MPIDI_av_entry_t * addr, MPIR_Request ** request,
                                    int is_blocking, int type, MPIR_Errflag_t errflag)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_Request *sreq = *request;
    if (sreq == NULL) {
        sreq = MPIDIG_request_create(MPIR_REQUEST_KIND__SEND, 2);
        MPIR_ERR_CHKANDSTMT((sreq) == NULL, mpi_errno, MPIX_ERR_NOREQ, goto fn_fail, "**nomemreq");
    } else {
        MPIDIG_request_init(sreq, MPIR_REQUEST_KIND__SEND);
    }
    *request = sreq;

#ifdef HAVE_DEBUGGER_SUPPORT
    MPIDIG_REQUEST(sreq, datatype) = datatype;
    MPIDIG_REQUEST(sreq, buffer) = (char *) buf;
    MPIDIG_REQUEST(sreq, count) = count;
#endif

    size_t data_sz;
    MPIR_Datatype *dt_ptr;
    MPIDI_Datatype_get_size_dt_ptr(count, datatype, data_sz, dt_ptr);

    if (type == MPIDIG_SSEND_REQ) {
        mpi_errno =
            MPIDIG_do_ssend(buf, count, datatype, rank, tag, comm, context_offset, addr, sreq,
                            errflag);
    } else if (data_sz > get_eager_limit(MPIDI_av_is_local(addr))) {
        mpi_errno =
            MPIDIG_do_long(buf, count, datatype, rank, tag, comm, context_offset, addr, sreq,
                           errflag);
    } else {
        mpi_errno =
            MPIDIG_do_isend(buf, count, datatype, rank, tag, comm, context_offset, addr, sreq,
                            errflag);
    }
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

/* This performs the same work as MPIDIG_isend, but sets the
 * error bits from errflag into the am_hdr instance.
 * */
static inline int MPIDIG_isend_coll_impl(const void *buf, MPI_Aint count, MPI_Datatype datatype,
                                         int rank, int tag, MPIR_Comm * comm, int context_offset,
                                         MPIDI_av_entry_t * addr, MPIR_Request ** request,
                                         int is_blocking, int type, MPIR_Errflag_t * errflag)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIG_AM_ISEND_COLL);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIG_AM_ISEND_COLL);

    mpi_errno = MPIDIG_isend_impl(buf, count, datatype, rank, tag, comm, context_offset, addr,
                                  request, is_blocking, type, *errflag);
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIG_AM_ISEND_COLL);
    return mpi_errno;

  fn_fail:
    goto fn_exit;
}

static inline int MPIDIG_am_isend(const void *buf, MPI_Aint count, MPI_Datatype datatype,
                                  int rank, int tag, MPIR_Comm * comm, int context_offset,
                                  MPIDI_av_entry_t * addr, MPIR_Request ** request,
                                  int is_blocking, int type)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIG_AM_ISEND);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIG_AM_ISEND);

    mpi_errno = MPIDIG_isend_impl(buf, count, datatype, rank, tag, comm, context_offset, addr,
                                  request, is_blocking, type, MPIR_ERR_NONE);
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIG_AM_ISEND);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPIDIG_mpi_send(const void *buf,
                                             MPI_Aint count,
                                             MPI_Datatype datatype,
                                             int rank,
                                             int tag, MPIR_Comm * comm, int context_offset,
                                             MPIDI_av_entry_t * addr, MPIR_Request ** request)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIG_MPI_SEND);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIG_MPI_SEND);

    mpi_errno = MPIDIG_isend_impl(buf, count, datatype, rank, tag, comm, context_offset, addr,
                                  request, MPIDI_BLOCKING, MPIDIG_SEND, MPIR_ERR_NONE);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIG_MPI_SEND);
    return mpi_errno;
}

MPL_STATIC_INLINE_PREFIX int MPIDIG_send_coll(const void *buf, MPI_Aint count,
                                              MPI_Datatype datatype, int rank,
                                              int tag, MPIR_Comm * comm, int context_offset,
                                              MPIDI_av_entry_t * addr, MPIR_Request ** request,
                                              MPIR_Errflag_t * errflag)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIG_SEND_COLL);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIG_SEND_COLL);

    mpi_errno =
        MPIDIG_isend_impl(buf, count, datatype, rank, tag, comm, context_offset, addr, request,
                          MPIDI_BLOCKING, MPIDIG_SEND, *errflag);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIG_SEND_COLL);
    return mpi_errno;
}

MPL_STATIC_INLINE_PREFIX int MPIDIG_mpi_isend(const void *buf,
                                              MPI_Aint count,
                                              MPI_Datatype datatype,
                                              int rank,
                                              int tag, MPIR_Comm * comm, int context_offset,
                                              MPIDI_av_entry_t * addr, MPIR_Request ** request)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIG_MPI_ISEND);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIG_MPI_ISEND);

    mpi_errno = MPIDIG_isend_impl(buf, count, datatype, rank, tag, comm, context_offset, addr,
                                  request, MPIDI_NONBLOCKING, MPIDIG_SEND, MPIR_ERR_NONE);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIG_MPI_ISEND);
    return mpi_errno;
}


MPL_STATIC_INLINE_PREFIX int MPIDIG_isend_coll(const void *buf, MPI_Aint count,
                                               MPI_Datatype datatype, int rank,
                                               int tag, MPIR_Comm * comm, int context_offset,
                                               MPIDI_av_entry_t * addr,
                                               MPIR_Request ** request, MPIR_Errflag_t * errflag)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIG_ISEND_COLL);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIG_ISEND_COLL);

    mpi_errno =
        MPIDIG_isend_impl(buf, count, datatype, rank, tag, comm, context_offset, addr, request,
                          MPIDI_NONBLOCKING, MPIDIG_SEND, *errflag);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIG_ISEND_COLL);
    return mpi_errno;
}


MPL_STATIC_INLINE_PREFIX int MPIDIG_mpi_rsend(const void *buf,
                                              MPI_Aint count,
                                              MPI_Datatype datatype,
                                              int rank,
                                              int tag, MPIR_Comm * comm, int context_offset,
                                              MPIDI_av_entry_t * addr, MPIR_Request ** request)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIG_MPI_RSEND);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIG_MPI_RSEND);

    mpi_errno = MPIDIG_isend_impl(buf, count, datatype, rank, tag, comm, context_offset, addr,
                                  request, MPIDI_BLOCKING, MPIDIG_SEND, MPIR_ERR_NONE);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIG_MPI_RSEND);
    return mpi_errno;
}


MPL_STATIC_INLINE_PREFIX int MPIDIG_mpi_irsend(const void *buf,
                                               MPI_Aint count,
                                               MPI_Datatype datatype,
                                               int rank,
                                               int tag, MPIR_Comm * comm, int context_offset,
                                               MPIDI_av_entry_t * addr, MPIR_Request ** request)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIG_MPI_IRSEND);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIG_MPI_IRSEND);

    mpi_errno = MPIDIG_isend_impl(buf, count, datatype, rank, tag, comm, context_offset, addr,
                                  request, MPIDI_NONBLOCKING, MPIDIG_SEND, MPIR_ERR_NONE);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIG_MPI_IRSEND);
    return mpi_errno;
}

MPL_STATIC_INLINE_PREFIX int MPIDIG_mpi_ssend(const void *buf,
                                              MPI_Aint count,
                                              MPI_Datatype datatype,
                                              int rank,
                                              int tag, MPIR_Comm * comm, int context_offset,
                                              MPIDI_av_entry_t * addr, MPIR_Request ** request)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIG_MPI_SSEND);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIG_MPI_SSEND);

    mpi_errno = MPIDIG_isend_impl(buf, count, datatype, rank, tag, comm, context_offset, addr,
                                  request, MPIDI_BLOCKING, MPIDIG_SSEND_REQ, MPIR_ERR_NONE);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIG_MPI_SSEND);
    return mpi_errno;
}

MPL_STATIC_INLINE_PREFIX int MPIDIG_mpi_issend(const void *buf,
                                               MPI_Aint count,
                                               MPI_Datatype datatype,
                                               int rank,
                                               int tag, MPIR_Comm * comm, int context_offset,
                                               MPIDI_av_entry_t * addr, MPIR_Request ** request)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIG_MPI_ISSEND);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIG_MPI_ISSEND);

    mpi_errno = MPIDIG_isend_impl(buf, count, datatype, rank, tag, comm, context_offset, addr,
                                  request, MPIDI_NONBLOCKING, MPIDIG_SSEND_REQ, MPIR_ERR_NONE);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIG_MPI_ISSEND);
    return mpi_errno;
}

MPL_STATIC_INLINE_PREFIX int MPIDIG_mpi_cancel_send(MPIR_Request * sreq)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIG_MPI_CANCEL_SEND);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIG_MPI_CANCEL_SEND);

    /* cannot cancel send */

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIG_MPI_CANCEL_SEND);
    return mpi_errno;
}

#endif /* MPIDIG_SEND_H_INCLUDED */
