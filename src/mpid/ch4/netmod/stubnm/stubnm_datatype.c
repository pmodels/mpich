/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpidimpl.h"
#include "stubnm_impl.h"

int MPIDI_STUBNM_mpi_type_free_hook(MPIR_Datatype * datatype_p)
{
    MPIR_Assert(0);
    return 0;
}

int MPIDI_STUBNM_mpi_type_commit_hook(MPIR_Datatype * datatype_p)
{
    MPIR_Assert(0);
    return 0;
}
