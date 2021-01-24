/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Cart_map */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Cart_map = PMPI_Cart_map
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Cart_map  MPI_Cart_map
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Cart_map as PMPI_Cart_map
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPI_Cart_map(MPI_Comm comm, int ndims, const int dims[], const int periods[], int *newrank)
    __attribute__ ((weak, alias("PMPI_Cart_map")));
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Cart_map
#define MPI_Cart_map PMPI_Cart_map
#endif

/*@
MPI_Cart_map - Maps process to Cartesian topology information

Input Parameters:
+ comm - input communicator (handle)
. ndims - number of dimensions of Cartesian structure (integer)
. dims - integer array of size 'ndims' specifying the number of processes in
  each coordinate direction
- periods - logical array of size 'ndims' specifying the periodicity
  specification in each coordinate direction

Output Parameters:
. newrank - reordered rank of the calling process; 'MPI_UNDEFINED' if
  calling process does not belong to grid (integer)

.N SignalSafe

.N Fortran

.N Errors
.N MPI_SUCCESS
.N MPI_ERR_COMM
.N MPI_ERR_DIMS
.N MPI_ERR_ARG
@*/
int MPI_Cart_map(MPI_Comm comm, int ndims, const int dims[], const int periods[], int *newrank)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Comm *comm_ptr = NULL;
    MPIR_FUNC_TERSE_STATE_DECL(MPID_STATE_MPI_CART_MAP);

    MPIR_ERRTEST_INITIALIZED_ORDIE();

    MPIR_FUNC_TERSE_ENTER(MPID_STATE_MPI_CART_MAP);

    /* Validate parameters, especially handles needing to be converted */
#ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            MPIR_ERRTEST_COMM(comm, mpi_errno);
        }
        MPID_END_ERROR_CHECKS;
    }
#endif

    /* Convert MPI object handles to object pointers */
    MPIR_Comm_get_ptr(comm, comm_ptr);

    /* Validate parameters and objects (post conversion) */
#ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            /* Validate comm_ptr */
            MPIR_Comm_valid_ptr(comm_ptr, mpi_errno, TRUE);
            if (mpi_errno)
                goto fn_fail;
            /* If comm_ptr is not valid, it will be reset to null */
            MPIR_ERRTEST_ARGNULL(newrank, "newrank", mpi_errno);
            MPIR_ERRTEST_ARGNULL(dims, "dims", mpi_errno);
            /* As of MPI 2.1, 0-dimensional cartesian topologies are valid */
            if (ndims < 0) {
                mpi_errno = MPIR_Err_create_code(MPI_SUCCESS,
                                                 MPIR_ERR_RECOVERABLE,
                                                 __func__, __LINE__, MPI_ERR_DIMS,
                                                 "**dims", "**dims %d", ndims);
                goto fn_fail;
            }
        }
        MPID_END_ERROR_CHECKS;
    }
#endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */
    if (comm_ptr->topo_fns != NULL && comm_ptr->topo_fns->cartMap != NULL) {
        /* --BEGIN USEREXTENSION-- */
        mpi_errno = comm_ptr->topo_fns->cartMap(comm_ptr, ndims, dims, periods, newrank);
        /* --END USEREXTENSION-- */
    } else {
        mpi_errno = MPIR_Cart_map_impl(comm_ptr, ndims, dims, periods, newrank);
    }
    if (mpi_errno) {
        goto fn_fail;
    }
    /* ... end of body of routine ... */

  fn_exit:
    MPIR_FUNC_TERSE_EXIT(MPID_STATE_MPI_CART_MAP);
    return mpi_errno;

    /* --BEGIN ERROR HANDLING-- */
  fn_fail:
#ifdef HAVE_ERROR_CHECKING
    {
        mpi_errno =
            MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, __func__, __LINE__, MPI_ERR_OTHER,
                                 "**mpi_cart_map", "**mpi_cart_map %C %d %p %p %p", comm, ndims,
                                 dims, periods, newrank);
    }
#endif
    mpi_errno = MPIR_Err_return_comm(comm_ptr, __func__, mpi_errno);
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}
