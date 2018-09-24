/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"


/*
=== BEGIN_MPI_T_CVAR_INFO_BLOCK ===

cvars:
    - name        : MPIR_CVAR_ALLTOALLV_INTRA_ALGORITHM
      category    : COLLECTIVE
      type        : string
      default     : auto
      class       : device
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : |-
        Variable to select alltoallv algorithm
        auto                      - Internal algorithm selection
        nb                        - Force nonblocking algorithm
        pairwise_sendrecv_replace - Force pairwise_sendrecv_replace algorithm
        scattered                 - Force scattered algorithm

    - name        : MPIR_CVAR_ALLTOALLV_INTER_ALGORITHM
      category    : COLLECTIVE
      type        : string
      default     : auto
      class       : device
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : |-
        Variable to select alltoallv algorithm
        auto              - Internal algorithm selection
        pairwise_exchange - Force pairwise exchange algorithm
        nb                - Force nonblocking algorithm

    - name        : MPIR_CVAR_ALLTOALLV_DEVICE_COLLECTIVE
      category    : COLLECTIVE
      type        : boolean
      default     : true
      class       : device
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        If set to true, MPI_Alltoallv will allow the device to override the
        MPIR-level collective algorithms. The device still has the
        option to call the MPIR-level algorithms manually.
        If set to false, the device-level alltoallv function will not be
        called.

=== END_MPI_T_CVAR_INFO_BLOCK ===
*/

/* -- Begin Profiling Symbol Block for routine MPI_Alltoallv */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Alltoallv = PMPI_Alltoallv
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Alltoallv  MPI_Alltoallv
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Alltoallv as PMPI_Alltoallv
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPI_Alltoallv(const void *sendbuf, const int *sendcounts, const int *sdispls,
                  MPI_Datatype sendtype, void *recvbuf, const int *recvcounts,
                  const int *rdispls, MPI_Datatype recvtype, MPI_Comm comm)
    __attribute__ ((weak, alias("PMPI_Alltoallv")));
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Alltoallv
#define MPI_Alltoallv PMPI_Alltoallv

#undef FUNCNAME
#define FUNCNAME MPIR_Alltoallv_intra_auto
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Alltoallv_intra_auto(const void *sendbuf, const int *sendcounts, const int *sdispls,
                              MPI_Datatype sendtype, void *recvbuf, const int *recvcounts,
                              const int *rdispls, MPI_Datatype recvtype, MPIR_Comm * comm_ptr,
                              MPIR_Errflag_t * errflag)
{
    int mpi_errno = MPI_SUCCESS;

    if (sendbuf == MPI_IN_PLACE) {
        mpi_errno =
            MPIR_Alltoallv_intra_pairwise_sendrecv_replace(sendbuf, sendcounts, sdispls, sendtype,
                                                           recvbuf, recvcounts, rdispls, recvtype,
                                                           comm_ptr, errflag);
        if (mpi_errno)
            MPIR_ERR_POP(mpi_errno);
    } else {
        mpi_errno = MPIR_Alltoallv_intra_scattered(sendbuf, sendcounts, sdispls, sendtype,
                                                   recvbuf, recvcounts, rdispls, recvtype, comm_ptr,
                                                   errflag);
        if (mpi_errno)
            MPIR_ERR_POP(mpi_errno);
    }

  fn_exit:
    if (*errflag != MPIR_ERR_NONE)
        MPIR_ERR_SET(mpi_errno, *errflag, "**coll_fail");

    return mpi_errno;

  fn_fail:
    goto fn_exit;
}


#undef FUNCNAME
#define FUNCNAME MPIR_Alltoallv_inter_auto
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Alltoallv_inter_auto(const void *sendbuf, const int *sendcounts, const int *sdispls,
                              MPI_Datatype sendtype, void *recvbuf, const int *recvcounts,
                              const int *rdispls, MPI_Datatype recvtype, MPIR_Comm * comm_ptr,
                              MPIR_Errflag_t * errflag)
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = MPIR_Alltoallv_inter_pairwise_exchange(sendbuf, sendcounts, sdispls,
                                                       sendtype, recvbuf, recvcounts, rdispls,
                                                       recvtype, comm_ptr, errflag);

    return mpi_errno;
}


#undef FUNCNAME
#define FUNCNAME MPIR_Alltoallv_impl
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Alltoallv_impl(const void *sendbuf, const int *sendcounts, const int *sdispls,
                        MPI_Datatype sendtype, void *recvbuf, const int *recvcounts,
                        const int *rdispls, MPI_Datatype recvtype, MPIR_Comm * comm_ptr,
                        MPIR_Errflag_t * errflag)
{
    int mpi_errno = MPI_SUCCESS;

    if (comm_ptr->comm_kind == MPIR_COMM_KIND__INTRACOMM) {
        /* intracommunicator */
        switch (MPIR_Alltoallv_intra_algo_choice) {
            case MPIR_ALLTOALLV_INTRA_ALGO_PAIRWISE_SENDRECV_REPLACE:
                mpi_errno = MPIR_Alltoallv_intra_pairwise_sendrecv_replace(sendbuf, sendcounts,
                                                                           sdispls, sendtype,
                                                                           recvbuf, recvcounts,
                                                                           rdispls, recvtype,
                                                                           comm_ptr, errflag);
                break;
            case MPIR_ALLTOALLV_INTRA_ALGO_SCATTERED:
                mpi_errno = MPIR_Alltoallv_intra_scattered(sendbuf, sendcounts, sdispls,
                                                           sendtype, recvbuf, recvcounts,
                                                           rdispls, recvtype, comm_ptr, errflag);
                break;
            case MPIR_ALLTOALLV_INTRA_ALGO_NB:
                mpi_errno = MPIR_Alltoallv_allcomm_nb(sendbuf, sendcounts, sdispls,
                                                      sendtype, recvbuf, recvcounts,
                                                      rdispls, recvtype, comm_ptr, errflag);
                break;
            case MPIR_ALLTOALLV_INTRA_ALGO_AUTO:
                MPL_FALLTHROUGH;
            default:
                mpi_errno = MPIR_Alltoallv_intra_auto(sendbuf, sendcounts, sdispls,
                                                      sendtype, recvbuf, recvcounts,
                                                      rdispls, recvtype, comm_ptr, errflag);
                break;
        }
    } else {
        switch (MPIR_Alltoallv_inter_algo_choice) {
                /* intercommunicator */
            case MPIR_ALLTOALLV_INTER_ALGO_PAIRWISE_EXCHANGE:
                mpi_errno = MPIR_Alltoallv_inter_pairwise_exchange(sendbuf, sendcounts, sdispls,
                                                                   sendtype, recvbuf, recvcounts,
                                                                   rdispls, recvtype, comm_ptr,
                                                                   errflag);
                break;
            case MPIR_ALLTOALLV_INTER_ALGO_NB:
                mpi_errno = MPIR_Alltoallv_allcomm_nb(sendbuf, sendcounts, sdispls,
                                                      sendtype, recvbuf, recvcounts,
                                                      rdispls, recvtype, comm_ptr, errflag);
                break;
            case MPIR_ALLTOALLV_INTER_ALGO_AUTO:
                MPL_FALLTHROUGH;
            default:
                mpi_errno = MPIR_Alltoallv_inter_auto(sendbuf, sendcounts, sdispls,
                                                      sendtype, recvbuf, recvcounts,
                                                      rdispls, recvtype, comm_ptr, errflag);
                break;
        }
    }
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIR_Alltoallv
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Alltoallv(const void *sendbuf, const int *sendcounts, const int *sdispls,
                   MPI_Datatype sendtype, void *recvbuf, const int *recvcounts, const int *rdispls,
                   MPI_Datatype recvtype, MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag)
{
    int mpi_errno = MPI_SUCCESS;

    if (MPIR_CVAR_ALLTOALLV_DEVICE_COLLECTIVE && MPIR_CVAR_DEVICE_COLLECTIVES) {
        mpi_errno = MPID_Alltoallv(sendbuf, sendcounts, sdispls, sendtype, recvbuf, recvcounts,
                                   rdispls, recvtype, comm_ptr, errflag);
    } else {
        mpi_errno = MPIR_Alltoallv_impl(sendbuf, sendcounts, sdispls, sendtype, recvbuf, recvcounts,
                                        rdispls, recvtype, comm_ptr, errflag);
    }

    return mpi_errno;
}

#endif


#undef FUNCNAME
#define FUNCNAME MPI_Alltoallv
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
/*@
MPI_Alltoallv - Sends data from all to all processes; each process may
   send a different amount of data and provide displacements for the input
   and output data.

Input Parameters:
+ sendbuf - starting address of send buffer (choice)
. sendcounts - integer array equal to the group size
specifying the number of elements to send to each processor
. sdispls - integer array (of length group size). Entry
 'j'  specifies the displacement (relative to sendbuf  from
which to take the outgoing data destined for process  'j'
. sendtype - data type of send buffer elements (handle)
. recvcounts - integer array equal to the group size
specifying the maximum number of elements that can be received from
each processor
. rdispls - integer array (of length group size). Entry
 'i'  specifies the displacement (relative to recvbuf  at
which to place the incoming data from process  'i'
. recvtype - data type of receive buffer elements (handle)
- comm - communicator (handle)

Output Parameters:
. recvbuf - address of receive buffer (choice)

.N ThreadSafe

.N Fortran

.N Errors
.N MPI_ERR_COMM
.N MPI_ERR_COUNT
.N MPI_ERR_TYPE
.N MPI_ERR_BUFFER
@*/
int MPI_Alltoallv(const void *sendbuf, const int *sendcounts,
                  const int *sdispls, MPI_Datatype sendtype, void *recvbuf,
                  const int *recvcounts, const int *rdispls, MPI_Datatype recvtype, MPI_Comm comm)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Comm *comm_ptr = NULL;
    MPIR_Errflag_t errflag = MPIR_ERR_NONE;
    MPIR_FUNC_TERSE_STATE_DECL(MPID_STATE_MPI_ALLTOALLV);

    MPIR_ERRTEST_INITIALIZED_ORDIE();

    MPID_THREAD_CS_ENTER(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    MPIR_FUNC_TERSE_COLL_ENTER(MPID_STATE_MPI_ALLTOALLV);

    /* Validate parameters, especially handles needing to be converted */
#ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            MPIR_ERRTEST_COMM(comm, mpi_errno);
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
            MPIR_Datatype *sendtype_ptr = NULL, *recvtype_ptr = NULL;
            int i, comm_size;
            int check_send = (comm_ptr->comm_kind == MPIR_COMM_KIND__INTRACOMM &&
                              sendbuf != MPI_IN_PLACE);

            MPIR_Comm_valid_ptr(comm_ptr, mpi_errno, FALSE);
            if (mpi_errno != MPI_SUCCESS)
                goto fn_fail;

            if (comm_ptr->comm_kind == MPIR_COMM_KIND__INTRACOMM) {
                comm_size = comm_ptr->local_size;

                if (sendbuf != MPI_IN_PLACE && sendtype == recvtype && sendcounts == recvcounts)
                    MPIR_ERRTEST_ALIAS_COLL(sendbuf, recvbuf, mpi_errno);
            } else
                comm_size = comm_ptr->remote_size;

            if (comm_ptr->comm_kind == MPIR_COMM_KIND__INTERCOMM && sendbuf == MPI_IN_PLACE) {
                MPIR_ERR_SETANDJUMP(mpi_errno, MPI_ERR_OTHER, "**sendbuf_inplace");
            }

            for (i = 0; i < comm_size; i++) {
                if (check_send) {
                    MPIR_ERRTEST_COUNT(sendcounts[i], mpi_errno);
                    MPIR_ERRTEST_DATATYPE(sendtype, "sendtype", mpi_errno);
                }
                MPIR_ERRTEST_COUNT(recvcounts[i], mpi_errno);
                MPIR_ERRTEST_DATATYPE(recvtype, "recvtype", mpi_errno);
            }
            if (check_send && HANDLE_GET_KIND(sendtype) != HANDLE_KIND_BUILTIN) {
                MPIR_Datatype_get_ptr(sendtype, sendtype_ptr);
                MPIR_Datatype_valid_ptr(sendtype_ptr, mpi_errno);
                if (mpi_errno != MPI_SUCCESS)
                    goto fn_fail;
                MPIR_Datatype_committed_ptr(sendtype_ptr, mpi_errno);
                if (mpi_errno != MPI_SUCCESS)
                    goto fn_fail;
            }
            if (HANDLE_GET_KIND(recvtype) != HANDLE_KIND_BUILTIN) {
                MPIR_Datatype_get_ptr(recvtype, recvtype_ptr);
                MPIR_Datatype_valid_ptr(recvtype_ptr, mpi_errno);
                if (mpi_errno != MPI_SUCCESS)
                    goto fn_fail;
                MPIR_Datatype_committed_ptr(recvtype_ptr, mpi_errno);
                if (mpi_errno != MPI_SUCCESS)
                    goto fn_fail;
            }

            for (i = 0; i < comm_size && check_send; i++) {
                if (sendcounts[i] > 0) {
                    MPIR_ERRTEST_USERBUFFER(sendbuf, sendcounts[i], sendtype, mpi_errno);
                }
            }
            for (i = 0; i < comm_size; i++) {
                if (recvcounts[i] > 0) {
                    MPIR_ERRTEST_RECVBUF_INPLACE(recvbuf, recvcounts[i], mpi_errno);
                    MPIR_ERRTEST_USERBUFFER(recvbuf, recvcounts[i], recvtype, mpi_errno);
                    break;
                }
            }
        }
        MPID_END_ERROR_CHECKS;
    }
#endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */

    mpi_errno = MPIR_Alltoallv(sendbuf, sendcounts, sdispls,
                               sendtype, recvbuf, recvcounts,
                               rdispls, recvtype, comm_ptr, &errflag);
    if (mpi_errno)
        goto fn_fail;

    /* ... end of body of routine ... */

  fn_exit:
    MPIR_FUNC_TERSE_COLL_EXIT(MPID_STATE_MPI_ALLTOALLV);
    MPID_THREAD_CS_EXIT(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
#ifdef HAVE_ERROR_CHECKING
    {
        mpi_errno =
            MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER,
                                 "**mpi_alltoallv", "**mpi_alltoallv %p %p %p %D %p %p %p %D %C",
                                 sendbuf, sendcounts, sdispls, sendtype, recvbuf, recvcounts,
                                 rdispls, recvtype, comm);
    }
#endif
    mpi_errno = MPIR_Err_return_comm(comm_ptr, FCNAME, mpi_errno);
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}
