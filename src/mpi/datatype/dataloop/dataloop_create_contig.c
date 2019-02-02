/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */

/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"
#include "dataloop.h"

/*@
   Dataloop_contiguous - create the dataloop representation for a
   contiguous datatype

   Input Parameters:
+  int icount,
.  MPI_Datatype oldtype

   Output Parameters:
+  MPIR_Dataloop **dlp_p,
.  MPI_Aint *dlsz_p,
-  int *dldepth_p,


.N Errors
.N Returns 0 on success, -1 on failure.
@*/
int MPII_Dataloop_create_contiguous(MPI_Aint icount,
                                    MPI_Datatype oldtype, MPIR_Dataloop ** dlp_p, MPI_Aint * dlsz_p)
{
    MPI_Aint count;
    int is_builtin, apply_contig_coalescing = 0;
    MPI_Aint new_loop_sz;

    MPIR_Dataloop *new_dlp;

    count = icount;

    is_builtin = (MPII_DATALOOP_HANDLE_HASLOOP(oldtype)) ? 0 : 1;

    if (!is_builtin) {
        MPI_Aint old_size = 0, old_extent = 0;
        MPIR_Dataloop *old_loop_ptr;

        MPII_DATALOOP_GET_LOOPPTR(oldtype, old_loop_ptr);
        MPIR_Datatype_get_size_macro(oldtype, old_size);
        MPIR_Datatype_get_extent_macro(oldtype, old_extent);

        /* if we have a simple combination of contigs, coalesce */
        if (((old_loop_ptr->kind & MPII_DATALOOP_KIND_MASK) == MPII_DATALOOP_KIND_CONTIG)
            && (old_size == old_extent)) {
            /* will just copy contig and multiply count */
            apply_contig_coalescing = 1;
        }
    }

    if (is_builtin) {
        MPI_Aint basic_sz = 0;

        MPII_Dataloop_alloc(MPII_DATALOOP_KIND_CONTIG, count, &new_dlp, &new_loop_sz);
        /* --BEGIN ERROR HANDLING-- */
        if (!new_dlp)
            return -1;
        /* --END ERROR HANDLING-- */

        MPIR_Datatype_get_size_macro(oldtype, basic_sz);
        new_dlp->kind = MPII_DATALOOP_KIND_CONTIG | MPII_DATALOOP_FINAL_MASK;

        new_dlp->el_size = basic_sz;
        new_dlp->el_extent = new_dlp->el_size;
        new_dlp->el_type = oldtype;

        new_dlp->loop_params.c_t.count = count;
    } else {
        /* user-defined base type (oldtype) */
        MPIR_Dataloop *old_loop_ptr;
        MPI_Aint old_loop_sz = 0;

        MPII_DATALOOP_GET_LOOPPTR(oldtype, old_loop_ptr);
        MPII_DATALOOP_GET_LOOPSIZE(oldtype, old_loop_sz);

        if (apply_contig_coalescing) {
            /* make a copy of the old loop and multiply the count */
            MPIR_Dataloop_dup(old_loop_ptr, old_loop_sz, &new_dlp);
            /* --BEGIN ERROR HANDLING-- */
            if (!new_dlp)
                return -1;
            /* --END ERROR HANDLING-- */

            new_dlp->loop_params.c_t.count *= count;

            new_loop_sz = old_loop_sz;
        } else {
            /* allocate space for new loop including copy of old */
            MPII_Dataloop_alloc_and_copy(MPII_DATALOOP_KIND_CONTIG,
                                         count, old_loop_ptr, old_loop_sz, &new_dlp, &new_loop_sz);
            /* --BEGIN ERROR HANDLING-- */
            if (!new_dlp)
                return -1;
            /* --END ERROR HANDLING-- */

            new_dlp->kind = MPII_DATALOOP_KIND_CONTIG;
            MPIR_Datatype_get_size_macro(oldtype, new_dlp->el_size);
            MPIR_Datatype_get_extent_macro(oldtype, new_dlp->el_extent);
            MPIR_Datatype_get_basic_type(oldtype, new_dlp->el_type);

            new_dlp->loop_params.c_t.count = count;
        }
    }

    *dlp_p = new_dlp;
    *dlsz_p = new_loop_sz;

    return 0;
}
