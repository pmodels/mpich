/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef TYPEREP_INTERNAL_H_INCLUDED
#define TYPEREP_INTERNAL_H_INCLUDED

#include "mpiimpl.h"

int MPII_Typerep_convert_subarray(int ndims, int *array_of_sizes, int *array_of_subsizes,
                                  int *array_of_starts, int order, MPI_Datatype oldtype,
                                  MPI_Datatype * newtype);
int MPII_Typerep_convert_darray(int size, int rank, int ndims, const int *array_of_gsizes,
                                const int *array_of_distribs, const int *array_of_dargs,
                                const int *array_of_psizes, int order, MPI_Datatype oldtype,
                                MPI_Datatype * newtype);
#endif /* TYPEREP_INTERNAL_H_INCLUDED */
