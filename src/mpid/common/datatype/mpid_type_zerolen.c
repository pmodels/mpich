/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */

/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include <mpiimpl.h>
#include <mpid_dataloop.h>
#include <stdlib.h>

/* #define MPID_TYPE_ALLOC_DEBUG */

/*@
  MPID_Type_zerolen - create an empty datatype
 
Input Parameters:
. none

Output Parameters:
. newtype - handle of new contiguous datatype

  Return Value:
  MPI_SUCCESS on success, MPI error code on failure.
@*/

int MPID_Type_zerolen(MPI_Datatype *newtype)
{
    int mpi_errno;
    MPID_Datatype *new_dtp;

    /* allocate new datatype object and handle */
    new_dtp = (MPID_Datatype *) MPIU_Handle_obj_alloc(&MPID_Datatype_mem);
    /* --BEGIN ERROR HANDLING-- */
    if (!new_dtp)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE,
					 "MPID_Type_zerolen",
					 __LINE__, MPI_ERR_OTHER,
					 "**nomem", 0);
	return mpi_errno;
    }
    /* --END ERROR HANDLING-- */

    /* handle is filled in by MPIU_Handle_obj_alloc() */
    MPIU_Object_set_ref(new_dtp, 1);
    new_dtp->is_permanent = 0;
    new_dtp->is_committed = 0;
    new_dtp->attributes   = NULL;
    new_dtp->cache_id     = 0;
    new_dtp->name[0]      = 0;
    new_dtp->contents     = NULL;

    new_dtp->dataloop       = NULL;
    new_dtp->dataloop_size  = -1;
    new_dtp->dataloop_depth = -1;
    new_dtp->hetero_dloop       = NULL;
    new_dtp->hetero_dloop_size  = -1;
    new_dtp->hetero_dloop_depth = -1;
    
    new_dtp->size          = 0;
    new_dtp->has_sticky_ub = 0;
    new_dtp->has_sticky_lb = 0;
    new_dtp->lb            = 0;
    new_dtp->ub            = 0;
    new_dtp->true_lb       = 0;
    new_dtp->true_ub       = 0;
    new_dtp->extent        = 0;
    
    new_dtp->alignsize     = 0;
    new_dtp->builtin_element_size  = 0;
    new_dtp->basic_type        = 0;
    new_dtp->n_builtin_elements    = 0;
    new_dtp->is_contig     = 1;

    *newtype = new_dtp->handle;
    return MPI_SUCCESS;
}
