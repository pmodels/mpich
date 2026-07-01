/*
 * Copyright 2026 Argonne National Laboratory
 * SPDX-License-Identifier: Apache-2.0
 */

#include "mpidimpl.h"

int MPID_Mrecv(void *buf, MPI_Aint count, MPI_Datatype datatype,
               MPIR_Request *message, MPI_Status *status, MPIR_Request **rreq)
{
    int mpi_errno = MPI_SUCCESS;
    *rreq = NULL;

    /* There is no optimized MPID_Mrecv at this time because there is no real
     * optimization potential in that case.  MPID_Recv exists to prevent
     * creating a request unnecessarily for messages that are already present
     * and eligible for immediate completion. */
    mpi_errno = MPID_Imrecv(buf, count, datatype, message, rreq);
    MPIR_ERR_CHECK(mpi_errno);

fn_exit:
    return mpi_errno;
fn_fail:
    goto fn_exit;
}

