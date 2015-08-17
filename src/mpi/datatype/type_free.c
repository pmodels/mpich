/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Type_free */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Type_free = PMPI_Type_free
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Type_free  MPI_Type_free
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Type_free as PMPI_Type_free
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPI_Type_free(MPI_Datatype *datatype) __attribute__((weak,alias("PMPI_Type_free")));
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Type_free
#define MPI_Type_free PMPI_Type_free

#undef FUNCNAME
#define FUNCNAME MPIR_Type_free_impl
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
void MPIR_Type_free_impl(MPI_Datatype *datatype)
{
    MPID_Datatype *datatype_ptr = NULL;

    MPID_Datatype_get_ptr( *datatype, datatype_ptr );
    MPID_Datatype_release(datatype_ptr);
    *datatype = MPI_DATATYPE_NULL;
}

#endif

#undef FUNCNAME
#define FUNCNAME MPI_Type_free
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
/*@
    MPI_Type_free - Frees the datatype

Input Parameters:
. datatype - datatype that is freed (handle) 

Predefined types:

The MPI standard states that (in Opaque Objects)
.Bqs
MPI provides certain predefined opaque objects and predefined, static handles
to these objects. Such objects may not be destroyed.
.Bqe

Thus, it is an error to free a predefined datatype.  The same section makes
it clear that it is an error to free a null datatype.

.N ThreadSafe

.N Fortran

.N Errors
.N MPI_SUCCESS
.N MPI_ERR_TYPE
.N MPI_ERR_ARG
@*/
int MPI_Type_free(MPI_Datatype *datatype)
{
    int mpi_errno = MPI_SUCCESS;
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_TYPE_FREE);

    MPIR_ERRTEST_INITIALIZED_ORDIE();
    
    MPID_THREAD_CS_ENTER(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    MPID_MPI_FUNC_ENTER(MPID_STATE_MPI_TYPE_FREE);
    
    /* Validate parameters, especially handles needing to be converted */
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
	    MPIR_ERRTEST_ARGNULL(datatype, "datatype", mpi_errno);
	    MPIR_ERRTEST_DATATYPE(*datatype, "datatype", mpi_errno);
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif
    
    /* Convert MPI object handles to object pointers */
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            MPID_Datatype *datatype_ptr = NULL;

	    /* Check for built-in type */
	    if (HANDLE_GET_KIND(*datatype) == HANDLE_KIND_BUILTIN) {
		mpi_errno = MPIR_Err_create_code(MPI_SUCCESS,
						 MPIR_ERR_RECOVERABLE,
						 FCNAME, __LINE__,
						 MPI_ERR_TYPE,
						 "**dtypeperm", 0);
		goto fn_fail;
	    }

	    /* all but MPI_2INT of the pair types are not stored as builtins
	     * but should be treated similarly.
	     */
	    if (*datatype == MPI_FLOAT_INT ||
		*datatype == MPI_DOUBLE_INT ||
		*datatype == MPI_LONG_INT ||
		*datatype == MPI_SHORT_INT ||
		*datatype == MPI_LONG_DOUBLE_INT)
	    {
		mpi_errno = MPIR_Err_create_code(MPI_SUCCESS,
						 MPIR_ERR_RECOVERABLE,
						 FCNAME, __LINE__,
						 MPI_ERR_TYPE,
						  "**dtypeperm", 0);
		goto fn_fail;
	    }
            /* Validate parameters, especially handles needing to be converted */
            MPID_Datatype_get_ptr( *datatype, datatype_ptr );

            /* Validate datatype_ptr */
            MPID_Datatype_valid_ptr(datatype_ptr, mpi_errno);
            if (mpi_errno) goto fn_fail;
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */

    MPIR_Type_free_impl(datatype);

    /* ... end of body of routine ... */

#ifdef HAVE_ERROR_CHECKING
  fn_exit:
#endif
    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_TYPE_FREE);
    MPID_THREAD_CS_EXIT(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    return mpi_errno;

    /* --BEGIN ERROR HANDLING-- */
#   ifdef HAVE_ERROR_CHECKING
  fn_fail:
    {
	mpi_errno = MPIR_Err_create_code(
	    mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, 
	    "**mpi_type_free",
	    "**mpi_type_free %p", datatype);
    }
    mpi_errno = MPIR_Err_return_comm( NULL, FCNAME, mpi_errno );
    goto fn_exit;
#   endif
    /* --END ERROR HANDLING-- */
}
