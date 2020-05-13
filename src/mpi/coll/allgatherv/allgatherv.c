/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"

/*
=== BEGIN_MPI_T_CVAR_INFO_BLOCK ===

cvars:
    - name        : MPIR_CVAR_ALLGATHERV_PIPELINE_MSG_SIZE
      category    : COLLECTIVE
      type        : int
      default     : 32768
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        The smallest message size that will be used for the pipelined, large-message,
        ring algorithm in the MPI_Allgatherv implementation.

    - name        : MPIR_CVAR_ALLGATHERV_INTRA_ALGORITHM
      category    : COLLECTIVE
      type        : enum
      default     : auto
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : |-
        Variable to select allgatherv algorithm
        auto - Internal algorithm selection (can be overridden with MPIR_CVAR_COLL_SELECTION_TUNING_JSON_FILE)
        brucks             - Force brucks algorithm
        nb                 - Force nonblocking algorithm
        recursive_doubling - Force recursive doubling algorithm
        ring               - Force ring algorithm

    - name        : MPIR_CVAR_ALLGATHERV_INTER_ALGORITHM
      category    : COLLECTIVE
      type        : enum
      default     : auto
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : |-
        Variable to select allgatherv algorithm
        auto - Internal algorithm selection (can be overridden with MPIR_CVAR_COLL_SELECTION_TUNING_JSON_FILE)
        nb                        - Force nonblocking algorithm
        remote_gather_local_bcast - Force remote-gather-local-bcast algorithm

    - name        : MPIR_CVAR_ALLGATHERV_DEVICE_COLLECTIVE
      category    : COLLECTIVE
      type        : boolean
      default     : true
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        This CVAR is only used when MPIR_CVAR_DEVICE_COLLECTIVES
        is set to "percoll".  If set to true, MPI_Allgatherv will
        allow the device to override the MPIR-level collective
        algorithms.  The device might still call the MPIR-level
        algorithms manually.  If set to false, the device-override
        will be disabled.

=== END_MPI_T_CVAR_INFO_BLOCK ===
*/

/* -- Begin Profiling Symbol Block for routine MPI_Allgatherv */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Allgatherv = PMPI_Allgatherv
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Allgatherv  MPI_Allgatherv
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Allgatherv as PMPI_Allgatherv
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPI_Allgatherv(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf,
                   const int *recvcounts, const int *displs, MPI_Datatype recvtype, MPI_Comm comm)
    __attribute__ ((weak, alias("PMPI_Allgatherv")));
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Allgatherv
#define MPI_Allgatherv PMPI_Allgatherv

int MPIR_Allgatherv_allcomm_auto(const void *sendbuf,
                                 int sendcount,
                                 MPI_Datatype sendtype,
                                 void *recvbuf,
                                 const int *recvcounts,
                                 const int *displs,
                                 MPI_Datatype recvtype,
                                 MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_Csel_coll_sig_s coll_sig = {
        .coll_type = MPIR_CSEL_COLL_TYPE__ALLGATHERV,
        .comm_ptr = comm_ptr,

        .u.allgatherv.sendbuf = sendbuf,
        .u.allgatherv.sendcount = sendcount,
        .u.allgatherv.sendtype = sendtype,
        .u.allgatherv.recvbuf = recvbuf,
        .u.allgatherv.recvcounts = recvcounts,
        .u.allgatherv.displs = displs,
        .u.allgatherv.recvtype = recvtype,
    };

    MPII_Csel_container_s *cnt = MPIR_Csel_search(comm_ptr->csel_comm, coll_sig);
    MPIR_Assert(cnt);

    switch (cnt->id) {
        case MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Allgatherv_intra_brucks:
            mpi_errno =
                MPIR_Allgatherv_intra_brucks(sendbuf, sendcount, sendtype, recvbuf, recvcounts,
                                             displs, recvtype, comm_ptr, errflag);
            break;

        case MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Allgatherv_intra_recursive_doubling:
            mpi_errno =
                MPIR_Allgatherv_intra_recursive_doubling(sendbuf, sendcount, sendtype, recvbuf,
                                                         recvcounts, displs, recvtype, comm_ptr,
                                                         errflag);
            break;

        case MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Allgatherv_intra_ring:
            mpi_errno =
                MPIR_Allgatherv_intra_ring(sendbuf, sendcount, sendtype, recvbuf, recvcounts,
                                           displs, recvtype, comm_ptr, errflag);
            break;

        case MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Allgatherv_inter_remote_gather_local_bcast:
            mpi_errno =
                MPIR_Allgatherv_inter_remote_gather_local_bcast(sendbuf, sendcount, sendtype,
                                                                recvbuf, recvcounts, displs,
                                                                recvtype, comm_ptr, errflag);
            break;

        case MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Allgatherv_allcomm_nb:
            mpi_errno =
                MPIR_Allgatherv_allcomm_nb(sendbuf, sendcount, sendtype, recvbuf, recvcounts,
                                           displs, recvtype, comm_ptr, errflag);
            break;

        default:
            MPIR_Assert(0);
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIR_Allgatherv_impl(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                         void *recvbuf, const int *recvcounts, const int *displs,
                         MPI_Datatype recvtype, MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag)
{
    int mpi_errno = MPI_SUCCESS;

    if (comm_ptr->comm_kind == MPIR_COMM_KIND__INTRACOMM) {
        /* intracommunicator */
        switch (MPIR_CVAR_ALLGATHERV_INTRA_ALGORITHM) {
            case MPIR_CVAR_ALLGATHERV_INTRA_ALGORITHM_brucks:
                mpi_errno = MPIR_Allgatherv_intra_brucks(sendbuf, sendcount, sendtype,
                                                         recvbuf, recvcounts, displs, recvtype,
                                                         comm_ptr, errflag);
                break;
            case MPIR_CVAR_ALLGATHERV_INTRA_ALGORITHM_recursive_doubling:
                mpi_errno = MPIR_Allgatherv_intra_recursive_doubling(sendbuf, sendcount, sendtype,
                                                                     recvbuf, recvcounts, displs,
                                                                     recvtype, comm_ptr, errflag);
                break;
            case MPIR_CVAR_ALLGATHERV_INTRA_ALGORITHM_ring:
                mpi_errno = MPIR_Allgatherv_intra_ring(sendbuf, sendcount, sendtype,
                                                       recvbuf, recvcounts, displs, recvtype,
                                                       comm_ptr, errflag);
                break;
            case MPIR_CVAR_ALLGATHERV_INTRA_ALGORITHM_nb:
                mpi_errno = MPIR_Allgatherv_allcomm_nb(sendbuf, sendcount, sendtype,
                                                       recvbuf, recvcounts, displs, recvtype,
                                                       comm_ptr, errflag);
                break;
            case MPIR_CVAR_ALLGATHERV_INTRA_ALGORITHM_auto:
                mpi_errno = MPIR_Allgatherv_allcomm_auto(sendbuf, sendcount, sendtype,
                                                         recvbuf, recvcounts, displs, recvtype,
                                                         comm_ptr, errflag);
                break;
            default:
                MPIR_Assert(0);
        }
    } else {
        /* intercommunicator */
        switch (MPIR_CVAR_ALLGATHERV_INTER_ALGORITHM) {
            case MPIR_CVAR_ALLGATHERV_INTER_ALGORITHM_remote_gather_local_bcast:
                mpi_errno =
                    MPIR_Allgatherv_inter_remote_gather_local_bcast(sendbuf, sendcount, sendtype,
                                                                    recvbuf, recvcounts, displs,
                                                                    recvtype, comm_ptr, errflag);
                break;
            case MPIR_CVAR_ALLGATHERV_INTER_ALGORITHM_nb:
                mpi_errno = MPIR_Allgatherv_allcomm_nb(sendbuf, sendcount, sendtype,
                                                       recvbuf, recvcounts, displs, recvtype,
                                                       comm_ptr, errflag);
                break;
            case MPIR_CVAR_ALLGATHERV_INTER_ALGORITHM_auto:
                mpi_errno = MPIR_Allgatherv_allcomm_auto(sendbuf, sendcount, sendtype,
                                                         recvbuf, recvcounts, displs, recvtype,
                                                         comm_ptr, errflag);
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

int MPIR_Allgatherv(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                    void *recvbuf, const int *recvcounts, const int *displs, MPI_Datatype recvtype,
                    MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag)
{
    int mpi_errno = MPI_SUCCESS;

    if ((MPIR_CVAR_DEVICE_COLLECTIVES == MPIR_CVAR_DEVICE_COLLECTIVES_all) ||
        ((MPIR_CVAR_DEVICE_COLLECTIVES == MPIR_CVAR_DEVICE_COLLECTIVES_percoll) &&
         MPIR_CVAR_ALLGATHERV_DEVICE_COLLECTIVE)) {
        mpi_errno =
            MPID_Allgatherv(sendbuf, sendcount, sendtype, recvbuf, recvcounts, displs, recvtype,
                            comm_ptr, errflag);
    } else {
        mpi_errno = MPIR_Allgatherv_impl(sendbuf, sendcount, sendtype,
                                         recvbuf, recvcounts, displs, recvtype, comm_ptr, errflag);
    }

    return mpi_errno;
}

#endif


/*@

MPI_Allgatherv - Gathers data from all tasks and deliver the combined data
                 to all tasks

Input Parameters:
+ sendbuf - starting address of send buffer (choice)
. sendcount - number of elements in send buffer (integer)
. sendtype - data type of send buffer elements (handle)
. recvcounts - integer array (of length group size)
containing the number of elements that are to be received from each process
. displs - integer array (of length group size). Entry
 'i'  specifies the displacement (relative to recvbuf) at
which to place the incoming data from process  'i'
. recvtype - data type of receive buffer elements (handle)
- comm - communicator (handle)

Output Parameters:
. recvbuf - address of receive buffer (choice)

Notes:
 The MPI standard (1.0 and 1.1) says that
.n
.n
 The jth block of data sent from
 each process is received by every process and placed in the jth block of the
 buffer 'recvbuf'.
.n
.n
 This is misleading; a better description is
.n
.n
 The block of data sent from the jth process is received by every
 process and placed in the jth block of the buffer 'recvbuf'.
.n
.n
 This text was suggested by Rajeev Thakur, and has been adopted as a
 clarification to the MPI standard by the MPI-Forum.

.N ThreadSafe

.N Fortran

.N Errors
.N MPI_ERR_BUFFER
.N MPI_ERR_COUNT
.N MPI_ERR_TYPE
@*/
int MPI_Allgatherv(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                   void *recvbuf, const int *recvcounts, const int *displs,
                   MPI_Datatype recvtype, MPI_Comm comm)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Comm *comm_ptr = NULL;
    MPIR_Errflag_t errflag = MPIR_ERR_NONE;
    MPIR_FUNC_TERSE_STATE_DECL(MPID_STATE_MPI_ALLGATHERV);

    MPIR_ERRTEST_INITIALIZED_ORDIE();

    MPID_THREAD_CS_ENTER(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    MPIR_FUNC_TERSE_COLL_ENTER(MPID_STATE_MPI_ALLGATHERV);

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

    /* Validate parameters and objects (post conversion) */
#ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            MPIR_Datatype *recvtype_ptr = NULL, *sendtype_ptr = NULL;
            int i, comm_size;

            MPIR_Comm_valid_ptr(comm_ptr, mpi_errno, FALSE);
            if (mpi_errno != MPI_SUCCESS)
                goto fn_fail;

            if (comm_ptr->comm_kind == MPIR_COMM_KIND__INTERCOMM)
                MPIR_ERRTEST_SENDBUF_INPLACE(sendbuf, sendcount, mpi_errno);
            if (sendbuf != MPI_IN_PLACE) {
                MPIR_ERRTEST_COUNT(sendcount, mpi_errno);
                MPIR_ERRTEST_DATATYPE(sendtype, "sendtype", mpi_errno);
                if (!HANDLE_IS_BUILTIN(sendtype)) {
                    MPIR_Datatype_get_ptr(sendtype, sendtype_ptr);
                    MPIR_Datatype_valid_ptr(sendtype_ptr, mpi_errno);
                    if (mpi_errno != MPI_SUCCESS)
                        goto fn_fail;
                    MPIR_Datatype_committed_ptr(sendtype_ptr, mpi_errno);
                    if (mpi_errno != MPI_SUCCESS)
                        goto fn_fail;
                }
                MPIR_ERRTEST_USERBUFFER(sendbuf, sendcount, sendtype, mpi_errno);

                /* catch common aliasing cases */
                if (comm_ptr->comm_kind == MPIR_COMM_KIND__INTRACOMM &&
                    sendtype == recvtype && recvcounts[comm_ptr->rank] != 0 && sendcount != 0) {
                    int recvtype_size;
                    MPIR_Datatype_get_size_macro(recvtype, recvtype_size);
                    MPIR_ERRTEST_ALIAS_COLL(sendbuf,
                                            (char *) recvbuf +
                                            displs[comm_ptr->rank] * recvtype_size, mpi_errno);
                }
            }

            if (comm_ptr->comm_kind == MPIR_COMM_KIND__INTRACOMM)
                comm_size = comm_ptr->local_size;
            else
                comm_size = comm_ptr->remote_size;

            for (i = 0; i < comm_size; i++) {
                MPIR_ERRTEST_COUNT(recvcounts[i], mpi_errno);
                MPIR_ERRTEST_DATATYPE(recvtype, "recvtype", mpi_errno);
            }

            if (!HANDLE_IS_BUILTIN(recvtype)) {
                MPIR_Datatype_get_ptr(recvtype, recvtype_ptr);
                MPIR_Datatype_valid_ptr(recvtype_ptr, mpi_errno);
                if (mpi_errno != MPI_SUCCESS)
                    goto fn_fail;
                MPIR_Datatype_committed_ptr(recvtype_ptr, mpi_errno);
                if (mpi_errno != MPI_SUCCESS)
                    goto fn_fail;
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

    mpi_errno = MPIR_Allgatherv(sendbuf, sendcount, sendtype,
                                recvbuf, recvcounts, displs, recvtype, comm_ptr, &errflag);
    if (mpi_errno)
        goto fn_fail;

    /* ... end of body of routine ... */

  fn_exit:
    MPIR_FUNC_TERSE_COLL_EXIT(MPID_STATE_MPI_ALLGATHERV);
    MPID_THREAD_CS_EXIT(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
#ifdef HAVE_ERROR_CHECKING
    {
        mpi_errno =
            MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, __func__, __LINE__, MPI_ERR_OTHER,
                                 "**mpi_allgatherv", "**mpi_allgatherv %p %d %D %p %p %p %D %C",
                                 sendbuf, sendcount, sendtype, recvbuf, recvcounts, displs,
                                 recvtype, comm);
    }
#endif
    mpi_errno = MPIR_Err_return_comm(comm_ptr, __func__, mpi_errno);
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}
