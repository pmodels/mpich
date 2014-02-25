/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef CREATE_F90_UTIL_H_INCLUDED
#define CREATE_F90_UTIL_H_INCLUDED

#include "mpiimpl.h"

int MPIR_Create_unnamed_predefined( MPI_Datatype old, int combiner, 
                                    int r, int p, 
                                    MPI_Datatype *new_ptr );


#endif /* CREATE_F90_UTIL_H_INCLUDED */

