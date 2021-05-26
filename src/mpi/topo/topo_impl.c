/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"

int MPIR_Cart_coords_impl(MPIR_Comm * comm_ptr, int rank, int maxdims, int coords[])
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Topology *cart_ptr;

    cart_ptr = MPIR_Topology_get(comm_ptr);

    /* Calculate coords */
    int nnodes = cart_ptr->topo.cart.nnodes;
    for (int i = 0; i < cart_ptr->topo.cart.ndims; i++) {
        nnodes = nnodes / cart_ptr->topo.cart.dims[i];
        coords[i] = rank / nnodes;
        rank = rank % nnodes;
    }

    return mpi_errno;
}

int MPIR_Cart_create_impl(MPIR_Comm * comm_ptr, int ndims, const int dims[],
                          const int periods[], int reorder, MPIR_Comm ** comm_cart_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    int i, newsize, rank, nranks;
    MPIR_Comm *newcomm_ptr = NULL;
    MPIR_Topology *cart_ptr = NULL;
    MPIR_CHKPMEM_DECL(4);

    /* Check for invalid arguments */
    newsize = 1;
    for (i = 0; i < ndims; i++)
        newsize *= dims[i];

    /* Use ERR_ARG instead of ERR_TOPOLOGY because there is no topology yet */
    MPIR_ERR_CHKANDJUMP2((newsize > comm_ptr->remote_size), mpi_errno,
                         MPI_ERR_ARG, "**cartdim",
                         "**cartdim %d %d", comm_ptr->remote_size, newsize);

    if (ndims == 0) {
        /* specified as a 0D Cartesian topology in MPI 2.1.
         * Rank 0 returns a dup of COMM_SELF with the topology info attached.
         * Others return MPI_COMM_NULL. */

        rank = comm_ptr->rank;

        if (rank == 0) {
            MPIR_Comm *comm_self_ptr;
            MPIR_Comm_get_ptr(MPI_COMM_SELF, comm_self_ptr);
            mpi_errno = MPIR_Comm_dup_impl(comm_self_ptr, &newcomm_ptr);
            MPIR_ERR_CHECK(mpi_errno);

            /* Create the topology structure */
            MPIR_CHKPMEM_MALLOC(cart_ptr, MPIR_Topology *, sizeof(MPIR_Topology),
                                mpi_errno, "cart_ptr", MPL_MEM_COMM);

            cart_ptr->kind = MPI_CART;
            cart_ptr->topo.cart.nnodes = 1;
            cart_ptr->topo.cart.ndims = 0;

            /* make mallocs of size 1 int so that they get freed as part of the
             * normal free mechanism */

            MPIR_CHKPMEM_MALLOC(cart_ptr->topo.cart.dims, int *, sizeof(int),
                                mpi_errno, "cart.dims", MPL_MEM_COMM);
            MPIR_CHKPMEM_MALLOC(cart_ptr->topo.cart.periodic, int *, sizeof(int),
                                mpi_errno, "cart.periodic", MPL_MEM_COMM);
            MPIR_CHKPMEM_MALLOC(cart_ptr->topo.cart.position, int *, sizeof(int),
                                mpi_errno, "cart.position", MPL_MEM_COMM);
        } else {
            *comm_cart_ptr = NULL;
            goto fn_exit;
        }
    } else {

        /* Create a new communicator as a duplicate of the input communicator
         * (but do not duplicate the attributes) */
        if (reorder) {

            /* Allow the cart map routine to remap the assignment of ranks to
             * processes */
            mpi_errno = MPIR_Cart_map_impl(comm_ptr, ndims, (const int *) dims,
                                           (const int *) periods, &rank);
            MPIR_ERR_CHECK(mpi_errno);

            /* Create the new communicator with split, since we need to reorder
             * the ranks (including the related internals, such as the connection
             * tables */
            mpi_errno = MPIR_Comm_split_impl(comm_ptr,
                                             rank == MPI_UNDEFINED ? MPI_UNDEFINED : 1,
                                             rank, &newcomm_ptr);
            MPIR_ERR_CHECK(mpi_errno);

        } else {
            mpi_errno = MPII_Comm_copy((MPIR_Comm *) comm_ptr, newsize, NULL, &newcomm_ptr);
            MPIR_ERR_CHECK(mpi_errno);
            rank = comm_ptr->rank;
        }

        /* If this process is not in the resulting communicator, return a
         * null communicator and exit */
        if (rank >= newsize || rank == MPI_UNDEFINED) {
            *comm_cart_ptr = NULL;
            goto fn_exit;
        }

        /* Create the topololgy structure */
        MPIR_CHKPMEM_MALLOC(cart_ptr, MPIR_Topology *, sizeof(MPIR_Topology),
                            mpi_errno, "cart_ptr", MPL_MEM_COMM);

        cart_ptr->kind = MPI_CART;
        cart_ptr->topo.cart.nnodes = newsize;
        cart_ptr->topo.cart.ndims = ndims;
        MPIR_CHKPMEM_MALLOC(cart_ptr->topo.cart.dims, int *, ndims * sizeof(int),
                            mpi_errno, "cart.dims", MPL_MEM_COMM);
        MPIR_CHKPMEM_MALLOC(cart_ptr->topo.cart.periodic, int *, ndims * sizeof(int),
                            mpi_errno, "cart.periodic", MPL_MEM_COMM);
        MPIR_CHKPMEM_MALLOC(cart_ptr->topo.cart.position, int *, ndims * sizeof(int),
                            mpi_errno, "cart.position", MPL_MEM_COMM);
        nranks = newsize;
        for (i = 0; i < ndims; i++) {
            cart_ptr->topo.cart.dims[i] = dims[i];
            cart_ptr->topo.cart.periodic[i] = periods[i];
            nranks = nranks / dims[i];
            /* FIXME: nranks could be zero (?) */
            cart_ptr->topo.cart.position[i] = rank / nranks;
            rank = rank % nranks;
        }
    }


    /* Place this topology onto the communicator */
    mpi_errno = MPIR_Topology_put(newcomm_ptr, cart_ptr);
    MPIR_ERR_CHECK(mpi_errno);

    *comm_cart_ptr = newcomm_ptr;

  fn_exit:
    return mpi_errno;

  fn_fail:
    MPIR_CHKPMEM_REAP();
    goto fn_exit;
}

int MPIR_Cart_get_impl(MPIR_Comm * comm_ptr, int maxdims, int dims[], int periods[], int coords[])
{
    int mpi_errno = MPI_SUCCESS;
    int i, n, *vals;

    MPIR_Topology *cart_ptr = MPIR_Topology_get(comm_ptr);
    n = cart_ptr->topo.cart.ndims;

    vals = cart_ptr->topo.cart.dims;
    for (i = 0; i < n; i++)
        *dims++ = *vals++;

    /* Get periods */
    vals = cart_ptr->topo.cart.periodic;
    for (i = 0; i < n; i++)
        *periods++ = *vals++;

    /* Get coords */
    vals = cart_ptr->topo.cart.position;
    for (i = 0; i < n; i++)
        *coords++ = *vals++;

    return mpi_errno;
}

int MPIR_Cart_map_impl(MPIR_Comm * comm_ptr, int ndims, const int dims[],
                       const int periods[], int *newrank)
{
    int rank, nranks, i, size, mpi_errno = MPI_SUCCESS;

    MPL_UNREFERENCED_ARG(periods);

    /* Determine number of processes needed for topology */
    if (ndims == 0) {
        nranks = 1;
    } else {
        nranks = dims[0];
        for (i = 1; i < ndims; i++)
            nranks *= dims[i];
    }
    size = comm_ptr->remote_size;

    /* Test that the communicator is large enough */
    MPIR_ERR_CHKANDJUMP2(size < nranks, mpi_errno, MPI_ERR_DIMS, "**topotoolarge",
                         "**topotoolarge %d %d", size, nranks);

    /* Am I in this range? */
    rank = comm_ptr->rank;
    if (rank < nranks)
        /* This relies on the ranks *not* being reordered by the current
         * Cartesian routines */
        *newrank = rank;
    else
        *newrank = MPI_UNDEFINED;

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIR_Cart_rank_impl(MPIR_Comm * comm_ptr, const int coords[], int *rank)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_Topology *cart_ptr;
    int i, ndims, coord, multiplier;

    cart_ptr = MPIR_Topology_get(comm_ptr);
    MPIR_ERR_CHKANDJUMP((!cart_ptr || cart_ptr->kind != MPI_CART),
                        mpi_errno, MPI_ERR_TOPOLOGY, "**notcarttopo");
    ndims = cart_ptr->topo.cart.ndims;
    *rank = 0;
    multiplier = 1;
    for (i = ndims - 1; i >= 0; i--) {
        coord = coords[i];
        if (cart_ptr->topo.cart.periodic[i]) {
            if (coord >= cart_ptr->topo.cart.dims[i])
                coord = coord % cart_ptr->topo.cart.dims[i];
            else if (coord < 0) {
                coord = coord % cart_ptr->topo.cart.dims[i];
                if (coord)
                    coord = cart_ptr->topo.cart.dims[i] + coord;
            }
        }
        *rank += multiplier * coord;
        multiplier *= cart_ptr->topo.cart.dims[i];
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIR_Cart_shift_impl(MPIR_Comm * comm_ptr, int direction, int disp, int *rank_source,
                         int *rank_dest)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Topology *cart_ptr;
    int i;
    int pos[MAX_CART_DIM];

    cart_ptr = MPIR_Topology_get(comm_ptr);
    MPIR_ERR_CHKANDJUMP((!cart_ptr || cart_ptr->kind != MPI_CART),
                        mpi_errno, MPI_ERR_TOPOLOGY, "**notcarttopo");

    MPIR_ERR_CHKANDJUMP((cart_ptr->topo.cart.ndims == 0), mpi_errno, MPI_ERR_TOPOLOGY,
                        "**dimszero");
    MPIR_ERR_CHKANDJUMP2((direction >= cart_ptr->topo.cart.ndims), mpi_errno, MPI_ERR_ARG,
                         "**dimsmany", "**dimsmany %d %d", cart_ptr->topo.cart.ndims, direction);

    /* Check for the case of a 0 displacement */
    if (disp == 0) {
        *rank_source = *rank_dest = comm_ptr->rank;
    } else {
        /* To support advanced implementations that support MPI_Cart_create,
         * we compute the new position and call PMPI_Cart_rank to get the
         * source and destination.  We could bypass that step if we know that
         * the mapping is trivial.  Copy the current position. */
        for (i = 0; i < cart_ptr->topo.cart.ndims; i++) {
            pos[i] = cart_ptr->topo.cart.position[i];
        }
        /* We must return MPI_PROC_NULL if shifted over the edge of a
         * non-periodic mesh */
        pos[direction] += disp;
        if (!cart_ptr->topo.cart.periodic[direction] &&
            (pos[direction] >= cart_ptr->topo.cart.dims[direction] || pos[direction] < 0)) {
            *rank_dest = MPI_PROC_NULL;
        } else {
            MPIR_Cart_rank_impl(comm_ptr, pos, rank_dest);
        }

        pos[direction] = cart_ptr->topo.cart.position[direction] - disp;
        if (!cart_ptr->topo.cart.periodic[direction] &&
            (pos[direction] >= cart_ptr->topo.cart.dims[direction] || pos[direction] < 0)) {
            *rank_source = MPI_PROC_NULL;
        } else {
            MPIR_Cart_rank_impl(comm_ptr, pos, rank_source);
        }
    }


  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIR_Cart_sub_impl(MPIR_Comm * comm_ptr, const int remain_dims[], MPIR_Comm ** p_newcomm_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    int all_false;
    MPIR_Comm *newcomm_ptr;
    MPIR_Topology *topo_ptr, *toponew_ptr;
    int ndims, key, color, ndims_in_subcomm, nnodes_in_subcomm, i, j, rank;
    MPIR_CHKPMEM_DECL(4);

    /* Check that the communicator already has a Cartesian topology */
    topo_ptr = MPIR_Topology_get(comm_ptr);

    MPIR_ERR_CHKANDJUMP(!topo_ptr, mpi_errno, MPI_ERR_TOPOLOGY, "**notopology");
    MPIR_ERR_CHKANDJUMP(topo_ptr->kind != MPI_CART, mpi_errno, MPI_ERR_TOPOLOGY, "**notcarttopo");

    ndims = topo_ptr->topo.cart.ndims;

    all_false = 1;      /* all entries in remain_dims are false */
    for (i = 0; i < ndims; i++) {
        if (remain_dims[i]) {
            /* any 1 is true, set flag to 0 and break */
            all_false = 0;
            break;
        }
    }

    if (all_false) {
        /* ndims=0, or all entries in remain_dims are false.
         * MPI 2.1 says return a 0D Cartesian topology. */
        mpi_errno = MPIR_Cart_create_impl(comm_ptr, 0, NULL, NULL, 0, p_newcomm_ptr);
        MPIR_ERR_CHECK(mpi_errno);
    } else {
        /* Determine the number of remaining dimensions */
        ndims_in_subcomm = 0;
        nnodes_in_subcomm = 1;
        for (i = 0; i < ndims; i++) {
            if (remain_dims[i]) {
                ndims_in_subcomm++;
                nnodes_in_subcomm *= topo_ptr->topo.cart.dims[i];
            }
        }

        /* Split this communicator.  Do this even if there are no remaining
         * dimensions so that the topology information is attached */
        key = 0;
        color = 0;
        for (i = 0; i < ndims; i++) {
            if (remain_dims[i]) {
                key = (key * topo_ptr->topo.cart.dims[i]) + topo_ptr->topo.cart.position[i];
            } else {
                color = (color * topo_ptr->topo.cart.dims[i]) + topo_ptr->topo.cart.position[i];
            }
        }
        mpi_errno = MPIR_Comm_split_impl(comm_ptr, color, key, &newcomm_ptr);
        MPIR_ERR_CHECK(mpi_errno);

        *p_newcomm_ptr = newcomm_ptr;

        /* Save the topology of this new communicator */
        MPIR_CHKPMEM_MALLOC(toponew_ptr, MPIR_Topology *, sizeof(MPIR_Topology),
                            mpi_errno, "toponew_ptr", MPL_MEM_COMM);

        toponew_ptr->kind = MPI_CART;
        toponew_ptr->topo.cart.ndims = ndims_in_subcomm;
        toponew_ptr->topo.cart.nnodes = nnodes_in_subcomm;
        if (ndims_in_subcomm) {
            MPIR_CHKPMEM_MALLOC(toponew_ptr->topo.cart.dims, int *,
                                ndims_in_subcomm * sizeof(int), mpi_errno, "cart.dims",
                                MPL_MEM_COMM);
            MPIR_CHKPMEM_MALLOC(toponew_ptr->topo.cart.periodic, int *,
                                ndims_in_subcomm * sizeof(int), mpi_errno, "cart.periodic",
                                MPL_MEM_COMM);
            MPIR_CHKPMEM_MALLOC(toponew_ptr->topo.cart.position, int *,
                                ndims_in_subcomm * sizeof(int), mpi_errno, "cart.position",
                                MPL_MEM_COMM);
        } else {
            toponew_ptr->topo.cart.dims = 0;
            toponew_ptr->topo.cart.periodic = 0;
            toponew_ptr->topo.cart.position = 0;
        }

        j = 0;
        for (i = 0; i < ndims; i++) {
            if (remain_dims[i]) {
                toponew_ptr->topo.cart.dims[j] = topo_ptr->topo.cart.dims[i];
                toponew_ptr->topo.cart.periodic[j] = topo_ptr->topo.cart.periodic[i];
                j++;
            }
        }

        /* Compute the position of this process in the new communicator */
        rank = newcomm_ptr->rank;
        for (i = 0; i < ndims_in_subcomm; i++) {
            nnodes_in_subcomm /= toponew_ptr->topo.cart.dims[i];
            toponew_ptr->topo.cart.position[i] = rank / nnodes_in_subcomm;
            rank = rank % nnodes_in_subcomm;
        }

        mpi_errno = MPIR_Topology_put(newcomm_ptr, toponew_ptr);
        if (mpi_errno)
            goto fn_fail;
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    MPIR_CHKPMEM_REAP();
    goto fn_exit;
}

int MPIR_Cartdim_get_impl(MPIR_Comm * comm_ptr, int *ndims)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_Topology *cart_ptr = MPIR_Topology_get(comm_ptr);
    MPIR_ERR_CHKANDJUMP((!cart_ptr || cart_ptr->kind != MPI_CART),
                        mpi_errno, MPI_ERR_TOPOLOGY, "**notcarttopo");
    *ndims = cart_ptr->topo.cart.ndims;

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIR_Dist_graph_neighbors_impl(MPIR_Comm * comm_ptr,
                                   int maxindegree, int sources[], int sourceweights[],
                                   int maxoutdegree, int destinations[], int destweights[])
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Topology *topo_ptr = NULL;

    topo_ptr = MPIR_Topology_get(comm_ptr);
    MPIR_ERR_CHKANDJUMP(!topo_ptr ||
                        topo_ptr->kind != MPI_DIST_GRAPH, mpi_errno, MPI_ERR_TOPOLOGY,
                        "**notdistgraphtopo");

    if (maxindegree > 0) {
        MPIR_Memcpy(sources, topo_ptr->topo.dist_graph.in, maxindegree * sizeof(int));
        if (sourceweights != MPI_UNWEIGHTED && topo_ptr->topo.dist_graph.is_weighted) {
            MPIR_Memcpy(sourceweights, topo_ptr->topo.dist_graph.in_weights,
                        maxindegree * sizeof(int));
        }
    }

    if (maxoutdegree > 0) {
        MPIR_Memcpy(destinations, topo_ptr->topo.dist_graph.out, maxoutdegree * sizeof(int));

        if (destweights != MPI_UNWEIGHTED && topo_ptr->topo.dist_graph.is_weighted) {
            MPIR_Memcpy(destweights, topo_ptr->topo.dist_graph.out_weights,
                        maxoutdegree * sizeof(int));
        }
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

/* any utility functions should go here, usually prefixed with PMPI_LOCAL to
 * correctly handle weak symbols and the profiling interface */

int MPIR_Dist_graph_neighbors_count_impl(MPIR_Comm * comm_ptr, int *indegree, int *outdegree,
                                         int *weighted)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Topology *topo_ptr = NULL;

    topo_ptr = MPIR_Topology_get(comm_ptr);
    MPIR_ERR_CHKANDJUMP(!topo_ptr ||
                        topo_ptr->kind != MPI_DIST_GRAPH, mpi_errno, MPI_ERR_TOPOLOGY,
                        "**notdistgraphtopo");
    *indegree = topo_ptr->topo.dist_graph.indegree;
    *outdegree = topo_ptr->topo.dist_graph.outdegree;
    *weighted = topo_ptr->topo.dist_graph.is_weighted;

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIR_Graph_create_impl(MPIR_Comm * comm_ptr, int nnodes,
                           const int indx[], const int edges[], int reorder,
                           MPIR_Comm ** comm_graph_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    int i, nedges;
    MPIR_Comm *newcomm_ptr = NULL;
    MPIR_Topology *graph_ptr = NULL;
    MPIR_CHKPMEM_DECL(3);

    /* Create a new communicator */
    if (reorder) {
        int nrank;

        /* Allow the cart map routine to remap the assignment of ranks to
         * processes */
        mpi_errno = MPIR_Graph_map_impl(comm_ptr, nnodes, indx, edges, &nrank);
        MPIR_ERR_CHECK(mpi_errno);
        /* Create the new communicator with split, since we need to reorder
         * the ranks (including the related internals, such as the connection
         * tables */
        mpi_errno = MPIR_Comm_split_impl(comm_ptr,
                                         nrank == MPI_UNDEFINED ? MPI_UNDEFINED : 1,
                                         nrank, &newcomm_ptr);
        MPIR_ERR_CHECK(mpi_errno);
    } else {
        /* Just use the first nnodes processes in the communicator */
        mpi_errno = MPII_Comm_copy((MPIR_Comm *) comm_ptr, nnodes, NULL, &newcomm_ptr);
        MPIR_ERR_CHECK(mpi_errno);
    }


    /* If this process is not in the resulting communicator, return a
     * null communicator and exit */
    if (!newcomm_ptr) {
        *comm_graph_ptr = NULL;
        goto fn_exit;
    }

    nedges = indx[nnodes - 1];
    MPIR_CHKPMEM_MALLOC(graph_ptr, MPIR_Topology *, sizeof(MPIR_Topology),
                        mpi_errno, "graph_ptr", MPL_MEM_COMM);

    graph_ptr->kind = MPI_GRAPH;
    graph_ptr->topo.graph.nnodes = nnodes;
    graph_ptr->topo.graph.nedges = nedges;
    MPIR_CHKPMEM_MALLOC(graph_ptr->topo.graph.index, int *,
                        nnodes * sizeof(int), mpi_errno, "graph.index", MPL_MEM_COMM);
    MPIR_CHKPMEM_MALLOC(graph_ptr->topo.graph.edges, int *,
                        nedges * sizeof(int), mpi_errno, "graph.edges", MPL_MEM_COMM);
    for (i = 0; i < nnodes; i++)
        graph_ptr->topo.graph.index[i] = indx[i];
    for (i = 0; i < nedges; i++)
        graph_ptr->topo.graph.edges[i] = edges[i];

    /* Finally, place the topology onto the new communicator and return the
     * handle */
    mpi_errno = MPIR_Topology_put(newcomm_ptr, graph_ptr);
    MPIR_ERR_CHECK(mpi_errno);

    *comm_graph_ptr = newcomm_ptr;

  fn_exit:
    return mpi_errno;
  fn_fail:
    MPIR_CHKPMEM_REAP();
    goto fn_exit;
}

int MPIR_Graph_get_impl(MPIR_Comm * comm_ptr, int maxindex, int maxedges, int indx[], int edges[])
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Topology *topo_ptr;
    int i, n, *vals;

    topo_ptr = MPIR_Topology_get(comm_ptr);

    MPIR_ERR_CHKANDJUMP((!topo_ptr ||
                         topo_ptr->kind != MPI_GRAPH), mpi_errno, MPI_ERR_TOPOLOGY,
                        "**notgraphtopo");
    MPIR_ERR_CHKANDJUMP3((topo_ptr->topo.graph.nnodes > maxindex), mpi_errno, MPI_ERR_ARG,
                         "**argtoosmall", "**argtoosmall %s %d %d", "maxindex", maxindex,
                         topo_ptr->topo.graph.nnodes);
    MPIR_ERR_CHKANDJUMP3((topo_ptr->topo.graph.nedges > maxedges), mpi_errno, MPI_ERR_ARG,
                         "**argtoosmall", "**argtoosmall %s %d %d", "maxedges", maxedges,
                         topo_ptr->topo.graph.nedges);

    /* Get index */
    n = topo_ptr->topo.graph.nnodes;
    vals = topo_ptr->topo.graph.index;
    for (i = 0; i < n; i++)
        *indx++ = *vals++;

    /* Get edges */
    n = topo_ptr->topo.graph.nedges;
    vals = topo_ptr->topo.graph.edges;
    for (i = 0; i < n; i++)
        *edges++ = *vals++;

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIR_Graph_map_impl(MPIR_Comm * comm_ptr, int nnodes,
                        const int indx[], const int edges[], int *newrank)
{
    MPL_UNREFERENCED_ARG(indx);
    MPL_UNREFERENCED_ARG(edges);

    /* This is the trivial version that does not remap any processes. */
    if (comm_ptr->rank < nnodes) {
        *newrank = comm_ptr->rank;
    } else {
        *newrank = MPI_UNDEFINED;
    }
    return MPI_SUCCESS;
}

int MPIR_Graph_neighbors_impl(MPIR_Comm * comm_ptr, int rank, int maxneighbors, int neighbors[])
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Topology *graph_ptr;
    int i, is, ie;

    graph_ptr = MPIR_Topology_get(comm_ptr);

    MPIR_ERR_CHKANDJUMP((!graph_ptr ||
                         graph_ptr->kind != MPI_GRAPH), mpi_errno, MPI_ERR_TOPOLOGY,
                        "**notgraphtopo");
    MPIR_ERR_CHKANDJUMP2((rank < 0 ||
                          rank >= graph_ptr->topo.graph.nnodes), mpi_errno, MPI_ERR_RANK, "**rank",
                         "**rank %d %d", rank, graph_ptr->topo.graph.nnodes);

    /* Get location in edges array of the neighbors of the specified rank */
    if (rank == 0)
        is = 0;
    else
        is = graph_ptr->topo.graph.index[rank - 1];
    ie = graph_ptr->topo.graph.index[rank];

    /* Get neighbors */
    for (i = is; i < ie; i++)
        *neighbors++ = graph_ptr->topo.graph.edges[i];
  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIR_Graph_neighbors_count_impl(MPIR_Comm * comm_ptr, int rank, int *nneighbors)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Topology *graph_ptr;

    graph_ptr = MPIR_Topology_get(comm_ptr);

    MPIR_ERR_CHKANDJUMP((!graph_ptr ||
                         graph_ptr->kind != MPI_GRAPH), mpi_errno, MPI_ERR_TOPOLOGY,
                        "**notgraphtopo");
    MPIR_ERR_CHKANDJUMP2((rank < 0 ||
                          rank >= graph_ptr->topo.graph.nnodes), mpi_errno, MPI_ERR_RANK, "**rank",
                         "**rank %d %d", rank, graph_ptr->topo.graph.nnodes);

    if (rank == 0)
        *nneighbors = graph_ptr->topo.graph.index[rank];
    else
        *nneighbors = graph_ptr->topo.graph.index[rank] - graph_ptr->topo.graph.index[rank - 1];

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIR_Graphdims_get_impl(MPIR_Comm * comm_ptr, int *nnodes, int *nedges)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Topology *topo_ptr = MPIR_Topology_get(comm_ptr);
    MPIR_ERR_CHKANDJUMP((!topo_ptr || topo_ptr->kind != MPI_GRAPH),
                        mpi_errno, MPI_ERR_TOPOLOGY, "**notgraphtopo");
    *nnodes = topo_ptr->topo.graph.nnodes;
    *nedges = topo_ptr->topo.graph.nedges;

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIR_Topo_test_impl(MPIR_Comm * comm_ptr, int *status)
{
    MPIR_Topology *topo_ptr = MPIR_Topology_get(comm_ptr);
    if (topo_ptr) {
        *status = (int) (topo_ptr->kind);
    } else {
        *status = MPI_UNDEFINED;
    }
    return MPI_SUCCESS;
}
