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

/* -- Begin Profiling Symbol Block for routine MPI_Type_create_keyval */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Type_create_keyval = PMPI_Type_create_keyval
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Type_create_keyval  MPI_Type_create_keyval
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Type_create_keyval as PMPI_Type_create_keyval
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPI_Type_create_keyval(MPI_Type_copy_attr_function *type_copy_attr_fn,
                           MPI_Type_delete_attr_function *type_delete_attr_fn,
                           int *type_keyval, void *extra_state) __attribute__((weak,alias("PMPI_Type_create_keyval")));
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Type_create_keyval
#define MPI_Type_create_keyval PMPI_Type_create_keyval

#endif

#undef FUNCNAME
#define FUNCNAME MPI_Type_create_keyval

/*@
   MPI_Type_create_keyval - Create an attribute keyval for MPI datatypes

Input Parameters:
+ type_copy_attr_fn - copy callback function for type_keyval (function) 
. type_delete_attr_fn - delete callback function for type_keyval (function) 
- extra_state - extra state for callback functions 

Output Parameters:
. type_keyval - key value for future access (integer) 

Notes:

   Default copy and delete functions are available.  These are
+ MPI_TYPE_NULL_COPY_FN   - empty copy function
. MPI_TYPE_NULL_DELETE_FN - empty delete function
- MPI_TYPE_DUP_FN         - simple dup function

.N AttrErrReturn

.N ThreadSafe

.N Fortran

.N Errors
.N MPI_SUCCESS
.N MPI_ERR_OTHER
@*/
int MPI_Type_create_keyval(MPI_Type_copy_attr_function *type_copy_attr_fn, 
			   MPI_Type_delete_attr_function *type_delete_attr_fn,
			   int *type_keyval, void *extra_state)
{
    static const char FCNAME[] = "MPI_Type_create_keyval";
    int mpi_errno = MPI_SUCCESS;
    MPID_Keyval *keyval_ptr;
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_TYPE_CREATE_KEYVAL);

    MPIR_ERRTEST_INITIALIZED_ORDIE();
    
    MPID_THREAD_CS_ENTER(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    MPID_MPI_FUNC_ENTER(MPID_STATE_MPI_TYPE_CREATE_KEYVAL);
    
    /* Validate parameters and objects (post conversion) */
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
	    MPIR_ERRTEST_ARGNULL(type_keyval,"type_keyval",mpi_errno);
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */
    
    keyval_ptr = (MPID_Keyval *)MPIU_Handle_obj_alloc( &MPID_Keyval_mem );
    MPIR_ERR_CHKANDJUMP(!keyval_ptr,mpi_errno,MPI_ERR_OTHER,"**nomem");

    /* Initialize the attribute dup function */
    if (!MPIR_Process.attr_dup) {
	MPIR_Process.attr_dup  = MPIR_Attr_dup_list;
	MPIR_Process.attr_free = MPIR_Attr_delete_list;
    }

    /* The handle encodes the keyval kind.  Modify it to have the correct
       field */
    keyval_ptr->handle           = (keyval_ptr->handle & ~(0x03c00000)) |
	(MPID_DATATYPE << 22);
    MPIU_Object_set_ref(keyval_ptr,1);
    keyval_ptr->was_freed        = 0;
    keyval_ptr->kind	         = MPID_DATATYPE;
    keyval_ptr->extra_state      = extra_state;
    keyval_ptr->copyfn.user_function = type_copy_attr_fn;
    keyval_ptr->copyfn.proxy = MPIR_Attr_copy_c_proxy;
    keyval_ptr->delfn.user_function = type_delete_attr_fn;
    keyval_ptr->delfn.proxy = MPIR_Attr_delete_c_proxy;

    /* Tell finalize to check for attributes on permenant types */
    MPIR_DatatypeAttrFinalize();
    
    MPID_OBJ_PUBLISH_HANDLE(*type_keyval, keyval_ptr->handle);

    /* ... end of body of routine ... */

  fn_exit:
    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_TYPE_CREATE_KEYVAL);
    MPID_THREAD_CS_EXIT(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
#   ifdef HAVE_ERROR_CHECKING
    {
	mpi_errno = MPIR_Err_create_code(
	    mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**mpi_type_create_keyval",
	    "**mpi_type_create_keyval %p %p %p %p", type_copy_attr_fn, type_delete_attr_fn, type_keyval, extra_state);
    }
#   endif
    mpi_errno = MPIR_Err_return_comm( NULL, FCNAME, mpi_errno );
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}
