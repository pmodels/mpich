/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Type_get_contents */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Type_get_contents = PMPI_Type_get_contents
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Type_get_contents  MPI_Type_get_contents
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Type_get_contents as PMPI_Type_get_contents
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPI_Type_get_contents(MPI_Datatype datatype, int max_integers, int max_addresses,
                          int max_datatypes, int array_of_integers[],
                          MPI_Aint array_of_addresses[], MPI_Datatype array_of_datatypes[]) __attribute__((weak,alias("PMPI_Type_get_contents")));
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Type_get_contents
#define MPI_Type_get_contents PMPI_Type_get_contents

#endif

#undef FUNCNAME
#define FUNCNAME MPI_Type_get_contents

/*@
   MPI_Type_get_contents - get type contents

Input Parameters:
+  datatype - datatype to access (handle)
.  max_integers - number of elements in array_of_integers (non-negative integer)
.  max_addresses - number of elements in array_of_addresses (non-negative integer)
-  max_datatypes - number of elements in array_of_datatypes (non-negative integer)

Output Parameters:
+  array_of_integers - contains integer arguments used in constructing the datatype (array of integers)
.  array_of_addresses - contains address arguments used in constructing the datatype (array of integers)
-  array_of_datatypes - contains datatype arguments used in constructing the datatype (array of handles)

   Notes:

.N Fortran

.N Errors
.N MPI_SUCCESS
@*/
int MPI_Type_get_contents(MPI_Datatype datatype,
			  int max_integers,
			  int max_addresses,
			  int max_datatypes,
			  int array_of_integers[],
			  MPI_Aint array_of_addresses[],
			  MPI_Datatype array_of_datatypes[])
{
    static const char FCNAME[] = "MPI_Type_get_contents";
    int mpi_errno = MPI_SUCCESS;
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_TYPE_GET_CONTENTS);

    MPIR_ERRTEST_INITIALIZED_ORDIE();
    
    MPID_THREAD_CS_ENTER(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    MPID_MPI_FUNC_ENTER(MPID_STATE_MPI_TYPE_GET_CONTENTS);
    
    /* Validate parameters, especially handles needing to be converted */
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
	    MPIR_ERRTEST_DATATYPE(datatype, "datatype", mpi_errno);
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif
    
    /* Validate parameters and objects (post conversion) */
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            MPID_Datatype *datatype_ptr = NULL;

	    /* Check for built-in type */
	    if (HANDLE_GET_KIND(datatype) == HANDLE_KIND_BUILTIN) {
		mpi_errno = MPIR_Err_create_code(MPI_SUCCESS,
						 MPIR_ERR_RECOVERABLE,
						 FCNAME, __LINE__,
						 MPI_ERR_TYPE,
						 "**contentspredef", 0);
		goto fn_fail;
	    }

	    /* all but MPI_2INT of the pair types are not stored as builtins
	     * but should be treated similarly.
	     */
	    if (datatype == MPI_FLOAT_INT ||
		datatype == MPI_DOUBLE_INT ||
		datatype == MPI_LONG_INT ||
		datatype == MPI_SHORT_INT ||
		datatype == MPI_LONG_DOUBLE_INT)
	    {
		mpi_errno = MPIR_Err_create_code(MPI_SUCCESS,
						 MPIR_ERR_RECOVERABLE,
						 FCNAME, __LINE__,
						 MPI_ERR_TYPE,
						 "**contentspredef", 0);
		goto fn_fail;
	    }

            /* Convert MPI object handles to object pointers */
            MPID_Datatype_get_ptr(datatype, datatype_ptr);

            /* Validate datatype_ptr */
            MPID_Datatype_valid_ptr(datatype_ptr, mpi_errno);
	    /* If comm_ptr is not value, it will be reset to null */
            if (mpi_errno) goto fn_fail;
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */
    
    mpi_errno = MPID_Type_get_contents(datatype,
				       max_integers,
				       max_addresses,
				       max_datatypes,
				       array_of_integers,
				       array_of_addresses,
				       array_of_datatypes);
    if (mpi_errno != MPI_SUCCESS) goto fn_fail;

    /* ... end of body of routine ... */

  fn_exit:
    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_TYPE_GET_CONTENTS);
    MPID_THREAD_CS_EXIT(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
#   ifdef HAVE_ERROR_CHECKING
    {
	mpi_errno = MPIR_Err_create_code(
	    mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**mpi_type_get_contents",
	    "**mpi_type_get_contents %D %d %d %d %p %p %p", datatype, max_integers, max_addresses, max_datatypes,
	    array_of_integers, array_of_addresses, array_of_datatypes);
    }
#   endif
    mpi_errno = MPIR_Err_return_comm( NULL, FCNAME, mpi_errno );
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}



