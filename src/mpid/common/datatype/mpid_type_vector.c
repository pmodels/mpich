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
  MPID_Type_vector - create a vector datatype

Input Parameters:
+ count - number of blocks in vector
. blocklength - number of elements in each block
. stride - distance from beginning of one block to the next (see next
  parameter for units)
. strideinbytes - if nonzero, then stride is in bytes, otherwise stride
  is in terms of extent of oldtype
- oldtype - type (using handle) of datatype on which vector is based

Output Parameters:
. newtype - handle of new vector datatype

  Return Value:
  0 on success, MPI error code on failure.
@*/
int MPID_Type_vector(int count,
		     int blocklength,
		     MPI_Aint stride,
		     int strideinbytes,
		     MPI_Datatype oldtype,
		     MPI_Datatype *newtype)
{
    int mpi_errno = MPI_SUCCESS;
    int is_builtin, old_is_contig;
    MPI_Aint el_sz, old_sz;
    MPI_Datatype el_type;
    MPI_Aint old_lb, old_ub, old_extent, old_true_lb, old_true_ub, eff_stride;

    MPID_Datatype *new_dtp;

    if (count == 0) return MPID_Type_zerolen(newtype);

    /* allocate new datatype object and handle */
    new_dtp = (MPID_Datatype *) MPIU_Handle_obj_alloc(&MPID_Datatype_mem);
    if (!new_dtp) {
	/* --BEGIN ERROR HANDLING-- */
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE,
					 "MPID_Type_vector", __LINE__,
					 MPI_ERR_OTHER, "**nomem", 0);
	return mpi_errno;
	/* --END ERROR HANDLING-- */
    }

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

    if (is_builtin) {
	el_sz   = (MPI_Aint) MPID_Datatype_get_basic_size(oldtype);
	el_type = oldtype;

	old_lb        = 0;
	old_true_lb   = 0;
	old_ub        = el_sz;
	old_true_ub   = el_sz;
	old_sz        = el_sz;
	old_extent    = el_sz;
	old_is_contig = 1;

	new_dtp->size           = (MPI_Aint) count *
	                          (MPI_Aint) blocklength * el_sz;
	new_dtp->has_sticky_lb  = 0;
	new_dtp->has_sticky_ub  = 0;

	new_dtp->alignsize    = el_sz; /* ??? */
	new_dtp->n_builtin_elements   = count * blocklength;
	new_dtp->builtin_element_size = el_sz;
	new_dtp->basic_type       = el_type;

	new_dtp->max_contig_blocks = count;

	eff_stride = (strideinbytes) ? stride : (stride * el_sz);
    }
    else /* user-defined base type (oldtype) */ {
	MPID_Datatype *old_dtp;

	MPID_Datatype_get_ptr(oldtype, old_dtp);
	el_sz   = old_dtp->builtin_element_size;
	el_type = old_dtp->basic_type;

	old_lb        = old_dtp->lb;
	old_true_lb   = old_dtp->true_lb;
	old_ub        = old_dtp->ub;
	old_true_ub   = old_dtp->true_ub;
	old_sz        = old_dtp->size;
	old_extent    = old_dtp->extent;
	old_is_contig = old_dtp->is_contig;

	new_dtp->size           = count * blocklength * old_dtp->size;
	new_dtp->has_sticky_lb  = old_dtp->has_sticky_lb;
	new_dtp->has_sticky_ub  = old_dtp->has_sticky_ub;

	new_dtp->alignsize    = old_dtp->alignsize;
	new_dtp->n_builtin_elements   = count * blocklength * old_dtp->n_builtin_elements;
	new_dtp->builtin_element_size = el_sz;
	new_dtp->basic_type       = el_type;

	new_dtp->max_contig_blocks = old_dtp->max_contig_blocks * count * blocklength;

	eff_stride = (strideinbytes) ? stride : (stride * old_dtp->extent);
    }

    MPID_DATATYPE_VECTOR_LB_UB((MPI_Aint) count,
			       eff_stride,
			       (MPI_Aint) blocklength,
			       old_lb,
			       old_ub,
			       old_extent,
			       new_dtp->lb,
			       new_dtp->ub);
    new_dtp->true_lb = new_dtp->lb + (old_true_lb - old_lb);
    new_dtp->true_ub = new_dtp->ub + (old_true_ub - old_ub);
    new_dtp->extent  = new_dtp->ub - new_dtp->lb;

    /* new type is only contig for N types if old one was, and
     * size and extent of new type are equivalent, and stride is
     * equal to blocklength * size of old type.
     */
    if ((MPI_Aint)(new_dtp->size) == new_dtp->extent &&
	eff_stride == (MPI_Aint) blocklength * old_sz &&
	old_is_contig)
    {
	new_dtp->is_contig = 1;
        new_dtp->max_contig_blocks = 1;
    }
    else {
	new_dtp->is_contig = 0;
    }

    *newtype = new_dtp->handle;

    MPIU_DBG_MSG_P(DATATYPE,VERBOSE,"vector type %x created.",
		   new_dtp->handle);

    return mpi_errno;
}
