/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef TYPEREP_INTERNAL_H_INCLUDED
#define TYPEREP_INTERNAL_H_INCLUDED

#include "mpiimpl.h"

#define MPICH_DATATYPE_ENGINE_YAKSA    (1)
#define MPICH_DATATYPE_ENGINE_DATALOOP (2)

#if (MPICH_DATATYPE_ENGINE == MPICH_DATATYPE_ENGINE_YAKSA)

#include "yaksa.h"
yaksa_type_t MPII_Typerep_get_yaksa_type(MPI_Datatype type);

#else

int MPII_Typerep_convert_subarray(int ndims, int *array_of_sizes, int *array_of_subsizes,
                                  int *array_of_starts, int order, MPI_Datatype oldtype,
                                  MPI_Datatype * newtype);
int MPII_Typerep_convert_darray(int size, int rank, int ndims, const int *array_of_gsizes,
                                const int *array_of_distribs, const int *array_of_dargs,
                                const int *array_of_psizes, int order, MPI_Datatype oldtype,
                                MPI_Datatype * newtype);

#endif

#endif /* TYPEREP_INTERNAL_H_INCLUDED */
