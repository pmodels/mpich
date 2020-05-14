/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"

/*
=== BEGIN_MPI_T_CVAR_INFO_BLOCK ===

cvars:
    - name        : MPIR_CVAR_IGATHER_INTRA_ALGORITHM
      category    : COLLECTIVE
      type        : enum
      default     : auto
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : |-
        Variable to select igather algorithm
        auto - Internal algorithm selection (can be overridden with MPIR_CVAR_COLL_SELECTION_TUNING_JSON_FILE)
        sched_auto - Internal algorithm selection for sched-based algorithms
        sched_binomial     - Force binomial algorithm
        gentran_tree       - Force genetric transport based tree algorithm

    - name        : MPIR_CVAR_IGATHER_TREE_KVAL
      category    : COLLECTIVE
      type        : int
      default     : 2
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        k value for tree based igather

    - name        : MPIR_CVAR_IGATHER_INTER_ALGORITHM
      category    : COLLECTIVE
      type        : enum
      default     : auto
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : |-
        Variable to select igather algorithm
        auto - Internal algorithm selection (can be overridden with MPIR_CVAR_COLL_SELECTION_TUNING_JSON_FILE)
        sched_auto - Internal algorithm selection for sched-based algorithms
        sched_long  - Force long inter algorithm
        sched_short - Force short inter algorithm

    - name        : MPIR_CVAR_IGATHER_DEVICE_COLLECTIVE
      category    : COLLECTIVE
      type        : boolean
      default     : true
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        This CVAR is only used when MPIR_CVAR_DEVICE_COLLECTIVES
        is set to "percoll".  If set to true, MPI_Igather will
        allow the device to override the MPIR-level collective
        algorithms.  The device might still call the MPIR-level
        algorithms manually.  If set to false, the device-override
        will be disabled.

=== END_MPI_T_CVAR_INFO_BLOCK ===
*/

/* -- Begin Profiling Symbol Block for routine MPI_Igather */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Igather = PMPI_Igather
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Igather  MPI_Igather
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Igather as PMPI_Igather
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPI_Igather(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf,
                int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm,
                MPI_Request * request)
    __attribute__ ((weak, alias("PMPI_Igather")));
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Igather
#define MPI_Igather PMPI_Igather

int MPIR_Igather_allcomm_auto(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                              void *recvbuf, int recvcount, MPI_Datatype recvtype, int root,
                              MPIR_Comm * comm_ptr, MPIR_Request ** request)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_Csel_coll_sig_s coll_sig = {
        .coll_type = MPIR_CSEL_COLL_TYPE__IGATHER,
        .comm_ptr = comm_ptr,

        .u.igather.sendbuf = sendbuf,
        .u.igather.sendcount = sendcount,
        .u.igather.sendtype = sendtype,
        .u.igather.recvcount = recvcount,
        .u.igather.recvbuf = recvbuf,
        .u.igather.recvtype = recvtype,
        .u.igather.root = root,
    };

    MPII_Csel_container_s *cnt = MPIR_Csel_search(comm_ptr->csel_comm, coll_sig);
    MPIR_Assert(cnt);

    switch (cnt->id) {
        case MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Igather_intra_gentran_tree:
            mpi_errno =
                MPIR_Igather_intra_gentran_tree(sendbuf, sendcount, sendtype, recvbuf, recvcount,
                                                recvtype, root, comm_ptr,
                                                cnt->u.igather.intra_gentran_tree.k, request);
            break;

        case MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Igather_intra_sched_auto:
            MPII_SCHED_WRAPPER(MPIR_Igather_intra_sched_auto, comm_ptr, request, sendbuf, sendcount,
                               sendtype, recvbuf, recvcount, recvtype, root);
            break;

        case MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Igather_intra_sched_binomial:
            MPII_SCHED_WRAPPER(MPIR_Igather_intra_sched_binomial, comm_ptr, request, sendbuf,
                               sendcount, sendtype, recvbuf, recvcount, recvtype, root);
            break;

        case MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Igather_inter_sched_auto:
            MPII_SCHED_WRAPPER(MPIR_Igather_inter_sched_auto, comm_ptr, request, sendbuf, sendcount,
                               sendtype, recvbuf, recvcount, recvtype, root);
            break;

        case MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Igather_inter_sched_long:
            MPII_SCHED_WRAPPER(MPIR_Igather_inter_sched_long, comm_ptr, request, sendbuf, sendcount,
                               sendtype, recvbuf, recvcount, recvtype, root);
            break;

        case MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Igather_inter_sched_short:
            MPII_SCHED_WRAPPER(MPIR_Igather_inter_sched_short, comm_ptr, request, sendbuf,
                               sendcount, sendtype, recvbuf, recvcount, recvtype, root);
            break;

        default:
            MPIR_Assert(0);
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIR_Igather_intra_sched_auto(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                                  void *recvbuf, int recvcount, MPI_Datatype recvtype, int root,
                                  MPIR_Comm * comm_ptr, MPIR_Sched_t s)
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno =
        MPIR_Igather_intra_sched_binomial(sendbuf, sendcount, sendtype, recvbuf, recvcount,
                                          recvtype, root, comm_ptr, s);
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIR_Igather_inter_sched_auto(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                                  void *recvbuf, int recvcount, MPI_Datatype recvtype, int root,
                                  MPIR_Comm * comm_ptr, MPIR_Sched_t s)
{
    int mpi_errno = MPI_SUCCESS;
    MPI_Aint local_size, remote_size;
    MPI_Aint recvtype_size, sendtype_size, nbytes;

    if (root == MPI_PROC_NULL) {
        /* local processes other than root do nothing */
        goto fn_exit;
    }

    remote_size = comm_ptr->remote_size;
    local_size = comm_ptr->local_size;

    if (root == MPI_ROOT) {
        MPIR_Datatype_get_size_macro(recvtype, recvtype_size);
        nbytes = recvtype_size * recvcount * remote_size;
    } else {
        /* remote side */
        MPIR_Datatype_get_size_macro(sendtype, sendtype_size);
        nbytes = sendtype_size * sendcount * local_size;
    }

    if (nbytes < MPIR_CVAR_GATHER_INTER_SHORT_MSG_SIZE) {
        mpi_errno =
            MPIR_Igather_inter_sched_short(sendbuf, sendcount, sendtype, recvbuf, recvcount,
                                           recvtype, root, comm_ptr, s);
    } else {
        mpi_errno =
            MPIR_Igather_inter_sched_long(sendbuf, sendcount, sendtype, recvbuf, recvcount,
                                          recvtype, root, comm_ptr, s);
    }

  fn_exit:
    return mpi_errno;
}

int MPIR_Igather_sched_auto(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                            void *recvbuf, int recvcount, MPI_Datatype recvtype,
                            int root, MPIR_Comm * comm_ptr, MPIR_Sched_t s)
{
    int mpi_errno = MPI_SUCCESS;

    if (comm_ptr->comm_kind == MPIR_COMM_KIND__INTRACOMM) {
        mpi_errno = MPIR_Igather_intra_sched_auto(sendbuf, sendcount, sendtype,
                                                  recvbuf, recvcount, recvtype, root, comm_ptr, s);
    } else {
        mpi_errno = MPIR_Igather_inter_sched_auto(sendbuf, sendcount, sendtype, recvbuf,
                                                  recvcount, recvtype, root, comm_ptr, s);
    }

    return mpi_errno;
}

int MPIR_Igather_impl(const void *sendbuf, int sendcount,
                      MPI_Datatype sendtype, void *recvbuf, int recvcount,
                      MPI_Datatype recvtype, int root, MPIR_Comm * comm_ptr,
                      MPIR_Request ** request)
{
    int mpi_errno = MPI_SUCCESS;

    *request = NULL;

    /* If the user picks one of the transport-enabled algorithms, branch there
     * before going down to the MPIR_Sched-based algorithms. */
    /* TODO - Eventually the intention is to replace all of the
     * MPIR_Sched-based algorithms with transport-enabled algorithms, but that
     * will require sufficient performance testing and replacement algorithms. */
    if (comm_ptr->comm_kind == MPIR_COMM_KIND__INTRACOMM) {
        /* intracommunicator */
        switch (MPIR_CVAR_IGATHER_INTRA_ALGORITHM) {
            case MPIR_CVAR_IGATHER_INTRA_ALGORITHM_gentran_tree:
                mpi_errno =
                    MPIR_Igather_intra_gentran_tree(sendbuf, sendcount, sendtype,
                                                    recvbuf, recvcount, recvtype, root, comm_ptr,
                                                    MPIR_CVAR_IGATHER_TREE_KVAL, request);
                break;

            case MPIR_CVAR_IGATHER_INTRA_ALGORITHM_sched_binomial:
                MPII_SCHED_WRAPPER(MPIR_Igather_intra_sched_binomial, comm_ptr, request, sendbuf,
                                   sendcount, sendtype, recvbuf, recvcount, recvtype, root);
                break;

            case MPIR_CVAR_IGATHER_INTRA_ALGORITHM_sched_auto:
                MPII_SCHED_WRAPPER(MPIR_Igather_intra_sched_auto, comm_ptr, request, sendbuf,
                                   sendcount, sendtype, recvbuf, recvcount, recvtype, root);
                break;

            case MPIR_CVAR_IGATHER_INTRA_ALGORITHM_auto:
                mpi_errno =
                    MPIR_Igather_allcomm_auto(sendbuf, sendcount, sendtype, recvbuf, recvcount,
                                              recvtype, root, comm_ptr, request);
                break;

            default:
                MPIR_Assert(0);
        }
    } else {
        switch (MPIR_CVAR_IGATHER_INTER_ALGORITHM) {
            case MPIR_CVAR_IGATHER_INTER_ALGORITHM_sched_long:
                MPII_SCHED_WRAPPER(MPIR_Igather_inter_sched_long, comm_ptr, request, sendbuf,
                                   sendcount, sendtype, recvbuf, recvcount, recvtype, root);
                break;

            case MPIR_CVAR_IGATHER_INTER_ALGORITHM_sched_short:
                MPII_SCHED_WRAPPER(MPIR_Igather_inter_sched_short, comm_ptr, request, sendbuf,
                                   sendcount, sendtype, recvbuf, recvcount, recvtype, root);
                break;

            case MPIR_CVAR_IGATHER_INTER_ALGORITHM_sched_auto:
                MPII_SCHED_WRAPPER(MPIR_Igather_inter_sched_auto, comm_ptr, request, sendbuf,
                                   sendcount, sendtype, recvbuf, recvcount, recvtype, root);
                break;

            case MPIR_CVAR_IGATHER_INTER_ALGORITHM_auto:
                mpi_errno =
                    MPIR_Igather_allcomm_auto(sendbuf, sendcount, sendtype, recvbuf, recvcount,
                                              recvtype, root, comm_ptr, request);
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

int MPIR_Igather(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                 void *recvbuf, int recvcount, MPI_Datatype recvtype, int root,
                 MPIR_Comm * comm_ptr, MPIR_Request ** request)
{
    int mpi_errno = MPI_SUCCESS;

    if ((MPIR_CVAR_DEVICE_COLLECTIVES == MPIR_CVAR_DEVICE_COLLECTIVES_all) ||
        ((MPIR_CVAR_DEVICE_COLLECTIVES == MPIR_CVAR_DEVICE_COLLECTIVES_percoll) &&
         MPIR_CVAR_IGATHER_DEVICE_COLLECTIVE)) {
        mpi_errno =
            MPID_Igather(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, root, comm_ptr,
                         request);
    } else {
        mpi_errno = MPIR_Igather_impl(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype,
                                      root, comm_ptr, request);
    }

    return mpi_errno;
}

#endif /* MPICH_MPI_FROM_PMPI */

/*@
MPI_Igather - Gathers together values from a group of processes in
              a nonblocking way

Input Parameters:
+ sendbuf - starting address of the send buffer (choice)
. sendcount - number of elements in send buffer (non-negative integer)
. sendtype - data type of send buffer elements (handle)
. recvcount - number of elements for any single receive (non-negative integer, significant only at root)
. recvtype - data type of receive buffer elements (significant only at root) (handle)
. root - rank of receiving process (integer)
- comm - communicator (handle)

Output Parameters:
+ recvbuf - starting address of the receive buffer (significant only at root) (choice)
- request - communication request (handle)

.N ThreadSafe

.N Fortran

.N Errors
@*/
int MPI_Igather(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                void *recvbuf, int recvcount, MPI_Datatype recvtype,
                int root, MPI_Comm comm, MPI_Request * request)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Comm *comm_ptr = NULL;
    MPIR_Request *request_ptr = NULL;
    MPIR_FUNC_TERSE_STATE_DECL(MPID_STATE_MPI_IGATHER);

    MPID_THREAD_CS_ENTER(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    MPIR_FUNC_TERSE_ENTER(MPID_STATE_MPI_IGATHER);

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
            int rank;

            MPIR_Comm_valid_ptr(comm_ptr, mpi_errno, FALSE);
            if (mpi_errno != MPI_SUCCESS)
                goto fn_fail;

            if (comm_ptr->comm_kind == MPIR_COMM_KIND__INTRACOMM) {
                MPIR_ERRTEST_INTRA_ROOT(comm_ptr, root, mpi_errno);

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

                rank = comm_ptr->rank;
                if (rank == root) {
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
                    MPIR_ERRTEST_RECVBUF_INPLACE(recvbuf, recvcount, mpi_errno);
                    MPIR_ERRTEST_USERBUFFER(recvbuf, recvcount, recvtype, mpi_errno);

                    /* catch common aliasing cases */
                    if (recvbuf != MPI_IN_PLACE && sendtype == recvtype && sendcount == recvcount &&
                        sendcount != 0) {
                        MPI_Aint recvtype_size;
                        MPIR_Datatype_get_size_macro(recvtype, recvtype_size);
                        MPIR_ERRTEST_ALIAS_COLL(sendbuf,
                                                (char *) recvbuf +
                                                comm_ptr->rank * recvcount * recvtype_size,
                                                mpi_errno);
                    }
                } else
                    MPIR_ERRTEST_SENDBUF_INPLACE(sendbuf, sendcount, mpi_errno);
            }

            if (comm_ptr->comm_kind == MPIR_COMM_KIND__INTERCOMM) {
                MPIR_ERRTEST_INTER_ROOT(comm_ptr, root, mpi_errno);

                if (root == MPI_ROOT) {
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
                    MPIR_ERRTEST_RECVBUF_INPLACE(recvbuf, recvcount, mpi_errno);
                    MPIR_ERRTEST_USERBUFFER(recvbuf, recvcount, recvtype, mpi_errno);
                }

                else if (root != MPI_PROC_NULL) {
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
                    MPIR_ERRTEST_SENDBUF_INPLACE(sendbuf, sendcount, mpi_errno);
                    MPIR_ERRTEST_USERBUFFER(sendbuf, sendcount, sendtype, mpi_errno);
                }
            }
        }
        MPID_END_ERROR_CHECKS;
    }
#endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */

    mpi_errno = MPIR_Igather(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype,
                             root, comm_ptr, &request_ptr);
    MPIR_ERR_CHECK(mpi_errno);

    /* create a complete request, if needed */
    if (!request_ptr)
        request_ptr = MPIR_Request_create_complete(MPIR_REQUEST_KIND__COLL);
    /* return the handle of the request to the user */
    *request = request_ptr->handle;

    /* ... end of body of routine ... */

  fn_exit:
    MPIR_FUNC_TERSE_EXIT(MPID_STATE_MPI_IGATHER);
    MPID_THREAD_CS_EXIT(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
#ifdef HAVE_ERROR_CHECKING
    {
        mpi_errno =
            MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, __func__, __LINE__, MPI_ERR_OTHER,
                                 "**mpi_igather", "**mpi_igather %p %d %D %p %d %D %d %C %p",
                                 sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, root,
                                 comm, request);
    }
#endif
    mpi_errno = MPIR_Err_return_comm(comm_ptr, __func__, mpi_errno);
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}
