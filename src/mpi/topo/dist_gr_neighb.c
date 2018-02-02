/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2009 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Dist_graph_neighbors */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Dist_graph_neighbors = PMPI_Dist_graph_neighbors
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Dist_graph_neighbors  MPI_Dist_graph_neighbors
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Dist_graph_neighbors as PMPI_Dist_graph_neighbors
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPI_Dist_graph_neighbors(MPI_Comm comm, int maxindegree, int sources[], int sourceweights[],
                             int maxoutdegree, int destinations[], int destweights[])
    __attribute__ ((weak, alias("PMPI_Dist_graph_neighbors")));
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Dist_graph_neighbors
#define MPI_Dist_graph_neighbors PMPI_Dist_graph_neighbors
/* any utility functions should go here, usually prefixed with PMPI_LOCAL to
 * correctly handle weak symbols and the profiling interface */

#undef FUNCNAME
#define FUNCNAME MPIR_Dist_graph_neighbors_impl
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
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

    MPIR_Memcpy(sources, topo_ptr->topo.dist_graph.in, maxindegree * sizeof(int));
    MPIR_Memcpy(destinations, topo_ptr->topo.dist_graph.out, maxoutdegree * sizeof(int));

    if (sourceweights != MPI_UNWEIGHTED && topo_ptr->topo.dist_graph.is_weighted) {
        MPIR_Memcpy(sourceweights, topo_ptr->topo.dist_graph.in_weights, maxindegree * sizeof(int));
    }
    if (destweights != MPI_UNWEIGHTED && topo_ptr->topo.dist_graph.is_weighted) {
        MPIR_Memcpy(destweights, topo_ptr->topo.dist_graph.out_weights, maxoutdegree * sizeof(int));
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#endif

#undef FUNCNAME
#define FUNCNAME MPI_Dist_graph_neighbors
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
/*@
MPI_Dist_graph_neighbors - Provides adjacency information for a distributed graph topology.

Input Parameters:
+ comm - communicator with distributed graph topology (handle)
. maxindegree - size of sources and sourceweights arrays (non-negative integer)
- maxoutdegree - size of destinations and destweights arrays (non-negative integer)

Output Parameters:
+ sources - processes for which the calling process is a destination (array of non-negative integers)
. sourceweights - weights of the edges into the calling process (array of non-negative integers)
. destinations - processes for which the calling process is a source (array of non-negative integers)
- destweights - weights of the edges out of the calling process (array of non-negative integers)

.N ThreadSafe

.N Fortran

.N Errors
.N MPI_SUCCESS
@*/
int MPI_Dist_graph_neighbors(MPI_Comm comm,
                             int maxindegree, int sources[], int sourceweights[],
                             int maxoutdegree, int destinations[], int destweights[])
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Comm *comm_ptr = NULL;
    MPIR_FUNC_TERSE_STATE_DECL(MPID_STATE_MPI_DIST_GRAPH_NEIGHBORS);

    MPIR_ERRTEST_INITIALIZED_ORDIE();

    /* FIXME: Why does this routine need a CS */
    MPID_THREAD_CS_ENTER(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    MPIR_FUNC_TERSE_ENTER(MPID_STATE_MPI_DIST_GRAPH_NEIGHBORS);

    /* Validate parameters, especially handles needing to be converted */
#ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            MPIR_ERRTEST_COMM(comm, mpi_errno);
        }
        MPID_END_ERROR_CHECKS;
    }
#endif

    /* Convert MPI object handles to object pointers */
    MPIR_Comm_get_ptr(comm, comm_ptr);

    /* Validate parameters */
#ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            MPIR_Topology *topo_ptr = NULL;
            topo_ptr = MPIR_Topology_get(comm_ptr);
            MPIR_ERR_CHKANDJUMP(!topo_ptr ||
                                topo_ptr->kind != MPI_DIST_GRAPH, mpi_errno, MPI_ERR_TOPOLOGY,
                                "**notdistgraphtopo");

            MPIR_ERRTEST_ARGNEG(maxindegree, "maxindegree", mpi_errno);
            MPIR_ERRTEST_ARGNEG(maxoutdegree, "maxoutdegree", mpi_errno);
            MPIR_ERR_CHKANDJUMP3((maxindegree < topo_ptr->topo.dist_graph.indegree), mpi_errno,
                                 MPI_ERR_ARG, "**argtoosmall", "**argtoosmall %s %d %d",
                                 "maxindegree", maxindegree, topo_ptr->topo.dist_graph.indegree);
            MPIR_ERR_CHKANDJUMP3((maxoutdegree < topo_ptr->topo.dist_graph.outdegree), mpi_errno,
                                 MPI_ERR_ARG, "**argtoosmall", "**argtoosmall %s %d %d",
                                 "maxoutdegree", maxoutdegree, topo_ptr->topo.dist_graph.outdegree);
        }
        MPID_END_ERROR_CHECKS;
    }
#endif /* HAVE_ERROR_CHECKING */


    /* ... body of routine ...  */

    mpi_errno = MPIR_Dist_graph_neighbors_impl(comm_ptr,
                                               maxindegree, sources, sourceweights,
                                               maxoutdegree, destinations, destweights);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

    /* ... end of body of routine ... */

  fn_exit:
    MPIR_FUNC_TERSE_EXIT(MPID_STATE_MPI_DIST_GRAPH_NEIGHBORS);
    MPID_THREAD_CS_EXIT(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
#ifdef HAVE_ERROR_CHECKING
    mpi_errno =
        MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER,
                             "**mpi_dist_graph_neighbors",
                             "**mpi_dist_graph_neighbors %C %d %p %p %d %p %p", comm, maxindegree,
                             sources, sourceweights, maxoutdegree, destinations, destweights);
#endif
    mpi_errno = MPIR_Err_return_comm(comm_ptr, FCNAME, mpi_errno);
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}
