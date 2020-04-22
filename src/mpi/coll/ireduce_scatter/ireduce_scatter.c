/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"

/*
=== BEGIN_MPI_T_CVAR_INFO_BLOCK ===

cvars:
    - name        : MPIR_CVAR_IREDUCE_SCATTER_RECEXCH_KVAL
      category    : COLLECTIVE
      type        : int
      default     : 2
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        k value for recursive exchange based ireduce_scatter

    - name        : MPIR_CVAR_IREDUCE_SCATTER_INTRA_ALGORITHM
      category    : COLLECTIVE
      type        : enum
      default     : auto
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : |-
        Variable to select ireduce_scatter algorithm
        auto - Internal algorithm selection (can be overridden with MPIR_CVAR_COLL_SELECTION_TUNING_JSON_FILE)
        sched_auto - Internal algorithm selection for sched-based algorithms
        sched_noncommutative     - Force noncommutative algorithm
        sched_recursive_doubling - Force recursive doubling algorithm
        sched_pairwise           - Force pairwise algorithm
        sched_recursive_halving  - Force recursive halving algorithm
        gentran_recexch          - Force generic transport recursive exchange algorithm

    - name        : MPIR_CVAR_IREDUCE_SCATTER_INTER_ALGORITHM
      category    : COLLECTIVE
      type        : enum
      default     : auto
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : |-
        Variable to select ireduce_scatter algorithm
        auto - Internal algorithm selection (can be overridden with MPIR_CVAR_COLL_SELECTION_TUNING_JSON_FILE)
        sched_auto - Internal algorithm selection for sched-based algorithms
        sched_remote_reduce_local_scatterv - Force remote-reduce-local-scatterv algorithm

    - name        : MPIR_CVAR_IREDUCE_SCATTER_DEVICE_COLLECTIVE
      category    : COLLECTIVE
      type        : boolean
      default     : true
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        This CVAR is only used when MPIR_CVAR_DEVICE_COLLECTIVES
        is set to "percoll".  If set to true, MPI_Ireduce_scatter will
        allow the device to override the MPIR-level collective
        algorithms.  The device might still call the MPIR-level
        algorithms manually.  If set to false, the device-override
        will be disabled.

=== END_MPI_T_CVAR_INFO_BLOCK ===
*/

/* -- Begin Profiling Symbol Block for routine MPI_Ireduce_scatter */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Ireduce_scatter = PMPI_Ireduce_scatter
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Ireduce_scatter  MPI_Ireduce_scatter
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Ireduce_scatter as PMPI_Ireduce_scatter
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPI_Ireduce_scatter(const void *sendbuf, void *recvbuf, const int recvcounts[],
                        MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, MPI_Request * request)
    __attribute__ ((weak, alias("PMPI_Ireduce_scatter")));
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Ireduce_scatter
#define MPI_Ireduce_scatter PMPI_Ireduce_scatter


int MPIR_Ireduce_scatter_allcomm_auto(const void *sendbuf, void *recvbuf, const int *recvcounts,
                                      MPI_Datatype datatype, MPI_Op op, MPIR_Comm * comm_ptr,
                                      MPIR_Request ** request)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_Csel_coll_sig_s coll_sig = {
        .coll_type = MPIR_CSEL_COLL_TYPE__IREDUCE_SCATTER,
        .comm_ptr = comm_ptr,

        .u.ireduce_scatter.sendbuf = sendbuf,
        .u.ireduce_scatter.recvbuf = recvbuf,
        .u.ireduce_scatter.recvcounts = recvcounts,
        .u.ireduce_scatter.datatype = datatype,
        .u.ireduce_scatter.op = op,
    };

    MPII_Csel_container_s *cnt = MPIR_Csel_search(comm_ptr->csel_comm, coll_sig);
    MPIR_Assert(cnt);

    switch (cnt->id) {
        case MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Ireduce_scatter_intra_sched_auto:
            MPII_SCHED_WRAPPER(MPIR_Ireduce_scatter_intra_sched_auto, comm_ptr, request, sendbuf,
                               recvbuf, recvcounts, datatype, op);
            break;

        case MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Ireduce_scatter_intra_sched_noncommutative:
            MPII_SCHED_WRAPPER(MPIR_Ireduce_scatter_intra_sched_noncommutative, comm_ptr, request,
                               sendbuf, recvbuf, recvcounts, datatype, op);
            break;

        case MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Ireduce_scatter_intra_sched_pairwise:
            MPII_SCHED_WRAPPER(MPIR_Ireduce_scatter_intra_sched_pairwise, comm_ptr, request,
                               sendbuf, recvbuf, recvcounts, datatype, op);
            break;

        case MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Ireduce_scatter_intra_sched_recursive_doubling:
            MPII_SCHED_WRAPPER(MPIR_Ireduce_scatter_intra_sched_recursive_doubling, comm_ptr,
                               request, sendbuf, recvbuf, recvcounts, datatype, op);
            break;

        case MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Ireduce_scatter_intra_sched_recursive_halving:
            MPII_SCHED_WRAPPER(MPIR_Ireduce_scatter_intra_sched_recursive_halving, comm_ptr,
                               request, sendbuf, recvbuf, recvcounts, datatype, op);
            break;

        case MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Ireduce_scatter_intra_gentran_recexch:
            mpi_errno =
                MPIR_Ireduce_scatter_intra_gentran_recexch(sendbuf, recvbuf, recvcounts, datatype,
                                                           op, comm_ptr,
                                                           cnt->u.
                                                           ireduce_scatter.intra_gentran_recexch.k,
                                                           request);
            break;

        case MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Ireduce_scatter_inter_sched_auto:
            MPII_SCHED_WRAPPER(MPIR_Ireduce_scatter_inter_sched_auto, comm_ptr, request, sendbuf,
                               recvbuf, recvcounts, datatype, op);
            break;

        case MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Ireduce_scatter_inter_sched_remote_reduce_local_scatterv:
            MPII_SCHED_WRAPPER(MPIR_Ireduce_scatter_inter_sched_remote_reduce_local_scatterv,
                               comm_ptr, request, sendbuf, recvbuf, recvcounts, datatype, op);
            break;

        default:
            MPIR_Assert(0);
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIR_Ireduce_scatter_intra_sched_auto(const void *sendbuf, void *recvbuf,
                                          const int recvcounts[], MPI_Datatype datatype, MPI_Op op,
                                          MPIR_Comm * comm_ptr, MPIR_Sched_t s)
{
    int mpi_errno = MPI_SUCCESS;
    int i;
    int is_commutative;
    int total_count, type_size, nbytes;
    int comm_size;

    is_commutative = MPIR_Op_is_commutative(op);

    comm_size = comm_ptr->local_size;
    total_count = 0;
    for (i = 0; i < comm_size; i++) {
        total_count += recvcounts[i];
    }
    if (total_count == 0) {
        goto fn_exit;
    }
    MPIR_Datatype_get_size_macro(datatype, type_size);
    nbytes = total_count * type_size;

    /* select an appropriate algorithm based on commutivity and message size */
    if (is_commutative && (nbytes < MPIR_CVAR_REDUCE_SCATTER_COMMUTATIVE_LONG_MSG_SIZE)) {
        mpi_errno =
            MPIR_Ireduce_scatter_intra_sched_recursive_halving(sendbuf, recvbuf, recvcounts,
                                                               datatype, op, comm_ptr, s);
        MPIR_ERR_CHECK(mpi_errno);
    } else if (is_commutative && (nbytes >= MPIR_CVAR_REDUCE_SCATTER_COMMUTATIVE_LONG_MSG_SIZE)) {
        mpi_errno =
            MPIR_Ireduce_scatter_intra_sched_pairwise(sendbuf, recvbuf, recvcounts, datatype, op,
                                                      comm_ptr, s);
        MPIR_ERR_CHECK(mpi_errno);
    } else {    /* (!is_commutative) */

        int is_block_regular = TRUE;
        for (i = 0; i < (comm_size - 1); ++i) {
            if (recvcounts[i] != recvcounts[i + 1]) {
                is_block_regular = FALSE;
                break;
            }
        }

        if (MPL_is_pof2(comm_size, NULL) && is_block_regular) {
            /* noncommutative, pof2 size, and block regular */
            mpi_errno =
                MPIR_Ireduce_scatter_intra_sched_noncommutative(sendbuf, recvbuf, recvcounts,
                                                                datatype, op, comm_ptr, s);
            MPIR_ERR_CHECK(mpi_errno);
        } else {
            /* noncommutative and (non-pof2 or block irregular), use recursive doubling. */
            mpi_errno =
                MPIR_Ireduce_scatter_intra_sched_recursive_doubling(sendbuf, recvbuf, recvcounts,
                                                                    datatype, op, comm_ptr, s);
            MPIR_ERR_CHECK(mpi_errno);
        }
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIR_Ireduce_scatter_inter_sched_auto(const void *sendbuf, void *recvbuf,
                                          const int recvcounts[], MPI_Datatype datatype, MPI_Op op,
                                          MPIR_Comm * comm_ptr, MPIR_Sched_t s)
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno =
        MPIR_Ireduce_scatter_inter_sched_remote_reduce_local_scatterv(sendbuf, recvbuf, recvcounts,
                                                                      datatype, op, comm_ptr, s);

    return mpi_errno;
}

int MPIR_Ireduce_scatter_sched_auto(const void *sendbuf, void *recvbuf, const int recvcounts[],
                                    MPI_Datatype datatype, MPI_Op op, MPIR_Comm * comm_ptr,
                                    MPIR_Sched_t s)
{
    int mpi_errno = MPI_SUCCESS;

    if (comm_ptr->comm_kind == MPIR_COMM_KIND__INTRACOMM) {
        mpi_errno = MPIR_Ireduce_scatter_intra_sched_auto(sendbuf, recvbuf,
                                                          recvcounts, datatype, op, comm_ptr, s);
    } else {
        mpi_errno = MPIR_Ireduce_scatter_inter_sched_auto(sendbuf, recvbuf, recvcounts,
                                                          datatype, op, comm_ptr, s);
    }

    return mpi_errno;
}

int MPIR_Ireduce_scatter_impl(const void *sendbuf, void *recvbuf, const int recvcounts[],
                              MPI_Datatype datatype, MPI_Op op, MPIR_Comm * comm_ptr,
                              MPIR_Request ** request)
{
    int mpi_errno = MPI_SUCCESS;
    int is_commutative = MPIR_Op_is_commutative(op);

    *request = NULL;

    /* If the user picks one of the transport-enabled algorithms, branch there
     * before going down to the MPIR_Sched-based algorithms. */
    /* TODO - Eventually the intention is to replace all of the
     * MPIR_Sched-based algorithms with transport-enabled algorithms, but that
     * will require sufficient performance testing and replacement algorithms. */
    if (comm_ptr->comm_kind == MPIR_COMM_KIND__INTRACOMM) {
        switch (MPIR_CVAR_IREDUCE_SCATTER_INTRA_ALGORITHM) {
            case MPIR_CVAR_IREDUCE_SCATTER_INTRA_ALGORITHM_gentran_recexch:
                MPII_COLLECTIVE_FALLBACK_CHECK(comm_ptr->rank, is_commutative, mpi_errno,
                                               "Ireduce_scatter gentran_recexch cannot be applied.\n");
                mpi_errno =
                    MPIR_Ireduce_scatter_intra_gentran_recexch(sendbuf, recvbuf, recvcounts,
                                                               datatype, op, comm_ptr,
                                                               MPIR_CVAR_IREDUCE_SCATTER_RECEXCH_KVAL,
                                                               request);
                break;

            case MPIR_CVAR_IREDUCE_SCATTER_INTRA_ALGORITHM_sched_noncommutative:
                MPII_SCHED_WRAPPER(MPIR_Ireduce_scatter_intra_sched_noncommutative, comm_ptr,
                                   request, sendbuf, recvbuf, recvcounts, datatype, op);
                break;

            case MPIR_CVAR_IREDUCE_SCATTER_INTRA_ALGORITHM_sched_pairwise:
                MPII_SCHED_WRAPPER(MPIR_Ireduce_scatter_intra_sched_pairwise, comm_ptr, request,
                                   sendbuf, recvbuf, recvcounts, datatype, op);
                break;

            case MPIR_CVAR_IREDUCE_SCATTER_INTRA_ALGORITHM_sched_recursive_halving:
                MPII_SCHED_WRAPPER(MPIR_Ireduce_scatter_intra_sched_recursive_halving, comm_ptr,
                                   request, sendbuf, recvbuf, recvcounts, datatype, op);
                break;

            case MPIR_CVAR_IREDUCE_SCATTER_INTRA_ALGORITHM_sched_recursive_doubling:
                MPII_SCHED_WRAPPER(MPIR_Ireduce_scatter_intra_sched_recursive_doubling, comm_ptr,
                                   request, sendbuf, recvbuf, recvcounts, datatype, op);
                break;

            case MPIR_CVAR_IREDUCE_SCATTER_INTRA_ALGORITHM_sched_auto:
                MPII_SCHED_WRAPPER(MPIR_Ireduce_scatter_intra_sched_auto, comm_ptr, request,
                                   sendbuf, recvbuf, recvcounts, datatype, op);
                break;

            case MPIR_CVAR_IREDUCE_SCATTER_INTRA_ALGORITHM_auto:
                mpi_errno =
                    MPIR_Ireduce_scatter_allcomm_auto(sendbuf, recvbuf, recvcounts, datatype, op,
                                                      comm_ptr, request);
                break;

            default:
                MPIR_Assert(0);
        }
    } else {
        switch (MPIR_CVAR_IREDUCE_SCATTER_INTER_ALGORITHM) {
            case MPIR_CVAR_IREDUCE_SCATTER_INTER_ALGORITHM_sched_remote_reduce_local_scatterv:
                MPII_SCHED_WRAPPER(MPIR_Ireduce_scatter_inter_sched_remote_reduce_local_scatterv,
                                   comm_ptr, request, sendbuf, recvbuf, recvcounts, datatype, op);
                break;

            case MPIR_CVAR_IREDUCE_SCATTER_INTER_ALGORITHM_sched_auto:
                MPII_SCHED_WRAPPER(MPIR_Ireduce_scatter_inter_sched_auto, comm_ptr, request,
                                   sendbuf, recvbuf, recvcounts, datatype, op);
                break;

            case MPIR_CVAR_IREDUCE_SCATTER_INTER_ALGORITHM_auto:
                mpi_errno =
                    MPIR_Ireduce_scatter_allcomm_auto(sendbuf, recvbuf, recvcounts, datatype, op,
                                                      comm_ptr, request);
                break;

            default:
                MPIR_Assert(0);
        }
    }

    MPIR_ERR_CHECK(mpi_errno);
    goto fn_exit;

  fallback:
    if (comm_ptr->comm_kind == MPIR_COMM_KIND__INTRACOMM) {
        MPII_SCHED_WRAPPER(MPIR_Ireduce_scatter_intra_sched_auto, comm_ptr, request,
                           sendbuf, recvbuf, recvcounts, datatype, op);
    } else {
        MPII_SCHED_WRAPPER(MPIR_Ireduce_scatter_inter_sched_auto, comm_ptr, request,
                           sendbuf, recvbuf, recvcounts, datatype, op);
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIR_Ireduce_scatter(const void *sendbuf, void *recvbuf, const int recvcounts[],
                         MPI_Datatype datatype, MPI_Op op, MPIR_Comm * comm_ptr,
                         MPIR_Request ** request)
{
    int mpi_errno = MPI_SUCCESS;

    if ((MPIR_CVAR_DEVICE_COLLECTIVES == MPIR_CVAR_DEVICE_COLLECTIVES_all) ||
        ((MPIR_CVAR_DEVICE_COLLECTIVES == MPIR_CVAR_DEVICE_COLLECTIVES_percoll) &&
         MPIR_CVAR_IREDUCE_SCATTER_DEVICE_COLLECTIVE)) {
        mpi_errno =
            MPID_Ireduce_scatter(sendbuf, recvbuf, recvcounts, datatype, op, comm_ptr, request);
    } else {
        mpi_errno = MPIR_Ireduce_scatter_impl(sendbuf, recvbuf, recvcounts, datatype, op, comm_ptr,
                                              request);
    }

    return mpi_errno;
}

#endif /* MPICH_MPI_FROM_PMPI */

/*@
MPI_Ireduce_scatter - Combines values and scatters the results in
                      a nonblocking way

Input Parameters:
+ sendbuf - starting address of the send buffer (choice)
. recvcounts - non-negative integer array specifying the number of elements in result distributed to each process. Array must be identical on all calling processes.
. datatype - data type of elements of input buffer (handle)
. op - operation (handle)
- comm - communicator (handle)

Output Parameters:
+ recvbuf - starting address of the receive buffer (choice)
- request - communication request (handle)

.N ThreadSafe

.N Fortran

.N Errors
@*/
int MPI_Ireduce_scatter(const void *sendbuf, void *recvbuf, const int recvcounts[],
                        MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, MPI_Request * request)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Comm *comm_ptr = NULL;
    MPIR_Request *request_ptr = NULL;
    MPIR_FUNC_TERSE_STATE_DECL(MPID_STATE_MPI_IREDUCE_SCATTER);

    MPID_THREAD_CS_ENTER(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    MPIR_FUNC_TERSE_ENTER(MPID_STATE_MPI_IREDUCE_SCATTER);

    /* Validate parameters, especially handles needing to be converted */
#ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            MPIR_ERRTEST_DATATYPE(datatype, "datatype", mpi_errno);
            MPIR_ERRTEST_OP(op, mpi_errno);
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
            int i = 0;

            MPIR_Comm_valid_ptr(comm_ptr, mpi_errno, FALSE);
            if (mpi_errno != MPI_SUCCESS)
                goto fn_fail;

            MPIR_ERRTEST_ARGNULL(recvcounts, "recvcounts", mpi_errno);
            if (!HANDLE_IS_BUILTIN(datatype)) {
                MPIR_Datatype *datatype_ptr = NULL;
                MPIR_Datatype_get_ptr(datatype, datatype_ptr);
                MPIR_Datatype_valid_ptr(datatype_ptr, mpi_errno);
                if (mpi_errno != MPI_SUCCESS)
                    goto fn_fail;
                MPIR_Datatype_committed_ptr(datatype_ptr, mpi_errno);
                if (mpi_errno != MPI_SUCCESS)
                    goto fn_fail;
            }

            if (!HANDLE_IS_BUILTIN(op)) {
                MPIR_Op *op_ptr = NULL;
                MPIR_Op_get_ptr(op, op_ptr);
                MPIR_Op_valid_ptr(op_ptr, mpi_errno);
            } else {
                mpi_errno = (*MPIR_OP_HDL_TO_DTYPE_FN(op)) (datatype);
            }
            if (mpi_errno != MPI_SUCCESS)
                goto fn_fail;

            MPIR_ERRTEST_ARGNULL(request, "request", mpi_errno);

            while (i < comm_ptr->remote_size && recvcounts[i] == 0)
                ++i;

            if (comm_ptr->comm_kind == MPIR_COMM_KIND__INTRACOMM && sendbuf != MPI_IN_PLACE &&
                i < comm_ptr->remote_size) {
                MPIR_ERRTEST_ALIAS_COLL(sendbuf, recvbuf, mpi_errno)
            }
            /* TODO more checks may be appropriate (counts, in_place, etc) */
        }
        MPID_END_ERROR_CHECKS;
    }
#endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */

    mpi_errno = MPIR_Ireduce_scatter(sendbuf, recvbuf, recvcounts, datatype,
                                     op, comm_ptr, &request_ptr);
    MPIR_ERR_CHECK(mpi_errno);

    /* create a complete request, if needed */
    if (!request_ptr)
        request_ptr = MPIR_Request_create_complete(MPIR_REQUEST_KIND__COLL);
    /* return the handle of the request to the user */
    *request = request_ptr->handle;

    /* ... end of body of routine ... */

  fn_exit:
    MPIR_FUNC_TERSE_EXIT(MPID_STATE_MPI_IREDUCE_SCATTER);
    MPID_THREAD_CS_EXIT(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
#ifdef HAVE_ERROR_CHECKING
    {
        mpi_errno =
            MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, __func__, __LINE__, MPI_ERR_OTHER,
                                 "**mpi_ireduce_scatter",
                                 "**mpi_ireduce_scatter %p %p %p %D %O %C %p", sendbuf, recvbuf,
                                 recvcounts, datatype, op, comm, request);
    }
#endif
    mpi_errno = MPIR_Err_return_comm(comm_ptr, __func__, mpi_errno);
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}
