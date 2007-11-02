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
static int MPIR_Topology_finalize( void *p )
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
    int i;
    
    /* --BEGIN ERROR HANDLING-- */
    if (!new_p) {
	*err = MPI_ERR_OTHER;
	return 0;
    }
    /* --END ERROR HANDLING-- */
    for (i=0; i<n; i++) {
	new_p[i] = a[i];
    }
    return new_p;
}

/* The keyval copy and delete functions must handle copying and deleting 
   the assoicated topology structures 
   
   We can reduce the number of allocations by making a single allocation
   of enough integers for all fields (including the ones in the structure)
   and freeing the single object later.
*/
static int MPIR_Topology_copy_fn ( MPI_Comm comm, int keyval, void *extra_data,
				   void *attr_in, void *attr_out, 
				   int *flag )
{
    MPIR_Topology *old_topology = (MPIR_Topology *)attr_in;
    MPIR_Topology *copy_topology = (MPIR_Topology *)MPIU_Malloc( sizeof( MPIR_Topology) );
    int mpi_errno = 0;

    MPIU_UNREFERENCED_ARG(comm);
    MPIU_UNREFERENCED_ARG(keyval);
    MPIU_UNREFERENCED_ARG(extra_data);

    /* --BEGIN ERROR HANDLING-- */
    if (!copy_topology) {
	return MPI_ERR_OTHER;
    }
    /* --END ERROR HANDLING-- */

    copy_topology->kind = old_topology->kind;
    if (old_topology->kind == MPI_CART) {
	int ndims = old_topology->topo.cart.ndims;
	copy_topology->topo.cart.nnodes = old_topology->topo.cart.nnodes;
	copy_topology->topo.cart.ndims  = ndims;
	copy_topology->topo.cart.dims   = MPIR_Copy_array( ndims,
				     old_topology->topo.cart.dims, 
				     &mpi_errno );
	copy_topology->topo.cart.periodic = MPIR_Copy_array( ndims,
			       old_topology->topo.cart.periodic, &mpi_errno );
	copy_topology->topo.cart.position = MPIR_Copy_array( ndims, 
			       old_topology->topo.cart.position, &mpi_errno );
    }
    else if (old_topology->kind == MPI_GRAPH) {
	int nnodes = old_topology->topo.graph.nnodes;
	copy_topology->topo.graph.nnodes = nnodes;
	copy_topology->topo.graph.nedges = old_topology->topo.graph.nedges;
	copy_topology->topo.graph.index  = MPIR_Copy_array( nnodes,
				 old_topology->topo.graph.index, &mpi_errno );
	copy_topology->topo.graph.edges = MPIR_Copy_array( 
				 old_topology->topo.graph.nedges, 
				 old_topology->topo.graph.edges, &mpi_errno );
    }
    /* --BEGIN ERROR HANDLING-- */
    else {
	/* Unknown topology */
	return MPI_ERR_TOPOLOGY;
    }
    /* --END ERROR HANDLING-- */

    *(void **)attr_out = (void *)copy_topology;
    *flag = 1;
    /* Return mpi_errno in case one of the copy array functions failed */
    return mpi_errno;
}

static int MPIR_Topology_delete_fn ( MPI_Comm comm, int keyval, 
				     void *attr_val, void *extra_data )
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
    /* --BEGIN ERROR HANDLING-- */
    else {
	return MPI_ERR_TOPOLOGY;
    }
    /* --END ERROR HANDLING-- */
    return MPI_SUCCESS;
}


