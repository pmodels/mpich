/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"
#include "yaksa.h"

void MPIR_Typerep_commit(MPI_Datatype type)
{
    MPIR_FUNC_ENTER;

    MPIR_FUNC_EXIT;
    return;
}

void MPIR_Typerep_free(MPIR_Datatype * typeptr)
{
    MPIR_FUNC_ENTER;

    yaksa_type_t type = typeptr->typerep.handle;

    if (type != YAKSA_TYPE__FLOAT_INT && type != YAKSA_TYPE__DOUBLE_INT &&
        type != YAKSA_TYPE__LONG_INT && type != YAKSA_TYPE__SHORT_INT &&
        type != YAKSA_TYPE__LONG_DOUBLE_INT) {
        yaksa_type_free(type);
    }

    MPIR_FUNC_EXIT;
}
