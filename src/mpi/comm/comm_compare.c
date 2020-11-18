/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Comm_compare */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Comm_compare = PMPI_Comm_compare
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Comm_compare  MPI_Comm_compare
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Comm_compare as PMPI_Comm_compare
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPI_Comm_compare(MPI_Comm comm1, MPI_Comm comm2, int *result)
    __attribute__ ((weak, alias("PMPI_Comm_compare")));
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Comm_compare
#define MPI_Comm_compare PMPI_Comm_compare
#endif


/*@

MPI_Comm_compare - Compares two communicators

Input Parameters:
+ comm1 - comm1 (handle)
- comm2 - comm2 (handle)

Output Parameters:
. result - integer which is 'MPI_IDENT' if the contexts and groups are the
same, 'MPI_CONGRUENT' if different contexts but identical groups, 'MPI_SIMILAR'
if different contexts but similar groups, and 'MPI_UNEQUAL' otherwise

Using 'MPI_COMM_NULL' with 'MPI_Comm_compare':

It is an error to use 'MPI_COMM_NULL' as one of the arguments to
'MPI_Comm_compare'.  The relevant sections of the MPI standard are

$(2.4.1 Opaque Objects)
A null handle argument is an erroneous 'IN' argument in MPI calls, unless an
exception is explicitly stated in the text that defines the function.

$(5.4.1. Communicator Accessors)
where there is no text in 'MPI_COMM_COMPARE' allowing a null handle.

.N ThreadSafe
(To perform the communicator comparisions, this routine may need to
allocate some memory.  Memory allocation is not interrupt-safe, and hence
this routine is only thread-safe.)

.N Fortran

.N Errors
.N MPI_SUCCESS
.N MPI_ERR_ARG
@*/
int MPI_Comm_compare(MPI_Comm comm1, MPI_Comm comm2, int *result)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Comm *comm_ptr1 = NULL;
    MPIR_Comm *comm_ptr2 = NULL;
    MPIR_FUNC_TERSE_STATE_DECL(MPID_STATE_MPI_COMM_COMPARE);

    MPIR_ERRTEST_INITIALIZED_ORDIE();

    MPID_THREAD_CS_ENTER(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    MPIR_FUNC_TERSE_ENTER(MPID_STATE_MPI_COMM_COMPARE);

#ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            MPIR_ERRTEST_COMM(comm1, mpi_errno);
            MPIR_ERRTEST_COMM(comm2, mpi_errno);
        }
        MPID_END_ERROR_CHECKS;
    }
#endif /* HAVE_ERROR_CHECKING */

    /* Get handles to MPI objects. */
    MPIR_Comm_get_ptr(comm1, comm_ptr1);
    MPIR_Comm_get_ptr(comm2, comm_ptr2);

    /* Validate parameters and objects (post conversion) */
#ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            /* Validate comm_ptr */
            MPIR_Comm_valid_ptr(comm_ptr1, mpi_errno, TRUE);
            if (mpi_errno)
                goto fn_fail;
            MPIR_Comm_valid_ptr(comm_ptr2, mpi_errno, TRUE);
            if (mpi_errno)
                goto fn_fail;
            MPIR_ERRTEST_ARGNULL(result, "result", mpi_errno);
        }
        MPID_END_ERROR_CHECKS;
    }
#endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */
    mpi_errno = MPIR_Comm_compare_impl(comm_ptr1, comm_ptr2, result);
    MPIR_ERR_CHECK(mpi_errno);
    /* ... end of body of routine ... */

  fn_exit:
    MPIR_FUNC_TERSE_EXIT(MPID_STATE_MPI_COMM_COMPARE);
    MPID_THREAD_CS_EXIT(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    return mpi_errno;

    /* --BEGIN ERROR HANDLING-- */
  fn_fail:
#ifdef HAVE_ERROR_CHECKING
    {
        mpi_errno =
            MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, __func__, __LINE__, MPI_ERR_OTHER,
                                 "**mpi_comm_compare", "**mpi_comm_compare %C %C %p", comm1, comm2,
                                 result);
    }
    /* Use whichever communicator is non-null if possible */
#endif
    mpi_errno = MPIR_Err_return_comm(comm_ptr1 ? comm_ptr1 : comm_ptr2, __func__, mpi_errno);
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}
