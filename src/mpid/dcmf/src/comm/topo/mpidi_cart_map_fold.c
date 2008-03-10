/*  (C)Copyright IBM Corp.  2007, 2008  */
/**
 * \file src/comm/topo/mpidi_cart_map_fold.c
 * \brief ???
 */

#include "mpid_topo.h"

/* finished = perm_next( ndims, perm_array[ndims] )
       gets the next permutation. It returns 1 when there is no next permutation.
       For ndims = 3, the permutation sequence is
            0,1,2 --> 0,2,1 --> 1,0,2 --> 1,2,0 --> 2,0,1 --> 2,0,1 --> finished.

*/
static int perm_next( int size, int inperm[] )
{
    int i, j, place, place1, temp;

    place1 = -1;
    place = -1;
    for (i=size-1; i>0; i--)
    {
        if (inperm[i] < inperm[i-1] && place1 == -1) place1 = i;
        if (inperm[i] > inperm[i-1]) { place = i; break; }
    }

    /* last in the permutation */
    if (place==-1) return 1;

    for (i=size-1; i>place; i--)
    if (inperm[i] < inperm[i-1] && inperm[i] > inperm[place-1])
    {
        place1 = i; break;
    }

    /* long swap */
    if (place1 != -1 && inperm[place-1] < inperm[place1]) {
        temp = inperm[place1];
        for (i=place1; i>=place; i--) inperm[i] = inperm[i-1];
        inperm[place-1] = temp;
    }

    /* or short swap */
    else {
        temp            = inperm[place-1];
        inperm[place-1] = inperm[place];
        inperm[place]   = temp;
    }

    /* bubble sort the tail */
    for (i=place; i<size; i++) {
        int action = 0;
        for (j=place; j<size-1; j++) {
            if (inperm[j] > inperm[j+1]) {
                temp        = inperm[j];
                inperm[j]   = inperm[j+1];
                inperm[j+1] = temp;
                action = 1;
            }
        }
        if (!action) break;
    }

    return 0;
}

/* fail = find_fold( dims1[ndims1], dims2[ndims2], fold[3][3] )
       searchs a folding schedule, the folding schedule is stored in matrix fold[3][3]
       e.g. fold[i][j] = 3 indicates to unfold dimension i onto dimension j.
       fold[i][i] has no meaning.
       For 3D case as here, there will be at most 2 non-zero, non-diagonal entries.
       Diagonal entries are useless here.
       Further more, when the 2 non-zero entries are in the same row, the virtual cartesian is
       unfolded from the row_id dimension onto the other dimensions in physical cartesian.
       when the 2 entries are in the same coloum, the virtual cartesian is actually
       folded from the physical cartesian.
 */
static int find_fold( int nd1, int d1[], int nd2, int d2[], int fold[][3] )
{
    int neg_nfold=0, pos_nfold=0;
    int neg_fold=1,  pos_fold=1;
    int i, j;
#if 0
    static int count=0;
#endif

    /* initialize matrix */
    for (i=0; i<3; i++) for (j=0; j<3; j++) fold[i][j] = 0;

    /* count requesting folds and folds can gives away. */
    for (i=0; i<nd1; i++) {
        /* free folds */
        if (d1[i] < d2[i])
        {
            fold[i][i] = d2[i]/d1[i];			/* floor () */
            pos_fold *= fold[i][i];
            pos_nfold ++;
        }
        else
        /* needed folds */
        if (d1[i] > d2[i])
        {
            fold[i][i] = -(d1[i]+d2[i]-1)/d2[i];	/* roof  () */
            neg_fold *= (-fold[i][i]);
            neg_nfold ++;
        }
    }

    /* always: physical ndims >= virtual ndims */
    for (i=nd1; i<nd2; i++) if (d2[i] > 1) { fold[i][i] = d2[i]; pos_fold *= fold[i][i]; pos_nfold ++; }

    /* requesting folds > available folds --> can not fold. */
    if (neg_fold > pos_fold) return 1;

    /*
       Merge the negative folds and positive folds. For 3D case, there are following possible cases:
        0 dimension requests folds;
        1 dimension requests folds : must have 1 or 2 dimensions have free folds.
        2 dimensions requests folds: must have 1 dimension has free folds.
       Previous test excludes cases that free folds are fewer than needing folds.
     */

    /* no fold is needed */
    if (!neg_nfold) {
        for (i=0; i<nd2; i++) fold[i][i] = 0;
        return 0;
    }
    else
    /* only one dimension NEEDS folds from other 1/2 dimensions */
    if (neg_nfold == 1) {

        for (i=0; i<nd2; i++)
        if (fold[i][i] < 0) 		/* this is needy dimension */
        {
            int ask = -fold[i][i];	/* now how many folds do you want */

            for (j=0; j<nd2; j++) 	/* search through dimension to satisfy the need */
            {
                if (j==i) continue;
                if (fold[j][j] > 0 && ask > 1) 	/* j dimension has some folds */
                {
                    /* j dimension can fully satisfy the left need */
                    if (fold[j][j] >= ask) {
                        fold[j][i] = ask;
                        ask = 0;
                    }
                    /* j dimension can partially satisfy the left need */
                    else
                    {
                        fold[j][i] = fold[j][j];
                        ask = (ask + fold[j][j] -1) / fold[j][j];	/* roof () */
                    }
                }
            }

            /* end of the try, still left some needs --> fail */
            if (ask > 1) return 1;
        }
    }
    else
    /* only one dimension can GIVE folds to other 1/2 dimensions */
    if (pos_nfold == 1) {

        for (i=0; i<nd2; i++)
        if (fold[i][i] > 0) 		/* this is the donor dimension */
        {
            int has = fold[i][i];	/* how many folds can it give away */

            for (j=0; j<nd2; j++) 	/* search other dimensions to try to give */
            {
                if (j==i) continue;
                if (fold[j][j] < 0 && has > 1) 	/* j needs folds and i has some left */
                {
                    /* left free folds can satisfy j's request */
                    if (-fold[j][j] <= has) {
                        fold[i][j] = -fold[j][j];
                        has = has / (-fold[j][j]);
                    }
                    /* donor broken */
                    else {
                        has = 0; break;
                    }
                }
            }

            /* end of the try, left deficit --> fail */
            if (has < 1) return 1;
        }
    }

#if 0
    if (!count) {
    printf( "\t\tfold 1 = " );
    for (i=0; i<3; i++) { for (j=0; j<3; j++) printf( "%4d", fold[i][j] ); printf( "; " ); }
    printf( "\n" );
    }
    count ++;
#endif

    return 0;
}

/* Core of the whole folding story.
        unfold dimension "dim_from" onto "dim_onto" in 3D setting. The coordinates on
        dim_from (Z) and dim_onto (X) are both updated. The coordinate of the other
        dimension (Y) does not change.
*/
static void unfold_3d( int pdims[3], int coord[3], int dim_from, int dim_onto, int folds )
{
    int layers   = pdims[dim_from] / folds;
    int fold_num = coord[dim_from] / layers;
    int newz     = coord[dim_from] % layers;

    if (fold_num % 2)   /* reverse direction */
    {
        coord[dim_onto] = fold_num * pdims[dim_onto] + (pdims[dim_onto]-1 - coord[dim_onto]);
        coord[dim_from] = layers-1 - newz;
    }
    else                /* same direction */
    {
        coord[dim_onto] = fold_num * pdims[dim_onto] + coord[dim_onto];
        coord[dim_from] = newz;
    }

    pdims[dim_from] /= folds;
    pdims[dim_onto] *= folds;
}

/* perform_fold( vir_coord[], phy_coord[], fold[3][3] )
        does the folding following the schedule given by fold[3][3].
 */
static void perform_fold( int nd1, int d1[], int c1[], int nd2, int d2[], int c2[], int perm[], int fold[][3] )
{
    int i, j, nf, fold_list[9][3], t, dd2[3];

    /* fold[][] has 2 useful entries out of 9. Then it is a sparse matrix, right? */
    nf = 0;
    for (i=0; i<3; i++)
    for (j=0; j<3; j++)
    if (j!=i && fold[i][j] > 1)
    {
        fold_list[nf][0] = fold[i][j];
        fold_list[nf][1] = i;
        fold_list[nf][2] = j;
        nf ++;
    }

    /* 3x3 case, nf is 0, 1, 2 */
    if (nf == 2)
    {
        /* When the 2 non-zero entries are in the same row, the virtual cartesian is
           unfolded from the row_id dimension onto the other dimensions in physical cartesian.
           Then UNFOLD the dimension with more folds first to reduce dialation.
        */
        if (fold_list[0][1] == fold_list[1][1]) {
            if (fold_list[0][0] < fold_list[1][0]) {
                for (i=0; i<3; i++) {
                    t               = fold_list[0][i];
                    fold_list[0][i] = fold_list[1][i];
                    fold_list[1][i] = t;
                }
            }
        }
        /* When the 2 entries are in the same coloum, the virtual cartesian is actually
           folded from the physical cartesian.
           Then FOLD the dimension with less folds first to reduce dialation.
        */
        else {
            if (fold_list[0][0] > fold_list[1][0]) {
                for (i=0; i<3; i++) {
                    t               = fold_list[0][i];
                    fold_list[0][i] = fold_list[1][i];
                    fold_list[1][i] = t;
                }
            }
        }

        for (i=0; i<3; i++) c1[i]  = c2[perm[i]];
        for (i=0; i<3; i++) dd2[i] = d2[perm[i]];
        unfold_3d( dd2, c1, fold_list[0][1], fold_list[0][2], fold_list[0][0] );
        unfold_3d( dd2, c1, fold_list[1][1], fold_list[1][2], fold_list[1][0] );
    }

    /* Z to X and Y stays the same, how nice, no dialation */
    if (nf == 1)
    {
        for (i=0; i<3; i++) c1[i]  = c2[perm[i]];
        for (i=0; i<3; i++) dd2[i] = d2[perm[i]];
        unfold_3d( dd2, c1, fold_list[0][1], fold_list[0][2], fold_list[0][0] );
    }

    /* no fold, only permute the coordinate */
    if (nf == 0)
    {
        for (i=0; i<3; i++) c1[i]  = c2[perm[i]];
    }

    return;
}

/* Main control of the folding mapping.
        1. This routine only folds the 3 true dimensions. T dimension (if in virtual node mode)
           is handled specifically in the caller of this routine.
        2. finished = perm_next( ndims, perm_array[ndims] )
           gets the next permutation. It returns 1 when there is no next permutation.
           For ndims = 3, the permutation sequence is
                0,1,2 --> 0,2,1 --> 1,0,2 --> 1,2,0 --> 2,0,1 --> 2,0,1 --> finished.
        3. fail = find_fold( dims1[ndims1], dims2[ndims2], fold[3][3] )
           searchs a folding schedule, the folding schedule is stored in matrix fold[3][3]
           e.g. fold[i][j] = 3 indicates to unfold dimension i onto dimension j.
           fold[i][i] has no meaning.
           For 3D case as here, there will be at most 2 non-zero, non-diagonal entries.
           Diagonal entries are useless here.
           Further more, when the 2 non-zero entries are in the same row, the virtual cartesian is
           unfolded from the row_id dimension onto the other dimensions in physical cartesian.
           when the 2 entries are in the same coloum, the virtual cartesian is actually
           folded from the physical cartesian.
        4. perform_fold( vir_coord[], phy_coord[], fold[3][3] )
           does the folding following the schedule given by fold[3][3].
 */
static int perm_dims_match( int nd1, int d1[], int c1[], int nd2, int d2[], int c2[] )
{
    int perm[3] = {0,1,2};
    int fold[3][3] = {{0,0,0}, {0,0,0}, {0,0,0}};
    int fail, finished;
    int dd2[3], i;

    fail = 1;
    finished = 0;
    while( !finished )
    {
        for (i=0; i<3; i++) dd2[i] = d2[perm[i]];
        fail = find_fold( nd1, d1, nd2, dd2, fold );
        if (!fail) { break; }
        finished = perm_next( nd2, perm );
    }

    if (fail) return 1;

    perform_fold( nd1, d1, c1, nd2, d2, c2, perm, fold );

    return 0;
}

/* C_order means the right-most dimension is the fastest changing dimension.
    Of course, dims[3] is on the right of dims[0]. The cart utilities routines
    of MPICH2 follows this order; BG/L XYZT mapping following the reverse order
    (Fortran order).
 */
void MPIDI_Cart_map_coord_to_rank( int size, int nd, int dims[], int cc[], int *newrank )
{
    int radix, i;

    *newrank = 0; radix   = 1;
    for (i=nd-1; i>=0; i--)
    {
        if (cc[i] >= dims[i]) {   	/* outside vir_cart box */
            *newrank = MPI_UNDEFINED;
            break;
        }
        *newrank += cc[i] * radix;
        radix    *= dims[i];
    }
    if (*newrank >= size) *newrank = MPI_UNDEFINED;

    return;
}

/* Try to map arbitrary 2D-4D requests onto 3D/4D mesh (rectangular communicator).

   The basic idea is like to fold a paper in both dimension into a 3D mesh. There
   do exist some edge loss when folding in both dimensions and therefore the mapping
   dialation can be greater than 1.

   The core operator is defined in routine "unfold_3d" which unfolds dim_X onto dim_Z
   with dim_Y unchanged. When starting from physical coordinates / dimensions, the operator
   is transitive. i.e., one can do
        unfold_3d( X, Z, dims[], coord[] )
        unfold_3d( X, Y, dims[], coord[] )
   And the dims[] and coord[] all changes to the new cartesian.

   Currently, limitation is only for 4D request. For 4D request, there has to be one dimension
   with size 2 to match the T dimension. This is because I do not fully understand folding on
   4D cartesian.
 */

int MPIDI_Cart_map_fold( MPIDI_VirtualCart *vir_cart,
                         MPIDI_PhysicalCart *phy_cart,
                         int *newrank )
{
    int notdone, i, j;
    int c1[3], d1[3], c2[3], d2[3], cc[3];
    int vir_perm[4] = {0,1,2,3};
    int phy_perm[4] = {0,1,2,3};

    /* sort dimension in decreasing order to hope reduce the number of foldings. */
    MPIDI_Cart_dims_sort( vir_cart->ndims, vir_cart->dims, vir_perm );
    MPIDI_Cart_dims_sort( 3, phy_cart->dims, phy_perm );

    notdone = 1;

    /* covers case:
     *	1. 4 = phy_cart->ndims > vir_cart->ndims > 1
     * solution:
     * 	1. try each vir_cart->dims[]
     *	2.	vir_cart->dims[i] = roof (vir_cart->dims[i] / 2);
     *  3.	try fold
     * 	4.	coord[i] = coord[i] * 2 + cpu_id
     */
    if (phy_cart->ndims==4 && vir_cart->ndims<4)
    {
        for (i=vir_cart->ndims-1; i>=0; i--) {
            d1[i] = (vir_cart->dims[vir_perm[i]]+1)/2;
            for (j=0; j<vir_cart->ndims; j++) if (j!=i) d1[j] = vir_cart->dims[vir_perm[j]];

            for (j=0; j<3; j++) {
                c2[j] = phy_cart->coord[phy_perm[j]] - phy_cart->start[phy_perm[j]];
                d2[j] = phy_cart->dims [phy_perm[j]];
                c1[j] = 0;
            }

            if (perm_dims_match( vir_cart->ndims, d1, c1, 3, d2, c2 )) continue;

            for (j=0; j<3; j++) if (j!=i) cc[vir_perm[j]] = c1[j];
            cc[vir_perm[i]] = c1[i] * 2 + (phy_cart->coord[3] - phy_cart->start[3]);
            notdone = 0;
            break;
        }
    }
    /* covers cases:
     *	1. phy_cart->ndims == vir_cart->ndims == 4
     *		solution: remove the T dimension from both phy and vir cartesian. Then this case
     *			  becomes case 2.
     *	2. 3 = phy_cart->ndims >= vir_cart->ndims > 1
     *		solusion: just try fold.
     *
     */
    else
    {
        int vir_ndims = vir_cart->ndims;

        if (vir_ndims == 4) {
            if (vir_cart->dims[vir_perm[3]] != 2) return 1;
            vir_ndims = 3;
        }

        for (j=0; j<vir_ndims; j++) d1[j] = vir_cart->dims[vir_perm[j]];
        for (j=0; j<3; j++) {
            c2[j] = phy_cart->coord[phy_perm[j]] - phy_cart->start[phy_perm[j]];
            d2[j] = phy_cart->dims [phy_perm[j]];
            c1[j] = 0;
        }

        if (!perm_dims_match( vir_ndims, d1, c1, phy_cart->ndims, d2, c2 )) {
            for (j=0; j<3; j++) cc[vir_perm[j]] = c1[j];
            notdone = 0;
        }
    }

    if (notdone) return notdone;

    /* C_order means the right-most dimension is the fastest changing dimension.
        Of course, dims[3] is on the right of dims[0]. The cart utilities routines
        of MPICH2 follows this order; BG/L XYZT mapping following the reverse order
        (Fortran order).
     */
    MPIDI_Cart_map_coord_to_rank( vir_cart->size, vir_cart->ndims, vir_cart->dims, cc, newrank );

    /*
    printf( "\t<%2d,%2d,%2d,%2d> to %4d (notdone = %d)\n",
            phy_cart->coord[0],
            phy_cart->coord[1],
            phy_cart->coord[2],
            phy_cart->coord[3],
            *newrank,
            notdone );
    */

    return notdone;
}


/*
int main( int argc, char *argv[] )
{
    int perm[5] = {0,1,2,3,4}, next=0, cnt = 0, i, size=4;
    int fold[3][3];
    int ret, c2[3], c1[3];

    // int n1=450, nd1=3, d1[3] = {15,15,2};
    // int n2=512, nd2=3, d2[3] = {8,8,8};

    // int n1=343, nd1=3, d1[3] = {7,7,7};
    // int n2=512, nd2=3, d2[3] = {16,16,2};

    // int n1=343, nd1=3, d1[3] = {7,7,7};
    // int n2=512, nd2=3, d2[3] = {64,4,2};

    // int n1=465, nd1=2, d1[3] = {31,15};
    // int n2=512, nd2=3, d2[3] = {64,4,2};

    int n1=49,  nd1=2, d1[3] = {3,5,1};
    int n2=64,  nd2=3, d2[3] = {2,2,6};

    for (c2[0]=0; c2[0]<d2[0]; c2[0]++)
    for (c2[1]=0; c2[1]<d2[1]; c2[1]++)
    {
        for (c2[2]=0; c2[2]<d2[2]; c2[2]++)
        {
            ret = perm_dims_match( nd1, d1, c1, nd2, d2, c2 );
            // printf( "ret = %d\n", ret );
            printf( "<%2d/%2d,%2d/%2d,%2d/%2d> to <%2d/%2d,%2d/%2d,%2d/%2d>\n",
                    c2[0], d2[0],
                    c2[1], d2[1],
                    c2[2], d2[2],
                    c1[0], d1[0],
                    c1[1], d1[1],
                    c1[2], d1[2]
            );
            // cnt ++;
            // if (cnt > 10) return 0;
        }
        printf( "\n" );
    }

    return 0;
}
*/

/*
int main( int argc, char *argv[] )
{
    int perm[5] = {0,1,2,3,4}, next=0, cnt = 0, i, size=4;
    int fold[3][3];
    int ret;

    // int n1=450, nd1=3, d1[3] = {15,15,2};
    // int n2=512, nd2=3, d2[3] = {8,8,8};

    // int n1=343, nd1=3, d1[3] = {7,7,7};
    // int n2=512, nd2=3, d2[3] = {16,16,2};

    // int n1=343, nd1=3, d1[3] = {7,7,7};
    // int n2=512, nd2=3, d2[3] = {64,4,2};

    // int n1=465, nd1=2, d1[3] = {31,15};
    // int n2=512, nd2=3, d2[3] = {64,4,2};

    int n1=465, nd1=2, d1[3] = {31,15};
    int n2=512, nd2=3, d2[3] = {64,8,1};

    ret = find_fold( nd1, d1, nd2, d2, fold );
    printf( "ret = %d\n", ret );

    return 0;
}
*/
