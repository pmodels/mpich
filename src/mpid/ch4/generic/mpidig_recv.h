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

#ifndef MPIDIG_RECV_H_INCLUDED
#define MPIDIG_RECV_H_INCLUDED

#include "ch4_impl.h"
#include "ch4r_proc.h"
#include "ch4r_recv.h"

#undef FUNCNAME
#define FUNCNAME MPIDI_prepare_recv_req
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_prepare_recv_req(void *buf, int count, MPI_Datatype datatype,
                                         MPIR_Request * rreq)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_PREPARE_RECV_REQ);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_PREPARE_RECV_REQ);

    MPIDI_CH4U_REQUEST(rreq, datatype) = datatype;
    MPIDI_CH4U_REQUEST(rreq, buffer) = (char *) buf;
    MPIDI_CH4U_REQUEST(rreq, count) = count;

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_PREPARE_RECV_REQ);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_handle_unexpected
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_handle_unexpected(void *buf,
                                          int count,
                                          MPI_Datatype datatype,
                                          MPIR_Comm * comm, int context_offset, MPIR_Request * rreq)
{
    int mpi_errno = MPI_SUCCESS;
    int dt_contig;
    MPI_Aint dt_true_lb, last;
    MPIR_Datatype *dt_ptr;
    size_t in_data_sz, dt_sz, nbytes;
    MPIR_Segment *segment_ptr;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_HANDLE_UNEXPECTED);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_HANDLE_UNEXPECTED);

    in_data_sz = MPIDI_CH4U_REQUEST(rreq, count);
    MPIR_Datatype_get_size_macro(datatype, dt_sz);

    if (in_data_sz > dt_sz * count) {
        rreq->status.MPI_ERROR = MPI_ERR_TRUNCATE;
        nbytes = dt_sz * count;
    }
    else {
        rreq->status.MPI_ERROR = MPI_SUCCESS;
        nbytes = in_data_sz;
        count = dt_sz ? nbytes / dt_sz : 0;
    }
    MPIR_STATUS_SET_COUNT(rreq->status, nbytes);
    MPIDI_CH4U_REQUEST(rreq, datatype) = datatype;
    MPIDI_CH4U_REQUEST(rreq, count) = nbytes;

    MPIDI_Datatype_get_info(count, datatype, dt_contig, dt_sz, dt_ptr, dt_true_lb);

    if (!dt_contig) {
        segment_ptr = MPIR_Segment_alloc();
        MPIR_ERR_CHKANDJUMP1(segment_ptr == NULL, mpi_errno,
                             MPI_ERR_OTHER, "**nomem", "**nomem %s", "Recv MPIR_Segment_alloc");
        MPIR_Segment_init(buf, count, datatype, segment_ptr, 0);

        last = nbytes;
        MPIR_Segment_unpack(segment_ptr, 0, &last, MPIDI_CH4U_REQUEST(rreq, buffer));
        MPIR_Segment_free(segment_ptr);
        if (last != (MPI_Aint) (nbytes)) {
            mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE,
                                             __FUNCTION__, __LINE__,
                                             MPI_ERR_TYPE, "**dtypemismatch", 0);
            rreq->status.MPI_ERROR = mpi_errno;
        }
    }
    else {
        MPIR_Memcpy((char *) buf + dt_true_lb, MPIDI_CH4U_REQUEST(rreq, buffer), nbytes);
    }

    MPIDI_CH4U_REQUEST(rreq, req->status) &= ~MPIDI_CH4U_REQ_UNEXPECTED;
    MPL_free(MPIDI_CH4U_REQUEST(rreq, buffer));

    rreq->status.MPI_SOURCE = MPIDI_CH4U_REQUEST(rreq, rank);
    rreq->status.MPI_TAG = MPIDI_CH4U_get_tag(MPIDI_CH4U_REQUEST(rreq, tag));

    if (MPIDI_CH4U_REQUEST(rreq, req->status) & MPIDI_CH4U_REQ_PEER_SSEND) {
        mpi_errno = MPIDI_reply_ssend(rreq);
        if (mpi_errno)
            MPIR_ERR_POP(mpi_errno);
    }
    MPID_Request_complete(rreq);
  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_HANDLE_UNEXPECTED);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_do_irecv
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_do_irecv(void *buf,
                                 int count,
                                 MPI_Datatype datatype,
                                 int rank,
                                 int tag,
                                 MPIR_Comm * comm,
                                 int context_offset,
                                 MPIR_Request ** request, int alloc_req, uint64_t flags)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Request *rreq = NULL, *unexp_req = NULL;
    uint64_t match_bits, mask_bits;
    MPIR_Context_id_t context_id = comm->recvcontext_id + context_offset;
    MPIR_Comm *root_comm;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_DO_IRECV);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_DO_IRECV);

    match_bits = MPIDI_CH4U_init_recvtag(&mask_bits, context_id, rank, tag);
    root_comm = MPIDI_CH4U_context_id_to_comm(comm->recvcontext_id);
    unexp_req = MPIDI_CH4U_dequeue_unexp(match_bits, mask_bits,
                                         &MPIDI_CH4U_COMM(root_comm, unexp_list));

    if (unexp_req) {
        MPIR_Comm_release(root_comm);   /* -1 for removing from unexp_list */
        if (MPIDI_CH4U_REQUEST(unexp_req, req->status) & MPIDI_CH4U_REQ_BUSY) {
            MPIDI_CH4U_REQUEST(unexp_req, req->status) |= MPIDI_CH4U_REQ_MATCHED;
        }
        else if (MPIDI_CH4U_REQUEST(unexp_req, req->status) & MPIDI_CH4U_REQ_LONG_RTS) {
            /* Matching receive is now posted, tell the netmod */
            dtype_add_ref_if_not_builtin(datatype);
            MPIDI_CH4U_REQUEST(unexp_req, datatype) = datatype;
            MPIDI_CH4U_REQUEST(unexp_req, buffer) = (char *) buf;
            MPIDI_CH4U_REQUEST(unexp_req, count) = count;
            *request = unexp_req;
            mpi_errno = MPIDI_NM_am_recv(unexp_req);
            if (mpi_errno)
                MPIR_ERR_POP(mpi_errno);
            goto fn_exit;
        }
        else {
            *request = unexp_req;
            mpi_errno =
                MPIDI_handle_unexpected(buf, count, datatype, root_comm, context_id, unexp_req);
            if (mpi_errno)
                MPIR_ERR_POP(mpi_errno);
            goto fn_exit;
        }
    }

    if (alloc_req) {
        rreq = MPIDI_CH4I_am_request_create(MPIR_REQUEST_KIND__RECV, 2);
    }
    else {
        rreq = *request;
        MPIR_Assert(0);
    }

    *request = rreq;
    if (unlikely(rank == MPI_PROC_NULL)) {
        rreq->kind = MPIR_REQUEST_KIND__RECV;
        rreq->status.MPI_ERROR = MPI_SUCCESS;
        rreq->status.MPI_SOURCE = rank;
        rreq->status.MPI_TAG = tag;
        MPID_Request_complete(rreq);
        goto fn_exit;
    }

    dtype_add_ref_if_not_builtin(datatype);
    MPIDI_CH4U_REQUEST(rreq, tag) = match_bits;
    MPIDI_CH4U_REQUEST(rreq, req->rreq.ignore) = mask_bits;
    MPIDI_CH4U_REQUEST(rreq, datatype) = datatype;

    mpi_errno = MPIDI_prepare_recv_req(buf, count, datatype, rreq);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);


    if (!unexp_req) {
        /* MPIDI_CS_ENTER(); */
        /* Increment refcnt for comm before posting rreq to posted_list,
         * to make sure comm is alive while holding an entry in the posted_list */
        MPIR_Comm_add_ref(root_comm);
        MPIDI_CH4U_enqueue_posted(rreq, &MPIDI_CH4U_COMM(root_comm, posted_list));
        /* MPIDI_CS_EXIT(); */
    }
    else {
        MPIDI_CH4U_REQUEST(unexp_req, req->rreq.match_req) = (uint64_t) rreq;
    }
  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_DO_IRECV);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDIG_mpi_recv
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPIDIG_mpi_recv(void *buf,
                                             int count,
                                             MPI_Datatype datatype,
                                             int rank,
                                             int tag,
                                             MPIR_Comm * comm,
                                             int context_offset, MPI_Status * status,
                                             MPIR_Request ** request)
{
    int mpi_errno;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIG_MPI_RECV);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIG_MPI_RECV);

    mpi_errno =
        MPIDI_do_irecv(buf, count, datatype, rank, tag, comm, context_offset, request, 1, 0ULL);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIG_MPI_RECV);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDIG_mpi_recv_init
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPIDIG_mpi_recv_init(void *buf,
                                                  int count,
                                                  MPI_Datatype datatype,
                                                  int rank,
                                                  int tag,
                                                  MPIR_Comm * comm,
                                                  int context_offset, MPIR_Request ** request)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Request *rreq;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIG_MPI_RECV_INIT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIG_MPI_RECV_INIT);

    rreq = MPIDI_CH4I_am_request_create(MPIR_REQUEST_KIND__PREQUEST_RECV, 2);

    *request = rreq;
    rreq->comm = comm;
    MPIR_Comm_add_ref(comm);

    MPIDI_CH4U_REQUEST(rreq, buffer) = (void *) buf;
    MPIDI_CH4U_REQUEST(rreq, count) = count;
    MPIDI_CH4U_REQUEST(rreq, datatype) = datatype;
    MPIDI_CH4U_REQUEST(rreq, rank) = rank;
    MPIDI_CH4U_REQUEST(rreq, tag) =
        MPIDI_CH4U_init_send_tag(comm->context_id + context_offset, rank, tag);
    rreq->u.persist.real_request = NULL;
    MPID_Request_complete(rreq);
    MPIDI_CH4U_REQUEST(rreq, p_type) = MPIDI_PTYPE_RECV;
    dtype_add_ref_if_not_builtin(datatype);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIG_MPI_RECV_INIT);
    return mpi_errno;
}


#undef FUNCNAME
#define FUNCNAME MPIDIG_mpi_imrecv
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPIDIG_mpi_imrecv(void *buf,
                                               int count,
                                               MPI_Datatype datatype,
                                               MPIR_Request * message, MPIR_Request ** rreqp)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Request *rreq;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIG_MPI_IMRECV);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIG_MPI_IMRECV);

    if (message == NULL) {
        MPIDI_Request_create_null_rreq(rreq, mpi_errno, fn_fail);
        *rreqp = rreq;
        goto fn_exit;
    }

    MPIR_Assert(message->kind == MPIR_REQUEST_KIND__MPROBE);
    MPIDI_CH4U_REQUEST(message, req->rreq.mrcv_buffer) = buf;
    MPIDI_CH4U_REQUEST(message, req->rreq.mrcv_count) = count;
    MPIDI_CH4U_REQUEST(message, req->rreq.mrcv_datatype) = datatype;
    *rreqp = message;

    /* MPIDI_CS_ENTER(); */
    if (MPIDI_CH4U_REQUEST(message, req->status) & MPIDI_CH4U_REQ_BUSY) {
        MPIDI_CH4U_REQUEST(message, req->status) |= MPIDI_CH4U_REQ_UNEXP_CLAIMED;
    }
    else if (MPIDI_CH4U_REQUEST(message, req->status) & MPIDI_CH4U_REQ_LONG_RTS) {
        /* Matching receive is now posted, tell the netmod */
        message->kind = MPIR_REQUEST_KIND__RECV;
        dtype_add_ref_if_not_builtin(datatype);
        MPIDI_CH4U_REQUEST(message, datatype) = datatype;
        MPIDI_CH4U_REQUEST(message, buffer) = (char *) buf;
        MPIDI_CH4U_REQUEST(message, count) = count;
        mpi_errno = MPIDI_NM_am_recv(message);
    }
    else {
        mpi_errno = MPIDI_handle_unexp_mrecv(message);
        if (mpi_errno)
            MPIR_ERR_POP(mpi_errno);
    }
    /* MPIDI_CS_EXIT(); */

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIG_MPI_IMRECV);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDIG_mrecv
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPIDIG_mrecv(void *buf,
                                          int count,
                                          MPI_Datatype datatype,
                                          MPIR_Request * message, MPI_Status * status)
{
    int mpi_errno = MPI_SUCCESS, active_flag;
    MPI_Request req_handle;
    MPIR_Request *rreq = NULL;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIG_MRECV);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIG_MRECV);

    mpi_errno = MPID_Imrecv(buf, count, datatype, message, &rreq);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

    while (!MPIR_Request_is_complete(rreq)) {
        MPID_Progress_test();
    }

    MPIR_Request_extract_status(rreq, status);

    mpi_errno = MPIR_Request_complete(&req_handle, rreq, status, &active_flag);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIG_MRECV);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDIG_mpi_irecv
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPIDIG_mpi_irecv(void *buf,
                                              int count,
                                              MPI_Datatype datatype,
                                              int rank,
                                              int tag,
                                              MPIR_Comm * comm, int context_offset,
                                              MPIR_Request ** request)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIG_MPI_IRECV);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIG_MPI_IRECV);

    mpi_errno =
        MPIDI_do_irecv(buf, count, datatype, rank, tag, comm, context_offset, request, 1, 0ULL);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);
  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIG_MPI_IRECV);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDIG_mpi_cancel_recv
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPIDIG_mpi_cancel_recv(MPIR_Request * rreq)
{
    int mpi_errno = MPI_SUCCESS, found;
    MPIR_Comm *root_comm;
    uint64_t msg_tag;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIG_MPI_CANCEL_RECV);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIG_MPI_CANCEL_RECV);

    msg_tag = MPIDI_CH4U_REQUEST(rreq, tag);
    root_comm = MPIDI_CH4U_context_id_to_comm(MPIDI_CH4U_get_context(msg_tag));

    /* MPIDI_CS_ENTER(); */
    found =
        MPIDI_CH4U_delete_posted(&rreq->dev.ch4.am.req->rreq,
                                 &MPIDI_CH4U_COMM(root_comm, posted_list));
    /* MPIDI_CS_EXIT(); */

    if (found) {
        MPIR_STATUS_SET_CANCEL_BIT(rreq->status, TRUE);
        MPIR_STATUS_SET_COUNT(rreq->status, 0);
        MPIR_Comm_release(root_comm);   /* -1 for posted_list */
        MPID_Request_complete(rreq);
    }
    else {
        MPIR_STATUS_SET_CANCEL_BIT(rreq->status, FALSE);
    }
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIG_MPI_CANCEL_RECV);
    return mpi_errno;
}

#endif /* MPIDIG_RECV_H_INCLUDED */
