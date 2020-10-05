/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef CH4_SEND_H_INCLUDED
#define CH4_SEND_H_INCLUDED

#include "ch4_impl.h"
#include "ch4r_proc.h"

MPL_STATIC_INLINE_PREFIX int MPIDI_send_unsafe(const void *buf,
                                               MPI_Aint count,
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
        MPIDI_REQUEST(*request, is_local) = r;
#endif

    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_SEND_UNSAFE);
    return mpi_errno;

  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_send_coll_unsafe(const void *buf,
                                                    MPI_Aint count,
                                                    MPI_Datatype datatype,
                                                    int rank,
                                                    int tag,
                                                    MPIR_Comm * comm, int context_offset,
                                                    MPIDI_av_entry_t * av, MPIR_Request ** request,
                                                    MPIR_Errflag_t * errflag)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_SEND_COLL_UNSAFE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_SEND_COLL_UNSAFE);

#ifdef MPIDI_CH4_DIRECT_NETMOD
    mpi_errno =
        MPIDI_NM_send_coll(buf, count, datatype, rank, tag, comm, context_offset, av, request,
                           errflag);
#else
    int r;
    if ((r = MPIDI_av_is_local(av)))
        mpi_errno =
            MPIDI_SHM_send_coll(buf, count, datatype, rank, tag, comm, context_offset, av,
                                request, errflag);
    else
        mpi_errno =
            MPIDI_NM_send_coll(buf, count, datatype, rank, tag, comm, context_offset, av,
                               request, errflag);
    if (mpi_errno == MPI_SUCCESS && *request)
        MPIDI_REQUEST(*request, is_local) = r;
#endif

    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_SEND_COLL_UNSAFE);
    return mpi_errno;

  fn_fail:
    goto fn_exit;
}

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
        MPIDI_REQUEST(*request, is_local) = r;
#endif

    MPIR_ERR_CHECK(mpi_errno);
  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_ISEND_UNSAFE);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_isend_coll_unsafe(const void *buf,
                                                     MPI_Aint count,
                                                     MPI_Datatype datatype,
                                                     int rank,
                                                     int tag,
                                                     MPIR_Comm * comm, int context_offset,
                                                     MPIDI_av_entry_t * av,
                                                     MPIR_Request ** request,
                                                     MPIR_Errflag_t * errflag)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_ISEND_COLL_UNSAFE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_ISEND_COLL_UNSAFE);

#ifdef MPIDI_CH4_DIRECT_NETMOD
    mpi_errno =
        MPIDI_NM_isend_coll(buf, count, datatype, rank, tag, comm, context_offset, av, request,
                            errflag);
#else
    int r;
    if ((r = MPIDI_av_is_local(av)))
        mpi_errno =
            MPIDI_SHM_isend_coll(buf, count, datatype, rank, tag, comm, context_offset, av,
                                 request, errflag);
    else
        mpi_errno =
            MPIDI_NM_isend_coll(buf, count, datatype, rank, tag, comm, context_offset, av,
                                request, errflag);
    if (mpi_errno == MPI_SUCCESS)
        MPIDI_REQUEST(*request, is_local) = r;
#endif

    MPIR_ERR_CHECK(mpi_errno);
  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_ISEND_COLL_UNSAFE);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

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
        MPIDI_REQUEST(*req, is_local) = r;
#endif
    MPIR_ERR_CHECK(mpi_errno);
  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_SSEND_UNSAFE);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

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
        MPIDI_REQUEST(*req, is_local) = r;
#endif
    MPIR_ERR_CHECK(mpi_errno);
  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_ISSEND_UNSAFE);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_send_safe(const void *buf,
                                             MPI_Aint count,
                                             MPI_Datatype datatype,
                                             int rank,
                                             int tag,
                                             MPIR_Comm * comm, int context_offset,
                                             MPIDI_av_entry_t * av, MPIR_Request ** req)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_SEND_SAFE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_SEND_SAFE);

#ifdef MPIDI_CH4_USE_WORK_QUEUES
    /* NOTE: When WORKQ is enabled, we always enqueue. However, it probably makes sense
     *       to directly do lightweight send for small message. It is tricky since the
     *       threshold for lightweight send is controlled by lower layer. It'll be nice
     *       if the lower layer expose the threshold.
     */
    MPID_THREAD_CS_ENTER(VCI, MPIDI_VCI(0).lock);
    *(req) = MPIR_Request_create_from_pool(MPIR_REQUEST_KIND__SEND, 0);
    MPID_THREAD_CS_EXIT(VCI, MPIDI_VCI(0).lock);
    MPIR_ERR_CHKANDSTMT((*req) == NULL, mpi_errno, MPIX_ERR_NOREQ, goto fn_fail, "**nomemreq");
    MPIR_Datatype_add_ref_if_not_builtin(datatype);
    MPIDI_workq_pt2pt_enqueue(SEND, buf, NULL /*recv_buf */ , count, datatype,
                              rank, tag, comm, context_offset, av,
                              NULL /*status */ , *req, NULL /*flag */ ,
                              NULL /*message */ , NULL /*processed */);
#else
    *(req) = NULL;
    mpi_errno = MPIDI_send_unsafe(buf, count, datatype, rank, tag, comm, context_offset, av, req);
#endif

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_SEND_SAFE);
    return mpi_errno;

  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_send_coll_safe(const void *buf,
                                                  MPI_Aint count,
                                                  MPI_Datatype datatype,
                                                  int rank,
                                                  int tag,
                                                  MPIR_Comm * comm, int context_offset,
                                                  MPIDI_av_entry_t * av, MPIR_Request ** req,
                                                  MPIR_Errflag_t * errflag)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_SEND_COLL_SAFE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_SEND_COLL_SAFE);

#ifdef MPIDI_CH4_USE_WORK_QUEUES
    MPID_THREAD_CS_ENTER(VCI, MPIDI_VCI(0).lock);
    *(req) = MPIR_Request_create_from_pool(MPIR_REQUEST_KIND__SEND, 0);
    MPID_THREAD_CS_EXIT(VCI, MPIDI_VCI(0).lock);
    MPIR_ERR_CHKANDSTMT((*req) == NULL, mpi_errno, MPIX_ERR_NOREQ, goto fn_fail, "**nomemreq");
    MPIR_Datatype_add_ref_if_not_builtin(datatype);
    MPIDI_workq_csend_enqueue(CSEND, buf, count, datatype, rank, tag, comm,
                              context_offset, av, *req, errflag);
#else
    *(req) = NULL;
    mpi_errno = MPIDI_send_coll_unsafe(buf, count, datatype, rank, tag, comm,
                                       context_offset, av, req, errflag);
#endif

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_SEND_COLL_SAFE);
    return mpi_errno;

  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_isend_safe(const void *buf,
                                              MPI_Aint count,
                                              MPI_Datatype datatype,
                                              int rank,
                                              int tag,
                                              MPIR_Comm * comm, int context_offset,
                                              MPIDI_av_entry_t * av, MPIR_Request ** req)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_ISEND_SAFE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_ISEND_SAFE);

#ifdef MPIDI_CH4_USE_WORK_QUEUES
    MPID_THREAD_CS_ENTER(VCI, MPIDI_VCI(0).lock);
    *(req) = MPIR_Request_create_from_pool(MPIR_REQUEST_KIND__SEND, 0);
    MPID_THREAD_CS_EXIT(VCI, MPIDI_VCI(0).lock);
    MPIR_ERR_CHKANDSTMT((*req) == NULL, mpi_errno, MPIX_ERR_NOREQ, goto fn_fail, "**nomemreq");
    MPIR_Datatype_add_ref_if_not_builtin(datatype);
    MPIDI_workq_pt2pt_enqueue(ISEND, buf, NULL /*recv_buf */ , count, datatype,
                              rank, tag, comm, context_offset, av,
                              NULL /*status */ , *req, NULL /*flag */ ,
                              NULL /*message */ , NULL /*processed */);
#else
    *(req) = NULL;
    mpi_errno = MPIDI_isend_unsafe(buf, count, datatype, rank, tag, comm, context_offset, av, req);
#endif

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_ISEND_SAFE);
    return mpi_errno;

  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_isend_coll_safe(const void *buf,
                                                   MPI_Aint count,
                                                   MPI_Datatype datatype,
                                                   int rank,
                                                   int tag,
                                                   MPIR_Comm * comm, int context_offset,
                                                   MPIDI_av_entry_t * av, MPIR_Request ** req,
                                                   MPIR_Errflag_t * errflag)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_ISEND_COLL_SAFE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_ISEND_COLL_SAFE);

#ifdef MPIDI_CH4_USE_WORK_QUEUES
    MPID_THREAD_CS_ENTER(VCI, MPIDI_VCI(0).lock);
    *(req) = MPIR_Request_create_from_pool(MPIR_REQUEST_KIND__SEND, 0);
    MPID_THREAD_CS_EXIT(VCI, MPIDI_VCI(0).lock);
    MPIR_ERR_CHKANDSTMT((*req) == NULL, mpi_errno, MPIX_ERR_NOREQ, goto fn_fail, "**nomemreq");
    MPIR_Datatype_add_ref_if_not_builtin(datatype);
    MPIDI_workq_csend_enqueue(ICSEND, buf, count, datatype, rank, tag, comm,
                              context_offset, av, *req, errflag);
#else
    *(req) = NULL;
    mpi_errno = MPIDI_isend_coll_unsafe(buf, count, datatype, rank, tag, comm,
                                        context_offset, av, req, errflag);
#endif

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_ISEND_COLL_SAFE);
    return mpi_errno;

  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_ssend_safe(const void *buf,
                                              MPI_Aint count,
                                              MPI_Datatype datatype,
                                              int rank,
                                              int tag,
                                              MPIR_Comm * comm, int context_offset,
                                              MPIDI_av_entry_t * av, MPIR_Request ** req)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_SSEND_SAFE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_SSEND_SAFE);

#ifdef MPIDI_CH4_USE_WORK_QUEUES
    MPID_THREAD_CS_ENTER(VCI, MPIDI_VCI(0).lock);
    *(req) = MPIR_Request_create_from_pool(MPIR_REQUEST_KIND__SEND, 0);
    MPID_THREAD_CS_EXIT(VCI, MPIDI_VCI(0).lock);
    MPIR_ERR_CHKANDSTMT((*req) == NULL, mpi_errno, MPIX_ERR_NOREQ, goto fn_fail, "**nomemreq");
    MPIR_Datatype_add_ref_if_not_builtin(datatype);
    MPIDI_workq_pt2pt_enqueue(SSEND, buf, NULL /*recv_buf */ , count, datatype,
                              rank, tag, comm, context_offset, av,
                              NULL /*status */ , *req, NULL /*flag */ ,
                              NULL /*message */ , NULL /*processed */);
#else
    *(req) = NULL;
    mpi_errno = MPIDI_ssend_unsafe(buf, count, datatype, rank, tag, comm, context_offset, av, req);
#endif

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_SSEND_SAFE);
    return mpi_errno;

  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_issend_safe(const void *buf,
                                               MPI_Aint count,
                                               MPI_Datatype datatype,
                                               int rank,
                                               int tag,
                                               MPIR_Comm * comm, int context_offset,
                                               MPIDI_av_entry_t * av, MPIR_Request ** req)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_ISSEND_SAFE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_ISSEND_SAFE);

#ifdef MPIDI_CH4_USE_WORK_QUEUES
    MPID_THREAD_CS_ENTER(VCI, MPIDI_VCI(0).lock);
    *(req) = MPIR_Request_create_from_pool(MPIR_REQUEST_KIND__SEND, 0);
    MPID_THREAD_CS_EXIT(VCI, MPIDI_VCI(0).lock);
    MPIR_ERR_CHKANDSTMT((*req) == NULL, mpi_errno, MPIX_ERR_NOREQ, goto fn_fail, "**nomemreq");
    MPIR_Datatype_add_ref_if_not_builtin(datatype);
    MPIDI_workq_pt2pt_enqueue(SSEND, buf, NULL /*recv_buf */ , count, datatype,
                              rank, tag, comm, context_offset, av,
                              NULL /*status */ , *req, NULL /*flag */ ,
                              NULL /*message */ , NULL /*processed */);
#else
    *(req) = NULL;
    mpi_errno = MPIDI_issend_unsafe(buf, count, datatype, rank, tag, comm, context_offset, av, req);
#endif

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_ISSEND_SAFE);
    return mpi_errno;

  fn_fail:
    goto fn_exit;
}

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

    av = MPIDIU_comm_rank_to_av(comm, rank);
    mpi_errno = MPIDI_send_safe(buf, count, datatype, rank, tag, comm, context_offset, av, request);

    MPIR_ERR_CHECK(mpi_errno);
  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_SEND);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPID_Send_coll(const void *buf,
                                            MPI_Aint count,
                                            MPI_Datatype datatype,
                                            int rank,
                                            int tag,
                                            MPIR_Comm * comm, int context_offset,
                                            MPIR_Request ** request, MPIR_Errflag_t * errflag)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_av_entry_t *av = NULL;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_SEND_COLL);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_SEND_COLL);

    av = MPIDIU_comm_rank_to_av(comm, rank);
    mpi_errno = MPIDI_send_coll_safe(buf, count, datatype, rank, tag, comm, context_offset, av,
                                     request, errflag);

    MPIR_ERR_CHECK(mpi_errno);
  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_SEND_COLL);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}


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

    av = MPIDIU_comm_rank_to_av(comm, rank);
    mpi_errno =
        MPIDI_isend_safe(buf, count, datatype, rank, tag, comm, context_offset, av, request);

    MPIR_ERR_CHECK(mpi_errno);
  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_ISEND);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPID_Isend_coll(const void *buf,
                                             MPI_Aint count,
                                             MPI_Datatype datatype,
                                             int rank,
                                             int tag,
                                             MPIR_Comm * comm, int context_offset,
                                             MPIR_Request ** request, MPIR_Errflag_t * errflag)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_av_entry_t *av = NULL;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_ISEND_COLL);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_ISEND_COLL);

    av = MPIDIU_comm_rank_to_av(comm, rank);
    mpi_errno =
        MPIDI_isend_coll_safe(buf, count, datatype, rank, tag, comm, context_offset, av, request,
                              errflag);

    MPIR_ERR_CHECK(mpi_errno);
  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_ISEND_COLL);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}


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

    av = MPIDIU_comm_rank_to_av(comm, rank);
    mpi_errno = MPIDI_send_safe(buf, count, datatype, rank, tag, comm, context_offset, av, request);

    MPIR_ERR_CHECK(mpi_errno);
  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_RSEND);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

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

    av = MPIDIU_comm_rank_to_av(comm, rank);
    mpi_errno =
        MPIDI_isend_safe(buf, count, datatype, rank, tag, comm, context_offset, av, request);

    MPIR_ERR_CHECK(mpi_errno);
  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_IRSEND);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

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

    av = MPIDIU_comm_rank_to_av(comm, rank);
    mpi_errno =
        MPIDI_ssend_safe(buf, count, datatype, rank, tag, comm, context_offset, av, request);

    MPIR_ERR_CHECK(mpi_errno);
  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_SSEND);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

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

    av = MPIDIU_comm_rank_to_av(comm, rank);
    mpi_errno =
        MPIDI_issend_safe(buf, count, datatype, rank, tag, comm, context_offset, av, request);

    MPIR_ERR_CHECK(mpi_errno);
  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_ISSEND);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

/* Common internal rountine for send_init family */
MPL_STATIC_INLINE_PREFIX int MPIDI_psend_init(MPIDI_ptype ptype,
                                              const void *buf,
                                              MPI_Aint count,
                                              MPI_Datatype datatype,
                                              int rank,
                                              int tag,
                                              MPIR_Comm * comm, int context_offset,
                                              MPIR_Request ** request)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Request *sreq;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_PSEND_INIT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_PSEND_INIT);

    MPID_THREAD_CS_ENTER(VCI, MPIDI_VCI(0).lock);
    sreq = MPIR_Request_create_from_pool(MPIR_REQUEST_KIND__PREQUEST_SEND, 0);
    MPID_THREAD_CS_EXIT(VCI, MPIDI_VCI(0).lock);
    MPIR_ERR_CHKANDSTMT(sreq == NULL, mpi_errno, MPIX_ERR_NOREQ, goto fn_fail, "**nomemreq");
    *request = sreq;

    MPIR_Comm_add_ref(comm);
    sreq->comm = comm;

    MPIDI_PREQUEST(sreq, p_type) = ptype;
    MPIDI_PREQUEST(sreq, buffer) = (void *) buf;
    MPIDI_PREQUEST(sreq, count) = count;
    MPIDI_PREQUEST(sreq, datatype) = datatype;
    MPIDI_PREQUEST(sreq, rank) = rank;
    MPIDI_PREQUEST(sreq, tag) = tag;
    MPIDI_PREQUEST(sreq, context_id) = comm->context_id + context_offset;
    sreq->u.persist.real_request = NULL;
    MPIR_cc_set(sreq->cc_ptr, 0);

    MPIR_Datatype_add_ref_if_not_builtin(datatype);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_PSEND_INIT);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPID_Send_init(const void *buf,
                                            MPI_Aint count,
                                            MPI_Datatype datatype,
                                            int rank,
                                            int tag,
                                            MPIR_Comm * comm, int context_offset,
                                            MPIR_Request ** request)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_SEND_INIT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_SEND_INIT);

    mpi_errno = MPIDI_psend_init(MPIDI_PTYPE_SEND,
                                 buf, count, datatype, rank, tag, comm, context_offset, request);
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_SEND_INIT);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPID_Ssend_init(const void *buf,
                                             MPI_Aint count,
                                             MPI_Datatype datatype,
                                             int rank,
                                             int tag,
                                             MPIR_Comm * comm, int context_offset,
                                             MPIR_Request ** request)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_SSEND_INIT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_SSEND_INIT);

    mpi_errno = MPIDI_psend_init(MPIDI_PTYPE_SSEND,
                                 buf, count, datatype, rank, tag, comm, context_offset, request);
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_SSEND_INIT);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPID_Bsend_init(const void *buf,
                                             MPI_Aint count,
                                             MPI_Datatype datatype,
                                             int rank,
                                             int tag,
                                             MPIR_Comm * comm, int context_offset,
                                             MPIR_Request ** request)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_BSEND_INIT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_BSEND_INIT);

    mpi_errno = MPIDI_psend_init(MPIDI_PTYPE_BSEND,
                                 buf, count, datatype, rank, tag, comm, context_offset, request);
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_BSEND_INIT);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPID_Rsend_init(const void *buf,
                                             MPI_Aint count,
                                             MPI_Datatype datatype,
                                             int rank,
                                             int tag,
                                             MPIR_Comm * comm, int context_offset,
                                             MPIR_Request ** request)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_RSEND_INIT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_RSEND_INIT);

    /* TODO: Currently we don't distinguish SEND and RSEND */
    mpi_errno = MPIDI_psend_init(MPIDI_PTYPE_SEND,
                                 buf, count, datatype, rank, tag, comm, context_offset, request);
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_RSEND_INIT);
    return mpi_errno;
  fn_fail:
    goto fn_exit;

}

MPL_STATIC_INLINE_PREFIX int MPID_Cancel_send(MPIR_Request * sreq)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_CANCEL_SEND);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_CANCEL_SEND);
#ifdef MPIDI_CH4_DIRECT_NETMOD
    mpi_errno = MPIDI_NM_mpi_cancel_send(sreq);
#else
    if (MPIDI_REQUEST(sreq, is_local))
        mpi_errno = MPIDI_SHM_mpi_cancel_send(sreq);
    else
        mpi_errno = MPIDI_NM_mpi_cancel_send(sreq);
#endif
    MPIR_ERR_CHECK(mpi_errno);
  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_CANCEL_SEND);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#endif /* CH4_SEND_H_INCLUDED */
