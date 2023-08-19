/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef MPIDIG_RECV_UTILS_H_INCLUDED
#define MPIDIG_RECV_UTILS_H_INCLUDED

/* This file is for supporting routines used for generic layer message matching
 * and data transfer. They are used by protocols such as send, ssend, and send_long.
 * Other protocols that share semantic with message transport may also use these
 * routines to avoid code duplications.
 */

#define MPIDIG_ERR_TRUNCATE(data_sz, in_data_sz) \
    MPIR_Err_create_code(rreq->status.MPI_ERROR, MPIR_ERR_RECOVERABLE, \
                         __func__, __LINE__, \
                         MPI_ERR_TRUNCATE, "**truncate", "**truncate %d %d %d %d", \
                         rreq->status.MPI_SOURCE, rreq->status.MPI_TAG, \
                         (int) data_sz, (int) in_data_sz)

MPL_STATIC_INLINE_PREFIX void MPIDIG_recv_set_buffer_attr(MPIR_Request * rreq)
{
    MPL_pointer_attr_t attr;
    MPIR_GPU_query_pointer_attr(MPIDIG_REQUEST(rreq, buffer), &attr);

    MPIDIG_rreq_async_t *p = &(MPIDIG_REQUEST(rreq, req->recv_async));
    p->is_device_buffer = (attr.type == MPL_GPU_POINTER_DEV);
}

MPL_STATIC_INLINE_PREFIX int MPIDIG_recv_check_rndv_cb(MPIR_Request * rreq)
{
    int mpi_errno = MPI_SUCCESS;

    if (MPIDIG_REQUEST(rreq, req->recv_async).data_copy_cb) {
        /* the callback may complete the request. Increment the ref to prevent
         * rreq from being freed */
        MPIR_Request_add_ref(rreq);

        mpi_errno = MPIDIG_REQUEST(rreq, req->recv_async).data_copy_cb(rreq);
        MPIR_ERR_CHECK(mpi_errno);

        if (MPIDIG_REQUEST(rreq, rndv_hdr)) {
            MPL_free(MPIDIG_REQUEST(rreq, rndv_hdr));
            MPIDIG_REQUEST(rreq, rndv_hdr) = NULL;
        }
        /* the data_copy_cb may complete rreq (e.g. ucx am_send_hdr may invoke
         * progress recursively and finish several callbacks before it return.
         * Thus we need check MPIDIG_REQUEST(rreq, req) still there. */
        if (MPIDIG_REQUEST(rreq, req)) {
            MPIDIG_REQUEST(rreq, req->recv_async).data_copy_cb = NULL;
        }
        MPIR_Request_free_unsafe(rreq);
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

/* caching recv buffer information */
MPL_STATIC_INLINE_PREFIX int MPIDIG_recv_type_init(MPI_Aint in_data_sz, MPIR_Request * rreq)
{
    int mpi_errno = MPI_SUCCESS;

    MPIDIG_rreq_async_t *p = &(MPIDIG_REQUEST(rreq, req->recv_async));
    p->recv_type = MPIDIG_RECV_DATATYPE;
    p->in_data_sz = in_data_sz;

    MPIDIG_recv_set_buffer_attr(rreq);

    MPI_Aint max_data_size;
    MPIR_Datatype_get_size_macro(MPIDIG_REQUEST(rreq, datatype), max_data_size);
    max_data_size *= MPIDIG_REQUEST(rreq, count);
    if (in_data_sz > max_data_size) {
        rreq->status.MPI_ERROR = MPIDIG_ERR_TRUNCATE(max_data_size, in_data_sz);
    }

    mpi_errno = MPIDIG_recv_check_rndv_cb(rreq);
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPIDIG_recv_init(int is_contig, MPI_Aint in_data_sz,
                                              void *data, MPI_Aint data_sz, MPIR_Request * rreq)
{
    int mpi_errno = MPI_SUCCESS;

    MPIDIG_rreq_async_t *p = &(MPIDIG_REQUEST(rreq, req->recv_async));
    p->in_data_sz = in_data_sz;
    if (is_contig) {
        p->recv_type = MPIDIG_RECV_CONTIG;
        p->iov_one.iov_base = data;
        p->iov_one.iov_len = data_sz;
    } else {
        p->recv_type = MPIDIG_RECV_IOV;
        p->iov_ptr = data;
        p->iov_num = data_sz;
    }

    MPIDIG_recv_set_buffer_attr(rreq);

    mpi_errno = MPIDIG_recv_check_rndv_cb(rreq);
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX void MPIDIG_recv_finish(MPIR_Request * rreq)
{
    /* Free the iov array if we allocated it */
    if (MPIDIG_REQUEST(rreq, req->status) & MPIDIG_REQ_RCV_NON_CONTIG) {
        MPL_free(MPIDIG_REQUEST(rreq, req->iov));
    }
}

/* Transport-specific data copy, such as RDMA, need explicit iov pointers.
 * Providing helper routine keeps the internal of MPIDIG_rreq_async_t here.
 */
MPL_STATIC_INLINE_PREFIX void MPIDIG_convert_datatype(MPIR_Request * rreq);
MPL_STATIC_INLINE_PREFIX void MPIDIG_get_recv_data(int *is_contig, int *is_gpu, void **p_data,
                                                   MPI_Aint * p_data_sz, MPIR_Request * rreq)
{
    MPIDIG_rreq_async_t *p = &(MPIDIG_REQUEST(rreq, req->recv_async));
    if (p->recv_type == MPIDIG_RECV_DATATYPE) {
        MPIDIG_convert_datatype(rreq);
        MPIR_Assert(p->recv_type == MPIDIG_RECV_CONTIG || p->recv_type == MPIDIG_RECV_IOV);
    }

    if (p->recv_type == MPIDIG_RECV_CONTIG) {
        *is_contig = 1;
        *p_data = p->iov_one.iov_base;
        *p_data_sz = p->iov_one.iov_len;
    } else {
        *is_contig = 0;
        *p_data = p->iov_ptr;
        *p_data_sz = p->iov_num;
    }
    *is_gpu = p->is_device_buffer;
}

/* Some transport (e.g. ucx) can optimize a buffer transfer. Return buffer
 * address and associated information.
 */
MPL_STATIC_INLINE_PREFIX void MPIDIG_get_recv_buffer(void **p_data, MPI_Aint * p_data_sz,
                                                     bool * is_contig, MPI_Aint * in_data_sz,
                                                     MPIR_Request * rreq)
{
    MPIDIG_rreq_async_t *p = &(MPIDIG_REQUEST(rreq, req->recv_async));
    /* transport may need handle the case when promised data size and posted
     * buffer size mismatch */
    *in_data_sz = p->in_data_sz;
    if (p->recv_type == MPIDIG_RECV_DATATYPE) {
        int dt_contig;
        MPI_Aint data_sz;
        MPIR_Datatype *dt_ptr;
        MPI_Aint dt_true_lb;
        MPIDI_Datatype_get_info(MPIDIG_REQUEST(rreq, count), MPIDIG_REQUEST(rreq, datatype),
                                dt_contig, data_sz, dt_ptr, dt_true_lb);
        if (dt_contig) {
            *p_data = (char *) MPIDIG_REQUEST(rreq, buffer) + dt_true_lb;
        } else {
            *p_data = (char *) MPIDIG_REQUEST(rreq, buffer);
        }
        *is_contig = dt_contig;
        *p_data_sz = data_sz;
    } else if (p->recv_type == MPIDIG_RECV_CONTIG) {
        *is_contig = true;
        *p_data = p->iov_one.iov_base;
        *p_data_sz = p->iov_one.iov_len;
    } else {
        MPI_Aint data_sz = 0;
        for (int i = 0; i < p->iov_num; i++) {
            data_sz += p->iov_ptr[i].iov_len;
        }
        *is_contig = false;
        *p_data = NULL;
        *p_data_sz = data_sz;
    }
    /* process truncation error now */
    if (*in_data_sz > *p_data_sz) {
        rreq->status.MPI_ERROR = MPIDIG_ERR_TRUNCATE(*p_data_sz, *in_data_sz);
    }
}

/* Sometime the transport just need info to make algorithm choice */
MPL_STATIC_INLINE_PREFIX int MPIDIG_get_recv_iov_count(MPIR_Request * rreq)
{
    MPIDIG_rreq_async_t *p = &(MPIDIG_REQUEST(rreq, req->recv_async));
    if (p->recv_type == MPIDIG_RECV_DATATYPE) {
        MPI_Aint num_iov;
        MPIR_Typerep_get_iov_len(MPIDIG_REQUEST(rreq, count),
                                 MPIDIG_REQUEST(rreq, datatype), &num_iov);
        return num_iov;
    } else if (p->recv_type == MPIDIG_RECV_CONTIG) {
        return 1;
    } else {
        return p->iov_num;
    }
}

/* If the transport handles the data copy, call MPIDIG_recv_done to set status */
MPL_STATIC_INLINE_PREFIX void MPIDIG_recv_done(MPI_Aint got_data_sz, MPIR_Request * rreq)
{
    MPIR_STATUS_SET_COUNT(rreq->status, got_data_sz);
}

/* synchronous single-payload data transfer. This is the common case */
/* TODO: if transport flag callback, synchronous copy can/should be done inside the callback */
MPL_STATIC_INLINE_PREFIX void MPIDIG_recv_copy(void *in_data, MPIR_Request * rreq)
{
    MPIDIG_rreq_async_t *p = &(MPIDIG_REQUEST(rreq, req->recv_async));
    MPI_Aint in_data_sz = p->in_data_sz;
    if (in_data_sz == 0) {
        /* otherwise if recv size = 0, it is at least a truncation error */
        MPIR_STATUS_SET_COUNT(rreq->status, 0);
    } else if (p->recv_type == MPIDIG_RECV_DATATYPE) {
        MPI_Aint actual_unpack_bytes;
        MPIR_Typerep_unpack(in_data, in_data_sz,
                            MPIDIG_REQUEST(rreq, buffer),
                            MPIDIG_REQUEST(rreq, count),
                            MPIDIG_REQUEST(rreq, datatype),
                            0, &actual_unpack_bytes, MPIR_TYPEREP_FLAG_NONE);
        if (!rreq->status.MPI_ERROR && in_data_sz > actual_unpack_bytes) {
            /* Truncation error has been checked at MPIDIG_recv_type_init.
             * If the receive buffer had enough space, but we still
             * couldn't unpack the data, it means that the basic
             * datatype from the sender doesn't match that of the
             * receiver.  This doesn't catch all errors; for example,
             * cases where the sizes match but the basic datatypes are
             * still different. */
            rreq->status.MPI_ERROR =
                MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, __func__, __LINE__,
                                     MPI_ERR_TYPE, "**dtypemismatch", 0);
        }
        MPIR_STATUS_SET_COUNT(rreq->status, actual_unpack_bytes);
    } else if (p->recv_type == MPIDIG_RECV_CONTIG) {
        /* contig case */
        void *data = p->iov_one.iov_base;
        MPI_Aint data_sz = p->iov_one.iov_len;
        if (in_data_sz > data_sz) {
            rreq->status.MPI_ERROR = MPIDIG_ERR_TRUNCATE(data_sz, in_data_sz);
        }

        data_sz = MPL_MIN(data_sz, in_data_sz);
        MPIR_Typerep_copy(data, in_data, data_sz, MPIR_TYPEREP_FLAG_NONE);
        MPIR_STATUS_SET_COUNT(rreq->status, data_sz);
    } else {
        /* noncontig case */
        struct iovec *iov = p->iov_ptr;
        int iov_len = p->iov_num;

        int done = 0;
        int rem = in_data_sz;
        for (int i = 0; i < iov_len && rem > 0; i++) {
            int curr_len = MPL_MIN(rem, iov[i].iov_len);
            MPIR_Typerep_copy(iov[i].iov_base, (char *) in_data + done, curr_len,
                              MPIR_TYPEREP_FLAG_NONE);
            rem -= curr_len;
            done += curr_len;
        }

        if (rem) {
            MPI_Aint data_sz = in_data_sz - rem;
            rreq->status.MPI_ERROR = MPIDIG_ERR_TRUNCATE(data_sz, in_data_sz);
        }

        MPIR_STATUS_SET_COUNT(rreq->status, done);
    }
    /* all done */
}

/* setup for asynchronous multi-segment data transfer (ref posix_progress) */
MPL_STATIC_INLINE_PREFIX void MPIDIG_recv_setup(MPIR_Request * rreq)
{
    MPIDIG_rreq_async_t *p = &(MPIDIG_REQUEST(rreq, req->recv_async));
    p->offset = 0;
    if (p->recv_type == MPIDIG_RECV_DATATYPE) {
        /* it's ready, rreq status to be set */
    } else if (p->recv_type == MPIDIG_RECV_CONTIG) {
        p->iov_ptr = &(p->iov_one);
        p->iov_num = 1;

        MPI_Aint in_data_sz = p->in_data_sz;
        MPI_Aint data_sz = p->iov_one.iov_len;
        if (in_data_sz > data_sz) {
            rreq->status.MPI_ERROR = MPIDIG_ERR_TRUNCATE(data_sz, in_data_sz);
            MPIR_STATUS_SET_COUNT(rreq->status, data_sz);
        } else {
            MPIR_STATUS_SET_COUNT(rreq->status, in_data_sz);
        }
    } else {
        /* MPIDIG_RECV_IOV */
        MPI_Aint in_data_sz = p->in_data_sz;
        MPI_Aint recv_sz = 0;
        for (int i = 0; i < p->iov_num; i++) {
            recv_sz += p->iov_ptr[i].iov_len;
        }
        if (in_data_sz > recv_sz) {
            rreq->status.MPI_ERROR = MPIDIG_ERR_TRUNCATE(recv_sz, in_data_sz);
            MPIR_STATUS_SET_COUNT(rreq->status, recv_sz);
        } else {
            MPIR_STATUS_SET_COUNT(rreq->status, in_data_sz);
        }
    }
}

/* copy a segment to data buffer recorded in rreq. */
MPL_STATIC_INLINE_PREFIX int MPIDIG_recv_copy_seg(void *payload, MPI_Aint payload_sz,
                                                  MPIR_Request * rreq)
{
    MPIDIG_rreq_async_t *p = &(MPIDIG_REQUEST(rreq, req->recv_async));
    p->in_data_sz -= payload_sz;

    if (rreq->status.MPI_ERROR) {
        /* we already detected error, don't bother copying data, just check whether
         * it's the last segment */
        return (p->in_data_sz == 0);
    } else if (p->recv_type == MPIDIG_RECV_DATATYPE) {
        MPI_Aint actual_unpack_bytes;
        MPIR_Typerep_unpack(payload, payload_sz,
                            MPIDIG_REQUEST(rreq, buffer),
                            MPIDIG_REQUEST(rreq, count),
                            MPIDIG_REQUEST(rreq, datatype),
                            p->offset, &actual_unpack_bytes, MPIR_TYPEREP_FLAG_NONE);
        p->offset += payload_sz;
        if (payload_sz > actual_unpack_bytes) {
            /* basic element size mismatch */
            rreq->status.MPI_ERROR =
                MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, __func__, __LINE__,
                                     MPI_ERR_TYPE, "**dtypemismatch", 0);
            return (p->in_data_sz == 0);
        } else if (p->in_data_sz == 0) {
            /* done */
            MPIR_STATUS_SET_COUNT(rreq->status, p->offset);
            return 1;
        } else {
            /* not done */
            return 0;
        }
    } else {
        /* MPIDIG_RECV_CONTIG and MPIDIG_RECV_IOV */
        p->offset += payload_sz;
        int iov_done = 0;
        for (int i = 0; i < p->iov_num; i++) {
            if (payload_sz < p->iov_ptr[i].iov_len) {
                MPIR_Typerep_copy(p->iov_ptr[i].iov_base, payload, payload_sz,
                                  MPIR_TYPEREP_FLAG_NONE);
                p->iov_ptr[i].iov_base = (char *) p->iov_ptr[i].iov_base + payload_sz;
                p->iov_ptr[i].iov_len -= payload_sz;
                /* not done */
                break;
            } else {
                /* fill one iov */
                MPIR_Typerep_copy(p->iov_ptr[i].iov_base, payload, p->iov_ptr[i].iov_len,
                                  MPIR_TYPEREP_FLAG_NONE);
                payload = (char *) payload + p->iov_ptr[i].iov_len;
                payload_sz -= p->iov_ptr[i].iov_len;
                iov_done++;
            }
        }
        p->iov_num -= iov_done;
        p->iov_ptr += iov_done;

        if (p->iov_num == 0 || p->in_data_sz == 0) {
            /* all done */
            return 1;
        } else {
            /* not done */
            return 0;
        }
    }
}

MPL_STATIC_INLINE_PREFIX bool MPIDIG_recv_initialized(MPIR_Request * rreq)
{
    return MPIDIG_REQUEST(rreq, req->recv_async).recv_type != MPIDIG_RECV_NONE;
}

MPL_STATIC_INLINE_PREFIX MPI_Aint MPIDIG_recv_in_data_sz(MPIR_Request * rreq)
{
    return MPIDIG_REQUEST(rreq, req->recv_async).in_data_sz;
}

MPL_STATIC_INLINE_PREFIX void MPIDIG_recv_set_data_copy_cb(MPIR_Request * rreq,
                                                           MPIDIG_recv_data_copy_cb cb)
{
    MPIDIG_REQUEST(rreq, req->recv_async).data_copy_cb = cb;
}

/* internal routines */
MPL_STATIC_INLINE_PREFIX void MPIDIG_convert_datatype(MPIR_Request * rreq)
{
    int dt_contig;
    MPI_Aint data_sz;
    MPIR_Datatype *dt_ptr;
    MPI_Aint dt_true_lb;
    MPIDI_Datatype_get_info(MPIDIG_REQUEST(rreq, count),
                            MPIDIG_REQUEST(rreq, datatype), dt_contig, data_sz, dt_ptr, dt_true_lb);

    MPIDIG_rreq_async_t *p = &(MPIDIG_REQUEST(rreq, req->recv_async));
    if (dt_contig) {
        p->recv_type = MPIDIG_RECV_CONTIG;
        p->iov_one.iov_base = MPIR_get_contig_ptr(MPIDIG_REQUEST(rreq, buffer), dt_true_lb);
        p->iov_one.iov_len = data_sz;
    } else {
        struct iovec *iov;
        MPI_Aint num_iov;
        MPIR_Typerep_get_iov_len(MPIDIG_REQUEST(rreq, count),
                                 MPIDIG_REQUEST(rreq, datatype), &num_iov);
        MPIR_Assert(num_iov > 0);

        iov = MPL_malloc(num_iov * sizeof(struct iovec), MPL_MEM_OTHER);
        MPIR_Assert(iov);
        /* save it to be freed at cmpl */
        MPIDIG_REQUEST(rreq, req->iov) = iov;
        MPIDIG_REQUEST(rreq, req->status) |= MPIDIG_REQ_RCV_NON_CONTIG;

        MPI_Aint actual_iov_len;
        MPIR_Typerep_to_iov_offset(MPIDIG_REQUEST(rreq, buffer), MPIDIG_REQUEST(rreq, count),
                                   MPIDIG_REQUEST(rreq, datatype), 0, iov, num_iov,
                                   &actual_iov_len);

        p->recv_type = MPIDIG_RECV_IOV;
        p->iov_ptr = iov;
        p->iov_num = num_iov;
    }
}

#endif /* MPIDIG_RECV_UTILS_H_INCLUDED */
