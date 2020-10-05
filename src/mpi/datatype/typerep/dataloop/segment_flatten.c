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

/* NOTE: I don't think I've removed the need for bufp in here yet! -- RobR */

struct flatten_params {
    int index;
    MPI_Aint length;
    MPI_Aint last_end;
    MPI_Aint *blklens;
    MPI_Aint *disps;
};

static int leaf_contig_mpi_flatten(MPI_Aint * blocks_p,
                                   MPI_Datatype el_type,
                                   MPI_Aint rel_off, void *bufp, void *v_paramp);
static int leaf_vector_mpi_flatten(MPI_Aint * blocks_p,
                                   MPI_Aint count,
                                   MPI_Aint blksz,
                                   MPI_Aint stride,
                                   MPI_Datatype el_type,
                                   MPI_Aint rel_off, void *bufp, void *v_paramp);
static int leaf_blkidx_mpi_flatten(MPI_Aint * blocks_p,
                                   MPI_Aint count,
                                   MPI_Aint blksz,
                                   MPI_Aint * offsetarray,
                                   MPI_Datatype el_type,
                                   MPI_Aint rel_off, void *bufp, void *v_paramp);
static int leaf_index_mpi_flatten(MPI_Aint * blocks_p,
                                  MPI_Aint count,
                                  MPI_Aint * blockarray,
                                  MPI_Aint * offsetarray,
                                  MPI_Datatype el_type,
                                  MPI_Aint rel_off, void *bufp, void *v_paramp);

/* MPII_Dataloop_segment_flatten - flatten a type into a representation
 *                            appropriate for passing to hindexed create.
 *
 * NOTE: blocks will be in units of bytes when returned.
 *
 * WARNING: there's potential for overflow here as we convert from
 *          various types into an index of bytes.
 *
 * Parameters:
 * segp    - pointer to segment structure
 * first   - first byte in segment to pack
 * lastp   - in/out parameter describing last byte to pack (and afterwards
 *           the last byte _actually_ packed)
 *           NOTE: actually returns index of byte _after_ last one packed
 * blklens, disps - the usual blocklength and displacement arrays for MPI
 * lengthp - in/out parameter describing length of array (and afterwards
 *           the amount of the array that has actual data)
 */
void MPII_Dataloop_segment_flatten(MPIR_Segment * segp,
                                   MPI_Aint first,
                                   MPI_Aint * lastp,
                                   MPI_Aint * blklens, MPI_Aint * disps, MPI_Aint * lengthp)
{
    struct flatten_params params;

    MPIR_Assert(*lengthp > 0);

    params.index = 0;
    params.length = *lengthp;
    params.blklens = blklens;
    params.disps = disps;

    MPII_Segment_manipulate(segp,
                            first,
                            lastp,
                            leaf_contig_mpi_flatten,
                            leaf_vector_mpi_flatten,
                            leaf_blkidx_mpi_flatten, leaf_index_mpi_flatten, NULL, &params);

    /* last value already handled by MPII_Segment_manipulate */
    *lengthp = params.index;
    return;
}

/* PIECE FUNCTIONS BELOW */

/* leaf_contig_mpi_flatten
 *
 */
static int leaf_contig_mpi_flatten(MPI_Aint * blocks_p,
                                   MPI_Datatype el_type,
                                   MPI_Aint rel_off, void *bufp, void *v_paramp)
{
    int last_idx;
    MPI_Aint size;
    MPI_Aint el_size;
    intptr_t last_end = 0;
    struct flatten_params *paramp = v_paramp;

    MPIR_Datatype_get_size_macro(el_type, el_size);
    size = *blocks_p * el_size;

    last_idx = paramp->index - 1;
    if (last_idx >= 0) {
        last_end = (intptr_t)
            (paramp->disps[last_idx] + ((MPI_Aint) paramp->blklens[last_idx]));
    }

    if ((last_idx == paramp->length - 1) && (last_end != ((intptr_t) bufp + rel_off))) {
        /* we have used up all our entries, and this region doesn't fit on
         * the end of the last one.  setting blocks to 0 tells manipulation
         * function that we are done (and that we didn't process any blocks).
         */
        *blocks_p = 0;
        return 1;
    } else if (last_idx >= 0 && (last_end == ((intptr_t) bufp + rel_off))) {
        /* add this size to the last vector rather than using up another one */
        paramp->blklens[last_idx] += size;
    } else {
        paramp->disps[last_idx + 1] = (MPI_Aint) bufp + rel_off;
        paramp->blklens[last_idx + 1] = size;
        paramp->index++;
    }
    return 0;
}

/* leaf_vector_mpi_flatten
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
 *
 * TODO: MAKE THIS CODE SMARTER, USING THE SAME GENERAL APPROACH AS IN THE
 *       COUNT BLOCK CODE ABOVE.
 */
static int leaf_vector_mpi_flatten(MPI_Aint * blocks_p, MPI_Aint count, MPI_Aint blksz, MPI_Aint stride, MPI_Datatype el_type, MPI_Aint rel_off,        /* offset into buffer */
                                   void *bufp,  /* start of buffer */
                                   void *v_paramp)
{
    int i;
    MPI_Aint size, blocks_left;
    MPI_Aint el_size;
    struct flatten_params *paramp = v_paramp;

    MPIR_Datatype_get_size_macro(el_type, el_size);
    blocks_left = *blocks_p;

    MPIR_Assert(el_size != 0);

    for (i = 0; i < count && blocks_left > 0; i++) {
        int last_idx;
        intptr_t last_end = 0;

        if (blocks_left > blksz) {
            size = blksz * el_size;
            blocks_left -= blksz;
        } else {
            /* last pass */
            size = blocks_left * el_size;
            blocks_left = 0;
        }

        last_idx = paramp->index - 1;
        if (last_idx >= 0) {
            last_end = (intptr_t)
                (paramp->disps[last_idx] + (MPI_Aint) (paramp->blklens[last_idx]));
        }

        if ((last_idx == paramp->length - 1) && (last_end != ((intptr_t) bufp + rel_off))) {
            /* we have used up all our entries, and this one doesn't fit on
             * the end of the last one.
             */
            *blocks_p -= (blocks_left + (size / el_size));
#ifdef MPID_SP_VERBOSE
            MPL_DBG_MSG_FMT(MPIR_DBG_DATATYPE, VERBOSE,
                            (MPL_DBG_FDEST,
                             "\t[vector to vec exiting (1): next ind = %d, "
                             MPI_AINT_FMT_DEC_SPEC " blocks processed.\n",
                             paramp->u.pack_vector.index, *blocks_p));
#endif
            return 1;
        } else if (last_idx >= 0 && (last_end == ((intptr_t) bufp + rel_off))) {
            /* add this size to the last vector rather than using up new one */
            paramp->blklens[last_idx] += size;
        } else {
            paramp->disps[last_idx + 1] = (MPI_Aint) bufp + rel_off;
            paramp->blklens[last_idx + 1] = size;
            paramp->index++;
        }

        rel_off += stride;
    }

#ifdef MPID_SP_VERBOSE
    MPL_DBG_MSG_FMT(MPIR_DBG_DATATYPE, VERBOSE,
                    (MPL_DBG_FDEST,
                     "\t[vector to vec exiting (2): next ind = %d, " MPI_AINT_FMT_DEC_SPEC
                     " blocks processed.\n", paramp->u.pack_vector.index, *blocks_p));
#endif

    /* if we get here then we processed ALL the blocks; don't need to update
     * blocks_p
     */

    MPIR_Assert(blocks_left == 0);
    return 0;
}

static int leaf_blkidx_mpi_flatten(MPI_Aint * blocks_p,
                                   MPI_Aint count,
                                   MPI_Aint blksz,
                                   MPI_Aint * offsetarray,
                                   MPI_Datatype el_type,
                                   MPI_Aint rel_off, void *bufp, void *v_paramp)
{
    int i;
    MPI_Aint blocks_left, size;
    MPI_Aint el_size;
    struct flatten_params *paramp = v_paramp;

    MPIR_Datatype_get_size_macro(el_type, el_size);
    blocks_left = *blocks_p;

    MPIR_Assert(el_size != 0);

    for (i = 0; i < count && blocks_left > 0; i++) {
        int last_idx;
        intptr_t last_end = 0;

        if (blocks_left > blksz) {
            size = blksz * el_size;
            blocks_left -= blksz;
        } else {
            /* last pass */
            size = blocks_left * el_size;
            blocks_left = 0;
        }

        last_idx = paramp->index - 1;
        if (last_idx >= 0) {
            last_end = (intptr_t)
                (paramp->disps[last_idx] + ((MPI_Aint) paramp->blklens[last_idx]));
        }

        if ((last_idx == paramp->length - 1) &&
            (last_end != ((intptr_t) bufp + rel_off + offsetarray[i]))) {
            /* we have used up all our entries, and this one doesn't fit on
             * the end of the last one.
             */
            *blocks_p -= ((MPI_Aint) blocks_left + (((MPI_Aint) size) / el_size));
            return 1;
        } else if (last_idx >= 0 && (last_end == ((intptr_t) bufp + rel_off + offsetarray[i]))) {
            /* add this size to the last vector rather than using up new one */
            paramp->blklens[last_idx] += size;
        } else {
            paramp->disps[last_idx + 1] = (MPI_Aint) bufp + rel_off + offsetarray[i];
            paramp->blklens[last_idx + 1] = size;
            paramp->index++;
        }
    }

    /* if we get here then we processed ALL the blocks; don't need to update
     * blocks_p
     */
    MPIR_Assert(blocks_left == 0);
    return 0;
}

static int leaf_index_mpi_flatten(MPI_Aint * blocks_p,
                                  MPI_Aint count,
                                  MPI_Aint * blockarray,
                                  MPI_Aint * offsetarray,
                                  MPI_Datatype el_type,
                                  MPI_Aint rel_off, void *bufp, void *v_paramp)
{
    int i;
    MPI_Aint size, blocks_left;
    MPI_Aint el_size;
    struct flatten_params *paramp = v_paramp;

    MPIR_Datatype_get_size_macro(el_type, el_size);
    blocks_left = *blocks_p;

    MPIR_Assert(el_size != 0);

    for (i = 0; i < count && blocks_left > 0; i++) {
        int last_idx;
        intptr_t last_end = 0;

        if (blocks_left > blockarray[i]) {
            size = blockarray[i] * el_size;
            blocks_left -= blockarray[i];
        } else {
            /* last pass */
            size = blocks_left * el_size;
            blocks_left = 0;
        }

        last_idx = paramp->index - 1;
        if (last_idx >= 0) {
            last_end = (intptr_t)
                (paramp->disps[last_idx] + (MPI_Aint) (paramp->blklens[last_idx]));
        }

        if ((last_idx == paramp->length - 1) &&
            (last_end != ((intptr_t) bufp + rel_off + offsetarray[i]))) {
            /* we have used up all our entries, and this one doesn't fit on
             * the end of the last one.
             */
            *blocks_p -= (blocks_left + (size / el_size));
            return 1;
        } else if (last_idx >= 0 && (last_end == ((intptr_t) bufp + rel_off + offsetarray[i]))) {
            /* add this size to the last vector rather than using up new one */
            paramp->blklens[last_idx] += size;
        } else {
            paramp->disps[last_idx + 1] = (MPI_Aint) bufp + rel_off + offsetarray[i];
            paramp->blklens[last_idx + 1] = size;       /* these blocks are in bytes */
            paramp->index++;
        }
    }

    /* if we get here then we processed ALL the blocks; don't need to update
     * blocks_p
     */
    MPIR_Assert(blocks_left == 0);
    return 0;
}

/*
 * Local variables:
 * c-indent-tabs-mode: nil
 * End:
 */
