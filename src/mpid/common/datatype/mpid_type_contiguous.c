/* -*- Mode: C; c-basic-offset:4 ; -*- */

/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include <mpiimpl.h>
#include <mpid_dataloop.h>
#include <stdlib.h>

/* #define MPID_TYPE_ALLOC_DEBUG */

/*@
  MPID_Type_contiguous - create a contiguous datatype

  Input Parameters:
+ count - number of elements in the contiguous block
- oldtype - type (using handle) of datatype on which vector is based

  Output Parameters:
. newtype - handle of new contiguous datatype

  Return Value:
  MPI_SUCCESS on success, MPI error code on failure.
@*/
int MPID_Type_contiguous(int count,
			 MPI_Datatype oldtype,
			 MPI_Datatype *newtype)
{
    int mpi_errno = MPI_SUCCESS;
    int is_builtin;
    int el_sz;
    MPI_Datatype el_type;
    MPID_Datatype *new_dtp;

    if (count == 0) return MPID_Type_zerolen(newtype);

    /* allocate new datatype object and handle */
    new_dtp = (MPID_Datatype *) MPIU_Handle_obj_alloc(&MPID_Datatype_mem);
    /* --BEGIN ERROR HANDLING-- */
    if (!new_dtp)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE,
					 "MPID_Type_contiguous",
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

    is_builtin = (HANDLE_GET_KIND(oldtype) == HANDLE_KIND_BUILTIN);

    if (is_builtin)
    {
	el_sz   = MPID_Datatype_get_basic_size(oldtype);
	el_type = oldtype;

	new_dtp->size          = count * el_sz;
	new_dtp->has_sticky_ub = 0;
	new_dtp->has_sticky_lb = 0;
	new_dtp->true_lb       = 0;
	new_dtp->lb            = 0;
	new_dtp->true_ub       = count * el_sz;
	new_dtp->ub            = new_dtp->true_ub;
	new_dtp->extent        = new_dtp->ub - new_dtp->lb;

	new_dtp->alignsize     = el_sz;
	new_dtp->n_elements    = count;
	new_dtp->element_size  = el_sz;
        new_dtp->eltype        = el_type;
	new_dtp->is_contig     = 1;
        new_dtp->max_contig_blocks = 1;

    }
    else
    {
	/* user-defined base type (oldtype) */
	MPID_Datatype *old_dtp;

	MPID_Datatype_get_ptr(oldtype, old_dtp);
	el_sz   = old_dtp->element_size;
	el_type = old_dtp->eltype;

	new_dtp->size           = count * old_dtp->size;
	new_dtp->has_sticky_ub  = old_dtp->has_sticky_ub;
	new_dtp->has_sticky_lb  = old_dtp->has_sticky_lb;

	MPID_DATATYPE_CONTIG_LB_UB((MPI_Aint) count,
				   old_dtp->lb,
				   old_dtp->ub,
				   old_dtp->extent,
				   new_dtp->lb,
				   new_dtp->ub);

	/* easiest to calc true lb/ub relative to lb/ub; doesn't matter
	 * if there are sticky lb/ubs or not when doing this.
	 */
	new_dtp->true_lb = new_dtp->lb + (old_dtp->true_lb - old_dtp->lb);
	new_dtp->true_ub = new_dtp->ub + (old_dtp->true_ub - old_dtp->ub);
	new_dtp->extent  = new_dtp->ub - new_dtp->lb;

	new_dtp->alignsize    = old_dtp->alignsize;
	new_dtp->n_elements   = count * old_dtp->n_elements;
	new_dtp->element_size = old_dtp->element_size;
        new_dtp->eltype       = el_type;

	new_dtp->is_contig    = old_dtp->is_contig;
        if(old_dtp->is_contig)
            new_dtp->max_contig_blocks = 1;
        else
            new_dtp->max_contig_blocks = count * old_dtp->max_contig_blocks;
    }

    *newtype = new_dtp->handle;

    MPIU_DBG_MSG_P(DATATYPE,VERBOSE,"contig type %x created.",
		   new_dtp->handle);

    return mpi_errno;
}

