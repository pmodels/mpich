/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"
#include "topo.h"

/* -- Begin Profiling Symbol Block for routine MPI_Cart_shift */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Cart_shift = PMPI_Cart_shift
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Cart_shift  MPI_Cart_shift
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Cart_shift as PMPI_Cart_shift
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Cart_shift
#define MPI_Cart_shift PMPI_Cart_shift

#endif

#undef FUNCNAME
#define FUNCNAME MPI_Cart_shift
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
/*@
MPI_Cart_shift - Returns the shifted source and destination ranks, given a 
                 shift direction and amount

Input Parameters:
+ comm - communicator with cartesian structure (handle) 
. direction - coordinate dimension of shift (integer) 
- displ - displacement (> 0: upwards shift, < 0: downwards shift) (integer) 

Output Parameters:
+ source - rank of source process (integer) 
- dest - rank of destination process (integer) 

Notes:
The 'direction' argument is in the range '[0,n-1]' for an n-dimensional 
Cartesian mesh.

.N SignalSafe

.N Fortran

.N Errors
.N MPI_SUCCESS
.N MPI_ERR_TOPOLOGY
.N MPI_ERR_COMM
.N MPI_ERR_ARG
@*/
int MPI_Cart_shift(MPI_Comm comm, int direction, int displ, int *source, 
		   int *dest)
{
    int mpi_errno = MPI_SUCCESS;
    MPID_Comm *comm_ptr = NULL;
    MPIR_Topology *cart_ptr;
    int i;
    int pos[MAX_CART_DIM];
    int rank;
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_CART_SHIFT);

    MPIR_ERRTEST_INITIALIZED_ORDIE();
    
    MPID_MPI_FUNC_ENTER(MPID_STATE_MPI_CART_SHIFT);

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

    /* Validate parameters and objects (post conversion) */
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            /* Validate comm_ptr */
            MPID_Comm_valid_ptr( comm_ptr, mpi_errno );
	    /* If comm_ptr is not valid, it will be reset to null */

	    MPIR_ERRTEST_ARGNULL( source, "source", mpi_errno );
	    MPIR_ERRTEST_ARGNULL( dest, "dest", mpi_errno );
	    MPIR_ERRTEST_ARGNEG( direction, "direction", mpi_errno );
	    /* Nothing in the standard indicates that a zero displacement 
	       is not valid, so we don't check for a zero shift */
            if (mpi_errno) goto fn_fail;
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */
    
    cart_ptr = MPIR_Topology_get( comm_ptr );

    MPIU_ERR_CHKANDJUMP((!cart_ptr || cart_ptr->kind != MPI_CART), mpi_errno, MPI_ERR_TOPOLOGY, "**notcarttopo");
    MPIU_ERR_CHKANDJUMP((cart_ptr->topo.cart.ndims == 0), mpi_errno, MPI_ERR_TOPOLOGY, "**dimszero");
    MPIU_ERR_CHKANDJUMP2((direction >= cart_ptr->topo.cart.ndims), mpi_errno, MPI_ERR_ARG, "**dimsmany",
			 "**dimsmany %d %d", cart_ptr->topo.cart.ndims, direction);

    /* Check for the case of a 0 displacement */
    rank = comm_ptr->rank;
    if (displ == 0) {
	*source = *dest = rank;
    }
    else {
	/* To support advanced implementations that support MPI_Cart_create,
	   we compute the new position and call PMPI_Cart_rank to get the
	   source and destination.  We could bypass that step if we know that
	   the mapping is trivial.  Copy the current position. */
	for (i=0; i<cart_ptr->topo.cart.ndims; i++) {
	    pos[i] = cart_ptr->topo.cart.position[i];
	}
	/* We must return MPI_PROC_NULL if shifted over the edge of a 
	   non-periodic mesh */
	pos[direction] += displ;
	if (!cart_ptr->topo.cart.periodic[direction] &&
	    (pos[direction] >= cart_ptr->topo.cart.dims[direction] ||
	     pos[direction] < 0)) {
	    *dest = MPI_PROC_NULL;
	}
	else {
	    MPIR_Cart_rank_impl( cart_ptr, pos, dest );
	}

	pos[direction] = cart_ptr->topo.cart.position[direction] - displ;
	if (!cart_ptr->topo.cart.periodic[direction] &&
	    (pos[direction] >= cart_ptr->topo.cart.dims[direction] ||
	     pos[direction] < 0)) {
	    *source = MPI_PROC_NULL;
	}
	else {
	    MPIR_Cart_rank_impl( cart_ptr, pos, source );
	}
    }

    /* ... end of body of routine ... */

  fn_exit:
    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_CART_SHIFT);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
#   ifdef HAVE_ERROR_CHECKING
    {
	mpi_errno = MPIR_Err_create_code(
	    mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**mpi_cart_shift",
	    "**mpi_cart_shift %C %d %d %p %p", comm, direction, displ, source, dest);
    }
#   endif
    mpi_errno = MPIR_Err_return_comm( comm_ptr, FCNAME, mpi_errno );
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}
