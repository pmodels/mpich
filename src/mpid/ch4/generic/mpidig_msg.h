/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2020 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#ifndef MPIDIG_MSG_H_INCLUDED
#define MPIDIG_MSG_H_INCLUDED

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

/* synchronous single-payload data transfer. This is the common case */
MPL_STATIC_INLINE_PREFIX void MPIDIG_recv_copy(void *in_data, MPI_Aint in_data_sz,
                                               void *data, MPI_Aint data_sz,
                                               int is_contig, MPIR_Request * rreq)
{
    if (!data || !data_sz) {
        /* empty case */
        MPIR_STATUS_SET_COUNT(rreq->status, 0);
    } else if (is_contig) {
        /* contig case */
        if (in_data_sz > data_sz) {
            rreq->status.MPI_ERROR = MPIDIG_ERR_TRUNCATE(data_sz, in_data_sz);
        }

        data_sz = MPL_MIN(data_sz, in_data_sz);
        MPIR_Memcpy(data, in_data, data_sz);
        MPIR_STATUS_SET_COUNT(rreq->status, data_sz);
    } else {
        /* noncontig case */
        struct iovec *iov = (struct iovec *) data;
        int iov_len = data_sz;

        int done = 0;
        int rem = in_data_sz;
        for (int i = 0; i < iov_len && rem > 0; i++) {
            int curr_len = MPL_MIN(rem, iov[i].iov_len);
            MPIR_Memcpy(iov[i].iov_base, (char *) in_data + done, curr_len);
            rem -= curr_len;
            done += curr_len;
        }

        if (rem) {
            data_sz = in_data_sz - rem;
            rreq->status.MPI_ERROR = MPIDIG_ERR_TRUNCATE(data_sz, in_data_sz);
        }

        MPIR_STATUS_SET_COUNT(rreq->status, done);
    }
    /* all done */
}

/* setup for asynchronous multi-segment data transfer (ref posix_progress) */
MPL_STATIC_INLINE_PREFIX void MPIDIG_recv_setup(int is_contig, MPI_Aint in_data_sz,
                                                void *data, MPI_Aint data_sz, MPIR_Request * rreq)
{
    MPIDIG_rreq_async_t *p = &(MPIDIG_REQUEST(rreq, req->async));
    p->is_contig = is_contig;
    p->in_data_sz = in_data_sz;
    if (is_contig) {
        p->iov_one.iov_base = data;
        p->iov_one.iov_len = data_sz;
        p->iov_ptr = &(p->iov_one);
        p->iov_num = 1;
        if (in_data_sz > data_sz) {
            rreq->status.MPI_ERROR = MPIDIG_ERR_TRUNCATE(data_sz, in_data_sz);
            MPIR_STATUS_SET_COUNT(rreq->status, data_sz);
        } else {
            MPIR_STATUS_SET_COUNT(rreq->status, in_data_sz);
        }
    } else {
        p->iov_ptr = data;
        p->iov_num = data_sz;

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

#endif /* MPIDIG_MSG_H_INCLUDED */
