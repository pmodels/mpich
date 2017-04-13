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
#ifndef CH4_RECV_H_INCLUDED
#define CH4_RECV_H_INCLUDED

#include "ch4_impl.h"

#undef FUNCNAME
#define FUNCNAME MPID_Recv
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPID_Recv(void *buf,
                                        int count,
                                        MPI_Datatype datatype,
                                        int rank,
                                        int tag,
                                        MPIR_Comm * comm,
                                        int context_offset, MPI_Status * status,
                                        MPIR_Request ** request)
{
    int mpi_errno;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_RECV);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_RECV);

    if (unlikely(rank == MPI_PROC_NULL)) {
        MPIR_Request *rreq = MPIR_Request_create(MPIR_REQUEST_KIND__RECV);
        *request = rreq;
        MPIR_Request_add_ref(rreq);
        rreq->status.MPI_SOURCE = rank;
        rreq->status.MPI_TAG = tag;
        MPIDI_CH4U_request_complete(rreq);
        mpi_errno = MPI_SUCCESS;
        goto fn_exit;
    }

#ifndef MPIDI_CH4_EXCLUSIVE_SHM
    mpi_errno =
        MPIDI_NM_mpi_recv(buf, count, datatype, rank, tag, comm, context_offset, NULL, status, request);
#else
    if (unlikely(rank == MPI_ANY_SOURCE)) {
        mpi_errno =
            MPIDI_SHM_mpi_irecv(buf, count, datatype, rank, tag, comm, context_offset, request);

        if (mpi_errno != MPI_SUCCESS) {
            MPIR_ERR_POP(mpi_errno);
        }

        mpi_errno = MPIDI_NM_mpi_irecv(buf, count, datatype, rank, tag, comm, context_offset, NULL,
                                       &(MPIDI_CH4I_REQUEST_ANYSOURCE_PARTNER(*request)));

        if (mpi_errno != MPI_SUCCESS) {
            MPIR_ERR_POP(mpi_errno);
        }
        /* cancel the shm request if netmod/am handles the request from unexpected queue. */
        else if (*request) {
            if (MPIR_Request_is_complete(MPIDI_CH4I_REQUEST_ANYSOURCE_PARTNER(*request))) {
                mpi_errno = MPIDI_SHM_mpi_cancel_recv(*request);
                if (MPIR_STATUS_GET_CANCEL_BIT((*request)->status)) {
                    (*request)->status = MPIDI_CH4I_REQUEST_ANYSOURCE_PARTNER(*request)->status;
                }
                MPIR_Request_free(MPIDI_CH4I_REQUEST_ANYSOURCE_PARTNER(*request));
                goto fn_exit;
            }
            MPIDI_CH4I_REQUEST(*request, is_local) = 1;
            MPIDI_CH4I_REQUEST(MPIDI_CH4I_REQUEST_ANYSOURCE_PARTNER(*request), is_local) = 0;
        }

        MPIDI_CH4I_REQUEST_ANYSOURCE_PARTNER(MPIDI_CH4I_REQUEST_ANYSOURCE_PARTNER(*request)) =
            *request;
    }
    else {
        int r;
        if ((r = MPIDI_CH4_rank_is_local(rank, comm)))
            mpi_errno =
                MPIDI_SHM_mpi_recv(buf, count, datatype, rank, tag, comm, context_offset, status,
                                   request);
        else
            mpi_errno =
                MPIDI_NM_mpi_recv(buf, count, datatype, rank, tag, comm, context_offset, NULL, status,
                                  request);
        if (mpi_errno == MPI_SUCCESS && *request) {
            MPIDI_CH4I_REQUEST(*request, is_local) = r;
            MPIDI_CH4I_REQUEST_ANYSOURCE_PARTNER(*request) = NULL;
        }
    }
#endif

    if (mpi_errno != MPI_SUCCESS) {
        MPIR_ERR_POP(mpi_errno);
    }
  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_RECV);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPID_Recv_init
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPID_Recv_init(void *buf,
                                             int count,
                                             MPI_Datatype datatype,
                                             int rank,
                                             int tag,
                                             MPIR_Comm * comm, int context_offset,
                                             MPIR_Request ** request)
{
    int mpi_errno;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_RECV_INIT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_RECV_INIT);

#ifndef MPIDI_CH4_EXCLUSIVE_SHM
    mpi_errno =
        MPIDI_NM_mpi_recv_init(buf, count, datatype, rank, tag, comm, context_offset, NULL, request);
#else
    if (unlikely(rank == MPI_ANY_SOURCE)) {
        mpi_errno =
            MPIDI_SHM_mpi_recv_init(buf, count, datatype, rank, tag, comm, context_offset, request);

        if (mpi_errno != MPI_SUCCESS) {
            MPIR_ERR_POP(mpi_errno);
        }

        mpi_errno = MPIDI_NM_mpi_recv_init(buf, count, datatype, rank, tag, comm, context_offset, NULL,
                                           &(MPIDI_CH4I_REQUEST_ANYSOURCE_PARTNER(*request)));

        if (mpi_errno != MPI_SUCCESS) {
            MPIR_ERR_POP(mpi_errno);
        }

        MPIDI_CH4I_REQUEST(*request, is_local) = 1;
        MPIDI_CH4I_REQUEST(MPIDI_CH4I_REQUEST_ANYSOURCE_PARTNER(*request), is_local) = 0;

        MPIDI_CH4I_REQUEST_ANYSOURCE_PARTNER(MPIDI_CH4I_REQUEST_ANYSOURCE_PARTNER(*request)) =
            *request;
    }
    else {
        int r;
        if ((r = MPIDI_CH4_rank_is_local(rank, comm)))
            mpi_errno = MPIDI_SHM_mpi_recv_init(buf, count, datatype, rank, tag,
                                                comm, context_offset, request);
        else
            mpi_errno = MPIDI_NM_mpi_recv_init(buf, count, datatype, rank, tag,
                                               comm, context_offset, NULL, request);
        if (mpi_errno == MPI_SUCCESS) {
            MPIDI_CH4I_REQUEST(*request, is_local) = r;
            MPIDI_CH4I_REQUEST_ANYSOURCE_PARTNER(*request) = NULL;
        }
    }
  fn_exit:
#endif

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_RECV_INIT);
    return mpi_errno;

#ifdef MPIDI_CH4_EXCLUSIVE_SHM
  fn_fail:
    goto fn_exit;
#endif /* MPIDI_CH4_EXCLUSIVE_SHM */
}


#undef FUNCNAME
#define FUNCNAME MPID_Mrecv
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPID_Mrecv(void *buf,
                                         int count,
                                         MPI_Datatype datatype, MPIR_Request * message,
                                         MPI_Status * status)
{
    int mpi_errno;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_MRECV);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_MRECV);

    MPI_Request req_handle;
    int active_flag;
    MPIR_Request *rreq = NULL;

    if (message == NULL) {
        /* treat as though MPI_MESSAGE_NO_PROC was passed */
        MPIR_Status_set_procnull(status);
        mpi_errno = MPI_SUCCESS;
        goto fn_exit;
    }

#ifndef MPIDI_CH4_EXCLUSIVE_SHM
    mpi_errno = MPIDI_NM_mpi_imrecv(buf, count, datatype, message, &rreq);
#else
    if (MPIDI_CH4I_REQUEST(message, is_local))
        mpi_errno = MPIDI_SHM_mpi_imrecv(buf, count, datatype, message, &rreq);
    else
        mpi_errno = MPIDI_NM_mpi_imrecv(buf, count, datatype, message, &rreq);
#endif
    if (mpi_errno != MPI_SUCCESS) {
        MPIR_ERR_POP(mpi_errno);
    }
    while (!MPIR_Request_is_complete(rreq)) {
        MPID_Progress_test();
    }

    /* This should probably be moved to MPICH (above device) level */
    /* Someone neglected to put the blocking at the MPICH level    */
    mpi_errno = MPIR_Request_complete(&req_handle, rreq, status, &active_flag);
  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_MRECV);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPID_Imrecv
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPID_Imrecv(void *buf,
                                          int count,
                                          MPI_Datatype datatype,
                                          MPIR_Request * message, MPIR_Request ** rreqp)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_IMRECV);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_IMRECV);

    if (message == NULL) {
        MPIDI_Request_create_null_rreq(*rreqp, mpi_errno, goto fn_fail);
        goto fn_exit;
    }

#ifndef MPIDI_CH4_EXCLUSIVE_SHM
    mpi_errno = MPIDI_NM_mpi_imrecv(buf, count, datatype, message, rreqp);
#else
    if (MPIDI_CH4I_REQUEST(message, is_local))
        mpi_errno = MPIDI_SHM_mpi_imrecv(buf, count, datatype, message, rreqp);
    else
        mpi_errno = MPIDI_NM_mpi_imrecv(buf, count, datatype, message, rreqp);
#endif
    if (mpi_errno != MPI_SUCCESS) {
        MPIR_ERR_POP(mpi_errno);
    }
  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_IMRECV);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPID_Irecv
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPID_Irecv(void *buf,
                                         int count,
                                         MPI_Datatype datatype,
                                         int rank,
                                         int tag,
                                         MPIR_Comm * comm, int context_offset,
                                         MPIR_Request ** request)
{
    int mpi_errno;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_IRECV);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_IRECV);

    if (unlikely(rank == MPI_PROC_NULL)) {
        MPIR_Request *rreq = MPIR_Request_create(MPIR_REQUEST_KIND__RECV);
        *request = rreq;
        MPIR_Request_add_ref(rreq);
        rreq->status.MPI_SOURCE = rank;
        rreq->status.MPI_TAG = tag;
        MPIDI_CH4U_request_complete(rreq);
        mpi_errno = MPI_SUCCESS;
        goto fn_exit;
    }

#ifndef MPIDI_CH4_EXCLUSIVE_SHM
    mpi_errno = MPIDI_NM_mpi_irecv(buf, count, datatype, rank, tag, comm, context_offset, NULL, request);
#else
    if (unlikely(rank == MPI_ANY_SOURCE)) {
        mpi_errno =
            MPIDI_SHM_mpi_irecv(buf, count, datatype, rank, tag, comm, context_offset, request);

        if (mpi_errno != MPI_SUCCESS) {
            MPIR_ERR_POP(mpi_errno);
        }

        mpi_errno = MPIDI_NM_mpi_irecv(buf, count, datatype, rank, tag, comm, context_offset, NULL,
                                       &(MPIDI_CH4I_REQUEST_ANYSOURCE_PARTNER(*request)));

        if (mpi_errno != MPI_SUCCESS) {
            MPIR_ERR_POP(mpi_errno);
        }
        else if (*request) {
            MPIDI_CH4I_REQUEST(*request, is_local) = 1;
            MPIDI_CH4I_REQUEST(MPIDI_CH4I_REQUEST_ANYSOURCE_PARTNER(*request), is_local) = 0;
        }

        MPIDI_CH4I_REQUEST_ANYSOURCE_PARTNER(MPIDI_CH4I_REQUEST_ANYSOURCE_PARTNER(*request)) =
            *request;
    }
    else {
        int r;
        if ((r = MPIDI_CH4_rank_is_local(rank, comm)))
            mpi_errno =
                MPIDI_SHM_mpi_irecv(buf, count, datatype, rank, tag, comm, context_offset, request);
        else
            mpi_errno =
                MPIDI_NM_mpi_irecv(buf, count, datatype, rank, tag, comm, context_offset, NULL, request);
        if (mpi_errno == MPI_SUCCESS && *request) {
            MPIDI_CH4I_REQUEST(*request, is_local) = r;
            MPIDI_CH4I_REQUEST_ANYSOURCE_PARTNER(*request) = NULL;
        }
    }
#endif
    if (mpi_errno != MPI_SUCCESS) {
        MPIR_ERR_POP(mpi_errno);
    }
  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_IRECV);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_Cancel_Recv
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPID_Cancel_recv(MPIR_Request * rreq)
{
    int mpi_errno;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_CANCEL_RECV);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_CANCEL_RECV);
#ifndef MPIDI_CH4_EXCLUSIVE_SHM
    mpi_errno = MPIDI_NM_mpi_cancel_recv(rreq);
#else
    if (MPIDI_CH4I_REQUEST(rreq, is_local))
        mpi_errno = MPIDI_SHM_mpi_cancel_recv(rreq);
    else
        mpi_errno = MPIDI_NM_mpi_cancel_recv(rreq);
#endif
    if (mpi_errno != MPI_SUCCESS) {
        MPIR_ERR_POP(mpi_errno);
    }
  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_CANCEL_RECV);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#endif /* CH4_RECV_H_INCLUDED */
