/*  (C)Copyright IBM Corp.  2007, 2008  */
/**
 * \file src/comm/topo/mpid_topo.h
 * \brief ???
 */
#ifndef __mpidi_topo_h__
#define __mpidi_topo_h__

#include <values.h>
#include <stdio.h>
#include <stdlib.h>
#include "mpidimpl.h"
#include "../mpi/topo/topo.h"

#define DCMF_CART_MAX_NDIMS 4


int MPID_Cart_map( const MPID_Comm *comm_ptr, int ndims, const int dims[], const int periods[], int *newrank );


/* descriptor of physical cart topology and its methods */
typedef struct MPIDI_PhysicalCart_s
{
    int size;
    int ndims; /* 3 or 4 */
    int start  [DCMF_CART_MAX_NDIMS];
    int dims   [DCMF_CART_MAX_NDIMS];
    int coord  [DCMF_CART_MAX_NDIMS];

} MPIDI_PhysicalCart;

MPIDI_PhysicalCart * MPIDI_PhysicalCart_new();
void                 MPIDI_PhysicalCart_free( MPIDI_PhysicalCart *cart );
int                  MPIDI_PhysicalCart_init( MPIDI_PhysicalCart *cart, const MPID_Comm *comm );


/* descriptor of physical cart topology and its methods */
typedef struct MPIDI_VirtualCart_s
{
    int size;
    int ndims;
    int dims   [DCMF_CART_MAX_NDIMS];

} MPIDI_VirtualCart;

MPIDI_VirtualCart * MPIDI_VirtualCart_new();
void                MPIDI_VirtualCart_free( MPIDI_VirtualCart *cart );
int                 MPIDI_VirtualCart_init( MPIDI_VirtualCart *cart, int ndims, const int dims[] );

/* utilities */
void                MPIDI_Cart_dims_sort( int ndims, int dims[], int perm[] );

 /* C_order means the right-most dimension is the fastest changing dimension.
    Of course, dims[3] is on the right of dims[0]. The cart utilities routines
    of MPICH2 follows this order; BG/L XYZT mapping following the reverse order
    (Fortran order). in mpidi_cart_map_fold.c
 */
void MPIDI_Cart_map_coord_to_rank( int size, int nd, int dims[], int cc[], int *newrank );


/* working horse */
int MPIDI_Cart_map_fold(     MPIDI_VirtualCart *vir_cart,
                             MPIDI_PhysicalCart *phy_cart,
                             int *newrank );

int MPIDI_Cart_map_nofold(   MPIDI_VirtualCart *vir_cart,
                             MPIDI_PhysicalCart *phy_cart,
                             int *newrank );

int MPIDI_Cart_map_1D_snake( MPIDI_VirtualCart *vir_cart,
                             MPIDI_PhysicalCart *phy_cart,
                             int *newrank );

int MPIDI_Dims_create_work  ( int nnodes, int ndims, int *dims );
int MPIDI_Dims_create_nofold( MPIDI_VirtualCart *vir_cart, MPIDI_PhysicalCart *phy_cart );

void MPIDI_PhysicalCart_printf( MPIDI_PhysicalCart *c );
void MPIDI_VirtualCart_printf( MPIDI_VirtualCart *c );

#endif /* __mpidi_topo_h__ */
