/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2009 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Dist_graph_create */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Dist_graph_create = PMPI_Dist_graph_create
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Dist_graph_create  MPI_Dist_graph_create
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Dist_graph_create as PMPI_Dist_graph_create
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPI_Dist_graph_create(MPI_Comm comm_old, int n, const int sources[], const int degrees[],
                          const int destinations[], const int weights[], MPI_Info info,
                          int reorder, MPI_Comm * comm_dist_graph)
    __attribute__ ((weak, alias("PMPI_Dist_graph_create")));
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Dist_graph_create
#define MPI_Dist_graph_create PMPI_Dist_graph_create
/* any utility functions should go here, usually prefixed with PMPI_LOCAL to
 * correctly handle weak symbols and the profiling interface */
#endif

#undef FUNCNAME
#define FUNCNAME MPI_Dist_graph_create
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
/*@
MPI_Dist_graph_create - MPI_DIST_GRAPH_CREATE returns a handle to a new
communicator to which the distributed graph topology information is
attached.

Input Parameters:
+ comm_old - input communicator (handle)
. n - number of source nodes for which this process specifies edges
  (non-negative integer)
. sources - array containing the n source nodes for which this process
  specifies edges (array of non-negative integers)
. degrees - array specifying the number of destinations for each source node
  in the source node array (array of non-negative integers)
. destinations - destination nodes for the source nodes in the source node
  array (array of non-negative integers)
. weights - weights for source to destination edges (array of non-negative
  integers or MPI_UNWEIGHTED)
. info - hints on optimization and interpretation of weights (handle)
- reorder - the process may be reordered (true) or not (false) (logical)

Output Parameters:
. comm_dist_graph - communicator with distributed graph topology added (handle)

.N ThreadSafe

.N Fortran

.N Errors
.N MPI_SUCCESS
.N MPI_ERR_ARG
.N MPI_ERR_OTHER
@*/
int MPI_Dist_graph_create(MPI_Comm comm_old, int n, const int sources[],
                          const int degrees[], const int destinations[],
                          const int weights[],
                          MPI_Info info, int reorder, MPI_Comm * comm_dist_graph)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Comm *comm_ptr = NULL;
    MPIR_Comm *comm_dist_graph_ptr = NULL;
    MPIR_Request **reqs = NULL;
    MPIR_Topology *topo_ptr = NULL;
    MPII_Dist_graph_topology *dist_graph_ptr = NULL;
    int i;
    int j;
    int idx;
    int comm_size = 0;
    int in_capacity;
    int out_capacity;
    int **rout = NULL;
    int **rin = NULL;
    int *rin_sizes;
    int *rout_sizes;
    int *rin_idx;
    int *rout_idx;
    int *rs;
    int in_out_peers[2] = { -1, -1 };
    MPIR_Errflag_t errflag = MPIR_ERR_NONE;
    MPIR_CHKLMEM_DECL(9);
    MPIR_CHKPMEM_DECL(1);
    MPIR_FUNC_TERSE_STATE_DECL(MPID_STATE_MPI_DIST_GRAPH_CREATE);

    MPIR_ERRTEST_INITIALIZED_ORDIE();

    MPID_THREAD_CS_ENTER(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    MPIR_FUNC_TERSE_ENTER(MPID_STATE_MPI_DIST_GRAPH_CREATE);

    /* Validate parameters, especially handles needing to be converted */
#ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            MPIR_ERRTEST_COMM(comm_old, mpi_errno);
            MPIR_ERRTEST_INFO_OR_NULL(info, mpi_errno);
            if (mpi_errno != MPI_SUCCESS)
                goto fn_fail;
        }
        MPID_END_ERROR_CHECKS;
    }
#endif

    /* Convert MPI object handles to object pointers */
    MPIR_Comm_get_ptr(comm_old, comm_ptr);

    /* Validate parameters and objects (post conversion) */
#ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            /* Validate comm_ptr */
            MPIR_Comm_valid_ptr(comm_ptr, mpi_errno, FALSE);
            /* If comm_ptr is not valid, it will be reset to null */
            if (comm_ptr) {
                MPIR_ERRTEST_COMM_INTRA(comm_ptr, mpi_errno);
            }

            MPIR_ERRTEST_ARGNEG(n, "n", mpi_errno);
            if (n > 0) {
                int have_degrees = 0;
                MPIR_ERRTEST_ARGNULL(sources, "sources", mpi_errno);
                MPIR_ERRTEST_ARGNULL(degrees, "degrees", mpi_errno);
                for (i = 0; i < n; ++i) {
                    if (degrees[i]) {
                        have_degrees = 1;
                        break;
                    }
                }
                if (have_degrees) {
                    MPIR_ERRTEST_ARGNULL(destinations, "destinations", mpi_errno);
                    if (weights != MPI_UNWEIGHTED)
                        MPIR_ERRTEST_ARGNULL(weights, "weights", mpi_errno);
                }
            }

            if (mpi_errno != MPI_SUCCESS)
                goto fn_fail;
        }
        MPID_END_ERROR_CHECKS;
    }
#endif /* HAVE_ERROR_CHECKING */


    /* ... body of routine ...  */
    /* Implementation based on Torsten Hoefler's reference implementation
     * attached to MPI-2.2 ticket #33. */
    *comm_dist_graph = MPI_COMM_NULL;

    comm_size = comm_ptr->local_size;

    /* following the spirit of the old topo interface, attributes do not
     * propagate to the new communicator (see MPI-2.1 pp. 243 line 11) */
    mpi_errno = MPII_Comm_copy(comm_ptr, comm_size, &comm_dist_graph_ptr);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);
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
    idx = 0;
    for (i = 0; i < n; ++i) {
        MPIR_Assert(sources[i] < comm_size);
        for (j = 0; j < degrees[i]; ++j) {
            MPIR_Assert(destinations[idx] < comm_size);
            /* rout_sizes[i] is twice as long as the number of edges to be
             * sent to rank i by this process */
            rout_sizes[sources[i]] += 2;
            rin_sizes[destinations[idx]] += 2;
            ++idx;
        }
    }

    /* allocate arrays */
    for (i = 0; i < comm_size; ++i) {
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
    for (i = 0; i < n; ++i) {
        /* TODO add this assert as proper error checking above */
        int s_rank = sources[i];
        MPIR_Assert(s_rank < comm_size);
        MPIR_Assert(s_rank >= 0);

        for (j = 0; j < degrees[i]; ++j) {
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

    for (i = 0; i < comm_size; ++i) {
        /* sanity check that all arrays are fully populated */
        MPIR_Assert(rin_idx[i] == rin_sizes[i]);
        MPIR_Assert(rout_idx[i] == rout_sizes[i]);
    }

    MPIR_CHKLMEM_MALLOC(rs, int *, 2 * comm_size * sizeof(int), mpi_errno, "red-scat source buffer",
                        MPL_MEM_COMM);
    for (i = 0; i < comm_size; ++i) {
        rs[2 * i] = (rin_sizes[i] ? 1 : 0);
        rs[2 * i + 1] = (rout_sizes[i] ? 1 : 0);
    }

    /* compute the number of peers I will recv from */
    mpi_errno =
        MPIR_Reduce_scatter_block(rs, in_out_peers, 2, MPI_INT, MPI_SUM, comm_ptr, &errflag);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);
    MPIR_ERR_CHKANDJUMP(errflag, mpi_errno, MPI_ERR_OTHER, "**coll_fail");

    MPIR_Assert(in_out_peers[0] <= comm_size && in_out_peers[0] >= 0);
    MPIR_Assert(in_out_peers[1] <= comm_size && in_out_peers[1] >= 0);

    idx = 0;
    /* must be 2*comm_size requests because we will possibly send inbound and
     * outbound edges to everyone in our communicator */
    MPIR_CHKLMEM_MALLOC(reqs, MPIR_Request **, 2 * comm_size * sizeof(MPIR_Request *), mpi_errno,
                        "temp request array", MPL_MEM_COMM);
    for (i = 0; i < comm_size; ++i) {
        if (rin_sizes[i]) {
            /* send edges where i is a destination to process i */
            mpi_errno =
                MPIC_Isend(&rin[i][0], rin_sizes[i], MPI_INT, i, MPIR_TOPO_A_TAG, comm_ptr,
                           &reqs[idx++], &errflag);
            if (mpi_errno)
                MPIR_ERR_POP(mpi_errno);
        }
        if (rout_sizes[i]) {
            /* send edges where i is a source to process i */
            mpi_errno =
                MPIC_Isend(&rout[i][0], rout_sizes[i], MPI_INT, i, MPIR_TOPO_B_TAG, comm_ptr,
                           &reqs[idx++], &errflag);
            if (mpi_errno)
                MPIR_ERR_POP(mpi_errno);
        }
    }
    MPIR_Assert(idx <= (2 * comm_size));

    /* Create the topology structure */
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
    in_capacity = 10;   /* arbitrary */
    dist_graph_ptr->in = MPL_malloc(in_capacity * sizeof(int), MPL_MEM_COMM);
    if (dist_graph_ptr->is_weighted) {
        dist_graph_ptr->in_weights = MPL_malloc(in_capacity * sizeof(int), MPL_MEM_COMM);
        MPIR_Assert(dist_graph_ptr->in_weights != NULL);
    }
    out_capacity = 10;  /* arbitrary */
    dist_graph_ptr->out = MPL_malloc(out_capacity * sizeof(int), MPL_MEM_COMM);
    if (dist_graph_ptr->is_weighted) {
        dist_graph_ptr->out_weights = MPL_malloc(out_capacity * sizeof(int), MPL_MEM_COMM);
        MPIR_Assert(dist_graph_ptr->out_weights);
    }

    for (i = 0; i < in_out_peers[0]; ++i) {
        MPI_Status status;
        MPI_Aint count;
        int *buf;
        /* receive inbound edges */
        mpi_errno = MPIC_Probe(MPI_ANY_SOURCE, MPIR_TOPO_A_TAG, comm_old, &status);
        if (mpi_errno)
            MPIR_ERR_POP(mpi_errno);
        MPIR_Get_count_impl(&status, MPI_INT, &count);
        /* can't use CHKLMEM macros b/c we are in a loop */
        /* FIXME: Why not - there is only one allocated at a time. Is it only
         * that there is no defined macro to pop and free an item? */
        buf = MPL_malloc(count * sizeof(int), MPL_MEM_COMM);
        MPIR_ERR_CHKANDJUMP(!buf, mpi_errno, MPI_ERR_OTHER, "**nomem");

        mpi_errno =
            MPIC_Recv(buf, count, MPI_INT, MPI_ANY_SOURCE, MPIR_TOPO_A_TAG, comm_ptr,
                      MPI_STATUS_IGNORE, &errflag);
        /* FIXME: buf is never freed on error! */
        if (mpi_errno)
            MPIR_ERR_POP(mpi_errno);

        for (j = 0; j < count / 2; ++j) {
            int deg = dist_graph_ptr->indegree++;
            if (deg >= in_capacity) {
                in_capacity *= 2;
                /* FIXME: buf is never freed on error! */
                MPIR_REALLOC_ORJUMP(dist_graph_ptr->in, in_capacity * sizeof(int), MPL_MEM_COMM,
                                    mpi_errno);
                if (dist_graph_ptr->is_weighted)
                    /* FIXME: buf is never freed on error! */
                    MPIR_REALLOC_ORJUMP(dist_graph_ptr->in_weights, in_capacity * sizeof(int),
                                        MPL_MEM_COMM, mpi_errno);
            }
            dist_graph_ptr->in[deg] = buf[2 * j];
            if (dist_graph_ptr->is_weighted)
                dist_graph_ptr->in_weights[deg] = buf[2 * j + 1];
        }
        MPL_free(buf);
    }

    for (i = 0; i < in_out_peers[1]; ++i) {
        MPI_Status status;
        MPI_Aint count;
        int *buf;
        /* receive outbound edges */
        mpi_errno = MPIC_Probe(MPI_ANY_SOURCE, MPIR_TOPO_B_TAG, comm_old, &status);
        if (mpi_errno)
            MPIR_ERR_POP(mpi_errno);
        MPIR_Get_count_impl(&status, MPI_INT, &count);
        /* can't use CHKLMEM macros b/c we are in a loop */
        /* Why not? */
        buf = MPL_malloc(count * sizeof(int), MPL_MEM_COMM);
        MPIR_ERR_CHKANDJUMP(!buf, mpi_errno, MPI_ERR_OTHER, "**nomem");

        mpi_errno =
            MPIC_Recv(buf, count, MPI_INT, MPI_ANY_SOURCE, MPIR_TOPO_B_TAG, comm_ptr,
                      MPI_STATUS_IGNORE, &errflag);
        /* FIXME: buf is never freed on error! */
        if (mpi_errno)
            MPIR_ERR_POP(mpi_errno);

        for (j = 0; j < count / 2; ++j) {
            int deg = dist_graph_ptr->outdegree++;
            if (deg >= out_capacity) {
                out_capacity *= 2;
                /* FIXME: buf is never freed on error! */
                MPIR_REALLOC_ORJUMP(dist_graph_ptr->out, out_capacity * sizeof(int), MPL_MEM_COMM,
                                    mpi_errno);
                if (dist_graph_ptr->is_weighted)
                    /* FIXME: buf is never freed on error! */
                    MPIR_REALLOC_ORJUMP(dist_graph_ptr->out_weights, out_capacity * sizeof(int),
                                        MPL_MEM_COMM, mpi_errno);
            }
            dist_graph_ptr->out[deg] = buf[2 * j];
            if (dist_graph_ptr->is_weighted)
                dist_graph_ptr->out_weights[deg] = buf[2 * j + 1];
        }
        MPL_free(buf);
    }

    mpi_errno = MPIC_Waitall(idx, reqs, MPI_STATUSES_IGNORE, &errflag);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

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
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

    MPIR_CHKPMEM_COMMIT();

    MPIR_OBJ_PUBLISH_HANDLE(*comm_dist_graph, comm_dist_graph_ptr->handle);

    /* ... end of body of routine ... */

  fn_exit:
    for (i = 0; i < comm_size; ++i) {
        if (rin[i])
            MPL_free(rin[i]);
        if (rout[i])
            MPL_free(rout[i]);
    }

    MPIR_CHKLMEM_FREEALL();

    MPIR_FUNC_TERSE_EXIT(MPID_STATE_MPI_DIST_GRAPH_CREATE);
    MPID_THREAD_CS_EXIT(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    return mpi_errno;

    /* --BEGIN ERROR HANDLING-- */
  fn_fail:
    if (dist_graph_ptr && dist_graph_ptr->in)
        MPL_free(dist_graph_ptr->in);
    if (dist_graph_ptr && dist_graph_ptr->in_weights)
        MPL_free(dist_graph_ptr->in_weights);
    if (dist_graph_ptr && dist_graph_ptr->out)
        MPL_free(dist_graph_ptr->out);
    if (dist_graph_ptr && dist_graph_ptr->out_weights)
        MPL_free(dist_graph_ptr->out_weights);
    MPIR_CHKPMEM_REAP();
#ifdef HAVE_ERROR_CHECKING
    mpi_errno =
        MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER,
                             "**mpi_dist_graph_create",
                             "**mpi_dist_graph_create %C %d %p %p %p %p %I %d %p", comm_old, n,
                             sources, degrees, destinations, weights, info, reorder,
                             comm_dist_graph);
#endif
    mpi_errno = MPIR_Err_return_comm(comm_ptr, FCNAME, mpi_errno);
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}
