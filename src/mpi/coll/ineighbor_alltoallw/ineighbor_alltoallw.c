/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"

/*
=== BEGIN_MPI_T_CVAR_INFO_BLOCK ===

cvars:
    - name        : MPIR_CVAR_INEIGHBOR_ALLTOALLW_INTRA_ALGORITHM
      category    : COLLECTIVE
      type        : enum
      default     : auto
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : |-
        Variable to select ineighbor_alltoallw algorithm
        auto - Internal algorithm selection (can be overridden with MPIR_CVAR_COLL_SELECTION_TUNING_JSON_FILE)
        sched_auto - Internal algorithm selection for sched-based algorithms
        sched_linear          - Force linear algorithm
        gentran_linear        - Force generic transport based linear algorithm

    - name        : MPIR_CVAR_INEIGHBOR_ALLTOALLW_INTER_ALGORITHM
      category    : COLLECTIVE
      type        : enum
      default     : auto
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : |-
        Variable to select ineighbor_alltoallw algorithm
        auto - Internal algorithm selection (can be overridden with MPIR_CVAR_COLL_SELECTION_TUNING_JSON_FILE)
        sched_auto - Internal algorithm selection for sched-based algorithms
        sched_linear          - Force linear algorithm
        gentran_linear        - Force generic transport based linear algorithm

    - name        : MPIR_CVAR_INEIGHBOR_ALLTOALLW_DEVICE_COLLECTIVE
      category    : COLLECTIVE
      type        : boolean
      default     : true
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        This CVAR is only used when MPIR_CVAR_DEVICE_COLLECTIVES
        is set to "percoll".  If set to true, MPI_Ineighbor_alltoallw will
        allow the device to override the MPIR-level collective
        algorithms.  The device might still call the MPIR-level
        algorithms manually.  If set to false, the device-override
        will be disabled.

=== END_MPI_T_CVAR_INFO_BLOCK ===
*/

/* -- Begin Profiling Symbol Block for routine MPI_Ineighbor_alltoallw */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Ineighbor_alltoallw = PMPI_Ineighbor_alltoallw
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Ineighbor_alltoallw  MPI_Ineighbor_alltoallw
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Ineighbor_alltoallw as PMPI_Ineighbor_alltoallw
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPI_Ineighbor_alltoallw(const void *sendbuf, const int sendcounts[],
                            const MPI_Aint sdispls[], const MPI_Datatype sendtypes[],
                            void *recvbuf, const int recvcounts[], const MPI_Aint rdispls[],
                            const MPI_Datatype recvtypes[], MPI_Comm comm, MPI_Request * request)
    __attribute__ ((weak, alias("PMPI_Ineighbor_alltoallw")));
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Ineighbor_alltoallw
#define MPI_Ineighbor_alltoallw PMPI_Ineighbor_alltoallw


int MPIR_Ineighbor_alltoallw_allcomm_auto(const void *sendbuf, const int sendcounts[],
                                          const MPI_Aint sdispls[], const MPI_Datatype sendtypes[],
                                          void *recvbuf, const int recvcounts[],
                                          const MPI_Aint rdispls[], const MPI_Datatype recvtypes[],
                                          MPIR_Comm * comm_ptr, MPIR_Request ** request)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_Csel_coll_sig_s coll_sig = {
        .coll_type = MPIR_CSEL_COLL_TYPE__INEIGHBOR_ALLTOALLW,
        .comm_ptr = comm_ptr,

        .u.ineighbor_alltoallw.sendbuf = sendbuf,
        .u.ineighbor_alltoallw.sendcounts = sendcounts,
        .u.ineighbor_alltoallw.sdispls = sdispls,
        .u.ineighbor_alltoallw.sendtypes = sendtypes,
        .u.ineighbor_alltoallw.recvbuf = recvbuf,
        .u.ineighbor_alltoallw.recvcounts = recvcounts,
        .u.ineighbor_alltoallw.rdispls = rdispls,
        .u.ineighbor_alltoallw.recvtypes = recvtypes,
    };

    MPII_Csel_container_s *cnt = MPIR_Csel_search(comm_ptr->csel_comm, coll_sig);
    MPIR_Assert(cnt);

    switch (cnt->id) {
        case MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Ineighbor_alltoallw_allcomm_gentran_linear:
            mpi_errno =
                MPIR_Ineighbor_alltoallw_allcomm_gentran_linear(sendbuf, sendcounts, sdispls,
                                                                sendtypes, recvbuf, recvcounts,
                                                                rdispls, recvtypes, comm_ptr,
                                                                request);
            break;

        case MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Ineighbor_alltoallw_intra_sched_auto:
            MPII_SCHED_WRAPPER(MPIR_Ineighbor_alltoallw_intra_sched_auto, comm_ptr, request,
                               sendbuf, sendcounts, sdispls, sendtypes, recvbuf, recvcounts,
                               rdispls, recvtypes);
            break;

        case MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Ineighbor_alltoallw_inter_sched_auto:
            MPII_SCHED_WRAPPER(MPIR_Ineighbor_alltoallw_inter_sched_auto, comm_ptr, request,
                               sendbuf, sendcounts, sdispls, sendtypes, recvbuf, recvcounts,
                               rdispls, recvtypes);
            break;

        case MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Ineighbor_alltoallw_allcomm_sched_linear:
            MPII_SCHED_WRAPPER(MPIR_Ineighbor_alltoallw_allcomm_sched_linear, comm_ptr, request,
                               sendbuf, sendcounts, sdispls, sendtypes, recvbuf, recvcounts,
                               rdispls, recvtypes);
            break;

        default:
            MPIR_Assert(0);
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIR_Ineighbor_alltoallw_intra_sched_auto(const void *sendbuf, const int sendcounts[],
                                              const MPI_Aint sdispls[],
                                              const MPI_Datatype sendtypes[], void *recvbuf,
                                              const int recvcounts[], const MPI_Aint rdispls[],
                                              const MPI_Datatype recvtypes[], MPIR_Comm * comm_ptr,
                                              MPIR_Sched_t s)
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno =
        MPIR_Ineighbor_alltoallw_allcomm_sched_linear(sendbuf, sendcounts, sdispls, sendtypes,
                                                      recvbuf, recvcounts, rdispls, recvtypes,
                                                      comm_ptr, s);
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    return mpi_errno;

  fn_fail:
    goto fn_exit;
}

int MPIR_Ineighbor_alltoallw_inter_sched_auto(const void *sendbuf, const int sendcounts[],
                                              const MPI_Aint sdispls[],
                                              const MPI_Datatype sendtypes[], void *recvbuf,
                                              const int recvcounts[], const MPI_Aint rdispls[],
                                              const MPI_Datatype recvtypes[], MPIR_Comm * comm_ptr,
                                              MPIR_Sched_t s)
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno =
        MPIR_Ineighbor_alltoallw_allcomm_sched_linear(sendbuf, sendcounts, sdispls, sendtypes,
                                                      recvbuf, recvcounts, rdispls, recvtypes,
                                                      comm_ptr, s);
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    return mpi_errno;

  fn_fail:
    goto fn_exit;
}

int MPIR_Ineighbor_alltoallw_sched_auto(const void *sendbuf, const int sendcounts[],
                                        const MPI_Aint sdispls[], const MPI_Datatype sendtypes[],
                                        void *recvbuf, const int recvcounts[],
                                        const MPI_Aint rdispls[], const MPI_Datatype recvtypes[],
                                        MPIR_Comm * comm_ptr, MPIR_Sched_t s)
{
    int mpi_errno = MPI_SUCCESS;

    if (comm_ptr->comm_kind == MPIR_COMM_KIND__INTRACOMM) {
        mpi_errno =
            MPIR_Ineighbor_alltoallw_intra_sched_auto(sendbuf, sendcounts, sdispls,
                                                      sendtypes, recvbuf, recvcounts,
                                                      rdispls, recvtypes, comm_ptr, s);
    } else {
        mpi_errno =
            MPIR_Ineighbor_alltoallw_inter_sched_auto(sendbuf, sendcounts, sdispls,
                                                      sendtypes, recvbuf, recvcounts,
                                                      rdispls, recvtypes, comm_ptr, s);
    }

    return mpi_errno;
}

int MPIR_Ineighbor_alltoallw_impl(const void *sendbuf, const int sendcounts[],
                                  const MPI_Aint sdispls[],
                                  const MPI_Datatype sendtypes[],
                                  void *recvbuf, const int recvcounts[],
                                  const MPI_Aint rdispls[],
                                  const MPI_Datatype recvtypes[],
                                  MPIR_Comm * comm_ptr, MPIR_Request ** request)
{
    int mpi_errno = MPI_SUCCESS;

    *request = NULL;
    /* If the user picks one of the transport-enabled algorithms, branch there
     * before going down to the MPIR_Sched-based algorithms. */
    /* TODO - Eventually the intention is to replace all of the
     * MPIR_Sched-based algorithms with transport-enabled algorithms, but that
     * will require sufficient performance testing and replacement algorithms. */
    if (comm_ptr->comm_kind == MPIR_COMM_KIND__INTRACOMM) {
        switch (MPIR_CVAR_INEIGHBOR_ALLTOALLW_INTRA_ALGORITHM) {
            case MPIR_CVAR_INEIGHBOR_ALLTOALLW_INTRA_ALGORITHM_gentran_linear:
                mpi_errno =
                    MPIR_Ineighbor_alltoallw_allcomm_gentran_linear(sendbuf, sendcounts, sdispls,
                                                                    sendtypes, recvbuf, recvcounts,
                                                                    rdispls, recvtypes, comm_ptr,
                                                                    request);
                break;

            case MPIR_CVAR_INEIGHBOR_ALLTOALLW_INTRA_ALGORITHM_sched_linear:
                MPII_SCHED_WRAPPER(MPIR_Ineighbor_alltoallw_allcomm_sched_linear, comm_ptr, request,
                                   sendbuf, sendcounts, sdispls, sendtypes, recvbuf, recvcounts,
                                   rdispls, recvtypes);
                break;

            case MPIR_CVAR_INEIGHBOR_ALLTOALLW_INTRA_ALGORITHM_sched_auto:
                MPII_SCHED_WRAPPER(MPIR_Ineighbor_alltoallw_intra_sched_auto, comm_ptr, request,
                                   sendbuf, sendcounts, sdispls, sendtypes, recvbuf, recvcounts,
                                   rdispls, recvtypes);
                break;

            case MPIR_CVAR_INEIGHBOR_ALLTOALLW_INTRA_ALGORITHM_auto:
                mpi_errno =
                    MPIR_Ineighbor_alltoallw_allcomm_auto(sendbuf, sendcounts, sdispls, sendtypes,
                                                          recvbuf, recvcounts, rdispls, recvtypes,
                                                          comm_ptr, request);
                break;

            default:
                MPIR_Assert(0);
        }
    } else {
        switch (MPIR_CVAR_INEIGHBOR_ALLTOALLW_INTER_ALGORITHM) {
            case MPIR_CVAR_INEIGHBOR_ALLTOALLW_INTER_ALGORITHM_gentran_linear:
                mpi_errno =
                    MPIR_Ineighbor_alltoallw_allcomm_gentran_linear(sendbuf, sendcounts, sdispls,
                                                                    sendtypes, recvbuf, recvcounts,
                                                                    rdispls, recvtypes, comm_ptr,
                                                                    request);
                break;

            case MPIR_CVAR_INEIGHBOR_ALLTOALLW_INTER_ALGORITHM_sched_linear:
                MPII_SCHED_WRAPPER(MPIR_Ineighbor_alltoallw_allcomm_sched_linear, comm_ptr, request,
                                   sendbuf, sendcounts, sdispls, sendtypes, recvbuf, recvcounts,
                                   rdispls, recvtypes);
                break;

            case MPIR_CVAR_INEIGHBOR_ALLTOALLW_INTER_ALGORITHM_sched_auto:
                MPII_SCHED_WRAPPER(MPIR_Ineighbor_alltoallw_inter_sched_auto, comm_ptr, request,
                                   sendbuf, sendcounts, sdispls, sendtypes, recvbuf, recvcounts,
                                   rdispls, recvtypes);
                break;

            case MPIR_CVAR_INEIGHBOR_ALLTOALLW_INTER_ALGORITHM_auto:
                mpi_errno =
                    MPIR_Ineighbor_alltoallw_allcomm_auto(sendbuf, sendcounts, sdispls, sendtypes,
                                                          recvbuf, recvcounts, rdispls, recvtypes,
                                                          comm_ptr, request);
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

int MPIR_Ineighbor_alltoallw(const void *sendbuf, const int sendcounts[],
                             const MPI_Aint sdispls[],
                             const MPI_Datatype sendtypes[], void *recvbuf,
                             const int recvcounts[], const MPI_Aint rdispls[],
                             const MPI_Datatype recvtypes[],
                             MPIR_Comm * comm_ptr, MPIR_Request ** request)
{
    int mpi_errno = MPI_SUCCESS;

    if ((MPIR_CVAR_DEVICE_COLLECTIVES == MPIR_CVAR_DEVICE_COLLECTIVES_all) ||
        ((MPIR_CVAR_DEVICE_COLLECTIVES == MPIR_CVAR_DEVICE_COLLECTIVES_percoll) &&
         MPIR_CVAR_BARRIER_DEVICE_COLLECTIVE)) {
        mpi_errno =
            MPID_Ineighbor_alltoallw(sendbuf, sendcounts, sdispls, sendtypes, recvbuf, recvcounts,
                                     rdispls, recvtypes, comm_ptr, request);
    } else {
        mpi_errno = MPIR_Ineighbor_alltoallw_impl(sendbuf, sendcounts, sdispls, sendtypes, recvbuf,
                                                  recvcounts, rdispls, recvtypes, comm_ptr,
                                                  request);
    }

    return mpi_errno;
}

#endif /* MPICH_MPI_FROM_PMPI */

/*@
MPI_Ineighbor_alltoallw - Nonblocking version of MPI_Neighbor_alltoallw.

Input Parameters:
+ sendbuf - starting address of the send buffer (choice)
. sendcounts - non-negative integer array (of length outdegree) specifying the number of elements to send to each neighbor
. sdispls - integer array (of length outdegree).  Entry j specifies the displacement in bytes (relative to sendbuf) from which to take the outgoing data destined for neighbor j (array of integers)
. sendtypes - array of datatypes (of length outdegree).  Entry j specifies the type of data to send to neighbor j (array of handles)
. recvcounts - non-negative integer array (of length indegree) specifying the number of elements that are received from each neighbor
. rdispls - integer array (of length indegree).  Entry i specifies the displacement in bytes (relative to recvbuf) at which to place the incoming data from neighbor i (array of integers).
. recvtypes - array of datatypes (of length indegree).  Entry i specifies the type of data received from neighbor i (array of handles).
- comm - communicator with topology structure (handle)

Output Parameters:
+ recvbuf - starting address of the receive buffer (choice)
- request - communication request (handle)

.N ThreadSafe

.N Fortran

.N Errors
@*/
int MPI_Ineighbor_alltoallw(const void *sendbuf, const int sendcounts[], const MPI_Aint sdispls[],
                            const MPI_Datatype sendtypes[], void *recvbuf, const int recvcounts[],
                            const MPI_Aint rdispls[], const MPI_Datatype recvtypes[], MPI_Comm comm,
                            MPI_Request * request)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Comm *comm_ptr = NULL;
    MPIR_Request *request_ptr = NULL;
    MPIR_FUNC_TERSE_STATE_DECL(MPID_STATE_MPI_INEIGHBOR_ALLTOALLW);

    MPID_THREAD_CS_ENTER(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    MPIR_FUNC_TERSE_ENTER(MPID_STATE_MPI_INEIGHBOR_ALLTOALLW);

    /* Validate parameters, especially handles needing to be converted */
#ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            MPIR_ERRTEST_COMM(comm, mpi_errno);

            /* TODO more checks may be appropriate */
        }
        MPID_END_ERROR_CHECKS;
    }
#endif /* HAVE_ERROR_CHECKING */

    /* Convert MPI object handles to object pointers */
    MPIR_Comm_get_ptr(comm, comm_ptr);
    MPIR_Assert(comm_ptr != NULL);

    /* Validate parameters and objects (post conversion) */
#ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            MPIR_Comm_valid_ptr(comm_ptr, mpi_errno, FALSE);
            MPIR_ERRTEST_ARGNULL(request, "request", mpi_errno);
            /* TODO more checks may be appropriate (counts, in_place, buffer aliasing, etc) */
            if (mpi_errno != MPI_SUCCESS)
                goto fn_fail;
        }
        MPID_END_ERROR_CHECKS;
    }
#endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */

    mpi_errno =
        MPIR_Ineighbor_alltoallw(sendbuf, sendcounts, sdispls, sendtypes, recvbuf, recvcounts,
                                 rdispls, recvtypes, comm_ptr, &request_ptr);
    MPIR_ERR_CHECK(mpi_errno);

    /* create a complete request, if needed */
    if (!request_ptr)
        request_ptr = MPIR_Request_create_complete(MPIR_REQUEST_KIND__COLL);
    /* return the handle of the request to the user */
    *request = request_ptr->handle;

    /* ... end of body of routine ... */

  fn_exit:
    MPIR_FUNC_TERSE_EXIT(MPID_STATE_MPI_INEIGHBOR_ALLTOALLW);
    MPID_THREAD_CS_EXIT(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
#ifdef HAVE_ERROR_CHECKING
    {
        mpi_errno =
            MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, __func__, __LINE__, MPI_ERR_OTHER,
                                 "**mpi_ineighbor_alltoallw",
                                 "**mpi_ineighbor_alltoallw %p %p %p %p %p %p %p %p %C %p", sendbuf,
                                 sendcounts, sdispls, sendtypes, recvbuf, recvcounts, rdispls,
                                 recvtypes, comm, request);
    }
#endif
    mpi_errno = MPIR_Err_return_comm(NULL, __func__, mpi_errno);
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}
