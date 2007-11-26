/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*  
 *  (C) 2004 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "create_f90_util.h"

#include "mpiimpl.h"
#ifdef HAVE_F90_TYPE_ROUTINES
#include "mpif90model.h"
#else
/* Assume only 4 byte integers available */
#define MPIR_F90_INTEGER_MODEL_MAP { 9, 4, 4 }
#endif

/* -- Begin Profiling Symbol Block for routine MPI_Type_create_f90_integer */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Type_create_f90_integer = PMPI_Type_create_f90_integer
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Type_create_f90_integer  MPI_Type_create_f90_integer
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Type_create_f90_integer as PMPI_Type_create_f90_integer
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines.  You can use USE_WEAK_SYMBOLS to see if MPICH is
   using weak symbols to implement the MPI routines. */
#ifndef MPICH_MPI_FROM_PMPI
#define MPI_Type_create_f90_integer PMPI_Type_create_f90_integer

#endif

typedef struct intModel {
    int range, kind, bytes; } intModel;

#undef FUNCNAME
#define FUNCNAME MPI_Type_create_f90_integer

/*@
   MPI_Type_create_f90_integer - Return a predefined type that matches 
   the specified range

   Input Arguments:
.  range - Decimal range (number of digits) desired

   Output Arguments:
.  newtype - A predefine MPI Datatype that matches the range.

   Notes:
If there is no corresponding type for the specified range, the call is 
erroneous.  This implementation sets 'newtype' to 'MPI_DATATYPE_NULL' and
returns an error of class 'MPI_ERR_ARG'.

.N Fortran

.N Errors
.N MPI_SUCCESS
.N MPI_ERR_ARG
@*/
int MPI_Type_create_f90_integer( int range, MPI_Datatype *newtype )
{
    static const char FCNAME[] = "MPI_Type_create_f90_integer";
    int i, bytes;
    int mpi_errno = MPI_SUCCESS;
    MPI_Datatype basetype = MPI_DATATYPE_NULL;
    static intModel f90_integer_map[] = { MPIR_F90_INTEGER_MODEL_MAP
					  {0, 0, 0 } };
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_TYPE_CREATE_F90_INTEGER);

    MPID_MPI_FUNC_ENTER(MPID_STATE_MPI_TYPE_CREATE_F90_INTEGER);
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            MPIR_ERRTEST_INITIALIZED(mpi_errno);
	    if (mpi_errno) {
		return MPIR_Err_return_comm( 0, FCNAME, mpi_errno );
	    }
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */
    for (i=0; f90_integer_map[i].range > 0; i++) {
	if (f90_integer_map[i].range >= range) {
	    /* Find the corresponding INTEGER type */
	    bytes = f90_integer_map[i].bytes;
	    switch (bytes) {
	    case 1: basetype = MPI_INTEGER1; break;
	    case 2: basetype = MPI_INTEGER2; break;
	    case 4: basetype = MPI_INTEGER4; break;
	    case 8: basetype = MPI_INTEGER8; break;
	    default: break;
	    }
	    break;
	}
    }

    /* FIXME: Check on action if no match found */
    if (basetype == MPI_DATATYPE_NULL) {
	mpi_errno = MPIR_Err_create_code( MPI_SUCCESS, MPIR_ERR_RECOVERABLE,
					  "MPI_Type_create_f90_integer",
					  __LINE__, MPI_ERR_OTHER,
 					  "**f90typeintnone", 
					  "**f90typeintnone %d", range );
    }
    else {
	mpi_errno = MPIR_Create_unnamed_predefined( basetype, 
			    MPI_COMBINER_F90_INTEGER, range, -1, newtype );
	if (mpi_errno) {
	    mpi_errno = MPIR_Err_create_code( MPI_SUCCESS, 
					      MPIR_ERR_RECOVERABLE, 
				     "MPI_Type_create_f90_integer", __LINE__,
				      MPI_ERR_INTERN, "**f90typetoomany", 0 );
	}
    }

    /* ... end of body of routine ... */

    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_TYPE_CREATE_F90_INTEGER);
    return mpi_errno;
}
