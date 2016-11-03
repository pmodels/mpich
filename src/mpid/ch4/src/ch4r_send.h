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
#ifndef CH4R_SEND_H_INCLUDED
#define CH4R_SEND_H_INCLUDED

#include "ch4_impl.h"

#include <../mpi/pt2pt/bsendutil.h>

#undef FUNCNAME
#define FUNCNAME MPIDI_CH4I_do_send
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_CH4I_do_send(const void *buf,
                                     int count,
                                     MPI_Datatype datatype,
                                     int rank,
                                     int tag,
                                     MPIR_Comm * comm,
                                     int context_offset,
                                     MPIR_Request ** request, int type, MPIDI_call_context caller)
{
    int mpi_errno = MPI_SUCCESS, c;
    MPIR_Request *sreq = NULL;
    uint64_t match_bits;
    MPIDI_CH4U_hdr_t am_hdr;
    MPIDI_CH4U_ssend_req_msg_t ssend_req;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_CH4U_DO_SEND);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_CH4U_DO_SEND);

    sreq = MPIDI_CH4I_am_request_create(MPIR_REQUEST_KIND__SEND, caller);
    MPIR_Assert(sreq);

    *request = sreq;
    match_bits = MPIDI_CH4U_init_send_tag(comm->context_id + context_offset, comm->rank, tag);

    am_hdr.msg_tag = match_bits;
    am_hdr.src_rank = comm->rank;

    if (type == MPIDI_CH4U_SSEND_REQ) {

        ssend_req.hdr = am_hdr;
        ssend_req.sreq_ptr = (uint64_t) sreq;
        MPIR_cc_incr(sreq->cc_ptr, &c);

#ifdef MPIDI_CH4_EXCLUSIVE_SHM
        if (MPIDI_SHM == caller) {
            mpi_errno = MPIDI_SHM_am_isend(rank, comm, MPIDI_CH4U_SSEND_REQ,
                                           &ssend_req, sizeof(ssend_req),
                                           buf, count, datatype, sreq, NULL);
        }
        else {
#endif /* MPIDI_CH4_EXCLUSIVE_SHM */
            mpi_errno = MPIDI_NM_am_isend(rank, comm, MPIDI_CH4U_SSEND_REQ,
                                          &ssend_req, sizeof(ssend_req),
                                          buf, count, datatype, sreq, NULL);
#ifdef MPIDI_CH4_EXCLUSIVE_SHM
        }
#endif /* MPIDI_CH4_EXCLUSIVE_SHM */
        if (mpi_errno)
            MPIR_ERR_POP(mpi_errno);
    }
    else {
#ifdef MPIDI_CH4_EXCLUSIVE_SHM
        if (MPIDI_SHM == caller) {
            mpi_errno = MPIDI_SHM_am_isend(rank, comm, MPIDI_CH4U_SEND,
                                           &am_hdr, sizeof(am_hdr), buf, count, datatype, sreq,
                                           NULL);
        }
        else {
#endif /* MPIDI_CH4_EXCLUSIVE_SHM */
            mpi_errno = MPIDI_NM_am_isend(rank, comm, MPIDI_CH4U_SEND,
                                          &am_hdr, sizeof(am_hdr), buf, count, datatype, sreq,
                                          NULL);
#ifdef MPIDI_CH4_EXCLUSIVE_SHM
        }
#endif /* MPIDI_CH4_EXCLUSIVE_SHM */
        if (mpi_errno)
            MPIR_ERR_POP(mpi_errno);
    }

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_CH4U_DO_SEND);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH4I_send
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_CH4I_send(const void *buf, int count, MPI_Datatype datatype,
                                  int rank, int tag, MPIR_Comm * comm, int context_offset,
                                  MPIR_Request ** request, int noreq,
                                  int type, MPIDI_call_context caller)
{
    int mpi_errno;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_CH4U_NM_SEND);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_CH4U_NM_SEND);

    if (unlikely(rank == MPI_PROC_NULL)) {
        mpi_errno = MPI_SUCCESS;
        if (!noreq) {
            *request = MPIDI_CH4I_am_request_create(MPIR_REQUEST_KIND__SEND, caller);
            MPIDI_Request_complete((*request));
        }
        goto fn_exit;
    }

    mpi_errno =
        MPIDI_CH4I_do_send(buf, count, datatype, rank, tag, comm,
                           context_offset, request, type, caller);
  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_CH4U_NM_SEND);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH4I_psend
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_CH4I_psend(const void *buf,
                                   int count,
                                   MPI_Datatype datatype,
                                   int rank,
                                   int tag,
                                   MPIR_Comm * comm, int context_offset,
                                   MPIR_Request ** request, MPIDI_call_context caller)
{
    MPIR_Request *sreq;
    uint64_t match_bits;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_CH4U_NM_PSEND);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_CH4U_NM_PSEND);

    sreq = MPIDI_CH4I_am_request_create(MPIR_REQUEST_KIND__PREQUEST_SEND, caller);
    *request = sreq;

    MPIR_Comm_add_ref(comm);
    sreq->comm = comm;
    match_bits = MPIDI_CH4U_init_send_tag(comm->context_id + context_offset, rank, tag);

    MPIDI_CH4U_REQUEST(sreq, buffer) = (void *) buf;
    MPIDI_CH4U_REQUEST(sreq, count) = count;
    MPIDI_CH4U_REQUEST(sreq, datatype) = datatype;
    MPIDI_CH4U_REQUEST(sreq, tag) = match_bits;
    MPIDI_CH4U_REQUEST(sreq, src_rank) = rank;

    sreq->u.persist.real_request = NULL;
    MPIDI_Request_complete(sreq);

    dtype_add_ref_if_not_builtin(datatype);
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_NETMOD_OFI_NM_PSEND);
    return MPI_SUCCESS;
}


#undef FUNCNAME
#define FUNCNAME MPIDI_CH4U_mpi_send
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPIDI_CH4U_mpi_send(const void *buf,
                                                 int count,
                                                 MPI_Datatype datatype,
                                                 int rank,
                                                 int tag,
                                                 MPIR_Comm * comm, int context_offset,
                                                 MPIR_Request ** request, MPIDI_call_context caller)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_CH4U_SEND);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_CH4U_SEND);
    mpi_errno = MPIDI_CH4I_send(buf, count, datatype, rank, tag, comm,
                                context_offset, request, 1, 0ULL, caller);
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_CH4U_SEND);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH4U_mpi_isend
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPIDI_CH4U_mpi_isend(const void *buf,
                                                  int count,
                                                  MPI_Datatype datatype,
                                                  int rank,
                                                  int tag,
                                                  MPIR_Comm * comm, int context_offset,
                                                  MPIR_Request ** request,
                                                  MPIDI_call_context caller)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_CH4U_ISEND);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_CH4U_ISEND);
    mpi_errno = MPIDI_CH4I_send(buf, count, datatype, rank, tag, comm,
                                context_offset, request, 0, 0ULL, caller);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_CH4U_ISEND);
    return mpi_errno;
}


#undef FUNCNAME
#define FUNCNAME MPIDI_CH4U_mpi_rsend
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPIDI_CH4U_mpi_rsend(const void *buf,
                                                  int count,
                                                  MPI_Datatype datatype,
                                                  int rank,
                                                  int tag,
                                                  MPIR_Comm * comm, int context_offset,
                                                  MPIR_Request ** request,
                                                  MPIDI_call_context caller)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_CH4U_RSEND);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_CH4U_RSEND);
    mpi_errno = MPIDI_CH4I_send(buf, count, datatype, rank, tag, comm,
                                context_offset, request, 1, 0ULL, caller);
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_CH4U_RSEND);
    return mpi_errno;
}


#undef FUNCNAME
#define FUNCNAME MPIDI_CH4U_mpi_irsend
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPIDI_CH4U_mpi_irsend(const void *buf,
                                                   int count,
                                                   MPI_Datatype datatype,
                                                   int rank,
                                                   int tag,
                                                   MPIR_Comm * comm, int context_offset,
                                                   MPIR_Request ** request,
                                                   MPIDI_call_context caller)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_CH4U_IRSEND);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_CH4U_IRSEND);
    mpi_errno = MPIDI_CH4I_send(buf, count, datatype, rank, tag, comm,
                                context_offset, request, 0, 0ULL, caller);
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_CH4U_SEND);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH4U_mpi_ssend
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPIDI_CH4U_mpi_ssend(const void *buf,
                                                  int count,
                                                  MPI_Datatype datatype,
                                                  int rank,
                                                  int tag,
                                                  MPIR_Comm * comm, int context_offset,
                                                  MPIR_Request ** request,
                                                  MPIDI_call_context caller)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_CH4U_SSEND);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_CH4U_SSEND);
    mpi_errno = MPIDI_CH4I_send(buf, count, datatype, rank, tag, comm,
                                context_offset, request, 1, MPIDI_CH4U_SSEND_REQ, caller);
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_CH4U_SSEND);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH4U_mpi_issend
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPIDI_CH4U_mpi_issend(const void *buf,
                                                   int count,
                                                   MPI_Datatype datatype,
                                                   int rank,
                                                   int tag,
                                                   MPIR_Comm * comm, int context_offset,
                                                   MPIR_Request ** request,
                                                   MPIDI_call_context caller)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_CH4U_ISSEND);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_CH4U_ISSEND);
    mpi_errno = MPIDI_CH4I_send(buf, count, datatype, rank, tag, comm,
                                context_offset, request, 0, MPIDI_CH4U_SSEND_REQ, caller);
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_CH4U_ISSEND);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH4U_mpi_startall
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPIDI_CH4U_mpi_startall(int count, MPIR_Request * requests[])
{
    int mpi_errno = MPI_SUCCESS, i;
    int rank, tag, context_offset;
    MPI_Datatype datatype;
    uint64_t msg_tag;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_CH4U_STARTALL);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_CH4U_STARTALL);

    for (i = 0; i < count; i++) {
        MPIR_Request *const preq = requests[i];
        MPI_Request sreq_handle;

        msg_tag = MPIDI_CH4U_REQUEST(preq, tag);
        datatype = MPIDI_CH4U_REQUEST(preq, datatype);

        tag = MPIDI_CH4U_get_tag(msg_tag);
        rank = MPIDI_CH4U_REQUEST(preq, src_rank);
        context_offset = MPIDI_CH4U_get_context(msg_tag) - preq->comm->context_id;

        switch (MPIDI_CH4U_REQUEST(preq, p_type)) {

        case MPIDI_PTYPE_RECV:
#ifdef MPIDI_BUILD_CH4_SHM
            if (MPIDI_SHM == MPIDI_CH4U_REQUEST(preq, caller)) {
                mpi_errno = MPIDI_SHM_mpi_irecv(MPIDI_CH4U_REQUEST(preq, buffer),
                                                MPIDI_CH4U_REQUEST(preq, count),
                                                datatype, rank, tag,
                                                preq->comm, context_offset,
                                                &preq->u.persist.real_request);
            }
            else {
                mpi_errno = MPIDI_NM_mpi_irecv(MPIDI_CH4U_REQUEST(preq, buffer),
                                               MPIDI_CH4U_REQUEST(preq, count),
                                               datatype, rank, tag,
                                               preq->comm, context_offset,
                                               &preq->u.persist.real_request);
            }
#else
            mpi_errno = MPIDI_Irecv(MPIDI_CH4U_REQUEST(preq, buffer),
                                    MPIDI_CH4U_REQUEST(preq, count),
                                    datatype, rank, tag,
                                    preq->comm, context_offset, &preq->u.persist.real_request);
#endif
            break;

        case MPIDI_PTYPE_SEND:
#ifdef MPIDI_BUILD_CH4_SHM
            if (MPIDI_SHM == MPIDI_CH4U_REQUEST(preq, caller)) {
                mpi_errno = MPIDI_SHM_mpi_isend(MPIDI_CH4U_REQUEST(preq, buffer),
                                                MPIDI_CH4U_REQUEST(preq, count),
                                                datatype, rank, tag,
                                                preq->comm, context_offset,
                                                &preq->u.persist.real_request);
            }
            else {
                mpi_errno = MPIDI_NM_mpi_isend(MPIDI_CH4U_REQUEST(preq, buffer),
                                               MPIDI_CH4U_REQUEST(preq, count),
                                               datatype, rank, tag,
                                               preq->comm, context_offset,
                                               &preq->u.persist.real_request);
            }
#else
            mpi_errno = MPIDI_Isend(MPIDI_CH4U_REQUEST(preq, buffer),
                                    MPIDI_CH4U_REQUEST(preq, count),
                                    datatype, rank, tag,
                                    preq->comm, context_offset, &preq->u.persist.real_request);
#endif
            break;

        case MPIDI_PTYPE_SSEND:
#ifdef MPIDI_BUILD_CH4_SHM
            if (MPIDI_SHM == MPIDI_CH4U_REQUEST(preq, caller)) {
                mpi_errno = MPIDI_SHM_mpi_issend(MPIDI_CH4U_REQUEST(preq, buffer),
                                                 MPIDI_CH4U_REQUEST(preq, count),
                                                 datatype, rank, tag,
                                                 preq->comm, context_offset,
                                                 &preq->u.persist.real_request);
            }
            else {
                mpi_errno = MPIDI_NM_mpi_issend(MPIDI_CH4U_REQUEST(preq, buffer),
                                                MPIDI_CH4U_REQUEST(preq, count),
                                                datatype, rank, tag,
                                                preq->comm, context_offset,
                                                &preq->u.persist.real_request);
            }
#else
            mpi_errno = MPIDI_Issend(MPIDI_CH4U_REQUEST(preq, buffer),
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
                MPIDI_Request_set_completed(preq);
            }
            else
                preq->cc_ptr = &preq->u.persist.real_request->cc;
        }
        else {
            preq->u.persist.real_request = NULL;
            preq->status.MPI_ERROR = mpi_errno;
            preq->cc_ptr = &preq->cc;
            MPIDI_Request_set_completed(preq);
        }
        dtype_release_if_not_builtin(datatype);
    }

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_CH4U_STARTALL);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH4U_mpi_send_init
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPIDI_CH4U_mpi_send_init(const void *buf,
                                                      int count,
                                                      MPI_Datatype datatype,
                                                      int rank,
                                                      int tag,
                                                      MPIR_Comm * comm,
                                                      int context_offset,
                                                      MPIR_Request ** request,
                                                      MPIDI_call_context caller)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_CH4U_SEND_INIT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_CH4U_SEND_INIT);
    mpi_errno = MPIDI_CH4I_psend(buf, count, datatype, rank, tag, comm, context_offset,
                                 request, caller);
    MPIDI_CH4U_REQUEST((*request), p_type) = MPIDI_PTYPE_SEND;
#ifdef MPIDI_CH4_EXCLUSIVE_SHM
    MPIDI_CH4U_REQUEST((*request), caller) = caller;
#endif /* MPIDI_CH4_EXCLUSIVE_SHM */
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_CH4U_SEND_INIT);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH4U_mpi_ssend_init
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPIDI_CH4U_mpi_ssend_init(const void *buf,
                                                       int count,
                                                       MPI_Datatype datatype,
                                                       int rank,
                                                       int tag,
                                                       MPIR_Comm * comm,
                                                       int context_offset,
                                                       MPIR_Request ** request,
                                                       MPIDI_call_context caller)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_CH4U_SSEND_INIT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_CH4U_SSEND_INIT);
    mpi_errno = MPIDI_CH4I_psend(buf, count, datatype, rank, tag, comm, context_offset,
                                 request, caller);
    MPIDI_CH4U_REQUEST((*request), p_type) = MPIDI_PTYPE_SSEND;
#ifdef MPIDI_CH4_EXCLUSIVE_SHM
    MPIDI_CH4U_REQUEST((*request), caller) = caller;
#endif /* MPIDI_CH4_EXCLUSIVE_SHM */
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_CH4U_SSEND_INIT);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH4U_mpi_bsend_init
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPIDI_CH4U_mpi_bsend_init(const void *buf,
                                                       int count,
                                                       MPI_Datatype datatype,
                                                       int rank,
                                                       int tag,
                                                       MPIR_Comm * comm,
                                                       int context_offset,
                                                       MPIR_Request ** request,
                                                       MPIDI_call_context caller)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_CH4U_BSEND_INIT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_CH4U_BSEND_INIT);
    mpi_errno = MPIDI_CH4I_psend(buf, count, datatype, rank, tag, comm, context_offset,
                                 request, caller);
    MPIDI_CH4U_REQUEST((*request), p_type) = MPIDI_PTYPE_BSEND;
#ifdef MPIDI_CH4_EXCLUSIVE_SHM
    MPIDI_CH4U_REQUEST((*request), caller) = caller;
#endif /* MPIDI_CH4_EXCLUSIVE_SHM */
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_CH4U_BSEND_INIT);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH4U_mpi_rsend_init
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPIDI_CH4U_mpi_rsend_init(const void *buf,
                                                       int count,
                                                       MPI_Datatype datatype,
                                                       int rank,
                                                       int tag,
                                                       MPIR_Comm * comm,
                                                       int context_offset,
                                                       MPIR_Request ** request,
                                                       MPIDI_call_context caller)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_CH4U_RSEND_INIT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_CH4U_RSEND_INIT);
    mpi_errno = MPIDI_CH4I_psend(buf, count, datatype, rank, tag, comm, context_offset,
                                 request, caller);
    MPIDI_CH4U_REQUEST((*request), p_type) = MPIDI_PTYPE_SEND;
#ifdef MPIDI_CH4_EXCLUSIVE_SHM
    MPIDI_CH4U_REQUEST((*request), caller) = caller;
#endif /* MPIDI_CH4_EXCLUSIVE_SHM */
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_CH4U_RSEND_INIT);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH4U_mpi_cancel_send
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPIDI_CH4U_mpi_cancel_send(MPIR_Request * sreq,
                                                        MPIDI_call_context caller
                                                        __attribute__ ((__unused__)))
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_CH4U_CANCEL_SEND);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_CH4U_CANCEL_SEND);
    /* cannot cancel send */
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_CH4U_CANCEL_SEND);
    return mpi_errno;
}

#endif /* CH4R_SEND_H_INCLUDED */
