/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Cart_create */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Cart_create = PMPI_Cart_create
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Cart_create  MPI_Cart_create
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Cart_create as PMPI_Cart_create
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPI_Cart_create(MPI_Comm comm_old, int ndims, const int dims[], const int periods[],
                    int reorder, MPI_Comm * comm_cart)
    __attribute__ ((weak, alias("PMPI_Cart_create")));
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Cart_create
#define MPI_Cart_create PMPI_Cart_create

#endif

/*@

MPI_Cart_create - Makes a new communicator to which topology information
                  has been attached

Input Parameters:
+ comm_old - input communicator (handle)
. ndims - number of dimensions of cartesian grid (integer)
. dims - integer array of size ndims specifying the number of processes in
  each dimension
. periods - logical array of size ndims specifying whether the grid is
  periodic (true) or not (false) in each dimension
- reorder - ranking may be reordered (true) or not (false) (logical)

Output Parameters:
. comm_cart - communicator with new cartesian topology (handle)

Algorithm:
We ignore 'reorder' info currently.

.N ThreadSafe

.N Fortran

.N Errors
.N MPI_SUCCESS
.N MPI_ERR_TOPOLOGY
.N MPI_ERR_DIMS
.N MPI_ERR_ARG
@*/
int MPI_Cart_create(MPI_Comm comm_old, int ndims, const int dims[],
                    const int periods[], int reorder, MPI_Comm * comm_cart)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Comm *comm_ptr = NULL;
    MPIR_FUNC_TERSE_STATE_DECL(MPID_STATE_MPI_CART_CREATE);

    MPIR_ERRTEST_INITIALIZED_ORDIE();

    MPID_THREAD_CS_ENTER(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    MPIR_FUNC_TERSE_ENTER(MPID_STATE_MPI_CART_CREATE);

    /* Validate parameters, especially handles needing to be converted */
#ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            MPIR_ERRTEST_COMM(comm_old, mpi_errno);
        }
        MPID_END_ERROR_CHECKS;
    }
#endif

    /* Convert MPI object handles to object pointers */
    MPIR_Comm_get_ptr(comm_old, comm_ptr);

    /* Validate parameters and objects (post conversion) */
#ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            /* Validate comm_ptr */
            MPIR_Comm_valid_ptr(comm_ptr, mpi_errno, FALSE);
            if (mpi_errno != MPI_SUCCESS)
                goto fn_fail;
            /* If comm_ptr is not valid, it will be reset to null */
            if (comm_ptr) {
                MPIR_ERRTEST_COMM_INTRA(comm_ptr, mpi_errno);
            }

            if (ndims > 0) {
                MPIR_ERRTEST_ARGNULL(dims, "dims", mpi_errno);
                MPIR_ERRTEST_ARGNULL(periods, "periods", mpi_errno);
            }
            MPIR_ERRTEST_ARGNULL(comm_cart, "comm_cart", mpi_errno);
            if (ndims < 0) {
                /* Must have a non-negative number of dimensions */
                mpi_errno = MPIR_Err_create_code(MPI_SUCCESS,
                                                 MPIR_ERR_RECOVERABLE, __func__, __LINE__,
                                                 MPI_ERR_DIMS, "**dims", "**dims %d", 0);
                goto fn_fail;
            }
            MPIR_ERRTEST_ARGNEG(ndims, "ndims", mpi_errno);
            if (comm_ptr) {
                MPIR_ERRTEST_COMM_INTRA(comm_ptr, mpi_errno);
            }
        }
        MPID_END_ERROR_CHECKS;
    }
#endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */

    mpi_errno = MPIR_Cart_create_impl(comm_ptr, ndims,
                                      (const int *) dims,
                                      (const int *) periods, reorder, comm_cart);
    if (mpi_errno)
        goto fn_fail;
    /* ... end of body of routine ... */

  fn_exit:
    MPIR_FUNC_TERSE_EXIT(MPID_STATE_MPI_CART_CREATE);
    MPID_THREAD_CS_EXIT(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
#ifdef HAVE_ERROR_CHECKING
    {
        mpi_errno =
            MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, __func__, __LINE__, MPI_ERR_OTHER,
                                 "**mpi_cart_create", "**mpi_cart_create %C %d %p %p %d %p",
                                 comm_old, ndims, dims, periods, reorder, comm_cart);
    }
#endif
    mpi_errno = MPIR_Err_return_comm(comm_ptr, __func__, mpi_errno);
    /* --END ERROR HANDLING-- */
    goto fn_exit;
}
