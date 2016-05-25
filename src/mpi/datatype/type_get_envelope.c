/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Type_get_envelope */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Type_get_envelope = PMPI_Type_get_envelope
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Type_get_envelope  MPI_Type_get_envelope
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Type_get_envelope as PMPI_Type_get_envelope
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPI_Type_get_envelope(MPI_Datatype datatype, int *num_integers, int *num_addresses,
                          int *num_datatypes, int *combiner) __attribute__((weak,alias("PMPI_Type_get_envelope")));
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Type_get_envelope
#define MPI_Type_get_envelope PMPI_Type_get_envelope

#undef FUNCNAME
#define FUNCNAME MPIR_Type_get_envelope
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
void MPIR_Type_get_envelope(MPI_Datatype datatype,
                                 int *num_integers,
                                 int *num_addresses,
                                 int *num_datatypes,
                                 int *combiner)
{
    if (HANDLE_GET_KIND(datatype) == HANDLE_KIND_BUILTIN ||
	datatype == MPI_FLOAT_INT ||
	datatype == MPI_DOUBLE_INT ||
	datatype == MPI_LONG_INT ||
	datatype == MPI_SHORT_INT ||
	datatype == MPI_LONG_DOUBLE_INT)
    {
	*combiner      = MPI_COMBINER_NAMED;
	*num_integers  = 0;
	*num_addresses = 0;
	*num_datatypes = 0;
    }
    else {
	MPIR_Datatype *dtp;

	MPIR_Datatype_get_ptr(datatype, dtp);

	*combiner      = dtp->contents->combiner;
	*num_integers  = dtp->contents->nr_ints;
	*num_addresses = dtp->contents->nr_aints;
	*num_datatypes = dtp->contents->nr_types;
    }

}

#endif

#undef FUNCNAME
#define FUNCNAME MPI_Type_get_envelope
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
/*@
   MPI_Type_get_envelope - get type envelope

Input Parameters:
.  datatype - datatype to access (handle)

Output Parameters:
+  num_integers - number of input integers used in the call constructing combiner (non-negative integer)
.  num_addresses - number of input addresses used in the call constructing combiner (non-negative integer)
.  num_datatypes - number of input datatypes used in the call constructing combiner (non-negative integer)
-  combiner - combiner (state)

   Notes:

.N Fortran

.N Errors
.N MPI_SUCCESS
@*/
int MPI_Type_get_envelope(MPI_Datatype datatype,
			  int *num_integers,
			  int *num_addresses,
			  int *num_datatypes,
			  int *combiner)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_TERSE_STATE_DECL(MPID_STATE_MPI_TYPE_GET_ENVELOPE);

    MPIR_ERRTEST_INITIALIZED_ORDIE();
    
    MPIR_FUNC_TERSE_ENTER(MPID_STATE_MPI_TYPE_GET_ENVELOPE);
    
    /* Validate parameters, especially handles needing to be converted */
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
	    MPIR_ERRTEST_DATATYPE(datatype, "datatype", mpi_errno);
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif
    
    /* Validate parameters and objects (post conversion) */
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            MPIR_Datatype *datatype_ptr = NULL;

            /* Convert MPI object handles to object pointers */
            MPIR_Datatype_get_ptr( datatype, datatype_ptr );

	    /* Validate datatype_ptr */
            MPIR_Datatype_valid_ptr(datatype_ptr, mpi_errno);
	    /* If comm_ptr is not value, it will be reset to null */
            if (mpi_errno != MPI_SUCCESS) goto fn_fail;
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */
    
    MPIR_Type_get_envelope(datatype,
                                num_integers,
                                num_addresses,
                                num_datatypes,
                                combiner);

    /* ... end of body of routine ... */

#   ifdef HAVE_ERROR_CHECKING
 fn_exit:
#   endif
    MPIR_FUNC_TERSE_EXIT(MPID_STATE_MPI_TYPE_GET_ENVELOPE);
    return mpi_errno;

#   ifdef HAVE_ERROR_CHECKING
 fn_fail:
    /* --BEGIN ERROR HANDLING-- */
    {
	mpi_errno = MPIR_Err_create_code(
	    mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**mpi_type_get_envelope",
	    "**mpi_type_get_envelope %D %p %p %p %p", datatype, num_integers, num_addresses, num_datatypes, combiner);
    }
    mpi_errno = MPIR_Err_return_comm( NULL, FCNAME, mpi_errno );
    goto fn_exit;
    /* --END ERROR HANDLING-- */
#   endif
}



