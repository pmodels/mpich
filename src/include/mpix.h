/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef MPIX_H_INCLUDED
#define MPIX_H_INCLUDED

#include "mpi.h"

/* Prototypes in this file must be interpreted as C routines when this header
 * file is compiled by a C++ compiler.  "mpi.h" already does this for the main
 * MPI_ routines. */
#if defined(__cplusplus)
extern "C" {
#endif

int MPIX_Group_comm_create(MPI_Comm old_comm, MPI_Group group, int tag, MPI_Comm * new_comm);

#if defined(__cplusplus)
}
#endif

#endif /* MPIX_H_INCLUDED */
