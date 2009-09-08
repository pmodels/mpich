/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "mpiimpl.h"
#include "topo.h"

/* Keyval for topology information */
static int MPIR_Topology_keyval = MPI_KEYVAL_INVALID;  

/* Local functions */
static int MPIR_Topology_copy_fn ( MPI_Comm, int, void *, void *, void *, 
				   int * );
static int MPIR_Topology_delete_fn ( MPI_Comm, int, void *, void * );
static int MPIR_Topology_finalize ( void * );

/*
  Return a poiner to the topology structure on a communicator.
  Returns null if no topology structure is defined 
*/
MPIR_Topology *MPIR_Topology_get( MPID_Comm *comm_ptr )
{
    MPIR_Topology *topo_ptr;
    int flag;
    MPIU_THREADPRIV_DECL;

    if (MPIR_Topology_keyval == MPI_KEYVAL_INVALID) {
	return 0;
    }

    MPIU_THREADPRIV_GET;
    MPIR_Nest_incr();
    (void)NMPI_Comm_get_attr(comm_ptr->handle, MPIR_Topology_keyval,
			     &topo_ptr, &flag );
    MPIR_Nest_decr();

    if (flag) return topo_ptr;
    return 0;
}

int MPIR_Topology_put( MPID_Comm *comm_ptr, MPIR_Topology *topo_ptr )
{
    int mpi_errno;
    MPIU_THREADPRIV_DECL;

    MPIU_THREADPRIV_GET;

    MPIU_Assert(comm_ptr != NULL);

    if (MPIR_Topology_keyval == MPI_KEYVAL_INVALID) {
	/* Create a new keyval */
	/* FIXME - thread safe code needs a thread lock here, followed
	   by another test on the keyval to see if a different thread
	   got there first */
	MPIR_Nest_incr();
	mpi_errno = NMPI_Comm_create_keyval( MPIR_Topology_copy_fn, 
					     MPIR_Topology_delete_fn,
					     &MPIR_Topology_keyval, 0 );
	MPIR_Nest_decr();
	/* Register the finalize handler */
	if (mpi_errno) return mpi_errno;
	MPIR_Add_finalize( MPIR_Topology_finalize, (void*)0, 
			   MPIR_FINALIZE_CALLBACK_PRIO-1);
    }
    MPIR_Nest_incr();
    mpi_errno = NMPI_Comm_set_attr(comm_ptr->handle, MPIR_Topology_keyval, 
				   topo_ptr );
    MPIR_Nest_decr();
    return mpi_errno;
}

/* Ignore p */
/* begin:nested */
static int MPIR_Topology_finalize( void *p ATTRIBUTE((unused)) )
{
    MPIU_THREADPRIV_DECL;

    MPIU_THREADPRIV_GET;

    MPIR_Nest_incr();

    MPIU_UNREFERENCED_ARG(p);

    if (MPIR_Topology_keyval != MPI_KEYVAL_INVALID) {
	/* Just in case */
	NMPI_Comm_free_keyval( &MPIR_Topology_keyval );
    }
    MPIR_Nest_decr();
    return 0;
}
/* end:nested */

static int *MPIR_Copy_array( int n, const int a[], int *err )
{
    int *new_p = (int *)MPIU_Malloc( n * sizeof(int) );

    /* --BEGIN ERROR HANDLING-- */
    if (!new_p) {
	*err = MPI_ERR_OTHER;
	return 0;
    }
    /* --END ERROR HANDLING-- */
    MPIU_Memcpy(new_p, a, n * sizeof(int));
    return new_p;
}

/* The keyval copy and delete functions must handle copying and deleting 
   the assoicated topology structures 
   
   We can reduce the number of allocations by making a single allocation
   of enough integers for all fields (including the ones in the structure)
   and freeing the single object later.
*/
#undef FUNCNAME
#define FUNCNAME MPIR_Topology_copy_fn
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
static int MPIR_Topology_copy_fn ( MPI_Comm comm ATTRIBUTE((unused)), 
				   int keyval ATTRIBUTE((unused)), 
				   void *extra_data ATTRIBUTE((unused)),
				   void *attr_in, void *attr_out, 
				   int *flag )
{
    MPIR_Topology *old_topology = (MPIR_Topology *)attr_in;
    MPIR_Topology *copy_topology = NULL;
    MPIU_CHKPMEM_DECL(5);
    int mpi_errno = 0;

    MPIU_UNREFERENCED_ARG(comm);
    MPIU_UNREFERENCED_ARG(keyval);
    MPIU_UNREFERENCED_ARG(extra_data);

    *flag = 0;
    *(void **)attr_out = NULL;

    MPIU_CHKPMEM_MALLOC(copy_topology, MPIR_Topology *, sizeof(MPIR_Topology), mpi_errno, "copy_topology");

    /* simplify copying and error handling */
#define MPIR_ARRAY_COPY_HELPER(kind_,array_field_,count_field_) \
        do { \
            copy_topology->topo.kind_.array_field_ = \
                MPIR_Copy_array(old_topology->topo.kind_.count_field_, \
                                old_topology->topo.kind_.array_field_, \
                                &mpi_errno); \
            if (mpi_errno) MPIU_ERR_POP(mpi_errno); \
            MPIU_CHKPMEM_REGISTER(copy_topology->topo.kind_.array_field_); \
        } while (0)

    copy_topology->kind = old_topology->kind;
    if (old_topology->kind == MPI_CART) {
        copy_topology->topo.cart.ndims  = old_topology->topo.cart.ndims;
        copy_topology->topo.cart.nnodes = old_topology->topo.cart.nnodes;
        MPIR_ARRAY_COPY_HELPER(cart, dims, ndims);
        MPIR_ARRAY_COPY_HELPER(cart, periodic, ndims);
        MPIR_ARRAY_COPY_HELPER(cart, position, ndims);
    }
    else if (old_topology->kind == MPI_GRAPH) {
        copy_topology->topo.graph.nnodes = old_topology->topo.graph.nnodes;
        copy_topology->topo.graph.nedges = old_topology->topo.graph.nedges;
        MPIR_ARRAY_COPY_HELPER(graph, index, nnodes);
        MPIR_ARRAY_COPY_HELPER(graph, edges, nedges);
    }
    else if (old_topology->kind == MPI_DIST_GRAPH) {
        copy_topology->topo.dist_graph.indegree = old_topology->topo.dist_graph.indegree;
        copy_topology->topo.dist_graph.outdegree = old_topology->topo.dist_graph.outdegree;
        MPIR_ARRAY_COPY_HELPER(dist_graph, in, indegree);
        MPIR_ARRAY_COPY_HELPER(dist_graph, in_weights, indegree);
        MPIR_ARRAY_COPY_HELPER(dist_graph, out, outdegree);
        MPIR_ARRAY_COPY_HELPER(dist_graph, out_weights, outdegree);
    }
    /* --BEGIN ERROR HANDLING-- */
    else {
	/* Unknown topology */
	return MPI_ERR_TOPOLOGY;
    }
    /* --END ERROR HANDLING-- */
#undef MPIR_ARRAY_COPY_HELPER

    *(void **)attr_out = (void *)copy_topology;
    *flag = 1;
    MPIU_CHKPMEM_COMMIT();
fn_exit:
    /* Return mpi_errno in case one of the copy array functions failed */
    return mpi_errno;
fn_fail:
    MPIU_CHKPMEM_REAP();
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIR_Topology_delete_fn
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
static int MPIR_Topology_delete_fn ( MPI_Comm comm ATTRIBUTE((unused)), 
				     int keyval ATTRIBUTE((unused)), 
				     void *attr_val, 
				     void *extra_data ATTRIBUTE((unused)) )
{
    MPIR_Topology *topology = (MPIR_Topology *)attr_val;

    MPIU_UNREFERENCED_ARG(comm);
    MPIU_UNREFERENCED_ARG(keyval);
    MPIU_UNREFERENCED_ARG(extra_data);

    /* FIXME - free the attribute data structure */
    
    if (topology->kind == MPI_CART) {
	MPIU_Free( topology->topo.cart.dims );
	MPIU_Free( topology->topo.cart.periodic );
	MPIU_Free( topology->topo.cart.position );
	MPIU_Free( topology );
    }
    else if (topology->kind == MPI_GRAPH) {
	MPIU_Free( topology->topo.graph.index );
	MPIU_Free( topology->topo.graph.edges );
	MPIU_Free( topology );
    }
    else if (topology->kind == MPI_DIST_GRAPH) {
        MPIU_Free(topology->topo.dist_graph.in);
        MPIU_Free(topology->topo.dist_graph.out);
        if (topology->topo.dist_graph.in_weights)
            MPIU_Free(topology->topo.dist_graph.in_weights);
        if (topology->topo.dist_graph.out_weights)
            MPIU_Free(topology->topo.dist_graph.out_weights);
        MPIU_Free(topology );
    }
    /* --BEGIN ERROR HANDLING-- */
    else {
	return MPI_ERR_TOPOLOGY;
    }
    /* --END ERROR HANDLING-- */
    return MPI_SUCCESS;
}


