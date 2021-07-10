/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef MPIDIG_AM_RECV_H_INCLUDED
#define MPIDIG_AM_RECV_H_INCLUDED

#include "ch4_impl.h"
#include "ch4r_proc.h"

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

/* utility function for copying data from the unexp buffer into user buffer. This avoid the dup code
 * for handling unexp message in am_do_irecv, am_do_imrecv, and completion. Specifically involves
 * the following cases:
 * 1. am_do_irecv: unexp req matched when posting Irecv
 * 2. am_do_imrecv: unexp req matched when posting Imrecv
 * 3. completion: unexp req matched with Irecv/Imrecv enqueued by someone else (WorkQ model)
 * */
MPL_STATIC_INLINE_PREFIX int MPIDIG_copy_from_unexp_req(MPIR_Request * req, void *user_buf,
                                                        MPI_Datatype user_datatype,
                                                        MPI_Aint user_count)
{
    int mpi_errno = MPI_SUCCESS;
    int dt_contig;
    MPI_Aint dt_true_lb;
    MPIR_Datatype *dt_ptr;
    MPI_Aint nbytes;
    size_t unexp_data_sz, dt_sz;

    /* unexp req stores data in MPIDIG_REQUEST(rreq, buffer) as MPI_BYTE.
     * MPIDIG_REQUEST(rreq, count) is the data size */
    unexp_data_sz = MPIDIG_REQUEST(req, count);

    MPIR_Datatype_get_size_macro(user_datatype, dt_sz);

    if (unexp_data_sz > dt_sz * user_count) {
        req->status.MPI_ERROR = MPIR_Err_create_code(req->status.MPI_ERROR,
                                                     MPIR_ERR_RECOVERABLE, __FUNCTION__, __LINE__,
                                                     MPI_ERR_TRUNCATE, "**truncate",
                                                     "**truncate %d %d %d %d",
                                                     req->status.MPI_SOURCE, req->status.MPI_TAG,
                                                     dt_sz * user_count, unexp_data_sz);
        nbytes = dt_sz * user_count;
    } else {
        nbytes = unexp_data_sz;
    }

    MPIR_STATUS_SET_COUNT(req->status, nbytes);
    MPIDIG_REQUEST(req, datatype) = user_datatype;
    MPIDIG_REQUEST(req, count) = dt_sz ? nbytes / dt_sz : 0;

    /* Copy the data from the message. */
    if (nbytes > 0) {
        MPIDI_Datatype_get_info(user_count, user_datatype, dt_contig, dt_sz, dt_ptr, dt_true_lb);
        if (!dt_contig) {
            MPI_Aint actual_unpack_bytes;
            MPIR_Typerep_unpack(MPIDIG_REQUEST(req, buffer), nbytes, user_buf, user_count,
                                user_datatype, 0, &actual_unpack_bytes);
            if (actual_unpack_bytes != nbytes) {
                mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE,
                                                 __FUNCTION__, __LINE__,
                                                 MPI_ERR_TYPE, "**dtypemismatch", 0);
                req->status.MPI_ERROR = mpi_errno;
            }
        } else {
            /* Note: buf could be NULL.  In one case it is a zero size message such as
             * the one used in MPI_Barrier.  In another case, the datatype can specify
             * the absolute address of the buffer (e.g. buf == MPI_BOTTOM).
             */
            char *addr = (char *) user_buf + dt_true_lb;
            MPIR_Typerep_copy(addr, MPIDIG_REQUEST(req, buffer), nbytes);
        }
    }

    MPIDU_genq_private_pool_free_cell(MPIDI_global.unexp_pack_buf_pool,
                                      MPIDIG_REQUEST(req, buffer));

    return mpi_errno;
}

MPL_STATIC_INLINE_PREFIX int MPIDIG_reply_ssend(MPIR_Request * rreq)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDIG_ssend_ack_msg_t ack_msg;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIG_REPLY_SSEND);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIG_REPLY_SSEND);
    ack_msg.sreq_ptr = MPIDIG_REQUEST(rreq, req->rreq.peer_req_ptr);

#ifndef MPIDI_CH4_DIRECT_NETMOD
    if (MPIDI_REQUEST(rreq, is_local))
        mpi_errno =
            MPIDI_SHM_am_send_hdr_reply(rreq->comm,
                                        MPIDIG_REQUEST(rreq, rank), MPIDIG_SSEND_ACK, &ack_msg,
                                        (MPI_Aint) sizeof(ack_msg));
    else
#endif
    {
        mpi_errno =
            MPIDI_NM_am_send_hdr_reply(rreq->comm,
                                       MPIDIG_REQUEST(rreq, rank), MPIDIG_SSEND_ACK, &ack_msg,
                                       (MPI_Aint) sizeof(ack_msg));
    }

    MPIR_ERR_CHECK(mpi_errno);
  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIG_REPLY_SSEND);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPIDIG_handle_unexpected(void *buf, MPI_Aint count,
                                                      MPI_Datatype datatype, MPIR_Request * rreq)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIG_HANDLE_UNEXPECTED);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIG_HANDLE_UNEXPECTED);

    rreq->status.MPI_SOURCE = MPIDIG_REQUEST(rreq, rank);
    rreq->status.MPI_TAG = MPIDIG_REQUEST(rreq, tag);

    mpi_errno = MPIDIG_copy_from_unexp_req(rreq, buf, datatype, count);
    MPIR_ERR_CHECK(mpi_errno);

    MPIDIG_REQUEST(rreq, req->status) &= ~MPIDIG_REQ_UNEXPECTED;

    /* If this is a synchronous send, send back the reply indicating that the message has been
     * matched. */
    if (MPIDIG_REQUEST(rreq, req->status) & MPIDIG_REQ_PEER_SSEND) {
        mpi_errno = MPIDIG_reply_ssend(rreq);
        MPIR_ERR_CHECK(mpi_errno);
    }

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIG_HANDLE_UNEXPECTED);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPIDIG_handle_unexp_mrecv(MPIR_Request * rreq)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIG_HANDLE_UNEXP_MRECV);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIG_HANDLE_UNEXP_MRECV);

    mpi_errno = MPIDIG_handle_unexpected(MPIDIG_REQUEST(rreq, req->rreq.mrcv_buffer),
                                         MPIDIG_REQUEST(rreq, req->rreq.mrcv_count),
                                         MPIDIG_REQUEST(rreq, req->rreq.mrcv_datatype), rreq);
    MPIR_ERR_CHECK(mpi_errno);
    MPIR_Datatype_release_if_not_builtin(MPIDIG_REQUEST(rreq, req->rreq.mrcv_datatype));
    MPID_Request_complete(rreq);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIG_HANDLE_UNEXP_MRECV);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPIDIG_do_am_recv(MPIR_Request * rreq)
{
#ifdef MPIDI_CH4_DIRECT_NETMOD
    return MPIDI_NM_am_recv(rreq);
#else
    if (MPIDI_REQUEST(rreq, is_local)) {
        return MPIDI_SHM_am_recv(rreq);
    } else {
        return MPIDI_NM_am_recv(rreq);
    }
#endif
}

MPL_STATIC_INLINE_PREFIX int MPIDIG_do_irecv(void *buf, MPI_Aint count, MPI_Datatype datatype,
                                             int rank, int tag, MPIR_Comm * comm,
                                             int context_offset, MPIR_Request ** request,
                                             int alloc_req, uint64_t flags)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Request *rreq = NULL, *unexp_req = NULL;
    MPIR_Context_id_t context_id = comm->recvcontext_id + context_offset;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIG_DO_IRECV);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIG_DO_IRECV);

    unexp_req =
        MPIDIG_rreq_dequeue(rank, tag, context_id, &MPIDI_global.unexp_list, MPIDIG_PT2PT_UNEXP);

    if (unexp_req) {
        unexp_req->comm = comm;
        MPIR_Comm_add_ref(comm);
        if (MPIDIG_REQUEST(unexp_req, req->status) & MPIDIG_REQ_BUSY) {
            MPIDIG_REQUEST(unexp_req, req->status) |= MPIDIG_REQ_MATCHED;
        } else if (MPIDIG_REQUEST(unexp_req, req->status) & MPIDIG_REQ_RTS) {
            /* the count for unexpected long message is the data size */
            MPI_Aint data_sz = MPIDIG_REQUEST(unexp_req, count);
            /* Matching receive is now posted, tell the netmod/shmmod */
            MPIR_Datatype_add_ref_if_not_builtin(datatype);
            MPIDIG_REQUEST(unexp_req, datatype) = datatype;
            MPIDIG_REQUEST(unexp_req, buffer) = (char *) buf;
            MPIDIG_REQUEST(unexp_req, count) = count;
            MPIDIG_REQUEST(unexp_req, recv_ready) = true;
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
            MPIDIG_recv_type_init(data_sz, unexp_req);
            mpi_errno = MPIDIG_do_cts(unexp_req);
            MPIR_ERR_CHECK(mpi_errno);
            goto fn_exit;
        } else {
            if (MPIDIG_REQUEST(unexp_req, recv_ready)) {
                /* if the unexpected recv is ready, the data is in the unexpected buffer. Just
                 * copy them to complete */
                mpi_errno = MPIDIG_handle_unexpected(buf, count, datatype, unexp_req);
                MPIR_ERR_CHECK(mpi_errno);
                MPID_Request_complete(unexp_req);
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
                    MPIDI_CH4_REQUEST_FREE(unexp_req);
                }
            } else {
                /* if the unexpected recv is not ready, we put user recv buffer info and let
                 * transport layer to start the recv. */
                /* the count for unexpected long message is the data size */
                MPI_Aint data_sz = MPIDIG_REQUEST(unexp_req, count);
                /* Matching receive is now posted, tell the netmod/shmmod */
                MPIR_Datatype_add_ref_if_not_builtin(datatype);
                MPIDIG_REQUEST(unexp_req, datatype) = datatype;
                MPIDIG_REQUEST(unexp_req, buffer) = (char *) buf;
                MPIDIG_REQUEST(unexp_req, count) = count;
                MPIDIG_REQUEST(unexp_req, recv_ready) = true;
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
                MPIDIG_REQUEST(unexp_req, req->seq_no) =
                    MPL_atomic_fetch_add_uint64(&MPIDI_global.nxt_seq_no, 1);
                MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_GENERAL, VERBOSE,
                                (MPL_DBG_FDEST, "seq_no: me=%" PRIu64 " exp=%" PRIu64,
                                 MPIDIG_REQUEST(unexp_req, req->seq_no),
                                 MPL_atomic_load_uint64(&MPIDI_global.exp_seq_no)));
                MPIDIG_recv_type_init(data_sz, unexp_req);
                MPIDIG_do_am_recv(unexp_req);
                MPIR_ERR_CHECK(mpi_errno);
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
        rreq->comm = comm;
        MPIR_Comm_add_ref(comm);
    } else {
        rreq = *request;
        MPIR_Assert(0);
    }

    *request = rreq;

    MPIR_Datatype_add_ref_if_not_builtin(datatype);
    MPIDIG_prepare_recv_req(rank, tag, context_id, buf, count, datatype, rreq);

    if (!unexp_req) {
        /* MPIDI_CS_ENTER(); */
        MPIDIG_enqueue_request(rreq, &MPIDI_global.posted_list, MPIDIG_PT2PT_POSTED);
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
        /* the count for unexpected long message is the data size */
        MPI_Aint data_sz = MPIDIG_REQUEST(message, count);
        /* Matching receive is now posted, tell the netmod */
        MPIDIG_REQUEST(message, datatype) = datatype;
        MPIDIG_REQUEST(message, buffer) = (char *) buf;
        MPIDIG_REQUEST(message, count) = count;
        MPIDIG_REQUEST(message, recv_ready) = true;
        MPIDIG_REQUEST(message, req->status) &= ~MPIDIG_REQ_UNEXPECTED;
        MPIDIG_recv_type_init(data_sz, message);
        MPIDIG_do_cts(message);
    } else {
        if (MPIDIG_REQUEST(message, recv_ready)) {
            mpi_errno = MPIDIG_handle_unexp_mrecv(message);
            MPIR_ERR_CHECK(mpi_errno);
        } else {
            /* the count for unexpected long message is the data size */
            MPI_Aint data_sz = MPIDIG_REQUEST(message, count);
            /* Matching receive is now posted, tell the netmod */
            MPIDIG_REQUEST(message, datatype) = datatype;
            MPIDIG_REQUEST(message, buffer) = (char *) buf;
            MPIDIG_REQUEST(message, count) = count;
            MPIDIG_REQUEST(message, recv_ready) = true;
            MPIDIG_REQUEST(message, req->status) &= ~MPIDIG_REQ_UNEXPECTED;
            MPIDIG_REQUEST(message, req->seq_no) =
                MPL_atomic_fetch_add_uint64(&MPIDI_global.nxt_seq_no, 1);
            MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_GENERAL, VERBOSE,
                            (MPL_DBG_FDEST, "seq_no: me=%" PRIu64 " exp=%" PRIu64,
                             MPIDIG_REQUEST(message, req->seq_no),
                             MPL_atomic_load_uint64(&MPIDI_global.exp_seq_no)));
            MPIDIG_recv_type_init(data_sz, message);
            MPIDIG_do_am_recv(message);
        }
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

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIG_MPI_CANCEL_RECV);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIG_MPI_CANCEL_RECV);

    if (!MPIR_Request_is_complete(rreq) &&
        !MPIR_STATUS_GET_CANCEL_BIT(rreq->status) && !MPIDIG_REQUEST_IN_PROGRESS(rreq)) {

        /* MPIDI_CS_ENTER(); */
        found = MPIDIG_delete_posted(rreq, &MPIDI_global.posted_list);
        /* MPIDI_CS_EXIT(); */

        if (found) {
            MPIR_Datatype_release_if_not_builtin(MPIDIG_REQUEST(rreq, datatype));
        }

        MPIR_STATUS_SET_CANCEL_BIT(rreq->status, TRUE);
        MPIR_STATUS_SET_COUNT(rreq->status, 0);
        MPID_Request_complete(rreq);
    }

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIG_MPI_CANCEL_RECV);
    return mpi_errno;
}

#endif /* MPIDIG_AM_RECV_H_INCLUDED */
