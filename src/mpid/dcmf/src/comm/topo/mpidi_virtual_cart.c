/*  (C)Copyright IBM Corp.  2007, 2008  */
/**
 * \file src/comm/topo/mpidi_virtual_cart.c
 * \brief ???
 */

#include "mpid_topo.h"

MPIDI_VirtualCart *MPIDI_VirtualCart_new()
{
    return (MPIDI_VirtualCart *) MPIU_Malloc ( sizeof(MPIDI_VirtualCart) );
}

void MPIDI_VirtualCart_free( MPIDI_VirtualCart *cart )
{
    if (cart != NULL) MPIU_Free (cart);
}

int MPIDI_VirtualCart_init( MPIDI_VirtualCart *cart, int ndims, const int dims[] )
{
    int i;

    /* kick out the dimensions having size 1 */
    cart->size  = 1;
    cart->ndims = 0;
    for (i=0; i<ndims; i++) {
        if (dims[i] > 1) {
            cart->dims   [cart->ndims] = dims[i];
            cart->size                *= dims[i];
            cart->ndims ++;
        }
    }

    /* fill the garbage space with useful thing */
    for (i=cart->ndims; i<4; i++) {
        cart->dims[i]    = 1;
    }

    return 0;
}

/* bubble sort the dimension in decreasing order via a perm array */
void MPIDI_Cart_dims_sort( int ndims, int dims[], int perm[] )
{
    int i, j, temp;

    for (i=0; i<4; i++) perm[i] = i;
    for (i=0; i<ndims; i++) {
        int action = 0;
        for (j=0; j<ndims-1; j++) {
            if (dims[perm[j]] < dims[perm[j+1]]) {
                temp      = perm[j];
                perm[j]   = perm[j+1];
                perm[j+1] = temp;
                action    = 1;
            }
        }
        if (!action) break;
    }
}


void MPIDI_VirtualCart_printf( MPIDI_VirtualCart *c )
{
  printf("VirtualCart(%p), size=%d, ndims=%d\n", c, c->size, c->ndims);
  if (c->ndims == 4)
    {
      printf("  v dims   =<%d,%d,%d,%d>\n",c->dims[0],   c->dims[1],   c->dims[2],   c->dims[3]   );
    }
  else
    {
      printf("  v dims   =<%d,%d,%d>\n",c->dims[0],   c->dims[1],   c->dims[2]   );
    }
}
