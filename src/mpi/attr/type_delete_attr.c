/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"
#include "attr.h"

/* -- Begin Profiling Symbol Block for routine MPI_Type_delete_attr */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Type_delete_attr = PMPI_Type_delete_attr
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Type_delete_attr  MPI_Type_delete_attr
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Type_delete_attr as PMPI_Type_delete_attr
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPI_Type_delete_attr(MPI_Datatype datatype, int type_keyval) __attribute__((weak,alias("PMPI_Type_delete_attr")));
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Type_delete_attr
#define MPI_Type_delete_attr PMPI_Type_delete_attr

#endif

#undef FUNCNAME
#define FUNCNAME MPI_Type_delete_attr

/*@
   MPI_Type_delete_attr - Deletes an attribute value associated with a key on 
   a datatype

Input Parameters:
+  datatype - MPI datatype to which attribute is attached (handle)
-  type_keyval - The key value of the deleted attribute (integer) 

.N ThreadSafe

.N Fortran

.N Errors
.N MPI_SUCCESS
.N MPI_ERR_OTHER
.N MPI_ERR_KEYVAL
@*/
int MPI_Type_delete_attr(MPI_Datatype datatype, int type_keyval)
{
    static const char FCNAME[] = "MPI_Type_delete_attr";
    int mpi_errno = MPI_SUCCESS;
    MPIR_Datatype *type_ptr = NULL;
    MPIR_Attribute *p, **old_p;
    MPII_Keyval *keyval_ptr = 0;
    MPIR_FUNC_TERSE_STATE_DECL(MPID_STATE_MPI_TYPE_DELETE_ATTR);

    MPIR_ERRTEST_INITIALIZED_ORDIE();
    
    /* The thread lock prevents a valid attr delete on the same datatype
       but in a different thread from causing problems */
    MPID_THREAD_CS_ENTER(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    MPIR_FUNC_TERSE_ENTER(MPID_STATE_MPI_TYPE_DELETE_ATTR);
    
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

    /* Validate parameters and objects (post conversion) */
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
            if (mpi_errno) goto fn_fail;
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */
    
    /* Look for attribute.  They are ordered by keyval handle */

    old_p = &type_ptr->attributes;
    p     = type_ptr->attributes;
    while (p)
    {
	if (p->keyval->handle == keyval_ptr->handle)
	{
	    break;
	}
	old_p = &p->next;
	p     = p->next;
    }

    /* We can't unlock yet, because we must not free the attribute until
       we know whether the delete function has returned with a 0 status
       code */

    if (p)
    {
	/* Run the delete function, if any, and then free the attribute 
	   storage */
	mpi_errno = MPIR_Call_attr_delete( datatype, p );

	/* --BEGIN ERROR HANDLING-- */
	if (!mpi_errno)
	{
	    int in_use;
	    /* We found the attribute.  Remove it from the list */
	    *old_p = p->next;
	    /* Decrement the use of the keyval */
	    MPII_Keyval_release_ref( p->keyval, &in_use);
	    if (!in_use)
	    {
		MPIR_Handle_obj_free( &MPII_Keyval_mem, p->keyval );
	    }
	    MPID_Attr_free(p);
	}
	/* --END ERROR HANDLING-- */
    }

    if (mpi_errno != MPI_SUCCESS) goto fn_fail;
    
    /* ... end of body of routine ... */

  fn_exit:
    MPIR_FUNC_TERSE_EXIT(MPID_STATE_MPI_TYPE_DELETE_ATTR);
    MPID_THREAD_CS_EXIT(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
#   ifdef HAVE_ERROR_CHECKING
    {
	mpi_errno = MPIR_Err_create_code(
	    mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER,
	    "**mpi_type_delete_attr",
	    "**mpi_type_delete_attr %D %d", datatype, type_keyval);
    }
#   endif
    mpi_errno = MPIR_Err_return_comm( NULL, FCNAME, mpi_errno );
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}
