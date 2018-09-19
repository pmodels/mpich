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

#undef FUNCNAME
#define FUNCNAME MPIDI_am_isend
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_am_isend(const void *buf, MPI_Aint count, MPI_Datatype datatype,
                                 int rank, int tag, MPIR_Comm * comm, int context_offset,
                                 MPIDI_av_entry_t * addr, MPIR_Request ** request,
                                 int is_blocking, int type)
{
    int mpi_errno = MPI_SUCCESS, c;
    MPIR_Request *sreq = NULL;
    MPIDI_CH4U_hdr_t am_hdr;
    MPIDI_CH4U_ssend_req_msg_t ssend_req;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_AM_ISEND);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_AM_ISEND);

    /* If the message is going to MPI_PROC_NULL, we still need to create and complete the request so
     * we have something we can pass back up the call stack to track the request status. */
    if (unlikely(rank == MPI_PROC_NULL)) {
        mpi_errno = MPI_SUCCESS;
        /* for blocking calls, we directly complete the request */
        if (!is_blocking) {
            *request = MPIDI_CH4I_am_request_create(MPIR_REQUEST_KIND__SEND, 2);
            MPIR_ERR_CHKANDSTMT((*request) == NULL, mpi_errno, MPIX_ERR_NOREQ, goto fn_fail,
                                "**nomemreq");
            MPID_Request_complete((*request));
        }
        goto fn_exit;
    }

    sreq = MPIDI_CH4I_am_request_create(MPIR_REQUEST_KIND__SEND, 2);
    MPIR_ERR_CHKANDSTMT((sreq) == NULL, mpi_errno, MPIX_ERR_NOREQ, goto fn_fail, "**nomemreq");

    *request = sreq;

    am_hdr.src_rank = comm->rank;
    am_hdr.tag = tag;
    am_hdr.context_id = comm->context_id + context_offset;

    /* Synchronous send requires a special kind of AM header to track the return message so check
     * for that and fill in the appropriate struct if necessary. */
#ifdef MPIDI_CH4_DIRECT_NETMOD
    if (type == MPIDI_CH4U_SSEND_REQ) {
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
    if (type == MPIDI_CH4U_SSEND_REQ) {
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
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_AM_ISEND);
    return mpi_errno;

  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_psend_init
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_psend_init(const void *buf,
                                   int count,
                                   MPI_Datatype datatype,
                                   int rank,
                                   int tag, MPIR_Comm * comm, int context_offset,
                                   MPIR_Request ** request)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Request *sreq;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_PSEND_INIT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_PSEND_INIT);

    sreq = MPIDI_CH4I_am_request_create(MPIR_REQUEST_KIND__PREQUEST_SEND, 2);
    MPIR_ERR_CHKANDSTMT(sreq == NULL, mpi_errno, MPIX_ERR_NOREQ, goto fn_fail, "**nomemreq");
    *request = sreq;

    MPIR_Comm_add_ref(comm);
    sreq->comm = comm;

    MPIDI_CH4U_REQUEST(sreq, buffer) = (void *) buf;
    MPIDI_CH4U_REQUEST(sreq, count) = count;
    MPIDI_CH4U_REQUEST(sreq, datatype) = datatype;
    MPIDI_CH4U_REQUEST(sreq, rank) = rank;
    MPIDI_CH4U_REQUEST(sreq, tag) = tag;
    MPIDI_CH4U_REQUEST(sreq, context_id) = comm->context_id + context_offset;

    sreq->u.persist.real_request = NULL;
    MPID_Request_complete(sreq);

    MPIR_Datatype_add_ref_if_not_builtin(datatype);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_PSEND_INIT);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}


#undef FUNCNAME
#define FUNCNAME MPIDIG_mpi_send
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
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

    mpi_errno =
        MPIDI_am_isend(buf, count, datatype, rank, tag, comm, context_offset, addr, request,
                       MPIDI_BLOCKING, MPIDI_CH4U_SEND);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIG_MPI_SEND);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDIG_mpi_isend
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
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

    mpi_errno =
        MPIDI_am_isend(buf, count, datatype, rank, tag, comm, context_offset, addr, request,
                       MPIDI_NONBLOCKING, MPIDI_CH4U_SEND);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIG_MPI_ISEND);
    return mpi_errno;
}


#undef FUNCNAME
#define FUNCNAME MPIDIG_mpi_rsend
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
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

    mpi_errno =
        MPIDI_am_isend(buf, count, datatype, rank, tag, comm, context_offset, addr, request,
                       MPIDI_BLOCKING, MPIDI_CH4U_SEND);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIG_MPI_RSEND);
    return mpi_errno;
}


#undef FUNCNAME
#define FUNCNAME MPIDIG_mpi_irsend
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
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

    mpi_errno =
        MPIDI_am_isend(buf, count, datatype, rank, tag, comm, context_offset, addr, request,
                       MPIDI_NONBLOCKING, MPIDI_CH4U_SEND);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIG_MPI_IRSEND);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDIG_mpi_ssend
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
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

    mpi_errno =
        MPIDI_am_isend(buf, count, datatype, rank, tag, comm, context_offset, addr, request,
                       MPIDI_BLOCKING, MPIDI_CH4U_SSEND_REQ);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIG_MPI_SSEND);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDIG_mpi_issend
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
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

    mpi_errno =
        MPIDI_am_isend(buf, count, datatype, rank, tag, comm, context_offset, addr, request,
                       MPIDI_NONBLOCKING, MPIDI_CH4U_SSEND_REQ);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIG_MPI_ISSEND);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDIG_mpi_send_init
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
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

    mpi_errno = MPIDI_psend_init(buf, count, datatype, rank, tag, comm, context_offset, request);
    MPIDI_CH4U_REQUEST((*request), p_type) = MPIDI_PTYPE_SEND;

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIG_MPI_SEND_INIT);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDIG_mpi_ssend_init
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
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

    mpi_errno = MPIDI_psend_init(buf, count, datatype, rank, tag, comm, context_offset, request);
    MPIDI_CH4U_REQUEST((*request), p_type) = MPIDI_PTYPE_SSEND;

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIG_MPI_SSEND_INIT);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDIG_mpi_bsend_init
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
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

    mpi_errno = MPIDI_psend_init(buf, count, datatype, rank, tag, comm, context_offset, request);
    MPIDI_CH4U_REQUEST((*request), p_type) = MPIDI_PTYPE_BSEND;

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIG_MPI_BSEND_INIT);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDIG_mpi_rsend_init
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
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

    mpi_errno = MPIDI_psend_init(buf, count, datatype, rank, tag, comm, context_offset, request);
    MPIDI_CH4U_REQUEST((*request), p_type) = MPIDI_PTYPE_SEND;

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIG_MPI_RSEND_INIT);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDIG_mpi_cancel_send
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
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
