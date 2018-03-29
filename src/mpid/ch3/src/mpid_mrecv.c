/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2012 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"

int MPID_Mrecv(void *buf, int count, MPI_Datatype datatype,
               MPIR_Request *message, MPI_Status *status)
{
    int mpi_errno = MPI_SUCCESS;
    int active_flag;            /* dummy for MPIR_Request_completion_processing */
    MPIR_Request *rreq = NULL;

    if (message == NULL) {
        /* treat as though MPI_MESSAGE_NO_PROC was passed */
        MPIR_Status_set_procnull(status);
        goto fn_exit;
    }

    /* There is no optimized MPID_Mrecv at this time because there is no real
     * optimization potential in that case.  MPID_Recv exists to prevent
     * creating a request unnecessarily for messages that are already present
     * and eligible for immediate completion. */
    mpi_errno = MPID_Imrecv(buf, count, datatype, message, &rreq);
    if (mpi_errno) MPIR_ERR_POP(mpi_errno);

    if (!MPIR_Request_is_complete(rreq)) {
        MPID_Progress_state progress_state;

        MPID_Progress_start(&progress_state);
        while (!MPIR_Request_is_complete(rreq))
        {
            mpi_errno = MPID_Progress_wait(&progress_state);
            if (mpi_errno) {
                /* --BEGIN ERROR HANDLING-- */
                MPID_Progress_end(&progress_state);
                MPIR_ERR_POP(mpi_errno);
                /* --END ERROR HANDLING-- */
            }
        }
        MPID_Progress_end(&progress_state);
    }
    mpi_errno = MPIR_Request_completion_processing(rreq, status, &active_flag);
    MPIR_Request_free(rreq);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

fn_exit:
    return mpi_errno;
fn_fail:
    goto fn_exit;
}

