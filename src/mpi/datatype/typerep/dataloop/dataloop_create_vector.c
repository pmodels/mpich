/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"
#include "dataloop_internal.h"

/*@
   Dataloop_create_vector

   Arguments:
+  int icount
.  int iblocklength
.  MPI_Aint astride
.  int strideinbytes
.  MPI_Datatype oldtype
.  MPII_Dataloop **dlp_p

   Returns 0 on success, -1 on failure.

@*/
int MPIR_Dataloop_create_vector(MPI_Aint icount,
                                MPI_Aint iblocklength,
                                MPI_Aint astride,
                                int strideinbytes, MPI_Datatype oldtype, void **dlp_p)
{
    int mpi_errno = MPI_SUCCESS;

    int is_builtin;
    int old_is_contig;
    MPI_Aint old_num_contig;
    MPI_Aint old_extent;
    MPII_Dataloop *old_loop_ptr;

    MPI_Aint count, blocklength;
    MPI_Aint stride;
    MPII_Dataloop *new_dlp;

    count = (MPI_Aint) icount;  /* avoid subsequent casting */
    blocklength = (MPI_Aint) iblocklength;
    stride = (MPI_Aint) astride;

    /* if count or blocklength are zero, handle with contig code,
     * call it a int
     */
    if (count == 0 || blocklength == 0) {

        mpi_errno = MPIR_Dataloop_create_contiguous(0, MPI_INT, (void **) dlp_p);
        goto fn_exit;
    }

    /* optimization:
     *
     * if count == 1, store as a contiguous rather than a vector dataloop.
     */
    if (count == 1) {
        mpi_errno = MPIR_Dataloop_create_contiguous(iblocklength, oldtype, (void **) dlp_p);
        goto fn_exit;
    }

    MPIR_DATALOOP_GET_LOOPPTR(oldtype, old_loop_ptr);
    is_builtin = (old_loop_ptr == NULL);

    if (is_builtin) {
        MPIR_Datatype_get_size_macro(oldtype, old_extent);
        old_is_contig = 1;
        old_num_contig = 1;
    } else {    /* user-defined base type (oldtype) */

        MPIR_Datatype_get_extent_macro(oldtype, old_extent);
        old_is_contig = old_loop_ptr->is_contig;
        old_num_contig = old_loop_ptr->num_contig;
    }

    /* optimization
     *
     * All blocks join into a single contig block.
     */
    if (old_is_contig) {
        if ((strideinbytes && stride == old_extent * blocklength) || (stride == blocklength)) {
            mpi_errno =
                MPIR_Dataloop_create_contiguous(count * blocklength, oldtype, (void **) dlp_p);
            goto fn_exit;
        }
    }

    if (is_builtin) {
        MPII_Dataloop_alloc(MPII_DATALOOP_KIND_VECTOR, count, &new_dlp);
        MPIR_ERR_CHKANDJUMP(!new_dlp, mpi_errno, MPI_ERR_OTHER, "**nomem");

        new_dlp->kind = MPII_DATALOOP_KIND_VECTOR | MPII_DATALOOP_FINAL_MASK;

        new_dlp->el_size = old_extent;
        new_dlp->el_extent = old_extent;
        new_dlp->el_type = oldtype;
    } else {    /* user-defined base type (oldtype) */
        MPII_Dataloop_alloc_and_copy(MPII_DATALOOP_KIND_VECTOR, count, old_loop_ptr, &new_dlp);
        MPIR_ERR_CHKANDJUMP(!new_dlp, mpi_errno, MPI_ERR_OTHER, "**nomem");

        new_dlp->kind = MPII_DATALOOP_KIND_VECTOR;

        MPIR_Datatype_get_size_macro(oldtype, new_dlp->el_size);
        MPIR_Datatype_get_extent_macro(oldtype, new_dlp->el_extent);
        MPIR_Datatype_get_basic_type(oldtype, new_dlp->el_type);
    }

    /* vector-specific members
     *
     * stride stored in dataloop is always in bytes for local rep of type
     */
    new_dlp->loop_params.v_t.count = count;
    new_dlp->loop_params.v_t.blocksize = blocklength;
    new_dlp->loop_params.v_t.stride = (strideinbytes) ? stride : stride * new_dlp->el_extent;

    if (old_is_contig) {
        new_dlp->is_contig = 0;
        new_dlp->num_contig = count;
    } else {
        new_dlp->is_contig = 0;
        new_dlp->num_contig = count * blocklength * old_num_contig;
    }

    *dlp_p = new_dlp;

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
