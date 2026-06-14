/*
 * Copyright 2026 Argonne National Laboratory
 * SPDX-License-Identifier: Apache-2.0
 */

#include "mpidimpl.h"
#include "ucx_impl.h"

int MPIDI_UCX_mpi_op_free_hook(MPIR_Op * op_p)
{
    return 0;
}

int MPIDI_UCX_mpi_op_commit_hook(MPIR_Op * op_p)
{
    return 0;
}
