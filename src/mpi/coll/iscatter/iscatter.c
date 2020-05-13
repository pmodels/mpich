/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"

/*
=== BEGIN_MPI_T_CVAR_INFO_BLOCK ===

cvars:
    - name        : MPIR_CVAR_ISCATTER_INTRA_ALGORITHM
      category    : COLLECTIVE
      type        : enum
      default     : auto
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : |-
        Variable to select iscatter algorithm
        auto - Internal algorithm selection (can be overridden with MPIR_CVAR_COLL_SELECTION_TUNING_JSON_FILE)
        sched_auto - Internal algorithm selection for sched-based algorithms
        sched_binomial     - Force binomial algorithm
        gentran_tree       - Force genetric transport based tree algorithm

    - name        : MPIR_CVAR_ISCATTER_TREE_KVAL
      category    : COLLECTIVE
      type        : int
      default     : 2
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        k value for tree based iscatter

    - name        : MPIR_CVAR_ISCATTER_INTER_ALGORITHM
      category    : COLLECTIVE
      type        : enum
      default     : auto
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : |-
        Variable to select iscatter algorithm
        auto - Internal algorithm selection (can be overridden with MPIR_CVAR_COLL_SELECTION_TUNING_JSON_FILE)
        sched_auto - Internal algorithm selection for sched-based algorithms
        sched_linear                    - Force linear algorithm
        sched_remote_send_local_scatter - Force remote-send-local-scatter algorithm

    - name        : MPIR_CVAR_ISCATTER_DEVICE_COLLECTIVE
      category    : COLLECTIVE
      type        : boolean
      default     : true
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        This CVAR is only used when MPIR_CVAR_DEVICE_COLLECTIVES
        is set to "percoll".  If set to true, MPI_Iscatter will
        allow the device to override the MPIR-level collective
        algorithms.  The device might still call the MPIR-level
        algorithms manually.  If set to false, the device-override
        will be disabled.

=== END_MPI_T_CVAR_INFO_BLOCK ===
*/

/* -- Begin Profiling Symbol Block for routine MPI_Iscatter */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Iscatter = PMPI_Iscatter
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Iscatter  MPI_Iscatter
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Iscatter as PMPI_Iscatter
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPI_Iscatter(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf,
                 int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm,
                 MPI_Request * request)
    __attribute__ ((weak, alias("PMPI_Iscatter")));
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Iscatter
#define MPI_Iscatter PMPI_Iscatter

/* helper callbacks and associated state structures */
struct shared_state {
    int sendcount;
    int curr_count;
    MPI_Aint send_subtree_count;
    int nbytes;
    MPI_Status status;
};

/* any non-MPI functions go here, especially non-static ones */

int MPIR_Iscatter_allcomm_auto(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                               void *recvbuf, int recvcount, MPI_Datatype recvtype, int root,
                               MPIR_Comm * comm_ptr, MPIR_Request ** request)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_Csel_coll_sig_s coll_sig = {
        .coll_type = MPIR_CSEL_COLL_TYPE__ISCATTER,
        .comm_ptr = comm_ptr,

        .u.iscatter.sendbuf = sendbuf,
        .u.iscatter.sendcount = sendcount,
        .u.iscatter.sendtype = sendtype,
        .u.iscatter.recvcount = recvcount,
        .u.iscatter.recvbuf = recvbuf,
        .u.iscatter.recvtype = recvtype,
        .u.iscatter.root = root,
    };

    MPII_Csel_container_s *cnt = MPIR_Csel_search(comm_ptr->csel_comm, coll_sig);
    MPIR_Assert(cnt);

    switch (cnt->id) {
        case MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Iscatter_intra_gentran_tree:
            mpi_errno =
                MPIR_Iscatter_intra_gentran_tree(sendbuf, sendcount, sendtype, recvbuf, recvcount,
                                                 recvtype, root, comm_ptr,
                                                 cnt->u.iscatter.intra_gentran_tree.k, request);
            break;

        case MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Iscatter_intra_sched_auto:
            MPII_SCHED_WRAPPER(MPIR_Iscatter_intra_sched_auto, comm_ptr, request, sendbuf,
                               sendcount, sendtype, recvbuf, recvcount, recvtype, root);
            break;

        case MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Iscatter_intra_sched_binomial:
            MPII_SCHED_WRAPPER(MPIR_Iscatter_intra_sched_binomial, comm_ptr, request, sendbuf,
                               sendcount, sendtype, recvbuf, recvcount, recvtype, root);
            break;

        case MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Iscatter_inter_sched_auto:
            MPII_SCHED_WRAPPER(MPIR_Iscatter_inter_sched_auto, comm_ptr, request, sendbuf,
                               sendcount, sendtype, recvbuf, recvcount, recvtype, root);
            break;

        case MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Iscatter_inter_sched_linear:
            MPII_SCHED_WRAPPER(MPIR_Iscatter_inter_sched_linear, comm_ptr, request, sendbuf,
                               sendcount, sendtype, recvbuf, recvcount, recvtype, root);
            break;

        case MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Iscatter_inter_sched_remote_send_local_scatter:
            MPII_SCHED_WRAPPER(MPIR_Iscatter_inter_sched_remote_send_local_scatter, comm_ptr,
                               request, sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype,
                               root);
            break;

        default:
            MPIR_Assert(0);
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIR_Iscatter_intra_sched_auto(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                                   void *recvbuf, int recvcount, MPI_Datatype recvtype,
                                   int root, MPIR_Comm * comm_ptr, MPIR_Sched_t s)
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno =
        MPIR_Iscatter_intra_sched_binomial(sendbuf, sendcount, sendtype, recvbuf, recvcount,
                                           recvtype, root, comm_ptr, s);
    MPIR_ERR_CHECK(mpi_errno);


  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIR_Iscatter_inter_sched_auto(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                                   void *recvbuf, int recvcount, MPI_Datatype recvtype,
                                   int root, MPIR_Comm * comm_ptr, MPIR_Sched_t s)
{
    int local_size, remote_size, sendtype_size, recvtype_size, nbytes;
    int mpi_errno = MPI_SUCCESS;

    remote_size = comm_ptr->remote_size;
    local_size = comm_ptr->local_size;

    if (root == MPI_ROOT) {
        MPIR_Datatype_get_size_macro(sendtype, sendtype_size);
        nbytes = sendtype_size * sendcount * remote_size;
    } else {
        MPIR_Datatype_get_size_macro(recvtype, recvtype_size);
        nbytes = recvtype_size * recvcount * local_size;
    }

    if (nbytes < MPIR_CVAR_SCATTER_INTER_SHORT_MSG_SIZE) {
        mpi_errno =
            MPIR_Iscatter_inter_sched_remote_send_local_scatter(sendbuf, sendcount, sendtype,
                                                                recvbuf, recvcount, recvtype, root,
                                                                comm_ptr, s);
    } else {
        mpi_errno = MPIR_Iscatter_inter_sched_linear(sendbuf, sendcount, sendtype,
                                                     recvbuf, recvcount, recvtype, root, comm_ptr,
                                                     s);
    }
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    return mpi_errno;

  fn_fail:
    goto fn_exit;
}

int MPIR_Iscatter_sched_auto(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                             void *recvbuf, int recvcount, MPI_Datatype recvtype,
                             int root, MPIR_Comm * comm_ptr, MPIR_Sched_t s)
{
    int mpi_errno = MPI_SUCCESS;

    if (comm_ptr->comm_kind == MPIR_COMM_KIND__INTRACOMM) {
        mpi_errno = MPIR_Iscatter_intra_sched_auto(sendbuf, sendcount, sendtype,
                                                   recvbuf, recvcount, recvtype, root, comm_ptr, s);
    } else {
        mpi_errno = MPIR_Iscatter_inter_sched_auto(sendbuf, sendcount, sendtype,
                                                   recvbuf, recvcount, recvtype, root, comm_ptr, s);
    }

    return mpi_errno;
}

int MPIR_Iscatter_impl(const void *sendbuf, int sendcount,
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
        switch (MPIR_CVAR_ISCATTER_INTRA_ALGORITHM) {
            case MPIR_CVAR_ISCATTER_INTRA_ALGORITHM_gentran_tree:
                mpi_errno =
                    MPIR_Iscatter_intra_gentran_tree(sendbuf, sendcount, sendtype,
                                                     recvbuf, recvcount, recvtype, root, comm_ptr,
                                                     MPIR_CVAR_ISCATTER_TREE_KVAL, request);
                break;

            case MPIR_CVAR_ISCATTER_INTRA_ALGORITHM_sched_binomial:
                MPII_SCHED_WRAPPER(MPIR_Iscatter_intra_sched_binomial, comm_ptr, request, sendbuf,
                                   sendcount, sendtype, recvbuf, recvcount, recvtype, root);
                break;

            case MPIR_CVAR_ISCATTER_INTRA_ALGORITHM_sched_auto:
                MPII_SCHED_WRAPPER(MPIR_Iscatter_intra_sched_auto, comm_ptr, request, sendbuf,
                                   sendcount, sendtype, recvbuf, recvcount, recvtype, root);
                break;

            case MPIR_CVAR_ISCATTER_INTRA_ALGORITHM_auto:
                mpi_errno =
                    MPIR_Iscatter_allcomm_auto(sendbuf, sendcount, sendtype, recvbuf, recvcount,
                                               recvtype, root, comm_ptr, request);
                break;

            default:
                MPIR_Assert(0);
        }
    } else {
        switch (MPIR_CVAR_ISCATTER_INTER_ALGORITHM) {
            case MPIR_CVAR_ISCATTER_INTER_ALGORITHM_sched_linear:
                MPII_SCHED_WRAPPER(MPIR_Iscatter_inter_sched_linear, comm_ptr, request, sendbuf,
                                   sendcount, sendtype, recvbuf, recvcount, recvtype, root);
                break;

            case MPIR_CVAR_ISCATTER_INTER_ALGORITHM_sched_remote_send_local_scatter:
                MPII_SCHED_WRAPPER(MPIR_Iscatter_inter_sched_remote_send_local_scatter, comm_ptr,
                                   request, sendbuf, sendcount, sendtype, recvbuf, recvcount,
                                   recvtype, root);
                break;

            case MPIR_CVAR_ISCATTER_INTER_ALGORITHM_sched_auto:
                MPII_SCHED_WRAPPER(MPIR_Iscatter_inter_sched_auto, comm_ptr, request, sendbuf,
                                   sendcount, sendtype, recvbuf, recvcount, recvtype, root);
                break;

            case MPIR_CVAR_ISCATTER_INTER_ALGORITHM_auto:
                mpi_errno =
                    MPIR_Iscatter_allcomm_auto(sendbuf, sendcount, sendtype, recvbuf, recvcount,
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

int MPIR_Iscatter(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                  void *recvbuf, int recvcount, MPI_Datatype recvtype,
                  int root, MPIR_Comm * comm_ptr, MPIR_Request ** request)
{
    int mpi_errno = MPI_SUCCESS;

    if ((MPIR_CVAR_DEVICE_COLLECTIVES == MPIR_CVAR_DEVICE_COLLECTIVES_all) ||
        ((MPIR_CVAR_DEVICE_COLLECTIVES == MPIR_CVAR_DEVICE_COLLECTIVES_percoll) &&
         MPIR_CVAR_ISCATTER_DEVICE_COLLECTIVE)) {
        mpi_errno =
            MPID_Iscatter(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, root,
                          comm_ptr, request);
    } else {
        mpi_errno = MPIR_Iscatter_impl(sendbuf, sendcount, sendtype, recvbuf, recvcount,
                                       recvtype, root, comm_ptr, request);
    }

    return mpi_errno;
}

#endif /* MPICH_MPI_FROM_PMPI */

/*@
MPI_Iscatter - Sends data from one process to all other processes in a
               communicator in a nonblocking way

Input Parameters:
+ sendbuf - address of send buffer (significant only at root) (choice)
. sendcount - number of elements sent to each process (significant only at root) (non-negative integer)
. sendtype - data type of send buffer elements (significant only at root) (handle)
. recvcount - number of elements in receive buffer (non-negative integer)
. recvtype - data type of receive buffer elements (handle)
. root - rank of sending process (integer)
- comm - communicator (handle)

Output Parameters:
+ recvbuf - starting address of the receive buffer (choice)
- request - communication request (handle)

.N ThreadSafe

.N Fortran

.N Errors
@*/
int MPI_Iscatter(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                 void *recvbuf, int recvcount, MPI_Datatype recvtype, int root,
                 MPI_Comm comm, MPI_Request * request)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Comm *comm_ptr = NULL;
    MPIR_Request *request_ptr = NULL;
    MPIR_FUNC_TERSE_STATE_DECL(MPID_STATE_MPI_ISCATTER);

    MPID_THREAD_CS_ENTER(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    MPIR_FUNC_TERSE_ENTER(MPID_STATE_MPI_ISCATTER);

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
            MPIR_Datatype *sendtype_ptr, *recvtype_ptr;
            MPIR_Comm_valid_ptr(comm_ptr, mpi_errno, FALSE);
            if (comm_ptr->comm_kind == MPIR_COMM_KIND__INTRACOMM) {
                MPIR_ERRTEST_INTRA_ROOT(comm_ptr, root, mpi_errno);

                if (comm_ptr->rank == root) {
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
                    MPIR_ERRTEST_SENDBUF_INPLACE(sendbuf, sendcount, mpi_errno);

                    /* catch common aliasing cases */
                    if (recvbuf != MPI_IN_PLACE && sendtype == recvtype && sendcount == recvcount &&
                        recvcount != 0) {
                        int sendtype_size;
                        MPIR_Datatype_get_size_macro(sendtype, sendtype_size);
                        MPIR_ERRTEST_ALIAS_COLL(recvbuf,
                                                (char *) sendbuf +
                                                comm_ptr->rank * sendcount * sendtype_size,
                                                mpi_errno);
                    }
                } else
                    MPIR_ERRTEST_RECVBUF_INPLACE(recvbuf, recvcount, mpi_errno);

                if (recvbuf != MPI_IN_PLACE) {
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
            }

            if (comm_ptr->comm_kind == MPIR_COMM_KIND__INTERCOMM) {
                MPIR_ERRTEST_INTER_ROOT(comm_ptr, root, mpi_errno);

                if (root == MPI_ROOT) {
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
                } else if (root != MPI_PROC_NULL) {
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
            }
        }
        MPID_END_ERROR_CHECKS;
    }
#endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */

    mpi_errno = MPIR_Iscatter(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, root,
                              comm_ptr, &request_ptr);
    MPIR_ERR_CHECK(mpi_errno);

    /* create a complete request, if needed */
    if (!request_ptr)
        request_ptr = MPIR_Request_create_complete(MPIR_REQUEST_KIND__COLL);
    /* return the handle of the request to the user */
    *request = request_ptr->handle;

    /* ... end of body of routine ... */

  fn_exit:
    MPIR_FUNC_TERSE_EXIT(MPID_STATE_MPI_ISCATTER);
    MPID_THREAD_CS_EXIT(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
#ifdef HAVE_ERROR_CHECKING
    {
        mpi_errno =
            MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, __func__, __LINE__, MPI_ERR_OTHER,
                                 "**mpi_iscatter", "**mpi_iscatter %p %d %D %p %d %D %d %C %p",
                                 sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, root,
                                 comm, request);
    }
#endif
    mpi_errno = MPIR_Err_return_comm(comm_ptr, __func__, mpi_errno);
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}
