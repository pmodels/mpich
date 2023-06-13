/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"
#include "datatype.h"

static void MPIR_Type_get_envelope(MPI_Datatype datatype, MPI_Aint * num_integers,
                                   MPI_Aint * num_addresses, MPI_Aint * num_large_counts,
                                   MPI_Aint * num_datatypes, int *combiner)
{
    if (MPIR_DATATYPE_IS_PREDEFINED(datatype)) {
        *combiner = MPI_COMBINER_NAMED;
        *num_integers = 0;
        *num_addresses = 0;
        *num_datatypes = 0;
        *num_large_counts = 0;
    } else {
        MPIR_Datatype *dtp;
        MPIR_Datatype_get_ptr(datatype, dtp);

        *combiner = dtp->contents->combiner;
        *num_integers = dtp->contents->nr_ints;
        *num_addresses = dtp->contents->nr_aints;
        *num_datatypes = dtp->contents->nr_types;
        *num_large_counts = dtp->contents->nr_counts;
    }
}

int MPIR_Type_get_contents_impl(MPI_Datatype datatype, int max_integers, int max_addresses,
                                int max_datatypes, int array_of_integers[],
                                MPI_Aint array_of_addresses[], MPI_Datatype array_of_datatypes[])
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_Assert(!MPIR_DATATYPE_IS_PREDEFINED(datatype));

    MPIR_Datatype_contents *cp;
    MPIR_Datatype *dtp;
    MPIR_Datatype_get_ptr(datatype, dtp);
    cp = dtp->contents;
    MPIR_Assert(cp != NULL);

    if (cp->nr_counts > 0) {
        mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE,
                                         __func__, __LINE__, MPI_ERR_TYPE,
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

    int *ints;
    MPI_Aint *aints, *counts;
    MPI_Datatype *types;
    MPIR_Datatype_access_contents(cp, &ints, &aints, &counts, &types);

    for (int i = 0; i < cp->nr_ints; i++) {
        array_of_integers[i] = ints[i];
    }
    for (int i = 0; i < cp->nr_aints; i++) {
        array_of_addresses[i] = aints[i];
    }
    for (int i = 0; i < cp->nr_types; i++) {
        array_of_datatypes[i] = types[i];
    }

    for (int i = 0; i < cp->nr_types; i++) {
        if (!HANDLE_IS_BUILTIN(array_of_datatypes[i])) {
            MPIR_Datatype_get_ptr(array_of_datatypes[i], dtp);
            MPIR_Datatype_ptr_add_ref(dtp);
        }
    }

    return mpi_errno;
}

int MPIR_Type_get_contents_large_impl(MPI_Datatype datatype, MPI_Aint max_integers,
                                      MPI_Aint max_addresses, MPI_Aint max_counts,
                                      MPI_Aint max_datatypes, int array_of_integers[],
                                      MPI_Aint array_of_addresses[], MPI_Count array_of_counts[],
                                      MPI_Datatype array_of_datatypes[])
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_Assert(!MPIR_DATATYPE_IS_PREDEFINED(datatype));

    MPIR_Datatype_contents *cp;
    MPIR_Datatype *dtp;
    MPIR_Datatype_get_ptr(datatype, dtp);
    cp = dtp->contents;
    MPIR_Assert(cp != NULL);

    /* --BEGIN ERROR HANDLING-- */
    if (max_integers < cp->nr_ints || max_addresses < cp->nr_aints ||
        max_datatypes < cp->nr_types || max_counts < cp->nr_counts) {
        mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE,
                                         __func__, __LINE__, MPI_ERR_OTHER, "**dtype", 0);
        return mpi_errno;
    }
    /* --END ERROR HANDLING-- */

    int *ints;
    MPI_Aint *aints, *counts;
    MPI_Datatype *types;
    MPIR_Datatype_access_contents(cp, &ints, &aints, &counts, &types);

    for (int i = 0; i < cp->nr_ints; i++) {
        array_of_integers[i] = ints[i];
    }
    for (int i = 0; i < cp->nr_aints; i++) {
        array_of_addresses[i] = aints[i];
    }
    for (int i = 0; i < cp->nr_counts; i++) {
        array_of_counts[i] = counts[i];
    }
    for (int i = 0; i < cp->nr_types; i++) {
        array_of_datatypes[i] = types[i];
    }

    for (int i = 0; i < cp->nr_types; i++) {
        if (!HANDLE_IS_BUILTIN(array_of_datatypes[i])) {
            MPIR_Datatype_get_ptr(array_of_datatypes[i], dtp);
            MPIR_Datatype_ptr_add_ref(dtp);
        }
    }

    return mpi_errno;
}

int MPIR_Type_get_envelope_impl(MPI_Datatype datatype,
                                int *num_integers,
                                int *num_addresses, int *num_datatypes, int *combiner)
{
    int mpi_errno = MPI_SUCCESS;

    MPI_Aint nr_ints, nr_aints, nr_counts, nr_types;
    MPIR_Type_get_envelope(datatype, &nr_ints, &nr_aints, &nr_counts, &nr_types, combiner);

    if (nr_counts > 0) {
        mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE,
                                         __func__, __LINE__, MPI_ERR_TYPE,
                                         "**need_get_envelope_c", 0);
        return mpi_errno;
    }

    *num_integers = nr_ints;
    *num_addresses = nr_aints;
    *num_datatypes = nr_types;

    return mpi_errno;
}

int MPIR_Type_get_envelope_large_impl(MPI_Datatype datatype,
                                      MPI_Aint * num_integers, MPI_Aint * num_addresses,
                                      MPI_Aint * num_large_counts, MPI_Aint * num_datatypes,
                                      int *combiner)
{
    MPIR_Type_get_envelope(datatype, num_integers, num_addresses, num_large_counts,
                           num_datatypes, combiner);
    return MPI_SUCCESS;
}
