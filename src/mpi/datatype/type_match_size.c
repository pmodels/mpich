/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
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
int MPI_Type_match_size(int typeclass, int size, MPI_Datatype * datatype)
    __attribute__ ((weak, alias("PMPI_Type_match_size")));
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Type_match_size
#define MPI_Type_match_size PMPI_Type_match_size

#endif

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
'(typeclass, size)'.

.N ThreadSafe

.N Fortran

.N Errors
.N MPI_SUCCESS
.N MPI_ERR_ARG
@*/
int MPI_Type_match_size(int typeclass, int size, MPI_Datatype * datatype)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_TERSE_STATE_DECL(MPID_STATE_MPI_TYPE_MATCH_SIZE);

    MPIR_ERRTEST_INITIALIZED_ORDIE();

    /* FIXME: This routine does not require the global critical section */
    MPID_THREAD_CS_ENTER(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    MPIR_FUNC_TERSE_ENTER(MPID_STATE_MPI_TYPE_MATCH_SIZE);

    /* Validate parameters and objects (post conversion) */
#ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            MPIR_ERRTEST_ARGNULL(datatype, "datatype", mpi_errno);
        }
        MPID_END_ERROR_CHECKS;
    }
#endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */

    mpi_errno = MPIR_Type_match_size_impl(typeclass, size, datatype);
    if (mpi_errno != MPI_SUCCESS)
        goto fn_fail;

    /* ... end of body of routine ... */

  fn_exit:
    MPIR_FUNC_TERSE_EXIT(MPID_STATE_MPI_TYPE_MATCH_SIZE);
    MPID_THREAD_CS_EXIT(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
#ifdef HAVE_ERROR_CHECKING
    {
        mpi_errno =
            MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, __func__, __LINE__, MPI_ERR_OTHER,
                                 "**mpi_type_match_size", "**mpi_type_match_size %d %d %p",
                                 typeclass, size, datatype);
    }
#endif
    mpi_errno = MPIR_Err_return_comm(NULL, __func__, mpi_errno);
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}
