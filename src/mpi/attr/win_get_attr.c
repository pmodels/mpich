/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Win_get_attr */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Win_get_attr = PMPI_Win_get_attr
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Win_get_attr  MPI_Win_get_attr
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Win_get_attr as PMPI_Win_get_attr
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPI_Win_get_attr(MPI_Win win, int win_keyval, void *attribute_val, int *flag) __attribute__((weak,alias("PMPI_Win_get_attr")));
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Win_get_attr
#define MPI_Win_get_attr PMPI_Win_get_attr

#undef FUNCNAME
#define FUNCNAME MPIR_WinGetAttr
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_WinGetAttr( MPI_Win win, int win_keyval, void *attribute_val, 
		     int *flag, MPIR_AttrType outAttrType )
{
    int mpi_errno = MPI_SUCCESS;
    MPID_Win *win_ptr = NULL;
    MPID_MPI_STATE_DECL(MPID_STATE_MPIR_WIN_GET_ATTR);

    MPIR_ERRTEST_INITIALIZED_ORDIE();
    
    MPID_THREAD_CS_ENTER(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    MPID_MPI_FUNC_ENTER(MPID_STATE_MPIR_WIN_GET_ATTR);

    /* Validate parameters, especially handles needing to be converted */
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
	    MPIR_ERRTEST_WIN(win, mpi_errno);
	    MPIR_ERRTEST_KEYVAL(win_keyval, MPID_WIN, "window", mpi_errno);
#           ifdef NEEDS_POINTER_ALIGNMENT_ADJUST
            /* A common user error is to pass the address of a 4-byte
	       int when the address of a pointer (or an address-sized int)
	       should have been used.  We can test for this specific
	       case.  Note that this code assumes sizeof(MPIU_Pint) is
	       a power of 2. */
	    if ((MPIU_Pint)attribute_val & (sizeof(MPIU_Pint)-1)) {
		MPIR_ERR_SETANDSTMT(mpi_errno,MPI_ERR_ARG,goto fn_fail,"**attrnotptr");
	    }
#           endif
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif
    
    /* Convert MPI object handles to object pointers */
    MPID_Win_get_ptr( win, win_ptr );
    
    /* Validate parameters and objects (post conversion) */
#   ifdef HAVE_ERROR_CHECKING
    {
	MPID_BEGIN_ERROR_CHECKS;
	{
            /* Validate win_ptr */
            MPID_Win_valid_ptr( win_ptr, mpi_errno );
	    /* If win_ptr is not valid, it will be reset to null */
	    MPIR_ERRTEST_ARGNULL(attribute_val, "attribute_val", mpi_errno);
	    MPIR_ERRTEST_ARGNULL(flag, "flag", mpi_errno);
	}
	MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */    

    /* ... body of routine ...  */
    
    /* Check for builtin attribute */
    /* This code is ok for correct programs, but it would be better
       to copy the values from the per-process block and pass the user
       a pointer to a copy */
    /* Note that if we are called from Fortran, we must return the values,
       not the addresses, of these attributes */
    if (HANDLE_GET_KIND(win_keyval) == HANDLE_KIND_BUILTIN) {
	void **attr_val_p = (void **)attribute_val;
#ifdef HAVE_FORTRAN_BINDING
	/* Note that this routine only has a Fortran 90 binding,
	   so the attribute value is an address-sized int */
	MPIU_Pint  *attr_int = (MPIU_Pint *)attribute_val;
#endif
	*flag = 1;

	/* 
	 * The C versions of the attributes return the address of a 
	 * *COPY* of the value (to prevent the user from changing it)
	 * and the Fortran versions provide the actual value (as a Fint)
	 */
	switch (win_keyval) {
	case MPI_WIN_BASE:
	    *attr_val_p = win_ptr->base;
	    break;
	case MPI_WIN_SIZE:
	    win_ptr->copySize = win_ptr->size;
	    *attr_val_p = &win_ptr->copySize;
	    break;
	case MPI_WIN_DISP_UNIT:
	    win_ptr->copyDispUnit = win_ptr->disp_unit;
	    *attr_val_p = &win_ptr->copyDispUnit;
	    break;
	case MPI_WIN_CREATE_FLAVOR:
	    win_ptr->copyCreateFlavor = win_ptr->create_flavor;
	    *attr_val_p = &win_ptr->copyCreateFlavor;
	    break;
	case MPI_WIN_MODEL:
	    win_ptr->copyModel = win_ptr->model;
	    *attr_val_p = &win_ptr->copyModel;
	    break;
#ifdef HAVE_FORTRAN_BINDING
	case MPIR_ATTR_C_TO_FORTRAN(MPI_WIN_BASE):
	    /* The Fortran routine that matches this routine should
	       provide an address-sized integer, not an MPI_Fint */
	    *attr_int = MPIU_VOID_PTR_CAST_TO_MPI_AINT(win_ptr->base);
	    break;
        case MPIR_ATTR_C_TO_FORTRAN(MPI_WIN_SIZE):
	    /* We do not need to copy because we return the value,
	       not a pointer to the value */
	    *attr_int = win_ptr->size;
	    break;
	case MPIR_ATTR_C_TO_FORTRAN(MPI_WIN_DISP_UNIT):
	    /* We do not need to copy because we return the value,
	       not a pointer to the value */
	    *attr_int = win_ptr->disp_unit;
	    break;
	case MPIR_ATTR_C_TO_FORTRAN(MPI_WIN_CREATE_FLAVOR):
	    *attr_int = win_ptr->create_flavor;
	    break;
	case MPIR_ATTR_C_TO_FORTRAN(MPI_WIN_MODEL):
	    *attr_int = win_ptr->model;
	    break;
#endif
        default:
            MPIU_Assert(FALSE);
            break;
	}
    }
    else {
	MPID_Attribute *p = win_ptr->attributes;

	*flag = 0;
	while (p) {
	    if (p->keyval->handle == win_keyval) {
		*flag = 1;
		if (outAttrType == MPIR_ATTR_PTR) {
		    if (p->attrType == MPIR_ATTR_INT) {
			/* This is the tricky case: if the system is
			   bigendian, and we have to return a pointer to
			   an int, then we may need to point to the 
			   correct location in the word. */
#if defined(WORDS_LITTLEENDIAN) || (SIZEOF_VOID_P == SIZEOF_INT)
			*(void**)attribute_val = &(p->value);
#else
			int *p_loc = (int *)&(p->value);
#if SIZEOF_VOID_P == 2 * SIZEOF_INT
			p_loc++;
#else 
#error Expected sizeof(void*) to be either sizeof(int) or 2*sizeof(int)
#endif
			*(void **)attribute_val = p_loc;
#endif
		    }
		    else if (p->attrType == MPIR_ATTR_AINT) {
			*(void**)attribute_val = &(p->value);
		    }
		    else {
			*(void**)attribute_val = (void *)(MPIU_Pint)(p->value);
		    }
		}
		else
		    *(void**)attribute_val = (void *)(MPIU_Pint)(p->value);
		
		break;
	    }
	    p = p->next;
	}
    }

    /* ... end of body of routine ... */

#ifdef HAVE_ERROR_CHECKING
  fn_exit:
#endif
    MPID_MPI_FUNC_EXIT(MPID_STATE_MPIR_WIN_GET_ATTR);
    MPID_THREAD_CS_EXIT(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    return mpi_errno;

    /* --BEGIN ERROR HANDLING-- */
#   ifdef HAVE_ERROR_CHECKING
  fn_fail:
    {
	mpi_errno = MPIR_Err_create_code(
	    mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, 
	    "**mpir_wingetattr", 
	    "**mpir_wingetattr %W %d %p %p", 
	    win, win_keyval, attribute_val, flag);
    }
    mpi_errno = MPIR_Err_return_win( win_ptr, FCNAME, mpi_errno );
    goto fn_exit;
#   endif
    /* --END ERROR HANDLING-- */
}
#endif

#undef FUNCNAME
#define FUNCNAME MPI_Win_get_attr
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
/*@
   MPI_Win_get_attr - Get attribute cached on an MPI window object

Input Parameters:
+ win - window to which the attribute is attached (handle) 
- win_keyval - key value (integer) 

Output Parameters:
+ attribute_val - attribute value, unless flag is false 
- flag - false if no attribute is associated with the key (logical) 

   Notes:
   The following attributes are predefined for all MPI Window objects\:

+ MPI_WIN_BASE - window base address. 
. MPI_WIN_SIZE - window size, in bytes. 
- MPI_WIN_DISP_UNIT - displacement unit associated with the window. 

.N ThreadSafe

.N Fortran

.N Errors
.N MPI_SUCCESS
.N MPI_ERR_WIN
.N MPI_ERR_KEYVAL
.N MPI_ERR_OTHER
@*/
int MPI_Win_get_attr(MPI_Win win, int win_keyval, void *attribute_val, 
		     int *flag)
{
    int mpi_errno = MPI_SUCCESS;
#ifdef HAVE_ERROR_CHECKING
    MPID_Win *win_ptr = NULL;
#endif
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_WIN_GET_ATTR);

    MPIR_ERRTEST_INITIALIZED_ORDIE();
    
    MPID_MPI_FUNC_ENTER(MPID_STATE_MPI_WIN_GET_ATTR);

    /* ... body of routine ...  */
    mpi_errno = MPIR_WinGetAttr( win, win_keyval, attribute_val, flag, 
				 MPIR_ATTR_PTR );
    if (mpi_errno) goto fn_fail;
    /* ... end of body of routine ... */

  fn_exit:
    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_WIN_GET_ATTR);
    return mpi_errno;

    /* --BEGIN ERROR HANDLING-- */
  fn_fail:
#ifdef HAVE_ERROR_CHECKING
    {
	mpi_errno = MPIR_Err_create_code(
	    mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, 
	    "**mpi_win_get_attr", 
	    "**mpi_win_get_attr %W %d %p %p", 
	    win, win_keyval, attribute_val, flag);
    }
    MPID_Win_get_ptr( win, win_ptr );
    mpi_errno = MPIR_Err_return_win( win_ptr, FCNAME, mpi_errno );
#endif
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}
