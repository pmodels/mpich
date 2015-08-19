/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 * Portions of this code were written by Microsoft. Those portions are
 * Copyright (c) 2007 Microsoft Corporation. Microsoft grants
 * permission to use, reproduce, prepare derivative works, and to
 * redistribute to others. The code is licensed "as is." The User
 * bears the risk of using it. Microsoft gives no express warranties,
 * guarantees or conditions. To the extent permitted by law, Microsoft
 * excludes the implied warranties of merchantability, fitness for a
 * particular purpose and non-infringement.
 */

#include "mpiimpl.h"
#include "attr.h"

/* -- Begin Profiling Symbol Block for routine MPI_Comm_create_keyval */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Comm_create_keyval = PMPI_Comm_create_keyval
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Comm_create_keyval  MPI_Comm_create_keyval
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Comm_create_keyval as PMPI_Comm_create_keyval
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPI_Comm_create_keyval(MPI_Comm_copy_attr_function *comm_copy_attr_fn,
                           MPI_Comm_delete_attr_function *comm_delete_attr_fn, int *comm_keyval,
                           void *extra_state) __attribute__((weak,alias("PMPI_Comm_create_keyval")));
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Comm_create_keyval
#define MPI_Comm_create_keyval PMPI_Comm_create_keyval

#undef FUNCNAME
#define FUNCNAME MPIR_Comm_create_keyval_impl
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Comm_create_keyval_impl(MPI_Comm_copy_attr_function *comm_copy_attr_fn,
                                 MPI_Comm_delete_attr_function *comm_delete_attr_fn,
                                 int *comm_keyval, void *extra_state)
{
    int mpi_errno = MPI_SUCCESS;
    MPID_Keyval *keyval_ptr;
        
    keyval_ptr = (MPID_Keyval *)MPIU_Handle_obj_alloc( &MPID_Keyval_mem );
    MPIR_ERR_CHKANDJUMP(!keyval_ptr, mpi_errno, MPI_ERR_OTHER,"**nomem");

    /* Initialize the attribute dup function */
    if (!MPIR_Process.attr_dup) {
	MPIR_Process.attr_dup  = MPIR_Attr_dup_list;
	MPIR_Process.attr_free = MPIR_Attr_delete_list;
    }

    /* The handle encodes the keyval kind.  Modify it to have the correct
       field */
    keyval_ptr->handle           = (keyval_ptr->handle & ~(0x03c00000)) |
	                           (MPID_COMM << 22);
    MPIU_Object_set_ref(keyval_ptr,1);
    keyval_ptr->was_freed        = 0;
    keyval_ptr->kind	         = MPID_COMM;
    keyval_ptr->extra_state      = extra_state;
    keyval_ptr->copyfn.user_function = comm_copy_attr_fn;
    keyval_ptr->copyfn.proxy = MPIR_Attr_copy_c_proxy;
    keyval_ptr->delfn.user_function = comm_delete_attr_fn;
    keyval_ptr->delfn.proxy = MPIR_Attr_delete_c_proxy;

    MPID_OBJ_PUBLISH_HANDLE(*comm_keyval, keyval_ptr->handle);

 fn_exit:
    return mpi_errno;
 fn_fail:

    goto fn_exit;
}

#endif

#undef FUNCNAME
#define FUNCNAME MPI_Comm_create_keyval
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
/*@
   MPI_Comm_create_keyval - Create a new attribute key 

Input Parameters:
+ comm_copy_attr_fn - Copy callback function for 'keyval'
. comm_delete_attr_fn - Delete callback function for 'keyval'
- extra_state - Extra state for callback functions 

Output Parameters:
. comm_keyval - key value for future access (integer) 

Notes:
Key values are global (available for any and all communicators).

Default copy and delete functions are available.  These are
+ MPI_COMM_NULL_COPY_FN   - empty copy function
. MPI_COMM_NULL_DELETE_FN - empty delete function
- MPI_COMM_DUP_FN         - simple dup function

There are subtle differences between C and Fortran that require that the
copy_fn be written in the same language from which 'MPI_Comm_create_keyval'
is called.
This should not be a problem for most users; only programmers using both
Fortran and C in the same program need to be sure that they follow this rule.

.N AttrErrReturn

.N ThreadSafe

.N Fortran

.N Errors
.N MPI_SUCCESS

.seealso MPI_Comm_free_keyval
@*/
int MPI_Comm_create_keyval(MPI_Comm_copy_attr_function *comm_copy_attr_fn, 
			   MPI_Comm_delete_attr_function *comm_delete_attr_fn, 
			   int *comm_keyval, void *extra_state)
{
    int mpi_errno = MPI_SUCCESS;
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_COMM_CREATE_KEYVAL);

    MPIR_ERRTEST_INITIALIZED_ORDIE();
    
    MPID_THREAD_CS_ENTER(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    MPID_MPI_FUNC_ENTER(MPID_STATE_MPI_COMM_CREATE_KEYVAL);

    /* Validate parameters and objects (post conversion) */
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
	    MPIR_ERRTEST_ARGNULL(comm_keyval, "comm_keyval", mpi_errno);
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */

    mpi_errno = MPIR_Comm_create_keyval_impl(comm_copy_attr_fn, comm_delete_attr_fn, comm_keyval, extra_state);
    if (mpi_errno) goto fn_fail;
    
    /* ... end of body of routine ... */

  fn_exit:
    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_COMM_CREATE_KEYVAL);
    MPID_THREAD_CS_EXIT(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
#   ifdef HAVE_ERROR_CHECKING
    {
	mpi_errno = MPIR_Err_create_code(
	    mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**mpi_comm_create_keyval",
	    "**mpi_comm_create_keyval %p %p %p %p", comm_copy_attr_fn, comm_delete_attr_fn, comm_keyval, extra_state);
    }
#   endif
    mpi_errno = MPIR_Err_return_comm( NULL, FCNAME, mpi_errno );
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}
