/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Type_get_true_extent */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Type_get_true_extent = PMPI_Type_get_true_extent
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Type_get_true_extent  MPI_Type_get_true_extent
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Type_get_true_extent as PMPI_Type_get_true_extent
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPI_Type_get_true_extent(MPI_Datatype datatype, MPI_Aint *true_lb, MPI_Aint *true_extent) __attribute__((weak,alias("PMPI_Type_get_true_extent")));
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Type_get_true_extent
#define MPI_Type_get_true_extent PMPI_Type_get_true_extent

#undef FUNCNAME
#define FUNCNAME MPIR_Type_get_true_extent_impl
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
void MPIR_Type_get_true_extent_impl(MPI_Datatype datatype, MPI_Aint *true_lb, MPI_Aint *true_extent)
{
    MPI_Count true_lb_x, true_extent_x;

    MPIR_Type_get_true_extent_x_impl(datatype, &true_lb_x, &true_extent_x);
    *true_lb = (true_lb_x > MPIR_AINT_MAX) ? MPI_UNDEFINED : (MPI_Aint)true_lb_x;
    *true_extent = (true_extent_x > MPIR_AINT_MAX) ? MPI_UNDEFINED : (MPI_Aint)true_extent_x;
}

#endif

#undef FUNCNAME
#define FUNCNAME MPI_Type_get_true_extent
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
/*@
   MPI_Type_get_true_extent - Get the true lower bound and extent for a
     datatype

Input Parameters:
. datatype - datatype to get information on (handle)

Output Parameters:
+ true_lb - true lower bound of datatype (address integer)
- true_extent - true size of datatype (address integer)

.N SignalSafe

.N Fortran

.N Errors
.N MPI_SUCCESS
.N MPI_ERR_TYPE
.N MPI_ERR_ARG
@*/
int MPI_Type_get_true_extent(MPI_Datatype datatype, MPI_Aint *true_lb,
			     MPI_Aint *true_extent)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_TERSE_STATE_DECL(MPID_STATE_MPI_TYPE_GET_TRUE_EXTENT);

    MPIR_ERRTEST_INITIALIZED_ORDIE();

    MPIR_FUNC_TERSE_ENTER(MPID_STATE_MPI_TYPE_GET_TRUE_EXTENT);

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
            MPIR_Datatype_get_ptr(datatype, datatype_ptr);

            /* Validate datatype_ptr */
            MPIR_Datatype_valid_ptr(datatype_ptr, mpi_errno);
            if (mpi_errno) goto fn_fail;
            MPIR_ERRTEST_ARGNULL(true_lb, "true_lb", mpi_errno);
            MPIR_ERRTEST_ARGNULL(true_extent, "true_extent", mpi_errno);
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */

    MPIR_Type_get_true_extent_impl(datatype, true_lb, true_extent);

    /* ... end of body of routine ... */

#ifdef HAVE_ERROR_CHECKING
  fn_exit:
#endif
    MPIR_FUNC_TERSE_EXIT(MPID_STATE_MPI_TYPE_GET_TRUE_EXTENT);
    return mpi_errno;

    /* --BEGIN ERROR HANDLING-- */
#   ifdef HAVE_ERROR_CHECKING
  fn_fail:
    {
	mpi_errno = MPIR_Err_create_code(
	    mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER,
	    "**mpi_type_get_true_extent",
	    "**mpi_type_get_true_extent %D %p %p",
	    datatype, true_lb, true_extent);
    }
    mpi_errno = MPIR_Err_return_comm(NULL, FCNAME, mpi_errno);
    goto fn_exit;
#   endif
    /* --END ERROR HANDLING-- */
}
