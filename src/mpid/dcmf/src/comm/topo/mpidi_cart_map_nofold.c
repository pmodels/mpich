/*  (C)Copyright IBM Corp.  2007, 2008  */
/**
 * \file src/comm/topo/mpidi_cart_map_nofold.c
 * \brief ???
 */

#include "mpid_topo.h"

static int try_permute_match( int *vir_cart,
                              int *phy_cart,
                              int nd_sort, int vir_perm[], int phy_perm[] )
{
    int nomatch, i;

    /* sort the dimensions of both cart in decreasing order, keeps order in _perm arrays */
    MPIDI_Cart_dims_sort( nd_sort, vir_cart, vir_perm );
    MPIDI_Cart_dims_sort( nd_sort, phy_cart, phy_perm );

    nomatch = 0;
    for (i=0; i<nd_sort; i++) {
        if (vir_cart[vir_perm[i]] > phy_cart[phy_perm[i]]) { nomatch = 1; break; }
    }
    return nomatch;
}

/* non-1D exact match:
   1. when req and phy are both 4D:  permut-match.
   2. phy is 4D and req is 2D or 3D: find a req dimension to embed the T dimension.
                                     Then do a permute-match excluding T in both req and phy dimensions.
   3. phy is 3D and req is 2D or 3D: permute-match with req empty dimension (if exists) filled with 1.
 */
int MPIDI_Cart_map_nofold( MPIDI_VirtualCart *vir_cart,
                           MPIDI_PhysicalCart *phy_cart,
                           int *newrank )
{
    int phy_perm[DCMF_CART_MAX_NDIMS];
    int vir_perm[DCMF_CART_MAX_NDIMS];
    int thedim = -1;

    int i, rcoord[DCMF_CART_MAX_NDIMS];
    int permute_match;
    int notdone = 1;

    if (vir_cart->ndims <= 3 && phy_cart->ndims == 4) {

        for (i=vir_cart->ndims; i<DCMF_CART_MAX_NDIMS; i++) rcoord[i] = 0;

        notdone = 1;

        /* look for an exact inclusion */
        if (vir_cart->size < phy_cart->size) {
            if( try_permute_match( vir_cart->dims, phy_cart->dims, phy_cart->ndims, vir_perm, phy_perm ) ) {
            }
            else {
                /* permute the 4 coordinates */
                for (i=0; i<phy_cart->ndims; i++)
                    rcoord[ vir_perm[i] ] = phy_cart->coord[ phy_perm[i] ] - phy_cart->start[ phy_perm[i] ];
                notdone = 0;
            }
          }

        if (notdone) {
            /* now try embed T into requested dimensions */
            int orig_dim;
            for (i=phy_cart->ndims-1; i>=1; i--) {
                orig_dim = phy_cart->dims[i];
                phy_cart->dims[i] *= phy_cart->dims[0];
                permute_match = try_permute_match( vir_cart->dims, phy_cart->dims+1, vir_cart->ndims, vir_perm, phy_perm );
                phy_cart->dims[i] = orig_dim;
                if (!permute_match) break;
            }
            --i;
            if (i < 0) return 1;

            /* the dimension that contains the T */
            thedim = i;

            /* permute the 3 coordinates */
            {
              int temp[DCMF_CART_MAX_NDIMS];
              for (i=0; i<vir_cart->ndims; i++)
                  temp[ i ] = phy_cart->coord[ i+1 ] - phy_cart->start[ i+1 ];
              /* fill the T dimension in here */
              temp[ thedim ] = temp[ thedim ] * phy_cart->dims[0] + (phy_cart->coord[0] - phy_cart->start[0]);
              for (i=0; i<vir_cart->ndims; i++)
                  rcoord[ vir_perm[i] ] = temp[ phy_perm[i] ];
            }
        }
    }
    else
    if (vir_cart->ndims == 4 && phy_cart->ndims ==4) {
        if( try_permute_match( vir_cart->dims, phy_cart->dims, phy_cart->ndims, vir_perm, phy_perm ) ) return 1;

        /* permute the 4 coordinates */
        for (i=0; i<phy_cart->ndims; i++)
            rcoord[ vir_perm[i] ] = phy_cart->coord[ phy_perm[i] ] - phy_cart->start[ phy_perm[i] ];
    }
    else
    if (vir_cart->ndims <= 3 && phy_cart->ndims == 3) {
        for (i=vir_cart->ndims; i<DCMF_CART_MAX_NDIMS; i++) rcoord[i] = 0;

        if( try_permute_match( vir_cart->dims, phy_cart->dims, 3, vir_perm, phy_perm ) ) return 1;

        /* permute the 3 coordinates */
        for (i=0; i<phy_cart->ndims; i++)
            rcoord[ vir_perm[i] ] = phy_cart->coord[ phy_perm[i] ] - phy_cart->start[ phy_perm[i] ];
    }
    else return 1;

    MPIDI_Cart_map_coord_to_rank( vir_cart->size, 4, vir_cart->dims, rcoord, newrank );

#if 0
    printf( "<" );
    for (i=0;i<phy_cart->ndims;i++) {
        printf( "%d/%d", phy_cart->coord[i], phy_cart->dims[i] );
        if (i != phy_cart->ndims-1) printf( "," );
    }
    printf( "> to <" );
    for (i=0;i<vir_cart->ndims;i++) {
        printf( "%d/%d", rcoord[i], vir_cart->dims[i] );
        if (i != phy_cart->ndims-1) printf( "," );
    }
    printf( ">, r=%d\n", *newrank );
#endif


    return 0;
}
