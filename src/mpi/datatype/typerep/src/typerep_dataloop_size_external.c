/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"
#include "mpir_typerep.h"
#include "dataloop.h"

int MPIR_Typerep_size_external32(MPI_Datatype type)
{
    return MPIR_Dataloop_size_external32(type);
}
