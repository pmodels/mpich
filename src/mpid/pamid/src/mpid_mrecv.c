/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2012 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"

int MPID_Mrecv(void *buf, int count, MPI_Datatype datatype,
               MPID_Request *message, MPI_Status *status)
{
    int mpi_errno = MPI_SUCCESS;
    MPI_Request req_handle; /* dummy for MPIR_Request_complete */
    int active_flag; /* dummy for MPIR_Request_complete */
    MPID_Request *rreq = NULL;

    if (message == NULL) {
        /* treat as though MPI_MESSAGE_NO_PROC was passed */
        MPIR_Status_set_procnull(status);
	return mpi_errno;
    }

    /* There is no optimized MPID_Mrecv at this time because there is no real
     * optimization potential in that case.  MPID_Recv exists to prevent
     * creating a request unnecessarily for messages that are already present
     * and eligible for immediate completion. */
    mpi_errno = MPID_Imrecv(buf, count, datatype, message, &rreq);

    MPID_PROGRESS_WAIT_WHILE(!MPID_Request_is_complete(rreq));

    mpi_errno = MPIR_Request_complete(&req_handle, rreq, status, &active_flag);
    return mpi_errno;
}

