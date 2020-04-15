/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"
#include "dataloop_internal.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

/* NOTE: bufp values are unused, ripe for removal */

static int leaf_contig_count_block(MPI_Aint * blocks_p,
                                   MPI_Datatype el_type,
                                   MPI_Aint rel_off, void *bufp, void *v_paramp);
static int leaf_vector_count_block(MPI_Aint * blocks_p,
                                   MPI_Aint count,
                                   MPI_Aint blksz,
                                   MPI_Aint stride,
                                   MPI_Datatype el_type,
                                   MPI_Aint rel_off, void *bufp, void *v_paramp);
static int leaf_blkidx_count_block(MPI_Aint * blocks_p,
                                   MPI_Aint count,
                                   MPI_Aint blksz,
                                   MPI_Aint * offsetarray,
                                   MPI_Datatype el_type,
                                   MPI_Aint rel_off, void *bufp, void *v_paramp);
static int leaf_index_count_block(MPI_Aint * blocks_p,
                                  MPI_Aint count,
                                  MPI_Aint * blockarray,
                                  MPI_Aint * offsetarray,
                                  MPI_Datatype el_type,
                                  MPI_Aint rel_off, void *bufp, void *v_paramp);

struct MPIR_contig_blocks_params {
    MPI_Aint count;
    MPI_Aint last_loc;
};

/* MPIR_Segment_count_contig_blocks()
 *
 * Count number of contiguous regions in segment between first and last.
 */
void MPIR_Segment_count_contig_blocks(MPIR_Segment * segp,
                                      MPI_Aint first, MPI_Aint * lastp, MPI_Aint * countp)
{
    struct MPIR_contig_blocks_params params;

    params.count = 0;
    params.last_loc = 0;

    /* FIXME: The blkidx and index functions are not used since they
     * optimize the count by coalescing contiguous segments, while
     * functions using the count do not optimize in the same way
     * (e.g., flatten code) */
    MPII_Segment_manipulate(segp, first, lastp, leaf_contig_count_block, leaf_vector_count_block, leaf_blkidx_count_block, leaf_index_count_block, NULL,        /* size fn */
                            (void *) &params);

    *countp = params.count;
    return;
}

/* PIECE FUNCTIONS BELOW */

/* MPID_Leaf_contig_count_block
 *
 * Note: because bufp is just an offset, we can ignore it in our
 *       calculations of # of contig regions.
 */
static int leaf_contig_count_block(MPI_Aint * blocks_p,
                                   MPI_Datatype el_type,
                                   MPI_Aint rel_off, void *bufp ATTRIBUTE((unused)), void *v_paramp)
{
    MPI_Aint size, el_size;
    struct MPIR_contig_blocks_params *paramp = v_paramp;

    MPIR_Assert(*blocks_p > 0);

    MPIR_Datatype_get_size_macro(el_type, el_size);
    size = *blocks_p * el_size;

#ifdef MPID_SP_VERBOSE
    MPL_DBG_MSG_FMT(MPIR_DBG_DATATYPE, VERBOSE,
                    (MPL_DBG_FDEST,
                     "contig count block: count = %d, buf+off = %d, lastloc = "
                     MPI_AINT_FMT_DEC_SPEC "\n", (int) paramp->count,
                     (int) ((char *) bufp + rel_off), paramp->last_loc));
#endif

    if (paramp->count > 0 && rel_off == paramp->last_loc) {
        /* this region is adjacent to the last */
        paramp->last_loc += size;
    } else {
        /* new region */
        paramp->last_loc = rel_off + size;
        paramp->count++;
    }
    return 0;
}

/* leaf_vector_count_block
 *
 * Input Parameters:
 * blocks_p - [inout] pointer to a count of blocks (total, for all noncontiguous pieces)
 * count    - # of noncontiguous regions
 * blksz    - size of each noncontiguous region
 * stride   - distance in bytes from start of one region to start of next
 * el_type - elemental type (e.g. MPI_INT)
 * ...
 *
 * Note: this is only called when the starting position is at the beginning
 * of a whole block in a vector type.
 */
static int leaf_vector_count_block(MPI_Aint * blocks_p, MPI_Aint count, MPI_Aint blksz, MPI_Aint stride, MPI_Datatype el_type, MPI_Aint rel_off,        /* offset into buffer */
                                   void *bufp ATTRIBUTE((unused)), void *v_paramp)
{
    MPI_Aint new_blk_count;
    MPI_Aint size, el_size;
    struct MPIR_contig_blocks_params *paramp = v_paramp;

    MPIR_Assert(count > 0 && blksz > 0 && *blocks_p > 0);

    MPIR_Datatype_get_size_macro(el_type, el_size);
    size = el_size * blksz;
    new_blk_count = count;

    /* if size == stride, then blocks are adjacent to one another */
    if (size == stride)
        new_blk_count = 1;

    if (paramp->count > 0 && rel_off == paramp->last_loc) {
        /* first block sits at end of last block */
        new_blk_count--;
    }

    paramp->last_loc = rel_off + ((MPI_Aint) (count - 1)) * stride + size;
    paramp->count += new_blk_count;
    return 0;
}

/* leaf_blkidx_count_block
 *
 * Note: this is only called when the starting position is at the
 * beginning of a whole block in a blockindexed type.
 */
static int leaf_blkidx_count_block(MPI_Aint * blocks_p,
                                   MPI_Aint count,
                                   MPI_Aint blksz,
                                   MPI_Aint * offsetarray,
                                   MPI_Datatype el_type,
                                   MPI_Aint rel_off, void *bufp ATTRIBUTE((unused)), void *v_paramp)
{
    MPI_Aint i, new_blk_count;
    MPI_Aint size, el_size, last_loc;
    struct MPIR_contig_blocks_params *paramp = v_paramp;

    MPIR_Assert(count > 0 && blksz > 0 && *blocks_p > 0);

    MPIR_Datatype_get_size_macro(el_type, el_size);
    size = el_size * (MPI_Aint) blksz;
    new_blk_count = count;

    if (paramp->count > 0 && ((rel_off + offsetarray[0]) == paramp->last_loc)) {
        /* first block sits at end of last block */
        new_blk_count--;
    }

    last_loc = rel_off + offsetarray[0] + size;
    for (i = 1; i < count; i++) {
        if (last_loc == rel_off + offsetarray[i])
            new_blk_count--;

        last_loc = rel_off + offsetarray[i] + size;
    }

    paramp->last_loc = last_loc;
    paramp->count += new_blk_count;
    return 0;
}

/* leaf_index_count_block
 *
 * Note: this is only called when the starting position is at the
 * beginning of a whole block in an indexed type.
 */
static int leaf_index_count_block(MPI_Aint * blocks_p,
                                  MPI_Aint count,
                                  MPI_Aint * blockarray,
                                  MPI_Aint * offsetarray,
                                  MPI_Datatype el_type,
                                  MPI_Aint rel_off, void *bufp ATTRIBUTE((unused)), void *v_paramp)
{
    MPI_Aint new_blk_count;
    MPI_Aint el_size, last_loc;
    struct MPIR_contig_blocks_params *paramp = v_paramp;

    MPIR_Assert(count > 0 && *blocks_p > 0);

    MPIR_Datatype_get_size_macro(el_type, el_size);
    new_blk_count = count;

    if (paramp->count > 0 && ((rel_off + offsetarray[0]) == paramp->last_loc)) {
        /* first block sits at end of last block */
        new_blk_count--;
    }

    /* Note: when we build an indexed type we combine adjacent regions,
     *       so we're not going to go through and check every piece
     *       separately here. if someone else were building indexed
     *       dataloops by hand, then the loop here might be necessary.
     *       MPI_Aint i and MPI_Aint size would need to be
     *       declared above.
     */
#if 0
    last_loc = rel_off * offsetarray[0] + ((MPI_Aint) blockarray[0]) * el_size;
    for (i = 1; i < count; i++) {
        if (last_loc == rel_off + offsetarray[i])
            new_blk_count--;

        last_loc = rel_off + offsetarray[i] + ((MPI_Aint) blockarray[i]) * el_size;
    }
#else
    last_loc = rel_off + offsetarray[count - 1] + ((MPI_Aint) blockarray[count - 1]) * el_size;
#endif

    paramp->last_loc = last_loc;
    paramp->count += new_blk_count;
    return 0;
}

/*
 * Local variables:
 * c-indent-tabs-mode: nil
 * End:
 */
