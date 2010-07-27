/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"
#include "topo.h"

/* -- Begin Profiling Symbol Block for routine MPI_Cart_map */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Cart_map = PMPI_Cart_map
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Cart_map  MPI_Cart_map
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Cart_map as PMPI_Cart_map
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Cart_map
#define MPI_Cart_map PMPI_Cart_map

#undef FUNCNAME
#define FUNCNAME MPIR_Cart_map
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
int MPIR_Cart_map( const MPID_Comm *comm_ptr, int ndims, const int dims[], 
		   const int periodic[], int *newrank )
{
    int rank, nranks, i, size, mpi_errno = MPI_SUCCESS;

    MPIU_UNREFERENCED_ARG(periodic);

    /* Determine number of processes needed for topology */
    if (ndims == 0) {
	nranks = 1;
    } else {
	nranks = dims[0];
	for ( i=1; i<ndims; i++ )
	    nranks *= dims[i];
    }
    size = comm_ptr->remote_size;
    
    /* Test that the communicator is large enough */
    MPIU_ERR_CHKANDJUMP2(size < nranks, mpi_errno, MPI_ERR_DIMS, "**topotoolarge",
                         "**topotoolarge %d %d", size, nranks);

    /* Am I in this range? */
    rank = comm_ptr->rank;
    if ( rank < nranks )
	/* This relies on the ranks *not* being reordered by the current
	   Cartesian routines */
	*newrank = rank;
    else
	*newrank = MPI_UNDEFINED;

 fn_exit:
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIR_Cart_map_impl
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
int MPIR_Cart_map_impl(const MPID_Comm *comm_ptr, int ndims, const int dims[],
                       const int periods[], int *newrank)
{
    int mpi_errno = MPI_SUCCESS;
        
    if (comm_ptr->topo_fns != NULL && comm_ptr->topo_fns->cartMap != NULL) {
	mpi_errno = comm_ptr->topo_fns->cartMap( comm_ptr, ndims,
						 (const int*) dims,
						 (const int*) periods,
						 newrank );
        if (mpi_errno) MPIU_ERR_POP(mpi_errno);
    } else {
	mpi_errno = MPIR_Cart_map( comm_ptr, ndims,
				   (const int*) dims,
				   (const int*) periods, newrank );
        if (mpi_errno) MPIU_ERR_POP(mpi_errno);
    }

 fn_exit:
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}

#endif

#undef FUNCNAME
#define FUNCNAME MPI_Cart_map
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
/*@
MPI_Cart_map - Maps process to Cartesian topology information 

Input Parameters:
+ comm - input communicator (handle) 
. ndims - number of dimensions of Cartesian structure (integer) 
. dims - integer array of size 'ndims' specifying the number of processes in 
  each coordinate direction 
- periods - logical array of size 'ndims' specifying the periodicity 
  specification in each coordinate direction 

Output Parameter:
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
int MPI_Cart_map(MPI_Comm comm_old, int ndims, int *dims, int *periods, 
		 int *newrank)
{
    int mpi_errno = MPI_SUCCESS;
    MPID_Comm *comm_ptr = NULL;
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_CART_MAP);

    MPIR_ERRTEST_INITIALIZED_ORDIE();
    
    MPID_MPI_FUNC_ENTER(MPID_STATE_MPI_CART_MAP);

    /* Validate parameters, especially handles needing to be converted */
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
	    MPIR_ERRTEST_COMM(comm_old, mpi_errno);
            if (mpi_errno != MPI_SUCCESS) goto fn_fail;
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif
    
    /* Convert MPI object handles to object pointers */
    MPID_Comm_get_ptr( comm_old, comm_ptr );

    /* Validate parameters and objects (post conversion) */
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            /* Validate comm_ptr */
            MPID_Comm_valid_ptr( comm_ptr, mpi_errno );
	    /* If comm_ptr is not valid, it will be reset to null */
	    MPIR_ERRTEST_ARGNULL(newrank,"newrank",mpi_errno);
	    MPIR_ERRTEST_ARGNULL(dims,"dims",mpi_errno);
	    /* As of MPI 2.1, 0-dimensional cartesian topologies are valid */
	    if (ndims < 0) {
		mpi_errno = MPIR_Err_create_code( MPI_SUCCESS, 
				  MPIR_ERR_RECOVERABLE, 
				  FCNAME, __LINE__, MPI_ERR_DIMS,
				  "**dims", "**dims %d", ndims );
		}
            if (mpi_errno) goto fn_fail;
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */
    mpi_errno = MPIR_Cart_map_impl( comm_ptr, ndims, 
                                    (const int*) dims,
                                    (const int*) periods, newrank );
    if (mpi_errno) MPIU_ERR_POP(mpi_errno);
    /* ... end of body of routine ... */

  fn_exit:
    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_CART_MAP);
    return mpi_errno;

    /* --BEGIN ERROR HANDLING-- */
  fn_fail:
#   ifdef HAVE_ERROR_CHECKING
    {
	mpi_errno = MPIR_Err_create_code(
	    mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, 
	    "**mpi_cart_map",
	    "**mpi_cart_map %C %d %p %p %p", comm_old, ndims, dims, periods, 
	    newrank);
    }
#   endif
    mpi_errno = MPIR_Err_return_comm( comm_ptr, FCNAME, mpi_errno );
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}
