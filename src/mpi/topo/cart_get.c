/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"
#include "topo.h"

/* -- Begin Profiling Symbol Block for routine MPI_Cart_get */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Cart_get = PMPI_Cart_get
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Cart_get  MPI_Cart_get
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Cart_get as PMPI_Cart_get
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Cart_get
#define MPI_Cart_get PMPI_Cart_get

#endif

#undef FUNCNAME
#define FUNCNAME MPI_Cart_get

/*@

MPI_Cart_get - Retrieves Cartesian topology information associated with a 
               communicator

Input Parameters:
+ comm - communicator with cartesian structure (handle) 
- maxdims - length of vectors  'dims', 'periods', and 'coords'
in the calling program (integer) 

Output Parameters:
+ dims - number of processes for each cartesian dimension (array of integer) 
. periods - periodicity (true/false) for each cartesian dimension 
(array of logical) 
- coords - coordinates of calling process in cartesian structure 
(array of integer) 

.N ThreadSafe

.N Fortran

.N Errors
.N MPI_SUCCESS
.N MPI_ERR_TOPOLOGY
.N MPI_ERR_COMM
.N MPI_ERR_ARG
@*/
int MPI_Cart_get(MPI_Comm comm, int maxdims, int *dims, int *periods, 
                 int *coords)
{
    static const char FCNAME[] = "MPI_Cart_get";
    int mpi_errno = MPI_SUCCESS;
    MPID_Comm *comm_ptr = NULL;
    MPIR_Topology *cart_ptr;
    int i, n, *vals;
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_CART_GET);

    MPIR_ERRTEST_INITIALIZED_ORDIE();
    
    MPID_MPI_FUNC_ENTER(MPID_STATE_MPI_CART_GET);
    
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
            if (mpi_errno) goto fn_fail;
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */
    
    cart_ptr = MPIR_Topology_get( comm_ptr );

#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
	    MPIU_ERR_CHKANDJUMP((!cart_ptr || cart_ptr->kind != MPI_CART), mpi_errno, MPI_ERR_TOPOLOGY, "**notcarttopo");
	    MPIU_ERR_CHKANDJUMP2((cart_ptr->topo.cart.ndims > maxdims), mpi_errno, MPI_ERR_ARG, "**dimsmany",
			 "**dimsmany %d %d", cart_ptr->topo.cart.ndims, maxdims);

	    if (cart_ptr->topo.cart.ndims) {
		MPIR_ERRTEST_ARGNULL( dims, "dims", mpi_errno );
		MPIR_ERRTEST_ARGNULL( periods, "periods", mpi_errno );
		MPIR_ERRTEST_ARGNULL( coords, "coords", mpi_errno ); 
	    }
            if (mpi_errno) goto fn_fail;
	}
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    n = cart_ptr->topo.cart.ndims;

    vals = cart_ptr->topo.cart.dims;
    for ( i=0; i<n; i++ )
	*dims++ = *vals++;
    
    /* Get periods */
    vals = cart_ptr->topo.cart.periodic;
    for ( i=0; i<n; i++ )
	*periods++ = *vals++;
    
    /* Get coords */
    vals = cart_ptr->topo.cart.position;
    for ( i=0; i<n; i++ )
	*coords++ = *vals++;
    
    /* ... end of body of routine ... */

  fn_exit:
    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_CART_GET);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
#   ifdef HAVE_ERROR_CHECKING
    {
	mpi_errno = MPIR_Err_create_code(
	    mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**mpi_cart_get",
	    "**mpi_cart_get %C %d %p %p %p", comm, maxdims, dims, periods, coords);
    }
#   endif
    mpi_errno = MPIR_Err_return_comm( comm_ptr, FCNAME, mpi_errno );
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}
