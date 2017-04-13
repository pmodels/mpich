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
#ifndef CH4_SEND_H_INCLUDED
#define CH4_SEND_H_INCLUDED

#include "ch4_impl.h"
#include "ch4r_proc.h"
#undef FUNCNAME
#define FUNCNAME MPID_Send
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPID_Send(const void *buf,
                                        int count,
                                        MPI_Datatype datatype,
                                        int rank,
                                        int tag,
                                        MPIR_Comm * comm, int context_offset,
                                        MPIR_Request ** request)
{
    int mpi_errno;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_SEND);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_SEND);

    if (unlikely(rank == MPI_PROC_NULL)) {
        MPIR_Request *rreq = MPIR_Request_create(MPIR_REQUEST_KIND__SEND);
        MPIR_Request_add_ref(rreq);
        *request = rreq;
        MPIDI_CH4U_request_complete(rreq);
        mpi_errno = MPI_SUCCESS;
        goto fn_exit;
    }

#ifndef MPIDI_CH4_EXCLUSIVE_SHM
    mpi_errno = MPIDI_NM_mpi_send(buf, count, datatype, rank, tag, comm, context_offset, NULL, request);
#else
    int r;
    if ((r = MPIDI_CH4_rank_is_local(rank, comm)))
        mpi_errno =
            MPIDI_SHM_mpi_send(buf, count, datatype, rank, tag, comm, context_offset, request);
    else
        mpi_errno =
            MPIDI_NM_mpi_send(buf, count, datatype, rank, tag, comm, context_offset, NULL, request);
    if (mpi_errno == MPI_SUCCESS && *request)
        MPIDI_CH4I_REQUEST(*request, is_local) = r;
#endif
    if (mpi_errno != MPI_SUCCESS) {
        MPIR_ERR_POP(mpi_errno);
    }
  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_SEND);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPID_Isend
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPID_Isend(const void *buf,
                                         int count,
                                         MPI_Datatype datatype,
                                         int rank,
                                         int tag,
                                         MPIR_Comm * comm, int context_offset,
                                         MPIR_Request ** request)
{
    int mpi_errno;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_ISEND);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_ISEND);

    if (unlikely(rank == MPI_PROC_NULL)) {
        MPIR_Request *rreq = MPIR_Request_create(MPIR_REQUEST_KIND__SEND);
        MPIR_Request_add_ref(rreq);
        *request = rreq;
        MPIDI_CH4U_request_complete(rreq);
        mpi_errno = MPI_SUCCESS;
        goto fn_exit;
    }

#ifndef MPIDI_CH4_EXCLUSIVE_SHM
    mpi_errno = MPIDI_NM_mpi_isend(buf, count, datatype, rank, tag, comm, context_offset, NULL, request);
#else
    int r;
    if ((r = MPIDI_CH4_rank_is_local(rank, comm)))
        mpi_errno =
            MPIDI_SHM_mpi_isend(buf, count, datatype, rank, tag, comm, context_offset, request);
    else
        mpi_errno =
            MPIDI_NM_mpi_isend(buf, count, datatype, rank, tag, comm, context_offset, NULL, request);
    if (mpi_errno == MPI_SUCCESS)
        MPIDI_CH4I_REQUEST(*request, is_local) = r;
#endif
    if (mpi_errno != MPI_SUCCESS) {
        MPIR_ERR_POP(mpi_errno);
    }
  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_ISEND);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}


#undef FUNCNAME
#define FUNCNAME MPID_Rsend
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPID_Rsend(const void *buf,
                                         int count,
                                         MPI_Datatype datatype,
                                         int rank,
                                         int tag,
                                         MPIR_Comm * comm, int context_offset,
                                         MPIR_Request ** request)
{
    int mpi_errno;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_RSEND);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_RSEND);

    if (unlikely(rank == MPI_PROC_NULL)) {
        MPIR_Request *rreq = MPIR_Request_create(MPIR_REQUEST_KIND__SEND);
        MPIR_Request_add_ref(rreq);
        *request = rreq;
        MPIDI_CH4U_request_complete(rreq);
        mpi_errno = MPI_SUCCESS;
        goto fn_exit;
    }

#ifndef MPIDI_CH4_EXCLUSIVE_SHM
    mpi_errno = MPIDI_NM_mpi_send(buf, count, datatype, rank, tag, comm, context_offset, NULL, request);
#else
    int r;
    if ((r = MPIDI_CH4_rank_is_local(rank, comm)))
        mpi_errno =
            MPIDI_SHM_mpi_send(buf, count, datatype, rank, tag, comm, context_offset, request);
    else
        mpi_errno =
            MPIDI_NM_mpi_send(buf, count, datatype, rank, tag, comm, context_offset, NULL, request);
    if (mpi_errno == MPI_SUCCESS && *request)
        MPIDI_CH4I_REQUEST(*request, is_local) = r;
#endif
    if (mpi_errno != MPI_SUCCESS) {
        MPIR_ERR_POP(mpi_errno);
    }
  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_RSEND);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}


#undef FUNCNAME
#define FUNCNAME MPID_Irsend
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPID_Irsend(const void *buf,
                                          int count,
                                          MPI_Datatype datatype,
                                          int rank,
                                          int tag,
                                          MPIR_Comm * comm, int context_offset,
                                          MPIR_Request ** request)
{
    int mpi_errno;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_IRSEND);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_IRSEND);

    if (unlikely(rank == MPI_PROC_NULL)) {
        MPIR_Request *rreq = MPIR_Request_create(MPIR_REQUEST_KIND__SEND);
        MPIR_Request_add_ref(rreq);
        *request = rreq;
        MPIDI_CH4U_request_complete(rreq);
        mpi_errno = MPI_SUCCESS;
        goto fn_exit;
    }

#ifndef MPIDI_CH4_EXCLUSIVE_SHM
    mpi_errno = MPIDI_NM_mpi_isend(buf, count, datatype, rank, tag, comm, context_offset, NULL, request);
#else
    int r;
    if ((r = MPIDI_CH4_rank_is_local(rank, comm)))
        mpi_errno =
            MPIDI_SHM_mpi_isend(buf, count, datatype, rank, tag, comm, context_offset, request);
    else
        mpi_errno =
            MPIDI_NM_mpi_isend(buf, count, datatype, rank, tag, comm, context_offset, NULL, request);
    if (mpi_errno == MPI_SUCCESS)
        MPIDI_CH4I_REQUEST(*request, is_local) = r;
#endif
    if (mpi_errno != MPI_SUCCESS) {
        MPIR_ERR_POP(mpi_errno);
    }
  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_IRSEND);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPID_Ssend
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPID_Ssend(const void *buf,
                                         int count,
                                         MPI_Datatype datatype,
                                         int rank,
                                         int tag,
                                         MPIR_Comm * comm, int context_offset,
                                         MPIR_Request ** request)
{
    int mpi_errno;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_SSEND);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_SSEND);

    if (unlikely(rank == MPI_PROC_NULL)) {
        MPIR_Request *rreq = MPIR_Request_create(MPIR_REQUEST_KIND__SEND);
        MPIR_Request_add_ref(rreq);
        *request = rreq;
        MPIDI_CH4U_request_complete(rreq);
        mpi_errno = MPI_SUCCESS;
        goto fn_exit;
    }

#ifndef MPIDI_CH4_EXCLUSIVE_SHM
    mpi_errno = MPIDI_NM_mpi_ssend(buf, count, datatype, rank, tag, comm, context_offset, NULL, request);
#else
    int r;
    if ((r = MPIDI_CH4_rank_is_local(rank, comm)))
        mpi_errno =
            MPIDI_SHM_mpi_ssend(buf, count, datatype, rank, tag, comm, context_offset, request);
    else
        mpi_errno =
            MPIDI_NM_mpi_ssend(buf, count, datatype, rank, tag, comm, context_offset, NULL, request);
    if (mpi_errno == MPI_SUCCESS && *request)
        MPIDI_CH4I_REQUEST(*request, is_local) = r;
#endif
    if (mpi_errno != MPI_SUCCESS) {
        MPIR_ERR_POP(mpi_errno);
    }
  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_SSEND);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPID_Issend
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPID_Issend(const void *buf,
                                          int count,
                                          MPI_Datatype datatype,
                                          int rank,
                                          int tag,
                                          MPIR_Comm * comm, int context_offset,
                                          MPIR_Request ** request)
{
    int mpi_errno;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_ISSEND);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_ISSEND);

    if (unlikely(rank == MPI_PROC_NULL)) {
        MPIR_Request *rreq = MPIR_Request_create(MPIR_REQUEST_KIND__SEND);
        MPIR_Request_add_ref(rreq);
        *request = rreq;
        MPIDI_CH4U_request_complete(rreq);
        mpi_errno = MPI_SUCCESS;
        goto fn_exit;
    }

#ifndef MPIDI_CH4_EXCLUSIVE_SHM
    mpi_errno = MPIDI_NM_mpi_issend(buf, count, datatype, rank, tag, comm, context_offset, NULL, request);
#else
    int r;
    if ((r = MPIDI_CH4_rank_is_local(rank, comm)))
        mpi_errno =
            MPIDI_SHM_mpi_issend(buf, count, datatype, rank, tag, comm, context_offset, request);
    else
        mpi_errno =
            MPIDI_NM_mpi_issend(buf, count, datatype, rank, tag, comm, context_offset, NULL, request);
    if (mpi_errno == MPI_SUCCESS)
        MPIDI_CH4I_REQUEST(*request, is_local) = r;
#endif
    if (mpi_errno != MPI_SUCCESS) {
        MPIR_ERR_POP(mpi_errno);
    }
  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_ISSEND);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPID_Startall
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPID_Startall(int count, MPIR_Request * requests[])
{
    int mpi_errno;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_STARTALL);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_STARTALL);
#ifndef MPIDI_CH4_EXCLUSIVE_SHM
    mpi_errno = MPIDI_NM_mpi_startall(count, requests);
#else
    int i;
    for (i = 0; i < count; i++) {
        /* This is sub-optimal, can we do better? */
        if (MPIDI_CH4I_REQUEST_ANYSOURCE_PARTNER(requests[i])) {
            mpi_errno = MPIDI_SHM_mpi_startall(1, &requests[i]);
            if (mpi_errno == MPI_SUCCESS) {
                mpi_errno =
                    MPIDI_NM_mpi_startall(1, &MPIDI_CH4I_REQUEST_ANYSOURCE_PARTNER(requests[i]));
                MPIDI_CH4I_REQUEST_ANYSOURCE_PARTNER(requests[i]->u.persist.real_request) =
                    MPIDI_CH4I_REQUEST_ANYSOURCE_PARTNER(requests[i])->u.persist.real_request;
                MPIDI_CH4I_REQUEST_ANYSOURCE_PARTNER(MPIDI_CH4I_REQUEST_ANYSOURCE_PARTNER
                                                     (requests[i])->u.persist.real_request) =
                    requests[i]->u.persist.real_request;
            }
        }
        else if (MPIDI_CH4I_REQUEST(requests[i], is_local))
            mpi_errno = MPIDI_SHM_mpi_startall(1, &requests[i]);
        else
            mpi_errno = MPIDI_NM_mpi_startall(1, &requests[i]);
    }
#endif
    if (mpi_errno != MPI_SUCCESS) {
        MPIR_ERR_POP(mpi_errno);
    }
  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_STARTALL);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPID_Send_init
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPID_Send_init(const void *buf,
                                             int count,
                                             MPI_Datatype datatype,
                                             int rank,
                                             int tag,
                                             MPIR_Comm * comm, int context_offset,
                                             MPIR_Request ** request)
{
    int mpi_errno;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_SEND_INIT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_SEND_INIT);
#ifndef MPIDI_CH4_EXCLUSIVE_SHM
    mpi_errno =
        MPIDI_NM_mpi_send_init(buf, count, datatype, rank, tag, comm, context_offset, NULL, request);
#else
    int r;
    if ((r = MPIDI_CH4_rank_is_local(rank, comm)))
        mpi_errno = MPIDI_SHM_mpi_send_init(buf, count, datatype, rank, tag,
                                            comm, context_offset, request);
    else
        mpi_errno = MPIDI_NM_mpi_send_init(buf, count, datatype, rank, tag,
                                           comm, context_offset, NULL, request);
    if (mpi_errno == MPI_SUCCESS)
        MPIDI_CH4I_REQUEST(*request, is_local) = r;
    MPIDI_CH4I_REQUEST_ANYSOURCE_PARTNER(*request) = NULL;
#endif
    if (mpi_errno != MPI_SUCCESS) {
        MPIR_ERR_POP(mpi_errno);
    }
  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_SEND_INIT);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPID_Ssend_init
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPID_Ssend_init(const void *buf,
                                              int count,
                                              MPI_Datatype datatype,
                                              int rank,
                                              int tag,
                                              MPIR_Comm * comm, int context_offset,
                                              MPIR_Request ** request)
{
    int mpi_errno;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_SSEND_INIT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_SSEND_INIT);
#ifndef MPIDI_CH4_EXCLUSIVE_SHM
    mpi_errno =
        MPIDI_NM_mpi_ssend_init(buf, count, datatype, rank, tag, comm, context_offset, NULL, request);
#else
    int r;
    if ((r = MPIDI_CH4_rank_is_local(rank, comm)))
        mpi_errno = MPIDI_SHM_mpi_ssend_init(buf, count, datatype, rank, tag,
                                             comm, context_offset, request);
    else
        mpi_errno = MPIDI_NM_mpi_ssend_init(buf, count, datatype, rank, tag,
                                            comm, context_offset, NULL, request);
    if (mpi_errno == MPI_SUCCESS && *request) {
        MPIDI_CH4I_REQUEST(*request, is_local) = r;
        MPIDI_CH4I_REQUEST_ANYSOURCE_PARTNER(*request) = NULL;
    }
#endif
    if (mpi_errno != MPI_SUCCESS) {
        MPIR_ERR_POP(mpi_errno);
    }
  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_SSEND_INIT);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPID_Bsend_init
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPID_Bsend_init(const void *buf,
                                              int count,
                                              MPI_Datatype datatype,
                                              int rank,
                                              int tag,
                                              MPIR_Comm * comm, int context_offset,
                                              MPIR_Request ** request)
{
    int mpi_errno;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_BSEND_INIT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_BSEND_INIT);
#ifndef MPIDI_CH4_EXCLUSIVE_SHM
    mpi_errno =
        MPIDI_NM_mpi_bsend_init(buf, count, datatype, rank, tag, comm, context_offset, NULL, request);
#else
    int r;
    if ((r = MPIDI_CH4_rank_is_local(rank, comm)))
        mpi_errno = MPIDI_SHM_mpi_bsend_init(buf, count, datatype, rank, tag,
                                             comm, context_offset, request);
    else
        mpi_errno = MPIDI_NM_mpi_bsend_init(buf, count, datatype, rank, tag,
                                            comm, context_offset, NULL, request);
    if (mpi_errno == MPI_SUCCESS && *request) {
        MPIDI_CH4I_REQUEST(*request, is_local) = r;
        MPIDI_CH4I_REQUEST_ANYSOURCE_PARTNER(*request) = NULL;
    }
#endif
    if (mpi_errno != MPI_SUCCESS) {
        MPIR_ERR_POP(mpi_errno);
    }
  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_BSEND_INIT);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPID_Rsend_init
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPID_Rsend_init(const void *buf,
                                              int count,
                                              MPI_Datatype datatype,
                                              int rank,
                                              int tag,
                                              MPIR_Comm * comm, int context_offset,
                                              MPIR_Request ** request)
{
    int mpi_errno;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_RSEND_INIT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_RSEND_INIT);
#ifndef MPIDI_CH4_EXCLUSIVE_SHM
    mpi_errno =
        MPIDI_NM_mpi_rsend_init(buf, count, datatype, rank, tag, comm, context_offset, NULL, request);
#else
    int r;
    if ((r = MPIDI_CH4_rank_is_local(rank, comm)))
        mpi_errno = MPIDI_SHM_mpi_rsend_init(buf, count, datatype, rank, tag,
                                             comm, context_offset, request);
    else
        mpi_errno = MPIDI_NM_mpi_rsend_init(buf, count, datatype, rank, tag,
                                            comm, context_offset, NULL, request);
    if (mpi_errno == MPI_SUCCESS && *request) {
        MPIDI_CH4I_REQUEST(*request, is_local) = r;
        MPIDI_CH4I_REQUEST_ANYSOURCE_PARTNER(*request) = NULL;
    }
#endif
    if (mpi_errno != MPI_SUCCESS) {
        MPIR_ERR_POP(mpi_errno);
    }
  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_RSEND_INIT);
    return mpi_errno;
  fn_fail:
    goto fn_exit;

}

#undef FUNCNAME
#define FUNCNAME MPID_Cancel_send
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPID_Cancel_send(MPIR_Request * sreq)
{
    int mpi_errno;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_CANCEL_SEND);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_CANCEL_SEND);
#ifndef MPIDI_CH4_EXCLUSIVE_SHM
    mpi_errno = MPIDI_NM_mpi_cancel_send(sreq);
#else
    if (MPIDI_CH4I_REQUEST(sreq, is_local))
        mpi_errno = MPIDI_SHM_mpi_cancel_send(sreq);
    else
        mpi_errno = MPIDI_NM_mpi_cancel_send(sreq);
#endif
    if (mpi_errno != MPI_SUCCESS) {
        MPIR_ERR_POP(mpi_errno);
    }
  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_CANCEL_SEND);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#endif /* CH4_SEND_H_INCLUDED */
