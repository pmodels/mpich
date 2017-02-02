/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 *  Portions of this code were written by Intel Corporation.
 *  Copyright (C) 2011-2017 Intel Corporation.  Intel provides this material
 *  to Argonne National Laboratory subject to Software Grant and Corporate
 *  Contributor License Agreement dated February 8, 2012.
 */
#ifndef POSIX_AM_H_INCLUDED
#define POSIX_AM_H_INCLUDED

#include "posix_impl.h"
#include "posix_am_impl.h"

/* Enqueue a request header onto the postponed message queue. This is a helper function and most
 * likely shouldn't be used outside of this file. */
MPL_STATIC_INLINE_PREFIX int MPIDI_POSIX_am_enqueue_request(const void *am_hdr, size_t am_hdr_sz,
                                                            int handler_id, const int grank,
                                                            MPIDI_POSIX_am_header_t msg_hdr,
                                                            MPIDI_POSIX_am_header_t * msg_hdr_p,
                                                            struct iovec *iov_left_ptr,
                                                            size_t iov_num_left, size_t data_sz,
                                                            MPIR_Request * sreq)
{
    MPIDI_POSIX_am_request_header_t *curr_sreq_hdr = NULL;
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_POSIX_AM_ENQUEUE_REQUEST);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_POSIX_AM_ENQUEUE_REQUEST);

    /* Check to see if we need to create storage for the data to be sent. We did this above only if
     * we were sending a noncontiguous message, but we need it for all situations now. */
    if (!curr_sreq_hdr) {
        /* Prepare private storage */

        mpi_errno = MPIDI_POSIX_am_init_req_hdr(am_hdr, am_hdr_sz,
                                                &MPIDI_POSIX_AMREQUEST(sreq, req_hdr), sreq);
        if (mpi_errno)
            MPIR_ERR_POP(mpi_errno);

        curr_sreq_hdr = MPIDI_POSIX_AMREQUEST(sreq, req_hdr);
    }

    curr_sreq_hdr->handler_id = handler_id;
    curr_sreq_hdr->dst_grank = grank;
    curr_sreq_hdr->msg_hdr = NULL;

    /* If this is true, the header hasn't been sent so we should copy it from stack */
    if (msg_hdr_p) {
        curr_sreq_hdr->msg_hdr = &curr_sreq_hdr->msg_hdr_buf;
        curr_sreq_hdr->msg_hdr_buf = msg_hdr;
    }

    curr_sreq_hdr->iov_num = iov_num_left;
    curr_sreq_hdr->iov_ptr = curr_sreq_hdr->iov;

    /* If we haven't made it through the header yet, make sure we include it in the iovec */
    if (iov_num_left == 2) {
        curr_sreq_hdr->iov[0].iov_base = curr_sreq_hdr->am_hdr;
        curr_sreq_hdr->iov[0].iov_len = curr_sreq_hdr->am_hdr_sz;

        curr_sreq_hdr->iov[1] = iov_left_ptr[1];
    } else if (iov_num_left == 1) {
        if (data_sz == 0) {
            curr_sreq_hdr->iov[0].iov_base = curr_sreq_hdr->am_hdr;
            curr_sreq_hdr->iov[0].iov_len = curr_sreq_hdr->am_hdr_sz;
        } else {
            curr_sreq_hdr->iov[0] = iov_left_ptr[0];
        }
    }

    curr_sreq_hdr->request = sreq;

    DL_APPEND(MPIDI_POSIX_global.postponed_queue, curr_sreq_hdr);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_POSIX_AM_ENQUEUE_REQUEST);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_POSIX_am_isend
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPIDI_POSIX_am_isend(int rank,
                                                  MPIR_Comm * comm,
                                                  int handler_id,
                                                  const void *am_hdr,
                                                  size_t am_hdr_sz,
                                                  const void *data,
                                                  MPI_Count count,
                                                  MPI_Datatype datatype, MPIR_Request * sreq)
{
    int mpi_errno = MPI_SUCCESS;
    int result = MPIDI_POSIX_OK;
    int dt_contig;
    size_t data_sz;
    MPIR_Datatype *dt_ptr;
    MPI_Aint dt_true_lb;
    MPIDI_POSIX_am_header_t msg_hdr;
    uint8_t *send_buf = NULL;
    MPIDI_POSIX_am_header_t *msg_hdr_p = &msg_hdr;
    MPIDI_POSIX_am_request_header_t *curr_sreq_hdr = NULL;
    const int grank = MPIDI_CH4U_rank_to_lpid(rank, comm);
    struct iovec iov_left[2];
    struct iovec *iov_left_ptr;
    size_t iov_num_left;
#ifdef POSIX_AM_DEBUG
    static int seq_num = 0;
#endif /* POSIX_AM_DEBUG */

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_POSIX_AM_ISEND);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_POSIX_AM_ISEND);

#ifdef POSIX_AM_DEBUG
    msg_hdr.seq_num = seq_num++;
#endif /* POSIX_AM_DEBUG */

    MPIDI_Datatype_get_info(count, datatype, dt_contig, data_sz, dt_ptr, dt_true_lb);

    send_buf = (uint8_t *) data + dt_true_lb;

    msg_hdr.handler_id = handler_id;
    msg_hdr.am_hdr_sz = am_hdr_sz;
    msg_hdr.data_sz = data_sz;

    /* If the data being sent is not contiguous, pack it into a contiguous buffer using the datatype
     * engine. */
    if (unlikely(!dt_contig && (data_sz > 0))) {
        size_t segment_first;
        MPI_Aint last;
        struct MPIR_Segment *segment_ptr = NULL;

        segment_ptr = MPIR_Segment_alloc();

        MPIR_ERR_CHKANDJUMP1(segment_ptr == NULL, mpi_errno,
                             MPI_ERR_OTHER, "**nomem", "**nomem %s", "Send MPIR_Segment_alloc");

        MPIR_Segment_init(data, count, datatype, segment_ptr);

        segment_first = 0;
        last = data_sz;

        MPIDI_POSIX_AMREQUEST(sreq, req_hdr) = NULL;

        /* Prepare private storage with information about the pack buffer. */
        mpi_errno = MPIDI_POSIX_am_init_req_hdr(am_hdr, am_hdr_sz,
                                                &MPIDI_POSIX_AMREQUEST(sreq, req_hdr), sreq);
        if (mpi_errno)
            MPIR_ERR_POP(mpi_errno);

        curr_sreq_hdr = MPIDI_POSIX_AMREQUEST(sreq, req_hdr);

        curr_sreq_hdr->pack_buffer = (char *) MPL_malloc(data_sz, MPL_MEM_SHM);

        MPIR_ERR_CHKANDJUMP1(MPIDI_POSIX_AMREQUEST_HDR(sreq, pack_buffer) == NULL, mpi_errno,
                             MPI_ERR_OTHER, "**nomem", "**nomem %s", "Send Pack buffer alloc");

        MPIR_Segment_pack(segment_ptr, segment_first, &last,
                          MPIDI_POSIX_AMREQUEST_HDR(sreq, pack_buffer));
        MPIR_Segment_free(segment_ptr);

        send_buf = (uint8_t *) curr_sreq_hdr->pack_buffer;
    }

    iov_left[0].iov_base = (void *) am_hdr;
    iov_left[0].iov_len = am_hdr_sz;
    iov_left[1].iov_base = (void *) send_buf;
    iov_left[1].iov_len = data_sz;

    iov_left_ptr = iov_left;

    iov_num_left = 2;

    /* If there's no data to send, the second iov can be empty and doesn't need to be transfered. */
    if (data_sz == 0) {
        iov_num_left = 1;
    }

    /* If we already have messages in the postponed queue, this one will probably also end up being
     * queued so save some cycles and do it now. */
    if (unlikely(MPIDI_POSIX_global.postponed_queue)) {
        mpi_errno = MPIDI_POSIX_am_enqueue_request(am_hdr, am_hdr_sz, handler_id, grank, msg_hdr,
                                                   msg_hdr_p, iov_left_ptr, iov_num_left, data_sz,
                                                   sreq);
        if (mpi_errno)
            MPIR_ERR_POP(mpi_errno);

        goto fn_exit;
    }

    POSIX_TRACE("Direct OUT HDR [ POSIX AM [handler_id %" PRIu64 ", am_hdr_sz %" PRIu64
                ", data_sz %" PRIu64 ", seq_num = %d], " "tag = %d, src_rank = %d ] to %d\n",
                (uint64_t) msg_hdr_p->handler_id,
                (uint64_t) msg_hdr_p->am_hdr_sz, (uint64_t) msg_hdr_p->data_sz,
#ifdef POSIX_AM_DEBUG
                msg_hdr_p->seq_num,
#else /* POSIX_AM_DEBUG */
                -1,
#endif /* POSIX_AM_DEBUG */
                ((MPIDI_CH4U_hdr_t *) am_hdr)->tag, ((MPIDI_CH4U_hdr_t *) am_hdr)->src_rank, grank);

    result = MPIDI_POSIX_eager_send(grank, &msg_hdr_p, &iov_left_ptr, &iov_num_left);

    /* If the message was not completed, queue it to be sent later. */
    if (unlikely((MPIDI_POSIX_NOK == result) || iov_num_left)) {
        mpi_errno = MPIDI_POSIX_am_enqueue_request(am_hdr, am_hdr_sz, handler_id, grank, msg_hdr,
                                                   msg_hdr_p, iov_left_ptr, iov_num_left, data_sz,
                                                   sreq);
        if (mpi_errno)
            MPIR_ERR_POP(mpi_errno);

        goto fn_exit;
    }

    /* If we made it here, the request has been completed and we can clean up the tracking
     * information and trigger the appropriate callbacks. */
    if (unlikely(curr_sreq_hdr)) {
        MPL_free(curr_sreq_hdr->pack_buffer);
        curr_sreq_hdr->pack_buffer = NULL;
    }

    mpi_errno = MPIDIG_global.origin_cbs[handler_id] (sreq);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_POSIX_AM_ISEND);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_POSIX_am_isendv
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPIDI_POSIX_am_isendv(int rank,
                                                   MPIR_Comm * comm,
                                                   int handler_id,
                                                   struct iovec *am_hdr,
                                                   size_t iov_len,
                                                   const void *data,
                                                   MPI_Count count,
                                                   MPI_Datatype datatype, MPIR_Request * sreq)
{
    int mpi_errno = MPI_SUCCESS;
    int is_allocated;
    size_t am_hdr_sz = 0;
    int i;
    uint8_t *am_hdr_buf = NULL;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_POSIX_SEND_AMV);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_POSIX_SEND_AMV);

    for (i = 0; i < iov_len; i++) {
        am_hdr_sz += am_hdr[i].iov_len;
    }

    if (am_hdr_sz > MPIDI_POSIX_BUF_POOL_SIZE) {
        am_hdr_buf = (uint8_t *) MPL_malloc(am_hdr_sz, MPL_MEM_SHM);
        is_allocated = 1;
    } else {
        am_hdr_buf = (uint8_t *) MPIDI_CH4R_get_buf(MPIDI_POSIX_global.am_buf_pool);
        is_allocated = 0;
    }

    MPIR_Assert(am_hdr_buf);
    am_hdr_sz = 0;

    for (i = 0; i < iov_len; i++) {
        MPIR_Memcpy(am_hdr_buf + am_hdr_sz, am_hdr[i].iov_base, am_hdr[i].iov_len);
        am_hdr_sz += am_hdr[i].iov_len;
    }

    mpi_errno = MPIDI_POSIX_am_isend(rank, comm, handler_id, am_hdr_buf, am_hdr_sz,
                                     data, count, datatype, sreq);

    if (is_allocated)
        MPL_free(am_hdr_buf);
    else
        MPIDI_CH4R_release_buf(am_hdr_buf);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_POSIX_SEND_AMV);

    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_POSIX_am_isend_reply
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPIDI_POSIX_am_isend_reply(MPIR_Context_id_t context_id, int src_rank,
                                                        int handler_id,
                                                        const void *am_hdr,
                                                        size_t am_hdr_sz,
                                                        const void *data,
                                                        MPI_Count count,
                                                        MPI_Datatype datatype, MPIR_Request * sreq)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_POSIX_AM_ISEND_REPLY);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_POSIX_AM_ISEND_REPLY);

    mpi_errno = MPIDI_POSIX_am_isend(src_rank,
                                     MPIDI_CH4U_context_id_to_comm(context_id),
                                     handler_id, am_hdr, am_hdr_sz, data, count, datatype, sreq);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_POSIX_AM_ISEND_REPLY);

    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_POSIX_am_hdr_max_sz
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX size_t MPIDI_POSIX_am_hdr_max_sz(void)
{
    /* Maximum size that fits in short send */

    size_t max_shortsend = MPIDI_POSIX_DEFAULT_SHORT_SEND_SIZE - (sizeof(MPIDI_POSIX_am_header_t));

    /* Maximum payload size representable by MPIDI_POSIX_am_header_t::am_hdr_sz field */

    size_t max_representable = (1 << MPIDI_POSIX_AM_HDR_SZ_BITS) - 1;

    return MPL_MIN(max_shortsend, max_representable);
}

#undef FUNCNAME
#define FUNCNAME MPIDI_POSIX_am_enqueue_req_hdr
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
/* Enqueue a request header onto the postponed message queue. This is a helper function and most
 * likely shouldn't be used outside of this file. */
MPL_STATIC_INLINE_PREFIX int MPIDI_POSIX_am_enqueue_req_hdr(const void *am_hdr, size_t am_hdr_sz,
                                                            int handler_id, const int grank,
                                                            MPIDI_POSIX_am_header_t msg_hdr,
                                                            size_t iov_num_left)
{
    MPIDI_POSIX_am_request_header_t *curr_sreq_hdr = NULL;
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_POSIX_AM_ENQUEUE_REQ_HDR);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_POSIX_AM_ENQUEUE_REQ_HDR);

    /* Prepare private storage */
    mpi_errno = MPIDI_POSIX_am_init_req_hdr(am_hdr, am_hdr_sz, &curr_sreq_hdr, NULL);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

    curr_sreq_hdr->handler_id = handler_id;
    curr_sreq_hdr->dst_grank = grank;
    curr_sreq_hdr->msg_hdr = NULL;

    /* Header hasn't been sent so we should copy it from stack */
    curr_sreq_hdr->msg_hdr = &curr_sreq_hdr->msg_hdr_buf;
    curr_sreq_hdr->msg_hdr_buf = msg_hdr;

    curr_sreq_hdr->iov_num = iov_num_left;
    curr_sreq_hdr->iov_ptr = curr_sreq_hdr->iov;
    curr_sreq_hdr->iov[0].iov_base = curr_sreq_hdr->am_hdr;
    curr_sreq_hdr->iov[0].iov_len = curr_sreq_hdr->am_hdr_sz;

    curr_sreq_hdr->request = NULL;
    DL_APPEND(MPIDI_POSIX_global.postponed_queue, curr_sreq_hdr);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_POSIX_AM_ENQUEUE_REQ_HDR);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_POSIX_am_send_hdr
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPIDI_POSIX_am_send_hdr(int rank,
                                                     MPIR_Comm * comm,
                                                     int handler_id,
                                                     const void *am_hdr, size_t am_hdr_sz)
{
    int mpi_errno = MPI_SUCCESS;
    int result = MPIDI_POSIX_OK;
    MPIDI_POSIX_am_header_t msg_hdr;
    MPIDI_POSIX_am_header_t *msg_hdr_p = &msg_hdr;
    struct iovec iov_left[1];
    struct iovec *iov_left_ptr = iov_left;
    size_t iov_num_left = 1;
    const int grank = MPIDI_CH4U_rank_to_lpid(rank, comm);

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_POSIX_AM_SEND_HDR);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_POSIX_AM_SEND_HDR);

    iov_left[0].iov_base = (void *) am_hdr;
    iov_left[0].iov_len = am_hdr_sz;

    msg_hdr.handler_id = handler_id;
    msg_hdr.am_hdr_sz = am_hdr_sz;
    msg_hdr.data_sz = 0;

    if (unlikely(MPIDI_POSIX_global.postponed_queue)) {
        mpi_errno = MPIDI_POSIX_am_enqueue_req_hdr(am_hdr, am_hdr_sz, handler_id, grank, msg_hdr,
                                                   iov_num_left);
        if (mpi_errno)
            MPIR_ERR_POP(mpi_errno);
    } else {

        POSIX_TRACE("Direct OUT HDR [ POSIX AM HDR [handler_id %" PRIu64 ", am_hdr_sz %" PRIu64
                    ", data_sz %" PRIu64 ", seq_num = %d]] to %d\n",
                    (uint64_t) msg_hdr_p->handler_id,
                    (uint64_t) msg_hdr_p->am_hdr_sz, (uint64_t) msg_hdr_p->data_sz,
#ifdef POSIX_AM_DEBUG
                    msg_hdr_p->seq_num,
#else /* POSIX_AM_DEBUG */
                    -1,
#endif /* POSIX_AM_DEBUG */
                    grank);

        result = MPIDI_POSIX_eager_send(grank, &msg_hdr_p, &iov_left_ptr, &iov_num_left);
        if (unlikely((MPIDI_POSIX_NOK == result) || iov_num_left)) {
            mpi_errno =
                MPIDI_POSIX_am_enqueue_req_hdr(am_hdr, am_hdr_sz, handler_id, grank, msg_hdr,
                                               iov_num_left);
            if (mpi_errno)
                MPIR_ERR_POP(mpi_errno);
            else
                goto fn_exit;
        }
    }

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_POSIX_AM_SEND_HDR);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_POSIX_am_send_am_hdr_reply
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPIDI_POSIX_am_send_hdr_reply(MPIR_Context_id_t context_id,
                                                           int src_rank, int handler_id,
                                                           const void *am_hdr, size_t am_hdr_sz)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_POSIX_SEND_AM_HDR_REPLY);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_POSIX_SEND_AM_HDR_REPLY);

    mpi_errno = MPIDI_POSIX_am_send_hdr(src_rank,
                                        MPIDI_CH4U_context_id_to_comm(context_id),
                                        handler_id, am_hdr, am_hdr_sz);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_POSIX_SEND_AM_HDR_REPLY);

    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_POSIX_am_recv
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPIDI_POSIX_am_recv(MPIR_Request * req)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_POSIX_AM_RECV);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_POSIX_AM_RECV);

    MPIDI_CH4U_send_long_ack_msg_t msg;

    msg.sreq_ptr = (MPIDI_CH4U_REQUEST(req, req->rreq.peer_req_ptr));
    msg.rreq_ptr = (uint64_t) req;
    MPIR_Assert((void *) msg.sreq_ptr != NULL);
    mpi_errno =
        MPIDI_POSIX_am_send_hdr_reply(MPIDI_CH4U_REQUEST(req, context_id),
                                      MPIDI_CH4U_REQUEST(req, rank), MPIDI_CH4U_SEND_LONG_ACK, &msg,
                                      sizeof(msg));
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_POSIX_AM_RECV);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#endif /* POSIX_AM_H_INCLUDED */
