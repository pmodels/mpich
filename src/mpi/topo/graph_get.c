/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"
#include "topo.h"

/* -- Begin Profiling Symbol Block for routine MPI_Graph_get */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Graph_get = PMPI_Graph_get
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Graph_get  MPI_Graph_get
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Graph_get as PMPI_Graph_get
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPI_Graph_get(MPI_Comm comm, int maxindex, int maxedges, int indx[], int edges[]) __attribute__((weak,alias("PMPI_Graph_get")));
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Graph_get
#define MPI_Graph_get PMPI_Graph_get

#endif

#undef FUNCNAME
#define FUNCNAME MPI_Graph_get

/*@

MPI_Graph_get - Retrieves graph topology information associated with a 
                communicator

Input Parameters:
+ comm - communicator with graph structure (handle) 
. maxindex - length of vector 'indx' in the calling program  (integer) 
- maxedges - length of vector 'edges' in the calling program  (integer) 

Output Parameters:
+ indx - array of integers containing the graph structure (for details see the definition of 'MPI_GRAPH_CREATE') 
- edges - array of integers containing the graph structure 

.N SignalSafe

.N Fortran

.N Errors
.N MPI_SUCCESS
.N MPI_ERR_TOPOLOGY
.N MPI_ERR_COMM
.N MPI_ERR_ARG
@*/
int MPI_Graph_get(MPI_Comm comm, int maxindex, int maxedges, 
                  int indx[], int edges[])
{
    static const char FCNAME[] = "MPI_Graph_get";
    int mpi_errno = MPI_SUCCESS;
    MPID_Comm *comm_ptr = NULL;
    MPIR_Topology *topo_ptr;
    int i, n, *vals;
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_GRAPH_GET);

    MPIR_ERRTEST_INITIALIZED_ORDIE();
    
    MPID_MPI_FUNC_ENTER(MPID_STATE_MPI_GRAPH_GET);

    /* Validate parameters, especially handles needing to be converted */
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
	    MPIR_ERRTEST_COMM(comm, mpi_errno);
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
            MPID_Comm_valid_ptr( comm_ptr, mpi_errno, TRUE );
            if (mpi_errno) goto fn_fail;
	    /* If comm_ptr is not valid, it will be reset to null */
	    
	    MPIR_ERRTEST_ARGNULL( edges, "edges", mpi_errno );
	    MPIR_ERRTEST_ARGNULL( indx,  "indx", mpi_errno );
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */
    
    topo_ptr = MPIR_Topology_get( comm_ptr );

    MPIR_ERR_CHKANDJUMP((!topo_ptr || topo_ptr->kind != MPI_GRAPH), mpi_errno, MPI_ERR_TOPOLOGY, "**notgraphtopo");
    MPIR_ERR_CHKANDJUMP3((topo_ptr->topo.graph.nnodes > maxindex), mpi_errno, MPI_ERR_ARG, "**argtoosmall",
			 "**argtoosmall %s %d %d", "maxindex", maxindex, topo_ptr->topo.graph.nnodes);
    MPIR_ERR_CHKANDJUMP3((topo_ptr->topo.graph.nedges > maxedges), mpi_errno, MPI_ERR_ARG, "**argtoosmall",
			 "**argtoosmall %s %d %d", "maxedges", maxedges, topo_ptr->topo.graph.nedges);
    
    /* Get index */
    n = topo_ptr->topo.graph.nnodes;
    vals = topo_ptr->topo.graph.index;
    for ( i=0; i<n; i++ )
	*indx++ = *vals++;
	    
    /* Get edges */
    n = topo_ptr->topo.graph.nedges;
    vals = topo_ptr->topo.graph.edges;
    for ( i=0; i<n; i++ )
	*edges++ = *vals++;

    /* ... end of body of routine ... */

  fn_exit:
    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_GRAPH_GET);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
#   ifdef HAVE_ERROR_CHECKING
    {
	mpi_errno = MPIR_Err_create_code(
	    mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**mpi_graph_get",
	    "**mpi_graph_get %C %d %d %p %p", comm, maxindex, maxedges, indx, edges);
    }
#   endif
    mpi_errno = MPIR_Err_return_comm( comm_ptr, FCNAME, mpi_errno );
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}
