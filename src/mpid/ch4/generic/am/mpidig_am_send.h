/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef MPIDIG_AM_SEND_H_INCLUDED
#define MPIDIG_AM_SEND_H_INCLUDED

#include "ch4_impl.h"

/*
=== BEGIN_MPI_T_CVAR_INFO_BLOCK ===

cvars:
    - name        : MPIR_CVAR_CH4_EAGER_MAX_MSG_SIZE
      category    : CH4
      type        : int
      default     : -1
      class       : device
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_LOCAL
      description : >-
        If set to positive number, this cvar controls the message size at which CH4 switches from eager to rendezvous mode.
        If the number is negative, underlying netmod or shmmod automatically uses an optimal number depending on
        the underlying fabric or shared memory architecture.

=== END_MPI_T_CVAR_INFO_BLOCK ===
*/

#define MPIDIG_AM_SEND_HDR_SIZE  sizeof(MPIDIG_hdr_t)

MPL_STATIC_INLINE_PREFIX int mpidig_eager_limit(int is_local)
{
    if (MPIR_CVAR_CH4_EAGER_MAX_MSG_SIZE > 0) {
        return MPIR_CVAR_CH4_EAGER_MAX_MSG_SIZE;
    }

    int thresh;
#ifdef MPIDI_CH4_DIRECT_NETMOD
    thresh = MPIDI_NM_am_eager_limit();
#else
    if (is_local) {
        thresh = MPIDI_SHM_am_eager_limit();
    } else {
        thresh = MPIDI_NM_am_eager_limit();
    }
#endif
    thresh -= MPIDIG_AM_SEND_HDR_SIZE;
    MPIR_Assert(thresh > 0);

    return thresh;
}

MPL_STATIC_INLINE_PREFIX int MPIDIG_do_eager_send(const void *buf, MPI_Aint count,
                                                  MPI_Datatype datatype, int rank, int tag,
                                                  MPIR_Comm * comm, int context_offset,
                                                  MPIDI_av_entry_t * addr, MPIR_Request ** request,
                                                  MPIR_Errflag_t errflag);
MPL_STATIC_INLINE_PREFIX int MPIDIG_do_rndv_send(const void *buf, MPI_Aint count,
                                                 MPI_Datatype datatype, MPI_Aint data_sz, int rank,
                                                 int tag, MPIR_Comm * comm, int context_offset,
                                                 MPIDI_av_entry_t * addr, MPIR_Request ** request,
                                                 MPIR_Errflag_t errflag);
MPL_STATIC_INLINE_PREFIX int MPIDIG_do_ssend(const void *buf, MPI_Aint count, MPI_Datatype datatype,
                                             int rank, int tag, MPIR_Comm * comm,
                                             int context_offset, MPIDI_av_entry_t * addr,
                                             MPIR_Request ** request, MPIR_Errflag_t errflag);

MPL_STATIC_INLINE_PREFIX int MPIDIG_isend_impl(const void *buf, MPI_Aint count,
                                               MPI_Datatype datatype, int rank, int tag,
                                               MPIR_Comm * comm, int context_offset,
                                               MPIDI_av_entry_t * addr, MPIR_Request ** request,
                                               int type, MPIR_Errflag_t errflag)
{
    MPI_Aint data_sz;
    MPIR_Datatype *dt_ptr;
    MPIDI_Datatype_get_size_dt_ptr(count, datatype, data_sz, dt_ptr);

    int is_local = MPIDI_av_is_local(addr);
    if (type == MPIDIG_SSEND_REQ) {
        return MPIDIG_do_ssend(buf, count, datatype, rank, tag, comm, context_offset,
                               addr, request, errflag);
    } else if (data_sz > mpidig_eager_limit(is_local)) {
        return MPIDIG_do_rndv_send(buf, count, datatype, data_sz, rank, tag, comm, context_offset,
                                   addr, request, errflag);
    } else {
        return MPIDIG_do_eager_send(buf, count, datatype, rank, tag, comm, context_offset,
                                    addr, request, errflag);
    }
}

MPL_STATIC_INLINE_PREFIX int MPIDIG_do_eager_send(const void *buf, MPI_Aint count,
                                                  MPI_Datatype datatype, int rank, int tag,
                                                  MPIR_Comm * comm, int context_offset,
                                                  MPIDI_av_entry_t * addr, MPIR_Request ** request,
                                                  MPIR_Errflag_t errflag)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Request *sreq = *request;

    if (sreq == NULL) {
        sreq = MPIDIG_request_create(MPIR_REQUEST_KIND__SEND, 2);
        MPIR_ERR_CHKANDSTMT((sreq) == NULL, mpi_errno, MPIX_ERR_NOREQ, goto fn_fail, "**nomemreq");
    } else {
        MPIDIG_request_init(sreq, MPIR_REQUEST_KIND__SEND);
    }

    *request = sreq;

    MPIDIG_hdr_t am_hdr;
    am_hdr.src_rank = comm->rank;
    am_hdr.tag = tag;
    am_hdr.context_id = comm->context_id + context_offset;
    am_hdr.error_bits = errflag;

#ifdef HAVE_DEBUGGER_SUPPORT
    MPIDIG_REQUEST(sreq, datatype) = datatype;
    MPIDIG_REQUEST(sreq, buffer) = (char *) buf;
    MPIDIG_REQUEST(sreq, count) = count;
#endif

#ifndef MPIDI_CH4_DIRECT_NETMOD
    if (MPIDI_av_is_local(addr)) {
        mpi_errno = MPIDI_SHM_am_isend(rank, comm, MPIDIG_SEND, &am_hdr, sizeof(am_hdr),
                                       buf, count, datatype, sreq);
    } else
#endif
    {
        mpi_errno = MPIDI_NM_am_isend(rank, comm, MPIDIG_SEND, &am_hdr, sizeof(am_hdr),
                                      buf, count, datatype, sreq);
    }
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    return mpi_errno;

  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPIDIG_do_ssend(const void *buf, MPI_Aint count, MPI_Datatype datatype,
                                             int rank, int tag, MPIR_Comm * comm,
                                             int context_offset, MPIDI_av_entry_t * addr,
                                             MPIR_Request ** request, MPIR_Errflag_t errflag)
{
    int mpi_errno = MPI_SUCCESS, c;
    MPIR_Request *sreq = *request;

    if (sreq == NULL) {
        sreq = MPIDIG_request_create(MPIR_REQUEST_KIND__SEND, 2);
        MPIR_ERR_CHKANDSTMT((sreq) == NULL, mpi_errno, MPIX_ERR_NOREQ, goto fn_fail, "**nomemreq");
    } else {
        MPIDIG_request_init(sreq, MPIR_REQUEST_KIND__SEND);
    }

    *request = sreq;

    MPIDIG_ssend_req_msg_t am_hdr;
    am_hdr.hdr.src_rank = comm->rank;
    am_hdr.hdr.tag = tag;
    am_hdr.hdr.context_id = comm->context_id + context_offset;
    am_hdr.hdr.error_bits = errflag;
    am_hdr.sreq_ptr = sreq;

#ifdef HAVE_DEBUGGER_SUPPORT
    MPIDIG_REQUEST(sreq, datatype) = datatype;
    MPIDIG_REQUEST(sreq, buffer) = (char *) buf;
    MPIDIG_REQUEST(sreq, count) = count;
#endif

    /* Increment the completion counter once to account for the extra message that needs to come
     * back from the receiver to indicate completion. */
    MPIR_cc_incr(sreq->cc_ptr, &c);

#ifndef MPIDI_CH4_DIRECT_NETMOD
    if (MPIDI_av_is_local(addr)) {
        mpi_errno = MPIDI_SHM_am_isend(rank, comm, MPIDIG_SSEND_REQ, &am_hdr, sizeof(am_hdr),
                                       buf, count, datatype, sreq);
    } else
#endif
    {
        mpi_errno = MPIDI_NM_am_isend(rank, comm, MPIDIG_SSEND_REQ, &am_hdr, sizeof(am_hdr),
                                      buf, count, datatype, sreq);
    }
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    return mpi_errno;

  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPIDIG_do_rndv_send(const void *buf, MPI_Aint count,
                                                 MPI_Datatype datatype, MPI_Aint data_sz, int rank,
                                                 int tag, MPIR_Comm * comm, int context_offset,
                                                 MPIDI_av_entry_t * addr, MPIR_Request ** request,
                                                 MPIR_Errflag_t errflag)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Request *sreq = *request;

    if (sreq == NULL) {
        sreq = MPIDIG_request_create(MPIR_REQUEST_KIND__SEND, 2);
        MPIR_ERR_CHKANDSTMT((sreq) == NULL, mpi_errno, MPIX_ERR_NOREQ, goto fn_fail, "**nomemreq");
    } else {
        MPIDIG_request_init(sreq, MPIR_REQUEST_KIND__SEND);
    }

    *request = sreq;

    MPIDIG_send_long_req_msg_t am_hdr;
    am_hdr.hdr.src_rank = comm->rank;
    am_hdr.hdr.tag = tag;
    am_hdr.hdr.context_id = comm->context_id + context_offset;
    am_hdr.hdr.error_bits = errflag;
    am_hdr.data_sz = data_sz;
    am_hdr.sreq_ptr = sreq;
    MPIDIG_REQUEST(sreq, req->lreq).src_buf = buf;
    MPIDIG_REQUEST(sreq, req->lreq).count = count;
    MPIR_Datatype_add_ref_if_not_builtin(datatype);
    MPIDIG_REQUEST(sreq, req->lreq).datatype = datatype;
    MPIDIG_REQUEST(sreq, req->lreq).tag = am_hdr.hdr.tag;
    MPIDIG_REQUEST(sreq, req->lreq).rank = am_hdr.hdr.src_rank;
    MPIDIG_REQUEST(sreq, req->lreq).context_id = am_hdr.hdr.context_id;
    MPIDIG_REQUEST(sreq, rank) = rank;

#ifdef HAVE_DEBUGGER_SUPPORT
    MPIDIG_REQUEST(sreq, datatype) = datatype;
    MPIDIG_REQUEST(sreq, buffer) = (char *) buf;
    MPIDIG_REQUEST(sreq, count) = count;
#endif

#ifndef MPIDI_CH4_DIRECT_NETMOD
    if (MPIDI_av_is_local(addr)) {
        mpi_errno = MPIDI_SHM_am_send_hdr(rank, comm, MPIDIG_SEND_LONG_REQ,
                                          &am_hdr, sizeof(am_hdr));
    } else
#endif
    {
        mpi_errno = MPIDI_NM_am_send_hdr(rank, comm, MPIDIG_SEND_LONG_REQ, &am_hdr, sizeof(am_hdr));
    }
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    return mpi_errno;

  fn_fail:
    goto fn_exit;
}

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
    MPID_THREAD_CS_ENTER(VCI, MPIDI_VCI(0).lock);

    mpi_errno = MPIDIG_isend_impl(buf, count, datatype, rank, tag, comm, context_offset, addr,
                                  request, MPIDIG_SEND, MPIR_ERR_NONE);

    MPID_THREAD_CS_EXIT(VCI, MPIDI_VCI(0).lock);
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIG_MPI_SEND);
    return mpi_errno;
}

MPL_STATIC_INLINE_PREFIX int MPIDIG_send_coll(const void *buf, MPI_Aint count,
                                              MPI_Datatype datatype, int rank,
                                              int tag, MPIR_Comm * comm, int context_offset,
                                              MPIDI_av_entry_t * addr, MPIR_Request ** request,
                                              MPIR_Errflag_t * errflag)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIG_SEND_COLL);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIG_SEND_COLL);
    MPID_THREAD_CS_ENTER(VCI, MPIDI_VCI(0).lock);

    mpi_errno =
        MPIDIG_isend_impl(buf, count, datatype, rank, tag, comm, context_offset, addr, request,
                          MPIDIG_SEND, *errflag);

    MPID_THREAD_CS_EXIT(VCI, MPIDI_VCI(0).lock);
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIG_SEND_COLL);
    return mpi_errno;
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

    mpi_errno = MPIDIG_isend_impl(buf, count, datatype, rank, tag, comm, context_offset, addr,
                                  request, MPIDIG_SEND, MPIR_ERR_NONE);

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
        MPIDIG_isend_impl(buf, count, datatype, rank, tag, comm, context_offset, addr, request,
                          MPIDIG_SEND, *errflag);

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

    mpi_errno = MPIDIG_isend_impl(buf, count, datatype, rank, tag, comm, context_offset, addr,
                                  request, MPIDIG_SEND, MPIR_ERR_NONE);

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

    mpi_errno = MPIDIG_isend_impl(buf, count, datatype, rank, tag, comm, context_offset, addr,
                                  request, MPIDIG_SEND, MPIR_ERR_NONE);

    MPID_THREAD_CS_EXIT(VCI, MPIDI_VCI(0).lock);
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIG_MPI_IRSEND);
    return mpi_errno;
}

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
    MPID_THREAD_CS_ENTER(VCI, MPIDI_VCI(0).lock);

    mpi_errno = MPIDIG_isend_impl(buf, count, datatype, rank, tag, comm, context_offset, addr,
                                  request, MPIDIG_SSEND_REQ, MPIR_ERR_NONE);

    MPID_THREAD_CS_EXIT(VCI, MPIDI_VCI(0).lock);
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIG_MPI_SSEND);
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

    mpi_errno = MPIDIG_isend_impl(buf, count, datatype, rank, tag, comm, context_offset, addr,
                                  request, MPIDIG_SSEND_REQ, MPIR_ERR_NONE);

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
