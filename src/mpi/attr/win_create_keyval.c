/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"
#include "attr.h"

/* -- Begin Profiling Symbol Block for routine MPI_Win_create_keyval */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Win_create_keyval = PMPI_Win_create_keyval
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Win_create_keyval  MPI_Win_create_keyval
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Win_create_keyval as PMPI_Win_create_keyval
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Win_create_keyval
#define MPI_Win_create_keyval PMPI_Win_create_keyval

#endif

#undef FUNCNAME
#define FUNCNAME MPI_Win_create_keyval

/*@
   MPI_Win_create_keyval - Create an attribute keyval for MPI window objects

   Input Parameters:
+ win_copy_attr_fn - copy callback function for win_keyval (function) 
. win_delete_attr_fn - delete callback function for win_keyval (function) 
- extra_state - extra state for callback functions 

   Output Parameter:
. win_keyval - key value for future access (integer) 

   Notes:
   Default copy and delete functions are available.  These are
+ MPI_WIN_NULL_COPY_FN   - empty copy function
. MPI_WIN_NULL_DELETE_FN - empty delete function
- MPI_WIN_DUP_FN         - simple dup function

.N AttrErrReturn

.N ThreadSafe

.N Fortran

.N Errors
.N MPI_SUCCESS
.N MPI_ERR_ARG
.N MPI_ERR_OTHER
@*/
int MPI_Win_create_keyval(MPI_Win_copy_attr_function *win_copy_attr_fn, 
			  MPI_Win_delete_attr_function *win_delete_attr_fn, 
			  int *win_keyval, void *extra_state)
{
    static const char FCNAME[] = "MPI_Win_create_keyval";
    int mpi_errno = MPI_SUCCESS;
    MPID_Keyval *keyval_ptr;
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_WIN_CREATE_KEYVAL);

    MPIR_ERRTEST_INITIALIZED_ORDIE();
    
    MPIU_THREAD_CS_ENTER(ALLFUNC,);
    MPID_MPI_FUNC_ENTER(MPID_STATE_MPI_WIN_CREATE_KEYVAL);

    /* Validate parameters and objects (post conversion) */
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
	    MPIR_ERRTEST_ARGNULL(win_keyval,"win_keyval",mpi_errno);
            if (mpi_errno) goto fn_fail;
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */
    
    keyval_ptr = (MPID_Keyval *)MPIU_Handle_obj_alloc( &MPID_Keyval_mem );
    MPIU_ERR_CHKANDJUMP1(!keyval_ptr,mpi_errno,MPI_ERR_OTHER,"**nomem",
			 "**nomem %s", "MPID_Keyval" );
    /* Initialize the attribute dup function */
    if (!MPIR_Process.attr_dup) {
	MPIR_Process.attr_dup  = MPIR_Attr_dup_list;
	MPIR_Process.attr_free = MPIR_Attr_delete_list;
    }

    /* The handle encodes the keyval kind.  Modify it to have the correct
       field */
    keyval_ptr->handle           = (keyval_ptr->handle & ~(0x03c00000)) |
	(MPID_WIN << 22);
    MPIU_Object_set_ref(keyval_ptr,1);
    keyval_ptr->was_freed        = 0;
    keyval_ptr->kind	         = MPID_WIN;
    keyval_ptr->extra_state      = extra_state;
    keyval_ptr->copyfn.user_function = win_copy_attr_fn;
    keyval_ptr->copyfn.proxy = MPIR_Attr_copy_c_proxy;
    keyval_ptr->delfn.user_function = win_delete_attr_fn;
    keyval_ptr->delfn.proxy = MPIR_Attr_delete_c_proxy;
    
    MPIU_OBJ_PUBLISH_HANDLE(*win_keyval, keyval_ptr->handle);
    /* ... end of body of routine ... */

  fn_exit:
    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_WIN_CREATE_KEYVAL);
    MPIU_THREAD_CS_EXIT(ALLFUNC,);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
#   ifdef HAVE_ERROR_CHECKING
    {
	mpi_errno = MPIR_Err_create_code(
	    mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**mpi_win_create_keyval",
	    "**mpi_win_create_keyval %p %p %p %p", win_copy_attr_fn, win_delete_attr_fn, win_keyval, extra_state);
    }
#   endif
    mpi_errno = MPIR_Err_return_comm( NULL, FCNAME, mpi_errno );
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}
