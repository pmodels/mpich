/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 *  Portions of this code were written by Intel Corporation.
 *  Copyright (C) 2011-2016 Intel Corporation.  Intel provides this material
 *  to Argonne National Laboratory subject to Software Grant and Corporate
 *  Contributor License Agreement dated February 8, 2012.
 */
#ifndef POSIX_PROBE_H_INCLUDED
#define POSIX_PROBE_H_INCLUDED

#include "posix_impl.h"


static inline int MPIDI_POSIX_mpi_improbe(int source,
                                        int tag,
                                        MPIR_Comm * comm,
                                        int context_offset,
                                        int *flag, MPIR_Request ** message, MPI_Status * status)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Request *req, *matched_req = NULL;
    int count = 0;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_POSIX_MPI_IMPROBE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_POSIX_MPI_IMPROBE);

    MPID_THREAD_CS_ENTER(POBJ, MPIDI_POSIX_SHM_MUTEX);
    *message = NULL;

    if (unlikely(source == MPI_PROC_NULL)) {
        MPIR_Status_set_procnull(status);
        *flag = true;
        goto fn_exit;
    }

    for (req = MPIDI_POSIX_recvq_unexpected.head; req; req = MPIDI_POSIX_REQUEST(req)->next) {
        if (MPIDI_POSIX_ENVELOPE_MATCH
            (MPIDI_POSIX_REQUEST(req), source, tag, comm->recvcontext_id + context_offset)) {
            if (!matched_req)
                matched_req = req;

            if (req && MPIDI_POSIX_REQUEST(req)->type == MPIDI_POSIX_TYPEEAGER) {
                *message = matched_req;
                break;
            }
        }
    }

    if (*message) {
        MPIDI_POSIX_request_queue_t mqueue = { NULL, NULL };
        MPIR_Request *prev_req = NULL, *next_req = NULL;
        req = MPIDI_POSIX_recvq_unexpected.head;

        while (req) {
            next_req = MPIDI_POSIX_REQUEST(req)->next;

            if (MPIDI_POSIX_ENVELOPE_MATCH
                (MPIDI_POSIX_REQUEST(req), source, tag, comm->recvcontext_id + context_offset)) {
                if (mqueue.head == NULL) {
                    MPIR_Assert(req == matched_req);
                }

                count += MPIR_STATUS_GET_COUNT(req->status);
                MPIDI_POSIX_REQUEST_DEQUEUE(&req, prev_req, MPIDI_POSIX_recvq_unexpected);
                MPIDI_POSIX_REQUEST_ENQUEUE(req, mqueue);

                if (req && MPIDI_POSIX_REQUEST(req)->type == MPIDI_POSIX_TYPEEAGER)
                    break;
            }
            else
                prev_req = req;

            req = next_req;
        }

        *flag = 1;
        matched_req->kind = MPIR_REQUEST_KIND__MPROBE;
        matched_req->comm = comm;
        MPIR_Comm_add_ref(comm);
        status->MPI_TAG = matched_req->status.MPI_TAG;
        status->MPI_SOURCE = matched_req->status.MPI_SOURCE;
        MPIR_STATUS_SET_COUNT(*status, count);
    }
    else {
        *flag = 0;
        MPID_Progress_test();
    }

  fn_exit:
    MPID_THREAD_CS_EXIT(POBJ, MPIDI_POSIX_SHM_MUTEX);
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_POSIX_MPI_IMPROBE);
    return mpi_errno;
}

static inline int MPIDI_POSIX_mpi_iprobe(int source,
                                       int tag,
                                       MPIR_Comm * comm,
                                       int context_offset, int *flag, MPI_Status * status)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Request *req, *matched_req = NULL;
    int count = 0;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_POSIX_MPI_IPROBE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_POSIX_MPI_IPROBE);
    MPID_THREAD_CS_ENTER(POBJ, MPIDI_POSIX_SHM_MUTEX);

    if (unlikely(source == MPI_PROC_NULL)) {
        MPIR_Status_set_procnull(status);
        *flag = true;
        goto fn_exit;
    }

    for (req = MPIDI_POSIX_recvq_unexpected.head; req; req = MPIDI_POSIX_REQUEST(req)->next) {
        if (MPIDI_POSIX_ENVELOPE_MATCH
            (MPIDI_POSIX_REQUEST(req), source, tag, comm->recvcontext_id + context_offset)) {
            count += MPIR_STATUS_GET_COUNT(req->status);

            if (MPIDI_POSIX_REQUEST(req)->type == MPIDI_POSIX_TYPEEAGER) {
                matched_req = req;
                break;
            }
        }
    }

    if (matched_req) {
        *flag = 1;
        status->MPI_TAG = matched_req->status.MPI_TAG;
        status->MPI_SOURCE = matched_req->status.MPI_SOURCE;
        MPIR_STATUS_SET_COUNT(*status, count);
    }
    else {
        *flag = 0;
        MPID_THREAD_CS_EXIT(POBJ, MPIDI_POSIX_SHM_MUTEX);
        MPID_Progress_test();
        MPID_THREAD_CS_ENTER(POBJ, MPIDI_POSIX_SHM_MUTEX);
    }

  fn_exit:
    MPID_THREAD_CS_EXIT(POBJ, MPIDI_POSIX_SHM_MUTEX);
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_POSIX_MPI_IPROBE);
    return mpi_errno;
}

#endif /* POSIX_PROBE_H_INCLUDED */
