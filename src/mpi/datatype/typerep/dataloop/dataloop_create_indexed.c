/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */

/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"
#include "dataloop_internal.h"

#include <stdlib.h>

static void indexed_array_copy(size_t count,
                               size_t contig_count,
                               const size_t * input_blocklength_array,
                               const void *input_displacement_array,
                               size_t * output_blocklength_array,
                               size_t * out_disp_array, int dispinbytes, size_t old_extent);

/*@
   MPII_Dataloop_create_indexed

   Arguments:
+  int icount
.  size_t *iblocklength_array
.  void *displacement_array (either ints or MPI_Aints)
.  int dispinbytes
.  MPI_Datatype oldtype
.  MPII_Dataloop **dlp_p

.N Errors
.N Returns 0 on success, -1 on error.
@*/

int MPII_Dataloop_create_indexed(size_t icount,
                                 const size_t * blocklength_array,
                                 const void *displacement_array,
                                 int dispinbytes, MPI_Datatype oldtype, MPII_Dataloop ** dlp_p)
{
    int err, is_builtin;
    size_t i;
    size_t new_loop_sz, blksz;
    size_t first;

    size_t old_type_count = 0, contig_count, count;
    size_t old_extent;
    MPII_Dataloop *new_dlp;

    count = (MPI_Aint) icount;  /* avoid subsequent casting */


    /* if count is zero, handle with contig code, call it an int */
    if (count == 0) {
        err = MPII_Dataloop_create_contiguous(0, MPI_INT, dlp_p);
        return err;
    }

    /* Skip any initial zero-length blocks */
    for (first = 0; first < count; first++)
        if ((size_t) blocklength_array[first])
            break;


    is_builtin = (MPII_DATALOOP_HANDLE_HASLOOP(oldtype)) ? 0 : 1;

    if (is_builtin) {
        MPIR_Datatype_get_extent_macro(oldtype, old_extent);
    } else {
        MPIR_Datatype_get_extent_macro(oldtype, old_extent);
    }

    for (i = first; i < count; i++) {
        old_type_count += (size_t) blocklength_array[i];
    }

    contig_count = MPII_Datatype_indexed_count_contig(count,
                                                      blocklength_array,
                                                      displacement_array, dispinbytes, old_extent);

    /* if contig_count is zero (no data), handle with contig code */
    if (contig_count == 0) {
        err = MPII_Dataloop_create_contiguous(0, MPI_INT, dlp_p);
        return err;
    }

    /* optimization:
     *
     * if contig_count == 1 and block starts at displacement 0,
     * store it as a contiguous rather than an indexed dataloop.
     */
    if ((contig_count == 1) &&
        ((!dispinbytes && ((int *) displacement_array)[first] == 0) ||
         (dispinbytes && ((size_t *) displacement_array)[first] == 0))) {
        err = MPII_Dataloop_create_contiguous(old_type_count, oldtype, dlp_p);
        return err;
    }

    /* optimization:
     *
     * if contig_count == 1 (and displacement != 0), store this as
     * a single element blockindexed rather than a lot of individual
     * blocks.
     */
    if (contig_count == 1) {
        const void *disp_arr_tmp;       /* no ternary assignment to avoid clang warnings */
        if (dispinbytes)
            disp_arr_tmp = &(((const size_t *) displacement_array)[first]);
        else
            disp_arr_tmp = &(((const int *) displacement_array)[first]);
        err = MPII_Dataloop_create_blockindexed(1,
                                                old_type_count,
                                                disp_arr_tmp, dispinbytes, oldtype, dlp_p);

        return err;
    }

    /* optimization:
     *
     * if block length is the same for all blocks, store it as a
     * blockindexed rather than an indexed dataloop.
     */
    blksz = blocklength_array[first];
    for (i = first + 1; i < count; i++) {
        if (blocklength_array[i] != blksz) {
            blksz--;
            break;
        }
    }
    if (blksz == blocklength_array[first]) {
        const void *disp_arr_tmp;       /* no ternary assignment to avoid clang warnings */
        if (dispinbytes)
            disp_arr_tmp = &(((const size_t *) displacement_array)[first]);
        else
            disp_arr_tmp = &(((const int *) displacement_array)[first]);
        err = MPII_Dataloop_create_blockindexed(icount - first,
                                                blksz, disp_arr_tmp, dispinbytes, oldtype, dlp_p);

        return err;
    }

    /* note: blockindexed looks for the vector optimization */

    /* TODO: optimization:
     *
     * if an indexed of a contig, absorb the contig into the blocklen array
     * and keep the same overall depth
     */

    /* otherwise storing as an indexed dataloop */

    if (is_builtin) {
        MPII_Dataloop_alloc(MPII_DATALOOP_KIND_INDEXED, count, &new_dlp);
        /* --BEGIN ERROR HANDLING-- */
        if (!new_dlp)
            return -1;
        /* --END ERROR HANDLING-- */
        new_loop_sz = new_dlp->dloop_sz;

        new_dlp->kind = MPII_DATALOOP_KIND_INDEXED | MPII_DATALOOP_FINAL_MASK;

        new_dlp->el_size = old_extent;
        new_dlp->el_extent = old_extent;
        new_dlp->el_type = oldtype;
    } else {
        MPII_Dataloop *old_loop_ptr = NULL;

        MPII_DATALOOP_GET_LOOPPTR(oldtype, old_loop_ptr);

        MPII_Dataloop_alloc_and_copy(MPII_DATALOOP_KIND_INDEXED,
                                     contig_count, old_loop_ptr, &new_dlp);
        /* --BEGIN ERROR HANDLING-- */
        if (!new_dlp)
            return -1;
        /* --END ERROR HANDLING-- */
        new_loop_sz = new_dlp->dloop_sz;

        new_dlp->kind = MPII_DATALOOP_KIND_INDEXED;

        MPIR_Datatype_get_size_macro(oldtype, new_dlp->el_size);
        MPIR_Datatype_get_extent_macro(oldtype, new_dlp->el_extent);
        MPIR_Datatype_get_basic_type(oldtype, new_dlp->el_type);
    }

    new_dlp->loop_params.i_t.count = contig_count;
    new_dlp->loop_params.i_t.total_blocks = old_type_count;

    /* copy in blocklength and displacement parameters (in that order)
     *
     * regardless of dispinbytes, we store displacements in bytes in loop.
     */
    indexed_array_copy(count,
                       contig_count,
                       blocklength_array,
                       displacement_array,
                       new_dlp->loop_params.i_t.blocksize_array,
                       new_dlp->loop_params.i_t.offset_array, dispinbytes, old_extent);

    *dlp_p = new_dlp;
    new_dlp->dloop_sz = new_loop_sz;

    return MPI_SUCCESS;
}

/* indexed_array_copy()
 *
 * Copies arrays into place, combining adjacent contiguous regions and
 * dropping zero-length regions.
 *
 * Extent passed in is for the original type.
 *
 * Output displacements are always output in bytes, while block
 * lengths are always output in terms of the base type.
 */
static void indexed_array_copy(size_t count,
                               size_t contig_count,
                               const size_t * in_blklen_array,
                               const void *in_disp_array,
                               size_t * out_blklen_array,
                               size_t * out_disp_array, int dispinbytes, size_t old_extent)
{
    size_t i, first, cur_idx = 0;

    /* Skip any initial zero-length blocks */
    for (first = 0; first < count; ++first)
        if ((size_t) in_blklen_array[first])
            break;

    out_blklen_array[0] = (size_t) in_blklen_array[first];

    if (!dispinbytes) {
        out_disp_array[0] = (size_t)
            ((int *) in_disp_array)[first] * old_extent;

        for (i = first + 1; i < count; ++i) {
            if (in_blklen_array[i] == 0) {
                continue;
            } else if (out_disp_array[cur_idx] +
                       ((size_t) out_blklen_array[cur_idx]) * old_extent ==
                       ((size_t) ((int *) in_disp_array)[i]) * old_extent) {
                /* adjacent to current block; add to block */
                out_blklen_array[cur_idx] += (size_t) in_blklen_array[i];
            } else {
                cur_idx++;
                MPIR_Assert(cur_idx < contig_count);
                out_disp_array[cur_idx] = ((size_t) ((int *) in_disp_array)[i]) * old_extent;
                out_blklen_array[cur_idx] = in_blklen_array[i];
            }
        }
    } else {    /* input displacements already in bytes */

        out_disp_array[0] = (size_t) ((size_t *) in_disp_array)[first];

        for (i = first + 1; i < count; ++i) {
            if (in_blklen_array[i] == 0) {
                continue;
            } else if (out_disp_array[cur_idx] +
                       ((size_t) out_blklen_array[cur_idx]) * old_extent ==
                       ((size_t) ((size_t *) in_disp_array)[i])) {
                /* adjacent to current block; add to block */
                out_blklen_array[cur_idx] += in_blklen_array[i];
            } else {
                cur_idx++;
                MPIR_Assert(cur_idx < contig_count);
                out_disp_array[cur_idx] = (size_t) ((size_t *) in_disp_array)[i];
                out_blklen_array[cur_idx] = (size_t) in_blklen_array[i];
            }
        }
    }

    MPIR_Assert(cur_idx == contig_count - 1);
    return;
}
