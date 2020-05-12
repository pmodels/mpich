/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"
#include "yaksa.h"

void MPIR_Typerep_commit(MPI_Datatype type, void **typerep)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIR_TYPEREP_COMMIT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIR_TYPEREP_COMMIT);

    switch (type) {
        case MPI_FLOAT_INT:
            *typerep = (void *) YAKSA_TYPE__FLOAT_INT;
            break;

        case MPI_DOUBLE_INT:
            *typerep = (void *) YAKSA_TYPE__DOUBLE_INT;
            break;

        case MPI_LONG_INT:
            *typerep = (void *) YAKSA_TYPE__LONG_INT;
            break;

        case MPI_SHORT_INT:
            *typerep = (void *) YAKSA_TYPE__SHORT_INT;
            break;

        case MPI_LONG_DOUBLE_INT:
            *typerep = (void *) YAKSA_TYPE__LONG_DOUBLE_INT;
            break;

        default:
            break;
    }

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIR_TYPEREP_COMMIT);
}

void MPIR_Typerep_free(void **typerep)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIR_TYPEREP_FREE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIR_TYPEREP_FREE);

    yaksa_type_t type = (yaksa_type_t) (*typerep);

    if (type != YAKSA_TYPE__FLOAT_INT && type != YAKSA_TYPE__DOUBLE_INT &&
        type != YAKSA_TYPE__LONG_INT && type != YAKSA_TYPE__SHORT_INT &&
        type != YAKSA_TYPE__LONG_DOUBLE_INT) {
        yaksa_type_free(type);
    }

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIR_TYPEREP_FREE);
}
