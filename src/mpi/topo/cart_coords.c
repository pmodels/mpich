/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Cart_coords */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Cart_coords = PMPI_Cart_coords
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Cart_coords  MPI_Cart_coords
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Cart_coords as PMPI_Cart_coords
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPI_Cart_coords(MPI_Comm comm, int rank, int maxdims, int coords[])
    __attribute__ ((weak, alias("PMPI_Cart_coords")));
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Cart_coords
#define MPI_Cart_coords PMPI_Cart_coords

#endif

#undef FUNCNAME
#define FUNCNAME MPI_Cart_coords
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
/*@
MPI_Cart_coords - Determines process coords in cartesian topology given
                  rank in group

Input Parameters:
+ comm - communicator with cartesian structure (handle)
. rank - rank of a process within group of 'comm' (integer)
- maxdims - length of vector 'coords' in the calling program (integer)

Output Parameters:
. coords - integer array (of size 'ndims') containing the Cartesian
  coordinates of specified process (integer)

.N SignalSafe

.N Fortran

.N Errors
.N MPI_SUCCESS
.N MPI_ERR_TOPOLOGY
.N MPI_ERR_RANK
.N MPI_ERR_DIMS
.N MPI_ERR_ARG
@*/
int MPI_Cart_coords(MPI_Comm comm, int rank, int maxdims, int coords[])
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Comm *comm_ptr = NULL;
    MPIR_Topology *cart_ptr;
    int i, nnodes;
    MPIR_FUNC_TERSE_STATE_DECL(MPID_STATE_MPI_CART_COORDS);

    MPIR_ERRTEST_INITIALIZED_ORDIE();

    MPIR_FUNC_TERSE_ENTER(MPID_STATE_MPI_CART_COORDS);

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
            /* If comm_ptr is not valid, it will be reset to null */
            if (mpi_errno != MPI_SUCCESS)
                goto fn_fail;

            MPIR_ERRTEST_RANK(comm_ptr, rank, mpi_errno);
        }
        MPID_END_ERROR_CHECKS;
    }
#endif /* HAVE_ERROR_CHECKING */

    cart_ptr = MPIR_Topology_get(comm_ptr);

#ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            MPIR_ERR_CHKANDJUMP((!cart_ptr ||
                                 cart_ptr->kind != MPI_CART), mpi_errno, MPI_ERR_TOPOLOGY,
                                "**notcarttopo");
            MPIR_ERR_CHKANDJUMP2((cart_ptr->topo.cart.ndims > maxdims), mpi_errno, MPI_ERR_ARG,
                                 "**dimsmany", "**dimsmany %d %d", cart_ptr->topo.cart.ndims,
                                 maxdims);
            if (cart_ptr->topo.cart.ndims) {
                MPIR_ERRTEST_ARGNULL(coords, "coords", mpi_errno);
            }
        }
        MPID_END_ERROR_CHECKS;
    }
#endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */

    /* Calculate coords */
    nnodes = cart_ptr->topo.cart.nnodes;
    for (i = 0; i < cart_ptr->topo.cart.ndims; i++) {
        nnodes = nnodes / cart_ptr->topo.cart.dims[i];
        coords[i] = rank / nnodes;
        rank = rank % nnodes;
    }

    /* ... end of body of routine ... */

#ifdef HAVE_ERROR_CHECKING
  fn_exit:
#endif
    MPIR_FUNC_TERSE_EXIT(MPID_STATE_MPI_CART_COORDS);
    return mpi_errno;

#ifdef HAVE_ERROR_CHECKING
  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
    {
        mpi_errno =
            MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER,
                                 "**mpi_cart_coords", "**mpi_cart_coords %C %d %d %p", comm, rank,
                                 maxdims, coords);
    }
    mpi_errno = MPIR_Err_return_comm(comm_ptr, FCNAME, mpi_errno);
    goto fn_exit;
    /* --END ERROR HANDLING-- */
#endif
}
