/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/*
=== BEGIN_MPI_T_CVAR_INFO_BLOCK ===

categories :
   - name : COLLECTIVE
     description : A category for collective communication variables.

cvars:
   - name      : MPIR_CVAR_ALLTOALL_SHORT_MSG_SIZE
     category  : COLLECTIVE
     type      : int
     default   : 256
     class     : device
     verbosity : MPI_T_VERBOSITY_USER_BASIC
     scope     : MPI_T_SCOPE_ALL_EQ
     description : >-
       the short message algorithm will be used if the per-destination
       message size (sendcount*size(sendtype)) is <= this value
       (See also: MPIR_CVAR_ALLTOALL_MEDIUM_MSG_SIZE)

   - name      : MPIR_CVAR_ALLTOALL_MEDIUM_MSG_SIZE
     category  : COLLECTIVE
     type      : int
     default   : 32768
     class     : device
     verbosity : MPI_T_VERBOSITY_USER_BASIC
     scope     : MPI_T_SCOPE_ALL_EQ
     description : >-
       the medium message algorithm will be used if the per-destination
       message size (sendcount*size(sendtype)) is <= this value and
       larger than MPIR_CVAR_ALLTOALL_SHORT_MSG_SIZE
       (See also: MPIR_CVAR_ALLTOALL_SHORT_MSG_SIZE)

   - name      : MPIR_CVAR_ALLTOALL_THROTTLE
     category  : COLLECTIVE
     type      : int
     default   : 32
     class     : device
     verbosity : MPI_T_VERBOSITY_USER_BASIC
     scope     : MPI_T_SCOPE_ALL_EQ
     description : >-
       max no. of irecvs/isends posted at a time in some alltoall
       algorithms. Setting it to 0 causes all irecvs/isends to be
       posted at once

   - name        : MPIR_CVAR_ALLTOALL_INTRA_ALGORITHM
     category    : COLLECTIVE
     type        : string
     default     : auto
     class       : device
     verbosity   : MPI_T_VERBOSITY_USER_BASIC
     scope       : MPI_T_SCOPE_ALL_EQ
     description : |-
       Variable to select alltoall algorithm
       auto                      - Internal algorithm selection
       brucks                    - Force brucks algorithm
       nb                        - Force nonblocking algorithm
       pairwise                  - Force pairwise algorithm
       pairwise_sendrecv_replace - Force pairwise sendrecv replace algorithm
       scattered                 - Force scattered algorithm

   - name        : MPIR_CVAR_ALLTOALL_INTER_ALGORITHM
     category    : COLLECTIVE
     type        : string
     default     : auto
     class       : device
     verbosity   : MPI_T_VERBOSITY_USER_BASIC
     scope       : MPI_T_SCOPE_ALL_EQ
     description : |-
       Variable to select alltoall algorithm
       auto              - Internal algorithm selection
       nb                - Force nonblocking algorithm
       pairwise_exchange - Force pairwise exchange algorithm

   - name        : MPIR_CVAR_ALLTOALL_DEVICE_COLLECTIVE
     category    : COLLECTIVE
     type        : boolean
     default     : true
     class       : device
     verbosity   : MPI_T_VERBOSITY_USER_BASIC
     scope       : MPI_T_SCOPE_ALL_EQ
     description : >-
       If set to true, MPI_Alltoall will allow the device to override the
       MPIR-level collective algorithms. The device still has the
       option to call the MPIR-level algorithms manually.
       If set to false, the device-level alltoall function will not be
       called.

=== END_MPI_T_CVAR_INFO_BLOCK ===
*/

/* -- Begin Profiling Symbol Block for routine MPI_Alltoall */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Alltoall = PMPI_Alltoall
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Alltoall  MPI_Alltoall
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Alltoall as PMPI_Alltoall
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPI_Alltoall(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf,
                 int recvcount, MPI_Datatype recvtype, MPI_Comm comm)
    __attribute__ ((weak, alias("PMPI_Alltoall")));
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Alltoall
#define MPI_Alltoall PMPI_Alltoall

#undef FUNCNAME
#define FUNCNAME MPIR_Alltoall_intra_auto
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Alltoall_intra_auto(const void *sendbuf,
                             int sendcount,
                             MPI_Datatype sendtype,
                             void *recvbuf,
                             int recvcount,
                             MPI_Datatype recvtype, MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag)
{
    int comm_size;
    int mpi_errno = MPI_SUCCESS, nbytes;
    int sendtype_size;

    if (recvcount == 0)
        return MPI_SUCCESS;

    comm_size = comm_ptr->local_size;

    MPIR_Datatype_get_size_macro(sendtype, sendtype_size);
    nbytes = sendtype_size * sendcount;

    if (sendbuf == MPI_IN_PLACE) {
        mpi_errno =
            MPIR_Alltoall_intra_pairwise_sendrecv_replace(sendbuf, sendcount, sendtype, recvbuf,
                                                          recvcount, recvtype, comm_ptr, errflag);
    } else if ((nbytes <= MPIR_CVAR_ALLTOALL_SHORT_MSG_SIZE) && (comm_size >= 8)) {
        mpi_errno =
            MPIR_Alltoall_intra_brucks(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype,
                                       comm_ptr, errflag);
    } else if (nbytes <= MPIR_CVAR_ALLTOALL_MEDIUM_MSG_SIZE) {
        mpi_errno =
            MPIR_Alltoall_intra_scattered(sendbuf, sendcount, sendtype, recvbuf, recvcount,
                                          recvtype, comm_ptr, errflag);
    } else {
        mpi_errno =
            MPIR_Alltoall_intra_pairwise(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype,
                                         comm_ptr, errflag);
    }
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

  fn_exit:
    if (*errflag != MPIR_ERR_NONE)
        MPIR_ERR_SET(mpi_errno, *errflag, "**coll_fail");

    return mpi_errno;
  fn_fail:
    goto fn_exit;
}


#undef FUNCNAME
#define FUNCNAME MPIR_Alltoall_inter_auto
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Alltoall_inter_auto(const void *sendbuf,
                             int sendcount,
                             MPI_Datatype sendtype,
                             void *recvbuf,
                             int recvcount,
                             MPI_Datatype recvtype, MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag)
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = MPIR_Alltoall_inter_pairwise_exchange(sendbuf, sendcount, sendtype,
                                                      recvbuf, recvcount, recvtype, comm_ptr,
                                                      errflag);

    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIR_Alltoall_impl
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Alltoall_impl(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                       void *recvbuf, int recvcount, MPI_Datatype recvtype,
                       MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag)
{
    int mpi_errno = MPI_SUCCESS;

    if (comm_ptr->comm_kind == MPIR_COMM_KIND__INTRACOMM) {
        /* intracommunicator */
        switch (MPIR_Alltoall_intra_algo_choice) {
            case MPIR_ALLTOALL_INTRA_ALGO_BRUCKS:
                mpi_errno = MPIR_Alltoall_intra_brucks(sendbuf, sendcount, sendtype,
                                                       recvbuf, recvcount, recvtype,
                                                       comm_ptr, errflag);
                break;
            case MPIR_ALLTOALL_INTRA_ALGO_PAIRWISE:
                mpi_errno = MPIR_Alltoall_intra_pairwise(sendbuf, sendcount, sendtype,
                                                         recvbuf, recvcount, recvtype,
                                                         comm_ptr, errflag);
                break;
            case MPIR_ALLTOALL_INTRA_ALGO_PAIRWISE_SENDRECV_REPLACE:
                mpi_errno = MPIR_Alltoall_intra_pairwise_sendrecv_replace(sendbuf, sendcount,
                                                                          sendtype, recvbuf,
                                                                          recvcount, recvtype,
                                                                          comm_ptr, errflag);
                break;
            case MPIR_ALLTOALL_INTRA_ALGO_SCATTERED:
                mpi_errno = MPIR_Alltoall_intra_scattered(sendbuf, sendcount, sendtype,
                                                          recvbuf, recvcount, recvtype,
                                                          comm_ptr, errflag);
                break;
            case MPIR_ALLTOALL_INTRA_ALGO_NB:
                mpi_errno = MPIR_Alltoall_allcomm_nb(sendbuf, sendcount, sendtype,
                                                     recvbuf, recvcount, recvtype, comm_ptr,
                                                     errflag);
                break;
            case MPIR_ALLTOALL_INTRA_ALGO_AUTO:
                MPL_FALLTHROUGH;
            default:
                mpi_errno = MPIR_Alltoall_intra_auto(sendbuf, sendcount, sendtype,
                                                     recvbuf, recvcount, recvtype,
                                                     comm_ptr, errflag);
                break;
        }
    } else {
        /* intercommunicator */
        switch (MPIR_Alltoall_inter_algo_choice) {
            case MPIR_ALLTOALL_INTER_ALGO_PAIRWISE_EXCHANGE:
                mpi_errno = MPIR_Alltoall_inter_pairwise_exchange(sendbuf, sendcount, sendtype,
                                                                  recvbuf, recvcount, recvtype,
                                                                  comm_ptr, errflag);
                break;
            case MPIR_ALLTOALL_INTER_ALGO_NB:
                mpi_errno = MPIR_Alltoall_allcomm_nb(sendbuf, sendcount, sendtype,
                                                     recvbuf, recvcount, recvtype, comm_ptr,
                                                     errflag);
                break;
            case MPIR_ALLTOALL_INTER_ALGO_AUTO:
                MPL_FALLTHROUGH;
            default:
                mpi_errno = MPIR_Alltoall_inter_auto(sendbuf, sendcount, sendtype,
                                                     recvbuf, recvcount, recvtype,
                                                     comm_ptr, errflag);
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
#define FUNCNAME MPIR_Alltoall
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Alltoall(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                  void *recvbuf, int recvcount, MPI_Datatype recvtype,
                  MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag)
{
    int mpi_errno = MPI_SUCCESS;

    if (MPIR_CVAR_ALLTOALL_DEVICE_COLLECTIVE && MPIR_CVAR_DEVICE_COLLECTIVES) {
        mpi_errno = MPID_Alltoall(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype,
                                  comm_ptr, errflag);
    } else {
        mpi_errno = MPIR_Alltoall_impl(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype,
                                       comm_ptr, errflag);
    }

    return mpi_errno;
}

#endif

#undef FUNCNAME
#define FUNCNAME MPI_Alltoall
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
/*@
MPI_Alltoall - Sends data from all to all processes

Input Parameters:
+ sendbuf - starting address of send buffer (choice)
. sendcount - number of elements to send to each process (integer)
. sendtype - data type of send buffer elements (handle)
. recvcount - number of elements received from any process (integer)
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
int MPI_Alltoall(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                 void *recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Comm *comm_ptr = NULL;
    MPIR_Errflag_t errflag = MPIR_ERR_NONE;
    MPIR_FUNC_TERSE_STATE_DECL(MPID_STATE_MPI_ALLTOALL);

    MPIR_ERRTEST_INITIALIZED_ORDIE();

    MPID_THREAD_CS_ENTER(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    MPIR_FUNC_TERSE_COLL_ENTER(MPID_STATE_MPI_ALLTOALL);

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
            MPIR_Datatype *sendtype_ptr = NULL, *recvtype_ptr = NULL;

            MPIR_Comm_valid_ptr(comm_ptr, mpi_errno, FALSE);
            if (mpi_errno != MPI_SUCCESS)
                goto fn_fail;

            if (sendbuf != MPI_IN_PLACE) {
                MPIR_ERRTEST_COUNT(sendcount, mpi_errno);
                MPIR_ERRTEST_DATATYPE(sendtype, "sendtype", mpi_errno);
                if (HANDLE_GET_KIND(sendtype) != HANDLE_KIND_BUILTIN) {
                    MPIR_Datatype_get_ptr(sendtype, sendtype_ptr);
                    MPIR_Datatype_valid_ptr(sendtype_ptr, mpi_errno);
                    if (mpi_errno != MPI_SUCCESS)
                        goto fn_fail;
                    MPIR_Datatype_committed_ptr(sendtype_ptr, mpi_errno);
                    if (mpi_errno != MPI_SUCCESS)
                        goto fn_fail;
                }

                if (comm_ptr->comm_kind == MPIR_COMM_KIND__INTRACOMM &&
                    sendbuf != MPI_IN_PLACE &&
                    sendcount == recvcount && sendtype == recvtype && sendcount != 0)
                    MPIR_ERRTEST_ALIAS_COLL(sendbuf, recvbuf, mpi_errno);
            }

            MPIR_ERRTEST_COUNT(recvcount, mpi_errno);
            MPIR_ERRTEST_DATATYPE(recvtype, "recvtype", mpi_errno);
            if (HANDLE_GET_KIND(recvtype) != HANDLE_KIND_BUILTIN) {
                MPIR_Datatype_get_ptr(recvtype, recvtype_ptr);
                MPIR_Datatype_valid_ptr(recvtype_ptr, mpi_errno);
                if (mpi_errno != MPI_SUCCESS)
                    goto fn_fail;
                MPIR_Datatype_committed_ptr(recvtype_ptr, mpi_errno);
                if (mpi_errno != MPI_SUCCESS)
                    goto fn_fail;
            }

            if (comm_ptr->comm_kind == MPIR_COMM_KIND__INTERCOMM) {
                MPIR_ERRTEST_SENDBUF_INPLACE(sendbuf, sendcount, mpi_errno);
            }
            MPIR_ERRTEST_RECVBUF_INPLACE(recvbuf, recvcount, mpi_errno);
            MPIR_ERRTEST_USERBUFFER(sendbuf, sendcount, sendtype, mpi_errno);
            MPIR_ERRTEST_USERBUFFER(recvbuf, recvcount, recvtype, mpi_errno);
        }
        MPID_END_ERROR_CHECKS;
    }
#endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */

    mpi_errno = MPIR_Alltoall(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype,
                              comm_ptr, &errflag);
    if (mpi_errno)
        goto fn_fail;

    /* ... end of body of routine ... */

  fn_exit:
    MPIR_FUNC_TERSE_COLL_EXIT(MPID_STATE_MPI_ALLTOALL);
    MPID_THREAD_CS_EXIT(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
#ifdef HAVE_ERROR_CHECKING
    {
        mpi_errno =
            MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER,
                                 "**mpi_alltoall", "**mpi_alltoall %p %d %D %p %d %D %C", sendbuf,
                                 sendcount, sendtype, recvbuf, recvcount, recvtype, comm);
    }
#endif
    mpi_errno = MPIR_Err_return_comm(comm_ptr, FCNAME, mpi_errno);
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}
