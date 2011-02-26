/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2009 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"
#include "topo.h"

/* -- Begin Profiling Symbol Block for routine MPI_Dist_graph_create_adjacent */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Dist_graph_create_adjacent = PMPI_Dist_graph_create_adjacent
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Dist_graph_create_adjacent  MPI_Dist_graph_create_adjacent
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Dist_graph_create_adjacent as PMPI_Dist_graph_create_adjacent
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Dist_graph_create_adjacent
#define MPI_Dist_graph_create_adjacent PMPI_Dist_graph_create_adjacent
/* any utility functions should go here, usually prefixed with PMPI_LOCAL to
 * correctly handle weak symbols and the profiling interface */
#endif

#undef FUNCNAME
#define FUNCNAME MPI_Dist_graph_create_adjacent
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
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

Output Parameter:
. comm_dist_graph - communicator with distributed graph topology (handle)

.N ThreadSafe

.N Fortran

.N Errors
.N MPI_SUCCESS
.N MPI_ERR_ARG
.N MPI_ERR_OTHER
@*/
int MPI_Dist_graph_create_adjacent(MPI_Comm comm_old,
                                   int indegree, int sources[], int sourceweights[],
                                   int outdegree, int destinations[], int destweights[],
                                   MPI_Info info, int reorder, MPI_Comm *comm_dist_graph)
{
    int       mpi_errno = MPI_SUCCESS;
    MPID_Comm *comm_ptr = NULL;
    MPID_Comm *comm_dist_graph_ptr = NULL;
    MPID_Info *info_ptr = NULL;
    MPIR_Topology *topo_ptr = NULL;
    MPIR_Dist_graph_topology *dist_graph_ptr = NULL;
    MPIU_CHKPMEM_DECL(5);
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_DIST_GRAPH_CREATE_ADJACENT);

    MPIR_ERRTEST_INITIALIZED_ORDIE();

    MPIU_THREAD_CS_ENTER(ALLFUNC,);
    MPID_MPI_FUNC_ENTER(MPID_STATE_MPI_DIST_GRAPH_CREATE_ADJACENT);

    /* Validate parameters, especially handles needing to be converted */
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            MPIR_ERRTEST_COMM(comm_old, mpi_errno);
            MPIR_ERRTEST_INFO_OR_NULL(info, mpi_errno);
            if (mpi_errno != MPI_SUCCESS) goto fn_fail;
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif

    /* Convert MPI object handles to object pointers */
    MPID_Comm_get_ptr(comm_old, comm_ptr);
    MPID_Info_get_ptr(info, info_ptr);

    /* Validate parameters and objects (post conversion) */
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            /* Validate comm_ptr */
            MPID_Comm_valid_ptr(comm_ptr, mpi_errno);
            /* If comm_ptr is not valid, it will be reset to null */
            if (comm_ptr) {
                MPIR_ERRTEST_COMM_INTRA(comm_ptr, mpi_errno);
            }

            MPIR_ERRTEST_ARGNEG(indegree, "indegree", mpi_errno);
            MPIR_ERRTEST_ARGNEG(outdegree, "outdegree", mpi_errno);

            if (indegree > 0) {
                MPIR_ERRTEST_ARGNULL(sources, "sources", mpi_errno);
                if (sourceweights == MPI_UNWEIGHTED && destweights != MPI_UNWEIGHTED) {
                    MPIU_ERR_SET(mpi_errno, MPIR_ERR_RECOVERABLE, "**unweightedboth");
                }
                /* TODO check ranges for array elements too (**argarrayneg / **rankarray)*/
            }
            if (outdegree > 0) {
                MPIR_ERRTEST_ARGNULL(destinations, "destinations", mpi_errno);
                if (destweights == MPI_UNWEIGHTED && sourceweights != MPI_UNWEIGHTED) {
                    MPIU_ERR_SET(mpi_errno, MPIR_ERR_RECOVERABLE, "**unweightedboth");
                }
            }
            MPIR_ERRTEST_ARGNULL(comm_dist_graph, "comm_dist_graph", mpi_errno);

            if (mpi_errno) goto fn_fail;
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */

    /* Implementation based on Torsten Hoefler's reference implementation
     * attached to MPI-2.2 ticket #33. */
    *comm_dist_graph = MPI_COMM_NULL;

    /* following the spirit of the old topo interface, attributes do not
     * propagate to the new communicator (see MPI-2.1 pp. 243 line 11) */
    mpi_errno = MPIR_Comm_copy(comm_ptr, comm_ptr->local_size, &comm_dist_graph_ptr);
    if (mpi_errno) MPIU_ERR_POP(mpi_errno);

    /* Create the topology structure */
    MPIU_CHKPMEM_MALLOC(topo_ptr, MPIR_Topology *, sizeof(MPIR_Topology), mpi_errno, "topo_ptr");
    topo_ptr->kind = MPI_DIST_GRAPH;
    dist_graph_ptr = &topo_ptr->topo.dist_graph;
    dist_graph_ptr->indegree = indegree;
    dist_graph_ptr->in = NULL;
    dist_graph_ptr->in_weights = NULL;
    dist_graph_ptr->outdegree = outdegree;
    dist_graph_ptr->out = NULL;
    dist_graph_ptr->out_weights = NULL;

    MPIU_CHKPMEM_MALLOC(dist_graph_ptr->in, int *, indegree*sizeof(int), mpi_errno, "dist_graph_ptr->in");
    MPIU_CHKPMEM_MALLOC(dist_graph_ptr->out, int *, outdegree*sizeof(int), mpi_errno, "dist_graph_ptr->out");
    MPIU_Memcpy(dist_graph_ptr->in, sources, indegree*sizeof(int));
    MPIU_Memcpy(dist_graph_ptr->out, destinations, outdegree*sizeof(int));

    if (sourceweights != MPI_UNWEIGHTED) {
        MPIU_CHKPMEM_MALLOC(dist_graph_ptr->in_weights, int *, indegree*sizeof(int), mpi_errno, "dist_graph_ptr->in_weights");
        MPIU_CHKPMEM_MALLOC(dist_graph_ptr->out_weights, int *, outdegree*sizeof(int), mpi_errno, "dist_graph_ptr->out_weights");
        MPIU_Memcpy(dist_graph_ptr->in_weights, sourceweights, indegree*sizeof(int));
        MPIU_Memcpy(dist_graph_ptr->out_weights, destweights, outdegree*sizeof(int));
    }

    mpi_errno = MPIR_Topology_put(comm_dist_graph_ptr, topo_ptr);
    if (mpi_errno) MPIU_ERR_POP(mpi_errno);

    MPIU_OBJ_PUBLISH_HANDLE(*comm_dist_graph, comm_dist_graph_ptr->handle);
    /* ... end of body of routine ... */
  fn_exit:
    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_DIST_GRAPH_CREATE_ADJACENT);
    MPIU_THREAD_CS_EXIT(ALLFUNC,);
    return mpi_errno;

    /* --BEGIN ERROR HANDLING-- */
  fn_fail:
#ifdef HAVE_ERROR_CHECKING
    mpi_errno = MPIR_Err_create_code(
        mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER,
        "**mpi_dist_graph_create_adjacent",
        "**mpi_dist_graph_create_adjacent %C %d %p %p %d %p %p %I %d %p",
        comm_old, indegree, sources, sourceweights,
        outdegree, destinations, destweights,
        info, reorder, comm_dist_graph);
#endif
    mpi_errno = MPIR_Err_return_comm(comm_ptr, FCNAME, mpi_errno);
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}

