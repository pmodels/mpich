/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Attr_get */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Attr_get = PMPI_Attr_get
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Attr_get  MPI_Attr_get
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Attr_get as PMPI_Attr_get
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPI_Attr_get(MPI_Comm comm, int keyval, void *attribute_val, int *flag) __attribute__((weak,alias("PMPI_Attr_get")));
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Attr_get
#define MPI_Attr_get PMPI_Attr_get

#endif

#undef FUNCNAME
#define FUNCNAME MPI_Attr_get

/*@

MPI_Attr_get - Retrieves attribute value by key

Input Parameters:
+ comm - communicator to which attribute is attached (handle) 
- keyval - key value (integer) 

Output Parameters:
+ attribute_val - attribute value, unless 'flag' = false
- flag -  true if an attribute value was extracted;  false if no attribute is
  associated with the key 

Notes:
    Attributes must be extracted from the same language as they were inserted  
    in with 'MPI_ATTR_PUT'.  The notes for C and Fortran below explain why.

Notes for C:
    Even though the 'attribute_val' argument is declared as 'void *', it is
    really the address of a void pointer (i.e., a 'void **').  Using
    a 'void *', however, is more in keeping with C idiom and allows the
    pointer to be passed without additional casts.


.N ThreadSafe

.N Deprecated
   The replacement for this routine is 'MPI_Comm_get_attr'.

.N Fortran

    The 'attribute_val' in Fortran is a pointer to a Fortran integer, not
    a pointer to a 'void *'.  

.N Errors
.N MPI_SUCCESS
.N MPI_ERR_COMM
.N MPI_ERR_KEYVAL

.seealso MPI_Attr_put, MPI_Keyval_create, MPI_Attr_delete, MPI_Comm_get_attr
@*/
int MPI_Attr_get(MPI_Comm comm, int keyval, void *attribute_val, int *flag)
{
    static const char FCNAME[] = "MPI_Attr_get";
    int mpi_errno = MPI_SUCCESS;
    MPID_Comm *comm_ptr = NULL;
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_ATTR_GET);

    MPIR_ERRTEST_INITIALIZED_ORDIE();
    
    MPID_THREAD_CS_ENTER(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    MPID_MPI_FUNC_ENTER(MPID_STATE_MPI_ATTR_GET);

    /* Validate parameters, especially handles needing to be converted */
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
	    MPIR_ERRTEST_COMM(comm, mpi_errno);
#           ifdef NEEDS_POINTER_ALIGNMENT_ADJUST
            /* A common user error is to pass the address of a 4-byte
	       int when the address of a pointer (or an address-sized int)
	       should have been used.  We can test for this specific
	       case.  Note that this code assumes sizeof(MPIU_Pint) is 
	       a power of 2. */
            MPIR_ERR_CHKANDJUMP((MPIU_Pint)attribute_val & (sizeof(MPIU_Pint)-1),
                                mpi_errno,MPI_ERR_ARG,"**attrnotptr");
#           endif
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
            MPID_Comm_valid_ptr( comm_ptr, mpi_errno, TRUE );
	    /* If comm_ptr is not valid, it will be reset to null */
	    MPIR_ERRTEST_ARGNULL(attribute_val, "attribute_val", mpi_errno);
	    MPIR_ERRTEST_ARGNULL(flag, "flag", mpi_errno);
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */

    mpi_errno = MPIR_CommGetAttr( comm, keyval, attribute_val, flag, MPIR_ATTR_PTR);
    if (mpi_errno != MPI_SUCCESS) goto fn_fail;

    /* ... end of body of routine ... */

  fn_exit:
    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_ATTR_GET);
    MPID_THREAD_CS_EXIT(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
#   ifdef HAVE_ERROR_CHECKING
    {
	mpi_errno = MPIR_Err_create_code(
	    mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**mpi_attr_get",
	    "**mpi_attr_get %C %d %p %p", comm, keyval, attribute_val, flag);
    }
#   endif
    mpi_errno = MPIR_Err_return_comm( comm_ptr, FCNAME, mpi_errno );
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}
