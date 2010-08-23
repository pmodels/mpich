/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Attr_put */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Attr_put = PMPI_Attr_put
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Attr_put  MPI_Attr_put
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Attr_put as PMPI_Attr_put
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Attr_put
#define MPI_Attr_put PMPI_Attr_put

#endif

#undef FUNCNAME
#define FUNCNAME MPI_Attr_put

/*@

MPI_Attr_put - Stores attribute value associated with a key

Input Parameters:
+ comm - communicator to which attribute will be attached (handle) 
. keyval - key value, as returned by  'MPI_KEYVAL_CREATE' (integer) 
- attribute_val - attribute value 

Notes:
Values of the permanent attributes 'MPI_TAG_UB', 'MPI_HOST', 'MPI_IO', 
'MPI_WTIME_IS_GLOBAL', 'MPI_UNIVERSE_SIZE', 'MPI_LASTUSEDCODE', and
'MPI_APPNUM'  may not be changed. 

The type of the attribute value depends on whether C, C++, or Fortran
is being used. 
In C and C++, an attribute value is a pointer ('void *'); in Fortran,
it is a single  
integer (`not` a pointer, since Fortran has no pointers and there are systems 
for which a pointer does not fit in an integer (e.g., any > 32 bit address 
system that uses 64 bits for Fortran 'DOUBLE PRECISION').

If an attribute is already present, the delete function (specified when the
corresponding keyval was created) will be called.

.N ThreadSafe

.N Deprecated
   The replacement for this routine is 'MPI_Comm_set_attr'.

.N Fortran

.N Errors
.N MPI_SUCCESS
.N MPI_ERR_COMM
.N MPI_ERR_KEYVAL
.N MPI_ERR_PERM_KEY

.seealso MPI_Attr_get, MPI_Keyval_create, MPI_Attr_delete, MPI_Comm_set_attr
@*/
int MPI_Attr_put(MPI_Comm comm, int keyval, void *attr_value)
{
    static const char FCNAME[] = "MPI_Attr_put";
    int mpi_errno = MPI_SUCCESS;
    MPID_Comm *comm_ptr = NULL;
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_ATTR_PUT);

    MPIR_ERRTEST_INITIALIZED_ORDIE();
    
    MPIU_THREAD_CS_ENTER(ALLFUNC,);
    MPID_MPI_FUNC_ENTER(MPID_STATE_MPI_ATTR_PUT);

    /* Validate parameters, especially handles needing to be converted */
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
	    MPIR_ERRTEST_COMM(comm, mpi_errno);
            MPIR_ERRTEST_KEYVAL(keyval, MPID_COMM, "communicator", mpi_errno);
            MPIR_ERRTEST_KEYVAL_PERM(keyval, mpi_errno);
            if (mpi_errno != MPI_SUCCESS) goto fn_fail;
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif
    
    /* Convert MPI object handles to object pointers */
    MPID_Comm_get_ptr( comm, comm_ptr );

    /* Validate parameters and objects (post conversion) */
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            /* Validate comm_ptr */
            MPID_Comm_valid_ptr( comm_ptr, mpi_errno );
	    /* If comm_ptr is not valid, it will be reset to null */
            if (mpi_errno) goto fn_fail;
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */

    mpi_errno = MPIR_Comm_set_attr_impl( comm_ptr, keyval, attr_value, MPIR_ATTR_PTR );
    if (mpi_errno != MPI_SUCCESS) goto fn_fail;

    /* ... end of body of routine ... */

  fn_exit:
    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_ATTR_PUT);
    MPIU_THREAD_CS_EXIT(ALLFUNC,);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
#   ifdef HAVE_ERROR_CHECKING
    {
	mpi_errno = MPIR_Err_create_code(
	    mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**mpi_attr_put",
	    "**mpi_attr_put %C %d %p", comm, keyval, attr_value);
    }
#   endif
    mpi_errno = MPIR_Err_return_comm( comm_ptr, FCNAME, mpi_errno );
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}
