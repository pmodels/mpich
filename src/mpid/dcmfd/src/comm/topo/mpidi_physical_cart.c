/*  (C)Copyright IBM Corp.  2007, 2008  */
/**
 * \file src/comm/topo/mpidi_physical_cart.c
 * \brief ???
 */

#include "mpid_topo.h"
#include "mpix.h"


MPIDI_PhysicalCart *MPIDI_PhysicalCart_new()
{
    return (MPIDI_PhysicalCart *) MPIU_Malloc ( sizeof(MPIDI_PhysicalCart) );
}

void MPIDI_PhysicalCart_free( MPIDI_PhysicalCart *cart )
{
    if (cart != NULL) MPIU_Free (cart);
}

/* To verify whether the given communicator is a rectangular communicator.

   Copied from mpid/bgltorus5/src/coll/mpid_collective.c:MPIDI_BGLTS_findRect().
   The reason for not using using comm->bglts.rectcomm is NOT valid: was not sure
   whether every communicator tried to construct the comm->bglts.rectcomm.

   Can go further: give existing BG/L Z major ranking. for a non-rectangular communicator,
   it is likely to find maximal cartesian partition without using the convex-hull algo.
 */
static int MPIDI_PhysicalCart_checkRect(const MPID_Comm *comm, int minc[], int maxc[] )
{
    int i, j, size;
    unsigned c[4];

    for (j=0; j<4; j++) {
        minc[j] = MAXINT; maxc[j] = 0;
    }

    for (i=0; i< comm->local_size; i++)
    {
        MPIX_rank2torus(comm->vcr[i], &c[3], &c[2], &c[1], &c[0]);
        for (j=0; j<4; j++) {
            if (c[j] < minc[j]) minc[j] = c[j];
            if (c[j] > maxc[j]) maxc[j] = c[j];
        }
    }

    size = 1;
    for (j=0; j<4; j++) size *= (maxc[j]-minc[j]+1);
    if (size != comm->local_size) return 1;

    return 0;
}

/* For a rectangular communicator, initialize the MPIDI_PhysicalCart object with
   dimension sizes and physical coordinates.

   For non rectangular communicator, return fail.
 */
int MPIDI_PhysicalCart_init( MPIDI_PhysicalCart *cart, const MPID_Comm *comm )
{

    int j;
    int minc[4], maxc[4];

    MPID_assert (cart != NULL);

    if ( MPIDI_PhysicalCart_checkRect( comm, minc, maxc ) )
        return 1;

    for (j=0; j<4; j++) {
        cart->start[j] = minc[j];
        cart->dims [j] = maxc[j] - minc[j] + 1;
    }

    cart->coord[0] = mpid_hw.tCoord;
    cart->coord[1] = mpid_hw.zCoord;
    cart->coord[2] = mpid_hw.yCoord;
    cart->coord[3] = mpid_hw.xCoord;

    for (j=0; j<4; j++) {
        MPID_assert ( cart->coord[j] >= cart->start[j] );
        MPID_assert ( cart->coord[j] <  cart->start[j] + cart->dims[j] );
    }

    cart->size = comm->local_size;
    if (cart->dims[0] == 1)
      {
        cart->ndims = 3;
        for (j=1; j<DCMF_CART_MAX_NDIMS; ++j) {
          cart->start  [j-1] = cart->start  [j];
          cart->dims   [j-1] = cart->dims   [j];
          cart->coord  [j-1] = cart->coord  [j];
        }
        cart->start  [j-1] = 0;
        cart->dims   [j-1] = 0;
        cart->coord  [j-1] = 0;
      }
    else
      cart->ndims = 4;

    return 0;
}


void MPIDI_PhysicalCart_printf( MPIDI_PhysicalCart *c )
{
  printf("PhysicalCart(%p), size=%d, ndims=%d\n", c, c->size, c->ndims);
  if (c->ndims == 4)
    {
      printf("  p dims   =<%d,%d,%d,%d>\n",c->dims[0],   c->dims[1],   c->dims[2],   c->dims[3]   );
      printf("  p coord  =<%d,%d,%d,%d>\n",c->coord[0],  c->coord[1],  c->coord[2],  c->coord[3]  );
      printf("  p start  =<%d,%d,%d,%d>\n",c->start[0],  c->start[1],  c->start[2],  c->start[3]  );
    }
  else
    {
      printf("  p dims   =<%d,%d,%d>\n",c->dims[0],   c->dims[1],   c->dims[2]   );
      printf("  p coord  =<%d,%d,%d>\n",c->coord[0],  c->coord[1],  c->coord[2]  );
      printf("  p start  =<%d,%d,%d>\n",c->start[0],  c->start[1],  c->start[2]  );
    }
}
