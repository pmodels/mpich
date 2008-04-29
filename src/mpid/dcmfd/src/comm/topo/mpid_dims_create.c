/*  (C)Copyright IBM Corp.  2007, 2008  */
/**
 * \file src/comm/topo/mpid_dims_create.c
 * \brief ???
 */

#include "mpid_topo.h"

/** \brief Hook function for a torus-geometry optimized version of MPI_Dims_Create */
int MPID_Dims_create( int nnodes, int ndims, int *dims )
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = MPIDI_Dims_create_work( nnodes, ndims, dims );
    if (mpi_errno)
      {
        mpi_errno = MPIR_Dims_create( nnodes, ndims, dims );
/*         puts("BAILING TO ** MPIR_Dims_create **   THAT'S BAD!"); */
      }

    return mpi_errno;
}
