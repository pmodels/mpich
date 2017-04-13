/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 *  Portions of this code were written by Intel Corporation.
 *  Copyright (C) 2011-2012 Intel Corporation.  Intel provides this material
 *  to Argonne National Laboratory subject to Software Grant and Corporate
 *  Contributor License Agreement dated February 8, 2012.
 */
#ifndef PTL_PROGRESS_H_INCLUDED
#define PTL_PROGRESS_H_INCLUDED

#include "ptl_impl.h"

static inline int MPIDI_PTL_am_handler(ptl_event_t * e)
{
    int mpi_errno;
    MPIR_Request *rreq = NULL;
    void *p_data;
    void *in_data;
    size_t data_sz, in_data_sz;
    MPIDIG_am_target_cmpl_cb target_cmpl_cb = NULL;
    struct iovec *iov;
    int i, is_contig, iov_len;
    size_t done, curr_len, rem;

    in_data_sz = data_sz = (e->hdr_data & MPIDI_PTL_MSG_SZ_MASK);
    in_data = p_data = (e->start + (e->mlength - data_sz));
    int handler_id = e->hdr_data >> 56;

    MPIDIG_global.target_msg_cbs[handler_id] (handler_id, e->start,
                                              &p_data, &data_sz, &is_contig, &target_cmpl_cb,
                                              &rreq);

    if (!rreq)
        goto fn_exit;

    if ((!p_data || !data_sz) && target_cmpl_cb) {
        target_cmpl_cb(rreq);
        goto fn_exit;
    }

    if (is_contig) {
        if (in_data_sz > data_sz) {
            rreq->status.MPI_ERROR = MPI_ERR_TRUNCATE;
        }
        else {
            rreq->status.MPI_ERROR = MPI_SUCCESS;
        }

        data_sz = MPL_MIN(data_sz, in_data_sz);
        MPIR_Memcpy(p_data, in_data, data_sz);
        MPIR_STATUS_SET_COUNT(rreq->status, data_sz);
    }
    else {
        done = 0;
        rem = in_data_sz;
        iov = (struct iovec *) p_data;
        iov_len = data_sz;

        for (i = 0; i < iov_len && rem > 0; i++) {
            curr_len = MPL_MIN(rem, iov[i].iov_len);
            MPIR_Memcpy(iov[i].iov_base, (char *) in_data + done, curr_len);
            rem -= curr_len;
            done += curr_len;
        }

        if (rem) {
            rreq->status.MPI_ERROR = MPI_ERR_TRUNCATE;
        }
        else {
            rreq->status.MPI_ERROR = MPI_SUCCESS;
        }

        MPIR_STATUS_SET_COUNT(rreq->status, done);
    }

    if (target_cmpl_cb) {
        target_cmpl_cb(rreq);
    }

  fn_exit:
    return mpi_errno;
}

static inline int MPIDI_NM_progress(int blocking)
{
    ptl_event_t e;
    unsigned int which;

    while (PtlEQPoll(MPIDI_PTL_global.eqs, 2, 0, &e, &which) != PTL_EQ_EMPTY) {
        switch (e.type) {
        case PTL_EVENT_PUT:
            MPIR_Assert(e.ptl_list == PTL_OVERFLOW_LIST);
            MPIDI_PTL_am_handler(&e);
            break;
        case PTL_EVENT_ACK:
            {
                int count;
                MPIR_Request *sreq = (MPIR_Request *) e.user_ptr;
                int handler_id = sreq->dev.ch4.am.netmod_am.portals4.handler_id;

                MPIR_cc_decr(sreq->cc_ptr, &count);
                MPIR_Assert(count >= 0);

                if (count == 0) {
                    MPIR_Request_free(sreq);
                    break;
                }
                MPIDIG_global.origin_cbs[handler_id] (sreq);
            }
            break;
        case PTL_EVENT_AUTO_UNLINK:
            MPIDI_PTL_global.overflow_me_handles[(size_t) e.user_ptr] = PTL_INVALID_HANDLE;
            break;
        case PTL_EVENT_AUTO_FREE:
            MPIDI_PTL_append_overflow((size_t) e.user_ptr);
            break;
        case PTL_EVENT_SEND:
            break;
        default:
            printf("ABORT: event = %d\n", e.type);
            abort();
        }
    }

    return MPI_SUCCESS;
}

static inline int MPIDI_NM_progress_test(void)
{
    MPIR_Assert(0);
    return MPI_SUCCESS;
}

static inline int MPIDI_NM_progress_poke(void)
{
    MPIR_Assert(0);
    return MPI_SUCCESS;
}

static inline void MPIDI_NM_progress_start(MPID_Progress_state * state)
{
    MPIR_Assert(0);
    return;
}

static inline void MPIDI_NM_progress_end(MPID_Progress_state * state)
{
    MPIR_Assert(0);
    return;
}

static inline int MPIDI_NM_progress_wait(MPID_Progress_state * state)
{
    MPIR_Assert(0);
    return MPI_SUCCESS;
}

static inline int MPIDI_NM_progress_register(int (*progress_fn) (int *), int *id)
{
    MPIR_Assert(0);
    return MPI_SUCCESS;
}

static inline int MPIDI_NM_progress_deregister(int id)
{
    MPIR_Assert(0);
    return MPI_SUCCESS;
}

static inline int MPIDI_NM_progress_activate(int id)
{
    MPIR_Assert(0);
    return MPI_SUCCESS;
}

static inline int MPIDI_NM_progress_deactivate(int id)
{
    MPIR_Assert(0);
    return MPI_SUCCESS;
}

#endif /* PTL_PROGRESS_H_INCLUDED */
