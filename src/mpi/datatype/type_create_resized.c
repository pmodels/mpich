/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Type_create_resized */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Type_create_resized = PMPI_Type_create_resized
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Type_create_resized  MPI_Type_create_resized
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Type_create_resized as PMPI_Type_create_resized
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPI_Type_create_resized(MPI_Datatype oldtype, MPI_Aint lb, MPI_Aint extent,
                            MPI_Datatype *newtype) __attribute__((weak,alias("PMPI_Type_create_resized")));
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Type_create_resized
#define MPI_Type_create_resized PMPI_Type_create_resized

#endif

#undef FUNCNAME
#define FUNCNAME MPI_Type_create_resized

/* --BEGIN ERROR HANDLING-- */
static int MPII_Type_create_resized_memory_error(void)
{
    int mpi_errno;

    mpi_errno = MPIR_Err_create_code(MPI_SUCCESS,
				     MPIR_ERR_RECOVERABLE,
				     "MPIR_Type_create_resized",
				     __LINE__,
				     MPI_ERR_OTHER,
				     "**nomem",
				     0);
    return mpi_errno;
}
/* --END ERROR HANDLING-- */

int MPIR_Type_create_resized(MPI_Datatype oldtype,
                             MPI_Aint lb,
                             MPI_Aint extent,
                             MPI_Datatype *newtype_p)
{
    MPIR_Datatype *new_dtp;

    new_dtp = (MPIR_Datatype *) MPIR_Handle_obj_alloc(&MPIR_Datatype_mem);
    /* --BEGIN ERROR HANDLING-- */
    if (!new_dtp) return MPII_Type_create_resized_memory_error();
    /* --END ERROR HANDLING-- */

    /* handle is filled in by MPIR_Handle_obj_alloc() */
    MPIR_Object_set_ref(new_dtp, 1);
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
        int oldsize = MPIR_Datatype_get_basic_size(oldtype);

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
        MPIR_Datatype *old_dtp;

        MPIR_Datatype_get_ptr(oldtype, old_dtp);

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

        if (extent == old_dtp->size)
            MPIR_Datatype_is_contig(oldtype, &new_dtp->is_contig);
        else
            new_dtp->is_contig = 0;
        new_dtp->max_contig_blocks = old_dtp->max_contig_blocks;
    }

    *newtype_p = new_dtp->handle;

    MPL_DBG_MSG_P(MPIR_DBG_DATATYPE,VERBOSE,"resized type %x created.",
                   new_dtp->handle);

    return MPI_SUCCESS;
}

/*@
   MPI_Type_create_resized - Create a datatype with a new lower bound and
     extent from an existing datatype

Input Parameters:
+ oldtype - input datatype (handle)
. lb - new lower bound of datatype (address integer)
- extent - new extent of datatype (address integer)

Output Parameters:
. newtype - output datatype (handle)

.N ThreadSafe

.N Fortran

.N Errors
.N MPI_SUCCESS
.N MPI_ERR_TYPE
@*/
int MPI_Type_create_resized(MPI_Datatype oldtype,
			    MPI_Aint lb,
			    MPI_Aint extent,
			    MPI_Datatype *newtype)
{
    static const char FCNAME[] = "MPI_Type_create_resized";
    int mpi_errno = MPI_SUCCESS;
    MPI_Datatype new_handle;
    MPIR_Datatype *new_dtp;
    MPI_Aint aints[2];
    MPIR_FUNC_TERSE_STATE_DECL(MPID_STATE_MPI_TYPE_CREATE_RESIZED);

    MPIR_ERRTEST_INITIALIZED_ORDIE();

    MPID_THREAD_CS_ENTER(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    MPIR_FUNC_TERSE_ENTER(MPID_STATE_MPI_TYPE_CREATE_RESIZED);

    /* Get handles to MPI objects. */
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
	    MPIR_Datatype *datatype_ptr = NULL;

	    MPIR_ERRTEST_DATATYPE(oldtype, "datatype", mpi_errno);

            /* Validate datatype_ptr */
	    MPIR_Datatype_get_ptr(oldtype, datatype_ptr);
            MPIR_Datatype_valid_ptr(datatype_ptr, mpi_errno);
	    /* If datatype_ptr is not valid, it will be reset to null */
            if (mpi_errno) goto fn_fail;
            MPIR_ERRTEST_ARGNULL(newtype, "newtype", mpi_errno);
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ... */

    mpi_errno = MPIR_Type_create_resized(oldtype, lb, extent, &new_handle);
    /* --BEGIN ERROR HANDLING-- */
    if (mpi_errno != MPI_SUCCESS)
	goto fn_fail;
    /* --END ERROR HANDLING-- */

    aints[0] = lb;
    aints[1] = extent;

    MPIR_Datatype_get_ptr(new_handle, new_dtp);
    mpi_errno = MPIR_Datatype_set_contents(new_dtp,
				           MPI_COMBINER_RESIZED,
				           0,
				           2, /* Aints */
				           1,
				           NULL,
				           aints,
				           &oldtype);

    if (mpi_errno != MPI_SUCCESS) goto fn_fail;

    MPIR_OBJ_PUBLISH_HANDLE(*newtype, new_handle);
    /* ... end of body of routine ... */

  fn_exit:
    MPIR_FUNC_TERSE_EXIT(MPID_STATE_MPI_TYPE_CREATE_RESIZED);
    MPID_THREAD_CS_EXIT(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
#   ifdef HAVE_ERROR_CHECKING
    {
	mpi_errno = MPIR_Err_create_code(
	    mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**mpi_type_create_resized",
	    "**mpi_type_create_resized %D %L %L %p", oldtype, lb, extent, newtype);
    }
#   endif
    mpi_errno = MPIR_Err_return_comm(NULL, FCNAME, mpi_errno);
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}

