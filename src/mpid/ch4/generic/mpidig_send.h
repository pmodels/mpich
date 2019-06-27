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

#define MPIDI_NONBLOCKING 0
#define MPIDI_BLOCKING    1

static inline int MPIDIG_isend_impl(const void *buf, MPI_Aint count, MPI_Datatype datatype,
                                    int rank, int tag, MPIR_Comm * comm, int context_offset,
                                    MPIDI_av_entry_t * addr, MPIR_Request ** request,
                                    int is_blocking, int type, MPIR_Errflag_t errflag)
{
    int mpi_errno = MPI_SUCCESS, c;
    MPIR_Request *sreq = *request;
    MPIDIG_hdr_t am_hdr;
    MPIDIG_ssend_req_msg_t ssend_req;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIG_AM_ISEND_IMPL);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIG_AM_ISEND_IMPL);

    /* If the message is going to MPI_PROC_NULL, we still need to create and complete the request so
     * we have something we can pass back up the call stack to track the request status. */
    if (unlikely(rank == MPI_PROC_NULL)) {
        mpi_errno = MPI_SUCCESS;
        /* for blocking calls, we directly complete the request */
        if (!is_blocking || sreq != NULL) {
            if (sreq == NULL) {
                *request = MPIDIG_request_create(MPIR_REQUEST_KIND__SEND, 2);
                MPIR_ERR_CHKANDSTMT((*request) == NULL, mpi_errno, MPIX_ERR_NOREQ, goto fn_fail,
                                    "**nomemreq");
            } else {
                MPIDIG_request_init(sreq, MPIR_REQUEST_KIND__SEND);
            }
            MPID_Request_complete((*request));
        }
        goto fn_exit;
    }

    if (sreq == NULL) {
        sreq = MPIDIG_request_create(MPIR_REQUEST_KIND__SEND, 2);
        MPIR_ERR_CHKANDSTMT((sreq) == NULL, mpi_errno, MPIX_ERR_NOREQ, goto fn_fail, "**nomemreq");
    } else {
        MPIDIG_request_init(sreq, MPIR_REQUEST_KIND__SEND);
    }

    *request = sreq;

    am_hdr.src_rank = comm->rank;
    am_hdr.tag = tag;
    am_hdr.context_id = comm->context_id + context_offset;
    am_hdr.error_bits = errflag;

#ifdef HAVE_DEBUGGER_SUPPORT
    MPIDIG_REQUEST(sreq, datatype) = datatype;
    MPIDIG_REQUEST(sreq, buffer) = (char *) buf;
    MPIDIG_REQUEST(sreq, count) = count;
#endif
    /* Synchronous send requires a special kind of AM header to track the return message so check
     * for that and fill in the appropriate struct if necessary. */
#ifdef MPIDI_CH4_DIRECT_NETMOD
    if (type == MPIDIG_SSEND_REQ) {
        ssend_req.hdr = am_hdr;

        /* Increment the completion counter once to account for the extra message that needs to come
         * back from the receiver to indicate completion. */
        ssend_req.sreq_ptr = (uint64_t) sreq;
        MPIR_cc_incr(sreq->cc_ptr, &c);

        mpi_errno = MPIDI_NM_am_isend(rank, comm, type,
                                      &ssend_req, sizeof(ssend_req), buf, count, datatype, sreq);
    } else {
        mpi_errno = MPIDI_NM_am_isend(rank, comm, type,
                                      &am_hdr, sizeof(am_hdr), buf, count, datatype, sreq);
    }
#else
    if (type == MPIDIG_SSEND_REQ) {
        ssend_req.hdr = am_hdr;
        ssend_req.sreq_ptr = (uint64_t) sreq;

        /* Increment the completion counter once to account for the extra message that needs to come
         * back from the receiver to indicate completion. */
        MPIR_cc_incr(sreq->cc_ptr, &c);
        if (MPIDI_av_is_local(addr)) {
            mpi_errno = MPIDI_SHM_am_isend(rank, comm, type,
                                           &ssend_req, sizeof(ssend_req), buf,
                                           count, datatype, sreq);
        } else {
            mpi_errno = MPIDI_NM_am_isend(rank, comm, type,
                                          &ssend_req, sizeof(ssend_req), buf,
                                          count, datatype, sreq);
        }
    } else {
        if (MPIDI_av_is_local(addr)) {
            mpi_errno = MPIDI_SHM_am_isend(rank, comm, type,
                                           &am_hdr, sizeof(am_hdr), buf, count, datatype, sreq);
        } else {
            mpi_errno = MPIDI_NM_am_isend(rank, comm, type,
                                          &am_hdr, sizeof(am_hdr), buf, count, datatype, sreq);
        }
    }
#endif
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIG_AM_ISEND_IMPL);
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
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

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
                                  request, is_blocking, type, MPI_SUCCESS);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIG_AM_ISEND);
    return mpi_errno;

  fn_fail:
    goto fn_exit;
}

static inline int MPIDIG_psend_init(const void *buf, int count, MPI_Datatype datatype, int rank,
                                    int tag, MPIR_Comm * comm, int context_offset,
                                    MPIR_Request ** request)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Request *sreq;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIG_PSEND_INIT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIG_PSEND_INIT);

    sreq = MPIDIG_request_create(MPIR_REQUEST_KIND__PREQUEST_SEND, 2);
    MPIR_ERR_CHKANDSTMT(sreq == NULL, mpi_errno, MPIX_ERR_NOREQ, goto fn_fail, "**nomemreq");
    *request = sreq;

    MPIR_Comm_add_ref(comm);
    sreq->comm = comm;

    MPIDIG_REQUEST(sreq, buffer) = (void *) buf;
    MPIDIG_REQUEST(sreq, count) = count;
    MPIDIG_REQUEST(sreq, datatype) = datatype;
    MPIDIG_REQUEST(sreq, rank) = rank;
    MPIDIG_REQUEST(sreq, tag) = tag;
    MPIDIG_REQUEST(sreq, context_id) = comm->context_id + context_offset;

    sreq->u.persist.real_request = NULL;
    MPID_Request_complete(sreq);

    MPIR_Datatype_add_ref_if_not_builtin(datatype);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIG_PSEND_INIT);
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

    mpi_errno = MPIDIG_am_isend(buf, count, datatype, rank, tag, comm, context_offset, addr,
                                request, MPIDI_BLOCKING, MPIDIG_SEND);

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
        MPIDIG_isend_coll_impl(buf, count, datatype, rank, tag, comm, context_offset, addr, request,
                               MPIDI_BLOCKING, MPIDIG_SEND, errflag);

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

    mpi_errno = MPIDIG_am_isend(buf, count, datatype, rank, tag, comm, context_offset, addr,
                                request, MPIDI_NONBLOCKING, MPIDIG_SEND);

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
        MPIDIG_isend_coll_impl(buf, count, datatype, rank, tag, comm, context_offset, addr, request,
                               MPIDI_NONBLOCKING, MPIDIG_SEND, errflag);

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

    mpi_errno = MPIDIG_am_isend(buf, count, datatype, rank, tag, comm, context_offset, addr,
                                request, MPIDI_BLOCKING, MPIDIG_SEND);

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

    mpi_errno = MPIDIG_am_isend(buf, count, datatype, rank, tag, comm, context_offset, addr,
                                request, MPIDI_NONBLOCKING, MPIDIG_SEND);

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

    mpi_errno = MPIDIG_am_isend(buf, count, datatype, rank, tag, comm, context_offset, addr,
                                request, MPIDI_BLOCKING, MPIDIG_SSEND_REQ);

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

    mpi_errno = MPIDIG_am_isend(buf, count, datatype, rank, tag, comm, context_offset, addr,
                                request, MPIDI_NONBLOCKING, MPIDIG_SSEND_REQ);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIG_MPI_ISSEND);
    return mpi_errno;
}

MPL_STATIC_INLINE_PREFIX int MPIDIG_mpi_send_init(const void *buf,
                                                  int count,
                                                  MPI_Datatype datatype,
                                                  int rank,
                                                  int tag, MPIR_Comm * comm, int context_offset,
                                                  MPIDI_av_entry_t * addr, MPIR_Request ** request)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIG_MPI_SEND_INIT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIG_MPI_SEND_INIT);

    mpi_errno = MPIDIG_psend_init(buf, count, datatype, rank, tag, comm, context_offset, request);
    MPIDIG_REQUEST((*request), p_type) = MPIDI_PTYPE_SEND;

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIG_MPI_SEND_INIT);
    return mpi_errno;
}

MPL_STATIC_INLINE_PREFIX int MPIDIG_mpi_ssend_init(const void *buf,
                                                   int count,
                                                   MPI_Datatype datatype,
                                                   int rank,
                                                   int tag, MPIR_Comm * comm, int context_offset,
                                                   MPIDI_av_entry_t * addr, MPIR_Request ** request)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIG_MPI_SSEND_INIT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIG_MPI_SSEND_INIT);

    mpi_errno = MPIDIG_psend_init(buf, count, datatype, rank, tag, comm, context_offset, request);
    MPIDIG_REQUEST((*request), p_type) = MPIDI_PTYPE_SSEND;

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIG_MPI_SSEND_INIT);
    return mpi_errno;
}

MPL_STATIC_INLINE_PREFIX int MPIDIG_mpi_bsend_init(const void *buf,
                                                   int count,
                                                   MPI_Datatype datatype,
                                                   int rank,
                                                   int tag, MPIR_Comm * comm, int context_offset,
                                                   MPIDI_av_entry_t * addr, MPIR_Request ** request)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIG_MPI_BSEND_INIT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIG_MPI_BSEND_INIT);

    mpi_errno = MPIDIG_psend_init(buf, count, datatype, rank, tag, comm, context_offset, request);
    MPIDIG_REQUEST((*request), p_type) = MPIDI_PTYPE_BSEND;

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIG_MPI_BSEND_INIT);
    return mpi_errno;
}

MPL_STATIC_INLINE_PREFIX int MPIDIG_mpi_rsend_init(const void *buf,
                                                   int count,
                                                   MPI_Datatype datatype,
                                                   int rank,
                                                   int tag, MPIR_Comm * comm, int context_offset,
                                                   MPIDI_av_entry_t * addr, MPIR_Request ** request)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIG_MPI_RSEND_INIT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIG_MPI_RSEND_INIT);

    mpi_errno = MPIDIG_psend_init(buf, count, datatype, rank, tag, comm, context_offset, request);
    MPIDIG_REQUEST((*request), p_type) = MPIDI_PTYPE_SEND;

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIG_MPI_RSEND_INIT);
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
