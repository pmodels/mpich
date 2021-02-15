/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"

#include "tsp_gentran.h"
#include "gentran_utils.h"

int MPIR_Sched_cb_free_buf(MPIR_Comm * comm, int tag, void *state)
{
    MPL_free(state);
    return MPI_SUCCESS;
}

int MPIR_Persist_coll_start(MPIR_Request * preq)
{
    int mpi_errno;

    MPIR_TSP_sched_reset(preq->u.persist.sched);
    mpi_errno = MPIR_TSP_sched_start(preq->u.persist.sched,
                                     preq->comm, &preq->u.persist.real_request);
    if (mpi_errno == MPI_SUCCESS) {
        preq->status.MPI_ERROR = MPI_SUCCESS;
        preq->cc_ptr = &preq->u.persist.real_request->cc;
    } else {
        /* If a failure occurs attempting to start the request, then we
         * assume that partner request was not created, and stuff
         * the error code in the persistent request.  The wait and test
         * routines will look at the error code in the persistent
         * request if a partner request is not present. */
        preq->u.persist.real_request = NULL;
        preq->status.MPI_ERROR = mpi_errno;
        preq->cc_ptr = &preq->cc;
        MPIR_cc_set(&preq->cc, 0);
    }

    return MPI_SUCCESS;
}

void MPIR_Persist_coll_free_cb(MPIR_Request * request)
{
    /* If this is an active persistent request, we must also
     * release the partner request. */
    if (request->u.persist.real_request != NULL) {
        MPIR_Request_free(request->u.persist.real_request);
    }
    MPII_Genutil_sched_free(request->u.persist.sched);
}
