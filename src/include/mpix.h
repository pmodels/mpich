/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef MPIX_H_INCLUDED
#define MPIX_H_INCLUDED

#include "mpi.h"

int MPIX_Group_comm_create(MPI_Comm old_comm, MPI_Group group, int tag, MPI_Comm * new_comm);

#endif /* MPIX_H_INCLUDED */
