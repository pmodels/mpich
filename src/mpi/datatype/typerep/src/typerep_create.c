/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include <mpiimpl.h>
#include <mpir_typerep.h>
#include <dataloop.h>
#include <stdlib.h>

void MPIR_Typerep_create(MPI_Datatype type, void **typerep_p)
{
    MPIR_Dataloop_create(type, typerep_p);
}

void MPIR_Typerep_free(void **typerep_p)
{
    MPIR_Dataloop_free(typerep_p);
}

void MPIR_Typerep_dup(void *old_typerep, void **new_typerep_p)
{
    MPIR_Dataloop_dup(old_typerep, new_typerep_p);
}
