/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"
#include "topo.h"

/* -- Begin Profiling Symbol Block for routine MPI_Cart_rank */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Cart_rank = PMPI_Cart_rank
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Cart_rank  MPI_Cart_rank
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Cart_rank as PMPI_Cart_rank
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Cart_rank
#define MPI_Cart_rank PMPI_Cart_rank

#undef FUNCNAME
#define FUNCNAME MPIR_Cart_rank_impl
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
void MPIR_Cart_rank_impl(MPIR_Topology *cart_ptr, int *coords, int *rank)
{
    int i, ndims, coord, multiplier;

    ndims = cart_ptr->topo.cart.ndims;
    *rank = 0;
    multiplier = 1;
    for ( i=ndims-1; i >=0; i-- ) {
        coord = coords[i];
        if ( cart_ptr->topo.cart.periodic[i] ) {
            if (coord >= cart_ptr->topo.cart.dims[i])
                coord = coord % cart_ptr->topo.cart.dims[i];
            else if (coord <  0) {
                coord = coord % cart_ptr->topo.cart.dims[i];
                if (coord) coord = cart_ptr->topo.cart.dims[i] + coord;
            }
        }
        *rank += multiplier * coord;
        multiplier *= cart_ptr->topo.cart.dims[i];
    }
    return;
}


#endif

#undef FUNCNAME
#define FUNCNAME MPI_Cart_rank
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
/*@
MPI_Cart_rank - Determines process rank in communicator given Cartesian
                location

Input Parameters:
+ comm - communicator with cartesian structure (handle) 
- coords - integer array (of size 'ndims', the number of dimensions of
    the Cartesian topology associated with 'comm') specifying the cartesian 
  coordinates of a process 

Output Parameter:
. rank - rank of specified process (integer) 

Notes:
 Out-of-range coordinates are erroneous for non-periodic dimensions.  
 Versions of MPICH before 1.2.2 returned 'MPI_PROC_NULL' for the rank in this 
 case.

.N SignalSafe

.N Fortran

.N Errors
.N MPI_SUCCESS
.N MPI_ERR_TOPOLOGY
.N MPI_ERR_RANK
.N MPI_ERR_ARG
@*/
int MPI_Cart_rank(MPI_Comm comm, int *coords, int *rank)
{
    int mpi_errno = MPI_SUCCESS;
    MPID_Comm *comm_ptr = NULL;
    MPIR_Topology *cart_ptr;
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_CART_RANK);

    MPIR_ERRTEST_INITIALIZED_ORDIE();
    
    MPID_MPI_FUNC_ENTER(MPID_STATE_MPI_CART_RANK);

    /* Validate parameters, especially handles needing to be converted */
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
	    MPIR_ERRTEST_COMM(comm, mpi_errno);
            if (mpi_errno != MPI_SUCCESS) goto fn_fail;
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif
    
    /* Convert MPI object handles to object pointers */
    MPID_Comm_get_ptr( comm, comm_ptr );
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            /* Validate comm_ptr */
            MPID_Comm_valid_ptr( comm_ptr, mpi_errno );
	    /* If comm_ptr is not valid, it will be reset to null */
	    MPIR_ERRTEST_ARGNULL(rank,"rank",mpi_errno);
            if (mpi_errno) goto fn_fail;
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    cart_ptr = MPIR_Topology_get( comm_ptr );
    MPIU_ERR_CHKANDJUMP((!cart_ptr || cart_ptr->kind != MPI_CART), mpi_errno, MPI_ERR_TOPOLOGY, "**notcarttopo");

    /* Validate coordinates */
#   ifdef HAVE_ERROR_CHECKING
    {
        int i, ndims, coord;
        MPID_BEGIN_ERROR_CHECKS;
        {
	    ndims = cart_ptr->topo.cart.ndims;
	    if (ndims != 0) {
		MPIR_ERRTEST_ARGNULL(coords,"coords",mpi_errno);
                if (mpi_errno) MPIU_ERR_POP(mpi_errno);
	    }
	    for (i=0; i<ndims; i++) {
		if (!cart_ptr->topo.cart.periodic[i]) {
		    coord = coords[i];
		    MPIU_ERR_CHKANDJUMP3(
			(coord < 0 || coord >= cart_ptr->topo.cart.dims[i] ), mpi_errno, MPI_ERR_ARG, "**cartcoordinvalid",
			"**cartcoordinvalid %d %d %d",i, coords[i], cart_ptr->topo.cart.dims[i]-1 )
		}
	    }
	}
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */
    MPIR_Cart_rank_impl(cart_ptr, coords, rank);
    /* ... end of body of routine ... */

  fn_exit:
    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_CART_RANK);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
#   ifdef HAVE_ERROR_CHECKING
    {
	mpi_errno = MPIR_Err_create_code(
	    mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**mpi_cart_rank",
	    "**mpi_cart_rank %C %p %p", comm, coords, rank);
    }
#   endif
    mpi_errno = MPIR_Err_return_comm( comm_ptr, FCNAME, mpi_errno );
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}
