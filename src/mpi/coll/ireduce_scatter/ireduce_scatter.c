/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2010 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/*
=== BEGIN_MPI_T_CVAR_INFO_BLOCK ===

cvars:
    - name        : MPIR_CVAR_IREDUCE_SCATTER_RECEXCH_KVAL
      category    : COLLECTIVE
      type        : int
      default     : 2
      class       : device
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        k value for recursive exchange based ireduce_scatter

    - name        : MPIR_CVAR_IREDUCE_SCATTER_INTRA_ALGORITHM
      category    : COLLECTIVE
      type        : enum
      default     : auto
      class       : device
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : |-
        Variable to select ireduce_scatter algorithm
        auto               - Internal algorithm selection
        noncommutative     - Force noncommutative algorithm
        recursive_doubling - Force recursive doubling algorithm
        pairwise           - Force pairwise algorithm
        recursive_halving  - Force recursive halving algorithm
        gentran_recexch    - Force generic transport recursive exchange algorithm

    - name        : MPIR_CVAR_IREDUCE_SCATTER_INTER_ALGORITHM
      category    : COLLECTIVE
      type        : enum
      default     : auto
      class       : device
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : |-
        Variable to select ireduce_scatter algorithm
        auto                         - Internal algorithm selection
        remote_reduce_local_scatterv - Force remote-reduce-local-scatterv algorithm

    - name        : MPIR_CVAR_IREDUCE_SCATTER_DEVICE_COLLECTIVE
      category    : COLLECTIVE
      type        : boolean
      default     : true
      class       : device
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        If set to true, MPI_Ireduce_scatter will allow the device to override the
        MPIR-level collective algorithms. The device still has the
        option to call the MPIR-level algorithms manually.
        If set to false, the device-level ireduce_scatter function will not be
        called.

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

int MPIR_Ireduce_scatter_sched_intra_auto(const void *sendbuf, void *recvbuf,
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
            MPIR_Ireduce_scatter_sched_intra_recursive_halving(sendbuf, recvbuf, recvcounts,
                                                               datatype, op, comm_ptr, s);
        MPIR_ERR_CHECK(mpi_errno);
    } else if (is_commutative && (nbytes >= MPIR_CVAR_REDUCE_SCATTER_COMMUTATIVE_LONG_MSG_SIZE)) {
        mpi_errno =
            MPIR_Ireduce_scatter_sched_intra_pairwise(sendbuf, recvbuf, recvcounts, datatype, op,
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
                MPIR_Ireduce_scatter_sched_intra_noncommutative(sendbuf, recvbuf, recvcounts,
                                                                datatype, op, comm_ptr, s);
            MPIR_ERR_CHECK(mpi_errno);
        } else {
            /* noncommutative and (non-pof2 or block irregular), use recursive doubling. */
            mpi_errno =
                MPIR_Ireduce_scatter_sched_intra_recursive_doubling(sendbuf, recvbuf, recvcounts,
                                                                    datatype, op, comm_ptr, s);
            MPIR_ERR_CHECK(mpi_errno);
        }
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIR_Ireduce_scatter_sched_inter_auto(const void *sendbuf, void *recvbuf,
                                          const int recvcounts[], MPI_Datatype datatype, MPI_Op op,
                                          MPIR_Comm * comm_ptr, MPIR_Sched_t s)
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno =
        MPIR_Ireduce_scatter_sched_inter_remote_reduce_local_scatterv(sendbuf, recvbuf, recvcounts,
                                                                      datatype, op, comm_ptr, s);

    return mpi_errno;
}

int MPIR_Ireduce_scatter_sched_impl(const void *sendbuf, void *recvbuf, const int recvcounts[],
                                    MPI_Datatype datatype, MPI_Op op, MPIR_Comm * comm_ptr,
                                    MPIR_Sched_t s)
{
    int mpi_errno = MPI_SUCCESS;

    if (comm_ptr->comm_kind == MPIR_COMM_KIND__INTRACOMM) {
        /* intracommunicator */
        switch (MPIR_CVAR_IREDUCE_SCATTER_INTRA_ALGORITHM) {
            case MPIR_CVAR_IREDUCE_SCATTER_INTRA_ALGORITHM_noncommutative:
                mpi_errno = MPIR_Ireduce_scatter_sched_intra_noncommutative(sendbuf, recvbuf,
                                                                            recvcounts, datatype,
                                                                            op, comm_ptr, s);
                break;
            case MPIR_CVAR_IREDUCE_SCATTER_INTRA_ALGORITHM_pairwise:
                mpi_errno = MPIR_Ireduce_scatter_sched_intra_pairwise(sendbuf, recvbuf,
                                                                      recvcounts, datatype, op,
                                                                      comm_ptr, s);
                break;
            case MPIR_CVAR_IREDUCE_SCATTER_INTRA_ALGORITHM_recursive_halving:
                mpi_errno = MPIR_Ireduce_scatter_sched_intra_recursive_halving(sendbuf, recvbuf,
                                                                               recvcounts, datatype,
                                                                               op, comm_ptr, s);
                break;
            case MPIR_CVAR_IREDUCE_SCATTER_INTRA_ALGORITHM_recursive_doubling:
                mpi_errno = MPIR_Ireduce_scatter_sched_intra_recursive_doubling(sendbuf, recvbuf,
                                                                                recvcounts,
                                                                                datatype, op,
                                                                                comm_ptr, s);
                break;
            case MPIR_CVAR_IREDUCE_SCATTER_INTRA_ALGORITHM_auto:
                MPL_FALLTHROUGH;
            default:
                mpi_errno = MPIR_Ireduce_scatter_sched_intra_auto(sendbuf, recvbuf,
                                                                  recvcounts, datatype, op,
                                                                  comm_ptr, s);
                break;
        }
    } else {
        /* intercommunicator */
        switch (MPIR_CVAR_IREDUCE_SCATTER_INTER_ALGORITHM) {
            case MPIR_CVAR_IREDUCE_SCATTER_INTER_ALGORITHM_remote_reduce_local_scatterv:
                mpi_errno =
                    MPIR_Ireduce_scatter_sched_inter_remote_reduce_local_scatterv(sendbuf, recvbuf,
                                                                                  recvcounts,
                                                                                  datatype, op,
                                                                                  comm_ptr, s);
                break;
            case MPIR_CVAR_IREDUCE_SCATTER_INTER_ALGORITHM_auto:
                MPL_FALLTHROUGH;
            default:
                mpi_errno = MPIR_Ireduce_scatter_sched_inter_auto(sendbuf, recvbuf, recvcounts,
                                                                  datatype, op, comm_ptr, s);
                break;
        }
    }

    return mpi_errno;
}

int MPIR_Ireduce_scatter_sched(const void *sendbuf, void *recvbuf, const int recvcounts[],
                               MPI_Datatype datatype, MPI_Op op, MPIR_Comm * comm_ptr,
                               MPIR_Sched_t s)
{
    int mpi_errno = MPI_SUCCESS;

    if (MPIR_CVAR_IREDUCE_SCATTER_DEVICE_COLLECTIVE && MPIR_CVAR_DEVICE_COLLECTIVES) {
        mpi_errno = MPID_Ireduce_scatter_sched(sendbuf, recvbuf, recvcounts, datatype, op, comm_ptr,
                                               s);
    } else {
        mpi_errno = MPIR_Ireduce_scatter_sched_impl(sendbuf, recvbuf, recvcounts, datatype, op,
                                                    comm_ptr, s);
    }

    return mpi_errno;
}

int MPIR_Ireduce_scatter_impl(const void *sendbuf, void *recvbuf, const int recvcounts[],
                              MPI_Datatype datatype, MPI_Op op, MPIR_Comm * comm_ptr,
                              MPIR_Request ** request)
{
    int mpi_errno = MPI_SUCCESS;
    int tag = -1;
    MPIR_Sched_t s = MPIR_SCHED_NULL;
    int is_commutative = MPIR_Op_is_commutative(op);

    *request = NULL;

    /* If the user picks one of the transport-enabled algorithms, branch there
     * before going down to the MPIR_Sched-based algorithms. */
    /* TODO - Eventually the intention is to replace all of the
     * MPIR_Sched-based algorithms with transport-enabled algorithms, but that
     * will require sufficient performance testing and replacement algorithms. */
    if (comm_ptr->comm_kind == MPIR_COMM_KIND__INTRACOMM) {
        /* intracommunicator */
        switch (MPIR_CVAR_IREDUCE_SCATTER_INTRA_ALGORITHM) {
            case MPIR_CVAR_IREDUCE_SCATTER_INTRA_ALGORITHM_gentran_recexch:
                if (is_commutative) {
                    mpi_errno =
                        MPIR_Ireduce_scatter_intra_gentran_recexch(sendbuf, recvbuf, recvcounts,
                                                                   datatype, op, comm_ptr, request);
                    MPIR_ERR_CHECK(mpi_errno);
                    goto fn_exit;
                }
                break;
            default:
                /* go down to the MPIR_Sched-based algorithms */
                break;
        }
    }

    mpi_errno = MPIR_Sched_next_tag(comm_ptr, &tag);
    MPIR_ERR_CHECK(mpi_errno);
    mpi_errno = MPIR_Sched_create(&s);
    MPIR_ERR_CHECK(mpi_errno);

    mpi_errno = MPIR_Ireduce_scatter_sched(sendbuf, recvbuf, recvcounts, datatype, op, comm_ptr, s);
    MPIR_ERR_CHECK(mpi_errno);

    mpi_errno = MPIR_Sched_start(&s, comm_ptr, tag, request);
    MPIR_ERR_CHECK(mpi_errno);

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

    if (MPIR_CVAR_IREDUCE_SCATTER_DEVICE_COLLECTIVE && MPIR_CVAR_DEVICE_COLLECTIVES) {
        mpi_errno = MPID_Ireduce_scatter(sendbuf, recvbuf, recvcounts, datatype, op, comm_ptr,
                                         request);
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


    MPID_THREAD_CS_ENTER(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    MPID_THREAD_CS_ENTER(VCI, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);


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

    MPID_THREAD_CS_EXIT(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    MPID_THREAD_CS_EXIT(VCI, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
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
