/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Graphdims_get */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Graphdims_get = PMPI_Graphdims_get
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Graphdims_get  MPI_Graphdims_get
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Graphdims_get as PMPI_Graphdims_get
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPI_Graphdims_get(MPI_Comm comm, int *nnodes, int *nedges)
    __attribute__ ((weak, alias("PMPI_Graphdims_get")));
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Graphdims_get
#define MPI_Graphdims_get PMPI_Graphdims_get

#endif

#undef FUNCNAME
#define FUNCNAME MPI_Graphdims_get
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
/*@

MPI_Graphdims_get - Retrieves graph topology information associated with a
                    communicator

Input Parameters:
. comm - communicator for group with graph structure (handle)

Output Parameters:
+ nnodes - number of nodes in graph (integer)
- nedges - number of edges in graph (integer)

.N SignalSafe

.N Fortran

.N Errors
.N MPI_SUCCESS
.N MPI_ERR_TOPOLOGY
.N MPI_ERR_COMM
.N MPI_ERR_ARG
@*/
int MPI_Graphdims_get(MPI_Comm comm, int *nnodes, int *nedges)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Comm *comm_ptr = NULL;
    MPIR_Topology *topo_ptr;
    MPIR_FUNC_TERSE_STATE_DECL(MPID_STATE_MPI_GRAPHDIMS_GET);

    MPIR_ERRTEST_INITIALIZED_ORDIE();

    MPIR_FUNC_TERSE_ENTER(MPID_STATE_MPI_GRAPHDIMS_GET);

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
            /* Validate comm_ptr */
            MPIR_Comm_valid_ptr(comm_ptr, mpi_errno, TRUE);
            if (mpi_errno)
                goto fn_fail;
            /* If comm_ptr is not valid, it will be reset to null */
            MPIR_ERRTEST_ARGNULL(nnodes, "nnodes", mpi_errno);
            MPIR_ERRTEST_ARGNULL(nedges, "nedges", mpi_errno);
        }
        MPID_END_ERROR_CHECKS;
    }
#endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */

    topo_ptr = MPIR_Topology_get(comm_ptr);

    MPIR_ERR_CHKANDJUMP((!topo_ptr ||
                         topo_ptr->kind != MPI_GRAPH), mpi_errno, MPI_ERR_TOPOLOGY,
                        "**notgraphtopo");

    /* Set nnodes */
    *nnodes = topo_ptr->topo.graph.nnodes;

    /* Set nedges */
    *nedges = topo_ptr->topo.graph.nedges;

    /* ... end of body of routine ... */

  fn_exit:
    MPIR_FUNC_TERSE_EXIT(MPID_STATE_MPI_GRAPHDIMS_GET);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
#ifdef HAVE_ERROR_CHECKING
    {
        mpi_errno =
            MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER,
                                 "**mpi_graphdims_get", "**mpi_graphdims_get %C %p %p", comm,
                                 nnodes, nedges);
    }
#endif
    mpi_errno = MPIR_Err_return_comm(comm_ptr, FCNAME, mpi_errno);
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}
