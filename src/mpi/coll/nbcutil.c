/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"
#include "tsp_impl.h"

int MPIR_Sched_cb_free_buf(MPIR_Comm * comm, int tag, void *state)
{
    MPL_free(state);
    return MPI_SUCCESS;
}

int MPIR_Persist_coll_start(MPIR_Request * preq)
{
    int mpi_errno = MPI_SUCCESS;

    if (preq->u.persist_coll.sched_type == MPIR_SCHED_NORMAL) {
        mpi_errno = MPIR_Sched_reset(preq->u.persist_coll.sched);
        MPIR_ERR_CHECK(mpi_errno);

        mpi_errno = MPIR_Sched_start(preq->u.persist_coll.sched,
                                     preq->comm, &preq->u.persist_coll.real_request);
        MPIR_ERR_CHECK(mpi_errno);
    } else if (preq->u.persist_coll.sched_type == MPIR_SCHED_GENTRAN) {
        MPIR_TSP_sched_reset(preq->u.persist_coll.sched);
        mpi_errno = MPIR_TSP_sched_start(preq->u.persist_coll.sched,
                                         preq->comm, &preq->u.persist_coll.real_request);
        MPIR_ERR_CHECK(mpi_errno);
    } else {
        /* TODO: proper error return */
        MPIR_Assert(0);
    }

    preq->status.MPI_ERROR = MPI_SUCCESS;
    preq->cc_ptr = &preq->u.persist_coll.real_request->cc;

  fn_exit:
    return mpi_errno;
  fn_fail:
    /* If a failure occurs attempting to start the request, then we
     * assume that partner request was not created, and stuff
     * the error code in the persistent request.  The wait and test
     * routines will look at the error code in the persistent
     * request if a partner request is not present. */
    preq->u.persist_coll.real_request = NULL;
    preq->status.MPI_ERROR = mpi_errno;
    preq->cc_ptr = &preq->cc;
    MPIR_cc_set(&preq->cc, 0);
    goto fn_exit;
}

void MPIR_Persist_coll_free_cb(MPIR_Request * request)
{
    /* If this is an active persistent request, we must also
     * release the partner request. */
    if (request->u.persist_coll.real_request != NULL) {
        MPIR_Request_free(request->u.persist_coll.real_request);
    }

    MPII_Coll_req_t *coll = &request->u.persist_coll.coll;
    if (coll->host_sendbuf) {
        MPIR_gpu_host_free(coll->host_sendbuf, coll->count, coll->datatype);
    }
    if (coll->host_recvbuf) {
        MPIR_gpu_host_free(coll->host_recvbuf, coll->count, coll->datatype);
        MPIR_Datatype_release_if_not_builtin(coll->datatype);
    }

    if (request->u.persist_coll.sched_type == MPIR_SCHED_NORMAL) {
        MPIR_Sched_free(request->u.persist_coll.sched);
    } else if (request->u.persist_coll.sched_type == MPIR_SCHED_GENTRAN) {
        MPIR_TSP_sched_free(request->u.persist_coll.sched);
    } else {
        /* TODO: proper error return */
        MPIR_Assert(0);
    }
}
