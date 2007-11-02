/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"
#include "attr.h"

/* -- Begin Profiling Symbol Block for routine MPI_Type_get_attr */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Type_get_attr = PMPI_Type_get_attr
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Type_get_attr  MPI_Type_get_attr
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Type_get_attr as PMPI_Type_get_attr
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Type_get_attr
#define MPI_Type_get_attr PMPI_Type_get_attr

#endif

#undef FUNCNAME
#define FUNCNAME MPI_Type_get_attr

/*@
   MPI_Type_get_attr - Retrieves attribute value by key

   Input Parameters:
+ type - datatype to which the attribute is attached (handle) 
- type_keyval - key value (integer) 

   Output Parameters:
+ attribute_val - attribute value, unless flag = false 
- flag - false if no attribute is associated with the key (logical) 

   Notes:
    Attributes must be extracted from the same language as they were inserted  
    in with 'MPI_Type_set_attr'.  The notes for C and Fortran below explain 
    why. 

Notes for C:
    Even though the 'attr_value' arguement is declared as 'void *', it is
    really the address of a void pointer.  See the rationale in the 
    standard for more details. 

.N ThreadSafe

.N Fortran

.N Errors
.N MPI_SUCCESS
.N MPI_ERR_KEYVAL
.N MPI_ERR_ARG
@*/
int MPI_Type_get_attr(MPI_Datatype type, int type_keyval, void *attribute_val, 
		      int *flag)
{
#ifdef HAVE_ERROR_CHECKING
    static const char FCNAME[] = "MPI_Type_get_attr";
#endif
    int mpi_errno = MPI_SUCCESS;
    MPID_Datatype *type_ptr = NULL;
    MPID_Attribute *p;
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_TYPE_GET_ATTR);

    MPIR_ERRTEST_INITIALIZED_ORDIE();
    
    MPIU_THREAD_SINGLE_CS_ENTER("attr");
    MPID_MPI_FUNC_ENTER(MPID_STATE_MPI_TYPE_GET_ATTR);
    
    /* Validate parameters, especially handles needing to be converted */
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
	    MPIR_ERRTEST_DATATYPE(type, "datatype", mpi_errno);
	    MPIR_ERRTEST_KEYVAL(type_keyval, MPID_DATATYPE, "datatype", mpi_errno);
#           ifdef NEEDS_POINTER_ALIGNMENT_ADJUST
            /* A common user error is to pass the address of a 4-byte
	       int when the address of a pointer (or an address-sized int)
	       should have been used.  We can test for this specific
	       case.  Note that this code assumes sizeof(MPI_Aint) is 
	       a power of 2. */
	    if ((MPI_Aint)attribute_val & (sizeof(MPI_Aint)-1)) {
		MPIU_ERR_SET(mpi_errno,MPI_ERR_ARG,"**attrnotptr");
	    }
#           endif
            if (mpi_errno != MPI_SUCCESS) goto fn_fail;
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif

    /* Convert MPI object handles to object pointers */
    MPID_Datatype_get_ptr( type, type_ptr );
    
    /* Validate parameters and objects (post conversion) */
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
	    /* Validate datatype pointer */
	    MPID_Datatype_valid_ptr( type_ptr, mpi_errno );
	    /* If type_ptr is not valid, it will be reset to null */
            if (mpi_errno) goto fn_fail;
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */
    
    *flag = 0;
    p = type_ptr->attributes;
    while (p) {
	if (p->keyval->handle == type_keyval) {
	    *flag = 1;
	    (*(void **)attribute_val) = p->value;
	    break;
	}
	p = p->next;
    }
    
    /* ... end of body of routine ... */

#ifdef HAVE_ERROR_CHECKING
  fn_exit:
#endif
    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_TYPE_GET_ATTR);
    MPIU_THREAD_SINGLE_CS_EXIT("attr");
    return mpi_errno;

    /* --BEGIN ERROR HANDLING-- */
#   ifdef HAVE_ERROR_CHECKING
  fn_fail:
    {
	mpi_errno = MPIR_Err_create_code(
	    mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, 
	    "**mpi_type_get_attr",
	    "**mpi_type_get_attr %D %d %p %p", 
	    type, type_keyval, attribute_val, flag);
    }
    mpi_errno = MPIR_Err_return_comm( NULL, FCNAME, mpi_errno );
    goto fn_exit;
#   endif
    /* --END ERROR HANDLING-- */
}
