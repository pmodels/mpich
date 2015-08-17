/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2011 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Type_get_true_extent_x */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Type_get_true_extent_x = PMPI_Type_get_true_extent_x
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Type_get_true_extent_x  MPI_Type_get_true_extent_x
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Type_get_true_extent_x as PMPI_Type_get_true_extent_x
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPI_Type_get_true_extent_x(MPI_Datatype datatype, MPI_Count *lb, MPI_Count *extent) __attribute__((weak,alias("PMPI_Type_get_true_extent_x")));
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Type_get_true_extent_x
#define MPI_Type_get_true_extent_x PMPI_Type_get_true_extent_x

/* any non-MPI functions go here, especially non-static ones */

#undef FUNCNAME
#define FUNCNAME MPIR_Type_get_true_extent_x_impl
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
void MPIR_Type_get_true_extent_x_impl(MPI_Datatype datatype, MPI_Count *true_lb, MPI_Count *true_extent)
{
    MPID_Datatype *datatype_ptr = NULL;

    MPID_Datatype_get_ptr(datatype, datatype_ptr);

    if (HANDLE_GET_KIND(datatype) == HANDLE_KIND_BUILTIN) {
        *true_lb     = 0;
        *true_extent = MPID_Datatype_get_basic_size(datatype);
    }
    else {
        *true_lb     = datatype_ptr->true_lb;
        *true_extent = datatype_ptr->true_ub - datatype_ptr->true_lb;
    }
}

#endif /* MPICH_MPI_FROM_PMPI */

#undef FUNCNAME
#define FUNCNAME MPI_Type_get_true_extent_x
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
/*@
MPI_Type_get_true_extent_x - XXX description here

Input Parameters:
. datatype - datatype (handle)

Output Parameters:
+ true_lb - true lower bound of datatype (integer)
- true_extent - true extent of datatype (integer)

.N ThreadSafe

.N Fortran

.N Errors
@*/
int MPI_Type_get_true_extent_x(MPI_Datatype datatype, MPI_Count *true_lb, MPI_Count *true_extent)
{
    int mpi_errno = MPI_SUCCESS;
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_TYPE_GET_TRUE_EXTENT_X);

    MPID_THREAD_CS_ENTER(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    MPID_MPI_FUNC_ENTER(MPID_STATE_MPI_TYPE_GET_TRUE_EXTENT_X);

    /* Validate parameters, especially handles needing to be converted */
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS
        {
            MPIR_ERRTEST_DATATYPE(datatype, "datatype", mpi_errno);

            /* TODO more checks may be appropriate */
            if (mpi_errno != MPI_SUCCESS) goto fn_fail;
        }
        MPID_END_ERROR_CHECKS
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* Convert MPI object handles to object pointers */

    /* Validate parameters and objects (post conversion) */
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS
        {
            if (HANDLE_GET_KIND(datatype) != HANDLE_KIND_BUILTIN) {
                MPID_Datatype *datatype_ptr = NULL;
                MPID_Datatype_get_ptr(datatype, datatype_ptr);
                MPID_Datatype_valid_ptr(datatype_ptr, mpi_errno);
            }

            /* TODO more checks may be appropriate (counts, in_place, buffer aliasing, etc) */
            if (mpi_errno != MPI_SUCCESS) goto fn_fail;
        }
        MPID_END_ERROR_CHECKS
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */

    MPIR_Type_get_true_extent_x_impl(datatype, true_lb, true_extent);

    /* ... end of body of routine ... */

fn_exit:
    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_TYPE_GET_TRUE_EXTENT_X);
    MPID_THREAD_CS_EXIT(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    return mpi_errno;

fn_fail:
    /* --BEGIN ERROR HANDLING-- */
#   ifdef HAVE_ERROR_CHECKING
    {
        mpi_errno = MPIR_Err_create_code(
            mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER,
            "**mpi_type_get_true_extent_x", "**mpi_type_get_true_extent_x %D %p %p", datatype, true_lb, true_extent);
    }
#   endif
    mpi_errno = MPIR_Err_return_comm(NULL, FCNAME, mpi_errno);
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}
