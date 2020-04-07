/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2020 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#ifndef MPIDIG_AM_MSG_H_INCLUDED
#define MPIDIG_AM_MSG_H_INCLUDED

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

/* caching recv buffer information */
MPL_STATIC_INLINE_PREFIX void MPIDIG_recv_type_init(MPI_Aint in_data_sz, MPIR_Request * rreq)
{
    MPIDIG_rreq_async_t *p = &(MPIDIG_REQUEST(rreq, req->async));
    p->recv_type = MPIDIG_RECV_DATATYPE;
    p->in_data_sz = in_data_sz;
}

MPL_STATIC_INLINE_PREFIX void MPIDIG_recv_init(int is_contig, MPI_Aint in_data_sz,
                                               void *data, MPI_Aint data_sz, MPIR_Request * rreq)
{
    MPIDIG_rreq_async_t *p = &(MPIDIG_REQUEST(rreq, req->async));
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
}

/* Transport-specific data copy, such as RDMA, need explicit iov pointers.
 * Providing helper routine keeps the internal of MPIDIG_rreq_async_t here.
 */
MPL_STATIC_INLINE_PREFIX void mpidig_convert_datatype(MPIR_Request * rreq);
MPL_STATIC_INLINE_PREFIX void MPIDIG_get_recv_data(int *is_contig, void **p_data,
                                                   MPI_Aint * p_data_sz, MPIR_Request * rreq)
{
    MPIDIG_rreq_async_t *p = &(MPIDIG_REQUEST(rreq, req->async));
    if (p->recv_type == MPIDIG_RECV_DATATYPE) {
        mpidig_convert_datatype(rreq);
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
}

/* synchronous single-payload data transfer. This is the common case */
/* TODO: if transport flag callback, synchronous copy can/should be done inside the callback */
MPL_STATIC_INLINE_PREFIX void MPIDIG_recv_copy(void *in_data, MPIR_Request * rreq)
{
    MPIDIG_rreq_async_t *p = &(MPIDIG_REQUEST(rreq, req->async));
    MPI_Aint in_data_sz = p->in_data_sz;
    if (in_data_sz == 0) {
        /* otherwise if recv size = 0, it is at least a truncation error */
        MPIR_STATUS_SET_COUNT(rreq->status, 0);
    } else if (p->recv_type == MPIDIG_RECV_DATATYPE) {
        MPI_Aint actual_unpack_bytes;
        MPIR_Typerep_unpack(in_data, in_data_sz,
                            MPIDIG_REQUEST(rreq, buffer),
                            MPIDIG_REQUEST(rreq, count),
                            MPIDIG_REQUEST(rreq, datatype), 0, &actual_unpack_bytes);
        if (in_data_sz > actual_unpack_bytes) {
            rreq->status.MPI_ERROR = MPIDIG_ERR_TRUNCATE(actual_unpack_bytes, in_data_sz);
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
        MPIR_Memcpy(data, in_data, data_sz);
        MPIR_STATUS_SET_COUNT(rreq->status, data_sz);
    } else {
        /* noncontig case */
        struct iovec *iov = p->iov_ptr;
        int iov_len = p->iov_num;

        int done = 0;
        int rem = in_data_sz;
        for (int i = 0; i < iov_len && rem > 0; i++) {
            int curr_len = MPL_MIN(rem, iov[i].iov_len);
            MPIR_Memcpy(iov[i].iov_base, (char *) in_data + done, curr_len);
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
    MPIDIG_rreq_async_t *p = &(MPIDIG_REQUEST(rreq, req->async));
    if (p->recv_type == MPIDIG_RECV_DATATYPE) {
        p->offset = 0;
        /* rreq status to be set */
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
    MPIDIG_rreq_async_t *p = &(MPIDIG_REQUEST(rreq, req->async));

    if (p->recv_type == MPIDIG_RECV_DATATYPE) {
        MPI_Aint actual_unpack_bytes;
        MPIR_Typerep_unpack(payload, payload_sz,
                            MPIDIG_REQUEST(rreq, buffer),
                            MPIDIG_REQUEST(rreq, count),
                            MPIDIG_REQUEST(rreq, datatype), p->offset, &actual_unpack_bytes);
        p->in_data_sz -= actual_unpack_bytes;
        p->offset += actual_unpack_bytes;
        if (payload_sz > actual_unpack_bytes) {
            /* did not fit */
            rreq->status.MPI_ERROR = MPIDIG_ERR_TRUNCATE(p->offset, p->offset + p->in_data_sz);
            MPIR_STATUS_SET_COUNT(rreq->status, p->offset);
            return 1;
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
        p->in_data_sz -= payload_sz;
        int iov_done = 0;
        for (int i = 0; i < p->iov_num; i++) {
            if (payload_sz < p->iov_ptr[i].iov_len) {
                MPIR_Memcpy(p->iov_ptr[i].iov_base, payload, payload_sz);
                p->iov_ptr[i].iov_base = (char *) p->iov_ptr[i].iov_base + payload_sz;
                p->iov_ptr[i].iov_len -= payload_sz;
                /* not done */
                break;
            } else {
                /* fill one iov */
                MPIR_Memcpy(p->iov_ptr[i].iov_base, payload, p->iov_ptr[i].iov_len);
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

/* internal routines */
MPL_STATIC_INLINE_PREFIX void mpidig_convert_datatype(MPIR_Request * rreq)
{
    int dt_contig;
    MPI_Aint data_sz;
    MPIR_Datatype *dt_ptr;
    MPI_Aint dt_true_lb;
    MPIDI_Datatype_get_info(MPIDIG_REQUEST(rreq, count), MPIDIG_REQUEST(rreq, datatype),
                            dt_contig, data_sz, dt_ptr, dt_true_lb);

    MPIDIG_rreq_async_t *p = &(MPIDIG_REQUEST(rreq, req->async));
    if (dt_contig) {
        p->recv_type = MPIDIG_RECV_CONTIG;
        p->iov_one.iov_base = (char *) MPIDIG_REQUEST(rreq, buffer) + dt_true_lb;
        p->iov_one.iov_len = data_sz;
    } else {
        struct iovec *iov;
        MPI_Aint num_iov;
        MPIR_Typerep_iov_len(MPIDIG_REQUEST(rreq, buffer), MPIDIG_REQUEST(rreq, count),
                             MPIDIG_REQUEST(rreq, datatype), 0, data_sz, &num_iov);
        MPIR_Assert(num_iov > 0);

        iov = MPL_malloc(num_iov * sizeof(struct iovec), MPL_MEM_OTHER);
        MPIR_Assert(iov);
        /* save it to be freed at cmpl */
        MPIDIG_REQUEST(rreq, req->iov) = iov;
        MPIDIG_REQUEST(rreq, req->status) |= MPIDIG_REQ_RCV_NON_CONTIG;

        int actual_iov_len;
        MPI_Aint actual_iov_bytes;
        MPIR_Typerep_to_iov(MPIDIG_REQUEST(rreq, buffer), MPIDIG_REQUEST(rreq, count),
                            MPIDIG_REQUEST(rreq, datatype), 0, iov, num_iov,
                            p->in_data_sz, &actual_iov_len, &actual_iov_bytes);

        if (actual_iov_bytes != p->in_data_sz) {
            rreq->status.MPI_ERROR = MPI_ERR_TYPE;
        }

        p->recv_type = MPIDIG_RECV_IOV;
        p->iov_ptr = iov;
        p->iov_num = num_iov;
    }
}

#endif /* MPIDIG_AM_MSG_H_INCLUDED */
