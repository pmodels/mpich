/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"
#include "attr.h"

/* -- Begin Profiling Symbol Block for routine MPI_Type_set_attr */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Type_set_attr = PMPI_Type_set_attr
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Type_set_attr  MPI_Type_set_attr
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Type_set_attr as PMPI_Type_set_attr
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPI_Type_set_attr(MPI_Datatype datatype, int type_keyval, void *attribute_val) __attribute__((weak,alias("PMPI_Type_set_attr")));
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Type_set_attr
#define MPI_Type_set_attr PMPI_Type_set_attr

#undef FUNCNAME
#define FUNCNAME MPII_Type_set_attr
int MPII_Type_set_attr(MPI_Datatype datatype, int type_keyval, void *attribute_val,
		     MPIR_Attr_type attrType )
{
    static const char FCNAME[] = "MPII_Type_set_attr";
    int mpi_errno = MPI_SUCCESS;
    MPIR_Datatype *type_ptr = NULL;
    MPII_Keyval *keyval_ptr = NULL;
    MPIR_Attribute *p, **old_p;
    MPIR_FUNC_TERSE_STATE_DECL(MPID_STATE_MPIR_TYPE_SET_ATTR);

    MPIR_ERRTEST_INITIALIZED_ORDIE();
    
    /* The thread lock prevents a valid attr delete on the same datatype
       but in a different thread from causing problems */
    MPID_THREAD_CS_ENTER(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    MPIR_FUNC_TERSE_ENTER(MPID_STATE_MPIR_TYPE_SET_ATTR);

    /* Validate parameters, especially handles needing to be converted */
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
	    MPIR_ERRTEST_DATATYPE(datatype, "datatype", mpi_errno);
	    MPIR_ERRTEST_KEYVAL(type_keyval, MPIR_DATATYPE, "datatype", mpi_errno);
	    MPIR_ERRTEST_KEYVAL_PERM(type_keyval, mpi_errno);
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif

    /* Convert MPI object handles to object pointers */
    MPIR_Datatype_get_ptr( datatype, type_ptr );
    MPII_Keyval_get_ptr( type_keyval, keyval_ptr );
    
    /* Validate parameters and objects (post conversion) */
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            /* Validate type_ptr */
            MPIR_Datatype_valid_ptr( type_ptr, mpi_errno );
	    /* If type_ptr is not valid, it will be reset to null */
	    /* Validate keyval_ptr */
		MPII_Keyval_valid_ptr( keyval_ptr, mpi_errno );
            if (mpi_errno) goto fn_fail;
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */
    /* Look for attribute.  They are ordered by keyval handle.  This uses 
       a simple linear list algorithm because few applications use more than a 
       handful of attributes */
    
    old_p = &type_ptr->attributes;
    p = type_ptr->attributes;
    while (p) {
	if (p->keyval->handle == keyval_ptr->handle) {
	    /* If found, call the delete function before replacing the 
	       attribute */
	    mpi_errno = MPIR_Call_attr_delete( datatype, p );
	    /* --BEGIN ERROR HANDLING-- */
	    if (mpi_errno) { 
		goto fn_fail;
	    }
	    /* --END ERROR HANDLING-- */
	    p->value    = (MPII_Attr_val_t)(intptr_t)attribute_val;
	    p->attrType = attrType;
	    break;
	}
	else if (p->keyval->handle > keyval_ptr->handle) {
	    MPIR_Attribute *new_p = MPID_Attr_alloc();
	    MPIR_ERR_CHKANDJUMP1(!new_p,mpi_errno,MPI_ERR_OTHER,
				 "**nomem","**nomem %s", "MPIR_Attribute" );
	    new_p->keyval	 = keyval_ptr;
	    new_p->attrType      = attrType;
	    new_p->pre_sentinal	 = 0;
	    new_p->value	 = (MPII_Attr_val_t)(intptr_t)attribute_val;
	    new_p->post_sentinal = 0;
	    new_p->next		 = p->next;
	    MPII_Keyval_add_ref( keyval_ptr );
	    p->next		 = new_p;
	    break;
	}
	old_p = &p->next;
	p = p->next;
    }
    if (!p)
    {
	MPIR_Attribute *new_p = MPID_Attr_alloc();
	MPIR_ERR_CHKANDJUMP1(!new_p,mpi_errno,MPI_ERR_OTHER,
			     "**nomem","**nomem %s", "MPIR_Attribute" );
	/* Did not find in list.  Add at end */
	new_p->keyval	     = keyval_ptr;
	new_p->attrType      = attrType;
	new_p->pre_sentinal  = 0;
	new_p->value	     = (MPII_Attr_val_t)(intptr_t)attribute_val;
	new_p->post_sentinal = 0;
	new_p->next	     = 0;
	MPII_Keyval_add_ref( keyval_ptr );
	*old_p		     = new_p;
    }
    
    /* Here is where we could add a hook for the device to detect attribute
       value changes, using something like
       MPID_Type_attr_hook( type_ptr, keyval, attribute_val );
    */

    /* ... end of body of routine ... */

  fn_exit:
    MPIR_FUNC_TERSE_EXIT(MPID_STATE_MPIR_TYPE_SET_ATTR);
    MPID_THREAD_CS_EXIT(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
#   ifdef HAVE_ERROR_CHECKING
    {
	mpi_errno = MPIR_Err_create_code(
	    mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**mpi_type_set_attr", 
	    "**mpi_type_set_attr %D %d %p", datatype, type_keyval, attribute_val);
    }
#   endif
    mpi_errno = MPIR_Err_return_comm( NULL, FCNAME, mpi_errno );
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}

#endif

#undef FUNCNAME
#define FUNCNAME MPI_Type_set_attr

/*@
   MPI_Type_set_attr - Stores attribute value associated with a key

Input Parameters:
+ datatype - MPI Datatype to which attribute will be attached (handle)
. type_keyval - key value, as returned by  'MPI_Type_create_keyval' (integer)
- attribute_val - attribute value 

Notes:

The type of the attribute value depends on whether C or Fortran is being used.
In C, an attribute value is a pointer ('void *'); in Fortran, it is an 
address-sized integer.

If an attribute is already present, the delete function (specified when the
corresponding keyval was created) will be called.
.N Fortran

.N Errors
.N MPI_SUCCESS
.N MPI_ERR_TYPE
.N MPI_ERR_KEYVAL
@*/
int MPI_Type_set_attr(MPI_Datatype datatype, int type_keyval, void *attribute_val)
{
    static const char FCNAME[] = "MPI_Type_set_attr";
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_TERSE_STATE_DECL(MPID_STATE_MPI_TYPE_SET_ATTR);

    MPIR_ERRTEST_INITIALIZED_ORDIE();
    
    MPIR_FUNC_TERSE_ENTER(MPID_STATE_MPI_TYPE_SET_ATTR);

    mpi_errno = MPII_Type_set_attr( datatype, type_keyval, attribute_val,
				  MPIR_ATTR_PTR );
    if (mpi_errno) goto fn_fail;

  fn_exit:
    MPIR_FUNC_TERSE_EXIT(MPID_STATE_MPI_TYPE_SET_ATTR);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
#   ifdef HAVE_ERROR_CHECKING
    {
	mpi_errno = MPIR_Err_create_code(
	    mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**mpi_type_set_attr", 
	    "**mpi_type_set_attr %D %d %p", datatype, type_keyval, attribute_val);
    }
#   endif
    mpi_errno = MPIR_Err_return_comm( NULL, FCNAME, mpi_errno );
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}
