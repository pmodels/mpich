/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"

#ifndef BUILD_MPI_ABI
static int unweighted_dummy = 0x46618;
static int weights_empty_dummy = 0x022284;
/* cannot ==NULL, would be ambiguous */
int *const MPI_UNWEIGHTED = &unweighted_dummy;
int *const MPI_WEIGHTS_EMPTY = &weights_empty_dummy;
#endif

/* Keyval for topology information */
static int MPIR_Topology_keyval = MPI_KEYVAL_INVALID;

/* Local functions */
#ifndef BUILD_MPI_ABI
static int MPIR_Topology_copy_fn(MPI_Comm, int, void *, void *, void *, int *);
static int MPIR_Topology_delete_fn(MPI_Comm, int, void *, void *);
#else
static int MPIR_Topology_copy_fn(ABI_Comm, int, void *, void *, void *, int *);
static int MPIR_Topology_delete_fn(ABI_Comm, int, void *, void *);
#endif
static int MPIR_Topology_finalize(void *);

/*
  Return a pointer to the topology structure on a communicator.
  Returns null if no topology structure is defined
*/
MPIR_Topology *MPIR_Topology_get(MPIR_Comm * comm_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Topology *topo_ptr;
    int flag;

    if (MPIR_Topology_keyval == MPI_KEYVAL_INVALID) {
        return 0;
    }

    mpi_errno = MPIR_Comm_get_attr_impl(comm_ptr, MPIR_Topology_keyval,
                                        &topo_ptr, &flag, MPIR_ATTR_PTR);
    if (mpi_errno)
        return NULL;

    if (flag)
        return topo_ptr;
    return NULL;
}

int MPIR_Topology_put(MPIR_Comm * comm_ptr, MPIR_Topology * topo_ptr)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_Assert(comm_ptr != NULL);

    if (MPIR_Topology_keyval == MPI_KEYVAL_INVALID) {
        /* Create a new keyval */
        /* FIXME - thread safe code needs a thread lock here, followed
         * by another test on the keyval to see if a different thread
         * got there first */
        mpi_errno = MPIR_Comm_create_keyval_impl(MPIR_Topology_copy_fn,
                                                 MPIR_Topology_delete_fn, &MPIR_Topology_keyval, 0);
        /* Register the finalize handler */
        MPIR_ERR_CHECK(mpi_errno);
        MPIR_Add_finalize(MPIR_Topology_finalize, (void *) 0, MPIR_FINALIZE_CALLBACK_PRIO - 1);
    }
    MPII_Keyval *keyval_ptr;
    MPII_Keyval_get_ptr(MPIR_Topology_keyval, keyval_ptr);
    mpi_errno = MPIR_Comm_set_attr_impl(comm_ptr, keyval_ptr, topo_ptr, MPIR_ATTR_PTR);
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

/* Ignore p */

static int MPIR_Topology_finalize(void *p ATTRIBUTE((unused)))
{
    MPL_UNREFERENCED_ARG(p);

    if (MPIR_Topology_keyval != MPI_KEYVAL_INVALID) {
        /* Just in case */
        MPII_Keyval *keyval_ptr;
        MPII_Keyval_get_ptr(MPIR_Topology_keyval, keyval_ptr);
        MPIR_free_keyval(keyval_ptr);
        MPIR_Topology_keyval = MPI_KEYVAL_INVALID;
    }
    return 0;
}


static int *MPIR_Copy_array(int n, const int a[], int *err)
{
    int *new_p;

    /* the copy of NULL is NULL */
    if (a == NULL) {
        MPIR_Assert(n == 0);
        return NULL;
    }

    new_p = (int *) MPL_malloc(n * sizeof(int), MPL_MEM_OTHER);

    /* --BEGIN ERROR HANDLING-- */
    if (!new_p) {
        *err = MPI_ERR_OTHER;
        return 0;
    }
    /* --END ERROR HANDLING-- */
    MPIR_Memcpy(new_p, a, n * sizeof(int));
    return new_p;
}

/* The keyval copy and delete functions must handle copying and deleting
   the associated topology structures

   We can reduce the number of allocations by making a single allocation
   of enough integers for all fields (including the ones in the structure)
   and freeing the single object later.
*/
static int MPIR_Topology_copy_internal(void *attr_in, void *attr_out, int *flag)
{
    MPIR_Topology *old_topology = (MPIR_Topology *) attr_in;
    MPIR_Topology *copy_topology = NULL;
    MPIR_CHKPMEM_DECL();
    int mpi_errno = 0;

    *flag = 0;
    *(void **) attr_out = NULL;

    MPIR_CHKPMEM_MALLOC(copy_topology, sizeof(MPIR_Topology), MPL_MEM_COMM);

    MPL_VG_MEM_INIT(copy_topology, sizeof(MPIR_Topology));

    /* simplify copying and error handling */
#define MPIR_ARRAY_COPY_HELPER(kind_,array_field_,count_field_) \
        do { \
            copy_topology->topo.kind_.array_field_ = \
                MPIR_Copy_array(old_topology->topo.kind_.count_field_, \
                                old_topology->topo.kind_.array_field_, \
                                &mpi_errno); \
            MPIR_ERR_CHECK(mpi_errno); \
            MPIR_CHKPMEM_REGISTER(copy_topology->topo.kind_.array_field_); \
        } while (0)

    copy_topology->kind = old_topology->kind;
    if (old_topology->kind == MPI_CART) {
        copy_topology->topo.cart.ndims = old_topology->topo.cart.ndims;
        copy_topology->topo.cart.nnodes = old_topology->topo.cart.nnodes;
        MPIR_ARRAY_COPY_HELPER(cart, dims, ndims);
        MPIR_ARRAY_COPY_HELPER(cart, periodic, ndims);
        MPIR_ARRAY_COPY_HELPER(cart, position, ndims);
    } else if (old_topology->kind == MPI_GRAPH) {
        copy_topology->topo.graph.nnodes = old_topology->topo.graph.nnodes;
        copy_topology->topo.graph.nedges = old_topology->topo.graph.nedges;
        MPIR_ARRAY_COPY_HELPER(graph, index, nnodes);
        MPIR_ARRAY_COPY_HELPER(graph, edges, nedges);
    } else if (old_topology->kind == MPI_DIST_GRAPH) {
        copy_topology->topo.dist_graph.indegree = old_topology->topo.dist_graph.indegree;
        copy_topology->topo.dist_graph.outdegree = old_topology->topo.dist_graph.outdegree;
        copy_topology->topo.dist_graph.is_weighted = old_topology->topo.dist_graph.is_weighted;
        MPIR_ARRAY_COPY_HELPER(dist_graph, in, indegree);
        MPIR_ARRAY_COPY_HELPER(dist_graph, out, outdegree);
        if (old_topology->topo.dist_graph.is_weighted) {
            MPIR_ARRAY_COPY_HELPER(dist_graph, in_weights, indegree);
            MPIR_ARRAY_COPY_HELPER(dist_graph, out_weights, outdegree);
        } else {
            copy_topology->topo.dist_graph.in_weights = NULL;
            copy_topology->topo.dist_graph.out_weights = NULL;
        }
    }
    /* --BEGIN ERROR HANDLING-- */
    else {
        /* Unknown topology */
        return MPI_ERR_TOPOLOGY;
    }
    /* --END ERROR HANDLING-- */
#undef MPIR_ARRAY_COPY_HELPER

    *(void **) attr_out = (void *) copy_topology;
    *flag = 1;
  fn_exit:
    /* Return mpi_errno in case one of the copy array functions failed */
    return mpi_errno;
  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
    MPIR_CHKPMEM_REAP();
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}

static int MPIR_Topology_delete_internal(void *attr_val)
{
    MPIR_Topology *topology = (MPIR_Topology *) attr_val;

    MPL_UNREFERENCED_ARG(comm);
    MPL_UNREFERENCED_ARG(keyval);
    MPL_UNREFERENCED_ARG(extra_data);

    /* FIXME - free the attribute data structure */

    if (topology->kind == MPI_CART) {
        MPL_free(topology->topo.cart.dims);
        MPL_free(topology->topo.cart.periodic);
        MPL_free(topology->topo.cart.position);
        MPL_free(topology);
    } else if (topology->kind == MPI_GRAPH) {
        MPL_free(topology->topo.graph.index);
        MPL_free(topology->topo.graph.edges);
        MPL_free(topology);
    } else if (topology->kind == MPI_DIST_GRAPH) {
        MPL_free(topology->topo.dist_graph.in);
        MPL_free(topology->topo.dist_graph.out);
        MPL_free(topology->topo.dist_graph.in_weights);
        MPL_free(topology->topo.dist_graph.out_weights);
        MPL_free(topology);
    }
    /* --BEGIN ERROR HANDLING-- */
    else {
        return MPI_ERR_TOPOLOGY;
    }
    /* --END ERROR HANDLING-- */
    return MPI_SUCCESS;
}

#ifndef BUILD_MPI_ABI
static int MPIR_Topology_copy_fn(MPI_Comm comm ATTRIBUTE((unused)),
                                 int keyval ATTRIBUTE((unused)),
                                 void *extra_data ATTRIBUTE((unused)),
                                 void *attr_in, void *attr_out, int *flag)
{
    MPL_UNREFERENCED_ARG(comm);
    MPL_UNREFERENCED_ARG(keyval);
    MPL_UNREFERENCED_ARG(extra_data);
    return MPIR_Topology_copy_internal(attr_in, attr_out, flag);
}

static int MPIR_Topology_delete_fn(MPI_Comm comm ATTRIBUTE((unused)),
                                   int keyval ATTRIBUTE((unused)),
                                   void *attr_val, void *extra_data ATTRIBUTE((unused)))
{
    MPL_UNREFERENCED_ARG(comm);
    MPL_UNREFERENCED_ARG(keyval);
    MPL_UNREFERENCED_ARG(extra_data);
    return MPIR_Topology_delete_internal(attr_val);
}
#else
static int MPIR_Topology_copy_fn(ABI_Comm comm ATTRIBUTE((unused)),
                                 int keyval ATTRIBUTE((unused)),
                                 void *extra_data ATTRIBUTE((unused)),
                                 void *attr_in, void *attr_out, int *flag)
{
    MPL_UNREFERENCED_ARG(comm);
    MPL_UNREFERENCED_ARG(keyval);
    MPL_UNREFERENCED_ARG(extra_data);
    return MPIR_Topology_copy_internal(attr_in, attr_out, flag);
}

static int MPIR_Topology_delete_fn(ABI_Comm comm ATTRIBUTE((unused)),
                                   int keyval ATTRIBUTE((unused)),
                                   void *attr_val, void *extra_data ATTRIBUTE((unused)))
{
    MPL_UNREFERENCED_ARG(comm);
    MPL_UNREFERENCED_ARG(keyval);
    MPL_UNREFERENCED_ARG(extra_data);
    return MPIR_Topology_delete_internal(attr_val);
}
#endif

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

int MPIR_Topo_canon_nhb_count(MPIR_Comm * comm_ptr, int *indegree, int *outdegree, int *weighted)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Topology *topo_ptr;

    topo_ptr = MPIR_Topology_get(comm_ptr);
    MPIR_ERR_CHKANDJUMP(!topo_ptr, mpi_errno, MPI_ERR_TOPOLOGY, "**notopology");

    /* TODO consider dispatching via a vtable instead of doing if/else */
    if (topo_ptr->kind == MPI_DIST_GRAPH) {
        mpi_errno = MPIR_Dist_graph_neighbors_count_impl(comm_ptr, indegree, outdegree, weighted);
        MPIR_ERR_CHECK(mpi_errno);
    } else if (topo_ptr->kind == MPI_GRAPH) {
        int nneighbors = 0;
        mpi_errno = MPIR_Graph_neighbors_count_impl(comm_ptr, comm_ptr->rank, &nneighbors);
        MPIR_ERR_CHECK(mpi_errno);
        *indegree = *outdegree = nneighbors;
        *weighted = FALSE;
    } else if (topo_ptr->kind == MPI_CART) {
        *indegree = 2 * topo_ptr->topo.cart.ndims;
        *outdegree = 2 * topo_ptr->topo.cart.ndims;
        *weighted = FALSE;
    } else {
        MPIR_Assert(FALSE);
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIR_Topo_canon_nhb(MPIR_Comm * comm_ptr,
                        int indegree, int sources[], int inweights[],
                        int outdegree, int dests[], int outweights[])
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Topology *topo_ptr;

    MPIR_FUNC_ENTER;

    topo_ptr = MPIR_Topology_get(comm_ptr);
    MPIR_ERR_CHKANDJUMP(!topo_ptr, mpi_errno, MPI_ERR_TOPOLOGY, "**notopology");

    /* TODO consider dispatching via a vtable instead of doing if/else */
    if (topo_ptr->kind == MPI_DIST_GRAPH) {
        mpi_errno = MPIR_Dist_graph_neighbors_impl(comm_ptr,
                                                   indegree, sources, inweights,
                                                   outdegree, dests, outweights);
        MPIR_ERR_CHECK(mpi_errno);
    } else if (topo_ptr->kind == MPI_GRAPH) {
        MPIR_Assert(indegree == outdegree);
        mpi_errno = MPIR_Graph_neighbors_impl(comm_ptr, comm_ptr->rank, indegree, sources);
        MPIR_ERR_CHECK(mpi_errno);
        MPIR_Memcpy(dests, sources, outdegree * sizeof(*dests));
        /* ignore inweights/outweights */
    } else if (topo_ptr->kind == MPI_CART) {
        int d;

        MPIR_Assert(indegree == outdegree);
        MPIR_Assert(indegree == 2 * topo_ptr->topo.cart.ndims);

        for (d = 0; d < topo_ptr->topo.cart.ndims; ++d) {
            mpi_errno = MPIR_Cart_shift_impl(comm_ptr, d, 1, &sources[2 * d], &sources[2 * d + 1]);
            MPIR_ERR_CHECK(mpi_errno);

            dests[2 * d] = sources[2 * d];
            dests[2 * d + 1] = sources[2 * d + 1];
        }
        /* ignore inweights/outweights */
    } else {
        MPIR_Assert(FALSE);
    }

#ifdef MPL_USE_DBG_LOGGING
    {
        int i;
        MPL_DBG_MSG_FMT(MPIR_DBG_PT2PT, VERBOSE,
                        (MPL_DBG_FDEST, "canonical neighbors for comm=0x%x comm_ptr=%p",
                         comm_ptr->handle, comm_ptr));
        for (i = 0; i < outdegree; ++i) {
            MPL_DBG_MSG_FMT(MPIR_DBG_PT2PT, VERBOSE,
                            (MPL_DBG_FDEST, "%d/%d: to   %d", i, outdegree, dests[i]));
        }
        for (i = 0; i < indegree; ++i) {
            MPL_DBG_MSG_FMT(MPIR_DBG_PT2PT, VERBOSE,
                            (MPL_DBG_FDEST, "%d/%d: from %d", i, indegree, sources[i]));
        }
    }
#endif

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
