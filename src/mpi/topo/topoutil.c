/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "mpiimpl.h"
#include "topo.h"

static int unweighted_dummy = 0x46618;
static int weights_empty_dummy = 0x022284;
/* cannot ==NULL, would be ambiguous */
int * const MPI_UNWEIGHTED = &unweighted_dummy;
int * const MPI_WEIGHTS_EMPTY = &weights_empty_dummy;

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
    int mpi_errno = MPI_SUCCESS;
    MPIR_Topology *topo_ptr;
    int flag;

    if (MPIR_Topology_keyval == MPI_KEYVAL_INVALID) {
	return 0;
    }

    mpi_errno = MPIR_CommGetAttr(comm_ptr->handle, MPIR_Topology_keyval,
                                 &topo_ptr, &flag, MPIR_ATTR_PTR );
    if (mpi_errno) return NULL;
    
    if (flag)
        return topo_ptr;
    return NULL;
}

#undef FUNCNAME
#define FUNCNAME MPIR_Topology_put
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Topology_put( MPID_Comm *comm_ptr, MPIR_Topology *topo_ptr )
{
    int mpi_errno = MPI_SUCCESS;

    MPIU_Assert(comm_ptr != NULL);

    if (MPIR_Topology_keyval == MPI_KEYVAL_INVALID) {
	/* Create a new keyval */
	/* FIXME - thread safe code needs a thread lock here, followed
	   by another test on the keyval to see if a different thread
	   got there first */
	mpi_errno = MPIR_Comm_create_keyval_impl( MPIR_Topology_copy_fn,
                                                  MPIR_Topology_delete_fn,
                                                  &MPIR_Topology_keyval, 0 );
	/* Register the finalize handler */
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);
        MPIR_Add_finalize( MPIR_Topology_finalize, (void*)0,
			   MPIR_FINALIZE_CALLBACK_PRIO-1);
    }
    mpi_errno = MPIR_Comm_set_attr_impl(comm_ptr, MPIR_Topology_keyval, topo_ptr, MPIR_ATTR_PTR);
    if (mpi_errno) MPIR_ERR_POP(mpi_errno);
    
 fn_exit:
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}

/* Ignore p */

static int MPIR_Topology_finalize( void *p ATTRIBUTE((unused)) )
{
    MPIU_UNREFERENCED_ARG(p);

    if (MPIR_Topology_keyval != MPI_KEYVAL_INVALID) {
	/* Just in case */
	MPIR_Comm_free_keyval_impl(MPIR_Topology_keyval);
        MPIR_Topology_keyval = MPI_KEYVAL_INVALID;
    }
    return 0;
}


static int *MPIR_Copy_array( int n, const int a[], int *err )
{
    int *new_p;

    /* the copy of NULL is NULL */
    if (a == NULL) {
        MPIU_Assert(n == 0);
        return NULL;
    }

    new_p = (int *)MPIU_Malloc( n * sizeof(int) );

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
   the associated topology structures 
   
   We can reduce the number of allocations by making a single allocation
   of enough integers for all fields (including the ones in the structure)
   and freeing the single object later.
*/
#undef FUNCNAME
#define FUNCNAME MPIR_Topology_copy_fn
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
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
            if (mpi_errno) MPIR_ERR_POP(mpi_errno); \
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
    /* --BEGIN ERROR HANDLING-- */
    MPIU_CHKPMEM_REAP();
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}

#undef FUNCNAME
#define FUNCNAME MPIR_Topology_delete_fn
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
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


/* the next two routines implement the following behavior (quoted from Section
 * 7.6 of the MPI-3.0 standard):
 *
 *     For a distributed graph topology, created with MPI_DIST_GRAPH_CREATE, the
 *     sequence of neighbors in the send and receive buffers at each process
 *     is defined as the sequence returned by MPI_DIST_GRAPH_NEIGHBORS for
 *     destinations and sources, respectively. For a general graph topology,
 *     created with MPI_GRAPH_CREATE, the order of neighbors in the send and
 *     receive buffers is defined as the sequence of neighbors as returned by
 *     MPI_GRAPH_NEIGHBORS. Note that general graph topologies should generally
 *     be replaced by the distributed graph topologies.
 *
 *     For a Cartesian topology, created with MPI_CART_CREATE, the sequence of
 *     neighbors in the send and receive buffers at each process is defined by
 *     order of the dimensions, first the neighbor in the negative direction and
 *     then in the positive direction with displacement 1. The numbers of
 *     sources and destinations in the communication routines are 2*ndims with
 *     ndims defined in MPI_CART_CREATE. If a neighbor does not exist, i.e., at
 *     the border of a Cartesian topology in the case of a non-periodic virtual
 *     grid dimension (i.e., periods[...]==false), then this neighbor is defined
 *     to be MPI_PROC_NULL.
 */

#undef FUNCNAME
#define FUNCNAME MPIR_Topo_canon_nhb_count
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Topo_canon_nhb_count(MPID_Comm *comm_ptr, int *indegree, int *outdegree, int *weighted)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Topology *topo_ptr;

    topo_ptr = MPIR_Topology_get(comm_ptr);

    /* TODO consider dispatching via a vtable instead of doing if/else */
    if (topo_ptr->kind == MPI_DIST_GRAPH) {
        mpi_errno = MPIR_Dist_graph_neighbors_count_impl(comm_ptr, indegree, outdegree, weighted);
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);
    }
    else if (topo_ptr->kind == MPI_GRAPH) {
        int nneighbors = 0;
        mpi_errno = MPIR_Graph_neighbors_count_impl(comm_ptr, comm_ptr->rank, &nneighbors);
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);
        *indegree = *outdegree = nneighbors;
        *weighted = FALSE;
    }
    else if (topo_ptr->kind == MPI_CART) {
        *indegree  = 2 * topo_ptr->topo.cart.ndims;
        *outdegree = 2 * topo_ptr->topo.cart.ndims;
        *weighted  = FALSE;
    }
    else {
        MPIU_Assert(FALSE);
    }

fn_exit:
    return mpi_errno;
fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIR_Topo_canon_nhb
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Topo_canon_nhb(MPID_Comm *comm_ptr,
                        int indegree, int sources[], int inweights[],
                        int outdegree, int dests[], int outweights[])
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Topology *topo_ptr;
    MPID_MPI_STATE_DECL(MPID_STATE_MPIR_TOPO_CANON_NHB);

    MPID_MPI_FUNC_ENTER(MPID_STATE_MPIR_TOPO_CANON_NHB);

    topo_ptr = MPIR_Topology_get(comm_ptr);

    /* TODO consider dispatching via a vtable instead of doing if/else */
    if (topo_ptr->kind == MPI_DIST_GRAPH) {
        mpi_errno = MPIR_Dist_graph_neighbors_impl(comm_ptr,
                                                   indegree, sources, inweights,
                                                   outdegree, dests, outweights);
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);
    }
    else if (topo_ptr->kind == MPI_GRAPH) {
        MPIU_Assert(indegree == outdegree);
        mpi_errno = MPIR_Graph_neighbors_impl(comm_ptr, comm_ptr->rank, indegree, sources);
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);
        MPIU_Memcpy(dests, sources, outdegree*sizeof(*dests));
        /* ignore inweights/outweights */
    }
    else if (topo_ptr->kind == MPI_CART) {
        int d;

        MPIU_Assert(indegree == outdegree);
        MPIU_Assert(indegree == 2*topo_ptr->topo.cart.ndims);

        for (d = 0; d < topo_ptr->topo.cart.ndims; ++d) {
            mpi_errno = MPIR_Cart_shift_impl(comm_ptr, d, 1, &sources[2*d], &sources[2*d+1]);
            if (mpi_errno) MPIR_ERR_POP(mpi_errno);

            dests[2*d]   = sources[2*d];
            dests[2*d+1] = sources[2*d+1];
        }
        /* ignore inweights/outweights */
    }
    else {
        MPIU_Assert(FALSE);
    }

#ifdef USE_DBG_LOGGING
    {
        int i;
        MPIU_DBG_MSG_FMT(PT2PT, VERBOSE, (MPIU_DBG_FDEST, "canonical neighbors for comm=0x%x comm_ptr=%p", comm_ptr->handle, comm_ptr));
        for (i = 0; i < outdegree; ++i) {
            MPIU_DBG_MSG_FMT(PT2PT, VERBOSE, (MPIU_DBG_FDEST, "%d/%d: to   %d", i, outdegree, dests[i]));
        }
        for (i = 0; i < indegree; ++i) {
            MPIU_DBG_MSG_FMT(PT2PT, VERBOSE, (MPIU_DBG_FDEST, "%d/%d: from %d", i, indegree, sources[i]));
        }
    }
#endif

fn_exit:
    MPID_MPI_FUNC_EXIT(MPID_STATE_MPIR_TOPO_CANON_NHB);
    return mpi_errno;
fn_fail:
    goto fn_exit;
}

