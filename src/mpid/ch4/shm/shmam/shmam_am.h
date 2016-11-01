/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 *  Portions of this code were written by Intel Corporation.
 *  Copyright (C) 2011-2016 Intel Corporation.  Intel provides this material
 *  to Argonne National Laboratory subject to Software Grant and Corporate
 *  Contributor License Agreement dated February 8, 2012.
 */
#ifndef SHM_SHMAM_AM_H_INCLUDED
#define SHM_SHMAM_AM_H_INCLUDED

#include "shmam_impl.h"
#include "shmam_am_impl.h"

#include <shmam_eager_impl.h>

#undef FUNCNAME
#define FUNCNAME MPIDI_SHM_am_reg_hdr_handler
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int
MPIDI_SHM_am_reg_handler(int handler_id,
                         MPIDI_NM_am_origin_handler_fn origin_handler_fn,
                         MPIDI_NM_am_target_handler_fn target_handler_fn)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_SHM_AM_REG_HANDLER);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_SHM_AM_REG_HANDLER);

    if (handler_id > MPIDI_SHM_MAX_AM_HANDLERS) {
        mpi_errno = MPI_ERR_OTHER;
        goto fn_fail;
    }

    MPIDI_SHM_Shmam_global.am_handlers[handler_id] = target_handler_fn;
    MPIDI_SHM_Shmam_global.am_send_cmpl_handlers[handler_id] = origin_handler_fn;
  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_SHM_REG_HDR_HANDLER);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_SHM_am_isend
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int
MPIDI_SHM_am_isend(int rank,
                   MPIR_Comm * comm,
                   int handler_id,
                   const void *am_hdr,
                   size_t am_hdr_sz,
                   const void *data,
                   MPI_Count count, MPI_Datatype datatype, MPIR_Request * sreq, void *shm_context)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_SHM_SEND_AM);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_SHM_SEND_AM);

    int result = MPIDI_SHMAM_OK;

    int dt_contig;
    size_t data_sz;
    MPIR_Datatype *dt_ptr;
    MPI_Aint dt_true_lb;

    uint8_t *send_buf = NULL;

    MPIDI_Datatype_get_info(count, datatype, dt_contig, data_sz, dt_ptr, dt_true_lb);

    send_buf = (uint8_t *) data + dt_true_lb;

    MPIDI_SHM_am_header_t msg_hdr = {.handler_id = handler_id,
        .am_hdr_sz = am_hdr_sz,
        .data_sz = data_sz
    };
#ifdef SHM_AM_DEBUG
    static int seq_num = 0;
    msg_hdr.seq_num = seq_num++;
#endif /* SHM_AM_DEBUG */

    MPIDI_SHM_am_header_t *msg_hdr_p = &msg_hdr;
    MPIDI_SHM_am_request_header_t *curr_sreq_hdr = NULL;

    if (unlikely(!dt_contig)) {
        size_t segment_first;
        MPI_Aint last;
        struct MPIDU_Segment *segment_ptr = NULL;

        segment_ptr = MPIDU_Segment_alloc();

        MPIR_ERR_CHKANDJUMP1(segment_ptr == NULL, mpi_errno,
                             MPI_ERR_OTHER, "**nomem", "**nomem %s", "Send MPIDU_Segment_alloc");

        MPIDU_Segment_init(data, count, datatype, segment_ptr, 0);

        segment_first = 0;
        last = data_sz;

        MPIDI_SHM_AMREQUEST(sreq, req_hdr) = NULL;

        /* Prepare private storage */

        mpi_errno = MPIDI_SHM_am_init_request(am_hdr, am_hdr_sz, sreq);
        if (mpi_errno)
            MPIR_ERR_POP(mpi_errno);

        curr_sreq_hdr = MPIDI_SHM_AMREQUEST(sreq, req_hdr);

        curr_sreq_hdr->pack_buffer = (char *) MPL_malloc(data_sz);

        MPIR_ERR_CHKANDJUMP1(MPIDI_SHM_AMREQUEST_HDR(sreq, pack_buffer) == NULL, mpi_errno,
                             MPI_ERR_OTHER, "**nomem", "**nomem %s", "Send Pack buffer alloc");

        MPIDU_Segment_pack(segment_ptr, segment_first, &last,
                           MPIDI_SHM_AMREQUEST_HDR(sreq, pack_buffer));
        MPIDU_Segment_free(segment_ptr);

        send_buf = (uint8_t *) curr_sreq_hdr->pack_buffer;
    }

    const int grank = MPIDI_CH4U_rank_to_lpid(rank, comm);

    struct iovec iov_left[2] = { {.iov_base = (void *) am_hdr,
                                  .iov_len = am_hdr_sz},
    {.iov_base = (void *) send_buf,
     .iov_len = data_sz}
    };

    struct iovec *iov_left_ptr = iov_left;

    size_t iov_num_left = 2;

    if (!data || !count) {
        iov_num_left = 1;
    }

    if (unlikely(MPIDI_SHM_Shmam_global.postponed_queue)) {
        goto enqueue_request;
    }

#ifdef SHM_AM_DEBUG
    fprintf(MPIDI_SHM_Shmam_global.logfile,
            "Direct OUT HDR [ SHM AM [handler_id %llu, am_hdr_sz %llu, data_sz %llu, seq_num = %d], "
            "tag = %08x, src_rank = %d ] to %d\n",
            msg_hdr_p->handler_id,
            msg_hdr_p->am_hdr_sz,
            msg_hdr_p->data_sz,
            msg_hdr_p->seq_num,
            ((MPIDI_CH4U_hdr_t *) am_hdr)->msg_tag, ((MPIDI_CH4U_hdr_t *) am_hdr)->src_rank, grank);
    fflush(MPIDI_SHM_Shmam_global.logfile);
#endif /* SHM_AM_DEBUG */

    result = MPIDI_SHMAM_eager_send(grank, &msg_hdr_p, &iov_left_ptr, &iov_num_left, 0);

    if (unlikely((MPIDI_SHMAM_NOK == result) || iov_num_left)) {
        goto enqueue_request;
    }

    /* Request has been completed */

    if (unlikely(curr_sreq_hdr && curr_sreq_hdr->pack_buffer)) {
        MPL_free(curr_sreq_hdr->pack_buffer);
        curr_sreq_hdr->pack_buffer = NULL;
    }

    mpi_errno = MPIDI_SHM_Shmam_global.am_send_cmpl_handlers[handler_id] (sreq);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

    if (unlikely(curr_sreq_hdr)) {
        MPIDI_SHM_am_clear_request(sreq);
    }

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_SHM_SEND_AM);
    return mpi_errno;
  fn_fail:
    goto fn_exit;

  enqueue_request:

#ifdef SHM_AM_DEBUG
    fprintf(MPIDI_SHM_Shmam_global.logfile, "%s enqueue send request\n", __func__);
    fflush(MPIDI_SHM_Shmam_global.logfile);
#endif /* SHM_AM_DEBUG */

    if (!curr_sreq_hdr) {
        /* Prepare private storage */

        mpi_errno = MPIDI_SHM_am_init_request(am_hdr, am_hdr_sz, sreq);
        if (mpi_errno)
            MPIR_ERR_POP(mpi_errno);

        curr_sreq_hdr = MPIDI_SHM_AMREQUEST(sreq, req_hdr);
    }

    curr_sreq_hdr->handler_id = handler_id;
    curr_sreq_hdr->dst_grank = grank;
    curr_sreq_hdr->msg_hdr = NULL;

    if (msg_hdr_p) {
        /* Header hasn't been sent so we should copy it from stack */

        curr_sreq_hdr->msg_hdr = &curr_sreq_hdr->msg_hdr_buf;
        curr_sreq_hdr->msg_hdr_buf = msg_hdr;
    }

    curr_sreq_hdr->iov_num = iov_num_left;
    curr_sreq_hdr->iov_ptr = curr_sreq_hdr->iov;

    int i;

    for (i = 0; i < iov_num_left; i++) {
        curr_sreq_hdr->iov[i] = iov_left_ptr[i];
    }

    MPIDI_CH4U_REQUEST(sreq, req->rreq.request) = (uint64_t) sreq;

    MPL_DL_APPEND(MPIDI_SHM_Shmam_global.postponed_queue, &sreq->dev.ch4.ch4u.req->rreq);

    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_SHM_am_isendv
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int
MPIDI_SHM_am_isendv(int rank,
                    MPIR_Comm * comm,
                    int handler_id,
                    struct iovec *am_hdr,
                    size_t iov_len,
                    const void *data,
                    MPI_Count count, MPI_Datatype datatype, MPIR_Request * sreq, void *shm_context)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_SHM_SEND_AMV);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_SHM_SEND_AMV);

    int is_allocated;
    size_t am_hdr_sz = 0;
    int i;
    uint8_t *am_hdr_buf = NULL;

    for (i = 0; i < iov_len; i++) {
        am_hdr_sz += am_hdr[i].iov_len;
    }

    if (am_hdr_sz > MPIDI_SHM_BUF_POOL_SIZE) {
        am_hdr_buf = (uint8_t *) MPL_malloc(am_hdr_sz);
        is_allocated = 1;
    }
    else {
        am_hdr_buf = (uint8_t *) MPIDI_CH4R_get_buf(MPIDI_SHM_Shmam_global.am_buf_pool);
        is_allocated = 0;
    }

    MPIR_Assert(am_hdr_buf);
    am_hdr_sz = 0;

    for (i = 0; i < iov_len; i++) {
        MPIR_Memcpy(am_hdr_buf + am_hdr_sz, am_hdr[i].iov_base, am_hdr[i].iov_len);
        am_hdr_sz += am_hdr[i].iov_len;
    }

    mpi_errno = MPIDI_SHM_am_isend(rank, comm, handler_id, am_hdr_buf, am_hdr_sz,
                                   data, count, datatype, sreq, shm_context);

    if (is_allocated)
        MPL_free(am_hdr_buf);
    else
        MPIDI_CH4R_release_buf(am_hdr_buf);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_SHM_SEND_AMV);

    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_SHM_am_isend_reply
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int
MPIDI_SHM_am_isend_reply(MPIR_Context_id_t context_id,
                         int src_rank,
                         int handler_id,
                         const void *am_hdr,
                         size_t am_hdr_sz,
                         const void *data,
                         MPI_Count count, MPI_Datatype datatype, MPIR_Request * sreq)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_SHM_AM_ISEND_REPLY);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_SHM_AM_ISEND_REPLY);

    mpi_errno = MPIDI_SHM_am_isend(src_rank,
                                   MPIDI_CH4U_context_id_to_comm(context_id),
                                   handler_id,
                                   am_hdr, am_hdr_sz, data, count, datatype, sreq, NULL);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_SHM_AM_ISEND_REPLY);

    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_SHM_am_hdr_max_sz
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX size_t MPIDI_SHM_am_hdr_max_sz(void)
{
    /* Maximum size that fits in short send */

    size_t max_shortsend = MPIDI_SHM_DEFAULT_SHORT_SEND_SIZE - (sizeof(MPIDI_SHM_am_header_t));

    /* Maximum payload size representable by MPIDI_SHM_am_header_t::am_hdr_sz field */

    size_t max_representable = (1 << MPIDI_SHM_AM_HDR_SZ_BITS) - 1;

    return MPL_MIN(max_shortsend, max_representable);
}

#undef FUNCNAME
#define FUNCNAME MPIDI_SHM_am_send_hdr
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int
MPIDI_SHM_am_send_hdr(int rank,
                      MPIR_Comm * comm,
                      int handler_id, const void *am_hdr, size_t am_hdr_sz, void *shm_context)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_SHM_AM_SEND_HDR);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_SHM_AM_SEND_HDR);

    int result = MPIDI_SHMAM_OK;

    MPIDI_SHM_am_header_t msg_hdr = {.handler_id = handler_id,
        .am_hdr_sz = am_hdr_sz,
        .data_sz = 0
    };

    MPIDI_SHM_am_header_t *msg_hdr_p = &msg_hdr;

    struct iovec iov_left[1] = { {.iov_base = (void *) am_hdr,
                                  .iov_len = am_hdr_sz}
    };

    struct iovec *iov_left_ptr = iov_left;

    size_t iov_num_left = 1;

    const int grank = MPIDI_CH4U_rank_to_lpid(rank, comm);

    result = MPIDI_SHMAM_eager_send(grank, &msg_hdr_p, &iov_left_ptr, &iov_num_left, 1);

    mpi_errno = (result == MPIDI_SHMAM_OK) ? MPI_SUCCESS : MPI_ERR_OTHER;

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_SHM_AM_SEND_HDR);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_SHM_am_send_am_hdr_reply
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int
MPIDI_SHM_am_send_hdr_reply(MPIR_Context_id_t context_id,
                            int src_rank, int handler_id, const void *am_hdr, size_t am_hdr_sz)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_SHM_SEND_AM_HDR_REPLY);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_SHM_SEND_AM_HDR_REPLY);

    mpi_errno = MPIDI_SHM_am_send_hdr(src_rank,
                                      MPIDI_CH4U_context_id_to_comm(context_id),
                                      handler_id, am_hdr, am_hdr_sz, NULL);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_SHM_SEND_AM_HDR_REPLY);

    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_SHM_am_recv
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_am_recv(MPIR_Request * req)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_SHM_AM_RECV);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_SHM_AM_RECV);

    MPIDI_CH4U_send_long_ack_msg_t msg;

    msg.sreq_ptr = (MPIDI_CH4U_REQUEST(req, req->rreq.peer_req_ptr));
    msg.rreq_ptr = (uint64_t) req;
    MPIR_Assert((void *) msg.sreq_ptr != NULL);
    mpi_errno = MPIDI_SHM_am_send_hdr_reply(MPIDI_CH4U_get_context(MPIDI_CH4U_REQUEST(req, tag)),
                                            MPIDI_CH4U_REQUEST(req, src_rank),
                                            MPIDI_CH4U_SEND_LONG_ACK, &msg, sizeof(msg));
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_SHM_AM_RECV);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#endif /* SHM_SHMAM_AM_H_INCLUDED */
