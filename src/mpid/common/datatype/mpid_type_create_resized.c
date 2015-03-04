/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */

/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include <mpiimpl.h>
#include <mpid_dataloop.h>
#include <stdlib.h>

/* #define MPID_TYPE_ALLOC_DEBUG */

static int MPIDI_Type_create_resized_memory_error(void);

int MPID_Type_create_resized(MPI_Datatype oldtype,
			     MPI_Aint lb,
			     MPI_Aint extent,
			     MPI_Datatype *newtype_p)
{
    MPID_Datatype *new_dtp;

    new_dtp = (MPID_Datatype *) MPIU_Handle_obj_alloc(&MPID_Datatype_mem);
    /* --BEGIN ERROR HANDLING-- */
    if (!new_dtp) return MPIDI_Type_create_resized_memory_error();
    /* --END ERROR HANDLING-- */

    /* handle is filled in by MPIU_Handle_obj_alloc() */
    MPIU_Object_set_ref(new_dtp, 1);
    new_dtp->is_permanent = 0;
    new_dtp->is_committed = 0;
    new_dtp->attributes   = 0;
    new_dtp->cache_id     = 0;
    new_dtp->name[0]      = 0;
    new_dtp->contents     = 0;

    new_dtp->dataloop       = NULL;
    new_dtp->dataloop_size  = -1;
    new_dtp->dataloop_depth = -1;
    new_dtp->hetero_dloop       = NULL;
    new_dtp->hetero_dloop_size  = -1;
    new_dtp->hetero_dloop_depth = -1;

    /* if oldtype is a basic, we build a contiguous dataloop of count = 1 */
    if (HANDLE_GET_KIND(oldtype) == HANDLE_KIND_BUILTIN)
    {
	int oldsize = MPID_Datatype_get_basic_size(oldtype);

	new_dtp->size           = oldsize;
	new_dtp->has_sticky_ub  = 0;
	new_dtp->has_sticky_lb  = 0;
	new_dtp->dataloop_depth = 1;
	new_dtp->true_lb        = 0;
	new_dtp->lb             = lb;
	new_dtp->true_ub        = oldsize;
	new_dtp->ub             = lb + extent;
	new_dtp->extent         = extent;
	new_dtp->alignsize      = oldsize; /* FIXME ??? */
	new_dtp->n_builtin_elements     = 1;
	new_dtp->builtin_element_size   = oldsize;
	new_dtp->is_contig      = (extent == oldsize) ? 1 : 0;
        new_dtp->basic_type         = oldtype;
	new_dtp->max_contig_blocks = 3;  /* lb, data, ub */
    }
    else
    {
	/* user-defined base type */
	MPID_Datatype *old_dtp;

	MPID_Datatype_get_ptr(oldtype, old_dtp);

	new_dtp->size           = old_dtp->size;
	new_dtp->has_sticky_ub  = 0;
	new_dtp->has_sticky_lb  = 0;
	new_dtp->dataloop_depth = old_dtp->dataloop_depth;
	new_dtp->true_lb        = old_dtp->true_lb;
	new_dtp->lb             = lb;
	new_dtp->true_ub        = old_dtp->true_ub;
	new_dtp->ub             = lb + extent;
	new_dtp->extent         = extent;
	new_dtp->alignsize      = old_dtp->alignsize;
	new_dtp->n_builtin_elements     = old_dtp->n_builtin_elements;
	new_dtp->builtin_element_size   = old_dtp->builtin_element_size;
        new_dtp->basic_type         = old_dtp->basic_type;

	new_dtp->is_contig      =
	    (extent == old_dtp->size) ? old_dtp->is_contig : 0;
	new_dtp->max_contig_blocks = old_dtp->max_contig_blocks;
    }

    *newtype_p = new_dtp->handle;

    MPIU_DBG_MSG_P(DATATYPE,VERBOSE,"resized type %x created.", 
		   new_dtp->handle);

    return MPI_SUCCESS;
}

/* --BEGIN ERROR HANDLING-- */
static int MPIDI_Type_create_resized_memory_error(void)
{
    int mpi_errno;

    mpi_errno = MPIR_Err_create_code(MPI_SUCCESS,
				     MPIR_ERR_RECOVERABLE,
				     "MPID_Type_create_resized",
				     __LINE__,
				     MPI_ERR_OTHER,
				     "**nomem",
				     0);
    return mpi_errno;
}
/* --END ERROR HANDLING-- */
