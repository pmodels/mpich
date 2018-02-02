/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2009 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Dist_graph_neighbors_count */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Dist_graph_neighbors_count = PMPI_Dist_graph_neighbors_count
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Dist_graph_neighbors_count  MPI_Dist_graph_neighbors_count
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Dist_graph_neighbors_count as PMPI_Dist_graph_neighbors_count
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPI_Dist_graph_neighbors_count(MPI_Comm comm, int *indegree, int *outdegree, int *weighted)
    __attribute__ ((weak, alias("PMPI_Dist_graph_neighbors_count")));
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Dist_graph_neighbors_count
#define MPI_Dist_graph_neighbors_count PMPI_Dist_graph_neighbors_count
/* any utility functions should go here, usually prefixed with PMPI_LOCAL to
 * correctly handle weak symbols and the profiling interface */

#undef FUNCNAME
#define FUNCNAME MPIR_Dist_graph_neighbors_count_impl
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
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

#endif

#undef FUNCNAME
#define FUNCNAME MPI_Dist_graph_neighbors_count
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
/*@
MPI_Dist_graph_neighbors_count - Provides adjacency information for a distributed graph topology.

Input Parameters:
. comm - communicator with distributed graph topology (handle)

Output Parameters:
+ indegree - number of edges into this process (non-negative integer)
. outdegree - number of edges out of this process (non-negative integer)
- weighted - false if MPI_UNWEIGHTED was supplied during creation, true otherwise (logical)

.N ThreadSafe

.N Fortran

.N Errors
.N MPI_SUCCESS
@*/
int MPI_Dist_graph_neighbors_count(MPI_Comm comm, int *indegree, int *outdegree, int *weighted)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Comm *comm_ptr = NULL;
    MPIR_FUNC_TERSE_STATE_DECL(MPID_STATE_MPI_DIST_GRAPH_NEIGHBORS_COUNT);

    MPIR_ERRTEST_INITIALIZED_ORDIE();

    /* FIXME: Why does this routine require a CS? */
    MPID_THREAD_CS_ENTER(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    MPIR_FUNC_TERSE_ENTER(MPID_STATE_MPI_DIST_GRAPH_NEIGHBORS_COUNT);

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

    /* Validate parameters and objects (post conversion) */
#ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            MPIR_Comm_valid_ptr(comm_ptr, mpi_errno, TRUE);
            if (mpi_errno != MPI_SUCCESS)
                goto fn_fail;

            MPIR_ERRTEST_ARGNULL(indegree, "indegree", mpi_errno);
            MPIR_ERRTEST_ARGNULL(outdegree, "outdegree", mpi_errno);
            MPIR_ERRTEST_ARGNULL(weighted, "weighted", mpi_errno);
        }
        MPID_END_ERROR_CHECKS;
    }
#endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */

    mpi_errno = MPIR_Dist_graph_neighbors_count_impl(comm_ptr, indegree, outdegree, weighted);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

    /* ... end of body of routine ... */

  fn_exit:
    MPIR_FUNC_TERSE_EXIT(MPID_STATE_MPI_DIST_GRAPH_NEIGHBORS_COUNT);
    MPID_THREAD_CS_EXIT(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
#ifdef HAVE_ERROR_CHECKING
    {
        mpi_errno =
            MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER,
                                 "**mpi_dist_graph_neighbors_count",
                                 "**mpi_dist_graph_neighbors_count %C %p %p %p", comm, indegree,
                                 outdegree, weighted);
    }
#endif
    mpi_errno = MPIR_Err_return_comm(comm_ptr, FCNAME, mpi_errno);
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}
