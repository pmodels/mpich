/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */

/*
 *  (C) 2013 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "dataloop.h"

/* #define MPICH_DEBUG_DATALOOP */
#ifdef MPICH_DEBUG_DATALOOP
static int firstCall = 1;
static int printDataloop = 0;
static int printIfOptimized = 0;

/* Print format:
   (spaces for level).(el_size,el_extent,el_type)(count)....
*/
static void dl_print_dataloop( int, int, DLOOP_Dataloop * );
static void dl_print_contig( int, DLOOP_Dataloop * );
static void dl_print_vector( int, DLOOP_Dataloop * );
static void dl_print_blockindexed( int, DLOOP_Dataloop * );
static void dl_print_struct( int, DLOOP_Dataloop * );
static void dl_print( int, const char * );

static void dl_print_tab( int l )
{
    int i;
    for (i=2*l; i!=0; i--) printf( "%c", ' ' );
}
static void dl_print_base( DLOOP_Dataloop *dp )
{
    printf( "(%ld,%ld,%lx)(%ld)", (long)dp->el_size, (long)dp->el_extent,
            (long)dp->el_type, (long)dp->loop_params.count );
}
static void dl_print( int l, const char *s )
{
    dl_print_tab(l);
    printf( "%s", s );
}
static void dl_print_contig( int l, DLOOP_Dataloop *dp )
{
    dl_print_tab(l);
    printf( "CONTIG " );
    dl_print_base( dp );
    printf( "\n" );
}
static void dl_print_vector( int l, DLOOP_Dataloop *dp )
{
    int stride = dp->loop_params.v_t.stride;
    int blocksize = dp->loop_params.v_t.blocksize ;
    dl_print_tab(l);
    printf( "VECTOR " );
    dl_print_base( dp );
    printf( ":Stride %d Blocksize %d\n", stride, blocksize );
}
static void dl_print_blockindexed( int l, DLOOP_Dataloop *dp )
{
    int blocksize = dp->loop_params.bi_t.blocksize ;
    DLOOP_Offset *offarray = dp->loop_params.bi_t.offset_array;
    int i, n;
    dl_print_tab(l);
    printf( "BLOCKINDEXED " );
    dl_print_base( dp );
    printf( ":Blocksize %d:", blocksize );
    n = dp->loop_params.bi_t.count;
    if (n > 8) n = 8;
    for (i=0; i<n; i++) {
        printf( "%lx,", (long)offarray[i] );
    }
    if (dp->loop_params.bi_t.count > n) printf( "..." );
    printf( "\n" );
}
static void dl_print_indexed( int l, DLOOP_Dataloop *dp )
{
    DLOOP_Count  *blocksizearray = dp->loop_params.i_t.blocksize_array ;
    DLOOP_Offset *offarray = dp->loop_params.i_t.offset_array;
    int          i, n;
    int          minblock, maxblock;
    dl_print_tab(l);
    printf( "INDEXED " );
    dl_print_base( dp );
    n = dp->loop_params.i_t.count;
    minblock = maxblock = (n>0) ? blocksizearray[0] : 0;
    for (i=0; i<n; i++) {
        if (blocksizearray[i] > maxblock) maxblock = blocksizearray[i];
        if (blocksizearray[i] < minblock) minblock = blocksizearray[i];
    }
    printf( "blocks in [%d,%d]", minblock, maxblock );

    if (n > 8) n = 8;
    for (i=0; i<n; i++) {
        printf( "(%lx,%ld)", (long)offarray[i], (long)blocksizearray[i] );
    }
    if (dp->loop_params.i_t.count > n) printf( "..." );
    printf( "\n" );
}

static void dl_print_struct( int l, DLOOP_Dataloop *dp )
{
    DLOOP_Count  *blocksizearray = dp->loop_params.s_t.blocksize_array ;
    DLOOP_Offset *offarray = dp->loop_params.s_t.offset_array;
    DLOOP_Dataloop **looparray = dp->loop_params.s_t.dataloop_array;
    int          i, n;
    dl_print_tab(l);
    printf( "STRUCT " );
    dl_print_base( dp );
    printf( "\n" );
    n = dp->loop_params.i_t.count;
    if (n > 8) n = 8;
    for (i=0; i<n; i++) {
        dl_print_tab(l+1);
        printf( "(%lx,%ld):\n", (long)offarray[i], (long)blocksizearray[i] );
        dl_print_dataloop( l+1, 0, looparray[i] );
    }
    if (dp->loop_params.i_t.count > n) printf( "...\n" );
}
static void dl_print_dataloop( int l, int doBase, DLOOP_Dataloop *dp )
{
    dl_print_tab( l );
    if (doBase)
        dl_print_base( dp );
    switch (dp->kind & DLOOP_KIND_MASK) {
    case DLOOP_KIND_CONTIG:
        dl_print_contig( l, dp );
        break;
    case DLOOP_KIND_VECTOR:
        dl_print_vector( l, dp );
        break;
    case DLOOP_KIND_BLOCKINDEXED:
        dl_print_blockindexed( l, dp );
        break;
    case DLOOP_KIND_INDEXED:
        dl_print_indexed( l, dp );
        break;
    case DLOOP_KIND_STRUCT:
        dl_print_struct( l, dp );
        break;
    default:
        dl_print( l, "Unknown dataloop type " );
        printf( "\n" );
        break;
    }
}
#endif

void PREPEND_PREFIX(Dataloop_debug_print)( DLOOP_Dataloop *dp )
{
#ifdef MPICH_DEBUG_DATALOOP
    if (firstCall) {
        char *s = getenv( "MPICH_DATALOOP_PRINT" );
        if (s && (strcmp(s,"yes")==0 || strcmp(s,"YES") == 0)) {
            printDataloop = 1;
            printIfOptimized = 1;
        }
        firstCall = 0;
    }
    if (printDataloop) {
        printf( "In Dataloop_debug_print:\n" );
        dl_print_dataloop( 1, 0, dp );
    }
#endif
}

/*
 * Indicates whether a dataloop is a basic and final contig type.
 * This can be used to determine when a contig type can be removed
 * in a dataloop.
 */
static int dl_contig_isFinal( DLOOP_Dataloop *dp )
{
    if ((dp->kind & DLOOP_KIND_MASK) != DLOOP_KIND_CONTIG) return 0;
    if (dp->el_size == dp->el_extent &&
        (dp->kind & DLOOP_FINAL_MASK))
        return 1;
    return 0;
}


/*
 * Optimize a dataloop
 *
 * Apply the following transformations and return a new dataloop.
 * 1. Convert all predefined types to UINTS with the best alignment (may be BYTE
 *    in worst case)
 * 2. Convert blocks of contiguous into a single block of basic unit (e.g.,
 *    a vector type with a block count of 27 applied to a contiguous type of
 *    6 ints will be turned into a block count of (27*6) UINTs)
 * 3. Convert struct (with different dataloops (from different MPI datatypes)
 *    into indexed when all types are contig
 * 4. Convert dataloops with counts of 1 into simpler types (e.g., q vector
 *    with 1 element is really a contig type)
 *
 * Value of these optimizations
 * A 2012 paper[1] compared performance of Open MPI, MPICH2, and user-written code
 * for some datatypes, and found MPICH2 often performed poorer than other
 * options.  An investigation showed that some of the issues are due to
 * a failure to perform optimizations of these type (especially #1 and 2).
 * It may also be necessary to enhance the dataloop execution engine, but
 * that will b a separate step.
 *
 * [1] T. Schneider and R. Gerstenberger and T. Hoefler, "Micro-Applications
 *     for Communication Data Access Patterns and MPI Datatypes", EuroMPI 2012
 *
 * The level argument is used primarily for debugging output; it keeps track
 * of how deep a recursive application of this routine has gone.
 */
int PREPEND_PREFIX(Dataloop_optimize)(DLOOP_Dataloop *dlpOld_p, int level )
{
    int i;

#ifdef MPICH_DEBUG_DATALOOP
    /* Temp for debugging */
    /* This is threadsafe in the sense that we don't care */
    if (firstCall) {
        char *s = getenv( "MPICH_DATALOOP_PRINT" );
        if (s && (strcmp(s,"yes")==0 || strcmp(s,"YES") == 0)) {
            printDataloop = 1;
            printIfOptimized = 1;
        }
        firstCall = 0;
    }
    if (printDataloop && level == 0)
        printf( "About to optimize in commit...\n" );
#endif

    switch (dlpOld_p->kind & DLOOP_KIND_MASK) {
    case DLOOP_KIND_CONTIG:
#ifdef MPICH_DEBUG_DATALOOP
        if (printDataloop)
            dl_print_contig( level, dlpOld_p );
#endif
        /* replace contig of (non-basic) contig with contig (basic) */
        if (!(dlpOld_p->kind & DLOOP_FINAL_MASK)) {
            DLOOP_Dataloop *dlpChild_p = dlpOld_p->loop_params.c_t.dataloop;
            PREPEND_PREFIX(Dataloop_optimize)( dlpChild_p, level+1 );
            if ((dlpChild_p->kind & DLOOP_KIND_MASK) == DLOOP_KIND_CONTIG &&
                dl_contig_isFinal( dlpChild_p )) {
                if (dlpOld_p->el_size == dlpOld_p->el_extent &&
                    !MPIU_Prod_overflows_max(
                             dlpChild_p->loop_params.c_t.count,
                             dlpOld_p->loop_params.c_t.count,
                             INT_MAX ) ) {

#ifdef MPICH_DEBUG_DATALOOP
                    if (printDataloop)
                        printf( "replacing with contig\n" );
#endif
                    dlpOld_p->loop_params.c_t.count *= dlpChild_p->loop_params.c_t.count;
                    dlpOld_p->el_size   = dlpChild_p->el_size;
                    dlpOld_p->el_extent = dlpChild_p->el_extent;
                    dlpOld_p->el_type   = dlpChild_p->el_type;
                    dlpOld_p->kind     |= DLOOP_FINAL_MASK;
                    dlpOld_p->loop_params.c_t.dataloop = 0;
#ifdef MPICH_DEBUG_DATALOOP
                    if (printIfOptimized || printDataloop) {
                        printf( "replacement contig is:\n" );
                        dl_print_contig( level, dlpOld_p );
                    }
#endif
                }
                else {
                    /* */
                    /* printf( "not replacing...\n" ); */
                    /* If the low level contig is a single byte,
                       we could make that replacement. Not done. */
                    /* By doing nothing here, we ensure that the dataloop
                       is correct if not fully optimized */
                    ;
                }
            }
        }
        break;

    case DLOOP_KIND_VECTOR:
        /* if sub-dloop is (non-basic) contig, merge with blockcount */
#ifdef MPICH_DEBUG_DATALOOP
        if (printDataloop)
            dl_print_vector( level, dlpOld_p );
#endif

        if (!(dlpOld_p->kind & DLOOP_FINAL_MASK)) {
            DLOOP_Dataloop *dlpChild_p = dlpOld_p->loop_params.v_t.dataloop;
            PREPEND_PREFIX(Dataloop_optimize)( dlpChild_p, level+1 );

            if (dl_contig_isFinal( dlpChild_p ) &&
                    !MPIU_Prod_overflows_max(
                             dlpChild_p->loop_params.count,
                             dlpOld_p->loop_params.v_t.blocksize,
                             INT_MAX ) ) {
                /* We can replace the contig type by enlarging the blocksize */
                if (dlpOld_p->el_size == dlpOld_p->el_extent ||
                    dlpOld_p->loop_params.v_t.blocksize == 1) {
                    /* Reset the kind to final, free the child type,
                       set to null */
                    dlpOld_p->loop_params.v_t.blocksize *=
                        dlpChild_p->loop_params.count;
                    dlpOld_p->el_size   = dlpChild_p->el_size;
                    dlpOld_p->el_type   = dlpChild_p->el_type;
                    /*dlpOld_p->el_extent = dlpChild_p->el_extent; */
                    dlpOld_p->kind     |= DLOOP_FINAL_MASK;
                    dlpOld_p->loop_params.v_t.dataloop = 0;
#ifdef MPICH_DEBUG_DATALOOP
                    if (printIfOptimized || printDataloop) {
                        printf( "replacement Vector is:\n" );
                        dl_print_vector( level, dlpOld_p );
                    }
#endif
                }
                else {
                    /* TODO: If the vector elements do not have
                       size==extent, and the blocksize is greater than 1,
                       then it may be better to replace the elements with
                       a single strided(vector) copy with blocksize elements:
                       New vector:
                         stride <- extent
                         el_size <- size
                         extent <- ?
                         count <- blocksize
                         blocksize <- 1
                       Old vector become
                         blocksize <- 1
                         extent <- ?
                */
                    dlpChild_p->loop_params.v_t.stride =
                        dlpOld_p->el_extent;
                    dlpChild_p->el_size = 1;
                    dlpChild_p->el_type = MPI_BYTE;
                    dlpChild_p->loop_params.v_t.dataloop = 0;
                    dlpChild_p->loop_params.v_t.count =
                        dlpOld_p->loop_params.v_t.blocksize;
                    dlpChild_p->loop_params.v_t.blocksize = dlpOld_p->el_size;
                    dlpChild_p->kind = DLOOP_KIND_VECTOR |
                        DLOOP_FINAL_MASK;
                    dlpOld_p->loop_params.v_t.blocksize = 1;
#ifdef MPICH_DEBUG_DATALOOP
                    if (printIfOptimized || printDataloop) {
                        printf( "Replacing vector of contig with vector of vector\n" );
                        printf( "replacement Vector is:\n" );
                        dl_print_vector( level, dlpOld_p );
                        dl_print_vector( level+1, dlpChild_p );
                    }
#endif
                }
            }
        }
        /* replace vector of a single element with contig */
        if ((dlpOld_p->kind & DLOOP_FINAL_MASK)) {
            int blocksize = dlpOld_p->loop_params.v_t.blocksize;
            int count     = dlpOld_p->loop_params.v_t.count;
            if (dlpOld_p->el_size * blocksize ==
                dlpOld_p->loop_params.v_t.stride &&
                    !MPIU_Prod_overflows_max( count, blocksize, INT_MAX ) ) {
                dlpOld_p->kind = DLOOP_KIND_CONTIG | DLOOP_FINAL_MASK;
                dlpOld_p->loop_params.c_t.dataloop = 0;
                dlpOld_p->loop_params.c_t.count = count * blocksize;
#ifdef MPICH_DEBUG_DATALOOP
                if (printIfOptimized || printDataloop) {
                    printf( "replacement Contig is:\n" );
                    dl_print_contig( level, dlpOld_p );
                }
#endif
            }
        }
        /* replace vector that is contiguous with contiguous */
        break;

    case DLOOP_KIND_BLOCKINDEXED:
        /* if subdloop is (non-basic) contig, merge with blockcount */
#ifdef MPICH_DEBUG_DATALOOP
        if (printDataloop)
            dl_print_blockindexed( level, dlpOld_p );
#endif
        if (!(dlpOld_p->kind & DLOOP_FINAL_MASK)) {
            DLOOP_Dataloop *dlpChild_p = dlpOld_p->loop_params.bi_t.dataloop;
            PREPEND_PREFIX(Dataloop_optimize)( dlpChild_p, level+1 );
            if (dl_contig_isFinal( dlpChild_p ) &&
                    !MPIU_Prod_overflows_max(
                             dlpChild_p->loop_params.count,
                             dlpOld_p->loop_params.bi_t.blocksize,
                             INT_MAX ) ) {
                /* We can replace the contig type by enlarging the blocksize */

                /* Reset the kind to final, free the child type, set to null */
                dlpOld_p->loop_params.bi_t.blocksize *= dlpChild_p->loop_params.count;
                dlpOld_p->el_size   = dlpChild_p->el_size;
                /*dlpOld_p->el_extent = dlpChild_p->el_extent;*/
                dlpOld_p->el_type   = dlpChild_p->el_type;
                dlpOld_p->kind     |= DLOOP_FINAL_MASK;
                dlpOld_p->loop_params.bi_t.dataloop = 0;
#ifdef MPICH_DEBUG_DATALOOP
                if (printIfOptimized || printDataloop) {
                    printf( "replacement BlockIndexed is:\n" );
                    dl_print_blockindexed( level, dlpOld_p );
                }
#endif
            }
        }
        break;

    case DLOOP_KIND_INDEXED:
        /* if sub-dloop is (non-basic) contig, merge with blockcount */
#ifdef MPICH_DEBUG_DATALOOP
        if (printDataloop)
            dl_print_indexed( level, dlpOld_p );
#endif
        if (!(dlpOld_p->kind & DLOOP_FINAL_MASK)) {
            DLOOP_Dataloop *dlpChild_p = dlpOld_p->loop_params.i_t.dataloop;
            PREPEND_PREFIX(Dataloop_optimize)( dlpChild_p, level+1 );
            if (dl_contig_isFinal( dlpChild_p ) ) {
                /* Could include the child type in the blocksize counts */
            }
        }

        /* replace indexed of a single element with contig */

        /* If all block counts are multiples of the smallest, and if most
           blocks are smallest, then the other blocks could be split into
           separate blocks with appropriate offsets, replacing indexed with
           blockindexed */

        break;

    case DLOOP_KIND_STRUCT:
        /* if sub-dloops are all contig, replace with indexed */
        /* Not done yet - but first step is to recurse and
           simply/optimize the component dataloops */
#ifdef MPICH_DEBUG_DATALOOP
        if (printDataloop) {
            dl_print_struct( level, dlpOld_p );
            printf( "now optimizing...\n" );
        }
#endif
        if (!(dlpOld_p->kind & DLOOP_FINAL_MASK)) {
            for (i=0; i<dlpOld_p->loop_params.s_t.count; i++) {
                PREPEND_PREFIX(Dataloop_optimize)(
                          dlpOld_p->loop_params.s_t.dataloop_array[i],
                          level+1);
            }
        }
        /* Can the preceding if case ever be false? */
        /* Heres where we might check the following:
            Are all child dataloops CONTIG?
            Are all extents equal to sizes?
            Are all LBs equal to 0?
           If these are all true and in addition they are contiguous,
           replace with a single contig (but be careful of the extent)
           Otherwise, if these are all true, then replace with INDEXED.
        */
        if (!(dlpOld_p->kind & DLOOP_FINAL_MASK)) {
            int isContig = 1;
            int allContig = 1;
            MPI_Aint lastAdd = 0;
            for (i=0; i<dlpOld_p->loop_params.s_t.count; i++) {
                DLOOP_Dataloop *dlpChild_p =
                    dlpOld_p->loop_params.s_t.dataloop_array[i];
                if ((dlpChild_p->kind & DLOOP_KIND_MASK) != DLOOP_KIND_CONTIG) {
                    allContig = 0; break;
                }
                if (/* dlpChild_p->el_lb != 0 || */  /* No lb in dataloop(?) */
                    dlpChild_p->el_extent != dlpChild_p->el_size) {
#ifdef MPICH_DEBUG_DATALOOP
                    if (printDataloop)
                        printf( "not natural contig\n" );
#endif
                    allContig = 0; break;
                }
                if (isContig &&
                    lastAdd != dlpOld_p->loop_params.s_t.offset_array[i]) {
#ifdef MPICH_DEBUG_DATALOOP
                    if (printDataloop)
                        printf( "Not contiguous bytes: %lx != %lx\n",
                            (long)lastAdd,
                            (long)dlpOld_p->loop_params.s_t.offset_array[i] );
#endif
                    isContig = 0;
                }
                else {
                    lastAdd += dlpChild_p->el_extent *
                        dlpChild_p->loop_params.count;
                }
            }
            if (allContig) {
#ifdef MPICH_DEBUG_DATALOOP
                if (printDataloop)
                    printf( "All subtypes are contig - can replace with index\n" );
#endif
                if (isContig) {
#ifdef MPICH_DEBUG_DATALOOP
                    if (printDataloop)
                        printf( "All subtypes consequtive - can replace with a single contig\n" );
#endif
                ;
                }
            }
        }

        break;
    default:
#ifdef MPICH_DEBUG_DATALOOP
        if (printDataloop)
            dl_print( level, "Unknown type!" );
#endif
        break;
    }

#ifdef MPICH_DEBUG_DATALOOP
    if (printDataloop && level == 0)
        printf( "Done!\n" );
#endif

    return 0;
}


/*
 * Make an estimate at the complexity of a datatype.  This can be used
 * to determine whether flattening the datatype to an indexed type is
 * likely to be efficient.
 */
int PREPEND_PREFIX(Dataloop_est_complexity)(DLOOP_Dataloop *dlp_p,
                                            MPI_Aint *nElms,
                                            MPI_Aint *nDesc )
{
    int i;
    MPI_Aint myElms = 0;
    MPI_Aint myDesc = 0;
    MPI_Aint childElms = 0, childDesc = 0;
    DLOOP_Dataloop *dlpChild_p;

    switch (dlp_p->kind & DLOOP_KIND_MASK) {
    case DLOOP_KIND_CONTIG:
        /* Data moved is count*size of the child type */

        if (!(dlp_p->kind & DLOOP_FINAL_MASK)) {
            dlpChild_p = dlp_p->loop_params.c_t.dataloop;
            PREPEND_PREFIX(Dataloop_est_complexity)( dlpChild_p, &childElms,
                                                     &childDesc );
        }
        else {
            childElms = dlp_p->el_size;
            childDesc = 0;
        }
        myElms += dlp_p->loop_params.c_t.count * childElms;
        myDesc += childDesc + 1;

        break;

    case DLOOP_KIND_VECTOR:
        /* Data moved is count*size of the child type */

        if (!(dlp_p->kind & DLOOP_FINAL_MASK)) {
            dlpChild_p = dlp_p->loop_params.v_t.dataloop;
            PREPEND_PREFIX(Dataloop_est_complexity)( dlpChild_p, &childElms,
                                                     &childDesc );
        }
        else {
            childElms = dlp_p->el_size;
            childDesc = 0;
        }
        myElms += dlp_p->loop_params.v_t.count *
            dlp_p->loop_params.v_t.blocksize * childElms;
        myDesc += childDesc + 2;

        break;

    case DLOOP_KIND_BLOCKINDEXED:
        if (!(dlp_p->kind & DLOOP_FINAL_MASK)) {
            dlpChild_p = dlp_p->loop_params.bi_t.dataloop;
            PREPEND_PREFIX(Dataloop_est_complexity)( dlpChild_p, &childElms,
                                                     &childDesc );
        }
        else {
            childElms = dlp_p->el_size;
            childDesc = 0;
        }
        myElms += dlp_p->loop_params.bi_t.count *
            dlp_p->loop_params.bi_t.blocksize * childElms;
        myDesc += childDesc + dlp_p->loop_params.bi_t.count;
        break;

    case DLOOP_KIND_INDEXED:

        if (!(dlp_p->kind & DLOOP_FINAL_MASK)) {
            dlpChild_p = dlp_p->loop_params.i_t.dataloop;
            PREPEND_PREFIX(Dataloop_est_complexity)( dlpChild_p, &childElms,
                                                     &childDesc );
        }
        else {
            childElms = dlp_p->el_size;
            childDesc = 0;
        }
        myElms += dlp_p->loop_params.i_t.total_blocks * childElms;
        myDesc += childDesc + 2 * dlp_p->loop_params.i_t.count;

        break;

    case DLOOP_KIND_STRUCT:
        if (!(dlp_p->kind & DLOOP_FINAL_MASK)) {
            MPI_Aint celm, cdesc;
            for (i=0; i<dlp_p->loop_params.s_t.count; i++) {
                celm = 0; cdesc = 0;
                PREPEND_PREFIX(Dataloop_est_complexity)(
                                dlp_p->loop_params.s_t.dataloop_array[i],
                               &celm, &cdesc );
                childElms += celm * dlp_p->loop_params.s_t.blocksize_array[i];
                childDesc += cdesc + 3;
            }
        }
        else {
            int elsize = dlp_p->el_size;
            childElms = 0;
            childDesc = 0;
            for (i=0; i<dlp_p->loop_params.s_t.count; i++) {
                childElms += elsize * dlp_p->loop_params.s_t.blocksize_array[i];
                childDesc += 3;
            }
        }

        myElms += childElms;
        myDesc += childDesc;
        break;

    default:
        break;
    }

    /* Return the final values */
    *nElms += myElms;
    *nDesc += myDesc;

    return 0;
}

/*
 * Estimate the complexity of a struct Dataloop before it is constructed.
 */
int PREPEND_PREFIX(Dataloop_est_struct_complexity)( int count,
                                                    const int blklens[],
                                                    const DLOOP_Type oldtypes[],
                                                    MPI_Aint *nElms,
                                                    MPI_Aint *nDesc )
{
    MPI_Aint myElms = 0, myDesc = 0;
    int i;
    int flag = MPID_DATALOOP_ALL_BYTES;

    for (i=0; i<count; i++) {
        DLOOP_Dataloop *dlp_p = 0;
        MPI_Aint celms = 0, cdesc = 0;

        DLOOP_Handle_get_loopptr_macro(oldtypes[i],dlp_p,flag);
        if (dlp_p) {
            PREPEND_PREFIX(Dataloop_est_complexity)( dlp_p,
                                                     &celms, &cdesc );
        }
        else {
            celms = 1;
            cdesc = 1;
        }
        myElms += celms * blklens[i];
        myDesc += cdesc;
    }
    *nElms = myElms;
    *nDesc = myDesc;

    return MPI_SUCCESS;
}
