/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Comm_get_attr */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Comm_get_attr = PMPI_Comm_get_attr
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Comm_get_attr  MPI_Comm_get_attr
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Comm_get_attr as PMPI_Comm_get_attr
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPI_Comm_get_attr(MPI_Comm comm, int comm_keyval, void *attribute_val, int *flag) __attribute__((weak,alias("PMPI_Comm_get_attr")));
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Comm_get_attr
#define MPI_Comm_get_attr PMPI_Comm_get_attr

#undef FUNCNAME
#define FUNCNAME MPIR_CommGetAttr

/* Find the requested attribute.  If it exists, return either the attribute
   entry or the address of the entry, based on whether the request is for 
   a pointer-valued attribute (C or C++) or an integer-valued attribute
   (Fortran, either 77 or 90).

   If the attribute has the same type as the request, it is returned as-is.
   Otherwise, the address of the attribute is returned.
*/
int MPIR_CommGetAttr( MPI_Comm comm, int comm_keyval, void *attribute_val, 
		      int *flag, MPIR_AttrType outAttrType )
{
    static const char FCNAME[] = "MPIR_CommGetAttr";
    int mpi_errno = MPI_SUCCESS;
    MPID_Comm *comm_ptr = NULL;
    static PreDefined_attrs attr_copy;    /* Used to provide a copy of the
					     predefined attributes */
    MPID_MPI_STATE_DECL(MPID_STATE_MPIR_COMM_GET_ATTR);

    MPIR_ERRTEST_INITIALIZED_ORDIE();
    
    MPID_MPI_FUNC_ENTER(MPID_STATE_MPIR_COMM_GET_ATTR);
    
    /* Validate parameters, especially handles needing to be converted */
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
	    MPIR_ERRTEST_COMM(comm, mpi_errno);
	    MPIR_ERRTEST_KEYVAL(comm_keyval, MPID_COMM, "communicator", mpi_errno);
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
    MPID_Comm_get_ptr( comm, comm_ptr );

    /* Validate parameters and objects (post conversion) */
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            /* Validate comm_ptr */
            MPID_Comm_valid_ptr( comm_ptr, mpi_errno, TRUE );
	    /* If comm_ptr is not valid, it will be reset to null */
	    MPIR_ERRTEST_ARGNULL(attribute_val, "attr_val", mpi_errno);
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
    if (HANDLE_GET_KIND(comm_keyval) == HANDLE_KIND_BUILTIN) {
	int attr_idx = comm_keyval & 0x0000000f;
	void **attr_val_p = (void **)attribute_val;
#ifdef HAVE_FORTRAN_BINDING
	/* This is an address-sized int instead of a Fortran (MPI_Fint)
	   integer because, even for the Fortran keyvals, the C interface is 
	   used which stores the result in a pointer (hence we need a
	   pointer-sized int).  Thus we use MPIU_Pint instead of MPI_Fint.
	   On some 64-bit plaforms, such as Solaris-SPARC, using an MPI_Fint
	   will cause the value to placed into the high, rather than low,
	   end of the output value. */
#endif
	*flag = 1;

	/* FIXME : We could initialize some of these here; only tag_ub is 
	 used in the error checking. */
	/* 
	 * The C versions of the attributes return the address of a 
	 * *COPY* of the value (to prevent the user from changing it)
	 * and the Fortran versions provide the actual value (as an Fint)
	 */
	attr_copy = MPIR_Process.attrs;
	switch (attr_idx) {
	case 1: /* TAG_UB */
	case 2:
	    *attr_val_p = &attr_copy.tag_ub;
	    break;
	case 3: /* HOST */
	case 4:
	    *attr_val_p = &attr_copy.host;
	    break;
	case 5: /* IO */
	case 6:
	    *attr_val_p = &attr_copy.io;
	    break;
	case 7: /* WTIME */
	case 8:
	    *attr_val_p = &attr_copy.wtime_is_global;
	    break;
	case 9: /* UNIVERSE_SIZE */
	case 10:
	    /* This is a special case.  If universe is not set, then we
	       attempt to get it from the device.  If the device is doesn't
	       supply a value, then we set the flag accordingly.  Note that
	       we must initialize the cached value, in MPIR_Process.attr, 
	       not the value in the copy, since that is vulnerable to being
	       overwritten by the user, and because we always re-initialize
	       the copy from the cache (yes, there was a time when the code
	       did the wrong thing here). */
	    if (attr_copy.universe >= 0)
	    { 
		*attr_val_p = &attr_copy.universe;
	    }
	    else if (attr_copy.universe == MPIR_UNIVERSE_SIZE_NOT_AVAILABLE)
	    {
		*flag = 0;
	    }
	    else
	    {
		/* We call MPID_Get_universe_size only once (see 10.5.1).
		   Thus, we must put the value into the "master" copy*/
		mpi_errno = MPID_Get_universe_size(&MPIR_Process.attrs.universe);
		/* --BEGIN ERROR HANDLING-- */
		if (mpi_errno != MPI_SUCCESS)
		{
		    MPIR_Process.attrs.universe = MPIR_UNIVERSE_SIZE_NOT_AVAILABLE;
		    goto fn_fail;
		}
		/* --END ERROR HANDLING-- */
		attr_copy.universe = MPIR_Process.attrs.universe;
		if (attr_copy.universe >= 0)
		{
		    *attr_val_p = &attr_copy.universe;
		}
		else
		{
		    *flag = 0;
		}
	    }
	    break;
	case 11: /* LASTUSEDCODE */
	case 12:
	    *attr_val_p = &attr_copy.lastusedcode;
	    break;
	case 13: /* APPNUM */
	case 14:
	    /* This is another special case.  If appnum is negative,
	       we take that as indicating no value of APPNUM, and set
	       the flag accordingly */
	    if (attr_copy.appnum < 0) {
		*flag = 0;
	    }
	    else {
		*attr_val_p = &attr_copy.appnum;
	    }
	    break;
	}
	/* All of the predefined attributes are stored internally as C ints; 
	   since we've set the output value as the pointer to these, we need 
	   to dereference it here. We must also be very careful of the 3 
	   different output cases, since the two Fortran cases correspond
	   to MPI_Fint and MPIR_FAint (an internal MPICH typedef for C 
	   version of INTEGER (KIND=MPI_ADDRESS_KIND) ) */
	if (*flag) {
            /* Use the internal pointer-sized-int for systems (e.g., BG/P)
               that define MPI_Aint as a different size than MPIU_Pint.
	       The casts must be as they are:
	       On the right, the value is a pointer to an int, so to 
	       get the correct value, we need to extract the int.
	       On the left, the output type is given by the argument 
	       outAttrType - and the cast must match the intended results */
	    /* FIXME: This code is broken.  The MPIR_ATTR_INT is for Fortran
	       MPI_Fint types, not int, and MPIR_ATTR_AINT is for Fortran
	       INTEGER(KIND=MPI_ADDRESS_KIND), which is probably an MPI_Aint,
	       and MPIU_Pint is for exactly the case where MPI_Aint is not
	       the same as MPIU_Pint. 
	       This code needs to be fixed in every place that it occurs 
	       (i.e., see the win and type get_attr routines). */
	    if (outAttrType == MPIR_ATTR_AINT)
		*(MPIU_Pint*)attr_val_p = *(int*)*(void **)attr_val_p;
	    else if (outAttrType == MPIR_ATTR_INT) {
		/* *(int*)attr_val_p = *(int *)*(void **)attr_val_p;*/
                /* This is correct, because the cooresponding code 
                   in the Fortran interface expects to find a pointer-length
                   integer value.  Thus, this works for both big and little
                   endian systems. Any changes made here must have
                   corresponding changes in src/binding/f77/attr_getf.c ,
                   which is generated by src/binding/f77/buildiface . */
                *(MPIU_Pint*)attr_val_p = *(int*)*(void **)attr_val_p;
            }
	}
    }
    else {
	MPID_Attribute *p = comm_ptr->attributes;

	/*   */
	*flag = 0;
	while (p) {
	    if (p->keyval->handle == comm_keyval) {
		*flag                  = 1;
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
		else {
		    *(void**)attribute_val = (void *)(MPIU_Pint)(p->value);
                }

		break;
	    }
	    p = p->next;
	}
    }
    /* ... end of body of routine ... */

  fn_exit:
    MPID_MPI_FUNC_EXIT(MPID_STATE_MPIR_COMM_GET_ATTR);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
#   ifdef HAVE_ERROR_CHECKING
    {
	mpi_errno = MPIR_Err_create_code(
	    mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**mpir_comm_get_attr",
	    "**mpir_comm_get_attr %C %d %p %p", comm, comm_keyval, attribute_val, flag);
    }
#   endif
    mpi_errno = MPIR_Err_return_comm( comm_ptr, FCNAME, mpi_errno );
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}

/* This function is called by the fortran bindings. */
/* FIXME: There is no reason to have this routine since it unnecessarily 
   duplicates the MPIR_CommGetAttr interface. */
int MPIR_CommGetAttr_fort(MPI_Comm comm, int comm_keyval, void *attribute_val,
                          int *flag, MPIR_AttrType outAttrType )
{
    int mpi_errno;
    
    MPID_THREAD_CS_ENTER(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    mpi_errno = MPIR_CommGetAttr(comm, comm_keyval, attribute_val, flag, outAttrType);
    MPID_THREAD_CS_EXIT(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);

    return mpi_errno;
}


#endif /* MPICH_MPI_FROM_PMPI */

#undef FUNCNAME
#define FUNCNAME MPI_Comm_get_attr
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)

/* FIXME: Attributes must be visable from all languages */
/*@
   MPI_Comm_get_attr - Retrieves attribute value by key

Input Parameters:
+ comm - communicator to which attribute is attached (handle) 
- comm_keyval - key value (integer)

Output Parameters:
+ attribute_val - attribute value, unless 'flag' = false
- flag -  true if an attribute value was extracted;  false if no attribute is
  associated with the key 

   Notes:
    Attributes must be extracted from the same language as they were inserted  
    in with 'MPI_Comm_set_attr'.  The notes for C and Fortran below explain 
    why. 

Notes for C:
    Even though the 'attr_value' argument is declared as 'void *', it is
    really the address of a void pointer.  See the rationale in the 
    standard for more details. 

.N ThreadSafe

.N Fortran

.N Errors
.N MPI_SUCCESS
.N MPI_ERR_COMM
.N MPI_ERR_KEYVAL
@*/
int MPI_Comm_get_attr(MPI_Comm comm, int comm_keyval, void *attribute_val, 
		      int *flag)
{
    int mpi_errno = MPI_SUCCESS;
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_COMM_GET_ATTR);

    MPIR_ERRTEST_INITIALIZED_ORDIE();
    
    MPID_THREAD_CS_ENTER(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    MPID_MPI_FUNC_ENTER(MPID_STATE_MPI_COMM_GET_ATTR);

    /* Instead, ask for a desired type. */
    mpi_errno = MPIR_CommGetAttr( comm, comm_keyval, attribute_val, flag, 
				  MPIR_ATTR_PTR );
    if (mpi_errno) goto fn_fail;
    /* ... end of body of routine ... */

  fn_exit:
    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_COMM_GET_ATTR);
    MPID_THREAD_CS_EXIT(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
#   ifdef HAVE_ERROR_CHECKING
    {
	mpi_errno = MPIR_Err_create_code(
	    mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**mpi_comm_get_attr",
	    "**mpi_comm_get_attr %C %d %p %p", comm, comm_keyval, attribute_val, flag);
    }
#   endif
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}
