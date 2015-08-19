/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Type_dup */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Type_dup = PMPI_Type_dup
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Type_dup  MPI_Type_dup
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Type_dup as PMPI_Type_dup
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPI_Type_dup(MPI_Datatype oldtype, MPI_Datatype *newtype) __attribute__((weak,alias("PMPI_Type_dup")));
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Type_dup
#define MPI_Type_dup PMPI_Type_dup

#endif

#undef FUNCNAME
#define FUNCNAME MPI_Type_dup

/*@
   MPI_Type_dup - Duplicate a datatype

Input Parameters:
. oldtype - datatype (handle)

Output Parameters:
. newtype - copy of type (handle) 

.N ThreadSafe

.N Fortran

.N Errors
.N MPI_SUCCESS
.N MPI_ERR_TYPE
@*/
int MPI_Type_dup(MPI_Datatype oldtype, MPI_Datatype *newtype)
{
    static const char FCNAME[] = "MPI_Type_dup";
    int mpi_errno = MPI_SUCCESS;
    MPI_Datatype new_handle;
    MPID_Datatype *datatype_ptr = NULL;
    MPID_Datatype *new_dtp;
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_TYPE_DUP);

    MPIR_ERRTEST_INITIALIZED_ORDIE();
    
    MPID_THREAD_CS_ENTER(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    MPID_MPI_FUNC_ENTER(MPID_STATE_MPI_TYPE_DUP);
    
    /* Validate parameters, especially handles needing to be converted */
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
	    MPIR_ERRTEST_DATATYPE(oldtype, "datatype", mpi_errno);
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif
    
    /* Convert MPI object handles to object pointers */
    MPID_Datatype_get_ptr( oldtype, datatype_ptr );
    
    /* Convert MPI object handles to object pointers */
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            /* Validate datatype_ptr */
            MPID_Datatype_valid_ptr( datatype_ptr, mpi_errno );
	    /* If comm_ptr is not valid, it will be reset to null */
	    MPIR_ERRTEST_ARGNULL(newtype, "newtype", mpi_errno);
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */
    
    mpi_errno = MPID_Type_dup(oldtype, &new_handle);

    if (mpi_errno != MPI_SUCCESS) goto fn_fail;

    MPID_Datatype_get_ptr(new_handle, new_dtp);
    mpi_errno = MPID_Datatype_set_contents(new_dtp,
				           MPI_COMBINER_DUP,
				           0, /* ints */
				           0, /* aints */
				           1, /* types */
				           NULL,
				           NULL,
				           &oldtype);

    mpi_errno = MPID_Type_commit(&new_handle);
    if (mpi_errno) { MPIR_ERR_POP(mpi_errno); }

    /* Copy attributes, executing the attribute copy functions */
    /* This accesses the attribute dup function through the perprocess
       structure to prevent type_dup from forcing the linking of the
       attribute functions.  The actual function is (by default)
       MPIR_Attr_dup_list 
    */
    if (mpi_errno == MPI_SUCCESS && MPIR_Process.attr_dup)
    {
	new_dtp->attributes = 0;
	mpi_errno = MPIR_Process.attr_dup( oldtype,
	    datatype_ptr->attributes, 
	    &new_dtp->attributes );
	if (mpi_errno)
	{
            MPID_Datatype_release(new_dtp);
	    goto fn_fail;
	}
    }

    if (mpi_errno != MPI_SUCCESS) goto fn_fail;

    MPID_OBJ_PUBLISH_HANDLE(*newtype, new_handle);
    /* ... end of body of routine ... */

  fn_exit:
    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_TYPE_DUP);
    MPID_THREAD_CS_EXIT(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
    *newtype = MPI_DATATYPE_NULL;
#   ifdef HAVE_ERROR_CHECKING
    {
	mpi_errno = MPIR_Err_create_code(
	    mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**mpi_type_dup",
	    "**mpi_type_dup %D %p", oldtype, newtype);
    }
#   endif
    mpi_errno = MPIR_Err_return_comm( NULL, FCNAME, mpi_errno );
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}
