/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"

/*
=== BEGIN_MPI_T_CVAR_INFO_BLOCK ===

cvars:
    - name        : MPIR_CVAR_ALLGATHER_SHORT_MSG_SIZE
      category    : COLLECTIVE
      type        : int
      default     : 81920
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        For MPI_Allgather and MPI_Allgatherv, the short message algorithm will
        be used if the send buffer size is < this value (in bytes).
        (See also: MPIR_CVAR_ALLGATHER_LONG_MSG_SIZE)

    - name        : MPIR_CVAR_ALLGATHER_LONG_MSG_SIZE
      category    : COLLECTIVE
      type        : int
      default     : 524288
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        For MPI_Allgather and MPI_Allgatherv, the long message algorithm will be
        used if the send buffer size is >= this value (in bytes)
        (See also: MPIR_CVAR_ALLGATHER_SHORT_MSG_SIZE)

    - name        : MPIR_CVAR_ALLGATHER_INTRA_ALGORITHM
      category    : COLLECTIVE
      type        : enum
      default     : auto
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : |-
        Variable to select allgather algorithm
        auto - Internal algorithm selection (can be overridden with MPIR_CVAR_COLL_SELECTION_TUNING_JSON_FILE)
        brucks             - Force brucks algorithm
        nb                 - Force nonblocking algorithm
        recursive_doubling - Force recursive doubling algorithm
        ring               - Force ring algorithm

    - name        : MPIR_CVAR_ALLGATHER_INTER_ALGORITHM
      category    : COLLECTIVE
      type        : enum
      default     : auto
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : |-
        Variable to select allgather algorithm
        auto - Internal algorithm selection (can be overridden with MPIR_CVAR_COLL_SELECTION_TUNING_JSON_FILE)
        local_gather_remote_bcast - Force local-gather-remote-bcast algorithm
        nb                        - Force nonblocking algorithm

    - name        : MPIR_CVAR_ALLGATHER_DEVICE_COLLECTIVE
      category    : COLLECTIVE
      type        : boolean
      default     : true
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        This CVAR is only used when MPIR_CVAR_DEVICE_COLLECTIVES
        is set to "percoll".  If set to true, MPI_Allgather will
        allow the device to override the MPIR-level collective
        algorithms.  The device might still call the MPIR-level
        algorithms manually.  If set to false, the device-override
        will be disabled.

=== END_MPI_T_CVAR_INFO_BLOCK ===
*/

/* -- Begin Profiling Symbol Block for routine MPI_Allgather */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Allgather = PMPI_Allgather
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Allgather  MPI_Allgather
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Allgather as PMPI_Allgather
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPI_Allgather(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf,
                  int recvcount, MPI_Datatype recvtype, MPI_Comm comm)
    __attribute__ ((weak, alias("PMPI_Allgather")));
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Allgather
#define MPI_Allgather PMPI_Allgather

int MPIR_Allgather_allcomm_auto(const void *sendbuf,
                                int sendcount,
                                MPI_Datatype sendtype,
                                void *recvbuf,
                                int recvcount,
                                MPI_Datatype recvtype, MPIR_Comm * comm_ptr,
                                MPIR_Errflag_t * errflag)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_Csel_coll_sig_s coll_sig = {
        .coll_type = MPIR_CSEL_COLL_TYPE__ALLGATHER,
        .comm_ptr = comm_ptr,

        .u.allgather.sendbuf = sendbuf,
        .u.allgather.sendcount = sendcount,
        .u.allgather.sendtype = sendtype,
        .u.allgather.recvbuf = recvbuf,
        .u.allgather.recvcount = recvcount,
        .u.allgather.recvtype = recvtype,
    };

    MPII_Csel_container_s *cnt = MPIR_Csel_search(comm_ptr->csel_comm, coll_sig);
    MPIR_Assert(cnt);

    switch (cnt->id) {
        case MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Allgather_intra_brucks:
            mpi_errno =
                MPIR_Allgather_intra_brucks(sendbuf, sendcount, sendtype, recvbuf, recvcount,
                                            recvtype, comm_ptr, errflag);
            break;

        case MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Allgather_intra_recursive_doubling:
            mpi_errno =
                MPIR_Allgather_intra_recursive_doubling(sendbuf, sendcount, sendtype, recvbuf,
                                                        recvcount, recvtype, comm_ptr, errflag);
            break;

        case MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Allgather_intra_ring:
            mpi_errno =
                MPIR_Allgather_intra_ring(sendbuf, sendcount, sendtype, recvbuf, recvcount,
                                          recvtype, comm_ptr, errflag);
            break;

        case MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Allgather_inter_local_gather_remote_bcast:
            mpi_errno =
                MPIR_Allgather_inter_local_gather_remote_bcast(sendbuf, sendcount, sendtype,
                                                               recvbuf, recvcount, recvtype,
                                                               comm_ptr, errflag);
            break;

        case MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Allgather_allcomm_nb:
            mpi_errno =
                MPIR_Allgather_allcomm_nb(sendbuf, sendcount, sendtype, recvbuf, recvcount,
                                          recvtype, comm_ptr, errflag);
            break;

        default:
            MPIR_Assert(0);
    }

    return mpi_errno;
}

int MPIR_Allgather_impl(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                        void *recvbuf, int recvcount, MPI_Datatype recvtype,
                        MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag)
{
    int mpi_errno = MPI_SUCCESS;

    if (comm_ptr->comm_kind == MPIR_COMM_KIND__INTRACOMM) {
        /* intracommunicator */
        switch (MPIR_CVAR_ALLGATHER_INTRA_ALGORITHM) {
            case MPIR_CVAR_ALLGATHER_INTRA_ALGORITHM_brucks:
                mpi_errno =
                    MPIR_Allgather_intra_brucks(sendbuf, sendcount, sendtype, recvbuf, recvcount,
                                                recvtype, comm_ptr, errflag);
                break;
            case MPIR_CVAR_ALLGATHER_INTRA_ALGORITHM_recursive_doubling:
                mpi_errno =
                    MPIR_Allgather_intra_recursive_doubling(sendbuf, sendcount, sendtype, recvbuf,
                                                            recvcount, recvtype, comm_ptr, errflag);
                break;
            case MPIR_CVAR_ALLGATHER_INTRA_ALGORITHM_ring:
                mpi_errno =
                    MPIR_Allgather_intra_ring(sendbuf, sendcount, sendtype, recvbuf, recvcount,
                                              recvtype, comm_ptr, errflag);
                break;
            case MPIR_CVAR_ALLGATHER_INTRA_ALGORITHM_nb:
                mpi_errno =
                    MPIR_Allgather_allcomm_nb(sendbuf, sendcount, sendtype, recvbuf, recvcount,
                                              recvtype, comm_ptr, errflag);
                break;
            case MPIR_CVAR_ALLGATHER_INTRA_ALGORITHM_auto:
                mpi_errno =
                    MPIR_Allgather_allcomm_auto(sendbuf, sendcount, sendtype, recvbuf, recvcount,
                                                recvtype, comm_ptr, errflag);
                break;
            default:
                MPIR_Assert(0);
        }
    } else {
        /* intercommunicator */
        switch (MPIR_CVAR_ALLGATHER_INTER_ALGORITHM) {
            case MPIR_CVAR_ALLGATHER_INTER_ALGORITHM_local_gather_remote_bcast:
                mpi_errno =
                    MPIR_Allgather_inter_local_gather_remote_bcast(sendbuf, sendcount, sendtype,
                                                                   recvbuf, recvcount, recvtype,
                                                                   comm_ptr, errflag);
                break;
            case MPIR_CVAR_ALLGATHER_INTER_ALGORITHM_nb:
                mpi_errno =
                    MPIR_Allgather_allcomm_nb(sendbuf, sendcount, sendtype, recvbuf, recvcount,
                                              recvtype, comm_ptr, errflag);
                break;
            case MPIR_CVAR_ALLGATHER_INTER_ALGORITHM_auto:
                mpi_errno =
                    MPIR_Allgather_allcomm_auto(sendbuf, sendcount, sendtype, recvbuf, recvcount,
                                                recvtype, comm_ptr, errflag);
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

int MPIR_Allgather(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                   void *recvbuf, int recvcount, MPI_Datatype recvtype,
                   MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag)
{
    int mpi_errno = MPI_SUCCESS;

    if ((MPIR_CVAR_DEVICE_COLLECTIVES == MPIR_CVAR_DEVICE_COLLECTIVES_all) ||
        ((MPIR_CVAR_DEVICE_COLLECTIVES == MPIR_CVAR_DEVICE_COLLECTIVES_percoll) &&
         MPIR_CVAR_ALLGATHER_DEVICE_COLLECTIVE)) {
        mpi_errno =
            MPID_Allgather(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, comm_ptr,
                           errflag);
    } else {
        mpi_errno = MPIR_Allgather_impl(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype,
                                        comm_ptr, errflag);
    }

    return mpi_errno;
}

#endif


/*@
MPI_Allgather - Gathers data from all tasks and distribute the combined
    data to all tasks

Input Parameters:
+ sendbuf - starting address of send buffer (choice)
. sendcount - number of elements in send buffer (integer)
. sendtype - data type of send buffer elements (handle)
. recvcount - number of elements received from any process (integer)
. recvtype - data type of receive buffer elements (handle)
- comm - communicator (handle)

Output Parameters:
. recvbuf - address of receive buffer (choice)

Notes:
 The MPI standard (1.0 and 1.1) says that
.n
.n
 The jth block of data sent from  each process is received by every process
 and placed in the jth block of the buffer 'recvbuf'.
.n
.n
 This is misleading; a better description is
.n
.n
 The block of data sent from the jth process is received by every
 process and placed in the jth block of the buffer 'recvbuf'.
.n
.n
 This text was suggested by Rajeev Thakur and has been adopted as a
 clarification by the MPI Forum.

.N ThreadSafe

.N Fortran

.N Errors
.N MPI_ERR_COMM
.N MPI_ERR_COUNT
.N MPI_ERR_TYPE
.N MPI_ERR_BUFFER
@*/
int MPI_Allgather(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                  void *recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Comm *comm_ptr = NULL;
    MPIR_Errflag_t errflag = MPIR_ERR_NONE;
    MPIR_FUNC_TERSE_STATE_DECL(MPID_STATE_MPI_ALLGATHER);

    MPIR_ERRTEST_INITIALIZED_ORDIE();

    MPID_THREAD_CS_ENTER(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    MPIR_FUNC_TERSE_COLL_ENTER(MPID_STATE_MPI_ALLGATHER);

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

            MPIR_Comm_valid_ptr(comm_ptr, mpi_errno, FALSE);
            if (mpi_errno != MPI_SUCCESS)
                goto fn_fail;

            if (comm_ptr->comm_kind == MPIR_COMM_KIND__INTERCOMM) {
                MPIR_ERRTEST_SENDBUF_INPLACE(sendbuf, sendcount, mpi_errno);
            } else {
                /* catch common aliasing cases */
                if (sendbuf != MPI_IN_PLACE && sendtype == recvtype && recvcount != 0 &&
                    sendcount != 0) {
                    int recvtype_size;
                    MPIR_Datatype_get_size_macro(recvtype, recvtype_size);
                    MPIR_ERRTEST_ALIAS_COLL(sendbuf,
                                            (char *) recvbuf +
                                            comm_ptr->rank * recvcount * recvtype_size, mpi_errno);
                }
            }

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
            }

            MPIR_ERRTEST_RECVBUF_INPLACE(recvbuf, recvcount, mpi_errno);
            MPIR_ERRTEST_COUNT(recvcount, mpi_errno);
            MPIR_ERRTEST_DATATYPE(recvtype, "recvtype", mpi_errno);
            if (!HANDLE_IS_BUILTIN(recvtype)) {
                MPIR_Datatype_get_ptr(recvtype, recvtype_ptr);
                MPIR_Datatype_valid_ptr(recvtype_ptr, mpi_errno);
                if (mpi_errno != MPI_SUCCESS)
                    goto fn_fail;
                MPIR_Datatype_committed_ptr(recvtype_ptr, mpi_errno);
                if (mpi_errno != MPI_SUCCESS)
                    goto fn_fail;
            }
            MPIR_ERRTEST_USERBUFFER(recvbuf, recvcount, recvtype, mpi_errno);
        }
        MPID_END_ERROR_CHECKS;
    }
#endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */
    mpi_errno = MPIR_Allgather(sendbuf, sendcount, sendtype,
                               recvbuf, recvcount, recvtype, comm_ptr, &errflag);
    if (mpi_errno)
        goto fn_fail;

    /* ... end of body of routine ... */

  fn_exit:
    MPIR_FUNC_TERSE_COLL_EXIT(MPID_STATE_MPI_ALLGATHER);
    MPID_THREAD_CS_EXIT(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
#ifdef HAVE_ERROR_CHECKING
    {
        mpi_errno =
            MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, __func__, __LINE__, MPI_ERR_OTHER,
                                 "**mpi_allgather", "**mpi_allgather %p %d %D %p %d %D %C", sendbuf,
                                 sendcount, sendtype, recvbuf, recvcount, recvtype, comm);
    }
#endif
    mpi_errno = MPIR_Err_return_comm(comm_ptr, __func__, mpi_errno);
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}
