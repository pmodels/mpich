/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"
#include "mpicomm.h"

/* -- Begin Profiling Symbol Block for routine MPI_Comm_dup */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Comm_dup = PMPI_Comm_dup
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Comm_dup  MPI_Comm_dup
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Comm_dup as PMPI_Comm_dup
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Comm_dup
#define MPI_Comm_dup PMPI_Comm_dup

#undef FUNCNAME
#define FUNCNAME MPIR_Comm_dup_impl
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
int MPIR_Comm_dup_impl(MPID_Comm *comm_ptr, MPID_Comm **newcomm_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    MPID_Attribute *new_attributes = 0;

    /* Copy attributes, executing the attribute copy functions */
    /* This accesses the attribute dup function through the perprocess
       structure to prevent comm_dup from forcing the linking of the
       attribute functions.  The actual function is (by default)
       MPIR_Attr_dup_list
    */
    if (MPIR_Process.attr_dup) {
	mpi_errno = MPIR_Process.attr_dup( comm_ptr->handle,
                                           comm_ptr->attributes,
                                           &new_attributes );
        if (mpi_errno) MPIU_ERR_POP(mpi_errno);
    }

    
    /* Generate a new context value and a new communicator structure */
    /* We must use the local size, because this is compared to the
       rank of the process in the communicator.  For intercomms,
       this must be the local size */
    mpi_errno = MPIR_Comm_copy( comm_ptr, comm_ptr->local_size, newcomm_ptr );
    if (mpi_errno) MPIU_ERR_POP(mpi_errno);

    (*newcomm_ptr)->attributes = new_attributes;

 fn_exit:
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}


#endif

#undef FUNCNAME
#define FUNCNAME MPI_Comm_dup
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
/*@

MPI_Comm_dup - Duplicates an existing communicator with all its cached
               information

Input Parameter:
. comm - Communicator to be duplicated (handle) 

Output Parameter:
. newcomm - A new communicator over the same group as 'comm' but with a new
  context. See notes.  (handle) 

Notes:
  This routine is used to create a new communicator that has a new
  communication context but contains the same group of processes as
  the input communicator.  Since all MPI communication is performed
  within a communicator (specifies as the group of processes `plus`
  the context), this routine provides an effective way to create a
  private communicator for use by a software module or library.  In
  particular, no library routine should use 'MPI_COMM_WORLD' as the
  communicator; instead, a duplicate of a user-specified communicator
  should always be used.  For more information, see Using MPI, 2nd
  edition. 

  Because this routine essentially produces a copy of a communicator,
  it also copies any attributes that have been defined on the input
  communicator, using the attribute copy function specified by the
  'copy_function' argument to 'MPI_Keyval_create'.  This is
  particularly useful for (a) attributes that describe some property
  of the group associated with the communicator, such as its
  interconnection topology and (b) communicators that are given back
  to the user; the attibutes in this case can track subsequent
  'MPI_Comm_dup' operations on this communicator.

.N ThreadSafe

.N Fortran

.N Errors
.N MPI_SUCCESS
.N MPI_ERR_COMM

.seealso: MPI_Comm_free, MPI_Keyval_create, MPI_Attr_put, MPI_Attr_delete,
 MPI_Comm_create_keyval, MPI_Comm_set_attr, MPI_Comm_delete_attr
@*/
int MPI_Comm_dup(MPI_Comm comm, MPI_Comm *newcomm)
{
    int mpi_errno = MPI_SUCCESS;
    MPID_Comm *comm_ptr = NULL, *newcomm_ptr;
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_COMM_DUP);

    MPIR_ERRTEST_INITIALIZED_ORDIE();
    
    MPIU_THREAD_CS_ENTER(ALLFUNC,);
    MPID_MPI_FUNC_ENTER(MPID_STATE_MPI_COMM_DUP);
    
    /* Validate parameters, especially handles needing to be converted */
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
	    MPIR_ERRTEST_COMM(comm, mpi_errno);
            if (mpi_errno != MPI_SUCCESS) goto fn_fail;
	}
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

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
            MPIR_ERRTEST_ARGNULL(newcomm, "newcomm", mpi_errno);
            if (mpi_errno) goto fn_fail;
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */
    
    mpi_errno = MPIR_Comm_dup_impl(comm_ptr, &newcomm_ptr);
    if (mpi_errno) MPIU_ERR_POP(mpi_errno);

    MPIU_OBJ_PUBLISH_HANDLE(*newcomm, newcomm_ptr->handle);
    /* ... end of body of routine ... */

  fn_exit:
    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_COMM_DUP);
    MPIU_THREAD_CS_EXIT(ALLFUNC,);
    return mpi_errno;
    
  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
#   ifdef HAVE_ERROR_CHECKING
    {
	mpi_errno = MPIR_Err_create_code(
	    mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**mpi_comm_dup",
	    "**mpi_comm_dup %C %p", comm, newcomm);
    }
#   endif
    *newcomm = MPI_COMM_NULL;
    mpi_errno = MPIR_Err_return_comm( comm_ptr, FCNAME, mpi_errno );
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}

