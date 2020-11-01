/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"

/*
 * This CVAR is used for debugging support.  An alternative would be
 * to use the MPIU_DBG interface, which predates the CVAR interface,
 * and also provides different levels of debugging support.  In the
 * long run, the MPIU_DBG interface should be updated to make use of
 * CVARs.
 */

/* -- Begin Profiling Symbol Block for routine MPI_Dims_create */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Dims_create = PMPI_Dims_create
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Dims_create  MPI_Dims_create
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Dims_create as PMPI_Dims_create
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPI_Dims_create(int nnodes, int ndims, int dims[])
    __attribute__ ((weak, alias("PMPI_Dims_create")));
#endif
/* -- End Profiling Symbol Block */



#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Dims_create
#define MPI_Dims_create PMPI_Dims_create
#endif /* PMPI Local */


/*@
    MPI_Dims_create - Creates a division of processors in a cartesian grid

Input Parameters:
+ nnodes - number of nodes in a grid (integer)
- ndims - number of cartesian dimensions (integer)

Input/Output Parameters:
. dims - integer array of size  'ndims' specifying the number of nodes in each
 dimension.  A value of 0 indicates that 'MPI_Dims_create' should fill in a
 suitable value.

.N ThreadSafe

.N Fortran

.N Errors
.N MPI_SUCCESS
@*/
int MPI_Dims_create(int nnodes, int ndims, int dims[])
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_TERSE_STATE_DECL(MPID_STATE_MPI_DIMS_CREATE);

    MPIR_ERRTEST_INITIALIZED_ORDIE();
    MPIR_FUNC_TERSE_ENTER(MPID_STATE_MPI_DIMS_CREATE);

    if (ndims == 0)
        goto fn_exit;

    /* Validate parameters and objects (post conversion) */
#ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            MPIR_ERRTEST_ARGNEG(nnodes, "nnodes", mpi_errno);
            MPIR_ERRTEST_ARGNEG(ndims, "ndims", mpi_errno);
            MPIR_ERRTEST_ARGNULL(dims, "dims", mpi_errno);
        }
        MPID_END_ERROR_CHECKS;
    }
#endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */
    mpi_errno = MPIR_Dims_create_impl(nnodes, ndims, dims);
    if (mpi_errno) {
        goto fn_fail;
    }
    /* ... end of body of routine ... */

  fn_exit:
    MPIR_FUNC_TERSE_EXIT(MPID_STATE_MPI_DIMS_CREATE);
    return mpi_errno;

    /* --BEGIN ERROR HANDLING-- */
  fn_fail:
#ifdef HAVE_ERROR_CHECKING
    {
        mpi_errno =
            MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, __func__, __LINE__, MPI_ERR_OTHER,
                                 "**mpi_dims_create", "**mpi_dims_create %d %d %p", nnodes, ndims,
                                 dims);
    }
#endif
    mpi_errno = MPIR_Err_return_comm(NULL, __func__, mpi_errno);
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}
