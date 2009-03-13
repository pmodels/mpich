/* -*- Mode: C; c-basic-offset:4 ; -*- */

/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include <mpiimpl.h>
#include <mpid_dataloop.h>
#include <stdlib.h>

#undef MPID_TYPE_ALLOC_DEBUG

/*@
  MPID_Type_blockindexed - create a block indexed datatype

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

int MPID_Type_blockindexed(int count,
			   int blocklength,
			   void *displacement_array,
			   int dispinbytes,
			   MPI_Datatype oldtype,
			   MPI_Datatype *newtype)
{
    int mpi_errno = MPI_SUCCESS, i;
    int is_builtin, contig_count, old_is_contig;
    MPI_Aint el_sz;
    MPI_Datatype el_type;
    MPI_Aint old_lb, old_ub, old_extent, old_true_lb, old_true_ub;
    MPI_Aint min_lb = 0, max_ub = 0, eff_disp;

    MPID_Datatype *new_dtp;

    if (count == 0) return MPID_Type_zerolen(newtype);

    /* allocate new datatype object and handle */
    new_dtp = (MPID_Datatype *) MPIU_Handle_obj_alloc(&MPID_Datatype_mem);
    /* --BEGIN ERROR HANDLING-- */
    if (!new_dtp)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE,
					 "MPID_Type_vector", __LINE__,
					 MPI_ERR_OTHER, "**nomem", 0);
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
	el_sz   = (MPI_Aint) MPID_Datatype_get_basic_size(oldtype);
	el_type = oldtype;

	old_lb        = 0;
	old_true_lb   = 0;
	old_ub        = el_sz;
	old_true_ub   = el_sz;
	old_extent    = el_sz;
	old_is_contig = 1;

	new_dtp->size          = (MPI_Aint) count *
	                         (MPI_Aint) blocklength * el_sz;
	new_dtp->has_sticky_lb = 0;
	new_dtp->has_sticky_ub = 0;

	new_dtp->alignsize    = el_sz; /* ??? */
	new_dtp->n_elements   = count * blocklength;
	new_dtp->element_size = el_sz;
	new_dtp->eltype       = el_type;

	new_dtp->max_contig_blocks = count;
    }
    else
    {
	/* user-defined base type (oldtype) */
	MPID_Datatype *old_dtp;

	MPID_Datatype_get_ptr(oldtype, old_dtp);
	el_sz   = old_dtp->element_size;
	el_type = old_dtp->eltype;

	old_lb        = old_dtp->lb;
	old_true_lb   = old_dtp->true_lb;
	old_ub        = old_dtp->ub;
	old_true_ub   = old_dtp->true_ub;
	old_extent    = old_dtp->extent;
	old_is_contig = old_dtp->is_contig;

	new_dtp->size           = (MPI_Aint) count *
	                          (MPI_Aint) blocklength *
	                          (MPI_Aint) old_dtp->size;
	new_dtp->has_sticky_lb  = old_dtp->has_sticky_lb;
	new_dtp->has_sticky_ub  = old_dtp->has_sticky_ub;

	new_dtp->alignsize    = old_dtp->alignsize;
	new_dtp->n_elements   = count * blocklength * old_dtp->n_elements;
	new_dtp->element_size = el_sz;
	new_dtp->eltype       = el_type;

	new_dtp->max_contig_blocks = old_dtp->max_contig_blocks * count * blocklength;
    }

    /* priming for loop */
    eff_disp = (dispinbytes) ? ((MPI_Aint *) displacement_array)[0] :
	(((MPI_Aint) ((int *) displacement_array)[0]) * old_extent);
    MPID_DATATYPE_BLOCK_LB_UB((MPI_Aint) blocklength,
			      eff_disp,
			      old_lb,
			      old_ub,
			      old_extent,
			      min_lb,
			      max_ub);

    /* determine new min lb and max ub */
    for (i=1; i < count; i++)
    {
	MPI_Aint tmp_lb, tmp_ub;

	eff_disp = (dispinbytes) ? ((MPI_Aint *) displacement_array)[i] :
	    (((MPI_Aint) ((int *) displacement_array)[i]) * old_extent);
	MPID_DATATYPE_BLOCK_LB_UB((MPI_Aint) blocklength,
				  eff_disp,
				  old_lb,
				  old_ub,
				  old_extent,
				  tmp_lb,
				  tmp_ub);

	if (tmp_lb < min_lb) min_lb = tmp_lb;
	if (tmp_ub > max_ub) max_ub = tmp_ub;
    }

    new_dtp->lb      = min_lb;
    new_dtp->ub      = max_ub;
    new_dtp->true_lb = min_lb + (old_true_lb - old_lb);
    new_dtp->true_ub = max_ub + (old_true_ub - old_ub);
    new_dtp->extent  = max_ub - min_lb;

    /* new type is contig for N types if it is all one big block,
     * its size and extent are the same, and the old type was also
     * contiguous.
     */
    new_dtp->is_contig = 0;
    if (old_is_contig)
    {
	contig_count = MPID_Type_blockindexed_count_contig(count,
							   blocklength,
							   displacement_array,
							   dispinbytes,
							   old_extent);
        new_dtp->max_contig_blocks = contig_count;
        if( (contig_count == 1) &&
                ((MPI_Aint) new_dtp->size == new_dtp->extent) )
        {
            new_dtp->is_contig = 1; 
        }
    }

    *newtype = new_dtp->handle;
    return mpi_errno;
}
