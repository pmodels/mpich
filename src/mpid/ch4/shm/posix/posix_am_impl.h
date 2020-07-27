/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef POSIX_AM_IMPL_H_INCLUDED
#define POSIX_AM_IMPL_H_INCLUDED

#include "posix_types.h"
#include "posix_impl.h"
#include "mpidu_genq.h"

int MPIDI_POSIX_deferred_am_isend_issue(MPIDI_POSIX_deferred_am_isend_req_t * dreq);
void MPIDI_POSIX_deferred_am_isend_dequeue(MPIDI_POSIX_deferred_am_isend_req_t * dreq);

static inline int MPIDI_POSIX_am_release_req_hdr(MPIDI_POSIX_am_request_header_t ** req_hdr_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_POSIX_AM_RELEASE_REQ_HDR);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_POSIX_AM_RELEASE_REQ_HDR);

    if ((*req_hdr_ptr)->am_hdr != &(*req_hdr_ptr)->am_hdr_buf[0]) {
        MPL_free((*req_hdr_ptr)->am_hdr);
    }
#ifndef POSIX_AM_REQUEST_INLINE
    MPIDU_genq_private_pool_free_cell(MPIDI_POSIX_global.am_hdr_buf_pool, (*req_hdr_ptr));
#endif

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_POSIX_AM_RELEASE_REQ_HDR);
    return mpi_errno;
}

static inline int MPIDI_POSIX_am_init_req_hdr(const void *am_hdr,
                                              size_t am_hdr_sz,
                                              MPIDI_POSIX_am_request_header_t ** req_hdr_ptr,
                                              MPIR_Request * sreq)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_POSIX_am_request_header_t *req_hdr = *req_hdr_ptr;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_POSIX_AM_INIT_REQ_HDR);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_POSIX_AM_INIT_REQ_HDR);

#ifdef POSIX_AM_REQUEST_INLINE
    if (req_hdr == NULL && sreq != NULL) {
        req_hdr = &MPIDI_POSIX_AMREQUEST(sreq, req_hdr_buffer);
    }
#endif /* POSIX_AM_REQUEST_INLINE */

    if (req_hdr == NULL) {
        MPIDU_genq_private_pool_alloc_cell(MPIDI_POSIX_global.am_hdr_buf_pool, (void **) &req_hdr);
        MPIR_ERR_CHKANDJUMP(!req_hdr, mpi_errno, MPI_ERR_OTHER, "**nomem");

        req_hdr->am_hdr = (void *) &req_hdr->am_hdr_buf[0];
        req_hdr->am_hdr_sz = am_hdr_sz;

        req_hdr->pack_buffer = NULL;
    }

    /* If the header is larger than what we'd preallocated, get rid of the preallocated buffer and
     * create a new one of the correct size. */
    if (am_hdr_sz > MPIDI_POSIX_MAX_AM_HDR_SIZE) {
        if (req_hdr->am_hdr != &req_hdr->am_hdr_buf[0])
            MPL_free(req_hdr->am_hdr);

        req_hdr->am_hdr = MPL_malloc(am_hdr_sz, MPL_MEM_SHM);
        MPIR_ERR_CHKANDJUMP(!(req_hdr->am_hdr), mpi_errno, MPI_ERR_OTHER, "**nomem");
        req_hdr->am_hdr_sz = am_hdr_sz;
    }

    if (am_hdr) {
        MPIR_Typerep_copy(req_hdr->am_hdr, am_hdr, am_hdr_sz);
    }

    *req_hdr_ptr = req_hdr;

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_POSIX_AM_INIT_REQ_HDR);
    return mpi_errno;

  fn_fail:
    goto fn_exit;
}

static inline int MPIDI_POSIX_deferred_am_isend_enqueue(int op, int grank, const void *buf,
                                                        MPI_Aint count, MPI_Datatype datatype,
                                                        MPIDI_POSIX_am_header_t * msg_hdr,
                                                        const void *am_hdr, MPIR_Request * sreq)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_POSIX_deferred_am_isend_req_t *dreq = NULL;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_POSIX_DEFERRED_AM_ISEND_ENQUEUE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_POSIX_DEFERRED_AM_ISEND_ENQUEUE);

    dreq = (MPIDI_POSIX_deferred_am_isend_req_t *)
        MPL_malloc(sizeof(MPIDI_POSIX_deferred_am_isend_req_t), MPL_MEM_OTHER);
    MPIR_ERR_CHKANDJUMP(!dreq, mpi_errno, MPI_ERR_OTHER, "**nomem");

    dreq->op = op;
    dreq->grank = grank;
    dreq->buf = buf;
    dreq->count = count;
    dreq->datatype = datatype;
    dreq->sreq = sreq;
    memcpy(&(dreq->msg_hdr), msg_hdr, sizeof(MPIDI_POSIX_am_header_t));
    dreq->am_hdr = MPL_malloc(msg_hdr->am_hdr_sz, MPL_MEM_OTHER);
    MPIR_ERR_CHKANDJUMP(!dreq->am_hdr, mpi_errno, MPI_ERR_OTHER, "**nomem");
    memcpy(dreq->am_hdr, am_hdr, msg_hdr->am_hdr_sz);

    DL_APPEND(MPIDI_POSIX_global.deferred_am_isend_q, dreq);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_POSIX_DEFERRED_AM_ISEND_ENQUEUE);
    return mpi_errno;
  fn_fail:
    MPL_free(dreq);
    goto fn_exit;
}

static inline int MPIDI_POSIX_do_am_isend_hdr(int grank, MPIDI_POSIX_am_header_t * msg_hdr,
                                              const void *am_hdr)
{
    int mpi_errno = MPI_SUCCESS;
    size_t send_size = 0, eager_buf_sz = 0;
    void *eager_buf = NULL;
    uint8_t *payload = NULL;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_POSIX_DO_AM_ISEND_HDR);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_POSIX_DO_AM_ISEND_HDR);

    if (MPIDI_POSIX_global.deferred_am_isend_q) {
        /* if the deferred queue is not empty, all new ops must be deferred to maintain ordering */
        mpi_errno = MPIDI_POSIX_deferred_am_isend_enqueue(MPIDI_POSIX_DEFERRED_AM_ISEND_OP__HDR,
                                                          grank, NULL, 0, MPI_DATATYPE_NULL,
                                                          msg_hdr, am_hdr, NULL);
        MPIR_ERR_CHECK(mpi_errno);
        goto fn_exit;
    }

    /* Try to get a new cell to hold the message */
    mpi_errno = MPIDI_POSIX_eager_get_buf(&eager_buf, &eager_buf_sz);

    /* If a cell wasn't available, let the caller know that we weren't able to send the message
     * immediately. */
    if (unlikely(!eager_buf)) {
        mpi_errno = MPIDI_POSIX_deferred_am_isend_enqueue(MPIDI_POSIX_DEFERRED_AM_ISEND_OP__HDR,
                                                          grank, NULL, 0, MPI_DATATYPE_NULL,
                                                          msg_hdr, am_hdr, NULL);
        MPIR_ERR_CHECK(mpi_errno);
        goto fn_exit;
    }
    MPIR_ERR_CHECK(mpi_errno);
    payload = eager_buf;

    /* first piece in copy to the payload is the CH4 level AM header */
    mpi_errno = MPIR_Typerep_copy(payload, am_hdr, msg_hdr->am_hdr_sz);
    MPIR_ERR_CHECK(mpi_errno);
    payload += msg_hdr->am_hdr_sz;

    send_size = msg_hdr->am_hdr_sz;

    msg_hdr->data_sz = 0;

    mpi_errno = MPIDI_POSIX_eager_send(eager_buf, send_size, grank, msg_hdr);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_POSIX_DO_AM_ISEND_HDR);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static inline int MPIDI_POSIX_do_am_isend_eager(const void *data, MPI_Aint count,
                                                MPI_Datatype datatype, int grank,
                                                MPIDI_POSIX_am_header_t * msg_hdr,
                                                const void *am_hdr, MPIR_Request * sreq)
{
    int mpi_errno = MPI_SUCCESS;
    int dt_contig = 0;
    size_t data_sz = 0, send_size = 0, eager_buf_sz = 0;
    MPIR_Datatype *dt_ptr = NULL;
    MPI_Aint dt_true_lb = 0;
    void *send_buf = NULL, *eager_buf = NULL;
    uint8_t *payload = NULL;
    bool need_packing = false;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_POSIX_DO_AM_ISEND_EAGER);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_POSIX_DO_AM_ISEND_EAGER);

    if (MPIDI_POSIX_global.deferred_am_isend_q) {
        /* if the deferred queue is not empty, all new ops must be deferred to maintain ordering */
        mpi_errno = MPIDI_POSIX_deferred_am_isend_enqueue(MPIDI_POSIX_DEFERRED_AM_ISEND_OP__EAGER,
                                                          grank, data, count, datatype, msg_hdr,
                                                          am_hdr, sreq);
        MPIR_ERR_CHECK(mpi_errno);
        goto fn_exit;
    }

    /* Try to get a new cell to hold the message */
    mpi_errno = MPIDI_POSIX_eager_get_buf(&eager_buf, &eager_buf_sz);

    /* If a cell wasn't available, let the caller know that we weren't able to send the message
     * immediately. */
    if (unlikely(!eager_buf)) {
        mpi_errno = MPIDI_POSIX_deferred_am_isend_enqueue(MPIDI_POSIX_DEFERRED_AM_ISEND_OP__EAGER,
                                                          grank, data, count, datatype, msg_hdr,
                                                          am_hdr, sreq);
        MPIR_ERR_CHECK(mpi_errno);
        goto fn_exit;
    }
    MPIR_ERR_CHECK(mpi_errno);
    payload = eager_buf;

    /* first piece in copy to the payload is the CH4 level AM header */
    mpi_errno = MPIR_Typerep_copy(payload, am_hdr, msg_hdr->am_hdr_sz);
    MPIR_ERR_CHECK(mpi_errno);
    payload += msg_hdr->am_hdr_sz;

    /* check user datatype */
    MPIDI_Datatype_get_info(count, datatype, dt_contig, data_sz, dt_ptr, dt_true_lb);
    if (data_sz) {
        need_packing = dt_contig ? false : true;
        send_buf = (uint8_t *) data + dt_true_lb;

        send_size = eager_buf_sz - msg_hdr->am_hdr_sz;
        /* unfortunately we need to check for the NULL request and NULL data for the send_hdr case */
        send_size = MPL_MIN(MPIDIG_REQUEST(sreq, data_sz_left), send_size);

        /* check buffer location, we will need packing if user buffer on GPU */
        MPL_pointer_attr_t attr;
        MPIR_GPU_query_pointer_attr(data, &attr);
        if (attr.type == MPL_GPU_POINTER_DEV) {
            /* Force packing of GPU buffer in host memory */
            need_packing = true;
        }

        /* next is user buffer, we pack into the payload if packing is needed.
         * otherwise just regular copy */
        if (unlikely(need_packing)) {
            MPIDI_POSIX_AMREQUEST(sreq, req_hdr) = NULL;
            /* Prepare private storage with information about the pack buffer. */
            mpi_errno = MPIDI_POSIX_am_init_req_hdr(am_hdr, msg_hdr->am_hdr_sz,
                                                    &MPIDI_POSIX_AMREQUEST(sreq, req_hdr), sreq);
            MPIR_ERR_CHECK(mpi_errno);
            MPIDI_POSIX_AMREQUEST(sreq, req_hdr)->pack_buffer = payload;

            MPI_Aint actual_pack_bytes;
            mpi_errno = MPIR_Typerep_pack(data, count, datatype, MPIDIG_REQUEST(sreq, offset),
                                          payload, send_size, &actual_pack_bytes);
            MPIR_ERR_CHECK(mpi_errno);
            send_size = actual_pack_bytes;
        } else {
            send_buf = (char *) send_buf + MPIDIG_REQUEST(sreq, offset);
            mpi_errno = MPIR_Typerep_copy(payload, send_buf, send_size);
            MPIR_ERR_CHECK(mpi_errno);
        }
    } else {
        send_size = 0;
    }
    msg_hdr->data_sz = send_size;

    mpi_errno = MPIDI_POSIX_eager_send(eager_buf, send_size, grank, msg_hdr);

    MPIDIG_REQUEST(sreq, data_sz_left) -= send_size;
    MPIDIG_REQUEST(sreq, offset) += send_size;

    mpi_errno = MPIDIG_global.origin_cbs[msg_hdr->handler_id] (sreq);
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_POSIX_DO_AM_ISEND_EAGER);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static inline int MPIDI_POSIX_do_am_isend_pipeline(const void *data, MPI_Aint count,
                                                   MPI_Datatype datatype, int grank,
                                                   MPIDI_POSIX_am_header_t * msg_hdr,
                                                   const void *am_hdr, MPIR_Request * sreq)
{
    int mpi_errno = MPI_SUCCESS;
    int dt_contig = 0;
    size_t data_sz = 0, send_size = 0, eager_buf_sz = 0;
    MPIR_Datatype *dt_ptr = NULL;
    MPI_Aint dt_true_lb = 0;
    void *send_buf = NULL, *eager_buf = NULL;
    uint8_t *payload = NULL;
    bool need_packing = false;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_POSIX_DO_AM_ISEND_PIPELINE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_POSIX_DO_AM_ISEND_PIPELINE);

    if (MPIDI_POSIX_global.deferred_am_isend_q) {
        /* if the deferred queue is not empty, all new ops must be deferred to maintain ordering */
        mpi_errno =
            MPIDI_POSIX_deferred_am_isend_enqueue(MPIDI_POSIX_DEFERRED_AM_ISEND_OP__PIPELINE, grank,
                                                  data, count, datatype, msg_hdr, am_hdr, sreq);
        MPIR_ERR_CHECK(mpi_errno);
        goto fn_exit;
    }

    /* Try to get a new cell to hold the message */
    mpi_errno = MPIDI_POSIX_eager_get_buf(&eager_buf, &eager_buf_sz);

    /* If a cell wasn't available, let the caller know that we weren't able to send the message
     * immediately. */
    if (unlikely(!eager_buf)) {
        mpi_errno =
            MPIDI_POSIX_deferred_am_isend_enqueue(MPIDI_POSIX_DEFERRED_AM_ISEND_OP__PIPELINE, grank,
                                                  data, count, datatype, msg_hdr, am_hdr, sreq);
        MPIR_ERR_CHECK(mpi_errno);
        goto fn_exit;
    }
    MPIR_ERR_CHECK(mpi_errno);
    payload = eager_buf;

    /* first piece in copy to the payload is the CH4 level AM header */
    mpi_errno = MPIR_Typerep_copy(payload, am_hdr, msg_hdr->am_hdr_sz);
    MPIR_ERR_CHECK(mpi_errno);
    payload += msg_hdr->am_hdr_sz;

    /* check user datatype */
    MPIDI_Datatype_get_info(count, datatype, dt_contig, data_sz, dt_ptr, dt_true_lb);
    need_packing = dt_contig ? false : true;
    send_buf = (uint8_t *) data + dt_true_lb;

    send_size = eager_buf_sz - msg_hdr->am_hdr_sz;
    /* unfortunately we need to check for the NULL request and NULL data for the send_hdr case */
    send_size = MPL_MIN(MPIDIG_REQUEST(sreq, data_sz_left), send_size);

    /* check buffer location, we will need packing if user buffer on GPU */
    MPL_pointer_attr_t attr;
    MPIR_GPU_query_pointer_attr(data, &attr);
    if (attr.type == MPL_GPU_POINTER_DEV) {
        /* Force packing of GPU buffer in host memory */
        need_packing = true;
    }

    /* next is user buffer, we pack into the payload if packing is needed.
     * otherwise just regular copy */
    if (unlikely(need_packing)) {
        /* Prepare private storage with information about the pack buffer. */
        mpi_errno = MPIDI_POSIX_am_init_req_hdr(am_hdr, msg_hdr->am_hdr_sz,
                                                &MPIDI_POSIX_AMREQUEST(sreq, req_hdr), sreq);
        MPIR_ERR_CHECK(mpi_errno);
        MPIDI_POSIX_AMREQUEST(sreq, req_hdr)->pack_buffer = payload;

        MPI_Aint actual_pack_bytes;
        mpi_errno = MPIR_Typerep_pack(data, count, datatype, MPIDIG_REQUEST(sreq, offset), payload,
                                      send_size, &actual_pack_bytes);
        MPIR_ERR_CHECK(mpi_errno);
        send_size = actual_pack_bytes;
    } else {
        send_buf = (char *) send_buf + MPIDIG_REQUEST(sreq, offset);
        mpi_errno = MPIR_Typerep_copy(payload, send_buf, send_size);
        MPIR_ERR_CHECK(mpi_errno);
    }

    msg_hdr->data_sz = send_size;

    mpi_errno = MPIDI_POSIX_eager_send(eager_buf, send_size, grank, msg_hdr);

    MPIDIG_REQUEST(sreq, data_sz_left) -= send_size;
    MPIDIG_REQUEST(sreq, offset) += send_size;
    MPIDIG_REQUEST(sreq, req->plreq).seg_next += 1;

    if (MPIDIG_REQUEST(sreq, data_sz_left) == 0) {
        mpi_errno = MPIDIG_global.origin_cbs[msg_hdr->handler_id] (sreq);
        MPIR_ERR_CHECK(mpi_errno);
    } else {
        mpi_errno =
            MPIDI_POSIX_deferred_am_isend_enqueue(MPIDI_POSIX_DEFERRED_AM_ISEND_OP__PIPELINE, grank,
                                                  data, count, datatype, msg_hdr, am_hdr, sreq);
        MPIR_ERR_CHECK(mpi_errno);
    }

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_POSIX_DO_AM_ISEND_PIPELINE);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#endif /* POSIX_AM_IMPL_H_INCLUDED */
