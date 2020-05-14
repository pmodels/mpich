/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpidimpl.h"
#include "stubnm_impl.h"

int MPIDI_STUBNM_mpi_op_free_hook(MPIR_Op * op_p)
{
    MPIR_Assert(0);
    return 0;
}

int MPIDI_STUBNM_mpi_op_commit_hook(MPIR_Op * op_p)
{
    MPIR_Assert(0);
    return 0;
}
