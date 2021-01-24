/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
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
#endif

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
    MPIR_Comm *newcomm_ptr = NULL;
    int mpi_errno = MPIR_Dist_graph_create_impl(comm_ptr, n, sources, degrees, destinations,
                                                weights, info_ptr, reorder, &newcomm_ptr);
    if (mpi_errno)
        goto fn_fail;
    if (newcomm_ptr)
        MPIR_OBJ_PUBLISH_HANDLE(*comm_dist_graph, newcomm_ptr->handle);

    /* ... end of body of routine ... */

  fn_exit:
    MPIR_FUNC_TERSE_EXIT(MPID_STATE_MPI_DIST_GRAPH_CREATE);
    MPID_THREAD_CS_EXIT(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    return mpi_errno;

    /* --BEGIN ERROR HANDLING-- */
  fn_fail:
#ifdef HAVE_ERROR_CHECKING
    mpi_errno =
        MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, __func__, __LINE__, MPI_ERR_OTHER,
                             "**mpi_dist_graph_create",
                             "**mpi_dist_graph_create %C %d %p %p %p %p %I %d %p", comm_old, n,
                             sources, degrees, destinations, weights, info, reorder,
                             comm_dist_graph);
#endif
    mpi_errno = MPIR_Err_return_comm(comm_ptr, __func__, mpi_errno);
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}
