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
#define FUNCNAME MPIDI_send_unsafe
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPIDI_send_unsafe(const void *buf,
                                               int count,
                                               MPI_Datatype datatype,
                                               int rank,
                                               int tag,
                                               MPIR_Comm * comm, int context_offset,
                                               MPIDI_av_entry_t * av, MPIR_Request ** request)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_SEND_UNSAFE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_SEND_UNSAFE);

#ifdef MPIDI_CH4_DIRECT_NETMOD
    mpi_errno =
        MPIDI_NM_mpi_send(buf, count, datatype, rank, tag, comm, context_offset, av, request);
#else
    int r;
    if ((r = MPIDI_av_is_local(av)))
        mpi_errno =
            MPIDI_SHM_mpi_send(buf, count, datatype, rank, tag, comm, context_offset, av, request);
    else
        mpi_errno =
            MPIDI_NM_mpi_send(buf, count, datatype, rank, tag, comm, context_offset, av, request);
    if (mpi_errno == MPI_SUCCESS && *request)
        MPIDI_CH4I_REQUEST(*request, is_local) = r;
#endif

    if (mpi_errno != MPI_SUCCESS) {
        MPIR_ERR_POP(mpi_errno);
    }

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_SEND_UNSAFE);
    return mpi_errno;

  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPID_isend_unsafe
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPIDI_isend_unsafe(const void *buf,
                                                MPI_Aint count,
                                                MPI_Datatype datatype,
                                                int rank,
                                                int tag,
                                                MPIR_Comm * comm, int context_offset,
                                                MPIDI_av_entry_t * av, MPIR_Request ** request)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_ISEND_UNSAFE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_ISEND_UNSAFE);

#ifdef MPIDI_CH4_DIRECT_NETMOD
    mpi_errno =
        MPIDI_NM_mpi_isend(buf, count, datatype, rank, tag, comm, context_offset, av, request);
#else
    int r;
    if ((r = MPIDI_av_is_local(av)))
        mpi_errno =
            MPIDI_SHM_mpi_isend(buf, count, datatype, rank, tag, comm, context_offset, av, request);
    else
        mpi_errno =
            MPIDI_NM_mpi_isend(buf, count, datatype, rank, tag, comm, context_offset, av, request);
    if (mpi_errno == MPI_SUCCESS)
        MPIDI_CH4I_REQUEST(*request, is_local) = r;
#endif

    if (mpi_errno != MPI_SUCCESS) {
        MPIR_ERR_POP(mpi_errno);
    }
  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_ISEND_UNSAFE);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_ssend_unsafe
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPIDI_ssend_unsafe(const void *buf,
                                                MPI_Aint count,
                                                MPI_Datatype datatype,
                                                int rank,
                                                int tag,
                                                MPIR_Comm * comm, int context_offset,
                                                MPIDI_av_entry_t * av, MPIR_Request ** req)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_SSEND_UNSAFE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_SSEND_UNSAFE);

    av = MPIDIU_comm_rank_to_av(comm, rank);
#ifdef MPIDI_CH4_DIRECT_NETMOD
    mpi_errno = MPIDI_NM_mpi_ssend(buf, count, datatype, rank, tag, comm, context_offset, av, req);
#else
    int r;
    if ((r = MPIDI_av_is_local(av)))
        mpi_errno =
            MPIDI_SHM_mpi_ssend(buf, count, datatype, rank, tag, comm, context_offset, av, req);
    else
        mpi_errno =
            MPIDI_NM_mpi_ssend(buf, count, datatype, rank, tag, comm, context_offset, av, req);

    if (mpi_errno == MPI_SUCCESS && *req)
        MPIDI_CH4I_REQUEST(*req, is_local) = r;
#endif
    if (mpi_errno != MPI_SUCCESS) {
        MPIR_ERR_POP(mpi_errno);
    }
  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_SSEND_UNSAFE);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPID_issend_unsafe
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPIDI_issend_unsafe(const void *buf,
                                                 MPI_Aint count,
                                                 MPI_Datatype datatype,
                                                 int rank,
                                                 int tag,
                                                 MPIR_Comm * comm, int context_offset,
                                                 MPIDI_av_entry_t * av, MPIR_Request ** req)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_ISSEND_UNSAFE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_ISSEND_UNSAFE);

#ifdef MPIDI_CH4_DIRECT_NETMOD
    mpi_errno = MPIDI_NM_mpi_issend(buf, count, datatype, rank, tag, comm, context_offset, av, req);
#else
    int r;
    if ((r = MPIDI_av_is_local(av)))
        mpi_errno =
            MPIDI_SHM_mpi_issend(buf, count, datatype, rank, tag, comm, context_offset, av, req);
    else
        mpi_errno =
            MPIDI_NM_mpi_issend(buf, count, datatype, rank, tag, comm, context_offset, av, req);

    if (mpi_errno == MPI_SUCCESS)
        MPIDI_CH4I_REQUEST(*req, is_local) = r;
#endif
    if (mpi_errno != MPI_SUCCESS) {
        MPIR_ERR_POP(mpi_errno);
    }
  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_ISSEND_UNSAFE);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_send_safe
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPIDI_send_safe(const void *buf,
                                             int count,
                                             MPI_Datatype datatype,
                                             int rank,
                                             int tag,
                                             MPIR_Comm * comm, int context_offset,
                                             MPIDI_av_entry_t * av, MPIR_Request ** req)
{
    int mpi_errno = MPI_SUCCESS, cs_acq = 0;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_SEND_SAFE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_SEND_SAFE);

    MPID_THREAD_SAFE_BEGIN(VNI, MPIDI_CH4_Global.vni_lock, cs_acq);

    if (!cs_acq) {
        *(req) = MPIR_Request_create(MPIR_REQUEST_KIND__SEND);
        MPIR_ERR_CHKANDSTMT((*req) == NULL, mpi_errno, MPIX_ERR_NOREQ, goto fn_fail, "**nomemreq");
        MPIR_Datatype_add_ref_if_not_builtin(datatype);
        MPIDI_workq_pt2pt_enqueue(SEND, buf, NULL /*recv_buf */ , count, datatype,
                                  rank, tag, comm, context_offset, av,
                                  NULL /*status */ , *req, NULL /*flag */ ,
                                  NULL /*message */ , NULL /*processed */);
    } else {
        *(req) = NULL;
        MPIDI_workq_vni_progress_unsafe();
        mpi_errno =
            MPIDI_send_unsafe(buf, count, datatype, rank, tag, comm, context_offset, av, req);
    }

  fn_exit:
    MPID_THREAD_SAFE_END(VNI, MPIDI_CH4_Global.vni_lock, cs_acq);
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_SEND_SAFE);
    return mpi_errno;

  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_isend_safe
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPIDI_isend_safe(const void *buf,
                                              int count,
                                              MPI_Datatype datatype,
                                              int rank,
                                              int tag,
                                              MPIR_Comm * comm, int context_offset,
                                              MPIDI_av_entry_t * av, MPIR_Request ** req)
{
    int mpi_errno = MPI_SUCCESS, cs_acq = 0;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_ISEND_SAFE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_ISEND_SAFE);

    MPID_THREAD_SAFE_BEGIN(VNI, MPIDI_CH4_Global.vni_lock, cs_acq);

    if (!cs_acq) {
        *(req) = MPIR_Request_create(MPIR_REQUEST_KIND__SEND);
        MPIR_ERR_CHKANDSTMT((*req) == NULL, mpi_errno, MPIX_ERR_NOREQ, goto fn_fail, "**nomemreq");
        MPIR_Datatype_add_ref_if_not_builtin(datatype);
        MPIDI_workq_pt2pt_enqueue(ISEND, buf, NULL /*recv_buf */ , count, datatype,
                                  rank, tag, comm, context_offset, av,
                                  NULL /*status */ , *req, NULL /*flag */ ,
                                  NULL /*message */ , NULL /*processed */);
    } else {
        *(req) = NULL;
        MPIDI_workq_vni_progress_unsafe();
        mpi_errno =
            MPIDI_isend_unsafe(buf, count, datatype, rank, tag, comm, context_offset, av, req);
    }

  fn_exit:
    MPID_THREAD_SAFE_END(VNI, MPIDI_CH4_Global.vni_lock, cs_acq);
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_ISEND_SAFE);
    return mpi_errno;

  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_ssend_safe
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPIDI_ssend_safe(const void *buf,
                                              int count,
                                              MPI_Datatype datatype,
                                              int rank,
                                              int tag,
                                              MPIR_Comm * comm, int context_offset,
                                              MPIDI_av_entry_t * av, MPIR_Request ** req)
{
    int mpi_errno = MPI_SUCCESS, cs_acq = 0;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_SSEND_SAFE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_SSEND_SAFE);

    MPID_THREAD_SAFE_BEGIN(VNI, MPIDI_CH4_Global.vni_lock, cs_acq);

    if (!cs_acq) {
        *(req) = MPIR_Request_create(MPIR_REQUEST_KIND__SEND);
        MPIR_ERR_CHKANDSTMT((*req) == NULL, mpi_errno, MPIX_ERR_NOREQ, goto fn_fail, "**nomemreq");
        MPIR_Datatype_add_ref_if_not_builtin(datatype);
        MPIDI_workq_pt2pt_enqueue(SSEND, buf, NULL /*recv_buf */ , count, datatype,
                                  rank, tag, comm, context_offset, av,
                                  NULL /*status */ , *req, NULL /*flag */ ,
                                  NULL /*message */ , NULL /*processed */);
    } else {
        *(req) = NULL;
        MPIDI_workq_vni_progress_unsafe();
        mpi_errno =
            MPIDI_ssend_unsafe(buf, count, datatype, rank, tag, comm, context_offset, av, req);
    }

  fn_exit:
    MPID_THREAD_SAFE_END(VNI, MPIDI_CH4_Global.vni_lock, cs_acq);
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_SSEND_SAFE);
    return mpi_errno;

  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_issend_safe
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPIDI_issend_safe(const void *buf,
                                               int count,
                                               MPI_Datatype datatype,
                                               int rank,
                                               int tag,
                                               MPIR_Comm * comm, int context_offset,
                                               MPIDI_av_entry_t * av, MPIR_Request ** req)
{
    int mpi_errno = MPI_SUCCESS, cs_acq = 0;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_ISSEND_SAFE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_ISSEND_SAFE);

    MPID_THREAD_SAFE_BEGIN(VNI, MPIDI_CH4_Global.vni_lock, cs_acq);

    if (!cs_acq) {
        *(req) = MPIR_Request_create(MPIR_REQUEST_KIND__SEND);
        MPIR_ERR_CHKANDSTMT((*req) == NULL, mpi_errno, MPIX_ERR_NOREQ, goto fn_fail, "**nomemreq");
        MPIR_Datatype_add_ref_if_not_builtin(datatype);
        MPIDI_workq_pt2pt_enqueue(SSEND, buf, NULL /*recv_buf */ , count, datatype,
                                  rank, tag, comm, context_offset, av,
                                  NULL /*status */ , *req, NULL /*flag */ ,
                                  NULL /*message */ , NULL /*processed */);
    } else {
        *(req) = NULL;
        MPIDI_workq_vni_progress_unsafe();
        mpi_errno =
            MPIDI_issend_unsafe(buf, count, datatype, rank, tag, comm, context_offset, av, req);
    }

  fn_exit:
    MPID_THREAD_SAFE_END(VNI, MPIDI_CH4_Global.vni_lock, cs_acq);
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_ISSEND_SAFE);
    return mpi_errno;

  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPID_Send
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPID_Send(const void *buf,
                                       MPI_Aint count,
                                       MPI_Datatype datatype,
                                       int rank,
                                       int tag,
                                       MPIR_Comm * comm, int context_offset,
                                       MPIR_Request ** request)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_av_entry_t *av = NULL;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_SEND);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_SEND);

    if (unlikely(rank == MPI_PROC_NULL)) {
        *request = MPIR_Request_create_complete(MPIR_REQUEST_KIND__SEND);
        MPIR_ERR_CHKANDSTMT(*request == NULL, mpi_errno, MPIX_ERR_NOREQ, goto fn_fail,
                            "**nomemreq");
        goto fn_exit;
    }

    av = MPIDIU_comm_rank_to_av(comm, rank);
    mpi_errno = MPIDI_send_safe(buf, count, datatype, rank, tag, comm, context_offset, av, request);

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
                                        MPI_Aint count,
                                        MPI_Datatype datatype,
                                        int rank,
                                        int tag,
                                        MPIR_Comm * comm, int context_offset,
                                        MPIR_Request ** request)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_av_entry_t *av = NULL;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_ISEND);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_ISEND);

    if (unlikely(rank == MPI_PROC_NULL)) {
        *request = MPIR_Request_create_complete(MPIR_REQUEST_KIND__SEND);
        MPIR_ERR_CHKANDSTMT(*request == NULL, mpi_errno, MPIX_ERR_NOREQ, goto fn_fail,
                            "**nomemreq");
        goto fn_exit;
    }

    av = MPIDIU_comm_rank_to_av(comm, rank);
    mpi_errno =
        MPIDI_isend_safe(buf, count, datatype, rank, tag, comm, context_offset, av, request);

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
                                        MPI_Aint count,
                                        MPI_Datatype datatype,
                                        int rank,
                                        int tag,
                                        MPIR_Comm * comm, int context_offset,
                                        MPIR_Request ** request)
{
    /*
     * FIXME: this implementation of MPID_Rsend is identical to that of MPID_Send.
     * Need to support rsend protocol, i.e., check if receive is posted on the
     * reveiver side before the send is posted.
     */

    int mpi_errno = MPI_SUCCESS;
    MPIDI_av_entry_t *av = NULL;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_RSEND);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_RSEND);

    if (unlikely(rank == MPI_PROC_NULL)) {
        *request = MPIR_Request_create_complete(MPIR_REQUEST_KIND__SEND);
        MPIR_ERR_CHKANDSTMT(*request == NULL, mpi_errno, MPIX_ERR_NOREQ, goto fn_fail,
                            "**nomemreq");
        goto fn_exit;
    }

    av = MPIDIU_comm_rank_to_av(comm, rank);
    mpi_errno = MPIDI_send_safe(buf, count, datatype, rank, tag, comm, context_offset, av, request);

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
                                         MPI_Aint count,
                                         MPI_Datatype datatype,
                                         int rank,
                                         int tag,
                                         MPIR_Comm * comm, int context_offset,
                                         MPIR_Request ** request)
{
    /*
     * FIXME: this implementation of MPID_Irsend is identical to that of MPID_Isend.
     * Need to support irsend protocol, i.e., check if receive is posted on the
     * reveiver side before the send is posted.
     */

    int mpi_errno = MPI_SUCCESS;
    MPIDI_av_entry_t *av = NULL;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_IRSEND);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_IRSEND);

    if (unlikely(rank == MPI_PROC_NULL)) {
        *request = MPIR_Request_create_complete(MPIR_REQUEST_KIND__SEND);
        MPIR_ERR_CHKANDSTMT(*request == NULL, mpi_errno, MPIX_ERR_NOREQ, goto fn_fail,
                            "**nomemreq");
        goto fn_exit;
    }

    av = MPIDIU_comm_rank_to_av(comm, rank);
    mpi_errno =
        MPIDI_isend_safe(buf, count, datatype, rank, tag, comm, context_offset, av, request);

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
                                        MPI_Aint count,
                                        MPI_Datatype datatype,
                                        int rank,
                                        int tag,
                                        MPIR_Comm * comm, int context_offset,
                                        MPIR_Request ** request)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_av_entry_t *av = NULL;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_SSEND);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_SSEND);

    if (unlikely(rank == MPI_PROC_NULL)) {
        *request = MPIR_Request_create_complete(MPIR_REQUEST_KIND__SEND);
        MPIR_ERR_CHKANDSTMT(*request == NULL, mpi_errno, MPIX_ERR_NOREQ, goto fn_fail,
                            "**nomemreq");
        goto fn_exit;
    }

    av = MPIDIU_comm_rank_to_av(comm, rank);
    mpi_errno =
        MPIDI_ssend_safe(buf, count, datatype, rank, tag, comm, context_offset, av, request);

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
                                         MPI_Aint count,
                                         MPI_Datatype datatype,
                                         int rank,
                                         int tag,
                                         MPIR_Comm * comm, int context_offset,
                                         MPIR_Request ** request)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_av_entry_t *av = NULL;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_ISSEND);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_ISSEND);

    if (unlikely(rank == MPI_PROC_NULL)) {
        *request = MPIR_Request_create_complete(MPIR_REQUEST_KIND__SEND);
        MPIR_ERR_CHKANDSTMT(*request == NULL, mpi_errno, MPIX_ERR_NOREQ, goto fn_fail,
                            "**nomemreq");
        goto fn_exit;
    }

    av = MPIDIU_comm_rank_to_av(comm, rank);
    mpi_errno =
        MPIDI_issend_safe(buf, count, datatype, rank, tag, comm, context_offset, av, request);

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
    int mpi_errno = MPI_SUCCESS;
    MPIDI_av_entry_t *av = NULL;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_SEND_INIT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_SEND_INIT);
    av = MPIDIU_comm_rank_to_av(comm, rank);
#ifdef MPIDI_CH4_DIRECT_NETMOD
    mpi_errno =
        MPIDI_NM_mpi_send_init(buf, count, datatype, rank, tag, comm, context_offset, av, request);
#else
    int r;
    if ((r = MPIDI_av_is_local(av)))
        mpi_errno = MPIDI_SHM_mpi_send_init(buf, count, datatype, rank, tag,
                                            comm, context_offset, request);
    else
        mpi_errno = MPIDI_NM_mpi_send_init(buf, count, datatype, rank, tag,
                                           comm, context_offset, av, request);
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
    int mpi_errno = MPI_SUCCESS;
    MPIDI_av_entry_t *av = NULL;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_SSEND_INIT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_SSEND_INIT);
    av = MPIDIU_comm_rank_to_av(comm, rank);
#ifdef MPIDI_CH4_DIRECT_NETMOD
    mpi_errno =
        MPIDI_NM_mpi_ssend_init(buf, count, datatype, rank, tag, comm, context_offset, av, request);
#else
    int r;
    if ((r = MPIDI_av_is_local(av)))
        mpi_errno = MPIDI_SHM_mpi_ssend_init(buf, count, datatype, rank, tag,
                                             comm, context_offset, request);
    else
        mpi_errno = MPIDI_NM_mpi_ssend_init(buf, count, datatype, rank, tag,
                                            comm, context_offset, av, request);
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
    int mpi_errno = MPI_SUCCESS;
    MPIDI_av_entry_t *av = NULL;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_BSEND_INIT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_BSEND_INIT);
    av = MPIDIU_comm_rank_to_av(comm, rank);
#ifdef MPIDI_CH4_DIRECT_NETMOD
    mpi_errno =
        MPIDI_NM_mpi_bsend_init(buf, count, datatype, rank, tag, comm, context_offset, av, request);
#else
    int r;
    if ((r = MPIDI_av_is_local(av)))
        mpi_errno = MPIDI_SHM_mpi_bsend_init(buf, count, datatype, rank, tag,
                                             comm, context_offset, request);
    else
        mpi_errno = MPIDI_NM_mpi_bsend_init(buf, count, datatype, rank, tag,
                                            comm, context_offset, av, request);
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
    int mpi_errno = MPI_SUCCESS;
    MPIDI_av_entry_t *av = NULL;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_RSEND_INIT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_RSEND_INIT);
    av = MPIDIU_comm_rank_to_av(comm, rank);
#ifdef MPIDI_CH4_DIRECT_NETMOD
    mpi_errno =
        MPIDI_NM_mpi_rsend_init(buf, count, datatype, rank, tag, comm, context_offset, av, request);
#else
    int r;
    if ((r = MPIDI_av_is_local(av)))
        mpi_errno = MPIDI_SHM_mpi_rsend_init(buf, count, datatype, rank, tag,
                                             comm, context_offset, request);
    else
        mpi_errno = MPIDI_NM_mpi_rsend_init(buf, count, datatype, rank, tag,
                                            comm, context_offset, av, request);
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
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_CANCEL_SEND);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_CANCEL_SEND);
#ifdef MPIDI_CH4_DIRECT_NETMOD
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
