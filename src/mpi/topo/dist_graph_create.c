/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"

/* Implementation for MPI_Dist_graph_create and MPI_Dist_graph_create_adjacent */

int MPIR_Dist_graph_create_impl(MPIR_Comm * comm_ptr,
                                int n, const int sources[], const int degrees[],
                                const int destinations[], const int weights[],
                                MPIR_Info * info_ptr, int reorder,
                                MPIR_Comm ** p_comm_dist_graph_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    /* Implementation based on Torsten Hoefler's reference implementation
     * attached to MPI-2.2 ticket #33. */
    MPIR_Comm *comm_dist_graph_ptr = NULL;
    MPII_Dist_graph_topology *dist_graph_ptr = NULL;
    int **rin = NULL, *rin_sizes, *rin_idx;
    int **rout = NULL, *rout_sizes, *rout_idx;
    int *buf = NULL;

    int comm_size = comm_ptr->local_size;
    MPIR_CHKLMEM_DECL(8);
    MPIR_CHKPMEM_DECL(1);

    /* following the spirit of the old topo interface, attributes do not
     * propagate to the new communicator (see MPI-2.1 pp. 243 line 11) */
    mpi_errno = MPII_Comm_copy(comm_ptr, comm_size, NULL, &comm_dist_graph_ptr);
    MPIR_ERR_CHECK(mpi_errno);
    MPIR_Assert(comm_dist_graph_ptr != NULL);

    /* rin is an array of size comm_size containing pointers to arrays of
     * rin_sizes[x].  rin[x] is locally known number of edges into this process
     * from rank x.
     *
     * rout is an array of comm_size containing pointers to arrays of
     * rout_sizes[x].  rout[x] is the locally known number of edges out of this
     * process to rank x. */
    MPIR_CHKLMEM_MALLOC(rout, int **, comm_size * sizeof(int *), mpi_errno, "rout", MPL_MEM_COMM);
    MPIR_CHKLMEM_MALLOC(rin, int **, comm_size * sizeof(int *), mpi_errno, "rin", MPL_MEM_COMM);
    MPIR_CHKLMEM_MALLOC(rin_sizes, int *, comm_size * sizeof(int), mpi_errno, "rin_sizes",
                        MPL_MEM_COMM);
    MPIR_CHKLMEM_MALLOC(rout_sizes, int *, comm_size * sizeof(int), mpi_errno, "rout_sizes",
                        MPL_MEM_COMM);
    MPIR_CHKLMEM_MALLOC(rin_idx, int *, comm_size * sizeof(int), mpi_errno, "rin_idx",
                        MPL_MEM_COMM);
    MPIR_CHKLMEM_MALLOC(rout_idx, int *, comm_size * sizeof(int), mpi_errno, "rout_idx",
                        MPL_MEM_COMM);

    memset(rout, 0, comm_size * sizeof(int *));
    memset(rin, 0, comm_size * sizeof(int *));
    memset(rin_sizes, 0, comm_size * sizeof(int));
    memset(rout_sizes, 0, comm_size * sizeof(int));
    memset(rin_idx, 0, comm_size * sizeof(int));
    memset(rout_idx, 0, comm_size * sizeof(int));

    /* compute array sizes */
    int idx = 0;
    for (int i = 0; i < n; ++i) {
        MPIR_Assert(sources[i] < comm_size);
        for (int j = 0; j < degrees[i]; ++j) {
            MPIR_Assert(destinations[idx] < comm_size);
            /* rout_sizes[i] is twice as long as the number of edges to be
             * sent to rank i by this process */
            rout_sizes[sources[i]] += 2;
            rin_sizes[destinations[idx]] += 2;
            ++idx;
        }
    }

    /* allocate arrays */
    for (int i = 0; i < comm_size; ++i) {
        /* can't use CHKLMEM macros b/c we are in a loop */
        if (rin_sizes[i]) {
            rin[i] = MPL_malloc(rin_sizes[i] * sizeof(int), MPL_MEM_COMM);
        }
        if (rout_sizes[i]) {
            rout[i] = MPL_malloc(rout_sizes[i] * sizeof(int), MPL_MEM_COMM);
        }
    }

    /* populate arrays */
    idx = 0;
    for (int i = 0; i < n; ++i) {
        /* TODO add this assert as proper error checking above */
        int s_rank = sources[i];
        MPIR_Assert(s_rank < comm_size);
        MPIR_Assert(s_rank >= 0);

        for (int j = 0; j < degrees[i]; ++j) {
            int d_rank = destinations[idx];
            int weight = (weights == MPI_UNWEIGHTED ? 0 : weights[idx]);
            /* TODO add this assert as proper error checking above */
            MPIR_Assert(d_rank < comm_size);
            MPIR_Assert(d_rank >= 0);

            /* XXX DJG what about self-edges? do we need to drop one of these
             * cases when there is a self-edge to avoid double-counting? */

            /* rout[s][2*x] is the value of d for the j'th edge between (s,d)
             * with weight rout[s][2*x+1], where x is the current end of the
             * outgoing edge list for s.  x==(rout_idx[s]/2) */
            rout[s_rank][rout_idx[s_rank]++] = d_rank;
            rout[s_rank][rout_idx[s_rank]++] = weight;

            /* rin[d][2*x] is the value of s for the j'th edge between (s,d)
             * with weight rout[d][2*x+1], where x is the current end of the
             * incoming edge list for d.  x==(rin_idx[d]/2) */
            rin[d_rank][rin_idx[d_rank]++] = s_rank;
            rin[d_rank][rin_idx[d_rank]++] = weight;

            ++idx;
        }
    }

    for (int i = 0; i < comm_size; ++i) {
        /* sanity check that all arrays are fully populated */
        MPIR_Assert(rin_idx[i] == rin_sizes[i]);
        MPIR_Assert(rout_idx[i] == rout_sizes[i]);
    }

    int *rs;
    MPIR_CHKLMEM_MALLOC(rs, int *, 2 * comm_size * sizeof(int), mpi_errno, "red-scat source buffer",
                        MPL_MEM_COMM);
    for (int i = 0; i < comm_size; ++i) {
        rs[2 * i] = (rin_sizes[i] ? 1 : 0);
        rs[2 * i + 1] = (rout_sizes[i] ? 1 : 0);
    }

    /* compute the number of peers I will recv from */
    int in_out_peers[2] = { -1, 1 };
    MPIR_Errflag_t errflag = MPIR_ERR_NONE;
    mpi_errno =
        MPIR_Reduce_scatter_block(rs, in_out_peers, 2, MPI_INT, MPI_SUM, comm_ptr, &errflag);
    MPIR_ERR_CHECK(mpi_errno);
    MPIR_ERR_CHKANDJUMP(errflag, mpi_errno, MPI_ERR_OTHER, "**coll_fail");

    MPIR_Assert(in_out_peers[0] <= comm_size && in_out_peers[0] >= 0);
    MPIR_Assert(in_out_peers[1] <= comm_size && in_out_peers[1] >= 0);

    idx = 0;
    /* must be 2*comm_size requests because we will possibly send inbound and
     * outbound edges to everyone in our communicator */
    MPIR_Request **reqs;
    MPIR_CHKLMEM_MALLOC(reqs, MPIR_Request **, 2 * comm_size * sizeof(MPIR_Request *), mpi_errno,
                        "temp request array", MPL_MEM_COMM);
    for (int i = 0; i < comm_size; ++i) {
        if (rin_sizes[i]) {
            /* send edges where i is a destination to process i */
            mpi_errno =
                MPIC_Isend(&rin[i][0], rin_sizes[i], MPI_INT, i, MPIR_TOPO_A_TAG, comm_ptr,
                           &reqs[idx++], &errflag);
            MPIR_ERR_CHECK(mpi_errno);
        }
        if (rout_sizes[i]) {
            /* send edges where i is a source to process i */
            mpi_errno =
                MPIC_Isend(&rout[i][0], rout_sizes[i], MPI_INT, i, MPIR_TOPO_B_TAG, comm_ptr,
                           &reqs[idx++], &errflag);
            MPIR_ERR_CHECK(mpi_errno);
        }
    }
    MPIR_Assert(idx <= (2 * comm_size));

    /* Create the topology structure */
    MPIR_Topology *topo_ptr = NULL;
    MPIR_CHKPMEM_MALLOC(topo_ptr, MPIR_Topology *, sizeof(MPIR_Topology), mpi_errno, "topo_ptr",
                        MPL_MEM_COMM);
    topo_ptr->kind = MPI_DIST_GRAPH;
    dist_graph_ptr = &topo_ptr->topo.dist_graph;
    dist_graph_ptr->indegree = 0;
    dist_graph_ptr->in = NULL;
    dist_graph_ptr->in_weights = NULL;
    dist_graph_ptr->outdegree = 0;
    dist_graph_ptr->out = NULL;
    dist_graph_ptr->out_weights = NULL;
    dist_graph_ptr->is_weighted = (weights != MPI_UNWEIGHTED);

    /* can't use CHKPMEM macros for this b/c we need to realloc */
    int in_capacity = 10;       /* arbitrary */
    dist_graph_ptr->in = MPL_malloc(in_capacity * sizeof(int), MPL_MEM_COMM);
    if (dist_graph_ptr->is_weighted) {
        dist_graph_ptr->in_weights = MPL_malloc(in_capacity * sizeof(int), MPL_MEM_COMM);
        MPIR_Assert(dist_graph_ptr->in_weights != NULL);
    }
    int out_capacity = 10;      /* arbitrary */
    dist_graph_ptr->out = MPL_malloc(out_capacity * sizeof(int), MPL_MEM_COMM);
    if (dist_graph_ptr->is_weighted) {
        dist_graph_ptr->out_weights = MPL_malloc(out_capacity * sizeof(int), MPL_MEM_COMM);
        MPIR_Assert(dist_graph_ptr->out_weights);
    }

    for (int i = 0; i < in_out_peers[0]; ++i) {
        MPI_Status status;
        MPI_Aint count;
        /* receive inbound edges */
        mpi_errno = MPIC_Probe(MPI_ANY_SOURCE, MPIR_TOPO_A_TAG, comm_ptr->handle, &status);
        MPIR_ERR_CHECK(mpi_errno);
        MPIR_Get_count_impl(&status, MPI_INT, &count);

        buf = MPL_malloc(count * sizeof(int), MPL_MEM_COMM);
        MPIR_ERR_CHKANDJUMP(!buf, mpi_errno, MPI_ERR_OTHER, "**nomem");

        mpi_errno = MPIC_Recv(buf, count, MPI_INT, MPI_ANY_SOURCE, MPIR_TOPO_A_TAG,
                              comm_ptr, MPI_STATUS_IGNORE, &errflag);
        MPIR_ERR_CHECK(mpi_errno);

        for (int j = 0; j < count / 2; ++j) {
            int deg = dist_graph_ptr->indegree++;
            if (deg >= in_capacity) {
                in_capacity *= 2;
                MPIR_REALLOC_ORJUMP(dist_graph_ptr->in, in_capacity * sizeof(int), MPL_MEM_COMM,
                                    mpi_errno);
                if (dist_graph_ptr->is_weighted)
                    MPIR_REALLOC_ORJUMP(dist_graph_ptr->in_weights, in_capacity * sizeof(int),
                                        MPL_MEM_COMM, mpi_errno);
            }
            dist_graph_ptr->in[deg] = buf[2 * j];
            if (dist_graph_ptr->is_weighted)
                dist_graph_ptr->in_weights[deg] = buf[2 * j + 1];
        }
        MPL_free(buf);
        buf = NULL;
    }

    for (int i = 0; i < in_out_peers[1]; ++i) {
        MPI_Status status;
        MPI_Aint count;
        /* receive outbound edges */
        mpi_errno = MPIC_Probe(MPI_ANY_SOURCE, MPIR_TOPO_B_TAG, comm_ptr->handle, &status);
        MPIR_ERR_CHECK(mpi_errno);
        MPIR_Get_count_impl(&status, MPI_INT, &count);

        buf = MPL_malloc(count * sizeof(int), MPL_MEM_COMM);
        MPIR_ERR_CHKANDJUMP(!buf, mpi_errno, MPI_ERR_OTHER, "**nomem");

        mpi_errno = MPIC_Recv(buf, count, MPI_INT, MPI_ANY_SOURCE, MPIR_TOPO_B_TAG,
                              comm_ptr, MPI_STATUS_IGNORE, &errflag);
        MPIR_ERR_CHECK(mpi_errno);

        for (int j = 0; j < count / 2; ++j) {
            int deg = dist_graph_ptr->outdegree++;
            if (deg >= out_capacity) {
                out_capacity *= 2;
                MPIR_REALLOC_ORJUMP(dist_graph_ptr->out, out_capacity * sizeof(int), MPL_MEM_COMM,
                                    mpi_errno);
                if (dist_graph_ptr->is_weighted)
                    MPIR_REALLOC_ORJUMP(dist_graph_ptr->out_weights, out_capacity * sizeof(int),
                                        MPL_MEM_COMM, mpi_errno);
            }
            dist_graph_ptr->out[deg] = buf[2 * j];
            if (dist_graph_ptr->is_weighted)
                dist_graph_ptr->out_weights[deg] = buf[2 * j + 1];
        }
        MPL_free(buf);
        buf = NULL;
    }

    mpi_errno = MPIC_Waitall(idx, reqs, MPI_STATUSES_IGNORE, &errflag);
    MPIR_ERR_CHECK(mpi_errno);

    /* remove any excess memory allocation */
    MPIR_REALLOC_ORJUMP(dist_graph_ptr->in, dist_graph_ptr->indegree * sizeof(int), MPL_MEM_COMM,
                        mpi_errno);
    MPIR_REALLOC_ORJUMP(dist_graph_ptr->out, dist_graph_ptr->outdegree * sizeof(int), MPL_MEM_COMM,
                        mpi_errno);
    if (dist_graph_ptr->is_weighted) {
        MPIR_REALLOC_ORJUMP(dist_graph_ptr->in_weights, dist_graph_ptr->indegree * sizeof(int),
                            MPL_MEM_COMM, mpi_errno);
        MPIR_REALLOC_ORJUMP(dist_graph_ptr->out_weights, dist_graph_ptr->outdegree * sizeof(int),
                            MPL_MEM_COMM, mpi_errno);
    }

    mpi_errno = MPIR_Topology_put(comm_dist_graph_ptr, topo_ptr);
    MPIR_ERR_CHECK(mpi_errno);

    *p_comm_dist_graph_ptr = comm_dist_graph_ptr;

  fn_exit:
    for (int i = 0; i < comm_size; ++i) {
        MPL_free(rin[i]);
        MPL_free(rout[i]);
    }
    MPIR_CHKLMEM_FREEALL();
    return mpi_errno;
  fn_fail:
    if (buf) {
        MPL_free(buf);
    }
    if (dist_graph_ptr) {
        MPL_free(dist_graph_ptr->in);
        MPL_free(dist_graph_ptr->in_weights);
        MPL_free(dist_graph_ptr->out);
        MPL_free(dist_graph_ptr->out_weights);
    }
    MPIR_CHKPMEM_REAP();
    goto fn_exit;
}

int MPIR_Dist_graph_create_adjacent_impl(MPIR_Comm * comm_ptr,
                                         int indegree, const int sources[],
                                         const int sourceweights[], int outdegree,
                                         const int destinations[], const int destweights[],
                                         MPIR_Info * info_ptr, int reorder,
                                         MPIR_Comm ** comm_dist_graph_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_CHKPMEM_DECL(5);

    /* Implementation based on Torsten Hoefler's reference implementation
     * attached to MPI-2.2 ticket #33. */

    /* following the spirit of the old topo interface, attributes do not
     * propagate to the new communicator (see MPI-2.1 pp. 243 line 11) */
    mpi_errno = MPII_Comm_copy(comm_ptr, comm_ptr->local_size, NULL, comm_dist_graph_ptr);
    MPIR_ERR_CHECK(mpi_errno);

    /* Create the topology structure */
    MPIR_Topology *topo_ptr;
    MPIR_CHKPMEM_MALLOC(topo_ptr, MPIR_Topology *, sizeof(MPIR_Topology), mpi_errno, "topo_ptr",
                        MPL_MEM_COMM);
    topo_ptr->kind = MPI_DIST_GRAPH;
    MPII_Dist_graph_topology *dist_graph_ptr = &topo_ptr->topo.dist_graph;
    dist_graph_ptr->indegree = indegree;
    dist_graph_ptr->in = NULL;
    dist_graph_ptr->in_weights = NULL;
    dist_graph_ptr->outdegree = outdegree;
    dist_graph_ptr->out = NULL;
    dist_graph_ptr->out_weights = NULL;
    dist_graph_ptr->is_weighted = (sourceweights != MPI_UNWEIGHTED);

    if (indegree > 0) {
        MPIR_CHKPMEM_MALLOC(dist_graph_ptr->in, int *, indegree * sizeof(int), mpi_errno,
                            "dist_graph_ptr->in", MPL_MEM_COMM);
        MPIR_Memcpy(dist_graph_ptr->in, sources, indegree * sizeof(int));
        if (dist_graph_ptr->is_weighted) {
            MPIR_CHKPMEM_MALLOC(dist_graph_ptr->in_weights, int *, indegree * sizeof(int),
                                mpi_errno, "dist_graph_ptr->in_weights", MPL_MEM_COMM);
            MPIR_Memcpy(dist_graph_ptr->in_weights, sourceweights, indegree * sizeof(int));
        }
    }

    if (outdegree > 0) {
        MPIR_CHKPMEM_MALLOC(dist_graph_ptr->out, int *, outdegree * sizeof(int), mpi_errno,
                            "dist_graph_ptr->out", MPL_MEM_COMM);
        MPIR_Memcpy(dist_graph_ptr->out, destinations, outdegree * sizeof(int));
        if (dist_graph_ptr->is_weighted) {
            MPIR_CHKPMEM_MALLOC(dist_graph_ptr->out_weights, int *, outdegree * sizeof(int),
                                mpi_errno, "dist_graph_ptr->out_weights", MPL_MEM_COMM);
            MPIR_Memcpy(dist_graph_ptr->out_weights, destweights, outdegree * sizeof(int));
        }
    }

    mpi_errno = MPIR_Topology_put(*comm_dist_graph_ptr, topo_ptr);
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    return mpi_errno;
  fn_fail:
    MPIR_CHKPMEM_REAP();
    goto fn_exit;
}
