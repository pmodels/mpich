/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"
#include "topo.h"

/* -- Begin Profiling Symbol Block for routine MPI_Graph_create */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Graph_create = PMPI_Graph_create
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Graph_create  MPI_Graph_create
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Graph_create as PMPI_Graph_create
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Graph_create
#define MPI_Graph_create PMPI_Graph_create

#endif

/* Note on index and indx - index is an (old) function name in string.h; 
   while it is valid to use it as a variable or parameter name, it can 
   solicit warning messages from the compiler.  To avoid those, the name
   indx is used instead of index in this file */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI

#undef FUNCNAME
#define FUNCNAME MPIR_Graph_create
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
int MPIR_Graph_create( MPID_Comm *comm_ptr, int nnodes, 
		       const int indx[], const int edges[], int reorder, 
		       MPI_Comm *comm_graph)
{
    int mpi_errno = MPI_SUCCESS;
    int i, nedges;
    MPID_Comm *newcomm_ptr = NULL;
    MPIR_Topology *graph_ptr = NULL;
    MPIU_CHKPMEM_DECL(3);

    /* Set this to null in case there is an error */
    *comm_graph = MPI_COMM_NULL;

    /* Create a new communicator */
    if (reorder) {
	int nrank;

	/* Allow the cart map routine to remap the assignment of ranks to 
	   processes */
	mpi_errno = MPIR_Graph_map_impl(comm_ptr, nnodes, indx, edges, &nrank);
        if (mpi_errno) MPIU_ERR_POP(mpi_errno);
	/* Create the new communicator with split, since we need to reorder
	   the ranks (including the related internals, such as the connection
	   tables */
        mpi_errno = MPIR_Comm_split_impl( comm_ptr,
                                          nrank == MPI_UNDEFINED ? MPI_UNDEFINED : 1,
                                          nrank, &newcomm_ptr );
        if (mpi_errno) MPIU_ERR_POP(mpi_errno);
    } else {
	/* Just use the first nnodes processes in the communicator */
	mpi_errno = MPIR_Comm_copy( (MPID_Comm *)comm_ptr, nnodes, 
				    &newcomm_ptr );
        if (mpi_errno) MPIU_ERR_POP(mpi_errno);
    }


    /* If this process is not in the resulting communicator, return a 
       null communicator and exit */
    if (!newcomm_ptr) {
	*comm_graph = MPI_COMM_NULL;
	goto fn_exit;
    }

    nedges = indx[nnodes-1];
    MPIU_CHKPMEM_MALLOC(graph_ptr,MPIR_Topology*,sizeof(MPIR_Topology),
			mpi_errno,"graph_ptr");
    
    graph_ptr->kind = MPI_GRAPH;
    graph_ptr->topo.graph.nnodes = nnodes;
    graph_ptr->topo.graph.nedges = nedges;
    MPIU_CHKPMEM_MALLOC(graph_ptr->topo.graph.index,int*,
			nnodes*sizeof(int),mpi_errno,"graph.index");
    MPIU_CHKPMEM_MALLOC(graph_ptr->topo.graph.edges,int*,
			nedges*sizeof(int),mpi_errno,"graph.edges");
    for (i=0; i<nnodes; i++) 
	graph_ptr->topo.graph.index[i] = indx[i];
    for (i=0; i<nedges; i++) 
	graph_ptr->topo.graph.edges[i] = edges[i];

    /* Finally, place the topology onto the new communicator and return the
       handle */
    mpi_errno = MPIR_Topology_put( newcomm_ptr, graph_ptr );
    if (mpi_errno != MPI_SUCCESS) goto fn_fail;

    MPIU_OBJ_PUBLISH_HANDLE(*comm_graph, newcomm_ptr->handle);

    /* ... end of body of routine ... */

  fn_exit:
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
    MPIU_CHKPMEM_REAP();
#   ifdef HAVE_ERROR_CHECKING
    {
	mpi_errno = MPIR_Err_create_code(
	    mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, 
	    "**mpi_graph_create",
	    "**mpi_graph_create %C %d %p %p %d %p", comm_ptr->handle, 
	    nnodes, indx, 
	    edges, reorder, comm_graph);
    }
#   endif
    mpi_errno = MPIR_Err_return_comm( (MPID_Comm*)comm_ptr, FCNAME, mpi_errno );
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}
#endif

#undef FUNCNAME
#define FUNCNAME MPI_Graph_create
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
/*@
MPI_Graph_create - Makes a new communicator to which topology information
                 has been attached

Input Parameters:
+ comm_old - input communicator without topology (handle) 
. nnodes - number of nodes in graph (integer) 
. indx - array of integers describing node degrees (see below) 
. edges - array of integers describing graph edges (see below) 
- reorder - ranking may be reordered (true) or not (false) (logical) 

Output Parameter:
. comm_graph - communicator with graph topology added (handle) 

Notes:
Each process must provide a description of the entire graph, not just the
neigbors of the calling process.  

Algorithm:
We ignore the 'reorder' info currently.

.N ThreadSafe

.N Fortran

.N Errors
.N MPI_SUCCESS
.N MPI_ERR_TOPOLOGY
.N MPI_ERR_COMM
.N MPI_ERR_ARG

@*/
int MPI_Graph_create(MPI_Comm comm_old, int nnodes, int *indx, int *edges, 
		     int reorder, MPI_Comm *comm_graph)
{
    int mpi_errno = MPI_SUCCESS;
    MPID_Comm *comm_ptr = NULL;
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_GRAPH_CREATE);

    MPIR_ERRTEST_INITIALIZED_ORDIE();
    
    MPIU_THREAD_CS_ENTER(ALLFUNC,);
    MPID_MPI_FUNC_ENTER(MPID_STATE_MPI_GRAPH_CREATE);
    
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
	    if (comm_ptr) {
		MPIR_ERRTEST_COMM_INTRA(comm_ptr,mpi_errno);
	    }
	    MPIR_ERRTEST_ARGNEG(nnodes,"nnodes",mpi_errno);
	    if (nnodes > 0) {
		MPIR_ERRTEST_ARGNULL(indx,"index",mpi_errno);
		MPIR_ERRTEST_ARGNULL(edges,"edges",mpi_errno);
	    }
	    MPIR_ERRTEST_ARGNULL(comm_graph,"comm_graph",mpi_errno);
            if (mpi_errno) goto fn_fail;
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
	    int comm_size = comm_ptr->remote_size;
            int i;

	    /* Check that the communicator is large enough */
	    if (nnodes > comm_size) {
		mpi_errno = MPIR_Err_create_code( MPI_SUCCESS, 
						  MPIR_ERR_RECOVERABLE, 
						  FCNAME, __LINE__, 
						  MPI_ERR_ARG,
				  "**topotoolarge", "**topotoolarge %d %d",
					  nnodes, comm_size );
	    }
	    
	    /* Perform the remaining tests only if nnodes is valid.  
	       This avoids SEGVs from accessing invalid parts of the
	       edges or index arrays */
            if (mpi_errno) goto fn_fail;
	    
	    /* Check that index is monotone nondecreasing */
	    /* Use ERR_ARG instead of ERR_TOPOLOGY because there is no
	       topology yet */
	    for (i=0; i<nnodes; i++) {
		if (indx[i] < 0) {
		    mpi_errno = MPIR_Err_create_code( MPI_SUCCESS, 
						      MPIR_ERR_RECOVERABLE, 
						      FCNAME, __LINE__, 
						      MPI_ERR_ARG,
			      "**indexneg", "**indexneg %d %d", i, indx[i] );
		}
		if (i+1<nnodes && indx[i] > indx[i+1]) {
		    mpi_errno = MPIR_Err_create_code( MPI_SUCCESS, 
						      MPIR_ERR_RECOVERABLE, 
						      FCNAME, __LINE__, 
						      MPI_ERR_ARG,
			   "**indexnonmonotone", "**indexnonmonotone %d %d %d",
					      i, indx[i], indx[i+1] );
		}
	    }

	    /* Check that edge number is in range. Note that the 
	       edges refer to a rank in the communicator, and can 
	       be greater than nnodes */
	    if (nnodes > 0) { 
		for (i=0; i<indx[nnodes-1]; i++) {
		    if (edges[i] > comm_size || edges[i] < 0) {
			mpi_errno = MPIR_Err_create_code( MPI_SUCCESS, 
							  MPIR_ERR_RECOVERABLE,
							  FCNAME, __LINE__, 
							  MPI_ERR_ARG,
				  "**edgeoutrange", "**edgeoutrange %d %d %d", 
						  i, edges[i], comm_size );
		    }
		}
	    }
            if (mpi_errno) goto fn_fail;
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */
    
    /* Test for empty communicator */
    if (nnodes == 0) {
	*comm_graph = MPI_COMM_NULL;
	goto fn_exit;
    }

    if (comm_ptr->topo_fns != NULL && 
	comm_ptr->topo_fns->graphCreate != NULL) {
	mpi_errno = comm_ptr->topo_fns->graphCreate( comm_ptr, nnodes, 
						     (const int *)indx,
						     (const int *)edges, 
						     reorder, comm_graph );
    }	
    else {
	mpi_errno = MPIR_Graph_create( comm_ptr, nnodes, 
				       (const int *)indx,
				       (const int *)edges, 
				       reorder, comm_graph );
    }
    if (mpi_errno) goto fn_fail;

    /* ... end of body of routine ... */

  fn_exit:
    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_GRAPH_CREATE);
    MPIU_THREAD_CS_EXIT(ALLFUNC,);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
#   ifdef HAVE_ERROR_CHECKING
    {
	mpi_errno = MPIR_Err_create_code(
	    mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, 
	    "**mpi_graph_create",
	    "**mpi_graph_create %C %d %p %p %d %p", comm_old, nnodes, indx, 
	    edges, reorder, comm_graph);
    }
#   endif
    mpi_errno = MPIR_Err_return_comm( comm_ptr, FCNAME, mpi_errno );
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}
