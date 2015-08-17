/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"
#include "attr.h"

/* -- Begin Profiling Symbol Block for routine MPI_Win_set_attr */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Win_set_attr = PMPI_Win_set_attr
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Win_set_attr  MPI_Win_set_attr
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Win_set_attr as PMPI_Win_set_attr
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPI_Win_set_attr(MPI_Win win, int win_keyval, void *attribute_val) __attribute__((weak,alias("PMPI_Win_set_attr")));
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Win_set_attr
#define MPI_Win_set_attr PMPI_Win_set_attr

#undef FUNCNAME
#define FUNCNAME MPIR_WinSetAttr
int MPIR_WinSetAttr( MPI_Win win, int win_keyval, void *attribute_val, 
		     MPIR_AttrType attrType )
{
    static const char FCNAME[] = "MPI_Win_set_attr";
    int mpi_errno = MPI_SUCCESS;
    MPID_Win *win_ptr = NULL;
    MPID_Keyval *keyval_ptr = NULL;
    MPID_Attribute *p, **old_p;
    MPID_MPI_STATE_DECL(MPID_STATE_MPIR_WIN_SET_ATTR);

    MPIR_ERRTEST_INITIALIZED_ORDIE();
    
    /* The thread lock prevents a valid attr delete on the same window
       but in a different thread from causing problems */
    MPID_THREAD_CS_ENTER(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    MPID_MPI_FUNC_ENTER(MPID_STATE_MPIR_WIN_SET_ATTR);

    /* Validate parameters, especially handles needing to be converted */
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
	    MPIR_ERRTEST_WIN(win, mpi_errno);
	    MPIR_ERRTEST_KEYVAL(win_keyval, MPID_WIN, "window", mpi_errno);
	    MPIR_ERRTEST_KEYVAL_PERM(win_keyval, mpi_errno);
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif

    /* Convert MPI object handles to object pointers */
    MPID_Win_get_ptr( win, win_ptr );
    MPID_Keyval_get_ptr( win_keyval, keyval_ptr );

    /* Validate parameters and objects (post conversion) */
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            /* Validate win_ptr */
            MPID_Win_valid_ptr( win_ptr, mpi_errno );
	    /* If win_ptr is not valid, it will be reset to null */
	    /* Validate keyval */
	    MPID_Keyval_valid_ptr( keyval_ptr, mpi_errno );
            if (mpi_errno) goto fn_fail;
	}
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */
    
    /* Look for attribute.  They are ordered by keyval handle.  This uses 
       a simple linear list algorithm because few applications use more than a 
       handful of attributes */
    
    old_p = &win_ptr->attributes;
    p = win_ptr->attributes;
    while (p)
    {
	if (p->keyval->handle == keyval_ptr->handle)
	{
	    /* If found, call the delete function before replacing the 
	       attribute */
	    mpi_errno = MPIR_Call_attr_delete( win, p );
	    /* --BEGIN ERROR HANDLING-- */
	    if (mpi_errno)
	    {
		/* FIXME : communicator of window? */
		goto fn_fail;
	    }
	    /* --END ERROR HANDLING-- */
	    p->value    = (MPID_AttrVal_t)(MPIU_Pint)attribute_val;
	    p->attrType = attrType;
	    /* Does not change the reference count on the keyval */
	    break;
	}
	else if (p->keyval->handle > keyval_ptr->handle) {
	    MPID_Attribute *new_p = MPID_Attr_alloc();
	    MPIR_ERR_CHKANDJUMP1(!new_p,mpi_errno,MPI_ERR_OTHER,
				 "**nomem", "**nomem %s", "MPID_Attribute" );
	    new_p->keyval	 = keyval_ptr;
	    new_p->attrType      = attrType;
	    new_p->pre_sentinal	 = 0;
	    new_p->value	 = (MPID_AttrVal_t)(MPIU_Pint)attribute_val;
	    new_p->post_sentinal = 0;
	    new_p->next		 = p->next;
	    MPIR_Keyval_add_ref( keyval_ptr );
	    p->next		 = new_p;
	    break;
	}
	old_p = &p->next;
	p = p->next;
    }
    if (!p)
    {
	MPID_Attribute *new_p = MPID_Attr_alloc();
	MPIR_ERR_CHKANDJUMP1(!new_p,mpi_errno,MPI_ERR_OTHER,
			     "**nomem", "**nomem %s", "MPID_Attribute" );
	/* Did not find in list.  Add at end */
	new_p->attrType      = attrType;
	new_p->keyval	     = keyval_ptr;
	new_p->pre_sentinal  = 0;
	new_p->value	     = (MPID_AttrVal_t)(MPIU_Pint)attribute_val;
	new_p->post_sentinal = 0;
	new_p->next	     = 0;
	MPIR_Keyval_add_ref( keyval_ptr );
	*old_p		     = new_p;
    }
    
    /* Here is where we could add a hook for the device to detect attribute
       value changes, using something like
       MPID_Dev_win_attr_hook( win_ptr, keyval, attribute_val );
    */
    
    /* ... end of body of routine ... */

  fn_exit:
    MPID_MPI_FUNC_EXIT(MPID_STATE_MPIR_WIN_SET_ATTR);
    MPID_THREAD_CS_EXIT(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX); 
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
#   ifdef HAVE_ERROR_CHECKING
    {
	mpi_errno = MPIR_Err_create_code(
	    mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**mpi_win_set_attr", 
	    "**mpi_win_set_attr %W %d %p", win, win_keyval, attribute_val);
    }
#   endif
    mpi_errno = MPIR_Err_return_win( win_ptr, FCNAME, mpi_errno );
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}
#endif

#undef FUNCNAME
#define FUNCNAME MPI_Win_set_attr
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)

/*@
   MPI_Win_set_attr - Stores attribute value associated with a key

Input Parameters:
+ win - MPI window object to which attribute will be attached (handle) 
. win_keyval - key value, as returned by  'MPI_Win_create_keyval' (integer)
- attribute_val - attribute value 

Notes:

The type of the attribute value depends on whether C or Fortran is being used.
In C, an attribute value is a pointer ('void *'); in Fortran, it is an 
address-sized integer.

If an attribute is already present, the delete function (specified when the
corresponding keyval was created) will be called.

.N ThreadSafe

.N Fortran

.N Errors
.N MPI_SUCCESS
.N MPI_ERR_WIN
.N MPI_ERR_KEYVAL
@*/
int MPI_Win_set_attr(MPI_Win win, int win_keyval, void *attribute_val)
{
    int mpi_errno = MPI_SUCCESS;
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_WIN_SET_ATTR);
    MPID_MPI_FUNC_ENTER(MPID_STATE_MPI_WIN_SET_ATTR);

    MPIR_ERRTEST_INITIALIZED_ORDIE();
    
    /* ... body of routine ...  */
    mpi_errno = MPIR_WinSetAttr( win, win_keyval, attribute_val, 
				 MPIR_ATTR_PTR );
    if (mpi_errno) goto fn_fail;
    /* ... end of body of routine ... */

  fn_exit:
    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_WIN_SET_ATTR);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
#   ifdef HAVE_ERROR_CHECKING
    {
	mpi_errno = MPIR_Err_create_code(
	    mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**mpi_win_set_attr", 
	    "**mpi_win_set_attr %W %d %p", win, win_keyval, attribute_val);
    }
#   endif
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}
