/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"
#include "dataloop.h"
#include "mpir_typerep.h"

int MPIR_Typerep_create_vector(int count, int blocklength, int stride, MPI_Datatype oldtype,
                               MPIR_Datatype * newtype)
{
    return MPI_SUCCESS;
}

int MPIR_Typerep_create_hvector(int count, int blocklength, MPI_Aint stride, MPI_Datatype oldtype,
                                MPIR_Datatype * newtype)
{
    return MPI_SUCCESS;
}

int MPIR_Typerep_create_contig(int count, MPI_Datatype oldtype, MPIR_Datatype * newtype)
{
    return MPI_SUCCESS;
}

int MPIR_Typerep_create_dup(MPI_Datatype oldtype, MPIR_Datatype * newtype)
{
    MPIR_Datatype *dtp;

    MPIR_Datatype_get_ptr(oldtype, dtp);
    if (dtp->is_committed)
        MPIR_Dataloop_dup(dtp->typerep.handle, &newtype->typerep.handle);

    return MPI_SUCCESS;
}

int MPIR_Typerep_create_indexed_block(int count, int blocklength, const int *array_of_displacements,
                                      MPI_Datatype oldtype, MPIR_Datatype * newtype)
{
    return MPI_SUCCESS;
}

int MPIR_Typerep_create_hindexed_block(int count, int blocklength,
                                       const MPI_Aint * array_of_displacements,
                                       MPI_Datatype oldtype, MPIR_Datatype * newtype)
{
    return MPI_SUCCESS;
}

int MPIR_Typerep_create_indexed(int count, const int *array_of_blocklengths,
                                const int *array_of_displacements, MPI_Datatype oldtype,
                                MPIR_Datatype * newtype)
{
    return MPI_SUCCESS;
}

int MPIR_Typerep_create_hindexed(int count, const int *array_of_blocklengths,
                                 const MPI_Aint * array_of_displacements, MPI_Datatype oldtype,
                                 MPIR_Datatype * newtype)
{
    return MPI_SUCCESS;
}

int MPIR_Typerep_create_resized(MPI_Datatype oldtype, MPI_Aint lb, MPI_Aint extent,
                                MPIR_Datatype * newtype)
{
    return MPI_SUCCESS;
}

int MPIR_Typerep_create_struct(int count, const int *array_of_blocklengths,
                               const MPI_Aint * array_of_displacements,
                               const MPI_Datatype * array_of_types, MPIR_Datatype * newtype)
{
    return MPI_SUCCESS;
}

int MPIR_Typerep_create_subarray(int ndims, const int *array_of_sizes, const int *array_of_subsizes,
                                 const int *array_of_starts, int order,
                                 MPI_Datatype oldtype, MPIR_Datatype * newtype)
{
    return MPI_SUCCESS;
}

int MPIR_Typerep_create_darray(int size, int rank, int ndims, const int *array_of_gsizes,
                               const int *array_of_distribs, const int *array_of_dargs,
                               const int *array_of_psizes, int order, MPI_Datatype oldtype,
                               MPIR_Datatype * newtype)
{
    return MPI_SUCCESS;
}
