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
#include <../mpi/pt2pt/bsendutil.h>

#undef FUNCNAME
#define FUNCNAME MPIDI_am_isend
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_am_isend(const void *buf, int count, MPI_Datatype datatype,
                                 int rank, int tag, MPIR_Comm * comm, int context_offset,
                                 MPIR_Request ** request, int is_blocking, int type)
{
    int mpi_errno = MPI_SUCCESS, c;
    MPIR_Request *sreq = NULL;
    uint64_t match_bits;
    MPIDI_CH4U_hdr_t am_hdr;
    MPIDI_CH4U_ssend_req_msg_t ssend_req;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_AM_ISEND);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_AM_ISEND);

    if (unlikely(rank == MPI_PROC_NULL)) {
        mpi_errno = MPI_SUCCESS;
        /* for blocking calls, we directly complete the request */
        if (!is_blocking) {
            *request = MPIDI_CH4I_am_request_create(MPIR_REQUEST_KIND__SEND, 2);
            MPID_Request_complete((*request));
        }
        goto fn_exit;
    }

    sreq = MPIDI_CH4I_am_request_create(MPIR_REQUEST_KIND__SEND, 2);
    MPIR_Assert(sreq);

    *request = sreq;
    match_bits = MPIDI_CH4U_init_send_tag(comm->context_id + context_offset, comm->rank, tag);

    am_hdr.msg_tag = match_bits;
    am_hdr.src_rank = comm->rank;
    if (type == MPIDI_CH4U_SSEND_REQ) {
        ssend_req.hdr = am_hdr;
        ssend_req.sreq_ptr = (uint64_t) sreq;
        MPIR_cc_incr(sreq->cc_ptr, &c);

        mpi_errno = MPIDI_NM_am_isend(rank, comm, MPIDI_CH4U_SSEND_REQ,
                                      &ssend_req, sizeof(ssend_req),
                                      buf, count, datatype, sreq);
        if (mpi_errno)
            MPIR_ERR_POP(mpi_errno);
    }
    else {
        mpi_errno = MPIDI_NM_am_isend(rank, comm, MPIDI_CH4U_SEND,
                                      &am_hdr, sizeof(am_hdr), buf, count, datatype, sreq);
        if (mpi_errno)
            MPIR_ERR_POP(mpi_errno);
    }

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
    MPIR_Request *sreq;
    uint64_t match_bits;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_PSEND_INIT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_PSEND_INIT);

    sreq = MPIDI_CH4I_am_request_create(MPIR_REQUEST_KIND__PREQUEST_SEND, 2);
    *request = sreq;

    MPIR_Comm_add_ref(comm);
    sreq->comm = comm;
    match_bits = MPIDI_CH4U_init_send_tag(comm->context_id + context_offset, rank, tag);

    MPIDI_CH4U_REQUEST(sreq, buffer) = (void *) buf;
    MPIDI_CH4U_REQUEST(sreq, count) = count;
    MPIDI_CH4U_REQUEST(sreq, datatype) = datatype;
    MPIDI_CH4U_REQUEST(sreq, tag) = match_bits;
    MPIDI_CH4U_REQUEST(sreq, rank) = rank;

    sreq->u.persist.real_request = NULL;
    MPID_Request_complete(sreq);

    dtype_add_ref_if_not_builtin(datatype);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_PSEND_INIT);
    return MPI_SUCCESS;
}


#undef FUNCNAME
#define FUNCNAME MPIDIG_mpi_send
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPIDIG_mpi_send(const void *buf,
                                             int count,
                                             MPI_Datatype datatype,
                                             int rank,
                                             int tag, MPIR_Comm * comm, int context_offset,
                                             MPIR_Request ** request)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIG_MPI_SEND);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIG_MPI_SEND);

    mpi_errno =
        MPIDI_am_isend(buf, count, datatype, rank, tag, comm, context_offset, request, 1, 0);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIG_MPI_SEND);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDIG_mpi_isend
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPIDIG_mpi_isend(const void *buf,
                                              int count,
                                              MPI_Datatype datatype,
                                              int rank,
                                              int tag, MPIR_Comm * comm, int context_offset,
                                              MPIR_Request ** request)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIG_MPI_ISEND);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIG_MPI_ISEND);

    mpi_errno =
        MPIDI_am_isend(buf, count, datatype, rank, tag, comm, context_offset, request, 0, 0);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIG_MPI_ISEND);
    return mpi_errno;
}


#undef FUNCNAME
#define FUNCNAME MPIDIG_mpi_rsend
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPIDIG_mpi_rsend(const void *buf,
                                              int count,
                                              MPI_Datatype datatype,
                                              int rank,
                                              int tag, MPIR_Comm * comm, int context_offset,
                                              MPIR_Request ** request)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIG_MPI_RSEND);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIG_MPI_RSEND);

    mpi_errno =
        MPIDI_am_isend(buf, count, datatype, rank, tag, comm, context_offset, request, 1, 0);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIG_MPI_RSEND);
    return mpi_errno;
}


#undef FUNCNAME
#define FUNCNAME MPIDIG_mpi_irsend
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPIDIG_mpi_irsend(const void *buf,
                                               int count,
                                               MPI_Datatype datatype,
                                               int rank,
                                               int tag, MPIR_Comm * comm, int context_offset,
                                               MPIR_Request ** request)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIG_MPI_IRSEND);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIG_MPI_IRSEND);

    mpi_errno =
        MPIDI_am_isend(buf, count, datatype, rank, tag, comm, context_offset, request, 0, 0);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIG_MPI_IRSEND);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDIG_mpi_ssend
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPIDIG_mpi_ssend(const void *buf,
                                              int count,
                                              MPI_Datatype datatype,
                                              int rank,
                                              int tag, MPIR_Comm * comm, int context_offset,
                                              MPIR_Request ** request)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIG_MPI_SSEND);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIG_MPI_SSEND);

    mpi_errno = MPIDI_am_isend(buf, count, datatype, rank, tag, comm,
                               context_offset, request, 1, MPIDI_CH4U_SSEND_REQ);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIG_MPI_SSEND);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDIG_mpi_issend
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPIDIG_mpi_issend(const void *buf,
                                               int count,
                                               MPI_Datatype datatype,
                                               int rank,
                                               int tag, MPIR_Comm * comm, int context_offset,
                                               MPIR_Request ** request)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIG_MPI_ISSEND);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIG_MPI_ISSEND);

    mpi_errno = MPIDI_am_isend(buf, count, datatype, rank, tag, comm,
                               context_offset, request, 0, MPIDI_CH4U_SSEND_REQ);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIG_MPI_ISSEND);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDIG_mpi_startall
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPIDIG_mpi_startall(int count, MPIR_Request * requests[])
{
    int mpi_errno = MPI_SUCCESS, i;
    int rank, tag, context_offset;
    MPI_Datatype datatype;
    uint64_t msg_tag;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIG_MPI_STARTALL);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIG_MPI_STARTALL);

    for (i = 0; i < count; i++) {
        MPIR_Request *const preq = requests[i];
        MPI_Request sreq_handle;

        msg_tag = MPIDI_CH4U_REQUEST(preq, tag);
        datatype = MPIDI_CH4U_REQUEST(preq, datatype);

        tag = MPIDI_CH4U_get_tag(msg_tag);
        rank = MPIDI_CH4U_REQUEST(preq, rank);
        context_offset = MPIDI_CH4U_get_context(msg_tag) - preq->comm->context_id;

        switch (MPIDI_CH4U_REQUEST(preq, p_type)) {

        case MPIDI_PTYPE_RECV:
#ifdef MPIDI_BUILD_CH4_SHM
            mpi_errno = MPIDI_NM_mpi_irecv(MPIDI_CH4U_REQUEST(preq, buffer),
                                           MPIDI_CH4U_REQUEST(preq, count),
                                           datatype, rank, tag,
                                           preq->comm, context_offset, NULL,
                                           &preq->u.persist.real_request);
#else
            mpi_errno = MPID_Irecv(MPIDI_CH4U_REQUEST(preq, buffer),
                                    MPIDI_CH4U_REQUEST(preq, count),
                                    datatype, rank, tag,
                                    preq->comm, context_offset, &preq->u.persist.real_request);
#endif
            break;

        case MPIDI_PTYPE_SEND:
#ifdef MPIDI_BUILD_CH4_SHM
            mpi_errno = MPIDI_NM_mpi_isend(MPIDI_CH4U_REQUEST(preq, buffer),
                                           MPIDI_CH4U_REQUEST(preq, count),
                                           datatype, rank, tag,
                                           preq->comm, context_offset, NULL,
                                           &preq->u.persist.real_request);
#else
            mpi_errno = MPID_Isend(MPIDI_CH4U_REQUEST(preq, buffer),
                                    MPIDI_CH4U_REQUEST(preq, count),
                                    datatype, rank, tag,
                                    preq->comm, context_offset, &preq->u.persist.real_request);
#endif
            break;

        case MPIDI_PTYPE_SSEND:
#ifdef MPIDI_BUILD_CH4_SHM
            mpi_errno = MPIDI_NM_mpi_issend(MPIDI_CH4U_REQUEST(preq, buffer),
                                            MPIDI_CH4U_REQUEST(preq, count),
                                            datatype, rank, tag,
                                            preq->comm, context_offset, NULL,
                                            &preq->u.persist.real_request);
#else
            mpi_errno = MPID_Issend(MPIDI_CH4U_REQUEST(preq, buffer),
                                     MPIDI_CH4U_REQUEST(preq, count),
                                     datatype, rank, tag,
                                     preq->comm, context_offset, &preq->u.persist.real_request);
#endif
            break;

        case MPIDI_PTYPE_BSEND:
            mpi_errno = MPIR_Ibsend_impl(MPIDI_CH4U_REQUEST(preq, buffer),
                                         MPIDI_CH4U_REQUEST(preq, count),
                                         datatype, rank, tag, preq->comm, &sreq_handle);
            if (mpi_errno == MPI_SUCCESS)
                MPIR_Request_get_ptr(sreq_handle, preq->u.persist.real_request);

            break;

        default:
            mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, __FUNCTION__,
                                             __LINE__, MPI_ERR_INTERN, "**ch3|badreqtype",
                                             "**ch3|badreqtype %d", MPIDI_CH4U_REQUEST(preq,
                                                                                       p_type));
        }

        if (mpi_errno == MPI_SUCCESS) {
            preq->status.MPI_ERROR = MPI_SUCCESS;

            if (MPIDI_CH4U_REQUEST(preq, p_type) == MPIDI_PTYPE_BSEND) {
                preq->cc_ptr = &preq->cc;
                MPID_Request_set_completed(preq);
            }
            else
                preq->cc_ptr = &preq->u.persist.real_request->cc;
        }
        else {
            preq->u.persist.real_request = NULL;
            preq->status.MPI_ERROR = mpi_errno;
            preq->cc_ptr = &preq->cc;
            MPID_Request_set_completed(preq);
        }
        dtype_release_if_not_builtin(datatype);
    }

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIG_MPI_STARTALL);
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
                                                  MPIR_Request ** request)
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
                                                   MPIR_Request ** request)
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
                                                   MPIR_Request ** request)
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
                                                   MPIR_Request ** request)
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
