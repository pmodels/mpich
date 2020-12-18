/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef POSIX_AM_H_INCLUDED
#define POSIX_AM_H_INCLUDED

#include "posix_impl.h"
#include "posix_am_impl.h"
#include "posix_eager.h"
#include "mpidu_genq.h"

#define MPIDI_POSIX_RESIZE_TO_MAX_ALIGN(x) \
    ((((x) / MAX_ALIGNMENT) + !!((x) % MAX_ALIGNMENT)) * MAX_ALIGNMENT)

MPL_STATIC_INLINE_PREFIX MPI_Aint MPIDI_POSIX_am_eager_limit(void)
{
    return MPIDI_POSIX_eager_payload_limit() - MAX_ALIGNMENT;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_POSIX_do_am_send_hdr(int grank,
                                                        MPIDI_POSIX_am_header_t * msg_hdr,
                                                        const void *am_hdr, bool issue_deferred);
MPL_STATIC_INLINE_PREFIX int MPIDI_POSIX_do_am_isend(int rank,
                                                     MPIDI_POSIX_am_header_t * msg_hdr,
                                                     const void *am_hdr,
                                                     MPI_Aint am_hdr_sz,
                                                     const void *data,
                                                     MPI_Count count,
                                                     MPI_Datatype datatype, MPIR_Request * sreq,
                                                     bool issue_deferred);

/* Enqueue a request header onto the postponed message queue. This is a helper function and most
 * likely shouldn't be used outside of this file. */
MPL_STATIC_INLINE_PREFIX int MPIDI_POSIX_am_enqueue_request(const void *am_hdr, MPI_Aint am_hdr_sz,
                                                            const int grank,
                                                            MPIDI_POSIX_am_header_t * msg_hdr_p,
                                                            MPIR_Request * sreq)
{
    MPIDI_POSIX_am_request_header_t *curr_sreq_hdr = MPIDI_POSIX_AMREQUEST(sreq, req_hdr);
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_POSIX_AM_ENQUEUE_REQUEST);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_POSIX_AM_ENQUEUE_REQUEST);

    MPIR_Assert(msg_hdr_p);

    /* Check to see if we need to create storage for the data to be sent. We did this above only if
     * we were sending a noncontiguous message, but we need it for all situations now. */
    if (!curr_sreq_hdr) {
        /* Prepare private storage */

        mpi_errno = MPIDI_POSIX_am_init_req_hdr(am_hdr, am_hdr_sz,
                                                &MPIDI_POSIX_AMREQUEST(sreq, req_hdr), sreq);
        MPIR_ERR_CHECK(mpi_errno);

        curr_sreq_hdr = MPIDI_POSIX_AMREQUEST(sreq, req_hdr);
    }

    curr_sreq_hdr->dst_grank = grank;
    curr_sreq_hdr->msg_hdr = NULL;

    curr_sreq_hdr->msg_hdr = &curr_sreq_hdr->msg_hdr_buf;
    curr_sreq_hdr->msg_hdr_buf = *msg_hdr_p;

    curr_sreq_hdr->request = sreq;

    DL_APPEND(MPIDI_POSIX_global.postponed_queue, curr_sreq_hdr);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_POSIX_AM_ENQUEUE_REQUEST);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_POSIX_am_isend(int rank,
                                                  MPIR_Comm * comm,
                                                  MPIDI_POSIX_am_header_kind_t kind,
                                                  int handler_id,
                                                  const void *am_hdr,
                                                  MPI_Aint am_hdr_sz,
                                                  const void *data,
                                                  MPI_Count count,
                                                  MPI_Datatype datatype, MPIR_Request * sreq)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_POSIX_am_header_t msg_hdr;
    const int grank = MPIDIU_rank_to_lpid(rank, comm);

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_POSIX_AM_ISEND);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_POSIX_AM_ISEND);

    msg_hdr.kind = kind;
    msg_hdr.handler_id = handler_id;
    msg_hdr.am_hdr_sz = am_hdr_sz;

    mpi_errno = MPIDI_POSIX_do_am_isend(grank, &msg_hdr, am_hdr, am_hdr_sz, data, count,
                                        datatype, sreq, false);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_POSIX_AM_ISEND);
    return mpi_errno;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_POSIX_am_isendv(int rank,
                                                   MPIR_Comm * comm,
                                                   MPIDI_POSIX_am_header_kind_t kind,
                                                   int handler_id,
                                                   struct iovec *am_hdr,
                                                   size_t iov_len,
                                                   const void *data,
                                                   MPI_Count count,
                                                   MPI_Datatype datatype, MPIR_Request * sreq)
{
    int mpi_errno = MPI_SUCCESS;
    int is_allocated;
    MPI_Aint am_hdr_sz = 0;
    int i;
    uint8_t *am_hdr_buf = NULL;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_POSIX_AM_ISENDV);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_POSIX_AM_ISENDV);

    for (i = 0; i < iov_len; i++) {
        am_hdr_sz += am_hdr[i].iov_len;
    }

    if (am_hdr_sz > MPIDI_POSIX_AM_HDR_POOL_CELL_SIZE) {
        am_hdr_buf = (uint8_t *) MPL_malloc(am_hdr_sz, MPL_MEM_SHM);
        is_allocated = 1;
    } else {
        MPIDU_genq_private_pool_alloc_cell(MPIDI_POSIX_global.am_hdr_buf_pool,
                                           (void **) &am_hdr_buf);
        MPIR_Assert(am_hdr_buf);
        is_allocated = 0;
    }

    MPIR_Assert(am_hdr_buf);
    am_hdr_sz = 0;

    for (i = 0; i < iov_len; i++) {
        MPIR_Typerep_copy(am_hdr_buf + am_hdr_sz, am_hdr[i].iov_base, am_hdr[i].iov_len);
        am_hdr_sz += am_hdr[i].iov_len;
    }

    mpi_errno = MPIDI_POSIX_am_isend(rank, comm, kind, handler_id, am_hdr_buf, am_hdr_sz,
                                     data, count, datatype, sreq);

    if (is_allocated)
        MPL_free(am_hdr_buf);
    else
        MPIDU_genq_private_pool_free_cell(MPIDI_POSIX_global.am_hdr_buf_pool, am_hdr_buf);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_POSIX_AM_ISENDV);

    return mpi_errno;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_POSIX_am_isend_reply(MPIR_Context_id_t context_id, int src_rank,
                                                        MPIDI_POSIX_am_header_kind_t kind,
                                                        int handler_id,
                                                        const void *am_hdr,
                                                        MPI_Aint am_hdr_sz,
                                                        const void *data,
                                                        MPI_Count count,
                                                        MPI_Datatype datatype, MPIR_Request * sreq)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_POSIX_AM_ISEND_REPLY);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_POSIX_AM_ISEND_REPLY);

    mpi_errno = MPIDI_POSIX_am_isend(src_rank, MPIDIG_context_id_to_comm(context_id), kind,
                                     handler_id, am_hdr, am_hdr_sz, data, count, datatype, sreq);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_POSIX_AM_ISEND_REPLY);

    return mpi_errno;
}

MPL_STATIC_INLINE_PREFIX MPI_Aint MPIDI_POSIX_am_hdr_max_sz(void)
{
    /* Maximum size that fits in short send */

    MPI_Aint max_shortsend = MPIDI_POSIX_eager_payload_limit();

    /* Maximum payload size representable by MPIDI_POSIX_am_header_t::am_hdr_sz field */
    return MPL_MIN(max_shortsend, MPIDI_POSIX_MAX_AM_HDR_SIZE);
}

MPL_STATIC_INLINE_PREFIX MPI_Aint MPIDI_POSIX_am_eager_buf_limit(void)
{
    return MPIDI_POSIX_eager_buf_limit();
}

/* Enqueue a request header onto the postponed message queue. This is a helper function and most
 * likely shouldn't be used outside of this file. */
MPL_STATIC_INLINE_PREFIX int MPIDI_POSIX_am_enqueue_req_hdr(const void *am_hdr, MPI_Aint am_hdr_sz,
                                                            const int grank,
                                                            MPIDI_POSIX_am_header_t * msg_hdr)
{
    MPIDI_POSIX_am_request_header_t *curr_sreq_hdr = NULL;
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_POSIX_AM_ENQUEUE_REQ_HDR);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_POSIX_AM_ENQUEUE_REQ_HDR);

    MPIR_Assert(msg_hdr);

    /* Prepare private storage */
    mpi_errno = MPIDI_POSIX_am_init_req_hdr(am_hdr, am_hdr_sz, &curr_sreq_hdr, NULL);
    MPIR_ERR_CHECK(mpi_errno);

    curr_sreq_hdr->dst_grank = grank;

    /* Header hasn't been sent so we should copy it from stack */
    curr_sreq_hdr->msg_hdr = &curr_sreq_hdr->msg_hdr_buf;
    curr_sreq_hdr->msg_hdr_buf = *msg_hdr;

    curr_sreq_hdr->request = NULL;
    DL_APPEND(MPIDI_POSIX_global.postponed_queue, curr_sreq_hdr);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_POSIX_AM_ENQUEUE_REQ_HDR);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_POSIX_am_send_hdr(int rank,
                                                     MPIR_Comm * comm,
                                                     MPIDI_POSIX_am_header_kind_t kind,
                                                     int handler_id,
                                                     const void *am_hdr, MPI_Aint am_hdr_sz)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_POSIX_am_header_t msg_hdr;
    const int grank = MPIDIU_rank_to_lpid(rank, comm);

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_POSIX_AM_SEND_HDR);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_POSIX_AM_SEND_HDR);

    msg_hdr.kind = kind;
    msg_hdr.handler_id = handler_id;
    msg_hdr.am_hdr_sz = am_hdr_sz;
    msg_hdr.am_type = MPIDI_POSIX_AM_TYPE__HDR;

    mpi_errno = MPIDI_POSIX_do_am_send_hdr(grank, &msg_hdr, am_hdr, false);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_POSIX_AM_SEND_HDR);
    return mpi_errno;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_POSIX_do_am_send_hdr(int grank,
                                                        MPIDI_POSIX_am_header_t * msg_hdr,
                                                        const void *am_hdr, bool issue_deferred)
{
    int mpi_errno = MPI_SUCCESS;
    int rc = MPIDI_POSIX_OK;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_POSIX_DO_AM_SEND_HDR);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_POSIX_DO_AM_SEND_HDR);

    if (!issue_deferred && MPIDI_POSIX_global.postponed_queue) {
        goto fn_deferred;
    }

    MPIR_Assert(msg_hdr);

    rc = MPIDI_POSIX_eager_send(grank, msg_hdr, am_hdr, msg_hdr->am_hdr_sz, NULL, 0,
                                MPI_DATATYPE_NULL, 0, NULL);

    if (rc == MPIDI_POSIX_NOK) {
        if (!issue_deferred) {
            goto fn_deferred;
        } else {
            goto fn_exit;
        }
    }

    /* hdr is sent, dequeue if was issuing deferred op */
    if (issue_deferred) {
        MPIDI_POSIX_am_request_header_t *curr_req_hdr = MPIDI_POSIX_global.postponed_queue;
        DL_DELETE(MPIDI_POSIX_global.postponed_queue, curr_req_hdr);
        MPIDI_POSIX_am_release_req_hdr(&curr_req_hdr);
    }

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_POSIX_DO_AM_SEND_HDR);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
  fn_deferred:
    mpi_errno = MPIDI_POSIX_am_enqueue_req_hdr(am_hdr, msg_hdr->am_hdr_sz, grank, msg_hdr);
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_POSIX_am_send_hdr_reply(MPIR_Context_id_t context_id,
                                                           int src_rank,
                                                           MPIDI_POSIX_am_header_kind_t kind,
                                                           int handler_id, const void *am_hdr,
                                                           MPI_Aint am_hdr_sz)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_POSIX_AM_SEND_HDR_REPLY);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_POSIX_AM_SEND_HDR_REPLY);

    mpi_errno = MPIDI_POSIX_am_send_hdr(src_rank,
                                        MPIDIG_context_id_to_comm(context_id),
                                        kind, handler_id, am_hdr, am_hdr_sz);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_POSIX_AM_SEND_HDR_REPLY);

    return mpi_errno;
}


MPL_STATIC_INLINE_PREFIX int MPIDI_POSIX_do_am_isend(int grank,
                                                     MPIDI_POSIX_am_header_t * msg_hdr,
                                                     const void *am_hdr,
                                                     MPI_Aint am_hdr_sz,
                                                     const void *data,
                                                     MPI_Count count,
                                                     MPI_Datatype datatype, MPIR_Request * sreq,
                                                     bool issue_deferred)
{
    int mpi_errno = MPI_SUCCESS;
    int rc = 0;
    MPI_Aint data_sz, send_size, offset = 0;
    MPIDI_POSIX_am_header_t *msg_hdr_p;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_POSIX_DO_AM_ISEND);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_POSIX_DO_AM_ISEND);

    /* NOTE: issue_deferred is set to true when progress use this function for deferred operations.
     * we need to skip some code path in the scenario. Also am_hdr, am_hdr_sz and data_sz are
     * ignored when issue_deferred is set to true. They should have been saved in the request. */

    if (!issue_deferred) {
        MPIDI_POSIX_AMREQUEST(sreq, req_hdr) = NULL;

        MPIDI_Datatype_check_size(datatype, count, data_sz);

        msg_hdr_p = msg_hdr;
        if (data_sz + am_hdr_sz <= MPIDI_POSIX_am_eager_limit()) {
            msg_hdr_p->am_type = MPIDI_POSIX_AM_TYPE__SHORT;
        } else {
            msg_hdr_p->am_type = MPIDI_POSIX_AM_TYPE__PIPELINE;
        }

        MPIDIG_am_send_async_init(sreq, datatype, data_sz);
    } else {
        /* if we are issuing as deferred operation, use the msg_hdr in sreq */
        msg_hdr_p = MPIDI_POSIX_AMREQUEST_HDR(sreq, msg_hdr);
    }

    if (!issue_deferred && MPIDI_POSIX_global.postponed_queue) {
        goto fn_deferred;
    }

    send_size = MPIDIG_am_send_async_get_data_sz_left(sreq);
    offset = MPIDIG_am_send_async_get_offset(sreq);
    if (offset) {
        rc = MPIDI_POSIX_eager_send(grank, NULL, NULL, 0, data, count, datatype, offset,
                                    MPIDIG_am_send_async_get_data_sz_left(sreq) ? &send_size
                                    : NULL);
    } else {
        rc = MPIDI_POSIX_eager_send(grank, msg_hdr, am_hdr, am_hdr_sz, data, count, datatype,
                                    offset, MPIDIG_am_send_async_get_data_sz_left(sreq) ? &send_size
                                    : NULL);
    }

    if (rc == MPIDI_POSIX_NOK) {
        if (!issue_deferred) {
            goto fn_deferred;
        } else {
            goto fn_exit;
        }
    }

    MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_GENERAL, VERBOSE,
                    (MPL_DBG_FDEST,
                     "issue seg for req handle=0x%x send_size %ld", sreq->handle, send_size));
    MPIDIG_am_send_async_issue_seg(sreq, send_size);
    MPIDIG_am_send_async_finish_seg(sreq);

    /* if there IS MORE DATA to be sent and we ARE NOT called for issue deferred op, enqueue.
     * if there NO MORE DATA and we ARE called for issuing deferred op, pipeline is done, dequeue
     * skip for all other cases */
    if (!MPIDIG_am_send_async_is_done(sreq)) {
        if (!issue_deferred) {
            goto fn_deferred;
        }
    } else {
        /* all segments are sent, complete regularly and dequeue (if was issuing deferred op) */
        if (issue_deferred) {
            MPIDI_POSIX_am_request_header_t *curr_req_hdr = MPIDI_POSIX_global.postponed_queue;
            DL_DELETE(MPIDI_POSIX_global.postponed_queue, curr_req_hdr);

            MPL_free(MPIDI_POSIX_AMREQUEST_HDR(sreq, pack_buffer));
            MPIDI_POSIX_AMREQUEST_HDR(sreq, pack_buffer) = NULL;
        }
        mpi_errno = MPIDIG_global.origin_cbs[msg_hdr->handler_id] (sreq);
        MPIR_ERR_CHECK(mpi_errno);
    }

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_POSIX_DO_AM_ISEND);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
  fn_deferred:
    MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_GENERAL, VERBOSE,
                    (MPL_DBG_FDEST, "deferred posix_am_isend req handle=0x%x", sreq->handle));

    mpi_errno = MPIDI_POSIX_am_enqueue_request(am_hdr, am_hdr_sz, grank, msg_hdr_p, sreq);
    MPIDI_POSIX_AMREQUEST_HDR(sreq, buf) = data;
    MPIDI_POSIX_AMREQUEST_HDR(sreq, datatype) = datatype;
    MPIDI_POSIX_AMREQUEST_HDR(sreq, count) = count;
    goto fn_exit;
}
#endif /* POSIX_AM_H_INCLUDED */
