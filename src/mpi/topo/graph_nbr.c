/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"
#include "topo.h"

/* -- Begin Profiling Symbol Block for routine MPI_Graph_neighbors */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Graph_neighbors = PMPI_Graph_neighbors
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Graph_neighbors  MPI_Graph_neighbors
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Graph_neighbors as PMPI_Graph_neighbors
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Graph_neighbors
#define MPI_Graph_neighbors PMPI_Graph_neighbors

#endif

#undef FUNCNAME
#define FUNCNAME MPI_Graph_neighbors

/*@
 MPI_Graph_neighbors - Returns the neighbors of a node associated 
                       with a graph topology

Input Parameters:
+ comm - communicator with graph topology (handle) 
. rank - rank of process in group of comm (integer) 
- maxneighbors - size of array neighbors (integer) 

Output Parameters:
. neighbors - ranks of processes that are neighbors to specified process
 (array of integer) 

.N SignalSafe

.N Fortran

.N Errors
.N MPI_SUCCESS
.N MPI_ERR_TOPOLOGY
.N MPI_ERR_COMM
.N MPI_ERR_ARG
.N MPI_ERR_RANK
@*/
int MPI_Graph_neighbors(MPI_Comm comm, int rank, int maxneighbors, 
			int *neighbors)
{
    static const char FCNAME[] = "MPI_Graph_neighbors";
    int mpi_errno = MPI_SUCCESS;
    MPID_Comm *comm_ptr = NULL;
    MPIR_Topology *graph_ptr;
    int i, is, ie;
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_GRAPH_NEIGHBORS);

    MPIR_ERRTEST_INITIALIZED_ORDIE();
    
    MPID_MPI_FUNC_ENTER(MPID_STATE_MPI_GRAPH_NEIGHBORS);

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
	    MPIR_ERRTEST_ARGNULL(neighbors,"neighbors",mpi_errno);
            if (mpi_errno) goto fn_fail;
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */
    
    graph_ptr = MPIR_Topology_get( comm_ptr );

    MPIU_ERR_CHKANDJUMP((!graph_ptr || graph_ptr->kind != MPI_GRAPH), mpi_errno, MPI_ERR_TOPOLOGY, "**notgraphtopo");
    MPIU_ERR_CHKANDJUMP2((rank < 0 || rank >= graph_ptr->topo.graph.nnodes), mpi_errno, MPI_ERR_RANK, "**rank",
			 "**rank %d %d", rank, graph_ptr->topo.graph.nnodes );

    /* Get location in edges array of the neighbors of the specified rank */
    if (rank == 0) is = 0;
    else           is = graph_ptr->topo.graph.index[rank-1];
    ie = graph_ptr->topo.graph.index[rank];

    /* Get neighbors */
    for (i=is; i<ie; i++) 
	*neighbors++ = graph_ptr->topo.graph.edges[i];
    
    /* ... end of body of routine ... */

  fn_exit:
    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_GRAPH_NEIGHBORS);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
#   ifdef HAVE_ERROR_CHECKING
    {
	mpi_errno = MPIR_Err_create_code(
	    mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**mpi_graph_neighbors",
	    "**mpi_graph_neighbors %C %d %d %p", comm, rank, maxneighbors, neighbors);
    }
#   endif
    mpi_errno = MPIR_Err_return_comm( comm_ptr, FCNAME, mpi_errno );
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}
