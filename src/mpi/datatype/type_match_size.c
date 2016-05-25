/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Type_match_size */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Type_match_size = PMPI_Type_match_size
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Type_match_size  MPI_Type_match_size
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Type_match_size as PMPI_Type_match_size
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPI_Type_match_size(int typeclass, int size, MPI_Datatype *datatype) __attribute__((weak,alias("PMPI_Type_match_size")));
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Type_match_size
#define MPI_Type_match_size PMPI_Type_match_size

#endif

#undef FUNCNAME
#define FUNCNAME MPI_Type_match_size

/*@
   MPI_Type_match_size - Find an MPI datatype matching a specified size

Input Parameters:
+ typeclass - generic type specifier (integer) 
- size - size, in bytes, of representation (integer) 

Output Parameters:
. datatype - datatype with correct type, size (handle)

Notes:
'typeclass' is one of 'MPI_TYPECLASS_REAL', 'MPI_TYPECLASS_INTEGER' and 
'MPI_TYPECLASS_COMPLEX', corresponding to the desired typeclass. 
The function returns an MPI datatype matching a local variable of type 
'( typeclass, size )'. 

.N ThreadSafe

.N Fortran

.N Errors
.N MPI_SUCCESS
.N MPI_ERR_ARG
@*/
int MPI_Type_match_size(int typeclass, int size, MPI_Datatype *datatype)
{
    static const char FCNAME[] = "MPI_Type_match_size";
    int mpi_errno = MPI_SUCCESS;
#ifdef HAVE_ERROR_CHECKING
    static const char *tname = 0;
#endif
    /* Note that all of the datatype have values, even if the type is 
       not available. We test for that case separately.  We also 
       prefer the Fortran types to the C type, if they are available */
    static MPI_Datatype real_types[] = { 
	MPI_REAL4, MPI_REAL8, MPI_REAL16,
	MPI_REAL, MPI_DOUBLE_PRECISION, 
	MPI_FLOAT, MPI_DOUBLE, MPI_LONG_DOUBLE
    };
    static MPI_Datatype int_types[] = { 
	MPI_INTEGER1, MPI_INTEGER2, MPI_INTEGER4, MPI_INTEGER8, MPI_INTEGER16,
	MPI_INTEGER, 
	MPI_CHAR, MPI_SHORT, MPI_INT, 
	MPI_LONG, MPI_LONG_LONG
    };
    static MPI_Datatype complex_types[] = { 
	MPI_COMPLEX8, MPI_COMPLEX16, MPI_COMPLEX32,
	MPI_COMPLEX, MPI_DOUBLE_COMPLEX,
	MPI_C_COMPLEX, MPI_C_DOUBLE_COMPLEX, MPI_C_LONG_DOUBLE_COMPLEX,
    };
    MPI_Datatype matched_datatype = MPI_DATATYPE_NULL;
    int i;
    MPI_Aint tsize;
    MPIR_FUNC_TERSE_STATE_DECL(MPID_STATE_MPI_TYPE_MATCH_SIZE);

    MPIR_ERRTEST_INITIALIZED_ORDIE();

    /* FIXME: This routine does not require the global critical section */
    MPID_THREAD_CS_ENTER(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    MPIR_FUNC_TERSE_ENTER(MPID_STATE_MPI_TYPE_MATCH_SIZE);

    /* Validate parameters and objects (post conversion) */
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
	    MPIR_ERRTEST_ARGNULL( datatype, "datatype", mpi_errno );
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */
    
    /* The following implementation follows the suggestion in the
       MPI-2 standard.  
       The version in the MPI-2 spec makes use of the Fortran optional types;
       currently, we don't support these from C (see mpi.h.in).  
       Thus, we look at the candidate types and make use of the first fit.
       Note that the standard doesn't require that this routine return 
       any particular choice of MPI datatype; e.g., it is not required
       to return MPI_INTEGER4 if a 4-byte integer is requested.
    */
    switch (typeclass) {
    case MPI_TYPECLASS_REAL: 
	{
	    int nRealTypes = sizeof(real_types) / sizeof(MPI_Datatype);
#ifdef HAVE_ERROR_CHECKING
	    tname = "MPI_TYPECLASS_REAL";
#endif
	    for (i=0; i<nRealTypes; i++) {
		if (real_types[i] == MPI_DATATYPE_NULL) { continue; }
		MPIR_Datatype_get_size_macro( real_types[i], tsize );
		if (tsize == size) {
		    matched_datatype = real_types[i];
		    break;
		}
	    }
	}
	break;
    case MPI_TYPECLASS_INTEGER:
	{
	    int nIntTypes = sizeof(int_types) / sizeof(MPI_Datatype);
#ifdef HAVE_ERROR_CHECKING
	    tname = "MPI_TYPECLASS_INTEGER";
#endif
	    for (i=0; i<nIntTypes; i++) {
		if (int_types[i] == MPI_DATATYPE_NULL) { continue; }
		MPIR_Datatype_get_size_macro( int_types[i], tsize );
		if (tsize == size) {
		    matched_datatype = int_types[i];
		    break;
		}
	    }
	}
	break;
    case MPI_TYPECLASS_COMPLEX:
	{
	    int nComplexTypes = sizeof(complex_types) / sizeof(MPI_Datatype);
#ifdef HAVE_ERROR_CHECKING
	    tname = "MPI_TYPECLASS_COMPLEX";
#endif
	    for (i=0; i<nComplexTypes; i++) {
		if (complex_types[i] == MPI_DATATYPE_NULL) { continue; }
		MPIR_Datatype_get_size_macro( complex_types[i], tsize );
		if (tsize == size) {
		    matched_datatype = complex_types[i];
		    break;
		}
	    }
	}
	break;
    default:
	/* --BEGIN ERROR HANDLING-- */
	MPIR_ERR_SETANDSTMT(mpi_errno, MPI_ERR_ARG, break, "**typematchnoclass");
	/* --END ERROR HANDLING-- */
    }

    if (mpi_errno == MPI_SUCCESS) {
	if (matched_datatype == MPI_DATATYPE_NULL) {
	    /* --BEGIN ERROR HANDLING-- */
	    MPIR_ERR_SETANDSTMT2(mpi_errno, MPI_ERR_ARG,;, "**typematchsize", "**typematchsize %s %d", tname, size);
	    /* --END ERROR HANDLING-- */
	}
	else {
	    *datatype = matched_datatype;
	}
    }
    if (mpi_errno != MPI_SUCCESS) goto fn_fail;

    /* ... end of body of routine ... */

  fn_exit:
    MPIR_FUNC_TERSE_EXIT(MPID_STATE_MPI_TYPE_MATCH_SIZE);
    MPID_THREAD_CS_EXIT(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
#   ifdef HAVE_ERROR_CHECKING
    {
	mpi_errno = MPIR_Err_create_code(
	    mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**mpi_type_match_size",
	    "**mpi_type_match_size %d %d %p",typeclass, size, datatype);
    }
#   endif
    mpi_errno = MPIR_Err_return_comm( NULL, FCNAME, mpi_errno );
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}
