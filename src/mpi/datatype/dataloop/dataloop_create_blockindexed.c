/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */

/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

#include <stdio.h>


static void DLOOP_Type_blockindexed_array_copy(MPI_Aint count,
                                               const void *disp_array,
                                               MPI_Aint * out_disp_array,
                                               int dispinbytes, MPI_Aint old_extent);

/*@
   Dataloop_create_blockindexed - create blockindexed dataloop

   Arguments:
+  MPI_Aint count
.  void *displacement_array (array of either MPI_Aints or ints)
.  int displacement_in_bytes (boolean)
.  MPI_Datatype old_type
.  MPIR_Dataloop **output_dataloop_ptr
.  int output_dataloop_size

.N Errors
.N Returns 0 on success, -1 on failure.
@*/
int MPIR_Dataloop_create_blockindexed(MPI_Aint icount,
                                      MPI_Aint iblklen,
                                      const void *disp_array,
                                      int dispinbytes,
                                      MPI_Datatype oldtype,
                                      MPIR_Dataloop ** dlp_p, MPI_Aint * dlsz_p)
{
    int err, is_builtin, is_vectorizable = 1;
    int i;
    MPI_Aint new_loop_sz;

    MPI_Aint contig_count, count, blklen;
    MPI_Aint old_extent, eff_disp0, eff_disp1, last_stride;
    MPIR_Dataloop *new_dlp;

    count = (MPI_Aint) icount;  /* avoid subsequent casting */
    blklen = (MPI_Aint) iblklen;

    /* if count or blklen are zero, handle with contig code, call it a int */
    if (count == 0 || blklen == 0) {
        err = MPIR_Dataloop_create_contiguous(0, MPI_INT, dlp_p, dlsz_p);
        return err;
    }

    is_builtin = (DLOOP_Handle_hasloop_macro(oldtype)) ? 0 : 1;

    if (is_builtin) {
        MPIR_Datatype_get_size_macro(oldtype, old_extent);
    } else {
        MPIR_Datatype_get_extent_macro(oldtype, old_extent);
    }

    contig_count = MPIR_Type_blockindexed_count_contig(count,
                                                       blklen, disp_array, dispinbytes, old_extent);

    /* optimization:
     *
     * if contig_count == 1 and block starts at displacement 0,
     * store it as a contiguous rather than a blockindexed dataloop.
     */
    if ((contig_count == 1) &&
        ((!dispinbytes && ((int *) disp_array)[0] == 0) ||
         (dispinbytes && ((MPI_Aint *) disp_array)[0] == 0))) {
        err = MPIR_Dataloop_create_contiguous(icount * iblklen, oldtype, dlp_p, dlsz_p);
        return err;
    }

    /* optimization:
     *
     * if contig_count == 1 store it as a blockindexed with one
     * element rather than as a lot of individual blocks.
     */
    if (contig_count == 1) {
        /* adjust count and blklen and drop through */
        blklen *= count;
        count = 1;
        iblklen *= icount;
        icount = 1;
    }

    /* optimization:
     *
     * if displacements start at zero and result in a fixed stride,
     * store it as a vector rather than a blockindexed dataloop.
     */
    eff_disp0 = (dispinbytes) ? ((MPI_Aint) ((MPI_Aint *) disp_array)[0]) :
        (((MPI_Aint) ((int *) disp_array)[0]) * old_extent);

    if (count > 1 && eff_disp0 == (MPI_Aint) 0) {
        eff_disp1 = (dispinbytes) ?
            ((MPI_Aint) ((MPI_Aint *) disp_array)[1]) :
            (((MPI_Aint) ((int *) disp_array)[1]) * old_extent);
        last_stride = eff_disp1 - eff_disp0;

        for (i = 2; i < count; i++) {
            eff_disp0 = eff_disp1;
            eff_disp1 = (dispinbytes) ?
                ((MPI_Aint) ((MPI_Aint *) disp_array)[i]) :
                (((MPI_Aint) ((int *) disp_array)[i]) * old_extent);
            if (eff_disp1 - eff_disp0 != last_stride) {
                is_vectorizable = 0;
                break;
            }
        }
        if (is_vectorizable) {
            err = MPIR_Dataloop_create_vector(count, blklen, last_stride, 1,    /* strideinbytes */
                                              oldtype, dlp_p, dlsz_p);
            return err;
        }
    }

    /* TODO: optimization:
     *
     * if displacements result in a fixed stride, but first displacement
     * is not zero, store it as a blockindexed (blklen == 1) of a vector.
     */

    /* TODO: optimization:
     *
     * if a blockindexed of a contig, absorb the contig into the blocklen
     * parameter and keep the same overall depth
     */

    /* otherwise storing as a blockindexed dataloop */

    /* Q: HOW CAN WE TELL IF IT IS WORTH IT TO STORE AS AN
     * INDEXED WITH FEWER CONTIG BLOCKS (IF CONTIG_COUNT IS SMALL)?
     */

    if (is_builtin) {
        MPIR_Dataloop_alloc(DLOOP_KIND_BLOCKINDEXED, count, &new_dlp, &new_loop_sz);
        /* --BEGIN ERROR HANDLING-- */
        if (!new_dlp)
            return -1;
        /* --END ERROR HANDLING-- */

        new_dlp->kind = DLOOP_KIND_BLOCKINDEXED | DLOOP_FINAL_MASK;

        new_dlp->el_size = old_extent;
        new_dlp->el_extent = old_extent;
        new_dlp->el_type = oldtype;
    } else {
        MPIR_Dataloop *old_loop_ptr = NULL;
        MPI_Aint old_loop_sz = 0;

        MPIR_Datatype_get_loopptr_macro(oldtype, old_loop_ptr);
        MPIR_Datatype_get_loopsize_macro(oldtype, old_loop_sz);

        MPIR_Dataloop_alloc_and_copy(DLOOP_KIND_BLOCKINDEXED,
                                     count, old_loop_ptr, old_loop_sz, &new_dlp, &new_loop_sz);
        /* --BEGIN ERROR HANDLING-- */
        if (!new_dlp)
            return -1;
        /* --END ERROR HANDLING-- */

        new_dlp->kind = DLOOP_KIND_BLOCKINDEXED;

        MPIR_Datatype_get_size_macro(oldtype, new_dlp->el_size);
        MPIR_Datatype_get_extent_macro(oldtype, new_dlp->el_extent);
        MPIR_Datatype_get_basic_type(oldtype, new_dlp->el_type);
    }

    new_dlp->loop_params.bi_t.count = count;
    new_dlp->loop_params.bi_t.blocksize = blklen;

    /* copy in displacement parameters
     *
     * regardless of dispinbytes, we store displacements in bytes in loop.
     */
    DLOOP_Type_blockindexed_array_copy(count,
                                       disp_array,
                                       new_dlp->loop_params.bi_t.offset_array,
                                       dispinbytes, old_extent);

    *dlp_p = new_dlp;
    *dlsz_p = new_loop_sz;

    return 0;
}

/* DLOOP_Type_blockindexed_array_copy
 *
 * Unlike the indexed version, this one does not compact adjacent
 * blocks, because that would really mess up the blockindexed type!
 */
static void DLOOP_Type_blockindexed_array_copy(MPI_Aint count,
                                               const void *in_disp_array,
                                               MPI_Aint * out_disp_array,
                                               int dispinbytes, MPI_Aint old_extent)
{
    int i;
    if (!dispinbytes) {
        for (i = 0; i < count; i++) {
            out_disp_array[i] = ((MPI_Aint) ((int *) in_disp_array)[i]) * old_extent;
        }
    } else {
        for (i = 0; i < count; i++) {
            out_disp_array[i] = ((MPI_Aint) ((MPI_Aint *) in_disp_array)[i]);
        }
    }
    return;
}

MPI_Aint MPIR_Type_blockindexed_count_contig(MPI_Aint count,
                                             MPI_Aint blklen,
                                             const void *disp_array,
                                             int dispinbytes, MPI_Aint old_extent)
{
    int i, contig_count = 1;

    if (!dispinbytes) {
        /* this is from the MPI type, is of type int */
        MPI_Aint cur_tdisp = (MPI_Aint) ((int *) disp_array)[0];

        for (i = 1; i < count; i++) {
            MPI_Aint next_tdisp = (MPI_Aint) ((int *) disp_array)[i];

            if (cur_tdisp + (MPI_Aint) blklen != next_tdisp) {
                contig_count++;
            }
            cur_tdisp = next_tdisp;
        }
    } else {
        /* this is from the MPI type, is of type MPI_Aint */
        MPI_Aint cur_bdisp = (MPI_Aint) ((MPI_Aint *) disp_array)[0];

        for (i = 1; i < count; i++) {
            MPI_Aint next_bdisp = (MPI_Aint) ((MPI_Aint *) disp_array)[i];

            if (cur_bdisp + (MPI_Aint) blklen * old_extent != next_bdisp) {
                contig_count++;
            }
            cur_bdisp = next_bdisp;
        }
    }
    return contig_count;
}
