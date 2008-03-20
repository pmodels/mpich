/*  (C)Copyright IBM Corp.  2007, 2008  */
/**
 * \file src/comm/topo/mpidi_cart_map_1d_snake.c
 * \brief ???
 */

#include "mpid_topo.h"

/* snake through the rectangular space following XYZT */
int MPIDI_Cart_map_1D_snake( MPIDI_VirtualCart *vir_cart,
                             MPIDI_PhysicalCart *phy_cart,
                             int *newrank )
{
    int norm_coord;
    int i, odd;

    if (vir_cart->size > phy_cart->size) return 1;

    *newrank = phy_cart->coord[0] - phy_cart->start[0];
    for (i=1; i<phy_cart->ndims; i++) {

        norm_coord = phy_cart->coord[i] - phy_cart->start[i];	/* normalized coord */
        odd = *newrank % 2;					/* odd or even */

        *newrank *= phy_cart->dims[i];
        if (!odd)						/* which way to count */
            *newrank += norm_coord;
        else
            *newrank += (phy_cart->dims[i] - 1 - norm_coord);
    }

    if (*newrank >= vir_cart->size) *newrank = MPI_UNDEFINED;

    return 0;
}
