/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"

int MPIR_Sched_cb_free_buf(MPIR_Comm * comm, int tag, void *state)
{
    MPL_free(state);
    return MPI_SUCCESS;
}
