/*  (C)Copyright IBM Corp.  2007, 2008  */
/**
 * \file src/misc/mpid_get_universe_size.c
 * \brief ???
 */
/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"

/*
 * MPID_Get_universe_size()
 */
int MPID_Get_universe_size(int  * universe_size)
{
  int mpi_errno = MPI_SUCCESS;
  *universe_size = MPIR_UNIVERSE_SIZE_NOT_AVAILABLE;
  return mpi_errno;
}
