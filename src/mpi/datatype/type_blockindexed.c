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
    int mpi_errno = MPI_SUCCESS;

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
