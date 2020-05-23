/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"
#include "yaksa.h"

void MPIR_Typerep_commit(MPI_Datatype type)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIR_TYPEREP_COMMIT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIR_TYPEREP_COMMIT);

    MPIR_Datatype *typeptr;
    MPIR_Datatype_get_ptr(type, typeptr);

    switch (type) {
        case MPI_FLOAT_INT:
            typeptr->typerep.handle = (void *) YAKSA_TYPE__FLOAT_INT;
            break;

        case MPI_DOUBLE_INT:
            typeptr->typerep.handle = (void *) YAKSA_TYPE__DOUBLE_INT;
            break;

        case MPI_LONG_INT:
            typeptr->typerep.handle = (void *) YAKSA_TYPE__LONG_INT;
            break;

        case MPI_SHORT_INT:
            typeptr->typerep.handle = (void *) YAKSA_TYPE__SHORT_INT;
            break;

        case MPI_LONG_DOUBLE_INT:
            typeptr->typerep.handle = (void *) YAKSA_TYPE__LONG_DOUBLE_INT;
            break;

        default:
            break;
    }

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIR_TYPEREP_COMMIT);
}

void MPIR_Typerep_free(MPIR_Datatype * typeptr)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIR_TYPEREP_FREE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIR_TYPEREP_FREE);

    yaksa_type_t type = (yaksa_type_t) typeptr->typerep.handle;

    if (type != YAKSA_TYPE__FLOAT_INT && type != YAKSA_TYPE__DOUBLE_INT &&
        type != YAKSA_TYPE__LONG_INT && type != YAKSA_TYPE__SHORT_INT &&
        type != YAKSA_TYPE__LONG_DOUBLE_INT) {
        yaksa_type_free(type);
    }

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIR_TYPEREP_FREE);
}
