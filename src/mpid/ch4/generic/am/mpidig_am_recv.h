/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef MPIDIG_AM_RECV_H_INCLUDED
#define MPIDIG_AM_RECV_H_INCLUDED

#include "ch4_impl.h"
#include "ch4r_proc.h"
#include "ch4r_recv.h"

MPL_STATIC_INLINE_PREFIX void MPIDIG_prepare_recv_req(int rank, int tag,
                                                      MPIR_Context_id_t context_id, void *buf,
                                                      MPI_Aint count, MPI_Datatype datatype,
                                                      MPIR_Request * rreq)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIG_PREPARE_RECV_REQ);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIG_PREPARE_RECV_REQ);

    MPIDIG_REQUEST(rreq, rank) = rank;
    MPIDIG_REQUEST(rreq, tag) = tag;
    MPIDIG_REQUEST(rreq, context_id) = context_id;
    MPIDIG_REQUEST(rreq, datatype) = datatype;
    MPIDIG_REQUEST(rreq, buffer) = (char *) buf;
    MPIDIG_REQUEST(rreq, count) = count;

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIG_PREPARE_RECV_REQ);
}

MPL_STATIC_INLINE_PREFIX int MPIDIG_handle_unexpected(void *buf, MPI_Aint count,
                                                      MPI_Datatype datatype, MPIR_Comm * comm,
                                                      int context_offset, MPIR_Request * rreq)
{
    int mpi_errno = MPI_SUCCESS;
    int dt_contig;
    MPI_Aint dt_true_lb;
    MPIR_Datatype *dt_ptr;
    size_t in_data_sz, dt_sz, nbytes;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIG_HANDLE_UNEXPECTED);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIG_HANDLE_UNEXPECTED);

    /* Calculate the size of the data from the active message information and update the request
     * object. */
    in_data_sz = MPIDIG_REQUEST(rreq, count);
    MPIR_Datatype_get_size_macro(datatype, dt_sz);

    if (in_data_sz > dt_sz * count) {
        rreq->status.MPI_ERROR = MPIR_Err_create_code(rreq->status.MPI_ERROR,
                                                      MPIR_ERR_RECOVERABLE, __FUNCTION__, __LINE__,
                                                      MPI_ERR_TRUNCATE, "**truncate",
                                                      "**truncate %d %d %d %d",
                                                      rreq->status.MPI_SOURCE, rreq->status.MPI_TAG,
                                                      dt_sz * count, in_data_sz);
        nbytes = dt_sz * count;
    } else {
        nbytes = in_data_sz;
    }
    MPIR_STATUS_SET_COUNT(rreq->status, nbytes);
    MPIDIG_REQUEST(rreq, datatype) = datatype;
    MPIDIG_REQUEST(rreq, count) = nbytes;

    MPIDI_Datatype_get_info(count, datatype, dt_contig, dt_sz, dt_ptr, dt_true_lb);

    /* Copy the data from the message. */

    if (!dt_contig) {
        MPI_Aint actual_unpack_bytes;
        MPIR_Typerep_unpack(MPIDIG_REQUEST(rreq, buffer), nbytes, buf, count, datatype, 0,
                            &actual_unpack_bytes);
        if (actual_unpack_bytes != (MPI_Aint) (nbytes)) {
            mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE,
                                             __FUNCTION__, __LINE__,
                                             MPI_ERR_TYPE, "**dtypemismatch", 0);
            rreq->status.MPI_ERROR = mpi_errno;
        }
    } else {
        /* Note: buf could be NULL.  In one case it is a zero size message such as
         * the one used in MPI_Barrier.  In another case, the datatype can specify
         * the absolute address of the buffer (e.g. buf == MPI_BOTTOM).
         */
        if (nbytes > 0) {
            char *addr = (char *) buf + dt_true_lb;
            assert(addr);       /* to supress gcc-8 warning: -Wnonnull */
            MPIR_Typerep_copy(addr, MPIDIG_REQUEST(rreq, buffer), nbytes);
        }
    }

    MPIDIG_REQUEST(rreq, req->status) &= ~MPIDIG_REQ_UNEXPECTED;
    MPIDU_genq_private_pool_free_cell(MPIDI_global.unexp_pack_buf_pool,
                                      MPIDIG_REQUEST(rreq, buffer));

    rreq->status.MPI_SOURCE = MPIDIG_REQUEST(rreq, rank);
    rreq->status.MPI_TAG = MPIDIG_REQUEST(rreq, tag);

    /* If this is a synchronous send, send back the reply indicating that the message has been
     * matched. */
    if (MPIDIG_REQUEST(rreq, req->status) & MPIDIG_REQ_PEER_SSEND) {
        mpi_errno = MPIDIG_reply_ssend(rreq);
        MPIR_ERR_CHECK(mpi_errno);
    }

    /* Decrement the refrence counter for the request object (for the reference held by the sending
     * process). */
    MPID_Request_complete(rreq);
  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIG_HANDLE_UNEXPECTED);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPIDIG_do_irecv(void *buf, MPI_Aint count, MPI_Datatype datatype,
                                             int rank, int tag, MPIR_Comm * comm,
                                             int context_offset, MPIR_Request ** request,
                                             int alloc_req, uint64_t flags)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Request *rreq = NULL, *unexp_req = NULL;
    MPIR_Context_id_t context_id = comm->recvcontext_id + context_offset;
    MPIR_Comm *root_comm;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIG_DO_IRECV);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIG_DO_IRECV);

    root_comm = MPIDIG_context_id_to_comm(context_id);
    unexp_req = MPIDIG_dequeue_unexp(rank, tag, context_id, &MPIDIG_COMM(root_comm, unexp_list));

    if (unexp_req) {
        MPIR_Comm_release(root_comm);   /* -1 for removing from unexp_list */
        if (MPIDIG_REQUEST(unexp_req, req->status) & MPIDIG_REQ_BUSY) {
            MPIDIG_REQUEST(unexp_req, req->status) |= MPIDIG_REQ_MATCHED;
        } else if (MPIDIG_REQUEST(unexp_req, req->status) & MPIDIG_REQ_RTS) {
            /* Matching receive is now posted, tell the netmod/shmmod */
            MPIR_Datatype_add_ref_if_not_builtin(datatype);
            MPIDIG_REQUEST(unexp_req, datatype) = datatype;
            MPIDIG_REQUEST(unexp_req, buffer) = (char *) buf;
            MPIDIG_REQUEST(unexp_req, count) = count;
            if (*request == NULL) {
                /* Regular (non-enqueuing) path: MPIDIG is responsbile for allocating
                 * a request. Here we simply return `unexp_req` */
                *request = unexp_req;
                /* Mark `match_req` as NULL so that we know nothing else to complete when
                 * `unexp_req` finally completes. (See below) */
                MPIDIG_REQUEST(unexp_req, req->rreq.match_req) = NULL;
            } else {
                /* Enqueuing path: CH4 already allocated a request.
                 * Record the passed `*request` to `match_req` so that we can complete it
                 * later when `unexp_req` completes.
                 * See MPIDI_recv_target_cmpl_cb for actual completion handler. */
                MPIDIG_REQUEST(unexp_req, req->rreq.match_req) = *request;
            }
            MPIDIG_REQUEST(unexp_req, req->status) &= ~MPIDIG_REQ_UNEXPECTED;
            mpi_errno = MPIDIG_do_cts(unexp_req);
            MPIR_ERR_CHECK(mpi_errno);
            goto fn_exit;
        } else {
            mpi_errno =
                MPIDIG_handle_unexpected(buf, count, datatype, root_comm, context_id, unexp_req);
            MPIR_ERR_CHECK(mpi_errno);
            if (*request == NULL) {
                /* Regular (non-enqueuing) path: MPIDIG is responsbile for allocating
                 * a request. Here we simply return `unexp_req`, which is already completed. */
                *request = unexp_req;
            } else {
                /* Enqueuing path: CH4 already allocated request as `*request`.
                 * Since the real operations has completed in `unexp_req`, here we
                 * simply copy the status to `*request` and complete it. */
                (*request)->status = unexp_req->status;
                MPIR_Request_add_ref(*request);
                MPID_Request_complete(*request);
                /* Need to free here because we don't return this to user */
                MPIR_Request_free_unsafe(unexp_req);
            }
            goto fn_exit;
        }
    }

    if (*request != NULL) {
        rreq = *request;
        MPIDIG_request_init(rreq, MPIR_REQUEST_KIND__RECV);
    } else if (alloc_req) {
        rreq = MPIDIG_request_create(MPIR_REQUEST_KIND__RECV, 2);
        MPIR_ERR_CHKANDSTMT(rreq == NULL, mpi_errno, MPIX_ERR_NOREQ, goto fn_fail, "**nomemreq");
    } else {
        rreq = *request;
        MPIR_Assert(0);
    }

    *request = rreq;

    MPIR_Datatype_add_ref_if_not_builtin(datatype);
    MPIDIG_prepare_recv_req(rank, tag, context_id, buf, count, datatype, rreq);

    if (!unexp_req) {
        /* MPIDI_CS_ENTER(); */
        /* Increment refcnt for comm before posting rreq to posted_list,
         * to make sure comm is alive while holding an entry in the posted_list */
        MPIR_Comm_add_ref(root_comm);
        MPIDIG_enqueue_posted(rreq, &MPIDIG_COMM(root_comm, posted_list));
        /* MPIDI_CS_EXIT(); */
    } else {
        MPIDIG_REQUEST(unexp_req, req->rreq.match_req) = rreq;
        MPIDIG_REQUEST(rreq, req->status) |= MPIDIG_REQ_IN_PROGRESS;
    }
  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIG_DO_IRECV);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPIDIG_mpi_recv(void *buf,
                                             MPI_Aint count,
                                             MPI_Datatype datatype,
                                             int rank,
                                             int tag,
                                             MPIR_Comm * comm,
                                             int context_offset, MPI_Status * status,
                                             MPIR_Request ** request, int is_local)
{
    int mpi_errno;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIG_MPI_RECV);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIG_MPI_RECV);
    MPID_THREAD_CS_ENTER(VCI, MPIDI_VCI(0).lock);

    mpi_errno =
        MPIDIG_do_irecv(buf, count, datatype, rank, tag, comm, context_offset, request, 1, 0ULL);
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    MPIDI_REQUEST_SET_LOCAL(*request, is_local, NULL);
    MPID_THREAD_CS_EXIT(VCI, MPIDI_VCI(0).lock);
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIG_MPI_RECV);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPIDIG_mpi_imrecv(void *buf,
                                               MPI_Aint count,
                                               MPI_Datatype datatype, MPIR_Request * message)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIG_MPI_IMRECV);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIG_MPI_IMRECV);
#if MPICH_THREAD_GRANULARITY == MPICH_THREAD_GRANULARITY__VCI
    int vci = MPIDI_Request_get_vci(message);
#endif
    MPID_THREAD_CS_ENTER(VCI, MPIDI_VCI(vci).lock);

    MPIDIG_REQUEST(message, req->rreq.mrcv_buffer) = buf;
    MPIDIG_REQUEST(message, req->rreq.mrcv_count) = count;
    MPIDIG_REQUEST(message, req->rreq.mrcv_datatype) = datatype;
    MPIR_Datatype_add_ref_if_not_builtin(datatype);

    /* MPIDI_CS_ENTER(); */
    if (MPIDIG_REQUEST(message, req->status) & MPIDIG_REQ_BUSY) {
        MPIDIG_REQUEST(message, req->status) |= MPIDIG_REQ_UNEXP_CLAIMED;
    } else if (MPIDIG_REQUEST(message, req->status) & MPIDIG_REQ_RTS) {
        /* Matching receive is now posted, tell the netmod */
        MPIDIG_REQUEST(message, datatype) = datatype;
        MPIDIG_REQUEST(message, buffer) = (char *) buf;
        MPIDIG_REQUEST(message, count) = count;
        MPIDIG_REQUEST(message, req->status) &= ~MPIDIG_REQ_UNEXPECTED;
        MPIDIG_do_cts(message);
    } else {
        mpi_errno = MPIDIG_handle_unexp_mrecv(message);
        MPIR_ERR_CHECK(mpi_errno);
    }
    /* MPIDI_CS_EXIT(); */

  fn_exit:
    MPID_THREAD_CS_EXIT(VCI, MPIDI_VCI(vci).lock);
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIG_MPI_IMRECV);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPIDIG_mpi_irecv(void *buf,
                                              MPI_Aint count,
                                              MPI_Datatype datatype,
                                              int rank,
                                              int tag,
                                              MPIR_Comm * comm, int context_offset,
                                              MPIR_Request ** request,
                                              int is_local, MPIR_Request * partner)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIG_MPI_IRECV);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIG_MPI_IRECV);
    /* For anysource recv, we may be called while holding the vci lock of shm request (to
     * prevent shm progress). Therefore, recursive locking is allowed here */
    MPID_THREAD_CS_ENTER_REC_VCI(MPIDI_VCI(0).lock);

    mpi_errno =
        MPIDIG_do_irecv(buf, count, datatype, rank, tag, comm, context_offset, request, 1, 0ULL);
    MPIR_ERR_CHECK(mpi_errno);
  fn_exit:
    MPIDI_REQUEST_SET_LOCAL(*request, is_local, partner);
    MPID_THREAD_CS_EXIT(VCI, MPIDI_VCI(0).lock);
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIG_MPI_IRECV);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPIDIG_mpi_cancel_recv(MPIR_Request * rreq)
{
    int mpi_errno = MPI_SUCCESS, found;
    MPIR_Comm *root_comm;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIG_MPI_CANCEL_RECV);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIG_MPI_CANCEL_RECV);

    if (!MPIR_Request_is_complete(rreq) &&
        !MPIR_STATUS_GET_CANCEL_BIT(rreq->status) && !MPIDIG_REQUEST_IN_PROGRESS(rreq)) {
        root_comm = MPIDIG_context_id_to_comm(MPIDIG_REQUEST(rreq, context_id));

        /* MPIDI_CS_ENTER(); */
        found =
            MPIDIG_delete_posted(&MPIDIG_REQUEST(rreq, req->rreq),
                                 &MPIDIG_COMM(root_comm, posted_list));
        /* MPIDI_CS_EXIT(); */

        if (found) {
            MPIR_Datatype_release_if_not_builtin(MPIDIG_REQUEST(rreq, datatype));
            MPIR_Comm_release(root_comm);       /* -1 for posted_list */
        }

        MPIR_STATUS_SET_CANCEL_BIT(rreq->status, TRUE);
        MPIR_STATUS_SET_COUNT(rreq->status, 0);
        MPID_Request_complete(rreq);
    }

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIG_MPI_CANCEL_RECV);
    return mpi_errno;
}

#endif /* MPIDIG_AM_RECV_H_INCLUDED */
