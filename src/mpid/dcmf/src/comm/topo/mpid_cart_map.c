/*  (C)Copyright IBM Corp.  2007, 2008  */
/**
 * \file src/comm/topo/mpid_cart_map.c
 * \brief ???
 */

#include "mpid_topo.h"

static int MPIDI_Cart_map_work( const MPID_Comm *comm_ptr,
                                int ndims, const int dims[], const int periods[],
                                int *newrank )
{
    int notdone = 1;
    MPIDI_PhysicalCart *phy_cart = MPIDI_PhysicalCart_new();
    MPIDI_VirtualCart  *vir_cart = MPIDI_VirtualCart_new();

    if (MPIDI_PhysicalCart_init( phy_cart, comm_ptr ))
      goto fn_fail;
    if (ndims > phy_cart->ndims+1)
      goto fn_fail;
    if (ndims == phy_cart->ndims+1)
      {
        if (dims[ndims-1] == 1)
          --ndims;
        else
          goto fn_fail;
      }

    MPIDI_VirtualCart_init( vir_cart, ndims, dims );
    if (vir_cart->size  > phy_cart->size)
      goto fn_fail;

    /* try different algorithms, from simple to difficult */
    if (notdone && vir_cart->ndims == 1)
        notdone = MPIDI_Cart_map_1D_snake( vir_cart, phy_cart, newrank );

    if (notdone)
        notdone = MPIDI_Cart_map_nofold( vir_cart, phy_cart, newrank );

    if (notdone)
        notdone = MPIDI_Cart_map_fold( vir_cart, phy_cart, newrank );

fn_fail:
    MPIDI_VirtualCart_free(  vir_cart );
    MPIDI_PhysicalCart_free( phy_cart );

    return notdone;
}

/* interface */
int MPID_Cart_map( const MPID_Comm *comm_ptr, int ndims, const int dims[], const int periods[], int *newrank )
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = MPIDI_Cart_map_work( comm_ptr, ndims, dims, periods, newrank );
    if (mpi_errno)
      {
        mpi_errno = MPIR_Cart_map( comm_ptr, ndims, dims, periods, newrank );
/*         puts("BAILING TO ** MPIR_Cart_map **   THAT'S BAD!"); */
      }

    return mpi_errno;
}
