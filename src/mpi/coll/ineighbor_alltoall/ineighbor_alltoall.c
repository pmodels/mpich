/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2012 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/*
=== BEGIN_MPI_T_CVAR_INFO_BLOCK ===

cvars:
    - name        : MPIR_CVAR_INEIGHBOR_ALLTOALL_INTRA_ALGORITHM
      category    : COLLECTIVE
      type        : enum
      default     : auto
      class       : device
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : |-
        Variable to select ineighbor_alltoall algorithm
        auto            - Internal algorithm selection
        linear          - Force linear algorithm
        gentran_linear  - Force generic transport based linear algorithm


    - name        : MPIR_CVAR_INEIGHBOR_ALLTOALL_INTER_ALGORITHM
      category    : COLLECTIVE
      type        : enum
      default     : auto
      class       : device
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : |-
        Variable to select ineighbor_alltoall algorithm
        auto            - Internal algorithm selection
        linear          - Force linear algorithm
        gentran_linear  - Force generic transport based linear algorithm

    - name        : MPIR_CVAR_INEIGHBOR_ALLTOALL_DEVICE_COLLECTIVE
      category    : COLLECTIVE
      type        : boolean
      default     : true
      class       : device
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        If set to true, MPI_ineighbor_alltoall will allow the device to override the
        MPIR-level collective algorithms. The device still has the
        option to call the MPIR-level algorithms manually.
        If set to false, the device-level ineighbor_alltoall function will not be
        called.

=== END_MPI_T_CVAR_INFO_BLOCK ===
*/

/* -- Begin Profiling Symbol Block for routine MPI_Ineighbor_alltoall */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Ineighbor_alltoall = PMPI_Ineighbor_alltoall
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Ineighbor_alltoall  MPI_Ineighbor_alltoall
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Ineighbor_alltoall as PMPI_Ineighbor_alltoall
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPI_Ineighbor_alltoall(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                           void *recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm,
                           MPI_Request * request)
    __attribute__ ((weak, alias("PMPI_Ineighbor_alltoall")));
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Ineighbor_alltoall
#define MPI_Ineighbor_alltoall PMPI_Ineighbor_alltoall

int MPIR_Ineighbor_alltoall_sched_intra_auto(const void *sendbuf, int sendcount,
                                             MPI_Datatype sendtype, void *recvbuf,
                                             int recvcount, MPI_Datatype recvtype,
                                             MPIR_Comm * comm_ptr, MPIR_Sched_t s)
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = MPIR_Ineighbor_alltoall_sched_allcomm_linear(sendbuf, sendcount, sendtype,
                                                             recvbuf, recvcount, recvtype, comm_ptr,
                                                             s);
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    return mpi_errno;

  fn_fail:
    goto fn_exit;
}

int MPIR_Ineighbor_alltoall_sched_inter_auto(const void *sendbuf, int sendcount,
                                             MPI_Datatype sendtype, void *recvbuf,
                                             int recvcount, MPI_Datatype recvtype,
                                             MPIR_Comm * comm_ptr, MPIR_Sched_t s)
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = MPIR_Ineighbor_alltoall_sched_allcomm_linear(sendbuf, sendcount, sendtype,
                                                             recvbuf, recvcount, recvtype, comm_ptr,
                                                             s);
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    return mpi_errno;

  fn_fail:
    goto fn_exit;
}

int MPIR_Ineighbor_alltoall_sched_impl(const void *sendbuf, int sendcount,
                                       MPI_Datatype sendtype, void *recvbuf,
                                       int recvcount, MPI_Datatype recvtype,
                                       MPIR_Comm * comm_ptr, MPIR_Sched_t s)
{
    int mpi_errno = MPI_SUCCESS;

    if (comm_ptr->comm_kind == MPIR_COMM_KIND__INTRACOMM) {
        switch (MPIR_CVAR_INEIGHBOR_ALLTOALL_INTRA_ALGORITHM) {
            case MPIR_CVAR_INEIGHBOR_ALLTOALL_INTRA_ALGORITHM_linear:
                mpi_errno =
                    MPIR_Ineighbor_alltoall_sched_allcomm_linear(sendbuf, sendcount, sendtype,
                                                                 recvbuf, recvcount, recvtype,
                                                                 comm_ptr, s);
                break;
            case MPIR_CVAR_INEIGHBOR_ALLTOALL_INTRA_ALGORITHM_auto:
                MPL_FALLTHROUGH;
            default:
                mpi_errno = MPIR_Ineighbor_alltoall_sched_intra_auto(sendbuf, sendcount, sendtype,
                                                                     recvbuf, recvcount, recvtype,
                                                                     comm_ptr, s);
                break;
        }
    } else {
        switch (MPIR_CVAR_INEIGHBOR_ALLTOALL_INTER_ALGORITHM) {
            case MPIR_CVAR_INEIGHBOR_ALLTOALL_INTER_ALGORITHM_linear:
                mpi_errno =
                    MPIR_Ineighbor_alltoall_sched_allcomm_linear(sendbuf, sendcount, sendtype,
                                                                 recvbuf, recvcount, recvtype,
                                                                 comm_ptr, s);
                break;
            case MPIR_CVAR_INEIGHBOR_ALLTOALL_INTER_ALGORITHM_auto:
                MPL_FALLTHROUGH;
            default:
                mpi_errno = MPIR_Ineighbor_alltoall_sched_inter_auto(sendbuf, sendcount, sendtype,
                                                                     recvbuf, recvcount, recvtype,
                                                                     comm_ptr, s);
                break;
        }
    }

    return mpi_errno;
}

int MPIR_Ineighbor_alltoall_sched(const void *sendbuf, int sendcount,
                                  MPI_Datatype sendtype, void *recvbuf,
                                  int recvcount, MPI_Datatype recvtype,
                                  MPIR_Comm * comm_ptr, MPIR_Sched_t s)
{
    int mpi_errno = MPI_SUCCESS;

    if (MPIR_CVAR_BARRIER_DEVICE_COLLECTIVE && MPIR_CVAR_DEVICE_COLLECTIVES) {
        mpi_errno = MPID_Ineighbor_alltoall_sched(sendbuf, sendcount, sendtype, recvbuf, recvcount,
                                                  recvtype, comm_ptr, s);
    } else {
        mpi_errno = MPIR_Ineighbor_alltoall_sched_impl(sendbuf, sendcount, sendtype, recvbuf,
                                                       recvcount, recvtype, comm_ptr, s);
    }

    return mpi_errno;
}

int MPIR_Ineighbor_alltoall_impl(const void *sendbuf, int sendcount,
                                 MPI_Datatype sendtype, void *recvbuf,
                                 int recvcount, MPI_Datatype recvtype,
                                 MPIR_Comm * comm_ptr, MPIR_Request ** request)
{
    int mpi_errno = MPI_SUCCESS;
    int tag = -1;
    MPIR_Sched_t s = MPIR_SCHED_NULL;

    *request = NULL;
    /* If the user picks one of the transport-enabled algorithms, branch there
     * before going down to the MPIR_Sched-based algorithms. */
    /* TODO - Eventually the intention is to replace all of the
     * MPIR_Sched-based algorithms with transport-enabled algorithms, but that
     * will require sufficient performance testing and replacement algorithms. */
    if (comm_ptr->comm_kind == MPIR_COMM_KIND__INTRACOMM) {
        /* intracommunicator */
        switch (MPIR_CVAR_INEIGHBOR_ALLTOALL_INTRA_ALGORITHM) {
            case MPIR_CVAR_INEIGHBOR_ALLTOALL_INTRA_ALGORITHM_gentran_linear:
                mpi_errno =
                    MPIR_Ineighbor_alltoall_allcomm_gentran_linear(sendbuf, sendcount, sendtype,
                                                                   recvbuf, recvcount, recvtype,
                                                                   comm_ptr, request);
                MPIR_ERR_CHECK(mpi_errno);
                goto fn_exit;
                break;
            default:
                /* go down to the MPIR_Sched-based algorithms */
                break;
        }
    } else {
        /* intercommunicator */
        switch (MPIR_CVAR_INEIGHBOR_ALLTOALL_INTER_ALGORITHM) {
            case MPIR_CVAR_INEIGHBOR_ALLTOALL_INTER_ALGORITHM_gentran_linear:
                mpi_errno =
                    MPIR_Ineighbor_alltoall_allcomm_gentran_linear(sendbuf, sendcount, sendtype,
                                                                   recvbuf, recvcount, recvtype,
                                                                   comm_ptr, request);
                MPIR_ERR_CHECK(mpi_errno);
                goto fn_exit;
                break;
            default:
                /* go down to the MPIR_Sched-based algorithms */
                break;
        }
    }

    /* If the user doesn't pick a transport-enabled algorithm, go to the old
     * sched function. */
    mpi_errno = MPIR_Sched_next_tag(comm_ptr, &tag);
    MPIR_ERR_CHECK(mpi_errno);
    mpi_errno = MPIR_Sched_create(&s);
    MPIR_ERR_CHECK(mpi_errno);
    mpi_errno = MPIR_Ineighbor_alltoall_sched(sendbuf, sendcount, sendtype,
                                              recvbuf, recvcount, recvtype, comm_ptr, s);
    MPIR_ERR_CHECK(mpi_errno);

    mpi_errno = MPIR_Sched_start(&s, comm_ptr, tag, request);
    MPIR_ERR_CHECK(mpi_errno);


  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIR_Ineighbor_alltoall(const void *sendbuf, int sendcount,
                            MPI_Datatype sendtype, void *recvbuf,
                            int recvcount, MPI_Datatype recvtype,
                            MPIR_Comm * comm_ptr, MPIR_Request ** request)
{
    int mpi_errno = MPI_SUCCESS;

    if (MPIR_CVAR_BARRIER_DEVICE_COLLECTIVE && MPIR_CVAR_DEVICE_COLLECTIVES) {
        mpi_errno = MPID_Ineighbor_alltoall(sendbuf, sendcount, sendtype, recvbuf, recvcount,
                                            recvtype, comm_ptr, request);
    } else {
        mpi_errno = MPIR_Ineighbor_alltoall_impl(sendbuf, sendcount, sendtype, recvbuf, recvcount,
                                                 recvtype, comm_ptr, request);
    }

    return mpi_errno;
}

#endif /* MPICH_MPI_FROM_PMPI */

/*@
MPI_Ineighbor_alltoall - Nonblocking version of MPI_Neighbor_alltoall.

Input Parameters:
+ sendbuf - starting address of the send buffer (choice)
. sendcount - number of elements sent to each neighbor (non-negative integer)
. sendtype - data type of send buffer elements (handle)
. recvcount - number of elements received from each neighbor (non-negative integer)
. recvtype - data type of receive buffer elements (handle)
- comm - communicator (handle)

Output Parameters:
+ recvbuf - starting address of the receive buffer (choice)
- request - communication request (handle)

.N ThreadSafe

.N Fortran

.N Errors
@*/
int MPI_Ineighbor_alltoall(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf,
                           int recvcount, MPI_Datatype recvtype, MPI_Comm comm,
                           MPI_Request * request)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Comm *comm_ptr = NULL;
    MPIR_Request *request_ptr = NULL;


    MPID_THREAD_CS_ENTER(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    MPID_THREAD_CS_ENTER(VCI, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);


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
                if (mpi_errno != MPI_SUCCESS)
                    goto fn_fail;
                MPIR_Datatype_committed_ptr(sendtype_ptr, mpi_errno);
                if (mpi_errno != MPI_SUCCESS)
                    goto fn_fail;
            }

            if (!HANDLE_IS_BUILTIN(recvtype)) {
                MPIR_Datatype *recvtype_ptr = NULL;
                MPIR_Datatype_get_ptr(recvtype, recvtype_ptr);
                MPIR_Datatype_valid_ptr(recvtype_ptr, mpi_errno);
                if (mpi_errno != MPI_SUCCESS)
                    goto fn_fail;
                MPIR_Datatype_committed_ptr(recvtype_ptr, mpi_errno);
                if (mpi_errno != MPI_SUCCESS)
                    goto fn_fail;
            }

            MPIR_Comm_valid_ptr(comm_ptr, mpi_errno, FALSE);
            if (mpi_errno != MPI_SUCCESS)
                goto fn_fail;
            MPIR_ERRTEST_ARGNULL(request, "request", mpi_errno);
            /* TODO more checks may be appropriate (counts, in_place, buffer aliasing, etc) */
        }
        MPID_END_ERROR_CHECKS;
    }
#endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */

    mpi_errno =
        MPIR_Ineighbor_alltoall(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype,
                                comm_ptr, &request_ptr);
    MPIR_ERR_CHECK(mpi_errno);

    /* create a complete request, if needed */
    if (!request_ptr)
        request_ptr = MPIR_Request_create_complete(MPIR_REQUEST_KIND__COLL);
    /* return the handle of the request to the user */
    *request = request_ptr->handle;

    /* ... end of body of routine ... */

  fn_exit:

    MPID_THREAD_CS_EXIT(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    MPID_THREAD_CS_EXIT(VCI, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
#ifdef HAVE_ERROR_CHECKING
    {
        mpi_errno =
            MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, __func__, __LINE__, MPI_ERR_OTHER,
                                 "**mpi_ineighbor_alltoall",
                                 "**mpi_ineighbor_alltoall %p %d %D %p %d %D %C %p", sendbuf,
                                 sendcount, sendtype, recvbuf, recvcount, recvtype, comm, request);
    }
#endif
    mpi_errno = MPIR_Err_return_comm(NULL, __func__, mpi_errno);
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}
