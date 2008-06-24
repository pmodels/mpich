/*  (C)Copyright IBM Corp.  2007, 2008  */
/**
 * \file src/comm/topo/mpidi_dims_create.c
 * \brief ???
 */

#include "mpid_topo.h"

static int compare (const void * ap, const void *bp)
{
  register int a = *(int*)ap;
  register int b = *(int*)bp;
  register int result = 0;
  if (a<b)
    result = 1;
  else if (a>b)
    result = -1;

  return result;
}

/*
 * With the possibly partial dimension request, return dims[3] as a permutation of phy_dims[3].
 * Function returns 0 when a match found; returns 1 when not found.
 */
static int permute_match( int ndims, const int phy_dims[], int *dims ) {
    int phy_done[4] = {0,0,0,0};
    int nomatch, i, j;

    /* match the filled-dimensions of the request with physical dimensions. */
    for (i=ndims-1; i>=0; i--) {
        if ( dims[i] <= 0 ) continue;
        nomatch = 1;
        for (j=ndims-1; j>=0; j--) {
            if ( phy_done[j] ) continue;
            if ( dims[i] == phy_dims[j] ) { phy_done[j] = 1; nomatch = 0; break; }
        }
        if ( nomatch ) return 1;
    }

    /* fill the empty-dimensions of the request with not used physical dimensions */
    for (i=0; i<ndims; i++) {
        if ( dims[i] > 0 ) continue;
        for (j=0; j<ndims; j++)
            if ( !phy_done[j] ) { dims[i] = phy_dims[j]; phy_done[j] = 1; break; }
    }

    return 0;
}

/* non-1D exact match:
   1. when req and phy are both 4D:  permut-match.
   2. phy is 4D and req is 2D or 3D: find a req dimension to embed the T dimension.
                                     Then do a permute-match excluding T in both req and phy dimensions.
   3. phy is 3D and req is 2D or 3D: permute-match with req empty dimension (if exists) filled with 1.

   For many cases, returned NOTDONE, because the MPICH2 default implementation has a fancy factorization
   algorithm which can generate fairly squared cartesian dimensions.

   In addition, since MPI_Dims_create() does not has communicator associated to it. Only implementation is
   to map to MPI_COMM_WORLD, which is most time sqaured cartesian in XYZ dimensions.

   So, the default implementation can work pretty well. Here, only to work the permutation right.
 */
int MPIDI_Dims_create_nofold( MPIDI_VirtualCart *vir_cart,
                              MPIDI_PhysicalCart *phy_cart )
{
    if (vir_cart->ndims < 3) return 1;
    if (vir_cart->size != phy_cart->size) return 1;

    if (phy_cart->ndims == 4)
      {
        if (vir_cart->ndims == 4)
          return permute_match( 4, phy_cart->dims, vir_cart->dims );
        else if (vir_cart->ndims == 3)
          {
            int i, error = 0;
            int vir_dims[4];
            int phy_dims[4];
            phy_dims[0] = phy_cart->dims[1];
            phy_dims[1] = phy_cart->dims[2];
            phy_dims[2] = phy_cart->dims[3];
            for (i=0; i<3; i++) {
              phy_dims[i] *= phy_cart->dims[0];
              vir_dims[0]  = vir_cart->dims[0];
              vir_dims[1]  = vir_cart->dims[1];
              vir_dims[2]  = vir_cart->dims[2];
              error = permute_match( 3, phy_dims, vir_dims );
              if (!error) break;
              phy_dims[i] = phy_cart->dims[i+1];
            }
            if (!error)
              {
                vir_cart->dims[0] = vir_dims[0];
                vir_cart->dims[1] = vir_dims[1];
                vir_cart->dims[2] = vir_dims[2];
                return 0;
              }
          }
      }
    else if (phy_cart->ndims == 3)
      {
        if (vir_cart->ndims == 4)
          {
            vir_cart->dims[DCMF_CART_MAX_NDIMS-1] = 1;
            return permute_match(3, phy_cart->dims, vir_cart->dims);
          }
        else if (vir_cart->ndims == 3)
          return permute_match(3, phy_cart->dims, vir_cart->dims);
      }

    return 1;
}

/*
 * First implementation, only effect when the query request fits in the dimensionality of the partition:
 *      condition 1: ndims = 3i
 *      condition 2: non-zero dims[] entries matches the dimensionality of the partition.
 */
int MPIDI_Dims_create_work( int nnodes, int ndims, int *dims )
{

    int notdone = 1;
    MPIDI_PhysicalCart *phy_cart;
    MPIDI_VirtualCart  *vir_cart;
    MPID_Comm *comm_ptr = MPIR_Process.comm_world;
    int set_cnt, node_cnt, empty_dim, i;

    set_cnt = 0;
    node_cnt = 1;
    empty_dim = -1;
    if (ndims  < 1) return MPI_ERR_ARG;
    if (nnodes < 1) return MPI_ERR_ARG;
    for (i=0; i<ndims; i++) {
        if (dims[i] > 0) {
            set_cnt++; node_cnt *= dims[i];
        } else if (dims[i] == 0) {
            empty_dim = i;
        } else {
          return MPI_ERR_DIMS;
        }
    }

    if ((nnodes / node_cnt) * node_cnt != nnodes ) return MPI_ERR_DIMS;
    if (set_cnt == ndims) return 0;
    if (set_cnt == ndims-1) {
        dims[empty_dim] = nnodes/node_cnt;
        return 0;
    }

    /* now dealing case:
        dims[2] = {0,0}
                when nnodes == phy_size, default factorization can do the trick for folding case.
        dims[3] = {0,0,?}
                when nodes == phy_size, default factorization can do the trick for general case for folding.
        dims[4] = {0,0,?,?}
                has to be exact match.

     */

    phy_cart = MPIDI_PhysicalCart_new();
    vir_cart = MPIDI_VirtualCart_new();

    if (MPIDI_PhysicalCart_init( phy_cart, comm_ptr )) goto fn_return;
    if (ndims  > DCMF_CART_MAX_NDIMS) goto fn_return;
    if (nnodes > phy_cart->size ) goto fn_return;

    MPIDI_VirtualCart_init( vir_cart, ndims, dims);
    vir_cart->size = nnodes;
    vir_cart->ndims = ndims;
    for(i=0; i<ndims; ++i)
      vir_cart->dims[i] = dims[i];

    notdone = MPIDI_Dims_create_nofold( vir_cart, phy_cart );

    /* return computed dimensions */
    if (!notdone)
      {
        int i,j;
        int sort_dims[ndims];
        for (j=i=0; i<ndims; ++i)
          if (dims[i] == 0)
            sort_dims[j++] = vir_cart->dims[i];
        qsort(sort_dims, j, sizeof(int), compare);
        for (j=i=0; i<ndims; ++i)
          if (dims[i] == 0)
            dims[i] = sort_dims[j++];
      }

fn_return:
    MPIDI_VirtualCart_free( vir_cart);
    MPIDI_PhysicalCart_free(phy_cart);

    return notdone;
}
