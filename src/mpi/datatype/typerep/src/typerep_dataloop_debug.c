/*
 * Copyright 2026 Argonne National Laboratory
 * SPDX-License-Identifier: Apache-2.0
 */

#include <mpiimpl.h>
#include <mpir_typerep.h>
#include <dataloop.h>
#include <stdlib.h>

void MPIR_Typerep_debug(MPI_Datatype type)
{
    MPIR_Dataloop_printf(type, 0, 1);
}
