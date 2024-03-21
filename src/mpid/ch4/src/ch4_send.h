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
                                         MPIR_Comm * comm, int attr,
                                         MPIDI_av_entry_t * av,
                                         MPIR_cc_t * parent_cc_ptr, MPIR_Request ** req)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_ENTER;

    *(req) = NULL;
#ifdef MPIDI_CH4_DIRECT_NETMOD
    mpi_errno =
        MPIDI_NM_mpi_isend(buf, count, datatype, rank, tag, comm, attr, av, parent_cc_ptr, req);
#else
    int r;
    if ((r = MPIDI_av_is_local(av)))
        mpi_errno =
            MPIDI_SHM_mpi_isend(buf, count, datatype, rank, tag, comm, attr, av, parent_cc_ptr,
                                req);
    else
        mpi_errno =
            MPIDI_NM_mpi_isend(buf, count, datatype, rank, tag, comm, attr, av, parent_cc_ptr, req);
    if (mpi_errno == MPI_SUCCESS)
        MPIDI_REQUEST(*req, is_local) = r;
#endif
    MPIR_ERR_CHECK(mpi_errno);

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
                                        MPIR_Comm * comm, int attr, MPIR_Request ** request)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_av_entry_t *av = NULL;
    MPIR_FUNC_ENTER;

    if (MPIR_is_self_comm(comm)) {
        mpi_errno = MPIDI_Self_isend(buf, count, datatype, rank, tag, comm, attr, NULL, request);
    } else {
        av = MPIDIU_comm_rank_to_av(comm, rank);
        mpi_errno = MPIDI_isend(buf, count, datatype, rank, tag, comm, attr, av, NULL, request);
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

MPL_STATIC_INLINE_PREFIX int MPID_Isend_parent(const void *buf,
                                               MPI_Aint count,
                                               MPI_Datatype datatype,
                                               int rank,
                                               int tag,
                                               MPIR_Comm * comm, int attr,
                                               MPIR_cc_t * parent_cc_ptr, MPIR_Request ** request)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_av_entry_t *av = NULL;
    MPIR_FUNC_ENTER;

    if (MPIDI_is_self_comm(comm)) {
        mpi_errno =
            MPIDI_Self_isend(buf, count, datatype, rank, tag, comm, attr, parent_cc_ptr, request);
    } else {
        av = MPIDIU_comm_rank_to_av(comm, rank);
        mpi_errno =
            MPIDI_isend(buf, count, datatype, rank, tag, comm, attr, av, parent_cc_ptr, request);
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
                                             MPIR_Comm * comm, int attr,
                                             MPIR_Request ** request, MPIR_Errflag_t * errflag)
{
    MPIR_PT2PT_ATTR_SET_ERRFLAG(attr, *errflag);
    return MPID_Isend(buf, count, datatype, rank, tag, comm, attr, request);
}

MPL_STATIC_INLINE_PREFIX int MPID_Send(const void *buf,
                                       MPI_Aint count,
                                       MPI_Datatype datatype,
                                       int rank,
                                       int tag, MPIR_Comm * comm, int attr, MPIR_Request ** request)
{
    return MPID_Isend(buf, count, datatype, rank, tag, comm, attr, request);
}

MPL_STATIC_INLINE_PREFIX int MPID_Rsend(const void *buf,
                                        MPI_Aint count,
                                        MPI_Datatype datatype,
                                        int rank,
                                        int tag,
                                        MPIR_Comm * comm, int attr, MPIR_Request ** request)
{
    return MPID_Irsend(buf, count, datatype, rank, tag, comm, attr, request);
}

MPL_STATIC_INLINE_PREFIX int MPID_Irsend(const void *buf,
                                         MPI_Aint count,
                                         MPI_Datatype datatype,
                                         int rank,
                                         int tag,
                                         MPIR_Comm * comm, int attr, MPIR_Request ** request)
{
    /*
     * FIXME: this implementation of MPID_Irsend is identical to that of MPID_Isend.
     * Need to support irsend protocol, i.e., check if receive is posted on the
     * reveiver side before the send is posted.
     */

    int mpi_errno = MPI_SUCCESS;
    MPIDI_av_entry_t *av = NULL;
    MPIR_FUNC_ENTER;

    if (MPIR_is_self_comm(comm)) {
        mpi_errno = MPIDI_Self_isend(buf, count, datatype, rank, tag, comm, attr, NULL, request);
    } else {
        av = MPIDIU_comm_rank_to_av(comm, rank);
        mpi_errno = MPIDI_isend(buf, count, datatype, rank, tag, comm, attr, av, NULL, request);
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
                                        MPIR_Comm * comm, int attr, MPIR_Request ** request)
{
    return MPID_Issend(buf, count, datatype, rank, tag, comm, attr, request);
}

MPL_STATIC_INLINE_PREFIX int MPID_Issend(const void *buf,
                                         MPI_Aint count,
                                         MPI_Datatype datatype,
                                         int rank,
                                         int tag,
                                         MPIR_Comm * comm, int attr, MPIR_Request ** request)
{
    MPIR_PT2PT_ATTR_SET_SYNCFLAG(attr);
    return MPID_Isend(buf, count, datatype, rank, tag, comm, attr, request);
}

MPL_STATIC_INLINE_PREFIX int MPID_Cancel_send(MPIR_Request * sreq)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_ENTER;

    if (sreq->comm && MPIR_is_self_comm(sreq->comm)) {
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
