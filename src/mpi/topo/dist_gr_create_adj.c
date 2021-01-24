/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Dist_graph_create_adjacent */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Dist_graph_create_adjacent = PMPI_Dist_graph_create_adjacent
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Dist_graph_create_adjacent  MPI_Dist_graph_create_adjacent
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Dist_graph_create_adjacent as PMPI_Dist_graph_create_adjacent
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPI_Dist_graph_create_adjacent(MPI_Comm comm_old, int indegree, const int sources[],
                                   const int sourceweights[], int outdegree,
                                   const int destinations[], const int destweights[],
                                   MPI_Info info, int reorder, MPI_Comm * comm_dist_graph)
    __attribute__ ((weak, alias("PMPI_Dist_graph_create_adjacent")));
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Dist_graph_create_adjacent
#define MPI_Dist_graph_create_adjacent PMPI_Dist_graph_create_adjacent
#endif

/*@
MPI_Dist_graph_create_adjacent - returns a handle to a new communicator to
which the distributed graph topology information is attached.

Input Parameters:
+ comm_old - input communicator (handle)
. indegree - size of sources and sourceweights arrays (non-negative integer)
. sources - ranks of processes for which the calling process is a
            destination (array of non-negative integers)
. sourceweights - weights of the edges into the calling
                  process (array of non-negative integers or MPI_UNWEIGHTED)
. outdegree - size of destinations and destweights arrays (non-negative integer)
. destinations - ranks of processes for which the calling process is a
                 source (array of non-negative integers)
. destweights - weights of the edges out of the calling process
                (array of non-negative integers or MPI_UNWEIGHTED)
. info - hints on optimization and interpretation of weights (handle)
- reorder - the ranks may be reordered (true) or not (false) (logical)

Output Parameters:
. comm_dist_graph - communicator with distributed graph topology (handle)

.N ThreadSafe

.N Fortran

.N Errors
.N MPI_SUCCESS
.N MPI_ERR_ARG
.N MPI_ERR_OTHER
@*/
int MPI_Dist_graph_create_adjacent(MPI_Comm comm_old,
                                   int indegree, const int sources[],
                                   const int sourceweights[],
                                   int outdegree, const int destinations[],
                                   const int destweights[],
                                   MPI_Info info, int reorder, MPI_Comm * comm_dist_graph)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Comm *comm_ptr = NULL;
    MPIR_FUNC_TERSE_STATE_DECL(MPID_STATE_MPI_DIST_GRAPH_CREATE_ADJACENT);

    MPIR_ERRTEST_INITIALIZED_ORDIE();

    MPID_THREAD_CS_ENTER(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    MPIR_FUNC_TERSE_ENTER(MPID_STATE_MPI_DIST_GRAPH_CREATE_ADJACENT);

    /* Validate parameters, especially handles needing to be converted */
#ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            MPIR_ERRTEST_COMM(comm_old, mpi_errno);
            MPIR_ERRTEST_INFO_OR_NULL(info, mpi_errno);
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
            if (mpi_errno != MPI_SUCCESS)
                goto fn_fail;
            /* If comm_ptr is not valid, it will be reset to null */
            if (comm_ptr) {
                MPIR_ERRTEST_COMM_INTRA(comm_ptr, mpi_errno);
            }

            MPIR_ERRTEST_ARGNEG(indegree, "indegree", mpi_errno);
            MPIR_ERRTEST_ARGNEG(outdegree, "outdegree", mpi_errno);

            if (indegree > 0) {
                MPIR_ERRTEST_ARGNULL(sources, "sources", mpi_errno);
                if (sourceweights == MPI_UNWEIGHTED && destweights != MPI_UNWEIGHTED) {
                    MPIR_ERR_SET(mpi_errno, MPI_ERR_TOPOLOGY, "**unweightedboth");
                    goto fn_fail;
                }
                /* TODO check ranges for array elements too (**argarrayneg / **rankarray) */
            }
            if (outdegree > 0) {
                MPIR_ERRTEST_ARGNULL(destinations, "destinations", mpi_errno);
                if (destweights == MPI_UNWEIGHTED && sourceweights != MPI_UNWEIGHTED) {
                    MPIR_ERR_SET(mpi_errno, MPI_ERR_TOPOLOGY, "**unweightedboth");
                    goto fn_fail;
                }
            }
            MPIR_ERRTEST_ARGNULL(comm_dist_graph, "comm_dist_graph", mpi_errno);
        }
        MPID_END_ERROR_CHECKS;
    }
#endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */
    MPIR_Comm *newcomm_ptr = NULL;
    int mpi_errno = MPIR_Dist_graph_create_adjacent_impl(comm_ptr, indegree, sources, sourceweights,
                                                         outdegree, destinations, destweights,
                                                         info_ptr, reorder, &newcomm_ptr);
    if (mpi_errno)
        goto fn_fail;
    if (newcomm_ptr)
        MPIR_OBJ_PUBLISH_HANDLE(*comm_dist_graph, newcomm_ptr->handle);

    /* ... end of body of routine ... */
  fn_exit:
    MPIR_FUNC_TERSE_EXIT(MPID_STATE_MPI_DIST_GRAPH_CREATE_ADJACENT);
    MPID_THREAD_CS_EXIT(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    return mpi_errno;

    /* --BEGIN ERROR HANDLING-- */
  fn_fail:
#ifdef HAVE_ERROR_CHECKING
    mpi_errno =
        MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, __func__, __LINE__, MPI_ERR_OTHER,
                             "**mpi_dist_graph_create_adjacent",
                             "**mpi_dist_graph_create_adjacent %C %d %p %p %d %p %p %I %d %p",
                             comm_old, indegree, sources, sourceweights, outdegree, destinations,
                             destweights, info, reorder, comm_dist_graph);
#endif
    mpi_errno = MPIR_Err_return_comm(comm_ptr, __func__, mpi_errno);
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}
