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

static inline int MPIDIG_do_eager_send_new(const void *buf, MPI_Aint count, MPI_Datatype datatype,
                                           MPI_Aint data_sz, int rank, int tag, MPIR_Comm * comm,
                                           int context_offset, MPIDI_av_entry_t * addr,
                                           MPIR_Request ** request, MPIR_Errflag_t errflag,
                                           int msg_handler_id, const void *msg_am_hdr,
                                           size_t msg_am_hdr_sz);
static inline int MPIDIG_do_pipeline_send(const void *buf, MPI_Aint count, MPI_Datatype datatype,
                                          MPI_Aint data_sz, int rank, int tag, MPIR_Comm * comm,
                                          int context_offset, MPIDI_av_entry_t * addr,
                                          MPIR_Request ** request, MPIR_Errflag_t errflag,
                                          int msg_handler_id, const void *msg_am_hdr,
                                          size_t msg_am_hdr_sz);
static inline int MPIDIG_do_rdma_read_send(const void *buf, MPI_Aint count, MPI_Datatype datatype,
                                           MPI_Aint data_sz, int rank, int tag, MPIR_Comm * comm,
                                           int context_offset, MPIDI_av_entry_t * addr,
                                           MPIR_Request ** request, MPIR_Errflag_t errflag,
                                           int msg_handler_id, const void *msg_am_hdr,
                                           size_t msg_am_hdr_sz);
static inline int MPIDIG_isend_impl_new(const void *buf, MPI_Aint count, MPI_Datatype datatype,
                                        int rank, int tag, MPIR_Comm * comm, int context_offset,
                                        MPIDI_av_entry_t * addr, MPIR_Request ** request,
                                        MPIR_Errflag_t errflag, int msg_handler_id,
                                        const void *msg_am_hdr, size_t msg_am_hdr_sz, int protocol);

static inline int MPIDIG_do_ssend_new(const void *buf, MPI_Aint count, MPI_Datatype datatype,
                                      int rank, int tag, MPIR_Comm * comm, int context_offset,
                                      MPIDI_av_entry_t * addr, MPIR_Request ** request,
                                      MPIR_Errflag_t errflag, int protocol);

static inline int MPIDIG_do_ssend_new(const void *buf, MPI_Aint count, MPI_Datatype datatype,
                                      int rank, int tag, MPIR_Comm * comm, int context_offset,
                                      MPIDI_av_entry_t * addr, MPIR_Request ** request,
                                      MPIR_Errflag_t errflag, int protocol)
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
    am_hdr.sreq_ptr = sreq;

#ifdef HAVE_DEBUGGER_SUPPORT
    MPIDIG_REQUEST(sreq, datatype) = datatype;
    MPIDIG_REQUEST(sreq, buffer) = (char *) buf;
    MPIDIG_REQUEST(sreq, count) = count;
#endif

    mpi_errno = MPIDIG_isend_impl_new(buf, count, datatype, rank, tag, comm, context_offset, addr,
                                      request, errflag, MPIDIG_SSEND_REQ, &am_hdr, sizeof(am_hdr),
                                      protocol);

    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    return mpi_errno;

  fn_fail:
    goto fn_exit;
}

static inline int MPIDIG_do_eager_send_new(const void *buf, MPI_Aint count, MPI_Datatype datatype,
                                           MPI_Aint data_sz, int rank, int tag, MPIR_Comm * comm,
                                           int context_offset, MPIDI_av_entry_t * addr,
                                           MPIR_Request ** request, MPIR_Errflag_t errflag,
                                           int msg_handler_id, const void *msg_am_hdr,
                                           size_t msg_am_hdr_sz)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Request *msg_req = NULL;
    MPIR_Request *sreq = NULL;
    MPIDIG_hdr_t am_hdr;
    MPIDIG_hdr_t *carrier_am_hdr = &am_hdr;
    void *send_am_hdr = &am_hdr;
    size_t send_am_hdr_sz = sizeof(MPIDIG_hdr_t);

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIG_DO_EAGER_SEND);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIG_DO_EAGER_SEND);

    if (msg_handler_id != MPIDIG_SEND) {
        /* this is not a non-regular AM send */
        msg_req = *request;
        sreq = *request;
        int c;
        MPIR_cc_incr(sreq->cc_ptr, &c);
        /* allocate a am_hdr buffer large enough for the long req message and the msg am_hdr */
        send_am_hdr_sz = sizeof(MPIDIG_hdr_t) + msg_am_hdr_sz;
        send_am_hdr = MPL_malloc(send_am_hdr_sz, MPL_MEM_OTHER);
        MPIR_ERR_CHKANDSTMT((send_am_hdr) == NULL, mpi_errno, MPIX_ERR_NOREQ, goto fn_fail,
                            "**nomemreq");
        /* set am_hdr for carrier request */
        carrier_am_hdr = (MPIDIG_hdr_t *) send_am_hdr;
        carrier_am_hdr->msg_handler_id = msg_handler_id;
        /* copy msg am_hdr */
        memcpy((char *) send_am_hdr + sizeof(MPIDIG_hdr_t), msg_am_hdr, msg_am_hdr_sz);
        MPIDIG_REQUEST(sreq, req->msg_handler_id) = msg_handler_id;
        MPIDIG_REQUEST(sreq, req->msg_req) = sreq;
    } else {
        /* This is a regular MPIDIG_SEND message, prepare message hdr */
        sreq = *request;
        if (sreq == NULL) {
            sreq = MPIDIG_request_create(MPIR_REQUEST_KIND__SEND, 2);
            MPIR_ERR_CHKANDSTMT((sreq) == NULL, mpi_errno, MPIX_ERR_NOREQ, goto fn_fail,
                                "**nomemreq");
            *request = sreq;
        } else {
            MPIDIG_request_init(sreq, MPIR_REQUEST_KIND__SEND);
        }

        carrier_am_hdr->msg_handler_id = MPIDIG_SEND;
        MPIDIG_REQUEST(sreq, req->msg_handler_id) = MPIDIG_SEND;
        MPIDIG_REQUEST(sreq, req->msg_req) = NULL;
    }

    carrier_am_hdr->src_rank = comm->rank;
    carrier_am_hdr->tag = tag;
    carrier_am_hdr->context_id = comm->context_id + context_offset;
    carrier_am_hdr->error_bits = errflag;
    MPIDIG_REQUEST(sreq, data_sz_left) = data_sz;
    MPIDIG_REQUEST(sreq, offset) = 0;

#ifdef HAVE_DEBUGGER_SUPPORT
    MPIDIG_REQUEST(sreq, datatype) = datatype;
    MPIDIG_REQUEST(sreq, buffer) = (char *) buf;
    MPIDIG_REQUEST(sreq, count) = count;
#endif

#ifndef MPIDI_CH4_DIRECT_NETMOD
    if (msg_req) {
        MPIDI_REQUEST(msg_req, is_local) = MPIDI_av_is_local(addr);
    }
    MPIDI_REQUEST(sreq, is_local) = MPIDI_av_is_local(addr);

    if (MPIDI_av_is_local(addr)) {
        mpi_errno = MPIDI_SHM_am_isend(rank, comm, MPIDIG_SEND, send_am_hdr, send_am_hdr_sz, buf,
                                       count, datatype, sreq);
    } else
#endif
    {
        mpi_errno = MPIDI_NM_am_isend(rank, comm, MPIDIG_SEND, send_am_hdr, send_am_hdr_sz, buf,
                                      count, datatype, sreq);
    }
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    if (msg_handler_id != MPIDIG_SEND) {
        MPL_free(send_am_hdr);
    }
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIG_DO_EAGER_SEND);
    return mpi_errno;

  fn_fail:
    goto fn_exit;

}

static inline int MPIDIG_do_pipeline_send(const void *buf, MPI_Aint count, MPI_Datatype datatype,
                                          MPI_Aint data_sz, int rank, int tag, MPIR_Comm * comm,
                                          int context_offset, MPIDI_av_entry_t * addr,
                                          MPIR_Request ** request, MPIR_Errflag_t errflag,
                                          int msg_handler_id, const void *msg_am_hdr,
                                          size_t msg_am_hdr_sz)
{
    int mpi_errno = MPI_SUCCESS;
    int c;
    MPIR_Request *msg_req = NULL;
    MPIR_Request *sreq = NULL;
    MPIDIG_send_pipeline_rts_msg_t am_hdr;
    MPIDIG_send_pipeline_rts_msg_t *carrier_am_hdr = &am_hdr;
    void *send_am_hdr = &am_hdr;
    size_t send_am_hdr_sz = sizeof(MPIDIG_send_pipeline_rts_msg_t);

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIG_DO_PIPELINE_SEND);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIG_DO_PIPELINE_SEND);

    if (msg_handler_id != MPIDIG_SEND) {
        /* this is not a non-regular AM send, create carrier request and message header */
        msg_req = *request;
        int c;
        /* create a carrier request */
        sreq = MPIDIG_request_create(MPIR_REQUEST_KIND__SEND, 1);
        MPIR_ERR_CHKANDSTMT((sreq) == NULL, mpi_errno, MPIX_ERR_NOREQ, goto fn_fail, "**nomemreq");
        /* allocate a am_hdr buffer large enough for the long req message and the msg am_hdr */
        send_am_hdr_sz = sizeof(MPIDIG_send_pipeline_rts_msg_t) + msg_am_hdr_sz;
        send_am_hdr = MPL_malloc(send_am_hdr_sz, MPL_MEM_OTHER);
        MPIR_ERR_CHKANDSTMT((send_am_hdr) == NULL, mpi_errno, MPIX_ERR_NOREQ, goto fn_fail,
                            "**nomemreq");
        /* set am_hdr for carrier request */
        carrier_am_hdr = (MPIDIG_send_pipeline_rts_msg_t *) send_am_hdr;
        carrier_am_hdr->hdr.msg_handler_id = msg_handler_id;
        /* copy msg am_hdr */
        memcpy((char *) send_am_hdr + sizeof(MPIDIG_send_pipeline_rts_msg_t), msg_am_hdr,
               msg_am_hdr_sz);
        MPIDIG_REQUEST(sreq, req->msg_handler_id) = msg_handler_id;
        MPIDIG_REQUEST(sreq, req->msg_req) = msg_req;
    } else {
        sreq = *request;
        if (sreq == NULL) {
            sreq = MPIDIG_request_create(MPIR_REQUEST_KIND__SEND, 2);
            MPIR_ERR_CHKANDSTMT((sreq) == NULL, mpi_errno, MPIX_ERR_NOREQ, goto fn_fail,
                                "**nomemreq");
            *request = sreq;
        } else {
            MPIDIG_request_init(sreq, MPIR_REQUEST_KIND__SEND);
        }

        carrier_am_hdr->hdr.msg_handler_id = MPIDIG_SEND;
        MPIDIG_REQUEST(sreq, req->msg_handler_id) = MPIDIG_SEND;
        MPIDIG_REQUEST(sreq, req->msg_req) = NULL;
    }

    carrier_am_hdr->hdr.src_rank = comm->rank;
    carrier_am_hdr->hdr.tag = tag;
    carrier_am_hdr->hdr.context_id = comm->context_id + context_offset;
    carrier_am_hdr->hdr.error_bits = errflag;
    carrier_am_hdr->data_sz = data_sz;
    carrier_am_hdr->sreq_ptr = sreq;

    MPIDIG_REQUEST(sreq, req->plreq).src_buf = buf;
    MPIDIG_REQUEST(sreq, req->plreq).count = count;
    MPIR_Datatype_add_ref_if_not_builtin(datatype);
    MPIDIG_REQUEST(sreq, req->plreq).datatype = datatype;
    MPIDIG_REQUEST(sreq, req->plreq).tag = carrier_am_hdr->hdr.tag;
    MPIDIG_REQUEST(sreq, req->plreq).rank = carrier_am_hdr->hdr.src_rank;
    MPIDIG_REQUEST(sreq, req->plreq).context_id = carrier_am_hdr->hdr.context_id;
    MPIDIG_REQUEST(sreq, req->plreq).seg_next = 1;      /* seg 0 is piggybacked on RTS */
    MPIDIG_REQUEST(sreq, data_sz_left) = data_sz;
    MPIDIG_REQUEST(sreq, offset) = 0;
    MPIDIG_REQUEST(sreq, rank) = rank;

    MPIR_cc_incr(sreq->cc_ptr, &c);     /* expect the completion after all segs are sent */

#ifdef HAVE_DEBUGGER_SUPPORT
    MPIDIG_REQUEST(sreq, datatype) = datatype;
    MPIDIG_REQUEST(sreq, buffer) = (char *) buf;
    MPIDIG_REQUEST(sreq, count) = count;
#endif

    MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_GENERAL, VERBOSE,
                    (MPL_DBG_FDEST, "pipeline send parent req handle=0x%x", sreq->handle));

#ifndef MPIDI_CH4_DIRECT_NETMOD
    if (msg_req) {
        MPIDI_REQUEST(msg_req, is_local) = MPIDI_av_is_local(addr);
    }
    MPIDI_REQUEST(sreq, is_local) = MPIDI_av_is_local(addr);

    if (MPIDI_av_is_local(addr)) {
        MPIR_cc_incr(sreq->cc_ptr, &c); /* for the same request is used for segement on SHM path */
        mpi_errno = MPIDI_SHM_am_isend_pipeline_rts(rank, comm, MPIDIG_SEND_PIPELINE_RTS,
                                                    send_am_hdr, send_am_hdr_sz, buf, count,
                                                    datatype, sreq);
    } else
#endif
    {
        mpi_errno = MPIDI_NM_am_isend_pipeline_rts(rank, comm, MPIDIG_SEND_PIPELINE_RTS,
                                                   send_am_hdr, send_am_hdr_sz, buf, count,
                                                   datatype, sreq);
    }
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    if (msg_handler_id != MPIDIG_SEND) {
        MPL_free(send_am_hdr);
    }
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIG_DO_PIPELINE_SEND);
    return mpi_errno;

  fn_fail:
    goto fn_exit;
}

static inline int MPIDIG_do_rdma_read_send(const void *buf, MPI_Aint count, MPI_Datatype datatype,
                                           MPI_Aint data_sz, int rank, int tag, MPIR_Comm * comm,
                                           int context_offset, MPIDI_av_entry_t * addr,
                                           MPIR_Request ** request, MPIR_Errflag_t errflag,
                                           int msg_handler_id, const void *msg_am_hdr,
                                           size_t msg_am_hdr_sz)
{
    int mpi_errno = MPI_SUCCESS;
    int c;
    MPIR_Request *msg_req = NULL;
    MPIR_Request *sreq = NULL;
    MPIDIG_send_rdma_read_req_msg_t am_hdr;
    MPIDIG_send_rdma_read_req_msg_t *carrier_am_hdr = &am_hdr;
    void *send_am_hdr = &am_hdr;
    size_t send_am_hdr_sz = sizeof(MPIDIG_send_rdma_read_req_msg_t);

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIG_DO_RDMA_READ_SEND);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIG_DO_RDMA_READ_SEND);

    if (msg_handler_id != MPIDIG_SEND) {
        /* this is not a non-regular AM send, create carrier request and message header */
        msg_req = *request;
        int c;
        /* create a carrier request */
        sreq = MPIDIG_request_create(MPIR_REQUEST_KIND__SEND, 1);
        MPIR_ERR_CHKANDSTMT((sreq) == NULL, mpi_errno, MPIX_ERR_NOREQ, goto fn_fail, "**nomemreq");
        /* allocate a am_hdr buffer large enough for the long req message and the msg am_hdr */
        send_am_hdr_sz = sizeof(MPIDIG_send_rdma_read_req_msg_t) + msg_am_hdr_sz;
        send_am_hdr = MPL_malloc(send_am_hdr_sz, MPL_MEM_OTHER);
        MPIR_ERR_CHKANDSTMT((send_am_hdr) == NULL, mpi_errno, MPIX_ERR_NOREQ, goto fn_fail,
                            "**nomemreq");
        /* set am_hdr for carrier request */
        carrier_am_hdr = (MPIDIG_send_rdma_read_req_msg_t *) send_am_hdr;
        carrier_am_hdr->hdr.msg_handler_id = msg_handler_id;
        /* copy msg am_hdr */
        memcpy((char *) send_am_hdr + sizeof(MPIDIG_send_rdma_read_req_msg_t), msg_am_hdr,
               msg_am_hdr_sz);
        MPIDIG_REQUEST(sreq, req->msg_handler_id) = msg_handler_id;
        MPIDIG_REQUEST(sreq, req->msg_req) = msg_req;
    } else {
        sreq = *request;
        if (sreq == NULL) {
            sreq = MPIDIG_request_create(MPIR_REQUEST_KIND__SEND, 2);
            MPIR_ERR_CHKANDSTMT((sreq) == NULL, mpi_errno, MPIX_ERR_NOREQ, goto fn_fail,
                                "**nomemreq");
            *request = sreq;
        } else {
            MPIDIG_request_init(sreq, MPIR_REQUEST_KIND__SEND);
        }

        carrier_am_hdr->hdr.msg_handler_id = MPIDIG_SEND;
        MPIDIG_REQUEST(sreq, req->msg_handler_id) = MPIDIG_SEND;
        MPIDIG_REQUEST(sreq, req->msg_req) = NULL;
    }

    carrier_am_hdr->hdr.src_rank = comm->rank;
    carrier_am_hdr->hdr.tag = tag;
    carrier_am_hdr->hdr.context_id = comm->context_id + context_offset;
    carrier_am_hdr->hdr.error_bits = errflag;
    carrier_am_hdr->data_sz = data_sz;
    carrier_am_hdr->sreq_ptr = sreq;

    MPIDIG_REQUEST(sreq, req->rdr_req).src_buf = buf;
    MPIDIG_REQUEST(sreq, req->rdr_req).count = count;
    MPIR_Datatype_add_ref_if_not_builtin(datatype);
    MPIDIG_REQUEST(sreq, req->rdr_req).datatype = datatype;
    MPIDIG_REQUEST(sreq, req->rdr_req).tag = carrier_am_hdr->hdr.tag;
    MPIDIG_REQUEST(sreq, req->rdr_req).rank = carrier_am_hdr->hdr.src_rank;
    MPIDIG_REQUEST(sreq, req->rdr_req).context_id = carrier_am_hdr->hdr.context_id;
    MPIDIG_REQUEST(sreq, data_sz_left) = data_sz;
    MPIDIG_REQUEST(sreq, offset) = 0;
    MPIDIG_REQUEST(sreq, rank) = rank;

    MPIR_cc_incr(sreq->cc_ptr, &c);     /* expect ACK or NAK */

#ifdef HAVE_DEBUGGER_SUPPORT
    MPIDIG_REQUEST(sreq, datatype) = datatype;
    MPIDIG_REQUEST(sreq, buffer) = (char *) buf;
    MPIDIG_REQUEST(sreq, count) = count;
#endif

#ifndef MPIDI_CH4_DIRECT_NETMOD
    if (msg_req) {
        MPIDI_REQUEST(msg_req, is_local) = MPIDI_av_is_local(addr);
    }
    MPIDI_REQUEST(sreq, is_local) = MPIDI_av_is_local(addr);

#endif
    mpi_errno = MPIDI_NM_am_isend_rdma_read_req(rank, comm, MPIDIG_SEND_RDMA_READ_REQ,
                                                send_am_hdr, send_am_hdr_sz, buf, count,
                                                datatype, sreq);
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    if (msg_handler_id != MPIDIG_SEND) {
        MPL_free(send_am_hdr);
    }
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIG_DO_RDMA_READ_SEND);
    return mpi_errno;

  fn_fail:
    goto fn_exit;
}

/* this is the new entry point for sending message. The _new prefix will be removed later */
static inline int MPIDIG_isend_impl_new(const void *buf, MPI_Aint count, MPI_Datatype datatype,
                                        int rank, int tag, MPIR_Comm * comm, int context_offset,
                                        MPIDI_av_entry_t * addr, MPIR_Request ** request,
                                        MPIR_Errflag_t errflag, int msg_handler_id,
                                        const void *msg_am_hdr, size_t msg_am_hdr_sz, int protocol)
{
    int mpi_errno = MPI_SUCCESS;
    MPI_Aint data_sz;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIG_ISEND_IMPL);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIG_ISEND_IMPL);

    MPIDI_Datatype_check_size(datatype, count, data_sz);

    switch (protocol) {
        case MPIDIG_AM_PROTOCOL__EAGER:
            mpi_errno = MPIDIG_do_eager_send_new(buf, count, datatype, data_sz, rank, tag, comm,
                                                 context_offset, addr, request, errflag,
                                                 msg_handler_id, msg_am_hdr, msg_am_hdr_sz);
            break;
        case MPIDIG_AM_PROTOCOL__PIPELINE:
            mpi_errno = MPIDIG_do_pipeline_send(buf, count, datatype, data_sz, rank, tag, comm,
                                                context_offset, addr, request, errflag,
                                                msg_handler_id, msg_am_hdr, msg_am_hdr_sz);
            break;
        case MPIDIG_AM_PROTOCOL__RDMA_READ:
            mpi_errno = MPIDIG_do_rdma_read_send(buf, count, datatype, data_sz, rank, tag, comm,
                                                 context_offset, addr, request, errflag,
                                                 msg_handler_id, msg_am_hdr, msg_am_hdr_sz);
            break;
        default:
            MPIR_Assert(0);
    }
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIG_ISEND_IMPL);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPIDIG_mpi_send_new(const void *buf, MPI_Aint count,
                                                 MPI_Datatype datatype, int rank,
                                                 int tag, MPIR_Comm * comm, int context_offset,
                                                 MPIDI_av_entry_t * addr, MPIR_Request ** request,
                                                 int protocol)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIG_MPI_SEND);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIG_MPI_SEND);
    MPID_THREAD_CS_ENTER(VCI, MPIDI_VCI(0).lock);

    mpi_errno = MPIDIG_isend_impl_new(buf, count, datatype, rank, tag, comm, context_offset, addr,
                                      request, MPIR_ERR_NONE, MPIDIG_SEND, NULL, 0, protocol);

    MPID_THREAD_CS_EXIT(VCI, MPIDI_VCI(0).lock);
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIG_MPI_SEND);
    return mpi_errno;
}

MPL_STATIC_INLINE_PREFIX int MPIDIG_send_coll_new(const void *buf, MPI_Aint count,
                                                  MPI_Datatype datatype, int rank,
                                                  int tag, MPIR_Comm * comm, int context_offset,
                                                  MPIDI_av_entry_t * addr, MPIR_Request ** request,
                                                  MPIR_Errflag_t * errflag, int protocol)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIG_SEND_COLL);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIG_SEND_COLL);
    MPID_THREAD_CS_ENTER(VCI, MPIDI_VCI(0).lock);

    mpi_errno = MPIDIG_isend_impl_new(buf, count, datatype, rank, tag, comm, context_offset, addr,
                                      request, *errflag, MPIDIG_SEND, NULL, 0, protocol);

    MPID_THREAD_CS_EXIT(VCI, MPIDI_VCI(0).lock);
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIG_SEND_COLL);
    return mpi_errno;
}

MPL_STATIC_INLINE_PREFIX int MPIDIG_mpi_isend_new(const void *buf, MPI_Aint count,
                                                  MPI_Datatype datatype, int rank,
                                                  int tag, MPIR_Comm * comm, int context_offset,
                                                  MPIDI_av_entry_t * addr, MPIR_Request ** request,
                                                  int protocol)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIG_MPI_ISEND);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIG_MPI_ISEND);
    MPID_THREAD_CS_ENTER(VCI, MPIDI_VCI(0).lock);

    mpi_errno = MPIDIG_isend_impl_new(buf, count, datatype, rank, tag, comm, context_offset, addr,
                                      request, MPIR_ERR_NONE, MPIDIG_SEND, NULL, 0, protocol);

    MPID_THREAD_CS_EXIT(VCI, MPIDI_VCI(0).lock);
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIG_MPI_ISEND);
    return mpi_errno;
}


MPL_STATIC_INLINE_PREFIX int MPIDIG_isend_coll_new(const void *buf, MPI_Aint count,
                                                   MPI_Datatype datatype, int rank,
                                                   int tag, MPIR_Comm * comm, int context_offset,
                                                   MPIDI_av_entry_t * addr, MPIR_Request ** request,
                                                   MPIR_Errflag_t * errflag, int protocol)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIG_ISEND_COLL);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIG_ISEND_COLL);
    MPID_THREAD_CS_ENTER(VCI, MPIDI_VCI(0).lock);

    mpi_errno = MPIDIG_isend_impl_new(buf, count, datatype, rank, tag, comm, context_offset, addr,
                                      request, *errflag, MPIDIG_SEND, NULL, 0, protocol);

    MPID_THREAD_CS_EXIT(VCI, MPIDI_VCI(0).lock);
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIG_ISEND_COLL);
    return mpi_errno;
}


MPL_STATIC_INLINE_PREFIX int MPIDIG_mpi_rsend_new(const void *buf, MPI_Aint count,
                                                  MPI_Datatype datatype, int rank,
                                                  int tag, MPIR_Comm * comm, int context_offset,
                                                  MPIDI_av_entry_t * addr, MPIR_Request ** request,
                                                  int protocol)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIG_MPI_RSEND);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIG_MPI_RSEND);
    MPID_THREAD_CS_ENTER(VCI, MPIDI_VCI(0).lock);

    mpi_errno = MPIDIG_isend_impl_new(buf, count, datatype, rank, tag, comm, context_offset, addr,
                                      request, MPIR_ERR_NONE, MPIDIG_SEND, NULL, 0, protocol);

    MPID_THREAD_CS_EXIT(VCI, MPIDI_VCI(0).lock);
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIG_MPI_RSEND);
    return mpi_errno;
}


MPL_STATIC_INLINE_PREFIX int MPIDIG_mpi_irsend_new(const void *buf, MPI_Aint count,
                                                   MPI_Datatype datatype, int rank,
                                                   int tag, MPIR_Comm * comm, int context_offset,
                                                   MPIDI_av_entry_t * addr, MPIR_Request ** request,
                                                   int protocol)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIG_MPI_IRSEND);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIG_MPI_IRSEND);
    MPID_THREAD_CS_ENTER(VCI, MPIDI_VCI(0).lock);

    mpi_errno = MPIDIG_isend_impl_new(buf, count, datatype, rank, tag, comm, context_offset, addr,
                                      request, MPIR_ERR_NONE, MPIDIG_SEND, NULL, 0, protocol);

    MPID_THREAD_CS_EXIT(VCI, MPIDI_VCI(0).lock);
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIG_MPI_IRSEND);
    return mpi_errno;
}

MPL_STATIC_INLINE_PREFIX int MPIDIG_mpi_ssend_new(const void *buf, MPI_Aint count,
                                                  MPI_Datatype datatype, int rank,
                                                  int tag, MPIR_Comm * comm, int context_offset,
                                                  MPIDI_av_entry_t * addr, MPIR_Request ** request,
                                                  int protocol)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIG_MPI_SSEND);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIG_MPI_SSEND);
    MPID_THREAD_CS_ENTER(VCI, MPIDI_VCI(0).lock);

    mpi_errno = MPIDIG_do_ssend_new(buf, count, datatype, rank, tag, comm, context_offset, addr,
                                    request, MPIR_ERR_NONE, protocol);

    MPID_THREAD_CS_EXIT(VCI, MPIDI_VCI(0).lock);
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIG_MPI_SSEND);
    return mpi_errno;
}

MPL_STATIC_INLINE_PREFIX int MPIDIG_mpi_issend_new(const void *buf, MPI_Aint count,
                                                   MPI_Datatype datatype, int rank,
                                                   int tag, MPIR_Comm * comm, int context_offset,
                                                   MPIDI_av_entry_t * addr, MPIR_Request ** request,
                                                   int protocol)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIG_MPI_ISSEND);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIG_MPI_ISSEND);
    MPID_THREAD_CS_ENTER(VCI, MPIDI_VCI(0).lock);

    mpi_errno = MPIDIG_do_ssend_new(buf, count, datatype, rank, tag, comm, context_offset, addr,
                                    request, MPIR_ERR_NONE, protocol);

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
