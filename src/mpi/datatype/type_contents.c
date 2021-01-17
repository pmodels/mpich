/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"
#include "datatype.h"

int MPIR_Type_get_contents_impl(MPI_Datatype datatype, int max_integers, int max_addresses,
                                int max_datatypes, int array_of_integers[],
                                MPI_Aint array_of_addresses[], MPI_Datatype array_of_datatypes[])
{
    int i, mpi_errno;
    MPIR_Datatype *dtp;
    MPIR_Datatype_contents *cp;

    /* --BEGIN ERROR HANDLING-- */
    /* these are checked at the MPI layer, so I feel that asserts
     * are appropriate.
     */
    MPIR_Assert(!MPIR_DATATYPE_IS_PREDEFINED(datatype));
    /* --END ERROR HANDLING-- */

    MPIR_Datatype_get_ptr(datatype, dtp);
    cp = dtp->contents;
    MPIR_Assert(cp != NULL);

    if (cp->nr_counts > 0) {
        mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE,
                                         __func__, __LINE__, MPI_ERR_OTHER,
                                         "**need_get_contents_c", 0);
        return mpi_errno;
    }

    /* --BEGIN ERROR HANDLING-- */
    if (max_integers < cp->nr_ints || max_addresses < cp->nr_aints || max_datatypes < cp->nr_types) {
        mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE,
                                         __func__, __LINE__, MPI_ERR_OTHER, "**dtype", 0);
        return mpi_errno;
    }
    /* --END ERROR HANDLING-- */

    if (cp->nr_ints > 0) {
        MPII_Datatype_get_contents_ints(cp, array_of_integers);
    }

    if (cp->nr_aints > 0) {
        MPII_Datatype_get_contents_aints(cp, array_of_addresses);
    }

    if (cp->nr_types > 0) {
        MPII_Datatype_get_contents_types(cp, array_of_datatypes);
    }

    for (i = 0; i < cp->nr_types; i++) {
        if (!HANDLE_IS_BUILTIN(array_of_datatypes[i])) {
            MPIR_Datatype_get_ptr(array_of_datatypes[i], dtp);
            MPIR_Datatype_ptr_add_ref(dtp);
        }
    }

    return MPI_SUCCESS;
}

int MPIR_Type_get_envelope_impl(MPI_Datatype datatype,
                                int *num_integers,
                                int *num_addresses, int *num_datatypes, int *combiner)
{
    int mpi_errno = MPI_SUCCESS;

    if (MPIR_DATATYPE_IS_PREDEFINED(datatype)) {
        *combiner = MPI_COMBINER_NAMED;
        *num_integers = 0;
        *num_addresses = 0;
        *num_datatypes = 0;
    } else {
        MPIR_Datatype *dtp;

        MPIR_Datatype_get_ptr(datatype, dtp);

        if (dtp->contents->nr_counts > 0) {
            mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE,
                                             __func__, __LINE__, MPI_ERR_OTHER,
                                             "**need_get_envelope_c", 0);
            return mpi_errno;
        }

        *combiner = dtp->contents->combiner;
        *num_integers = dtp->contents->nr_ints;
        *num_addresses = dtp->contents->nr_aints;
        *num_datatypes = dtp->contents->nr_types;
    }
    return MPI_SUCCESS;
}
