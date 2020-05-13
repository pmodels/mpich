/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"
#include "dataloop_internal.h"

#include <stdio.h>
#include <stdlib.h>

/*
=== BEGIN_MPI_T_CVAR_INFO_BLOCK ===

categories:
    - name        : DATALOOP
      description : Dataloop-related CVARs

cvars:
    - name        : MPIR_CVAR_DATALOOP_FAST_SEEK
      category    : DATALOOP
      type        : int
      default     : 1
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        use a datatype-specialized algorithm to shortcut seeking to
        the correct location in a noncontiguous buffer

=== END_MPI_T_CVAR_INFO_BLOCK ===
*/

#undef MPII_DATALOOP_DEBUG_MANIPULATE

/* Notes on functions:
 *
 * There are a few different sets of functions here:
 * - MPII_Segment_manipulate() - uses a "piece" function to perform operations
 *   using segments (piece functions defined elsewhere)
 * - MPIDU functions - these define the externally visible interface
 *   to segment functionality
 */

/* MPII_Segment_manipulate - do something to a segment
 *
 * If you think of all the data to be manipulated (packed, unpacked, whatever),
 * as a stream of bytes, it's easier to understand how first and last fit in.
 *
 * This function does all the work, calling the piecefn passed in when it
 * encounters a datatype element which falls into the range of first..(last-1).
 *
 * piecefn can be NULL, in which case this function doesn't do anything when it
 * hits a region.  This is used internally for repositioning within this stream.
 *
 * last is a byte offset to the byte just past the last byte in the stream
 * to operate on.  this makes the calculations all over MUCH cleaner.
 *
 * stream_off, stream_el_size, first, and last are all working in terms of the
 * types and sizes for the stream, which might be different from the local sizes
 * (in the heterogeneous case).
 *
 * This is a horribly long function.  Too bad; it's complicated :)! -- Rob
 *
 * NOTE: THIS IMPLEMENTATION CANNOT HANDLE STRUCT DATALOOPS.
 */

#define SEGMENT_SAVE_LOCAL_VALUES         \
    {                                           \
        segp->cur_sp     = cur_sp;              \
        segp->valid_sp   = valid_sp;            \
        segp->stream_off = stream_off;          \
        *lastp           = stream_off;          \
    }

#define SEGMENT_LOAD_LOCAL_VALUES         \
    {                                           \
        last       = *lastp;                    \
        cur_sp     = segp->cur_sp;              \
        valid_sp   = segp->valid_sp;            \
        stream_off = segp->stream_off;          \
        cur_elmp   = &(segp->stackelm[cur_sp]); \
    }

#define SEGMENT_RESET_VALUES                                      \
    {                                                                   \
        segp->stream_off     = 0;                                       \
        segp->cur_sp         = 0;                                       \
        cur_elmp             = &(segp->stackelm[0]);                    \
        cur_elmp->curcount   = cur_elmp->orig_count;                    \
        cur_elmp->orig_block = MPII_Dataloop_stackelm_blocksize(cur_elmp);      \
        cur_elmp->curblock   = cur_elmp->orig_block;                    \
        cur_elmp->curoffset  = cur_elmp->orig_offset +                  \
            MPII_Dataloop_stackelm_offset(cur_elmp);                            \
    }

#define SEGMENT_POP_AND_MAYBE_EXIT                        \
    {                                                           \
        cur_sp--;                                               \
        if (cur_sp >= 0) cur_elmp = &segp->stackelm[cur_sp];    \
        else {                                                  \
            SEGMENT_SAVE_LOCAL_VALUES;                    \
            return;                                             \
        }                                                       \
    }

#define SEGMENT_PUSH                      \
    {                                           \
        cur_sp++;                               \
        cur_elmp = &segp->stackelm[cur_sp];     \
    }

#define STACKELM_BLOCKINDEXED_OFFSET(elmp_, curcount_)    \
    (elmp_)->loop_p->loop_params.bi_t.offset_array[(curcount_)]

#define STACKELM_INDEXED_OFFSET(elmp_, curcount_)         \
    (elmp_)->loop_p->loop_params.i_t.offset_array[(curcount_)]

#define STACKELM_INDEXED_BLOCKSIZE(elmp_, curcount_)              \
    (elmp_)->loop_p->loop_params.i_t.blocksize_array[(curcount_)]

#define STACKELM_STRUCT_OFFSET(elmp_, curcount_)          \
    (elmp_)->loop_p->loop_params.s_t.offset_array[(curcount_)]

#define STACKELM_STRUCT_BLOCKSIZE(elmp_, curcount_)               \
    (elmp_)->loop_p->loop_params.s_t.blocksize_array[(curcount_)]

#define STACKELM_STRUCT_EL_EXTENT(elmp_, curcount_)               \
    (elmp_)->loop_p->loop_params.s_t.el_extent_array[(curcount_)]

#define STACKELM_STRUCT_DATALOOP(elmp_, curcount_)                \
    (elmp_)->loop_p->loop_params.s_t.dataloop_array[(curcount_)]

static void segment_seek(struct MPIR_Segment *segp, MPI_Aint position,
                         MPI_Aint(*sizefn) (MPI_Datatype el_type))
{
    struct MPII_Dataloop_stackelm *cur_elmp;
    struct MPII_Dataloop_stackelm *next_elmp;
    int cur_sp;

    MPIR_Assert(segp->stream_off < position);

    if (segp->stream_off || sizefn || !MPIR_CVAR_DATALOOP_FAST_SEEK) {
        goto fallback_path;
    }

    SEGMENT_RESET_VALUES;
    cur_sp = segp->cur_sp;

    /* in the common case where this is a new segment and user wants
     * to pack or unpack from a non-zero offset, try to skip through
     * large blocks and setup the segment cursor at the correct
     * position */
    /* in the below code, at the leaf-level, the curblocks is setup to
     * point to the remaining blocks.  But at the upper levels, the
     * curblocks are setup to be one lesser than the remaining blocks
     * (even if the lower-level block is completely unused). */
    cur_elmp->orig_offset = 0;
    cur_elmp->curoffset = 0;
    while (1) {
        switch ((cur_elmp->loop_p->kind & MPII_DATALOOP_KIND_MASK)) {

            case MPII_DATALOOP_KIND_CONTIG:
                {
                    MPI_Aint blocksize = MPII_Dataloop_stackelm_blocksize(cur_elmp);

                    MPI_Aint num_elems = (position - segp->stream_off) / cur_elmp->loop_p->el_size;
                    if (num_elems > blocksize)
                        num_elems = blocksize;
                    segp->stream_off += num_elems * cur_elmp->loop_p->el_size;

                    /* contig should have exactly one block */
                    MPIR_Assert(cur_elmp->orig_count == 1);

                    /* current (remaining) block count */
                    cur_elmp->curcount = (num_elems == blocksize ? 0 : 1);

                    /* current (remaining) block size */
                    cur_elmp->curblock = blocksize - num_elems;

                    /* current offset */
                    cur_elmp->curoffset = cur_elmp->orig_offset +
                        num_elems * cur_elmp->loop_p->el_extent;

                    /* if there is a child element, setup its
                     * parameters */
                    if ((cur_elmp->loop_p->kind & MPII_DATALOOP_FINAL_MASK) == 0) {
                        next_elmp = &(segp->stackelm[cur_sp + 1]);
                        next_elmp->orig_offset = cur_elmp->curoffset;
                        cur_elmp->curoffset = cur_elmp->orig_offset;

                        cur_elmp->curblock--;
                        segp->cur_sp++;

                        /* we can't skip any large blocks at this
                         * level anymore; move one level lower and
                         * repeat the same process */
                        SEGMENT_PUSH;

                        continue;
                    } else {
                        goto fn_exit;
                    }

                    break;
                }

            case MPII_DATALOOP_KIND_VECTOR:
                {
                    MPI_Aint blocksize = MPII_Dataloop_stackelm_blocksize(cur_elmp);

                    MPI_Aint num_blocks =
                        (position - segp->stream_off) / (cur_elmp->loop_p->el_size * blocksize);
                    if (num_blocks > cur_elmp->orig_count)
                        num_blocks = cur_elmp->orig_count;
                    segp->stream_off += num_blocks * cur_elmp->loop_p->el_size * blocksize;

                    MPI_Aint num_elems = (position - segp->stream_off) / cur_elmp->loop_p->el_size;
                    MPIR_Assert(num_elems < blocksize);
                    segp->stream_off += num_elems * cur_elmp->loop_p->el_size;

                    /* current (remaining) block count */
                    cur_elmp->curcount = cur_elmp->orig_count - num_blocks;

                    /* current (remaining) block size */
                    cur_elmp->curblock = blocksize - num_elems;

                    /* current offset */
                    cur_elmp->curoffset = cur_elmp->orig_offset +
                        num_blocks * cur_elmp->loop_p->loop_params.v_t.stride +
                        num_elems * cur_elmp->loop_p->el_extent;

                    /* if there is a child element, setup its
                     * parameters */
                    if ((cur_elmp->loop_p->kind & MPII_DATALOOP_FINAL_MASK) == 0) {
                        next_elmp = &(segp->stackelm[cur_sp + 1]);
                        next_elmp->orig_offset = cur_elmp->curoffset;
                        cur_elmp->curoffset = cur_elmp->orig_offset;

                        cur_elmp->curblock--;
                        segp->cur_sp++;

                        /* we can't skip any large blocks at this
                         * level anymore; move one level lower and
                         * repeat the same process */
                        SEGMENT_PUSH;

                        continue;
                    } else {
                        goto fn_exit;
                    }

                    break;
                }

            case MPII_DATALOOP_KIND_BLOCKINDEXED:
                {
                    MPI_Aint blocksize = MPII_Dataloop_stackelm_blocksize(cur_elmp);

                    MPI_Aint num_blocks =
                        (position - segp->stream_off) / (cur_elmp->loop_p->el_size * blocksize);
                    if (num_blocks > cur_elmp->orig_count)
                        num_blocks = cur_elmp->orig_count;
                    segp->stream_off += num_blocks * cur_elmp->loop_p->el_size * blocksize;

                    MPI_Aint num_elems = (position - segp->stream_off) / cur_elmp->loop_p->el_size;
                    MPIR_Assert(num_elems < blocksize);
                    segp->stream_off += num_elems * cur_elmp->loop_p->el_size;

                    /* current (remaining) block count */
                    cur_elmp->curcount = cur_elmp->orig_count - num_blocks;

                    /* current (remaining) block size */
                    cur_elmp->curblock = blocksize - num_elems;

                    /* current offset */
                    cur_elmp->curoffset = cur_elmp->orig_offset +
                        num_elems * cur_elmp->loop_p->el_extent +
                        STACKELM_BLOCKINDEXED_OFFSET(cur_elmp, num_blocks);

                    /* if there is a child element, setup its
                     * parameters */
                    if ((cur_elmp->loop_p->kind & MPII_DATALOOP_FINAL_MASK) == 0) {
                        next_elmp = &(segp->stackelm[cur_sp + 1]);
                        next_elmp->orig_offset = cur_elmp->curoffset;
                        cur_elmp->curoffset = cur_elmp->orig_offset;

                        cur_elmp->curblock--;
                        segp->cur_sp++;

                        /* we can't skip any large blocks at this
                         * level anymore; move one level lower and
                         * repeat the same process */
                        SEGMENT_PUSH;

                        continue;
                    } else {
                        goto fn_exit;
                    }

                    break;
                }

            case MPII_DATALOOP_KIND_INDEXED:
                {
                    MPI_Aint blocksize;
                    MPI_Aint num_blocks;

                    for (num_blocks = 0; num_blocks < cur_elmp->orig_count; num_blocks++) {
                        blocksize = STACKELM_INDEXED_BLOCKSIZE(cur_elmp, num_blocks);

                        if (position - segp->stream_off < cur_elmp->loop_p->el_size * blocksize) {
                            cur_elmp->orig_block = blocksize;
                            break;
                        }

                        segp->stream_off += cur_elmp->loop_p->el_size * blocksize;
                    }

                    blocksize = STACKELM_INDEXED_BLOCKSIZE(cur_elmp, num_blocks);

                    MPI_Aint num_elems = (position - segp->stream_off) / cur_elmp->loop_p->el_size;
                    MPIR_Assert(num_elems < blocksize);
                    segp->stream_off += num_elems * cur_elmp->loop_p->el_size;

                    /* current (remaining) block count */
                    cur_elmp->curcount = cur_elmp->orig_count - num_blocks;

                    /* current (remaining) block size */
                    cur_elmp->curblock = blocksize - num_elems;

                    /* current offset */
                    cur_elmp->curoffset = cur_elmp->orig_offset +
                        num_elems * cur_elmp->loop_p->el_extent +
                        STACKELM_INDEXED_OFFSET(cur_elmp, num_blocks);

                    /* if there is a child element, setup its
                     * parameters */
                    if ((cur_elmp->loop_p->kind & MPII_DATALOOP_FINAL_MASK) == 0) {
                        next_elmp = &(segp->stackelm[cur_sp + 1]);
                        next_elmp->orig_offset = cur_elmp->curoffset;
                        cur_elmp->curoffset = cur_elmp->orig_offset;

                        cur_elmp->curblock--;
                        segp->cur_sp++;

                        /* we can't skip any large blocks at this
                         * level anymore; move one level lower and
                         * repeat the same process */
                        SEGMENT_PUSH;

                        continue;
                    } else {
                        goto fn_exit;
                    }

                    break;
                }

            case MPII_DATALOOP_KIND_STRUCT:
                {
                    MPI_Aint blocksize;
                    MPI_Aint num_blocks;
                    MPII_Dataloop *dloop;

                    for (num_blocks = 0; num_blocks < cur_elmp->orig_count; num_blocks++) {
                        blocksize = STACKELM_INDEXED_BLOCKSIZE(cur_elmp, num_blocks);
                        dloop = STACKELM_STRUCT_DATALOOP(cur_elmp, num_blocks);

                        if (position - segp->stream_off < dloop->el_size * blocksize) {
                            cur_elmp->orig_block = blocksize;
                            break;
                        }

                        segp->stream_off += cur_elmp->loop_p->el_size * blocksize;
                    }

                    blocksize = STACKELM_INDEXED_BLOCKSIZE(cur_elmp, num_blocks);
                    dloop = STACKELM_STRUCT_DATALOOP(cur_elmp, num_blocks);

                    MPI_Aint num_elems = (position - segp->stream_off) / dloop->el_size;
                    MPIR_Assert(num_elems < blocksize);
                    segp->stream_off += num_elems * dloop->el_size;

                    /* current (remaining) block count */
                    cur_elmp->curcount = cur_elmp->orig_count - num_blocks;

                    /* current (remaining) block size */
                    cur_elmp->curblock = blocksize - num_elems;

                    /* current offset */
                    cur_elmp->curoffset = cur_elmp->orig_offset +
                        num_elems * STACKELM_STRUCT_EL_EXTENT(cur_elmp, num_blocks) +
                        STACKELM_STRUCT_OFFSET(cur_elmp, num_blocks);

                    /* structs cannot be leaves */
                    MPIR_Assert((cur_elmp->loop_p->kind & MPII_DATALOOP_FINAL_MASK) == 0);

                    /* if there is a child element, setup its
                     * parameters */
                    if ((cur_elmp->loop_p->kind & MPII_DATALOOP_FINAL_MASK) == 0) {
                        next_elmp = &(segp->stackelm[cur_sp + 1]);
                        next_elmp->orig_offset = cur_elmp->curoffset;
                        cur_elmp->curoffset = cur_elmp->orig_offset;

                        cur_elmp->curblock--;
                        segp->cur_sp++;

                        /* we can't skip any large blocks at this
                         * level anymore; move one level lower and
                         * repeat the same process */
                        SEGMENT_PUSH;

                        continue;
                    } else {
                        goto fn_exit;
                    }

                    break;
                }

            default:
                goto fallback_path;
        }

        MPIR_Assert(segp->stream_off == position);
        break;
    }

    goto fn_exit;

  fallback_path:
    {
        MPI_Aint tmp_last = position;

        /* use manipulate function with a NULL piecefn to advance
         * stream offset */
        MPII_Segment_manipulate(segp, segp->stream_off, &tmp_last, NULL,        /* contig fn */
                                NULL,   /* vector fn */
                                NULL,   /* blkidx fn */
                                NULL,   /* index fn */
                                sizefn, NULL);

        /* --BEGIN ERROR HANDLING-- */
        /* verify that we're in the right location */
        MPIR_Assert(tmp_last == position);
        /* --END ERROR HANDLING-- */
    }

  fn_exit:
    return;
}

void MPII_Segment_manipulate(struct MPIR_Segment *segp,
                             MPI_Aint first,
                             MPI_Aint * lastp,
                             int (*contigfn) (MPI_Aint * blocks_p,
                                              MPI_Datatype el_type,
                                              MPI_Aint rel_off,
                                              void *bufp,
                                              void *v_paramp),
                             int (*vectorfn) (MPI_Aint * blocks_p,
                                              MPI_Aint count,
                                              MPI_Aint blklen,
                                              MPI_Aint stride,
                                              MPI_Datatype el_type,
                                              MPI_Aint rel_off,
                                              void *bufp,
                                              void *v_paramp),
                             int (*blkidxfn) (MPI_Aint * blocks_p,
                                              MPI_Aint count,
                                              MPI_Aint blklen,
                                              MPI_Aint * offsetarray,
                                              MPI_Datatype el_type,
                                              MPI_Aint rel_off,
                                              void *bufp,
                                              void *v_paramp),
                             int (*indexfn) (MPI_Aint * blocks_p,
                                             MPI_Aint count,
                                             MPI_Aint * blockarray,
                                             MPI_Aint * offsetarray,
                                             MPI_Datatype el_type,
                                             MPI_Aint rel_off,
                                             void *bufp,
                                             void *v_paramp),
                             MPI_Aint(*sizefn) (MPI_Datatype el_type), void *pieceparams)
{
    /* these four are the "local values": cur_sp, valid_sp, last, stream_off */
    int cur_sp, valid_sp;
    MPI_Aint last, stream_off;

    struct MPII_Dataloop_stackelm *cur_elmp;
    enum { PF_NULL, PF_CONTIG, PF_VECTOR, PF_BLOCKINDEXED, PF_INDEXED } piecefn_type = PF_NULL;

    SEGMENT_LOAD_LOCAL_VALUES;

    if (first == *lastp) {
        /* nothing to do */
        MPL_DBG_MSG_FMT(MPIR_DBG_DATATYPE, VERBOSE,
                        (MPL_DBG_FDEST,
                         "dloop_segment_manipulate: warning: first == last ("
                         MPI_AINT_FMT_DEC_SPEC ")\n", first));
        return;
    }

    /* first we ensure that stream_off and first are in the same spot */
    if (first != stream_off) {
#ifdef MPII_DATALOOP_DEBUG_MANIPULATE
        MPL_DBG_MSG_FMT(MPIR_DBG_DATATYPE, VERBOSE,
                        (MPL_DBG_FDEST,
                         "first=" MPI_AINT_FMT_DEC_SPEC "; stream_off="
                         MPI_AINT_FMT_DEC_SPEC "; resetting.\n", first, stream_off));
#endif

        if (first < stream_off) {
            SEGMENT_RESET_VALUES;
            stream_off = 0;
        }

        if (first != stream_off) {
            segment_seek(segp, first, sizefn);
        }

        SEGMENT_LOAD_LOCAL_VALUES;

#ifdef MPII_DATALOOP_DEBUG_MANIPULATE
        MPL_DBG_MSG_FMT(MPIR_DBG_DATATYPE, VERBOSE,
                        (MPL_DBG_FDEST,
                         "done repositioning stream_off; first=" MPI_AINT_FMT_DEC_SPEC
                         ", stream_off=" MPI_AINT_FMT_DEC_SPEC ", last="
                         MPI_AINT_FMT_DEC_SPEC "\n", first, stream_off, last));
#endif
    }

    for (;;) {
#ifdef MPII_DATALOOP_DEBUG_MANIPULATE
#if 0
        MPL_DBG_MSG_FMT(MPIR_DBG_DATATYPE, VERBOSE,
                        (MPL_DBG_FDEST, "looptop; cur_sp=%d, cur_elmp=%x\n", cur_sp,
                         (unsigned) cur_elmp));
#endif
#endif

        if (cur_elmp->loop_p->kind & MPII_DATALOOP_FINAL_MASK) {
            int piecefn_indicated_exit = -1;
            MPI_Aint myblocks, local_el_size, stream_el_size;
            MPI_Datatype el_type;

            /* structs are never finals (leaves) */
            MPIR_Assert((cur_elmp->loop_p->kind & MPII_DATALOOP_KIND_MASK) !=
                        MPII_DATALOOP_KIND_STRUCT);

            /* pop immediately on zero count */
            if (cur_elmp->curcount == 0)
                SEGMENT_POP_AND_MAYBE_EXIT;

            /* size on this system of the int, double, etc. that is
             * the elementary type.
             */
            local_el_size = cur_elmp->loop_p->el_size;
            el_type = cur_elmp->loop_p->el_type;
            stream_el_size = (sizefn) ? sizefn(el_type) : local_el_size;

            /* calculate number of elem. types to work on and function to use.
             * default is to use the contig piecefn (if there is one).
             */
            myblocks = cur_elmp->curblock;
            piecefn_type = (contigfn ? PF_CONTIG : PF_NULL);

            /* check for opportunities to use other piecefns */
            switch (cur_elmp->loop_p->kind & MPII_DATALOOP_KIND_MASK) {
                case MPII_DATALOOP_KIND_CONTIG:
                    break;
                case MPII_DATALOOP_KIND_BLOCKINDEXED:
                    /* only use blkidx piecefn if at start of blkidx type */
                    if (blkidxfn &&
                        cur_elmp->orig_block == cur_elmp->curblock &&
                        cur_elmp->orig_count == cur_elmp->curcount) {
                        /* TODO: RELAX CONSTRAINTS */
                        myblocks = cur_elmp->curblock * cur_elmp->curcount;
                        piecefn_type = PF_BLOCKINDEXED;
                    }
                    break;
                case MPII_DATALOOP_KIND_INDEXED:
                    /* only use index piecefn if at start of the index type.
                     *   count test checks that we're on first block.
                     *   block test checks that we haven't made progress on first block.
                     */
                    if (indexfn &&
                        cur_elmp->orig_count == cur_elmp->curcount &&
                        cur_elmp->curblock == STACKELM_INDEXED_BLOCKSIZE(cur_elmp, 0)) {
                        /* TODO: RELAX CONSTRAINT ON COUNT? */
                        myblocks = cur_elmp->loop_p->loop_params.i_t.total_blocks;
                        piecefn_type = PF_INDEXED;
                    }
                    break;
                case MPII_DATALOOP_KIND_VECTOR:
                    /* only use the vector piecefn if at the start of a
                     * contiguous block.
                     */
                    if (vectorfn && cur_elmp->orig_block == cur_elmp->curblock) {
                        myblocks = cur_elmp->curblock * cur_elmp->curcount;
                        piecefn_type = PF_VECTOR;
                    }
                    break;
                default:
                    /* --BEGIN ERROR HANDLING-- */
                    MPIR_Assert(0);
                    break;
                    /* --END ERROR HANDLING-- */
            }

#ifdef MPII_DATALOOP_DEBUG_MANIPULATE
            MPL_DBG_MSG_FMT(MPIR_DBG_DATATYPE, VERBOSE,
                            (MPL_DBG_FDEST,
                             "\thit leaf; cur_sp=%d, elmp=%x, piece_sz=" MPI_AINT_FMT_DEC_SPEC
                             "\n", cur_sp, (unsigned) cur_elmp, myblocks * local_el_size));
#endif

            /* enforce the last parameter if necessary by reducing myblocks */
            if (last != MPIR_SEGMENT_IGNORE_LAST &&
                (stream_off + (myblocks * stream_el_size) > last)) {
                myblocks = ((last - stream_off) / stream_el_size);
#ifdef MPII_DATALOOP_DEBUG_MANIPULATE
                MPL_DBG_MSG_FMT(MPIR_DBG_DATATYPE, VERBOSE,
                                (MPL_DBG_FDEST,
                                 "\tpartial block count=" MPI_AINT_FMT_DEC_SPEC " ("
                                 MPI_AINT_FMT_DEC_SPEC " bytes)\n", myblocks,
                                 myblocks * stream_el_size));
#endif
                if (myblocks == 0) {
                    SEGMENT_SAVE_LOCAL_VALUES;
                    return;
                }
            }

            /* call piecefn to perform data manipulation */
            switch (piecefn_type) {
                case PF_NULL:
                    piecefn_indicated_exit = 0;
#ifdef MPII_DATALOOP_DEBUG_MANIPULATE
                    MPL_DBG_MSG("\tNULL piecefn for this piece\n");
#endif
                    break;
                case PF_CONTIG:
                    MPIR_Assert(myblocks <= cur_elmp->curblock);
                    piecefn_indicated_exit = contigfn(&myblocks, el_type, cur_elmp->curoffset,  /* relative to segp->ptr */
                                                      segp->ptr,        /* start of buffer (from segment) */
                                                      pieceparams);
                    break;
                case PF_VECTOR:
                    piecefn_indicated_exit =
                        vectorfn(&myblocks,
                                 cur_elmp->curcount,
                                 cur_elmp->orig_block,
                                 cur_elmp->loop_p->loop_params.v_t.stride,
                                 el_type, cur_elmp->curoffset, segp->ptr, pieceparams);
                    break;
                case PF_BLOCKINDEXED:
                    piecefn_indicated_exit = blkidxfn(&myblocks, cur_elmp->curcount, cur_elmp->orig_block, cur_elmp->loop_p->loop_params.bi_t.offset_array, el_type, cur_elmp->orig_offset,     /* blkidxfn adds offset */
                                                      segp->ptr, pieceparams);
                    break;
                case PF_INDEXED:
                    piecefn_indicated_exit = indexfn(&myblocks, cur_elmp->curcount, cur_elmp->loop_p->loop_params.i_t.blocksize_array, cur_elmp->loop_p->loop_params.i_t.offset_array, el_type, cur_elmp->orig_offset,  /* indexfn adds offset value */
                                                     segp->ptr, pieceparams);
                    break;
            }

            /* update local values based on piecefn returns (myblocks and
             * piecefn_indicated_exit)
             */
            MPIR_Assert(piecefn_indicated_exit >= 0);
            MPIR_Assert(myblocks >= 0);
            stream_off += myblocks * stream_el_size;

            /* myblocks of 0 or less than cur_elmp->curblock indicates
             * that we should stop processing and return.
             */
            if (myblocks == 0) {
                SEGMENT_SAVE_LOCAL_VALUES;
                return;
            } else if (myblocks < (MPI_Aint) (cur_elmp->curblock)) {
                cur_elmp->curoffset += myblocks * local_el_size;
                cur_elmp->curblock -= myblocks;

                SEGMENT_SAVE_LOCAL_VALUES;
                return;
            } else {    /* myblocks >= cur_elmp->curblock */

                MPI_Aint count_index = 0;

                /* this assumes we're either *just* processing the last parts
                 * of the current block, or we're processing as many blocks as
                 * we like starting at the beginning of one.
                 */

                switch (cur_elmp->loop_p->kind & MPII_DATALOOP_KIND_MASK) {
                    case MPII_DATALOOP_KIND_INDEXED:
                        while (myblocks > 0 && myblocks >= (MPI_Aint) (cur_elmp->curblock)) {
                            myblocks -= (MPI_Aint) (cur_elmp->curblock);
                            cur_elmp->curcount--;
                            MPIR_Assert(cur_elmp->curcount >= 0);

                            count_index = cur_elmp->orig_count - cur_elmp->curcount;
                            cur_elmp->curblock = STACKELM_INDEXED_BLOCKSIZE(cur_elmp, count_index);
                        }

                        if (cur_elmp->curcount == 0) {
                            /* don't bother to fill in values; we're popping anyway */
                            MPIR_Assert(myblocks == 0);
                            SEGMENT_POP_AND_MAYBE_EXIT;
                        } else {
                            cur_elmp->orig_block = cur_elmp->curblock;
                            cur_elmp->curoffset = cur_elmp->orig_offset +
                                STACKELM_INDEXED_OFFSET(cur_elmp, count_index);

                            cur_elmp->curblock -= myblocks;
                            cur_elmp->curoffset += myblocks * local_el_size;
                        }
                        break;
                    case MPII_DATALOOP_KIND_VECTOR:
                        /* this math relies on assertions at top of code block */
                        cur_elmp->curcount -= myblocks / (MPI_Aint) (cur_elmp->curblock);
                        if (cur_elmp->curcount == 0) {
                            MPIR_Assert(myblocks % ((MPI_Aint) (cur_elmp->curblock)) == 0);
                            SEGMENT_POP_AND_MAYBE_EXIT;
                        } else {
                            /* this math relies on assertions at top of code
                             * block
                             */
                            cur_elmp->curblock = cur_elmp->orig_block -
                                (myblocks % (MPI_Aint) (cur_elmp->curblock));
                            /* new offset = original offset +
                             *              stride * whole blocks +
                             *              leftover bytes
                             */
                            cur_elmp->curoffset = cur_elmp->orig_offset +
                                (((MPI_Aint) (cur_elmp->orig_count - cur_elmp->curcount)) *
                                 cur_elmp->loop_p->loop_params.v_t.stride) +
                                (((MPI_Aint) (cur_elmp->orig_block - cur_elmp->curblock)) *
                                 local_el_size);
                        }
                        break;
                    case MPII_DATALOOP_KIND_CONTIG:
                        /* contigs that reach this point have always been
                         * completely processed
                         */
                        MPIR_Assert(myblocks == (MPI_Aint) (cur_elmp->curblock) &&
                                    cur_elmp->curcount == 1);
                        SEGMENT_POP_AND_MAYBE_EXIT;
                        break;
                    case MPII_DATALOOP_KIND_BLOCKINDEXED:
                        while (myblocks > 0 && myblocks >= (MPI_Aint) (cur_elmp->curblock)) {
                            myblocks -= (MPI_Aint) (cur_elmp->curblock);
                            cur_elmp->curcount--;
                            MPIR_Assert(cur_elmp->curcount >= 0);

                            count_index = cur_elmp->orig_count - cur_elmp->curcount;
                            cur_elmp->curblock = cur_elmp->orig_block;
                        }
                        if (cur_elmp->curcount == 0) {
                            /* popping */
                            MPIR_Assert(myblocks == 0);
                            SEGMENT_POP_AND_MAYBE_EXIT;
                        } else {
                            /* cur_elmp->orig_block = cur_elmp->curblock; */
                            cur_elmp->curoffset = cur_elmp->orig_offset +
                                STACKELM_BLOCKINDEXED_OFFSET(cur_elmp, count_index);
                            cur_elmp->curblock -= myblocks;
                            cur_elmp->curoffset += myblocks * local_el_size;
                        }
                        break;
                }
            }

            if (piecefn_indicated_exit) {
                /* piece function indicated that we should quit processing */
                SEGMENT_SAVE_LOCAL_VALUES;
                return;
            }
        } /* end of if leaf */
        else if (cur_elmp->curblock == 0) {
#ifdef MPII_DATALOOP_DEBUG_MANIPULATE
            MPL_DBG_MSG_FMT(MPIR_DBG_DATATYPE, VERBOSE,
                            (MPL_DBG_FDEST, "\thit end of block; elmp=%x [%d]\n",
                             (unsigned) cur_elmp, cur_sp));
#endif
            cur_elmp->curcount--;

            /* new block.  for indexed and struct reset orig_block.
             * reset curblock for all types
             */
            switch (cur_elmp->loop_p->kind & MPII_DATALOOP_KIND_MASK) {
                case MPII_DATALOOP_KIND_CONTIG:
                case MPII_DATALOOP_KIND_VECTOR:
                case MPII_DATALOOP_KIND_BLOCKINDEXED:
                    break;
                case MPII_DATALOOP_KIND_INDEXED:
                    cur_elmp->orig_block =
                        STACKELM_INDEXED_BLOCKSIZE(cur_elmp,
                                                   cur_elmp->curcount ? cur_elmp->orig_count -
                                                   cur_elmp->curcount : 0);
                    break;
                case MPII_DATALOOP_KIND_STRUCT:
                    cur_elmp->orig_block =
                        STACKELM_STRUCT_BLOCKSIZE(cur_elmp,
                                                  cur_elmp->curcount ? cur_elmp->orig_count -
                                                  cur_elmp->curcount : 0);
                    break;
                default:
                    /* --BEGIN ERROR HANDLING-- */
                    MPIR_Assert(0);
                    break;
                    /* --END ERROR HANDLING-- */
            }
            cur_elmp->curblock = cur_elmp->orig_block;

            if (cur_elmp->curcount == 0) {
#ifdef MPII_DATALOOP_DEBUG_MANIPULATE
                MPL_DBG_MSG_FMT(MPIR_DBG_DATATYPE, VERBOSE,
                                (MPL_DBG_FDEST, "\talso hit end of count; elmp=%x [%d]\n",
                                 (unsigned) cur_elmp, cur_sp));
#endif
                SEGMENT_POP_AND_MAYBE_EXIT;
            }
        } else {        /* push the stackelm */

            MPII_Dataloop_stackelm *next_elmp;
            MPI_Aint count_index, block_index;

            count_index = cur_elmp->orig_count - cur_elmp->curcount;
            block_index = cur_elmp->orig_block - cur_elmp->curblock;

            /* reload the next stackelm if necessary */
            next_elmp = &(segp->stackelm[cur_sp + 1]);
            if (cur_elmp->may_require_reloading) {
                MPII_Dataloop *load_dlp = NULL;
                switch (cur_elmp->loop_p->kind & MPII_DATALOOP_KIND_MASK) {
                    case MPII_DATALOOP_KIND_CONTIG:
                    case MPII_DATALOOP_KIND_VECTOR:
                    case MPII_DATALOOP_KIND_BLOCKINDEXED:
                    case MPII_DATALOOP_KIND_INDEXED:
                        load_dlp = cur_elmp->loop_p->loop_params.cm_t.dataloop;
                        break;
                    case MPII_DATALOOP_KIND_STRUCT:
                        load_dlp = STACKELM_STRUCT_DATALOOP(cur_elmp, count_index);
                        break;
                    default:
                        /* --BEGIN ERROR HANDLING-- */
                        MPIR_Assert(0);
                        break;
                        /* --END ERROR HANDLING-- */
                }

#ifdef MPII_DATALOOP_DEBUG_MANIPULATE
                MPL_DBG_MSG_FMT(MPIR_DBG_DATATYPE, VERBOSE,
                                (MPL_DBG_FDEST, "\tloading dlp=%x, elmp=%x [%d]\n",
                                 (unsigned) load_dlp, (unsigned) next_elmp, cur_sp + 1));
#endif

                MPII_Dataloop_stackelm_load(next_elmp, load_dlp, 1);
            }
#ifdef MPII_DATALOOP_DEBUG_MANIPULATE
            MPL_DBG_MSG_FMT(MPIR_DBG_DATATYPE, VERBOSE,
                            (MPL_DBG_FDEST, "\tpushing type, elmp=%x [%d], count=%d, block=%d\n",
                             (unsigned) cur_elmp, cur_sp, count_index, block_index));
#endif
            /* set orig_offset and all cur values for new stackelm.
             * this is done in two steps: first set orig_offset based on
             * current stackelm, then set cur values based on new stackelm.
             */
            switch (cur_elmp->loop_p->kind & MPII_DATALOOP_KIND_MASK) {
                case MPII_DATALOOP_KIND_CONTIG:
                    next_elmp->orig_offset = cur_elmp->curoffset +
                        (MPI_Aint) block_index *cur_elmp->loop_p->el_extent;
                    break;
                case MPII_DATALOOP_KIND_VECTOR:
                    /* note: stride is in bytes */
                    next_elmp->orig_offset = cur_elmp->orig_offset +
                        (MPI_Aint) count_index *cur_elmp->loop_p->loop_params.v_t.stride +
                        (MPI_Aint) block_index *cur_elmp->loop_p->el_extent;
                    break;
                case MPII_DATALOOP_KIND_BLOCKINDEXED:
                    next_elmp->orig_offset = cur_elmp->orig_offset +
                        (MPI_Aint) block_index *cur_elmp->loop_p->el_extent +
                        STACKELM_BLOCKINDEXED_OFFSET(cur_elmp, count_index);
                    break;
                case MPII_DATALOOP_KIND_INDEXED:
                    next_elmp->orig_offset = cur_elmp->orig_offset +
                        (MPI_Aint) block_index *cur_elmp->loop_p->el_extent +
                        STACKELM_INDEXED_OFFSET(cur_elmp, count_index);
                    break;
                case MPII_DATALOOP_KIND_STRUCT:
                    next_elmp->orig_offset = cur_elmp->orig_offset +
                        (MPI_Aint) block_index *STACKELM_STRUCT_EL_EXTENT(cur_elmp,
                                                                          count_index) +
                        STACKELM_STRUCT_OFFSET(cur_elmp, count_index);
                    break;
                default:
                    /* --BEGIN ERROR HANDLING-- */
                    MPIR_Assert(0);
                    break;
                    /* --END ERROR HANDLING-- */
            }

#ifdef MPII_DATALOOP_DEBUG_MANIPULATE
            MPL_DBG_MSG_FMT(MPIR_DBG_DATATYPE, VERBOSE,
                            (MPL_DBG_FDEST,
                             "\tstep 1: next orig_offset = " MPI_AINT_FMT_DEC_SPEC " (0x"
                             MPI_AINT_FMT_HEX_SPEC ")\n", next_elmp->orig_offset,
                             next_elmp->orig_offset));
#endif

            switch (next_elmp->loop_p->kind & MPII_DATALOOP_KIND_MASK) {
                case MPII_DATALOOP_KIND_CONTIG:
                case MPII_DATALOOP_KIND_VECTOR:
                    next_elmp->curcount = next_elmp->orig_count;
                    next_elmp->curblock = next_elmp->orig_block;
                    next_elmp->curoffset = next_elmp->orig_offset;
                    break;
                case MPII_DATALOOP_KIND_BLOCKINDEXED:
                    next_elmp->curcount = next_elmp->orig_count;
                    next_elmp->curblock = next_elmp->orig_block;
                    next_elmp->curoffset = next_elmp->orig_offset +
                        STACKELM_BLOCKINDEXED_OFFSET(next_elmp, 0);
                    break;
                case MPII_DATALOOP_KIND_INDEXED:
                    next_elmp->curcount = next_elmp->orig_count;
                    next_elmp->curblock = STACKELM_INDEXED_BLOCKSIZE(next_elmp, 0);
                    next_elmp->curoffset = next_elmp->orig_offset +
                        STACKELM_INDEXED_OFFSET(next_elmp, 0);
                    break;
                case MPII_DATALOOP_KIND_STRUCT:
                    next_elmp->curcount = next_elmp->orig_count;
                    next_elmp->curblock = STACKELM_STRUCT_BLOCKSIZE(next_elmp, 0);
                    next_elmp->curoffset = next_elmp->orig_offset +
                        STACKELM_STRUCT_OFFSET(next_elmp, 0);
                    break;
                default:
                    /* --BEGIN ERROR HANDLING-- */
                    MPIR_Assert(0);
                    break;
                    /* --END ERROR HANDLING-- */
            }

#ifdef MPII_DATALOOP_DEBUG_MANIPULATE
            MPL_DBG_MSG_FMT(MPIR_DBG_DATATYPE, VERBOSE,
                            (MPL_DBG_FDEST,
                             "\tstep 2: next curoffset = " MPI_AINT_FMT_DEC_SPEC " (0x"
                             MPI_AINT_FMT_HEX_SPEC ")\n", next_elmp->curoffset,
                             next_elmp->curoffset));
#endif

            cur_elmp->curblock--;
            SEGMENT_PUSH;
        }       /* end of else push the stackelm */
    }   /* end of for (;;) */

#ifdef MPII_DATALOOP_DEBUG_MANIPULATE
    MPL_DBG_MSG("hit end of datatype\n");
#endif

    SEGMENT_SAVE_LOCAL_VALUES;
    return;
}

/* MPII_Dataloop_stackelm_blocksize - returns block size for stackelm based on current
 * count in stackelm.
 *
 * NOTE: loop_p, orig_count, and curcount members of stackelm MUST be correct
 * before this is called!
 *
 */
MPI_Aint MPII_Dataloop_stackelm_blocksize(struct MPII_Dataloop_stackelm * elmp)
{
    MPII_Dataloop *dlp = elmp->loop_p;

    switch (dlp->kind & MPII_DATALOOP_KIND_MASK) {
        case MPII_DATALOOP_KIND_CONTIG:
            /* NOTE: we're dropping the count into the
             * blksize field for contigs, as described
             * in the init call.
             */
            return dlp->loop_params.c_t.count;
            break;
        case MPII_DATALOOP_KIND_VECTOR:
            return dlp->loop_params.v_t.blocksize;
            break;
        case MPII_DATALOOP_KIND_BLOCKINDEXED:
            return dlp->loop_params.bi_t.blocksize;
            break;
        case MPII_DATALOOP_KIND_INDEXED:
            return dlp->loop_params.i_t.blocksize_array[elmp->orig_count - elmp->curcount];
            break;
        case MPII_DATALOOP_KIND_STRUCT:
            return dlp->loop_params.s_t.blocksize_array[elmp->orig_count - elmp->curcount];
            break;
        default:
            /* --BEGIN ERROR HANDLING-- */
            MPIR_Assert(0);
            break;
            /* --END ERROR HANDLING-- */
    }
    return -1;
}

/* MPII_Dataloop_stackelm_offset - returns starting offset (displacement) for stackelm
 * based on current count in stackelm.
 *
 * NOTE: loop_p, orig_count, and curcount members of stackelm MUST be correct
 * before this is called!
 *
 * also, this really is only good at init time for vectors and contigs
 * (all the time for indexed) at the moment.
 *
 */
MPI_Aint MPII_Dataloop_stackelm_offset(struct MPII_Dataloop_stackelm * elmp)
{
    MPII_Dataloop *dlp = elmp->loop_p;

    switch (dlp->kind & MPII_DATALOOP_KIND_MASK) {
        case MPII_DATALOOP_KIND_VECTOR:
        case MPII_DATALOOP_KIND_CONTIG:
            return 0;
            break;
        case MPII_DATALOOP_KIND_BLOCKINDEXED:
            return dlp->loop_params.bi_t.offset_array[elmp->orig_count - elmp->curcount];
            break;
        case MPII_DATALOOP_KIND_INDEXED:
            return dlp->loop_params.i_t.offset_array[elmp->orig_count - elmp->curcount];
            break;
        case MPII_DATALOOP_KIND_STRUCT:
            return dlp->loop_params.s_t.offset_array[elmp->orig_count - elmp->curcount];
            break;
        default:
            /* --BEGIN ERROR HANDLING-- */
            MPIR_Assert(0);
            break;
            /* --END ERROR HANDLING-- */
    }
    return -1;
}

/* MPII_Dataloop_stackelm_load
 * loop_p, orig_count, orig_block, and curcount are all filled by us now.
 * the rest are filled in at processing time.
 */
void MPII_Dataloop_stackelm_load(struct MPII_Dataloop_stackelm *elmp,
                                 MPII_Dataloop * dlp, int branch_flag)
{
    elmp->loop_p = dlp;

    if ((dlp->kind & MPII_DATALOOP_KIND_MASK) == MPII_DATALOOP_KIND_CONTIG) {
        elmp->orig_count = 1;   /* put in blocksize instead */
    } else {
        elmp->orig_count = dlp->loop_params.count;
    }

    if (branch_flag || (dlp->kind & MPII_DATALOOP_KIND_MASK) == MPII_DATALOOP_KIND_STRUCT) {
        elmp->may_require_reloading = 1;
    } else {
        elmp->may_require_reloading = 0;
    }

    /* required by MPII_Dataloop_stackelm_blocksize */
    elmp->curcount = elmp->orig_count;

    elmp->orig_block = MPII_Dataloop_stackelm_blocksize(elmp);
    /* TODO: GO AHEAD AND FILL IN CURBLOCK? */
}

/*
 * Local variables:
 * c-indent-tabs-mode: nil
 * End:
 */
