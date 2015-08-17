/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"
#include "attr.h"

/* -- Begin Profiling Symbol Block for routine MPI_Comm_set_attr */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Comm_set_attr = PMPI_Comm_set_attr
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Comm_set_attr  MPI_Comm_set_attr
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Comm_set_attr as PMPI_Comm_set_attr
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPI_Comm_set_attr(MPI_Comm comm, int comm_keyval, void *attribute_val) __attribute__((weak,alias("PMPI_Comm_set_attr")));
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Comm_set_attr
#define MPI_Comm_set_attr PMPI_Comm_set_attr

#undef FUNCNAME
#define FUNCNAME MPIR_Comm_set_attr_impl
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Comm_set_attr_impl(MPID_Comm *comm_ptr, int comm_keyval, void *attribute_val, 
                            MPIR_AttrType attrType)
{
    int mpi_errno = MPI_SUCCESS;
    MPID_Keyval *keyval_ptr = NULL;
    MPID_Attribute *p;

    MPIR_ERR_CHKANDJUMP(comm_keyval == MPI_KEYVAL_INVALID, mpi_errno, MPI_ERR_KEYVAL, "**keyvalinvalid");

    /* CHANGE FOR MPI 2.2:  Look for attribute.  They are ordered by when they
       were added, with the most recent first. This uses 
       a simple linear list algorithm because few applications use more than a 
       handful of attributes */

    MPID_Keyval_get_ptr( comm_keyval, keyval_ptr );
    MPIU_Assert(keyval_ptr != NULL);

    /* printf( "Setting attr val to %x\n", attribute_val ); */
    p     = comm_ptr->attributes;
    while (p) {
	if (p->keyval->handle == keyval_ptr->handle) {
	    /* If found, call the delete function before replacing the 
	       attribute */
	    mpi_errno = MPIR_Call_attr_delete( comm_ptr->handle, p );
	    if (mpi_errno) {
		goto fn_fail;
	    }
	    p->attrType = attrType;
	    /* FIXME: This code is incorrect in some cases, particularly
	       in the case where MPIU_Pint is different from MPI_Aint, 
	       since in that case, the Fortran 9x interface will provide
	       more bytes in the attribute_val than this allows. The 
	       dual casts are a sign that this is faulty. This will 
	       need to be fixed in the type/win set_attr routines as 
	       well. */
	    p->value    = (MPID_AttrVal_t)(MPIU_Pint)attribute_val;
	    /* printf( "Updating attr at %x\n", &p->value ); */
	    /* Does not change the reference count on the keyval */
	    break;
	}
	p = p->next;
    }
    /* CHANGE FOR MPI 2.2: If not found, add at the beginning */
    if (!p) {
	MPID_Attribute *new_p = MPID_Attr_alloc();
	MPIR_ERR_CHKANDJUMP(!new_p,mpi_errno,MPI_ERR_OTHER,"**nomem");
	/* Did not find in list.  Add at end */
	new_p->keyval	     = keyval_ptr;
	new_p->attrType      = attrType;
	new_p->pre_sentinal  = 0;
	/* FIXME: See the comment above on this dual cast. */
	new_p->value	     = (MPID_AttrVal_t)(MPIU_Pint)attribute_val;
	new_p->post_sentinal = 0;
	new_p->next	     = comm_ptr->attributes;
	MPIR_Keyval_add_ref( keyval_ptr );
	comm_ptr->attributes = new_p;
	/* printf( "Creating attr at %x\n", &new_p->value ); */
    }
    
    /* Here is where we could add a hook for the device to detect attribute
       value changes, using something like
       MPID_Dev_comm_attr_hook( comm_ptr, keyval, attribute_val );
    */
    

 fn_exit:
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}


#undef FUNCNAME
#define FUNCNAME MPIR_CommSetAttr
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_CommSetAttr( MPI_Comm comm, int comm_keyval, void *attribute_val, 
		      MPIR_AttrType attrType )
{
    int mpi_errno = MPI_SUCCESS;
    MPID_Comm *comm_ptr = NULL;
    MPID_MPI_STATE_DECL(MPID_STATE_MPIR_COMM_SET_ATTR);

    MPIR_ERRTEST_INITIALIZED_ORDIE();
    
    MPID_THREAD_CS_ENTER(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    MPID_MPI_FUNC_ENTER(MPID_STATE_MPIR_COMM_SET_ATTR);

    /* Validate parameters, especially handles needing to be converted */
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
	    MPIR_ERRTEST_COMM(comm, mpi_errno);
	    MPIR_ERRTEST_KEYVAL(comm_keyval, MPID_COMM, "communicator", mpi_errno);
	    MPIR_ERRTEST_KEYVAL_PERM(comm_keyval, mpi_errno);
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
            MPID_Keyval *keyval_ptr = NULL;

            /* Validate comm_ptr */
            MPID_Comm_valid_ptr( comm_ptr, mpi_errno, TRUE );
	    /* If comm_ptr is not valid, it will be reset to null */
	    /* Validate keyval_ptr */
            MPID_Keyval_get_ptr( comm_keyval, keyval_ptr );
	    MPID_Keyval_valid_ptr( keyval_ptr, mpi_errno );
            if (mpi_errno) goto fn_fail;
	}
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */
    mpi_errno = MPIR_Comm_set_attr_impl(comm_ptr, comm_keyval, attribute_val, attrType);
    if (mpi_errno) goto fn_fail;
        
    /* ... end of body of routine ... */

  fn_exit:
    MPID_MPI_FUNC_EXIT(MPID_STATE_MPIR_COMM_SET_ATTR);
    MPID_THREAD_CS_EXIT(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
#   ifdef HAVE_ERROR_CHECKING
    {
	mpi_errno = MPIR_Err_create_code(
	    mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**mpi_comm_set_attr",
	    "**mpi_comm_set_attr %C %d %p", comm, comm_keyval, attribute_val);
    }
#   endif
    mpi_errno = MPIR_Err_return_comm( comm_ptr, FCNAME, mpi_errno );
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}
#endif /* MPICH_MPI_FROM_PMPI */

#undef FUNCNAME
#define FUNCNAME MPI_Comm_set_attr
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
/*@
   MPI_Comm_set_attr - Stores attribute value associated with a key

Input Parameters:
+ comm - communicator to which attribute will be attached (handle) 
. comm_keyval - key value, as returned by  'MPI_Comm_create_keyval' (integer)
- attribute_val - attribute value 

Notes:
Values of the permanent attributes 'MPI_TAG_UB', 'MPI_HOST', 'MPI_IO', 
'MPI_WTIME_IS_GLOBAL', 'MPI_UNIVERSE_SIZE', 'MPI_LASTUSEDCODE', and 
'MPI_APPNUM' may not be changed. 

The type of the attribute value depends on whether C, C++, or Fortran
is being used. 
In C and C++, an attribute value is a pointer ('void *'); in Fortran, it is an 
address-sized integer.

If an attribute is already present, the delete function (specified when the
corresponding keyval was created) will be called.

.N ThreadSafe

.N Fortran

.N Errors
.N MPI_SUCCESS
.N MPI_ERR_COMM
.N MPI_ERR_KEYVAL
.N MPI_ERR_PERM_KEY

.seealso MPI_Comm_create_keyval, MPI_Comm_delete_attr
@*/
int MPI_Comm_set_attr(MPI_Comm comm, int comm_keyval, void *attribute_val)
{
    int mpi_errno = MPI_SUCCESS;
    MPID_Comm *comm_ptr = NULL;
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_COMM_SET_ATTR);

    MPID_THREAD_CS_ENTER(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    MPID_MPI_FUNC_ENTER(MPID_STATE_MPI_COMM_SET_ATTR);

     /* Validate parameters, especially handles needing to be converted */
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
	    MPIR_ERRTEST_COMM(comm, mpi_errno);
	    MPIR_ERRTEST_KEYVAL(comm_keyval, MPID_COMM, "communicator", mpi_errno);
	    MPIR_ERRTEST_KEYVAL_PERM(comm_keyval, mpi_errno);
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
            MPID_Keyval *keyval_ptr = NULL;

            /* Validate comm_ptr */
            MPID_Comm_valid_ptr( comm_ptr, mpi_errno, TRUE );
	    /* If comm_ptr is not valid, it will be reset to null */
	    /* Validate keyval_ptr */
            MPID_Keyval_get_ptr( comm_keyval, keyval_ptr );
	    MPID_Keyval_valid_ptr( keyval_ptr, mpi_errno );
            if (mpi_errno) goto fn_fail;
	}
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */
    mpi_errno = MPIR_Comm_set_attr_impl(comm_ptr, comm_keyval, attribute_val, MPIR_ATTR_PTR);
    if (mpi_errno) goto fn_fail;
    /* ... end of body of routine ... */

 fn_exit:
    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_COMM_SET_ATTR);
    MPID_THREAD_CS_EXIT(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
#   ifdef HAVE_ERROR_CHECKING
    {
	mpi_errno = MPIR_Err_create_code(
	    mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**mpi_comm_set_attr",
	    "**mpi_comm_set_attr %C %d %p", comm, comm_keyval, attribute_val);
    }
#   endif
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}
