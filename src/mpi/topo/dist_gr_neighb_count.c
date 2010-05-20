/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2009 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"
#include "topo.h"

/* -- Begin Profiling Symbol Block for routine MPI_Dist_graph_neighbors_count */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Dist_graph_neighbors_count = PMPI_Dist_graph_neighbors_count
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Dist_graph_neighbors_count  MPI_Dist_graph_neighbors_count
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Dist_graph_neighbors_count as PMPI_Dist_graph_neighbors_count
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Dist_graph_neighbors_count
#define MPI_Dist_graph_neighbors_count PMPI_Dist_graph_neighbors_count
/* any utility functions should go here, usually prefixed with PMPI_LOCAL to
 * correctly handle weak symbols and the profiling interface */
#endif

#undef FUNCNAME
#define FUNCNAME MPI_Dist_graph_neighbors_count
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
/*@
MPI_Dist_graph_neighbors_count - Provides adjacency information for a distributed graph topology.

Input Parameters:
. comm - communicator with distributed graph topology (handle)

Output Parameter:
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
    MPID_Comm *comm_ptr = NULL;
    MPIR_Topology *topo_ptr = NULL;
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_DIST_GRAPH_NEIGHBORS_COUNT);

    MPIR_ERRTEST_INITIALIZED_ORDIE();

    /* FIXME: Why does this routine require a CS? */
    MPIU_THREAD_CS_ENTER(ALLFUNC,);
    MPID_MPI_FUNC_ENTER(MPID_STATE_MPI_DIST_GRAPH_NEIGHBORS_COUNT);

    /* Validate parameters, especially handles needing to be converted */
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            MPIR_ERRTEST_COMM(comm, mpi_errno);
            if (mpi_errno != MPI_SUCCESS) goto fn_fail;
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif

    /* Convert MPI object handles to object pointers */
    MPID_Comm_get_ptr(comm, comm_ptr);

    /* Validate parameters and objects (post conversion) */
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            MPID_Comm_valid_ptr(comm_ptr, mpi_errno);

            MPIR_ERRTEST_ARGNULL(indegree, "indegree", mpi_errno);
            MPIR_ERRTEST_ARGNULL(outdegree, "outdegree", mpi_errno);
            MPIR_ERRTEST_ARGNULL(weighted, "weighted", mpi_errno);
            if (mpi_errno != MPI_SUCCESS) goto fn_fail;
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */
    topo_ptr = MPIR_Topology_get(comm_ptr);
    MPIU_ERR_CHKANDJUMP(!topo_ptr || topo_ptr->kind != MPI_DIST_GRAPH, mpi_errno, MPIR_ERR_RECOVERABLE, "**notdistgraphtopo");
    *indegree = topo_ptr->topo.dist_graph.indegree;
    *outdegree = topo_ptr->topo.dist_graph.outdegree;
    *weighted = (topo_ptr->topo.dist_graph.in_weights || topo_ptr->topo.dist_graph.out_weights);

    /* ... end of body of routine ... */

  fn_exit:
    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_DIST_GRAPH_NEIGHBORS_COUNT);
    MPIU_THREAD_CS_EXIT(ALLFUNC,);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
#   ifdef HAVE_ERROR_CHECKING
    {
        mpi_errno = MPIR_Err_create_code(
            mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER,
            "**mpi_dist_graph_neighbors_count", "**mpi_dist_graph_neighbors_count %C %p %p %p",
            comm, indegree, outdegree, weighted);
    }
#   endif
    mpi_errno = MPIR_Err_return_comm(comm_ptr, FCNAME, mpi_errno);
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}

