/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2010 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/*
=== BEGIN_MPI_T_CVAR_INFO_BLOCK ===

cvars:
    - name        : MPIR_CVAR_IALLTOALLW_INTRA_ALGORITHM
      category    : COLLECTIVE
      type        : string
      default     : auto
      class       : device
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : |-
        Variable to select ialltoallw algorithm
        auto              - Internal algorithm selection
        blocked           - Force blocked algorithm
        inplace           - Force inplace algorithm
        pairwise_exchange - Force pairwise exchange algorithm

    - name        : MPIR_CVAR_IALLTOALLW_INTER_ALGORITHM
      category    : COLLECTIVE
      type        : string
      default     : auto
      class       : device
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : |-
        Variable to select ialltoallw algorithm
        auto - Internal algorithm selection

    - name        : MPIR_CVAR_IALLTOALLW_DEVICE_COLLECTIVE
      category    : COLLECTIVE
      type        : boolean
      default     : true
      class       : device
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        If set to true, MPI_Ialltoallw will allow the device to override the
        MPIR-level collective algorithms. The device still has the
        option to call the MPIR-level algorithms manually.
        If set to false, the device-level ialltoallw function will not be
        called.

=== END_MPI_T_CVAR_INFO_BLOCK ===
*/

/* -- Begin Profiling Symbol Block for routine MPI_Ialltoallw */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Ialltoallw = PMPI_Ialltoallw
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Ialltoallw  MPI_Ialltoallw
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Ialltoallw as PMPI_Ialltoallw
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPI_Ialltoallw(const void *sendbuf, const int sendcounts[], const int sdispls[],
                   const MPI_Datatype sendtypes[], void *recvbuf, const int recvcounts[],
                   const int rdispls[], const MPI_Datatype recvtypes[], MPI_Comm comm,
                   MPI_Request * request) __attribute__ ((weak, alias("PMPI_Ialltoallw")));
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Ialltoallw
#define MPI_Ialltoallw PMPI_Ialltoallw

#undef FUNCNAME
#define FUNCNAME MPIR_Ialltoallw_sched_intra_auto
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Ialltoallw_sched_intra_auto(const void *sendbuf, const int sendcounts[],
                                     const int sdispls[], const MPI_Datatype sendtypes[],
                                     void *recvbuf, const int recvcounts[], const int rdispls[],
                                     const MPI_Datatype recvtypes[], MPIR_Comm * comm_ptr,
                                     MPIR_Sched_t s)
{
    int mpi_errno = MPI_SUCCESS;

    if (sendbuf == MPI_IN_PLACE) {
        mpi_errno = MPIR_Ialltoallw_sched_intra_inplace(sendbuf, sendcounts, sdispls,
                                                        sendtypes, recvbuf, recvcounts,
                                                        rdispls, recvtypes, comm_ptr, s);
    } else {
        mpi_errno = MPIR_Ialltoallw_sched_intra_blocked(sendbuf, sendcounts, sdispls,
                                                        sendtypes, recvbuf, recvcounts,
                                                        rdispls, recvtypes, comm_ptr, s);
    }

    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIR_Ialltoallw_sched_inter_auto
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Ialltoallw_sched_inter_auto(const void *sendbuf, const int sendcounts[],
                                     const int sdispls[], const MPI_Datatype sendtypes[],
                                     void *recvbuf, const int recvcounts[], const int rdispls[],
                                     const MPI_Datatype recvtypes[], MPIR_Comm * comm_ptr,
                                     MPIR_Sched_t s)
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = MPIR_Ialltoallw_sched_inter_pairwise_exchange(sendbuf, sendcounts, sdispls,
                                                              sendtypes, recvbuf, recvcounts,
                                                              rdispls, recvtypes, comm_ptr, s);

    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIR_Ialltoallw_sched_impl
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Ialltoallw_sched_impl(const void *sendbuf, const int sendcounts[], const int sdispls[],
                               const MPI_Datatype sendtypes[], void *recvbuf,
                               const int recvcounts[], const int rdispls[],
                               const MPI_Datatype recvtypes[], MPIR_Comm * comm_ptr, MPIR_Sched_t s)
{
    int mpi_errno = MPI_SUCCESS;

    if (comm_ptr->comm_kind == MPIR_COMM_KIND__INTRACOMM) {
        /* intracommunicator */
        switch (MPIR_Ialltoallw_intra_algo_choice) {
            case MPIR_IALLTOALLW_INTRA_ALGO_BLOCKED:
                mpi_errno = MPIR_Ialltoallw_sched_intra_blocked(sendbuf, sendcounts, sdispls,
                                                                sendtypes, recvbuf, recvcounts,
                                                                rdispls, recvtypes, comm_ptr, s);
                break;
            case MPIR_IALLTOALLW_INTRA_ALGO_INPLACE:
                mpi_errno = MPIR_Ialltoallw_sched_intra_inplace(sendbuf, sendcounts, sdispls,
                                                                sendtypes, recvbuf, recvcounts,
                                                                rdispls, recvtypes, comm_ptr, s);
                break;
            case MPIR_IALLTOALLW_INTRA_ALGO_AUTO:
                MPL_FALLTHROUGH;
            default:
                mpi_errno = MPIR_Ialltoallw_sched_intra_auto(sendbuf, sendcounts, sdispls,
                                                             sendtypes, recvbuf, recvcounts,
                                                             rdispls, recvtypes, comm_ptr, s);
                break;
        }
    } else {
        /* intercommunicator */
        switch (MPIR_Ialltoallw_inter_algo_choice) {
            case MPIR_IALLTOALLW_INTER_ALGO_PAIRWISE_EXCHANGE:
                mpi_errno =
                    MPIR_Ialltoallw_sched_inter_pairwise_exchange(sendbuf, sendcounts, sdispls,
                                                                  sendtypes, recvbuf, recvcounts,
                                                                  rdispls, recvtypes, comm_ptr, s);
                break;
            case MPIR_IALLTOALLW_INTER_ALGO_AUTO:
                MPL_FALLTHROUGH;
            default:
                mpi_errno = MPIR_Ialltoallw_sched_inter_auto(sendbuf, sendcounts, sdispls,
                                                             sendtypes, recvbuf, recvcounts,
                                                             rdispls, recvtypes, comm_ptr, s);
                break;
        }
    }

    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIR_Ialltoallw_sched
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Ialltoallw_sched(const void *sendbuf, const int sendcounts[], const int sdispls[],
                          const MPI_Datatype sendtypes[], void *recvbuf, const int recvcounts[],
                          const int rdispls[], const MPI_Datatype recvtypes[],
                          MPIR_Comm * comm_ptr, MPIR_Sched_t s)
{
    int mpi_errno = MPI_SUCCESS;

    if (MPIR_CVAR_IALLTOALLW_DEVICE_COLLECTIVE && MPIR_CVAR_DEVICE_COLLECTIVES) {
        mpi_errno = MPID_Ialltoallw_sched(sendbuf, sendcounts, sdispls, sendtypes, recvbuf,
                                          recvcounts, rdispls, recvtypes, comm_ptr, s);
    } else {
        mpi_errno = MPIR_Ialltoallw_sched_impl(sendbuf, sendcounts, sdispls, sendtypes, recvbuf,
                                               recvcounts, rdispls, recvtypes, comm_ptr, s);
    }

    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIR_Ialltoallw_impl
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Ialltoallw_impl(const void *sendbuf, const int sendcounts[], const int sdispls[],
                         const MPI_Datatype sendtypes[], void *recvbuf, const int recvcounts[],
                         const int rdispls[], const MPI_Datatype recvtypes[], MPIR_Comm * comm_ptr,
                         MPIR_Request ** request)
{
    int mpi_errno = MPI_SUCCESS;
    int tag = -1;
    MPIR_Sched_t s = MPIR_SCHED_NULL;

    *request = NULL;

    mpi_errno = MPIR_Sched_next_tag(comm_ptr, &tag);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);
    mpi_errno = MPIR_Sched_create(&s);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

    mpi_errno =
        MPIR_Ialltoallw_sched(sendbuf, sendcounts, sdispls, sendtypes, recvbuf, recvcounts, rdispls,
                              recvtypes, comm_ptr, s);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

    mpi_errno = MPIR_Sched_start(&s, comm_ptr, tag, request);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIR_Ialltoallw
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Ialltoallw(const void *sendbuf, const int sendcounts[], const int sdispls[],
                    const MPI_Datatype sendtypes[], void *recvbuf, const int recvcounts[],
                    const int rdispls[], const MPI_Datatype recvtypes[], MPIR_Comm * comm_ptr,
                    MPIR_Request ** request)
{
    int mpi_errno = MPI_SUCCESS;

    if (MPIR_CVAR_IALLTOALLW_DEVICE_COLLECTIVE && MPIR_CVAR_DEVICE_COLLECTIVES) {
        mpi_errno = MPID_Ialltoallw(sendbuf, sendcounts, sdispls, sendtypes, recvbuf,
                                    recvcounts, rdispls, recvtypes, comm_ptr, request);
    } else {
        mpi_errno = MPIR_Ialltoallw_impl(sendbuf, sendcounts, sdispls, sendtypes, recvbuf,
                                         recvcounts, rdispls, recvtypes, comm_ptr, request);
    }

    return mpi_errno;
}

#endif /* MPICH_MPI_FROM_PMPI */

#undef FUNCNAME
#define FUNCNAME MPI_Ialltoallw
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
/*@
MPI_Ialltoallw - Nonblocking generalized all-to-all communication allowing
   different datatypes, counts, and displacements for each partner

Input Parameters:
+ sendbuf - starting address of the send buffer (choice)
. sendcounts - non-negative integer array (of length group size) specifying the number of elements to send to each processor
. sdispls - integer array (of length group size). Entry j specifies the displacement relative to sendbuf from which to take the outgoing data destined for process j
. sendtypes - array of datatypes (of length group size). Entry j specifies the type of data to send to process j (array of handles)
. recvcounts - non-negative integer array (of length group size) specifying the number of elements that can be received from each processor
. rdispls - integer array (of length group size). Entry i specifies the displacement relative to recvbuf at which to place the incoming data from process i
. recvtypes - array of datatypes (of length group size). Entry i specifies the type of data received from process i (array of handles)
- comm - communicator (handle)

Output Parameters:
+ recvbuf - starting address of the receive buffer (choice)
- request - communication request (handle)

.N ThreadSafe

.N Fortran

.N Errors
@*/
int MPI_Ialltoallw(const void *sendbuf, const int sendcounts[], const int sdispls[],
                   const MPI_Datatype sendtypes[], void *recvbuf, const int recvcounts[],
                   const int rdispls[], const MPI_Datatype recvtypes[], MPI_Comm comm,
                   MPI_Request * request)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Comm *comm_ptr = NULL;
    MPIR_Request *request_ptr = NULL;
    MPIR_FUNC_TERSE_STATE_DECL(MPID_STATE_MPI_IALLTOALLW);

    MPID_THREAD_CS_ENTER(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    MPIR_FUNC_TERSE_ENTER(MPID_STATE_MPI_IALLTOALLW);

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

    /* Validate parameters and objects (post conversion) */
#ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            MPIR_Comm_valid_ptr(comm_ptr, mpi_errno, FALSE);
            if (mpi_errno != MPI_SUCCESS)
                goto fn_fail;

            if (sendbuf != MPI_IN_PLACE) {
                MPIR_ERRTEST_ARGNULL(sendcounts, "sendcounts", mpi_errno);
                MPIR_ERRTEST_ARGNULL(sdispls, "sdispls", mpi_errno);
                MPIR_ERRTEST_ARGNULL(sendtypes, "sendtypes", mpi_errno);

                if (comm_ptr->comm_kind == MPIR_COMM_KIND__INTRACOMM &&
                    sendcounts == recvcounts && sendtypes == recvtypes)
                    MPIR_ERRTEST_ALIAS_COLL(sendbuf, recvbuf, mpi_errno);
            }
            MPIR_ERRTEST_ARGNULL(recvcounts, "recvcounts", mpi_errno);
            MPIR_ERRTEST_ARGNULL(rdispls, "rdispls", mpi_errno);
            MPIR_ERRTEST_ARGNULL(recvtypes, "recvtypes", mpi_errno);
            if (comm_ptr->comm_kind == MPIR_COMM_KIND__INTERCOMM && sendbuf == MPI_IN_PLACE) {
                MPIR_ERR_SETANDJUMP(mpi_errno, MPI_ERR_OTHER, "**sendbuf_inplace");
            }
            MPIR_ERRTEST_ARGNULL(request, "request", mpi_errno);
            /* TODO more checks may be appropriate (counts, in_place, etc) */
        }
        MPID_END_ERROR_CHECKS;
    }
#endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */

    mpi_errno = MPIR_Ialltoallw(sendbuf, sendcounts, sdispls, sendtypes, recvbuf,
                                recvcounts, rdispls, recvtypes, comm_ptr, &request_ptr);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

    /* create a complete request, if needed */
    if (!request_ptr)
        request_ptr = MPIR_Request_create_complete(MPIR_REQUEST_KIND__COLL);
    /* return the handle of the request to the user */
    *request = request_ptr->handle;

    /* ... end of body of routine ... */

  fn_exit:
    MPIR_FUNC_TERSE_EXIT(MPID_STATE_MPI_IALLTOALLW);
    MPID_THREAD_CS_EXIT(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
#ifdef HAVE_ERROR_CHECKING
    {
        mpi_errno =
            MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER,
                                 "**mpi_ialltoallw",
                                 "**mpi_ialltoallw %p %p %p %p %p %p %p %p %C %p", sendbuf,
                                 sendcounts, sdispls, sendtypes, recvbuf, recvcounts, rdispls,
                                 recvtypes, comm, request);
    }
#endif
    mpi_errno = MPIR_Err_return_comm(comm_ptr, FCNAME, mpi_errno);
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}
