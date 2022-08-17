/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"
#include "mpir_typerep.h"
#include "dataloop.h"

int MPIR_Typerep_size_external32(MPI_Datatype type)
{
    MPI_Aint sz = MPIR_Dataloop_size_external32(type);
    MPIR_Assert(sz <= INT_MAX);
    return (int) sz;
}
