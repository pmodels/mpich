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

MPL_STATIC_INLINE_PREFIX MPI_Aint MPIDI_POSIX_am_eager_limit(void)
{
    return MPIDI_POSIX_eager_payload_limit() - MAX_ALIGNMENT;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_POSIX_do_am_send_hdr(int grank,
                                                        MPIDI_POSIX_am_header_t * msg_hdr,
                                                        const void *am_hdr, bool issue_deferred,
                                                        int src_vsi, int dst_vsi);
MPL_STATIC_INLINE_PREFIX int MPIDI_POSIX_do_am_isend(int rank, MPIDI_POSIX_am_header_t * msg_hdr,
                                                     const void *am_hdr, MPI_Aint am_hdr_sz,
                                                     const void *data, MPI_Aint count,
                                                     MPI_Datatype datatype, MPIR_Request * sreq,
                                                     bool issue_deferred, int src_vsi, int dst_vsi);

/* Enqueue a request header onto the postponed message queue. This is a helper function and most
 * likely shouldn't be used outside of this file. */
MPL_STATIC_INLINE_PREFIX int MPIDI_POSIX_am_enqueue_request(const void *am_hdr, MPI_Aint am_hdr_sz,
                                                            const int grank,
                                                            MPIDI_POSIX_am_header_t * msg_hdr_p,
                                                            MPIR_Request * sreq, int src_vsi,
                                                            int dst_vsi)
{
    MPIDI_POSIX_am_request_header_t *curr_sreq_hdr = MPIDI_POSIX_AMREQUEST(sreq, req_hdr);
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_ENTER;

    MPIR_Assert(msg_hdr_p);

    /* Check to see if we need to create storage for the data to be sent. We did this above only if
     * we were sending a noncontiguous message, but we need it for all situations now. */
    if (!curr_sreq_hdr) {
        /* Prepare private storage */

        mpi_errno = MPIDI_POSIX_am_init_req_hdr(am_hdr, am_hdr_sz,
                                                &MPIDI_POSIX_AMREQUEST(sreq, req_hdr), sreq,
                                                src_vsi);
        MPIR_ERR_CHECK(mpi_errno);

        curr_sreq_hdr = MPIDI_POSIX_AMREQUEST(sreq, req_hdr);
    }

    curr_sreq_hdr->dst_grank = grank;
    curr_sreq_hdr->msg_hdr = NULL;

    curr_sreq_hdr->msg_hdr = &curr_sreq_hdr->msg_hdr_buf;
    curr_sreq_hdr->msg_hdr_buf = *msg_hdr_p;

    curr_sreq_hdr->request = sreq;
    curr_sreq_hdr->src_vsi = src_vsi;
    curr_sreq_hdr->dst_vsi = dst_vsi;

    DL_APPEND(MPIDI_POSIX_global.per_vsi[src_vsi].postponed_queue, curr_sreq_hdr);

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_POSIX_am_isend(int rank,
                                                  MPIR_Comm * comm,
                                                  int handler_id,
                                                  const void *am_hdr,
                                                  MPI_Aint am_hdr_sz,
                                                  const void *data,
                                                  MPI_Aint count,
                                                  MPI_Datatype datatype,
                                                  int src_vci, int dst_vci, MPIR_Request * sreq)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_POSIX_am_header_t msg_hdr;
    const int grank = MPIDIU_rank_to_lpid(rank, comm);

    MPIR_FUNC_ENTER;

    msg_hdr.handler_id = handler_id;
    msg_hdr.am_hdr_sz = am_hdr_sz;

    MPIR_Assert(src_vci < MPIDI_POSIX_global.num_vsis);
    MPIR_Assert(dst_vci < MPIDI_POSIX_global.num_vsis);
    int src_vsi = src_vci;
    int dst_vsi = dst_vci;
    mpi_errno = MPIDI_POSIX_do_am_isend(grank, &msg_hdr, am_hdr, am_hdr_sz, data, count,
                                        datatype, sreq, false, src_vsi, dst_vsi);

    MPIR_FUNC_EXIT;
    return mpi_errno;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_POSIX_am_isend_reply(MPIR_Comm * comm, int src_rank,
                                                        int handler_id,
                                                        const void *am_hdr,
                                                        MPI_Aint am_hdr_sz,
                                                        const void *data,
                                                        MPI_Aint count,
                                                        MPI_Datatype datatype,
                                                        int src_vci, int dst_vci,
                                                        MPIR_Request * sreq)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_ENTER;

    mpi_errno = MPIDI_POSIX_am_isend(src_rank, comm, handler_id, am_hdr, am_hdr_sz,
                                     data, count, datatype, src_vci, dst_vci, sreq);

    MPIR_FUNC_EXIT;

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
                                                            MPIDI_POSIX_am_header_t * msg_hdr,
                                                            int src_vsi, int dst_vsi)
{
    MPIDI_POSIX_am_request_header_t *curr_sreq_hdr = NULL;
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_ENTER;

    MPIR_Assert(msg_hdr);

    /* Prepare private storage */
    mpi_errno = MPIDI_POSIX_am_init_req_hdr(am_hdr, am_hdr_sz, &curr_sreq_hdr, NULL, src_vsi);
    MPIR_ERR_CHECK(mpi_errno);

    curr_sreq_hdr->dst_grank = grank;
    curr_sreq_hdr->src_vsi = src_vsi;
    curr_sreq_hdr->dst_vsi = dst_vsi;

    /* Header hasn't been sent so we should copy it from stack */
    curr_sreq_hdr->msg_hdr = &curr_sreq_hdr->msg_hdr_buf;
    curr_sreq_hdr->msg_hdr_buf = *msg_hdr;

    curr_sreq_hdr->request = NULL;
    DL_APPEND(MPIDI_POSIX_global.per_vsi[src_vsi].postponed_queue, curr_sreq_hdr);

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_POSIX_am_send_hdr(int rank, MPIR_Comm * comm, int handler_id,
                                                     const void *am_hdr, MPI_Aint am_hdr_sz,
                                                     int src_vci, int dst_vci)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_POSIX_am_header_t msg_hdr;
    const int grank = MPIDIU_rank_to_lpid(rank, comm);

    MPIR_FUNC_ENTER;

    msg_hdr.handler_id = handler_id;
    msg_hdr.am_hdr_sz = am_hdr_sz;
    msg_hdr.am_type = MPIDI_POSIX_AM_TYPE__HDR;

    MPIR_Assert(src_vci < MPIDI_POSIX_global.num_vsis);
    MPIR_Assert(dst_vci < MPIDI_POSIX_global.num_vsis);
    int src_vsi = src_vci;
    int dst_vsi = dst_vci;
    mpi_errno = MPIDI_POSIX_do_am_send_hdr(grank, &msg_hdr, am_hdr, false, src_vsi, dst_vsi);

    MPIR_FUNC_EXIT;
    return mpi_errno;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_POSIX_do_am_send_hdr(int grank,
                                                        MPIDI_POSIX_am_header_t * msg_hdr,
                                                        const void *am_hdr, bool issue_deferred,
                                                        int src_vsi, int dst_vsi)
{
    int mpi_errno = MPI_SUCCESS;
    int rc = MPIDI_POSIX_OK;

    MPIR_FUNC_ENTER;

    if (!issue_deferred && MPIDI_POSIX_global.per_vsi[src_vsi].postponed_queue) {
        goto fn_deferred;
    }

    MPIR_Assert(msg_hdr);

    rc = MPIDI_POSIX_eager_send(grank, msg_hdr, am_hdr, msg_hdr->am_hdr_sz, NULL, 0,
                                MPI_DATATYPE_NULL, 0, src_vsi, dst_vsi, NULL);

    if (rc == MPIDI_POSIX_NOK) {
        if (!issue_deferred) {
            goto fn_deferred;
        } else {
            goto fn_exit;
        }
    }

    /* hdr is sent, dequeue if was issuing deferred op */
    if (issue_deferred) {
        MPIDI_POSIX_am_request_header_t *curr_req_hdr =
            MPIDI_POSIX_global.per_vsi[src_vsi].postponed_queue;
        DL_DELETE(MPIDI_POSIX_global.per_vsi[src_vsi].postponed_queue, curr_req_hdr);
        MPIDI_POSIX_am_release_req_hdr(&curr_req_hdr);
    }

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_deferred:
    mpi_errno = MPIDI_POSIX_am_enqueue_req_hdr(am_hdr, msg_hdr->am_hdr_sz, grank, msg_hdr,
                                               src_vsi, dst_vsi);
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_POSIX_am_send_hdr_reply(MPIR_Comm * comm, int src_rank,
                                                           int handler_id, const void *am_hdr,
                                                           MPI_Aint am_hdr_sz,
                                                           int src_vci, int dst_vci)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_ENTER;

    mpi_errno = MPIDI_POSIX_am_send_hdr(src_rank, comm, handler_id, am_hdr, am_hdr_sz,
                                        src_vci, dst_vci);

    MPIR_FUNC_EXIT;

    return mpi_errno;
}


MPL_STATIC_INLINE_PREFIX int MPIDI_POSIX_do_am_isend(int grank,
                                                     MPIDI_POSIX_am_header_t * msg_hdr,
                                                     const void *am_hdr,
                                                     MPI_Aint am_hdr_sz,
                                                     const void *data,
                                                     MPI_Aint count,
                                                     MPI_Datatype datatype, MPIR_Request * sreq,
                                                     bool issue_deferred, int src_vsi, int dst_vsi)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_ENTER;

    /* NOTE: issue_deferred is set to true when progress use this function for deferred operations.
     * we need to skip some code path in the scenario. Also am_hdr, am_hdr_sz and data_sz are
     * ignored when issue_deferred is set to true. They should have been saved in the request. */

    MPIDI_POSIX_am_header_t *msg_hdr_p;
    if (!issue_deferred) {
        MPI_Aint data_sz;

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

    if (!issue_deferred && MPIDI_POSIX_global.per_vsi[src_vsi].postponed_queue) {
        goto fn_deferred;
    }

    MPI_Aint offset, sent_size;
    offset = MPIDIG_am_send_async_get_offset(sreq);

    int rc;
    if (offset) {
        rc = MPIDI_POSIX_eager_send(grank, NULL, NULL, 0, data, count, datatype, offset,
                                    src_vsi, dst_vsi, &sent_size);
    } else {
        rc = MPIDI_POSIX_eager_send(grank, msg_hdr, am_hdr, am_hdr_sz, data, count, datatype,
                                    offset, src_vsi, dst_vsi, &sent_size);
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
                     "issue seg for req handle=0x%x sent_size %ld", sreq->handle, sent_size));
    MPIDIG_am_send_async_issue_seg(sreq, sent_size);
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
            MPIDI_POSIX_am_request_header_t *curr_req_hdr =
                MPIDI_POSIX_global.per_vsi[src_vsi].postponed_queue;
            DL_DELETE(MPIDI_POSIX_global.per_vsi[src_vsi].postponed_queue, curr_req_hdr);

            MPL_free(MPIDI_POSIX_AMREQUEST_HDR(sreq, pack_buffer));
            MPIDI_POSIX_AMREQUEST_HDR(sreq, pack_buffer) = NULL;
        }
        mpi_errno = MPIDIG_global.origin_cbs[msg_hdr->handler_id] (sreq);
        MPIR_ERR_CHECK(mpi_errno);
    }

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
  fn_deferred:
    MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_GENERAL, VERBOSE,
                    (MPL_DBG_FDEST, "deferred posix_am_isend req handle=0x%x", sreq->handle));

    mpi_errno =
        MPIDI_POSIX_am_enqueue_request(am_hdr, am_hdr_sz, grank, msg_hdr_p, sreq, src_vsi, dst_vsi);
    MPIDI_POSIX_AMREQUEST_HDR(sreq, buf) = data;
    MPIDI_POSIX_AMREQUEST_HDR(sreq, datatype) = datatype;
    MPIDI_POSIX_AMREQUEST_HDR(sreq, count) = count;
    goto fn_exit;
}
#endif /* POSIX_AM_H_INCLUDED */
