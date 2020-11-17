/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef MPIDIG_AM_SEND_H_INCLUDED
#define MPIDIG_AM_SEND_H_INCLUDED

#include "ch4_impl.h"

#define MPIDIG_AM_SEND_FLAGS_NONE (0)
#define MPIDIG_AM_SEND_FLAGS_SYNC (1)
#define MPIDIG_AM_SEND_FLAGS_RTS (1 << 1)

MPL_STATIC_INLINE_PREFIX bool MPIDIG_check_eager(int is_local, MPI_Aint am_hdr_sz, MPI_Aint data_sz,
                                                 const void *buf, MPI_Count count,
                                                 MPI_Datatype datatype, MPIR_Request * sreq)
{
#ifdef MPIDI_CH4_DIRECT_NETMOD
    return MPIDI_NM_am_check_eager(am_hdr_sz, data_sz, buf, count, datatype, sreq);
#else
    if (is_local) {
        return MPIDI_SHM_am_check_eager(am_hdr_sz, data_sz, buf, count, datatype, sreq);
    } else {
        return MPIDI_NM_am_check_eager(am_hdr_sz, data_sz, buf, count, datatype, sreq);
    }
#endif
}

MPL_STATIC_INLINE_PREFIX int MPIDIG_isend_impl(const void *buf, MPI_Aint count,
                                               MPI_Datatype datatype, int rank, int tag,
                                               MPIR_Comm * comm, int context_offset,
                                               MPIDI_av_entry_t * addr, uint8_t flags,
                                               MPIR_Request ** request, MPIR_Errflag_t errflag)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Request *sreq = *request;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIG_ISEND_IMPL);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIG_ISEND_IMPL);

    if (sreq == NULL) {
        sreq = MPIDIG_request_create(MPIR_REQUEST_KIND__SEND, 2);
        MPIR_ERR_CHKANDSTMT((sreq) == NULL, mpi_errno, MPIX_ERR_NOREQ, goto fn_fail, "**nomemreq");
        *request = sreq;
    } else {
        MPIDIG_request_init(sreq, MPIR_REQUEST_KIND__SEND);
    }

    MPIDIG_hdr_t am_hdr;
    am_hdr.src_rank = comm->rank;
    am_hdr.tag = tag;
    am_hdr.context_id = comm->context_id + context_offset;
    am_hdr.error_bits = errflag;
    am_hdr.sreq_ptr = sreq;
    am_hdr.flags = flags;
    if (flags & MPIDIG_AM_SEND_FLAGS_SYNC) {
        int c;
        MPIR_cc_incr(sreq->cc_ptr, &c); /* expecting SSEND_ACK */
    }
    MPI_Aint data_sz;
    MPIDI_Datatype_check_size(datatype, count, data_sz);
    am_hdr.data_sz = data_sz;

#ifdef HAVE_DEBUGGER_SUPPORT
    MPIDIG_REQUEST(sreq, datatype) = datatype;
    MPIDIG_REQUEST(sreq, buffer) = (char *) buf;
    MPIDIG_REQUEST(sreq, count) = count;
#endif

    int is_local = MPIDI_av_is_local(addr);
    if (MPIDIG_check_eager(is_local, sizeof(am_hdr), data_sz, buf, count, datatype, sreq)) {
        /* EAGER send */
#ifndef MPIDI_CH4_DIRECT_NETMOD
        if (is_local) {
            mpi_errno = MPIDI_SHM_am_isend(rank, comm, MPIDIG_SEND, &am_hdr, sizeof(am_hdr), buf,
                                           count, datatype, sreq);
        } else
#endif
        {
            mpi_errno = MPIDI_NM_am_isend(rank, comm, MPIDIG_SEND, &am_hdr, sizeof(am_hdr), buf,
                                          count, datatype, sreq);
        }
    } else {
        /* RNDV send */
        MPIDIG_REQUEST(sreq, req->sreq).src_buf = buf;
        MPIDIG_REQUEST(sreq, req->sreq).count = count;
        MPIDIG_REQUEST(sreq, req->sreq).datatype = datatype;
        MPIDIG_REQUEST(sreq, req->sreq).rank = am_hdr.src_rank;
        MPIDIG_REQUEST(sreq, req->sreq).context_id = am_hdr.context_id;
        MPIDIG_REQUEST(sreq, rank) = rank;
        MPIR_Datatype_add_ref_if_not_builtin(datatype);
        am_hdr.flags |= MPIDIG_AM_SEND_FLAGS_RTS;

#ifndef MPIDI_CH4_DIRECT_NETMOD
        if (is_local) {
            mpi_errno = MPIDI_SHM_am_send_hdr(rank, comm, MPIDIG_SEND, &am_hdr, sizeof(am_hdr));
        } else
#endif
        {
            mpi_errno = MPIDI_NM_am_send_hdr(rank, comm, MPIDIG_SEND, &am_hdr, sizeof(am_hdr));
        }
    }
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIG_ISEND_IMPL);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
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
    MPID_THREAD_CS_ENTER(VCI, MPIDI_VCI(0).lock);

    mpi_errno =
        MPIDIG_isend_impl(buf, count, datatype, rank, tag, comm, context_offset, addr,
                          MPIDIG_AM_SEND_FLAGS_NONE, request, MPIR_ERR_NONE);

    MPID_THREAD_CS_EXIT(VCI, MPIDI_VCI(0).lock);
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
    MPID_THREAD_CS_ENTER(VCI, MPIDI_VCI(0).lock);

    mpi_errno =
        MPIDIG_isend_impl(buf, count, datatype, rank, tag, comm, context_offset, addr,
                          MPIDIG_AM_SEND_FLAGS_NONE, request, *errflag);

    MPID_THREAD_CS_EXIT(VCI, MPIDI_VCI(0).lock);
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
    MPID_THREAD_CS_ENTER(VCI, MPIDI_VCI(0).lock);

    mpi_errno =
        MPIDIG_isend_impl(buf, count, datatype, rank, tag, comm, context_offset, addr,
                          MPIDIG_AM_SEND_FLAGS_NONE, request, MPIR_ERR_NONE);

    MPID_THREAD_CS_EXIT(VCI, MPIDI_VCI(0).lock);
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
    MPID_THREAD_CS_ENTER(VCI, MPIDI_VCI(0).lock);

    mpi_errno =
        MPIDIG_isend_impl(buf, count, datatype, rank, tag, comm, context_offset, addr,
                          MPIDIG_AM_SEND_FLAGS_NONE, request, MPIR_ERR_NONE);

    MPID_THREAD_CS_EXIT(VCI, MPIDI_VCI(0).lock);
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIG_MPI_IRSEND);
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
    MPID_THREAD_CS_ENTER(VCI, MPIDI_VCI(0).lock);

    mpi_errno =
        MPIDIG_isend_impl(buf, count, datatype, rank, tag, comm, context_offset, addr,
                          MPIDIG_AM_SEND_FLAGS_SYNC, request, MPIR_ERR_NONE);

    MPID_THREAD_CS_EXIT(VCI, MPIDI_VCI(0).lock);
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIG_MPI_ISSEND);
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

#endif /* MPIDIG_AM_SEND_H_INCLUDED */
