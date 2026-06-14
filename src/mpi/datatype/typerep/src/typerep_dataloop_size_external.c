/*
 * Copyright 2026 Argonne National Laboratory
 * SPDX-License-Identifier: Apache-2.0
 */

#include "mpiimpl.h"
#include "mpir_typerep.h"
#include "dataloop.h"

MPI_Aint MPIR_Typerep_size_external32(MPI_Datatype type)
{
    return MPIR_Dataloop_size_external32(type);
}
