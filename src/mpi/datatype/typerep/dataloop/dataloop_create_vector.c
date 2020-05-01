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
    int err, is_builtin;

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

        err = MPIR_Dataloop_create_contiguous(0, MPI_INT, (void **) dlp_p);
        return err;
    }

    /* optimization:
     *
     * if count == 1, store as a contiguous rather than a vector dataloop.
     */
    if (count == 1) {
        err = MPIR_Dataloop_create_contiguous(iblocklength, oldtype, (void **) dlp_p);
        return err;
    }

    is_builtin = (MPII_DATALOOP_HANDLE_HASLOOP(oldtype)) ? 0 : 1;

    if (is_builtin) {
        MPI_Aint basic_sz = 0;

        MPII_Dataloop_alloc(MPII_DATALOOP_KIND_VECTOR, count, &new_dlp);
        /* --BEGIN ERROR HANDLING-- */
        if (!new_dlp)
            return -1;
        /* --END ERROR HANDLING-- */

        MPIR_Datatype_get_size_macro(oldtype, basic_sz);
        new_dlp->kind = MPII_DATALOOP_KIND_VECTOR | MPII_DATALOOP_FINAL_MASK;

        new_dlp->el_size = basic_sz;
        new_dlp->el_extent = new_dlp->el_size;
        new_dlp->el_type = oldtype;
    } else {    /* user-defined base type (oldtype) */

        MPII_Dataloop *old_loop_ptr;

        MPIR_DATALOOP_GET_LOOPPTR(oldtype, old_loop_ptr);

        MPII_Dataloop_alloc_and_copy(MPII_DATALOOP_KIND_VECTOR, count, old_loop_ptr, &new_dlp);
        /* --BEGIN ERROR HANDLING-- */
        if (!new_dlp)
            return -1;
        /* --END ERROR HANDLING-- */

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

    *dlp_p = new_dlp;

    return 0;
}
