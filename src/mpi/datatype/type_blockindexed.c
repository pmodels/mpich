/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include <mpiimpl.h>
#include <stdlib.h>
#include "datatype.h"

#undef MPID_TYPE_ALLOC_DEBUG

/*@
  MPIR_Type_blockindexed - create a block indexed datatype

Input Parameters:
+ count - number of blocks in type
. blocklength - number of elements in each block
. displacement_array - offsets of blocks from start of type (see next
  parameter for units)
. dispinbytes - if nonzero, then displacements are in bytes, otherwise
  they in terms of extent of oldtype
- oldtype - type (using handle) of datatype on which new type is based

Output Parameters:
. newtype - handle of new block indexed datatype

  Return Value:
  MPI_SUCCESS on success, MPI error on failure.
@*/
int MPIR_Type_blockindexed(int count,
                           int blocklength,
                           const void *displacement_array,
                           int dispinbytes, MPI_Datatype oldtype, MPI_Datatype * newtype)
{
    int mpi_errno = MPI_SUCCESS, i;
    int old_is_contig;
    MPI_Aint contig_count;
    MPI_Aint old_lb, old_ub, old_extent, old_true_lb, old_true_ub;
    MPI_Aint min_lb = 0, max_ub = 0, eff_disp;

    MPIR_Datatype *new_dtp;

    if (count == 0)
        return MPII_Type_zerolen(newtype);

    /* allocate new datatype object and handle */
    new_dtp = (MPIR_Datatype *) MPIR_Handle_obj_alloc(&MPIR_Datatype_mem);
    /* --BEGIN ERROR HANDLING-- */
    if (!new_dtp) {
        mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE,
                                         "MPIR_Type_vector", __LINE__, MPI_ERR_OTHER, "**nomem", 0);
        return mpi_errno;
    }
    /* --END ERROR HANDLING-- */

    /* handle is filled in by MPIR_Handle_obj_alloc() */
    MPIR_Object_set_ref(new_dtp, 1);
    new_dtp->is_committed = 0;
    new_dtp->attributes = NULL;
    new_dtp->name[0] = 0;
    new_dtp->contents = NULL;
    new_dtp->flattened = NULL;

    new_dtp->typerep.handle = NULL;

    if (HANDLE_IS_BUILTIN(oldtype)) {
        MPI_Aint el_sz = (MPI_Aint) MPIR_Datatype_get_basic_size(oldtype);

        old_lb = 0;
        old_true_lb = 0;
        old_ub = el_sz;
        old_true_ub = el_sz;
        old_extent = el_sz;
        old_is_contig = 1;

        new_dtp->size = (MPI_Aint) count *(MPI_Aint) blocklength *el_sz;

        new_dtp->alignsize = el_sz;     /* ??? */
        new_dtp->n_builtin_elements = count * blocklength;
        new_dtp->builtin_element_size = el_sz;
        new_dtp->basic_type = oldtype;
    } else {
        /* user-defined base type (oldtype) */
        MPIR_Datatype *old_dtp;

        MPIR_Datatype_get_ptr(oldtype, old_dtp);

        old_lb = old_dtp->lb;
        old_true_lb = old_dtp->true_lb;
        old_ub = old_dtp->ub;
        old_true_ub = old_dtp->true_ub;
        old_extent = old_dtp->extent;
        MPIR_Datatype_is_contig(oldtype, &old_is_contig);

        new_dtp->size = (MPI_Aint) count *(MPI_Aint) blocklength *(MPI_Aint) old_dtp->size;

        new_dtp->alignsize = old_dtp->alignsize;
        new_dtp->n_builtin_elements = count * blocklength * old_dtp->n_builtin_elements;
        new_dtp->builtin_element_size = old_dtp->builtin_element_size;
        new_dtp->basic_type = old_dtp->basic_type;
    }

    /* priming for loop */
    eff_disp = (dispinbytes) ? ((MPI_Aint *) displacement_array)[0] :
        (((MPI_Aint) ((int *) displacement_array)[0]) * old_extent);
    MPII_DATATYPE_BLOCK_LB_UB((MPI_Aint) blocklength,
                              eff_disp, old_lb, old_ub, old_extent, min_lb, max_ub);

    /* determine new min lb and max ub */
    for (i = 1; i < count; i++) {
        MPI_Aint tmp_lb, tmp_ub;

        eff_disp = (dispinbytes) ? ((MPI_Aint *) displacement_array)[i] :
            (((MPI_Aint) ((int *) displacement_array)[i]) * old_extent);
        MPII_DATATYPE_BLOCK_LB_UB((MPI_Aint) blocklength,
                                  eff_disp, old_lb, old_ub, old_extent, tmp_lb, tmp_ub);

        if (tmp_lb < min_lb)
            min_lb = tmp_lb;
        if (tmp_ub > max_ub)
            max_ub = tmp_ub;
    }

    new_dtp->lb = min_lb;
    new_dtp->ub = max_ub;
    new_dtp->true_lb = min_lb + (old_true_lb - old_lb);
    new_dtp->true_ub = max_ub + (old_true_ub - old_ub);
    new_dtp->extent = max_ub - min_lb;

    /* new type is contig for N types if it is all one big block,
     * its size and extent are the same, and the old type was also
     * contiguous.
     */
    new_dtp->is_contig = 0;
    if (old_is_contig) {
        contig_count = MPII_Datatype_blockindexed_count_contig(count,
                                                               blocklength,
                                                               displacement_array,
                                                               dispinbytes, old_extent);
        if ((contig_count == 1) && ((MPI_Aint) new_dtp->size == new_dtp->extent)) {
            new_dtp->is_contig = 1;
        }
    }

    if (dispinbytes) {
        mpi_errno = MPIR_Typerep_create_hindexed_block(count, blocklength, displacement_array,
                                                       oldtype, new_dtp);
        MPIR_ERR_CHECK(mpi_errno);
    } else {
        mpi_errno = MPIR_Typerep_create_indexed_block(count, blocklength, displacement_array,
                                                      oldtype, new_dtp);
        MPIR_ERR_CHECK(mpi_errno);
    }

    *newtype = new_dtp->handle;

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
