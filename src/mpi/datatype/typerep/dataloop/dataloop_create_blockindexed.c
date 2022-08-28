/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"
#include "dataloop_internal.h"

#include <stdio.h>


static void blockindexed_array_copy(MPI_Aint count,
                                    const MPI_Aint * disp_array,
                                    MPI_Aint * out_disp_array,
                                    int dispinbytes, MPI_Aint old_extent);

/*@
   Dataloop_create_blockindexed - create blockindexed dataloop

   Arguments:
+  MPI_Aint count
.  void *displacement_array (array of either MPI_Aints or ints)
.  int displacement_in_bytes (boolean)
.  MPI_Datatype old_type
.  MPII_Dataloop **output_dataloop_ptr

.N Errors
.N Returns 0 on success, -1 on failure.
@*/
int MPIR_Dataloop_create_blockindexed(MPI_Aint icount,
                                      MPI_Aint iblklen,
                                      const MPI_Aint * disp_array,
                                      int dispinbytes, MPI_Datatype oldtype, void **dlp_p)
{
    int err, is_builtin, is_vectorizable = 1;
    int i;
    MPI_Aint new_loop_sz;

    MPI_Aint contig_count, count, blklen;
    MPI_Aint old_extent, eff_disp0, eff_disp1, last_stride;
    MPII_Dataloop *new_dlp;

    count = (MPI_Aint) icount;  /* avoid subsequent casting */
    blklen = (MPI_Aint) iblklen;

    /* if count or blklen are zero, handle with contig code, call it a int */
    if (count == 0 || blklen == 0) {
        err = MPIR_Dataloop_create_contiguous(0, MPI_INT, (void **) dlp_p);
        return err;
    }

    is_builtin = (MPII_DATALOOP_HANDLE_HASLOOP(oldtype)) ? 0 : 1;

    if (is_builtin) {
        MPIR_Datatype_get_size_macro(oldtype, old_extent);
    } else {
        MPIR_Datatype_get_extent_macro(oldtype, old_extent);
    }

    contig_count = MPII_Datatype_blockindexed_count_contig(count,
                                                           blklen, disp_array, dispinbytes,
                                                           old_extent);

    /* optimization:
     *
     * if contig_count == 1 and block starts at displacement 0,
     * store it as a contiguous rather than a blockindexed dataloop.
     */
    if ((contig_count == 1) && disp_array[0] == 0) {
        err = MPIR_Dataloop_create_contiguous(icount * iblklen, oldtype, (void **) dlp_p);
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
    eff_disp0 = (dispinbytes) ? disp_array[0] : disp_array[0] * old_extent;

    if (count > 1 && eff_disp0 == (MPI_Aint) 0) {
        eff_disp1 = (dispinbytes) ? disp_array[1] : disp_array[1] * old_extent;
        last_stride = eff_disp1 - eff_disp0;

        for (i = 2; i < count; i++) {
            eff_disp0 = eff_disp1;
            eff_disp1 = (dispinbytes) ? disp_array[i] : disp_array[i] * old_extent;
            if (eff_disp1 - eff_disp0 != last_stride) {
                is_vectorizable = 0;
                break;
            }
        }
        if (is_vectorizable) {
            err = MPIR_Dataloop_create_vector(count, blklen, last_stride, 1,    /* strideinbytes */
                                              oldtype, (void **) dlp_p);
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

    int old_is_contig;
    MPI_Aint old_num_contig;
    if (is_builtin) {
        MPII_Dataloop_alloc(MPII_DATALOOP_KIND_BLOCKINDEXED, count, &new_dlp);
        /* --BEGIN ERROR HANDLING-- */
        if (!new_dlp)
            return -1;
        /* --END ERROR HANDLING-- */
        new_loop_sz = new_dlp->dloop_sz;

        new_dlp->kind = MPII_DATALOOP_KIND_BLOCKINDEXED | MPII_DATALOOP_FINAL_MASK;

        new_dlp->el_size = old_extent;
        new_dlp->el_extent = old_extent;
        new_dlp->el_type = oldtype;
        old_is_contig = 1;
        old_num_contig = 1;
    } else {
        MPII_Dataloop *old_loop_ptr = NULL;

        MPIR_DATALOOP_GET_LOOPPTR(oldtype, old_loop_ptr);

        MPII_Dataloop_alloc_and_copy(MPII_DATALOOP_KIND_BLOCKINDEXED,
                                     count, old_loop_ptr, &new_dlp);
        /* --BEGIN ERROR HANDLING-- */
        if (!new_dlp)
            return -1;
        /* --END ERROR HANDLING-- */
        new_loop_sz = new_dlp->dloop_sz;

        new_dlp->kind = MPII_DATALOOP_KIND_BLOCKINDEXED;

        MPIR_Datatype_get_size_macro(oldtype, new_dlp->el_size);
        MPIR_Datatype_get_extent_macro(oldtype, new_dlp->el_extent);
        MPIR_Datatype_get_basic_type(oldtype, new_dlp->el_type);
        old_is_contig = old_loop_ptr->is_contig;
        old_num_contig = old_loop_ptr->num_contig;
    }

    new_dlp->loop_params.bi_t.count = count;
    new_dlp->loop_params.bi_t.blocksize = blklen;

    /* copy in displacement parameters
     *
     * regardless of dispinbytes, we store displacements in bytes in loop.
     */
    blockindexed_array_copy(count,
                            disp_array,
                            new_dlp->loop_params.bi_t.offset_array, dispinbytes, old_extent);

    *dlp_p = new_dlp;
    new_dlp->dloop_sz = new_loop_sz;

    /* all trivial cases have been optimized away */
    new_dlp->is_contig = 0;
    if (old_is_contig) {
        new_dlp->num_contig = count;
    } else {
        new_dlp->num_contig = count * blklen * old_num_contig;
    }

    return 0;
}

/* blockindexed_array_copy
 *
 * Unlike the indexed version, this one does not compact adjacent
 * blocks, because that would really mess up the blockindexed type!
 */
static void blockindexed_array_copy(MPI_Aint count,
                                    const MPI_Aint * in_disp_array,
                                    MPI_Aint * out_disp_array, int dispinbytes, MPI_Aint old_extent)
{
    int i;
    if (!dispinbytes) {
        for (i = 0; i < count; i++) {
            out_disp_array[i] = in_disp_array[i] * old_extent;
        }
    } else {
        for (i = 0; i < count; i++) {
            out_disp_array[i] = in_disp_array[i];
        }
    }
    return;
}
