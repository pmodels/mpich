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
	    MPID_Datatype_get_ptr(oldtype, datatype_ptr);
            MPIR_Datatype_valid_ptr(datatype_ptr, mpi_errno);
	    /* If datatype_ptr is not valid, it will be reset to null */
            if (mpi_errno) goto fn_fail;
            MPIR_ERRTEST_ARGNULL(newtype, "newtype", mpi_errno);
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ... */

    mpi_errno = MPID_Type_create_resized(oldtype, lb, extent, &new_handle);
    /* --BEGIN ERROR HANDLING-- */
    if (mpi_errno != MPI_SUCCESS)
	goto fn_fail;
    /* --END ERROR HANDLING-- */

    aints[0] = lb;
    aints[1] = extent;

    MPID_Datatype_get_ptr(new_handle, new_dtp);
    mpi_errno = MPID_Datatype_set_contents(new_dtp,
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

