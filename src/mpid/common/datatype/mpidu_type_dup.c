/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */

/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include <mpiimpl.h>
#include <mpidu_dataloop.h>
#include <stdlib.h>

#undef FCNAME
#define FCNAME "MPIDU_Type_dup"

/* #define MPID_TYPE_ALLOC_DEBUG */

/*@
  MPIDU_Type_dup - create a copy of a datatype
 
Input Parameters:
- oldtype - handle of original datatype

Output Parameters:
. newtype - handle of newly created copy of datatype

  Return Value:
  0 on success, MPI error code on failure.
@*/
int MPIDU_Type_dup(MPI_Datatype oldtype,
		  MPI_Datatype *newtype)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDU_Datatype *new_dtp = 0, *old_dtp;

    if (HANDLE_GET_KIND(oldtype) == HANDLE_KIND_BUILTIN) {
	/* create a new type and commit it. */
	mpi_errno = MPIDU_Type_contiguous(1, oldtype, newtype);
	if (mpi_errno) { MPIR_ERR_POP(mpi_errno); }
    }
    else {
      	/* allocate new datatype object and handle */
	new_dtp = (MPIDU_Datatype *) MPIR_Handle_obj_alloc(&MPIDU_Datatype_mem);
	if (!new_dtp) {
	    /* --BEGIN ERROR HANDLING-- */
	    mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE,
					     "MPIDU_Type_dup", __LINE__, MPI_ERR_OTHER,
					     "**nomem", 0);
	    goto fn_fail;
	    /* --END ERROR HANDLING-- */
	}

	MPIDU_Datatype_get_ptr(oldtype, old_dtp);

	/* fill in datatype */
	MPIR_Object_set_ref(new_dtp, 1);
	/* new_dtp->handle is filled in by MPIR_Handle_obj_alloc() */
	new_dtp->is_contig     = old_dtp->is_contig;
	new_dtp->size          = old_dtp->size;
	new_dtp->extent        = old_dtp->extent;
	new_dtp->ub            = old_dtp->ub;
	new_dtp->lb            = old_dtp->lb;
	new_dtp->true_ub       = old_dtp->true_ub;
	new_dtp->true_lb       = old_dtp->true_lb;
	new_dtp->alignsize     = old_dtp->alignsize;
	new_dtp->has_sticky_ub = old_dtp->has_sticky_ub;
	new_dtp->has_sticky_lb = old_dtp->has_sticky_lb;
	new_dtp->is_permanent  = old_dtp->is_permanent;
	new_dtp->is_committed  = old_dtp->is_committed;

	new_dtp->attributes    = NULL; /* Attributes are copied in the
					top-level MPI_Type_dup routine */
	new_dtp->cache_id      = -1;   /* ??? */
	new_dtp->name[0]       = 0;    /* The Object name is not copied on
					  a dup */
	new_dtp->n_builtin_elements    = old_dtp->n_builtin_elements;
	new_dtp->builtin_element_size  = old_dtp->builtin_element_size;
	new_dtp->basic_type        = old_dtp->basic_type;
	
	new_dtp->dataloop       = NULL;
	new_dtp->dataloop_size  = old_dtp->dataloop_size;
	new_dtp->dataloop_depth = old_dtp->dataloop_depth;
	new_dtp->hetero_dloop       = NULL;
	new_dtp->hetero_dloop_size  = old_dtp->hetero_dloop_size;
	new_dtp->hetero_dloop_depth = old_dtp->hetero_dloop_depth;
	*newtype = new_dtp->handle;

	if (old_dtp->is_committed) {
	    MPIR_Assert(old_dtp->dataloop != NULL);
	    MPIDU_Dataloop_dup(old_dtp->dataloop,
			      old_dtp->dataloop_size,
			      &new_dtp->dataloop);
	    if (old_dtp->hetero_dloop != NULL) {
		/* at this time MPI_COMPLEX doesn't have this loop...
		 * -- RBR, 02/01/2007
		 */
		MPIDU_Dataloop_dup(old_dtp->hetero_dloop,
				  old_dtp->hetero_dloop_size,
				  &new_dtp->hetero_dloop);
	    }

#ifdef MPID_Dev_datatype_commit_hook
            MPID_Dev_datatype_commit_hook(new_dtp);
#endif /* MPID_Dev_datatype_commit_hook */
      }
    }

    MPL_DBG_MSG_D(MPIR_DBG_DATATYPE,VERBOSE, "dup type %x created.", *newtype);

 fn_fail:
    return mpi_errno;
}
