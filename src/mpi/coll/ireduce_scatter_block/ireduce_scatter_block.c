/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2010 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/*
=== BEGIN_MPI_T_CVAR_INFO_BLOCK ===

cvars:
    - name        : MPIR_CVAR_IREDUCE_SCATTER_BLOCK_RECEXCH_KVAL
      category    : COLLECTIVE
      type        : int
      default     : 2
      class       : device
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        k value for recursive exchange based ireduce_scatter_block

    - name        : MPIR_CVAR_IREDUCE_SCATTER_BLOCK_INTRA_ALGORITHM
      category    : COLLECTIVE
      type        : string
      default     : auto
      class       : device
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : |-
        Variable to select ireduce_scatter_block algorithm
        auto               - Internal algorithm selection
        noncommutative     - Force noncommutative algorithm
        recursive_doubling - Force recursive doubling algorithm
        pairwise           - Force pairwise algorithm
        recursive_halving  - Force recursive halving algorithm
        recexch  - Force generic transport recursive exchange algorithm

    - name        : MPIR_CVAR_IREDUCE_SCATTER_BLOCK_INTER_ALGORITHM
      category    : COLLECTIVE
      type        : string
      default     : auto
      class       : device
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : |-
        Variable to select ireduce_scatter_block algorithm
        auto                         - Internal algorithm selection
        remote_reduce_local_scatterv - Force remote-reduce-local-scatterv algorithm

    - name        : MPIR_CVAR_IREDUCE_SCATTER_BLOCK_DEVICE_COLLECTIVE
      category    : COLLECTIVE
      type        : boolean
      default     : true
      class       : device
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        If set to true, MPI_Ireduce_scatter_block will allow the device to override the
        MPIR-level collective algorithms. The device still has the
        option to call the MPIR-level algorithms manually.
        If set to false, the device-level ireduce_scatter_block function will not be
        called.

=== END_MPI_T_CVAR_INFO_BLOCK ===
*/

/* -- Begin Profiling Symbol Block for routine MPI_Ireduce_scatter_block */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Ireduce_scatter_block = PMPI_Ireduce_scatter_block
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Ireduce_scatter_block  MPI_Ireduce_scatter_block
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Ireduce_scatter_block as PMPI_Ireduce_scatter_block
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPI_Ireduce_scatter_block(const void *sendbuf, void *recvbuf, int recvcount,
                              MPI_Datatype datatype, MPI_Op op, MPI_Comm comm,
                              MPI_Request * request)
    __attribute__ ((weak, alias("PMPI_Ireduce_scatter_block")));
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Ireduce_scatter_block
#define MPI_Ireduce_scatter_block PMPI_Ireduce_scatter_block

#undef FUNCNAME
#define FUNCNAME MPIR_Ireduce_scatter_block_sched_intra_auto
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Ireduce_scatter_block_sched_intra_auto(const void *sendbuf, void *recvbuf, int recvcount,
                                                MPI_Datatype datatype, MPI_Op op,
                                                MPIR_Comm * comm_ptr, MPIR_Sched_t s)
{
    int mpi_errno = MPI_SUCCESS;
    int is_commutative;
    int total_count, type_size, nbytes;
    int comm_size;

    is_commutative = MPIR_Op_is_commutative(op);

    comm_size = comm_ptr->local_size;
    total_count = recvcount * comm_size;
    if (total_count == 0) {
        goto fn_exit;
    }
    MPIR_Datatype_get_size_macro(datatype, type_size);
    nbytes = total_count * type_size;

    /* select an appropriate algorithm based on commutivity and message size */
    if (is_commutative && (nbytes < MPIR_CVAR_REDUCE_SCATTER_COMMUTATIVE_LONG_MSG_SIZE)) {
        mpi_errno =
            MPIR_Ireduce_scatter_block_sched_intra_recursive_halving(sendbuf, recvbuf, recvcount,
                                                                     datatype, op, comm_ptr, s);
        if (mpi_errno)
            MPIR_ERR_POP(mpi_errno);
    } else if (is_commutative && (nbytes >= MPIR_CVAR_REDUCE_SCATTER_COMMUTATIVE_LONG_MSG_SIZE)) {
        mpi_errno =
            MPIR_Ireduce_scatter_block_sched_intra_pairwise(sendbuf, recvbuf, recvcount, datatype,
                                                            op, comm_ptr, s);
        if (mpi_errno)
            MPIR_ERR_POP(mpi_errno);
    } else {    /* (!is_commutative) */

        if (MPL_is_pof2(comm_size, NULL)) {
            /* noncommutative, pof2 size */
            mpi_errno =
                MPIR_Ireduce_scatter_block_sched_intra_noncommutative(sendbuf, recvbuf, recvcount,
                                                                      datatype, op, comm_ptr, s);
            if (mpi_errno)
                MPIR_ERR_POP(mpi_errno);
        } else {
            /* noncommutative and non-pof2, use recursive doubling. */
            mpi_errno =
                MPIR_Ireduce_scatter_block_sched_intra_recursive_doubling(sendbuf, recvbuf,
                                                                          recvcount, datatype, op,
                                                                          comm_ptr, s);
            if (mpi_errno)
                MPIR_ERR_POP(mpi_errno);
        }
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}


#undef FUNCNAME
#define FUNCNAME MPIR_Ireduce_scatter_block_sched_inter_auto
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Ireduce_scatter_block_sched_inter_auto(const void *sendbuf, void *recvbuf, int recvcount,
                                                MPI_Datatype datatype, MPI_Op op,
                                                MPIR_Comm * comm_ptr, MPIR_Sched_t s)
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno =
        MPIR_Ireduce_scatter_block_sched_inter_remote_reduce_local_scatterv(sendbuf, recvbuf,
                                                                            recvcount, datatype, op,
                                                                            comm_ptr, s);

    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIR_Ireduce_scatter_block_sched_impl
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Ireduce_scatter_block_sched_impl(const void *sendbuf, void *recvbuf, int recvcount,
                                          MPI_Datatype datatype, MPI_Op op, MPIR_Comm * comm_ptr,
                                          MPIR_Sched_t s)
{
    int mpi_errno = MPI_SUCCESS;

    if (comm_ptr->comm_kind == MPIR_COMM_KIND__INTRACOMM) {
        /* intracommunicator */
        switch (MPIR_Ireduce_scatter_block_intra_algo_choice) {
            case MPIR_IREDUCE_SCATTER_BLOCK_INTRA_ALGO_NONCOMMUTATIVE:
                mpi_errno = MPIR_Ireduce_scatter_block_sched_intra_noncommutative(sendbuf, recvbuf,
                                                                                  recvcount,
                                                                                  datatype, op,
                                                                                  comm_ptr, s);
                break;
            case MPIR_IREDUCE_SCATTER_BLOCK_INTRA_ALGO_PAIRWISE:
                mpi_errno = MPIR_Ireduce_scatter_block_sched_intra_pairwise(sendbuf, recvbuf,
                                                                            recvcount, datatype, op,
                                                                            comm_ptr, s);
                break;
            case MPIR_IREDUCE_SCATTER_BLOCK_INTRA_ALGO_RECURSIVE_HALVING:
                mpi_errno =
                    MPIR_Ireduce_scatter_block_sched_intra_recursive_halving(sendbuf, recvbuf,
                                                                             recvcount, datatype,
                                                                             op, comm_ptr, s);
                break;
            case MPIR_IREDUCE_SCATTER_BLOCK_INTRA_ALGO_RECURSIVE_DOUBLING:
                mpi_errno =
                    MPIR_Ireduce_scatter_block_sched_intra_recursive_doubling(sendbuf, recvbuf,
                                                                              recvcount, datatype,
                                                                              op, comm_ptr, s);
                break;
            case MPIR_IREDUCE_SCATTER_BLOCK_INTRA_ALGO_AUTO:
                MPL_FALLTHROUGH;
            default:
                mpi_errno = MPIR_Ireduce_scatter_block_sched_intra_auto(sendbuf, recvbuf,
                                                                        recvcount, datatype, op,
                                                                        comm_ptr, s);
                break;
        }
    } else {
        /* intercommunicator */
        switch (MPIR_Ireduce_scatter_block_inter_algo_choice) {
            case MPIR_IREDUCE_SCATTER_BLOCK_INTER_ALGO_REMOTE_REDUCE_LOCAL_SCATTERV:
                mpi_errno =
                    MPIR_Ireduce_scatter_block_sched_inter_remote_reduce_local_scatterv(sendbuf,
                                                                                        recvbuf,
                                                                                        recvcount,
                                                                                        datatype,
                                                                                        op,
                                                                                        comm_ptr,
                                                                                        s);
                break;
            case MPIR_IREDUCE_SCATTER_BLOCK_INTER_ALGO_AUTO:
                MPL_FALLTHROUGH;
            default:
                mpi_errno = MPIR_Ireduce_scatter_block_sched_inter_auto(sendbuf, recvbuf,
                                                                        recvcount, datatype, op,
                                                                        comm_ptr, s);
        }
    }

    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIR_Ireduce_scatter_block_sched
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Ireduce_scatter_block_sched(const void *sendbuf, void *recvbuf, int recvcount,
                                     MPI_Datatype datatype, MPI_Op op, MPIR_Comm * comm_ptr,
                                     MPIR_Sched_t s)
{
    int mpi_errno = MPI_SUCCESS;

    if (MPIR_CVAR_IREDUCE_SCATTER_BLOCK_DEVICE_COLLECTIVE && MPIR_CVAR_DEVICE_COLLECTIVES) {
        mpi_errno = MPID_Ireduce_scatter_block_sched(sendbuf, recvbuf, recvcount, datatype, op,
                                                     comm_ptr, s);
    } else {
        mpi_errno = MPIR_Ireduce_scatter_block_sched_impl(sendbuf, recvbuf, recvcount, datatype, op,
                                                          comm_ptr, s);
    }

    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIR_Ireduce_scatter_block_impl
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Ireduce_scatter_block_impl(const void *sendbuf, void *recvbuf,
                                    int recvcount, MPI_Datatype datatype,
                                    MPI_Op op, MPIR_Comm * comm_ptr, MPIR_Request ** request)
{
    int mpi_errno = MPI_SUCCESS;
    int tag = -1;
    int is_commutative = MPIR_Op_is_commutative(op);
    MPIR_Sched_t s = MPIR_SCHED_NULL;

    *request = NULL;

    /* If the user picks one of the transport-enabled algorithms, branch there
     * before going down to the MPIR_Sched-based algorithms. */
    /* TODO - Eventually the intention is to replace all of the
     * MPIR_Sched-based algorithms with transport-enabled algorithms, but that
     * will require sufficient performance testing and replacement algorithms. */
    if (comm_ptr->comm_kind == MPIR_COMM_KIND__INTRACOMM) {
        /* intracommunicator */
        switch (MPIR_Ireduce_scatter_block_intra_algo_choice) {
            case MPIR_IREDUCE_SCATTER_BLOCK_INTRA_ALGO_GENTRAN_RECEXCH:
                if (is_commutative) {
                    mpi_errno =
                        MPIR_Ireduce_scatter_block_intra_recexch(sendbuf, recvbuf, recvcount,
                                                                 datatype, op, comm_ptr, request);
                    if (mpi_errno)
                        MPIR_ERR_POP(mpi_errno);
                    goto fn_exit;
                }
                break;
            default:
                /* go down to the MPIR_Sched-based algorithms */
                break;
        }
    }
    mpi_errno = MPIR_Sched_next_tag(comm_ptr, &tag);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);
    mpi_errno = MPIR_Sched_create(&s);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

    mpi_errno =
        MPIR_Ireduce_scatter_block_sched(sendbuf, recvbuf, recvcount, datatype, op, comm_ptr, s);
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
#define FUNCNAME MPIR_Ireduce_scatter_block
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Ireduce_scatter_block(const void *sendbuf, void *recvbuf,
                               int recvcount, MPI_Datatype datatype,
                               MPI_Op op, MPIR_Comm * comm_ptr, MPIR_Request ** request)
{
    int mpi_errno = MPI_SUCCESS;

    if (MPIR_CVAR_IREDUCE_SCATTER_BLOCK_DEVICE_COLLECTIVE && MPIR_CVAR_DEVICE_COLLECTIVES) {
        mpi_errno = MPID_Ireduce_scatter_block(sendbuf, recvbuf, recvcount, datatype, op, comm_ptr,
                                               request);
    } else {
        mpi_errno = MPIR_Ireduce_scatter_block_impl(sendbuf, recvbuf, recvcount, datatype, op,
                                                    comm_ptr, request);
    }

    return mpi_errno;
}

#endif /* MPICH_MPI_FROM_PMPI */

#undef FUNCNAME
#define FUNCNAME MPI_Ireduce_scatter_block
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
/*@
MPI_Ireduce_scatter_block - Combines values and scatters the results in
                            a nonblocking way

Input Parameters:
+ sendbuf - starting address of the send buffer (choice)
. recvcount - element count per block (non-negative integer)
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
int MPI_Ireduce_scatter_block(const void *sendbuf, void *recvbuf,
                              int recvcount,
                              MPI_Datatype datatype, MPI_Op op, MPI_Comm comm,
                              MPI_Request * request)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Comm *comm_ptr = NULL;
    MPIR_Request *request_ptr = NULL;
    MPIR_FUNC_TERSE_STATE_DECL(MPID_STATE_MPI_IREDUCE_SCATTER_BLOCK);

    MPID_THREAD_CS_ENTER(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    MPIR_FUNC_TERSE_ENTER(MPID_STATE_MPI_IREDUCE_SCATTER_BLOCK);

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
    MPIR_Assert(comm_ptr != NULL);

    /* Validate parameters and objects (post conversion) */
#ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            MPIR_Comm_valid_ptr(comm_ptr, mpi_errno, FALSE);
            if (HANDLE_GET_KIND(datatype) != HANDLE_KIND_BUILTIN) {
                MPIR_Datatype *datatype_ptr = NULL;
                MPIR_Datatype_get_ptr(datatype, datatype_ptr);
                MPIR_Datatype_valid_ptr(datatype_ptr, mpi_errno);
                if (mpi_errno != MPI_SUCCESS)
                    goto fn_fail;
                MPIR_Datatype_committed_ptr(datatype_ptr, mpi_errno);
                if (mpi_errno != MPI_SUCCESS)
                    goto fn_fail;
            }

            if (HANDLE_GET_KIND(op) != HANDLE_KIND_BUILTIN) {
                MPIR_Op *op_ptr = NULL;
                MPIR_Op_get_ptr(op, op_ptr);
                MPIR_Op_valid_ptr(op_ptr, mpi_errno);
            } else if (HANDLE_GET_KIND(op) == HANDLE_KIND_BUILTIN) {
                mpi_errno = (*MPIR_OP_HDL_TO_DTYPE_FN(op)) (datatype);
            }
            if (mpi_errno != MPI_SUCCESS)
                goto fn_fail;

            MPIR_ERRTEST_ARGNULL(request, "request", mpi_errno);

            if (comm_ptr->comm_kind == MPIR_COMM_KIND__INTRACOMM && sendbuf != MPI_IN_PLACE &&
                recvcount != 0) {
                MPIR_ERRTEST_ALIAS_COLL(sendbuf, recvbuf, mpi_errno)
            }
            /* TODO more checks may be appropriate (counts, in_place, etc) */
        }
        MPID_END_ERROR_CHECKS;
    }
#endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */

    mpi_errno = MPIR_Ireduce_scatter_block(sendbuf, recvbuf, recvcount, datatype, op, comm_ptr,
                                           &request_ptr);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

    /* create a complete request, if needed */
    if (!request_ptr)
        request_ptr = MPIR_Request_create_complete(MPIR_REQUEST_KIND__COLL);
    /* return the handle of the request to the user */
    *request = request_ptr->handle;

    /* ... end of body of routine ... */

  fn_exit:
    MPIR_FUNC_TERSE_EXIT(MPID_STATE_MPI_IREDUCE_SCATTER_BLOCK);
    MPID_THREAD_CS_EXIT(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
#ifdef HAVE_ERROR_CHECKING
    {
        mpi_errno =
            MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER,
                                 "**mpi_ireduce_scatter_block",
                                 "**mpi_ireduce_scatter_block %p %p %d %D %O %C %p", sendbuf,
                                 recvbuf, recvcount, datatype, op, comm, request);
    }
#endif
    mpi_errno = MPIR_Err_return_comm(comm_ptr, FCNAME, mpi_errno);
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}
