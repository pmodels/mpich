/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"

/*
=== BEGIN_MPI_T_CVAR_INFO_BLOCK ===

cvars:
    - name        : MPIR_CVAR_NEIGHBOR_ALLTOALL_INTRA_ALGORITHM
      category    : COLLECTIVE
      type        : enum
      default     : auto
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : |-
        Variable to select neighbor_alltoall algorithm
        auto - Internal algorithm selection (can be overridden with MPIR_CVAR_COLL_SELECTION_TUNING_JSON_FILE)
        nb   - Force nb algorithm

    - name        : MPIR_CVAR_NEIGHBOR_ALLTOALL_INTER_ALGORITHM
      category    : COLLECTIVE
      type        : enum
      default     : auto
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : |-
        Variable to select neighbor_alltoall algorithm
        auto - Internal algorithm selection (can be overridden with MPIR_CVAR_COLL_SELECTION_TUNING_JSON_FILE)
        nb   - Force nb algorithm

    - name        : MPIR_CVAR_NEIGHBOR_ALLTOALL_DEVICE_COLLECTIVE
      category    : COLLECTIVE
      type        : boolean
      default     : true
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        This CVAR is only used when MPIR_CVAR_DEVICE_COLLECTIVES
        is set to "percoll".  If set to true, MPI_Neighbor_alltoall will
        allow the device to override the MPIR-level collective
        algorithms.  The device might still call the MPIR-level
        algorithms manually.  If set to false, the device-override
        will be disabled.

=== END_MPI_T_CVAR_INFO_BLOCK ===
*/

/* -- Begin Profiling Symbol Block for routine MPI_Neighbor_alltoall */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Neighbor_alltoall = PMPI_Neighbor_alltoall
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Neighbor_alltoall  MPI_Neighbor_alltoall
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Neighbor_alltoall as PMPI_Neighbor_alltoall
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPI_Neighbor_alltoall(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                          void *recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm)
    __attribute__ ((weak, alias("PMPI_Neighbor_alltoall")));
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Neighbor_alltoall
#define MPI_Neighbor_alltoall PMPI_Neighbor_alltoall

/* any non-MPI functions go here, especially non-static ones */


int MPIR_Neighbor_alltoall_allcomm_auto(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                                        void *recvbuf, int recvcount, MPI_Datatype recvtype,
                                        MPIR_Comm * comm_ptr)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_Csel_coll_sig_s coll_sig = {
        .coll_type = MPIR_CSEL_COLL_TYPE__NEIGHBOR_ALLTOALL,
        .comm_ptr = comm_ptr,

        .u.neighbor_alltoall.sendbuf = sendbuf,
        .u.neighbor_alltoall.sendcount = sendcount,
        .u.neighbor_alltoall.sendtype = sendtype,
        .u.neighbor_alltoall.recvcount = recvcount,
        .u.neighbor_alltoall.recvbuf = recvbuf,
        .u.neighbor_alltoall.recvtype = recvtype,
    };

    MPII_Csel_container_s *cnt = MPIR_Csel_search(comm_ptr->csel_comm, coll_sig);
    MPIR_Assert(cnt);

    switch (cnt->id) {
        case MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Neighbor_alltoall_allcomm_nb:
            mpi_errno =
                MPIR_Neighbor_alltoall_allcomm_nb(sendbuf, sendcount, sendtype, recvbuf, recvcount,
                                                  recvtype, comm_ptr);
            break;

        default:
            MPIR_Assert(0);
    }

    return mpi_errno;
}

int MPIR_Neighbor_alltoall_impl(const void *sendbuf, int sendcount,
                                MPI_Datatype sendtype, void *recvbuf,
                                int recvcount, MPI_Datatype recvtype, MPIR_Comm * comm_ptr)
{
    int mpi_errno = MPI_SUCCESS;

    if (comm_ptr->comm_kind == MPIR_COMM_KIND__INTRACOMM) {
        switch (MPIR_CVAR_NEIGHBOR_ALLTOALL_INTRA_ALGORITHM) {
            case MPIR_CVAR_NEIGHBOR_ALLTOALL_INTRA_ALGORITHM_nb:
                mpi_errno = MPIR_Neighbor_alltoall_allcomm_nb(sendbuf, sendcount, sendtype,
                                                              recvbuf, recvcount, recvtype,
                                                              comm_ptr);
                break;
            case MPIR_CVAR_NEIGHBOR_ALLTOALL_INTRA_ALGORITHM_auto:
                mpi_errno = MPIR_Neighbor_alltoall_allcomm_auto(sendbuf, sendcount, sendtype,
                                                                recvbuf, recvcount, recvtype,
                                                                comm_ptr);
                break;
            default:
                MPIR_Assert(0);
        }
    } else {
        switch (MPIR_CVAR_NEIGHBOR_ALLTOALL_INTER_ALGORITHM) {
            case MPIR_CVAR_NEIGHBOR_ALLTOALL_INTER_ALGORITHM_nb:
                mpi_errno = MPIR_Neighbor_alltoall_allcomm_nb(sendbuf, sendcount, sendtype,
                                                              recvbuf, recvcount, recvtype,
                                                              comm_ptr);
                break;
            case MPIR_CVAR_NEIGHBOR_ALLTOALL_INTER_ALGORITHM_auto:
                mpi_errno = MPIR_Neighbor_alltoall_allcomm_auto(sendbuf, sendcount, sendtype,
                                                                recvbuf, recvcount, recvtype,
                                                                comm_ptr);
                break;
            default:
                MPIR_Assert(0);
        }
    }
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIR_Neighbor_alltoall(const void *sendbuf, int sendcount,
                           MPI_Datatype sendtype, void *recvbuf, int recvcount,
                           MPI_Datatype recvtype, MPIR_Comm * comm_ptr)
{
    int mpi_errno = MPI_SUCCESS;

    if ((MPIR_CVAR_DEVICE_COLLECTIVES == MPIR_CVAR_DEVICE_COLLECTIVES_all) ||
        ((MPIR_CVAR_DEVICE_COLLECTIVES == MPIR_CVAR_DEVICE_COLLECTIVES_percoll) &&
         MPIR_CVAR_BARRIER_DEVICE_COLLECTIVE)) {
        mpi_errno =
            MPID_Neighbor_alltoall(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype,
                                   comm_ptr);
    } else {
        mpi_errno = MPIR_Neighbor_alltoall_impl(sendbuf, sendcount, sendtype,
                                                recvbuf, recvcount, recvtype, comm_ptr);
    }

    return mpi_errno;
}

#endif /* MPICH_MPI_FROM_PMPI */

/*@
MPI_Neighbor_alltoall - In this function, each process i receives data items
from each process j if an edge (j,i) exists in the topology graph or Cartesian
topology.  Similarly, each process i sends data items to all processes j where an
edge (i,j) exists. This call is more general than MPI_NEIGHBOR_ALLGATHER in that
different data items can be sent to each neighbor. The k-th block in send buffer
is sent to the k-th neighboring process and the l-th block in the receive buffer
is received from the l-th neighbor.

Input Parameters:
+ sendbuf - starting address of the send buffer (choice)
. sendcount - number of elements sent to each neighbor (non-negative integer)
. sendtype - data type of send buffer elements (handle)
. recvcount - number of elements received from each neighbor (non-negative integer)
. recvtype - data type of receive buffer elements (handle)
- comm - communicator (handle)

Output Parameters:
. recvbuf - starting address of the receive buffer (choice)

.N ThreadSafe

.N Fortran

.N Errors
@*/
int MPI_Neighbor_alltoall(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf,
                          int recvcount, MPI_Datatype recvtype, MPI_Comm comm)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Comm *comm_ptr = NULL;
    MPIR_FUNC_TERSE_STATE_DECL(MPID_STATE_MPI_NEIGHBOR_ALLTOALL);

    MPID_THREAD_CS_ENTER(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    MPIR_FUNC_TERSE_ENTER(MPID_STATE_MPI_NEIGHBOR_ALLTOALL);

    /* Validate parameters, especially handles needing to be converted */
#ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            MPIR_ERRTEST_DATATYPE(sendtype, "sendtype", mpi_errno);
            MPIR_ERRTEST_DATATYPE(recvtype, "recvtype", mpi_errno);
            MPIR_ERRTEST_COMM(comm, mpi_errno);

            /* TODO more checks may be appropriate */
        }
        MPID_END_ERROR_CHECKS;
    }
#endif /* HAVE_ERROR_CHECKING */

    /* Convert MPI object handles to object pointers */
    MPIR_Comm_get_ptr(comm, comm_ptr);

    /* Validate parameters and objects (post conversion) */
#ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            if (!HANDLE_IS_BUILTIN(sendtype)) {
                MPIR_Datatype *sendtype_ptr = NULL;
                MPIR_Datatype_get_ptr(sendtype, sendtype_ptr);
                MPIR_Datatype_valid_ptr(sendtype_ptr, mpi_errno);
                MPIR_Datatype_committed_ptr(sendtype_ptr, mpi_errno);
            }

            if (!HANDLE_IS_BUILTIN(recvtype)) {
                MPIR_Datatype *recvtype_ptr = NULL;
                MPIR_Datatype_get_ptr(recvtype, recvtype_ptr);
                MPIR_Datatype_valid_ptr(recvtype_ptr, mpi_errno);
                MPIR_Datatype_committed_ptr(recvtype_ptr, mpi_errno);
            }

            MPIR_Comm_valid_ptr(comm_ptr, mpi_errno, FALSE);
            /* TODO more checks may be appropriate (counts, in_place, buffer aliasing, etc) */
            if (mpi_errno != MPI_SUCCESS)
                goto fn_fail;
        }
        MPID_END_ERROR_CHECKS;
    }
#endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */

    mpi_errno = MPIR_Neighbor_alltoall(sendbuf, sendcount, sendtype, recvbuf,
                                       recvcount, recvtype, comm_ptr);
    MPIR_ERR_CHECK(mpi_errno);

    /* ... end of body of routine ... */

  fn_exit:
    MPIR_FUNC_TERSE_EXIT(MPID_STATE_MPI_NEIGHBOR_ALLTOALL);
    MPID_THREAD_CS_EXIT(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
#ifdef HAVE_ERROR_CHECKING
    {
        mpi_errno =
            MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, __func__, __LINE__, MPI_ERR_OTHER,
                                 "**mpi_neighbor_alltoall",
                                 "**mpi_neighbor_alltoall %p %d %D %p %d %D %C", sendbuf, sendcount,
                                 sendtype, recvbuf, recvcount, recvtype, comm);
    }
#endif
    mpi_errno = MPIR_Err_return_comm(NULL, __func__, mpi_errno);
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}
