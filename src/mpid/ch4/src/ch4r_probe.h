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
#ifndef CH4R_PROBE_H_INCLUDED
#define CH4R_PROBE_H_INCLUDED

#include "ch4_impl.h"

MPL_STATIC_INLINE_PREFIX int MPIDIG_mpi_iprobe(int source, int tag, MPIR_Comm * comm,
                                               int context_offset, int *flag, MPI_Status * status)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Comm *root_comm;
    MPIR_Request *unexp_req;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIG_MPI_IPROBE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIG_MPI_IPROBE);

    root_comm = MPIDIG_context_id_to_comm(comm->context_id);

    /* MPIDI_CS_ENTER(); */
    unexp_req = MPIDIG_find_unexp(source, tag, root_comm->recvcontext_id + context_offset,
                                  &MPIDIG_COMM(root_comm, unexp_list));

    if (unexp_req) {
        *flag = 1;
        unexp_req->status.MPI_ERROR = MPI_SUCCESS;
        unexp_req->status.MPI_SOURCE = MPIDIG_REQUEST(unexp_req, rank);
        unexp_req->status.MPI_TAG = MPIDIG_REQUEST(unexp_req, tag);
        MPIR_STATUS_SET_COUNT(unexp_req->status, MPIDIG_REQUEST(unexp_req, count));

        status->MPI_TAG = unexp_req->status.MPI_TAG;
        status->MPI_SOURCE = unexp_req->status.MPI_SOURCE;
        MPIR_STATUS_SET_COUNT(*status, MPIDIG_REQUEST(unexp_req, count));
    } else {
        *flag = 0;
    }
    /* MPIDI_CS_EXIT(); */

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIG_MPI_IPROBE);
    return mpi_errno;
}

MPL_STATIC_INLINE_PREFIX int MPIDIG_mpi_improbe(int source, int tag, MPIR_Comm * comm,
                                                int context_offset, int *flag,
                                                MPIR_Request ** message, MPI_Status * status)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Comm *root_comm;
    MPIR_Request *unexp_req;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIG_MPI_IMPROBE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIG_MPI_IMPROBE);

    root_comm = MPIDIG_context_id_to_comm(comm->context_id);

    /* MPIDI_CS_ENTER(); */
    unexp_req = MPIDIG_dequeue_unexp(source, tag, root_comm->recvcontext_id + context_offset,
                                     &MPIDIG_COMM(root_comm, unexp_list));

    if (unexp_req) {
        *flag = 1;
        *message = unexp_req;

        (*message)->kind = MPIR_REQUEST_KIND__MPROBE;
        (*message)->comm = comm;
        /* Notes on refcounting comm:
         * We intentionally do nothing here because what we are supposed to do here
         * is -1 for dequeue(unexp_list) and +1 for (*message)->comm */

        unexp_req->status.MPI_ERROR = MPI_SUCCESS;
        unexp_req->status.MPI_SOURCE = MPIDIG_REQUEST(unexp_req, rank);
        unexp_req->status.MPI_TAG = MPIDIG_REQUEST(unexp_req, tag);
        MPIR_STATUS_SET_COUNT(unexp_req->status, MPIDIG_REQUEST(unexp_req, count));
        MPIDIG_REQUEST(unexp_req, req->status) |= MPIDIG_REQ_UNEXP_DQUED;

        status->MPI_TAG = unexp_req->status.MPI_TAG;
        status->MPI_SOURCE = unexp_req->status.MPI_SOURCE;
        MPIR_STATUS_SET_COUNT(*status, MPIDIG_REQUEST(unexp_req, count));
    } else {
        *flag = 0;
    }
    /* MPIDI_CS_EXIT(); */

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIG_MPI_IMPROBE);
    return mpi_errno;
}

#endif /* CH4R_PROBE_H_INCLUDED */
