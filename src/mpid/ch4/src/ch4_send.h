/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef CH4_SEND_H_INCLUDED
#define CH4_SEND_H_INCLUDED

#include "ch4_impl.h"
#include "ch4_proc.h"

MPL_STATIC_INLINE_PREFIX int MPIDI_isend(const void *buf,
                                         MPI_Aint count,
                                         MPI_Datatype datatype,
                                         int rank,
                                         int tag,
                                         MPIR_Comm * comm, int context_offset,
                                         MPIDI_av_entry_t * av, MPIR_Request ** req)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_ENTER;

#ifdef MPIDI_CH4_USE_WORK_QUEUES
    MPID_THREAD_CS_ENTER(VCI, MPIDI_VCI(0).lock);
    *(req) = MPIR_Request_create_from_pool(MPIR_REQUEST_KIND__SEND, 0, 1);
    MPID_THREAD_CS_EXIT(VCI, MPIDI_VCI(0).lock);
    MPIR_ERR_CHKANDSTMT((*req) == NULL, mpi_errno, MPIX_ERR_NOREQ, goto fn_fail, "**nomemreq");
    MPIR_Datatype_add_ref_if_not_builtin(datatype);
    MPIDI_workq_pt2pt_enqueue(ISEND, buf, NULL /*recv_buf */ , count, datatype,
                              rank, tag, comm, context_offset, av,
                              NULL /*status */ , *req, NULL /*flag */ ,
                              NULL /*message */ , NULL /*processed */);
#else
    *(req) = NULL;
#ifdef MPIDI_CH4_DIRECT_NETMOD
    mpi_errno = MPIDI_NM_mpi_isend(buf, count, datatype, rank, tag, comm, context_offset, av, req);
#else
    int r;
    if ((r = MPIDI_av_is_local(av)))
        mpi_errno =
            MPIDI_SHM_mpi_isend(buf, count, datatype, rank, tag, comm, context_offset, av, req);
    else
        mpi_errno =
            MPIDI_NM_mpi_isend(buf, count, datatype, rank, tag, comm, context_offset, av, req);
    if (mpi_errno == MPI_SUCCESS)
        MPIDI_REQUEST(*req, is_local) = r;
#endif
    MPIR_ERR_CHECK(mpi_errno);
#endif

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;

  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_isend_coll(const void *buf,
                                              MPI_Aint count,
                                              MPI_Datatype datatype,
                                              int rank,
                                              int tag,
                                              MPIR_Comm * comm, int context_offset,
                                              MPIDI_av_entry_t * av, MPIR_Request ** req,
                                              MPIR_Errflag_t * errflag)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_ENTER;

#ifdef MPIDI_CH4_USE_WORK_QUEUES
    MPID_THREAD_CS_ENTER(VCI, MPIDI_VCI(0).lock);
    *(req) = MPIR_Request_create_from_pool(MPIR_REQUEST_KIND__SEND, 0, 1);
    MPID_THREAD_CS_EXIT(VCI, MPIDI_VCI(0).lock);
    MPIR_ERR_CHKANDSTMT((*req) == NULL, mpi_errno, MPIX_ERR_NOREQ, goto fn_fail, "**nomemreq");
    MPIR_Datatype_add_ref_if_not_builtin(datatype);
    MPIDI_workq_csend_enqueue(ICSEND, buf, count, datatype, rank, tag, comm,
                              context_offset, av, *req, errflag);
#else
    *(req) = NULL;
#ifdef MPIDI_CH4_DIRECT_NETMOD
    mpi_errno =
        MPIDI_NM_isend_coll(buf, count, datatype, rank, tag, comm, context_offset, av, req,
                            errflag);
#else
    int r;
    if ((r = MPIDI_av_is_local(av)))
        mpi_errno =
            MPIDI_SHM_isend_coll(buf, count, datatype, rank, tag, comm, context_offset, av,
                                 req, errflag);
    else
        mpi_errno =
            MPIDI_NM_isend_coll(buf, count, datatype, rank, tag, comm, context_offset, av,
                                req, errflag);
    if (mpi_errno == MPI_SUCCESS)
        MPIDI_REQUEST(*req, is_local) = r;
#endif
    MPIR_ERR_CHECK(mpi_errno);
#endif

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;

  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_issend(const void *buf,
                                          MPI_Aint count,
                                          MPI_Datatype datatype,
                                          int rank,
                                          int tag,
                                          MPIR_Comm * comm, int context_offset,
                                          MPIDI_av_entry_t * av, MPIR_Request ** req)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_ENTER;

#ifdef MPIDI_CH4_USE_WORK_QUEUES
    MPID_THREAD_CS_ENTER(VCI, MPIDI_VCI(0).lock);
    *(req) = MPIR_Request_create_from_pool(MPIR_REQUEST_KIND__SEND, 0, 1);
    MPID_THREAD_CS_EXIT(VCI, MPIDI_VCI(0).lock);
    MPIR_ERR_CHKANDSTMT((*req) == NULL, mpi_errno, MPIX_ERR_NOREQ, goto fn_fail, "**nomemreq");
    MPIR_Datatype_add_ref_if_not_builtin(datatype);
    MPIDI_workq_pt2pt_enqueue(SSEND, buf, NULL /*recv_buf */ , count, datatype,
                              rank, tag, comm, context_offset, av,
                              NULL /*status */ , *req, NULL /*flag */ ,
                              NULL /*message */ , NULL /*processed */);
#else
    *(req) = NULL;
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
#endif

  fn_exit:
    MPIR_FUNC_EXIT;
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
    MPIR_FUNC_ENTER;

    if (MPIDI_is_self_comm(comm)) {
        mpi_errno =
            MPIDI_Self_isend(buf, count, datatype, rank, tag, comm, context_offset, request);
    } else {
        av = MPIDIU_comm_rank_to_av(comm, rank);
        mpi_errno = MPIDI_isend(buf, count, datatype, rank, tag, comm, context_offset, av, request);
    }

    MPIR_ERR_CHECK(mpi_errno);

    if (*request) {
        MPII_SENDQ_REMEMBER(*request, rank, tag, comm->recvcontext_id, buf, count);
    }

  fn_exit:
    MPIR_FUNC_EXIT;
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
    MPIR_FUNC_ENTER;

    if (MPIDI_is_self_comm(comm)) {
        /* FIXME: do we need do anything about errflag? */
        mpi_errno = MPIDI_Self_isend(buf, count, datatype, rank, tag, comm, context_offset,
                                     request);
    } else {
        av = MPIDIU_comm_rank_to_av(comm, rank);
        mpi_errno = MPIDI_isend_coll(buf, count, datatype, rank, tag, comm, context_offset,
                                     av, request, errflag);
    }

    MPIR_ERR_CHECK(mpi_errno);
  fn_exit:
    MPIR_FUNC_EXIT;
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
    return MPID_Isend(buf, count, datatype, rank, tag, comm, context_offset, request);
}

MPL_STATIC_INLINE_PREFIX int MPID_Send_coll(const void *buf,
                                            MPI_Aint count,
                                            MPI_Datatype datatype,
                                            int rank,
                                            int tag,
                                            MPIR_Comm * comm, int context_offset,
                                            MPIR_Request ** request, MPIR_Errflag_t * errflag)
{
    return MPID_Isend_coll(buf, count, datatype, rank, tag, comm, context_offset, request, errflag);
}

MPL_STATIC_INLINE_PREFIX int MPID_Rsend(const void *buf,
                                        MPI_Aint count,
                                        MPI_Datatype datatype,
                                        int rank,
                                        int tag,
                                        MPIR_Comm * comm, int context_offset,
                                        MPIR_Request ** request)
{
    return MPID_Irsend(buf, count, datatype, rank, tag, comm, context_offset, request);
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
    MPIR_FUNC_ENTER;

    if (MPIDI_is_self_comm(comm)) {
        mpi_errno =
            MPIDI_Self_isend(buf, count, datatype, rank, tag, comm, context_offset, request);
    } else {
        av = MPIDIU_comm_rank_to_av(comm, rank);
        mpi_errno = MPIDI_isend(buf, count, datatype, rank, tag, comm, context_offset, av, request);
    }

    MPIR_ERR_CHECK(mpi_errno);

    if (*request) {
        MPII_SENDQ_REMEMBER(*request, rank, tag, comm->recvcontext_id, buf, count);
    }

  fn_exit:
    MPIR_FUNC_EXIT;
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
    return MPID_Issend(buf, count, datatype, rank, tag, comm, context_offset, request);
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
    MPIR_FUNC_ENTER;

    if (MPIDI_is_self_comm(comm)) {
        mpi_errno =
            MPIDI_Self_isend(buf, count, datatype, rank, tag, comm, context_offset, request);
    } else {
        av = MPIDIU_comm_rank_to_av(comm, rank);
        mpi_errno =
            MPIDI_issend(buf, count, datatype, rank, tag, comm, context_offset, av, request);
    }

    MPIR_ERR_CHECK(mpi_errno);
  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

/* Common internal routine for send_init family */
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

    MPIR_FUNC_ENTER;

    MPID_THREAD_CS_ENTER(VCI, MPIDI_VCI(0).lock);
    MPIDI_CH4_REQUEST_CREATE(sreq, MPIR_REQUEST_KIND__PREQUEST_SEND, 0, 1);
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
    MPIR_FUNC_EXIT;
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

    MPIR_FUNC_ENTER;

    mpi_errno = MPIDI_psend_init(MPIDI_PTYPE_SEND,
                                 buf, count, datatype, rank, tag, comm, context_offset, request);
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    MPIR_FUNC_EXIT;
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
    MPIR_FUNC_ENTER;

    mpi_errno = MPIDI_psend_init(MPIDI_PTYPE_SSEND,
                                 buf, count, datatype, rank, tag, comm, context_offset, request);
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    MPIR_FUNC_EXIT;
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
    MPIR_FUNC_ENTER;

    mpi_errno = MPIDI_psend_init(MPIDI_PTYPE_BSEND,
                                 buf, count, datatype, rank, tag, comm, context_offset, request);
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    MPIR_FUNC_EXIT;
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

    MPIR_FUNC_ENTER;

    /* TODO: Currently we don't distinguish SEND and RSEND */
    mpi_errno = MPIDI_psend_init(MPIDI_PTYPE_SEND,
                                 buf, count, datatype, rank, tag, comm, context_offset, request);
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;

}

MPL_STATIC_INLINE_PREFIX int MPID_Cancel_send(MPIR_Request * sreq)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_ENTER;

    if (sreq->comm && MPIDI_is_self_comm(sreq->comm)) {
        mpi_errno = MPIDI_Self_cancel(sreq);
    } else {
#ifdef MPIDI_CH4_DIRECT_NETMOD
        mpi_errno = MPIDI_NM_mpi_cancel_send(sreq);
#else
        if (MPIDI_REQUEST(sreq, is_local))
            mpi_errno = MPIDI_SHM_mpi_cancel_send(sreq);
        else
            mpi_errno = MPIDI_NM_mpi_cancel_send(sreq);
#endif
    }
    MPIR_ERR_CHECK(mpi_errno);
  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#endif /* CH4_SEND_H_INCLUDED */
