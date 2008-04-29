/*  (C)Copyright IBM Corp.  2007, 2008  */
/**
 * \file src/misc/mpid_comm_spawn.c
 * \brief ???
 */
/* -*- Mode: C; c-basic-offset:2 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"

/*
 * MPID_Comm_spawn()
 */
int MPID_Comm_spawn(char *command, char *argv[], int maxprocs, MPI_Info info,
                    int root, MPID_Comm *comm, MPID_Comm *intercomm,
                    int array_of_errcodes[])
{
  int mpi_errno = MPI_SUCCESS;
  mpi_errno = MPI_ERR_INTERN;
  return mpi_errno;
}
