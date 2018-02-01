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
#ifndef POSIX_STARTALL_H_INCLUDED
#define POSIX_STARTALL_H_INCLUDED

#include "posix_impl.h"
#include "ch4_impl.h"

#undef FCNAME
#define FCNAME DECL_FUNC(MPIDI_POSIX_STARTALL)
static inline int MPIDI_POSIX_mpi_startall(int count, MPIR_Request * requests[])
{
    int i, mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_POSIX_MPI_STARTALL);

    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_POSIX_MPI_STARTALL);
    MPID_THREAD_CS_ENTER(POBJ, MPIDI_POSIX_SHM_MUTEX);

    for (i = 0; i < count; i++) {
        MPIR_Request *preq = requests[i];

        switch (preq->kind) {
            case MPIR_REQUEST_KIND__PREQUEST_SEND:
                if (MPIDI_POSIX_REQUEST(preq)->type != MPIDI_POSIX_TYPEBUFFERED) {
                    mpi_errno =
                        MPIDI_POSIX_do_isend(MPIDI_POSIX_REQUEST(preq)->user_buf,
                                             MPIDI_POSIX_REQUEST(preq)->user_count,
                                             MPIDI_POSIX_REQUEST(preq)->datatype,
                                             MPIDI_POSIX_REQUEST(preq)->dest,
                                             MPIDI_POSIX_REQUEST(preq)->tag, preq->comm,
                                             MPIDI_POSIX_REQUEST(preq)->context_id -
                                             preq->comm->context_id, &preq->u.persist.real_request,
                                             MPIDI_POSIX_REQUEST(preq)->type);
                } else {
                    MPI_Request sreq_handle;
                    mpi_errno =
                        MPIR_Ibsend_impl(MPIDI_POSIX_REQUEST(preq)->user_buf,
                                         MPIDI_POSIX_REQUEST(preq)->user_count,
                                         MPIDI_POSIX_REQUEST(preq)->datatype,
                                         MPIDI_POSIX_REQUEST(preq)->dest,
                                         MPIDI_POSIX_REQUEST(preq)->tag, preq->comm, &sreq_handle);

                    if (mpi_errno == MPI_SUCCESS)
                        MPIR_Request_get_ptr(sreq_handle, preq->u.persist.real_request);
                }

                break;

            case MPIR_REQUEST_KIND__PREQUEST_RECV:
                mpi_errno =
                    MPIDI_POSIX_do_irecv(MPIDI_POSIX_REQUEST(preq)->user_buf,
                                         MPIDI_POSIX_REQUEST(preq)->user_count,
                                         MPIDI_POSIX_REQUEST(preq)->datatype,
                                         MPIDI_POSIX_REQUEST(preq)->rank,
                                         MPIDI_POSIX_REQUEST(preq)->tag, preq->comm,
                                         MPIDI_POSIX_REQUEST(preq)->context_id -
                                         preq->comm->context_id, &preq->u.persist.real_request);

                break;

            default:
                MPIR_Assert(0);
                break;
        }

        if (mpi_errno == MPI_SUCCESS) {
            preq->status.MPI_ERROR = MPI_SUCCESS;

            if (MPIDI_POSIX_REQUEST(preq)->type == MPIDI_POSIX_TYPEBUFFERED) {
                preq->cc_ptr = &preq->cc;
                MPIR_cc_set(&preq->cc, 0);
            } else
                preq->cc_ptr = &preq->u.persist.real_request->cc;
        } else {
            preq->u.persist.real_request = NULL;
            preq->status.MPI_ERROR = mpi_errno;
            preq->cc_ptr = &preq->cc;
            MPIR_cc_set(&preq->cc, 0);
        }
    }

    MPID_THREAD_CS_EXIT(POBJ, MPIDI_POSIX_SHM_MUTEX);
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_POSIX_MPI_STARTALL);
    return mpi_errno;
}

#endif /* POSIX_STARTALL_H_INCLUDED */
