/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Get_elements_x */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Get_elements_x = PMPI_Get_elements_x
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Get_elements_x  MPI_Get_elements_x
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Get_elements_x as PMPI_Get_elements_x
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPI_Get_elements_x(const MPI_Status * status, MPI_Datatype datatype, MPI_Count * count)
    __attribute__ ((weak, alias("PMPI_Get_elements_x")));
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Get_elements_x
#define MPI_Get_elements_x PMPI_Get_elements_x
#endif /* MPICH_MPI_FROM_PMPI */

/* N.B. "count" is the name mandated by the MPI-3 standard, but it should
 * probably be called "elements" instead and is handled that way in the _impl
 * routine [goodell@ 2012-11-05 */
/*@
MPI_Get_elements_x - Returns the number of basic elements
                     in a datatype

Input Parameters:
+ status - return status of receive operation (Status)
- datatype - datatype used by receive operation (handle)

Output Parameters:
. count - number of received basic elements (integer)

.N ThreadSafe

.N Fortran

.N Errors
@*/
int MPI_Get_elements_x(const MPI_Status * status, MPI_Datatype datatype, MPI_Count * count)
{
    int mpi_errno = MPI_SUCCESS;
    MPI_Count byte_count;
    MPIR_FUNC_TERSE_STATE_DECL(MPID_STATE_MPI_GET_ELEMENTS_X);

    MPID_THREAD_CS_ENTER(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    MPIR_FUNC_TERSE_ENTER(MPID_STATE_MPI_GET_ELEMENTS_X);

    /* Validate parameters, especially handles needing to be converted */
#ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            MPIR_ERRTEST_DATATYPE(datatype, "datatype", mpi_errno);

            /* TODO more checks may be appropriate */
            if (mpi_errno != MPI_SUCCESS)
                goto fn_fail;
        }
        MPID_END_ERROR_CHECKS;
    }
#endif /* HAVE_ERROR_CHECKING */

    /* Convert MPI object handles to object pointers */

    /* Validate parameters and objects (post conversion) */
#ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            if (!HANDLE_IS_BUILTIN(datatype)) {
                MPIR_Datatype *datatype_ptr = NULL;
                MPIR_Datatype_get_ptr(datatype, datatype_ptr);
                MPIR_Datatype_valid_ptr(datatype_ptr, mpi_errno);
                MPIR_Datatype_committed_ptr(datatype_ptr, mpi_errno);
            }

            /* TODO more checks may be appropriate (counts, in_place, buffer aliasing, etc) */
            if (mpi_errno != MPI_SUCCESS)
                goto fn_fail;
        }
        MPID_END_ERROR_CHECKS;
    }
#endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */

    byte_count = MPIR_STATUS_GET_COUNT(*status);
    mpi_errno = MPIR_Get_elements_x_impl(&byte_count, datatype, count);
    MPIR_ERR_CHECK(mpi_errno);

    /* ... end of body of routine ... */

  fn_exit:
    MPIR_FUNC_TERSE_EXIT(MPID_STATE_MPI_GET_ELEMENTS_X);
    MPID_THREAD_CS_EXIT(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
#ifdef HAVE_ERROR_CHECKING
    {
        mpi_errno =
            MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, __func__, __LINE__, MPI_ERR_OTHER,
                                 "**mpi_get_elements_x", "**mpi_get_elements_x %p %D %p", status,
                                 datatype, count);
    }
#endif
    mpi_errno = MPIR_Err_return_comm(NULL, __func__, mpi_errno);
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}
