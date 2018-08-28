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
    - name        : MPIR_CVAR_REDUCE_SCATTER_COMMUTATIVE_LONG_MSG_SIZE
      category    : COLLECTIVE
      type        : int
      default     : 524288
      class       : device
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        the long message algorithm will be used if the operation is commutative
        and the send buffer size is >= this value (in bytes)

    - name        : MPIR_CVAR_REDUCE_SCATTER_INTRA_ALGORITHM
      category    : COLLECTIVE
      type        : string
      default     : auto
      class       : device
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : |-
        Variable to select reduce_scatter algorithm
        auto               - Internal algorithm selection
        nb                 - Force nonblocking algorithm
        noncommutative     - Force noncommutative algorithm
        pairwise           - Force pairwise algorithm
        recursive_doubling - Force recursive doubling algorithm
        recursive_halving  - Force recursive halving algorithm

    - name        : MPIR_CVAR_REDUCE_SCATTER_INTER_ALGORITHM
      category    : COLLECTIVE
      type        : string
      default     : auto
      class       : device
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : |-
        Variable to select reduce_scatter algorithm
        auto                        - Internal algorithm selection
        nb                          - Force nonblocking algorithm
        remote_reduce_local_scatter - Force remote-reduce-local-scatter algorithm

    - name        : MPIR_CVAR_REDUCE_SCATTER_DEVICE_COLLECTIVE
      category    : COLLECTIVE
      type        : boolean
      default     : true
      class       : device
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        If set to true, MPI_Redscat will allow the device to override the
        MPIR-level collective algorithms. The device still has the
        option to call the MPIR-level algorithms manually.
        If set to false, the device-level reduce_scatter function will not be
        called.

=== END_MPI_T_CVAR_INFO_BLOCK ===
*/

/* -- Begin Profiling Symbol Block for routine MPI_Reduce_scatter */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Reduce_scatter = PMPI_Reduce_scatter
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Reduce_scatter  MPI_Reduce_scatter
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Reduce_scatter as PMPI_Reduce_scatter
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPI_Reduce_scatter(const void *sendbuf, void *recvbuf, const int recvcounts[],
                       MPI_Datatype datatype, MPI_Op op, MPI_Comm comm)
    __attribute__ ((weak, alias("PMPI_Reduce_scatter")));
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Reduce_scatter
#define MPI_Reduce_scatter PMPI_Reduce_scatter

/* This is the machine-independent implementation of reduce_scatter. The algorithm is:

   Algorithm: MPI_Reduce_scatter

   If the operation is not commutative, we do the following:

   Possible improvements:

   End Algorithm: MPI_Reduce_scatter
*/

#undef FUNCNAME
#define FUNCNAME MPIR_Reduce_scatter_intra_auto
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Reduce_scatter_intra_auto(const void *sendbuf, void *recvbuf, const int recvcounts[],
                                   MPI_Datatype datatype, MPI_Op op, MPIR_Comm * comm_ptr,
                                   MPIR_Errflag_t * errflag)
{
    int comm_size, i;
    MPI_Aint true_extent, true_lb;
    int *disps;
    int mpi_errno = MPI_SUCCESS;
    int mpi_errno_ret = MPI_SUCCESS;
    int type_size, total_count, nbytes;
    int pof2;
    int is_commutative;
    MPIR_CHKLMEM_DECL(1);

    comm_size = comm_ptr->local_size;

    /* set op_errno to 0. stored in perthread structure */
    {
        MPIR_Per_thread_t *per_thread = NULL;
        int err = 0;

        MPID_THREADPRIV_KEY_GET_ADDR(MPIR_ThreadInfo.isThreaded, MPIR_Per_thread_key,
                                     MPIR_Per_thread, per_thread, &err);
        MPIR_Assert(err == 0);
        per_thread->op_errno = 0;
    }

    MPIR_Type_get_true_extent_impl(datatype, &true_lb, &true_extent);

    is_commutative = MPIR_Op_is_commutative(op);

    MPIR_CHKLMEM_MALLOC(disps, int *, comm_size * sizeof(int), mpi_errno, "disps", MPL_MEM_BUFFER);

    total_count = 0;
    for (i = 0; i < comm_size; i++) {
        disps[i] = total_count;
        total_count += recvcounts[i];
    }

    if (total_count == 0) {
        goto fn_exit;
    }

    MPIR_Datatype_get_size_macro(datatype, type_size);
    nbytes = total_count * type_size;

    if ((is_commutative) && (nbytes < MPIR_CVAR_REDUCE_SCATTER_COMMUTATIVE_LONG_MSG_SIZE)) {
        /* commutative and short. use recursive halving algorithm */

        mpi_errno =
            MPIR_Reduce_scatter_intra_recursive_halving(sendbuf, recvbuf, recvcounts, datatype, op,
                                                        comm_ptr, errflag);
        if (mpi_errno)
            MPIR_ERR_POP(mpi_errno);
    }

    if (is_commutative && (nbytes >= MPIR_CVAR_REDUCE_SCATTER_COMMUTATIVE_LONG_MSG_SIZE)) {

        /* commutative and long message, or noncommutative and long message.
         * use (p-1) pairwise exchanges */
        mpi_errno =
            MPIR_Reduce_scatter_intra_pairwise(sendbuf, recvbuf, recvcounts, datatype, op, comm_ptr,
                                               errflag);
        if (mpi_errno)
            MPIR_ERR_POP(mpi_errno);

    }

    if (!is_commutative) {
        int is_block_regular = 1;
        for (i = 0; i < (comm_size - 1); ++i) {
            if (recvcounts[i] != recvcounts[i + 1]) {
                is_block_regular = 0;
                break;
            }
        }

        /* power-of-two that is equal or greater than comm_size */
        pof2 = 1;
        while (pof2 < comm_size)
            pof2 <<= 1;

        if (pof2 == comm_size && is_block_regular) {
            /* noncommutative, pof2 size, and block regular */
            mpi_errno =
                MPIR_Reduce_scatter_intra_noncommutative(sendbuf, recvbuf, recvcounts, datatype, op,
                                                         comm_ptr, errflag);
        } else {
            /* noncommutative and (non-pof2 or block irregular), use recursive doubling. */
            mpi_errno =
                MPIR_Reduce_scatter_intra_recursive_doubling(sendbuf, recvbuf, recvcounts, datatype,
                                                             op, comm_ptr, errflag);
        }
        if (mpi_errno) {
            /* for communication errors, just record the error but continue */
            *errflag =
                MPIX_ERR_PROC_FAILED ==
                MPIR_ERR_GET_CLASS(mpi_errno) ? MPIR_ERR_PROC_FAILED : MPIR_ERR_OTHER;
            MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
            MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
        }
    }

  fn_exit:
    MPIR_CHKLMEM_FREEALL();

    {
        MPIR_Per_thread_t *per_thread = NULL;
        int err = 0;

        MPID_THREADPRIV_KEY_GET_ADDR(MPIR_ThreadInfo.isThreaded, MPIR_Per_thread_key,
                                     MPIR_Per_thread, per_thread, &err);
        MPIR_Assert(err == 0);
        if (per_thread->op_errno)
            mpi_errno = per_thread->op_errno;
    }

    if (mpi_errno_ret)
        mpi_errno = mpi_errno_ret;
    else if (*errflag != MPIR_ERR_NONE)
        MPIR_ERR_SET(mpi_errno, *errflag, "**coll_fail");
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIR_Reduce_scatter_inter_auto
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Reduce_scatter_inter_auto(const void *sendbuf, void *recvbuf, const int recvcounts[],
                                   MPI_Datatype datatype, MPI_Op op, MPIR_Comm * comm_ptr,
                                   MPIR_Errflag_t * errflag)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_Reduce_scatter_inter_remote_reduce_local_scatter(sendbuf, recvbuf, recvcounts, datatype,
                                                          op, comm_ptr, errflag);

    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIR_Reduce_scatter_impl
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Reduce_scatter_impl(const void *sendbuf, void *recvbuf,
                             const int recvcounts[], MPI_Datatype datatype,
                             MPI_Op op, MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag)
{
    int mpi_errno = MPI_SUCCESS;

    if (comm_ptr->comm_kind == MPIR_COMM_KIND__INTRACOMM) {
        /* intracommunicator */
        switch (MPIR_Reduce_scatter_intra_algo_choice) {
            case MPIR_REDUCE_SCATTER_INTRA_ALGO_NONCOMMUTATIVE:
                mpi_errno = MPIR_Reduce_scatter_intra_noncommutative(sendbuf, recvbuf,
                                                                     recvcounts, datatype, op,
                                                                     comm_ptr, errflag);
                break;
            case MPIR_REDUCE_SCATTER_INTRA_ALGO_PAIRWISE:
                mpi_errno = MPIR_Reduce_scatter_intra_pairwise(sendbuf, recvbuf,
                                                               recvcounts, datatype, op, comm_ptr,
                                                               errflag);
                break;
            case MPIR_REDUCE_SCATTER_INTRA_ALGO_RECURSIVE_HALVING:
                mpi_errno = MPIR_Reduce_scatter_intra_recursive_halving(sendbuf, recvbuf,
                                                                        recvcounts, datatype, op,
                                                                        comm_ptr, errflag);
                break;
            case MPIR_REDUCE_SCATTER_INTRA_ALGO_RECURSIVE_DOUBLING:
                mpi_errno = MPIR_Reduce_scatter_intra_recursive_doubling(sendbuf, recvbuf,
                                                                         recvcounts, datatype, op,
                                                                         comm_ptr, errflag);
                break;
            case MPIR_REDUCE_SCATTER_INTRA_ALGO_NB:
                mpi_errno = MPIR_Reduce_scatter_allcomm_nb(sendbuf, recvbuf,
                                                           recvcounts, datatype, op, comm_ptr,
                                                           errflag);
                break;
            case MPIR_REDUCE_SCATTER_INTRA_ALGO_AUTO:
                MPL_FALLTHROUGH;
            default:
                mpi_errno = MPIR_Reduce_scatter_intra_auto(sendbuf, recvbuf,
                                                           recvcounts, datatype, op, comm_ptr,
                                                           errflag);
                break;
        }
    } else {
        /* intercommunicator */
        switch (MPIR_Reduce_scatter_inter_algo_choice) {
            case MPIR_REDUCE_SCATTER_INTER_ALGO_REMOTE_REDUCE_LOCAL_SCATTER:
                mpi_errno =
                    MPIR_Reduce_scatter_inter_remote_reduce_local_scatter(sendbuf, recvbuf,
                                                                          recvcounts, datatype, op,
                                                                          comm_ptr, errflag);
                break;
            case MPIR_REDUCE_SCATTER_INTER_ALGO_NB:
                mpi_errno = MPIR_Reduce_scatter_allcomm_nb(sendbuf, recvbuf,
                                                           recvcounts, datatype, op, comm_ptr,
                                                           errflag);
                break;
            case MPIR_REDUCE_SCATTER_INTER_ALGO_AUTO:
                MPL_FALLTHROUGH;
            default:
                mpi_errno = MPIR_Reduce_scatter_inter_auto(sendbuf, recvbuf, recvcounts,
                                                           datatype, op, comm_ptr, errflag);
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
#define FUNCNAME MPIR_Reduce_scatter
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Reduce_scatter(const void *sendbuf, void *recvbuf,
                        const int recvcounts[], MPI_Datatype datatype,
                        MPI_Op op, MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag)
{
    int mpi_errno = MPI_SUCCESS;

    if (MPIR_CVAR_REDUCE_SCATTER_DEVICE_COLLECTIVE && MPIR_CVAR_DEVICE_COLLECTIVES) {
        mpi_errno = MPID_Reduce_scatter(sendbuf, recvbuf, recvcounts, datatype, op, comm_ptr,
                                        errflag);
    } else {
        mpi_errno = MPIR_Reduce_scatter_impl(sendbuf, recvbuf, recvcounts, datatype, op, comm_ptr,
                                             errflag);
    }

    return mpi_errno;
}

#endif

#undef FUNCNAME
#define FUNCNAME MPI_Reduce_scatter
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
/*@

MPI_Reduce_scatter - Combines values and scatters the results

Input Parameters:
+ sendbuf - starting address of send buffer (choice)
. recvcounts - integer array specifying the
number of elements in result distributed to each process.
Array must be identical on all calling processes.
. datatype - data type of elements of input buffer (handle)
. op - operation (handle)
- comm - communicator (handle)

Output Parameters:
. recvbuf - starting address of receive buffer (choice)

.N ThreadSafe

.N Fortran

.N collops

.N Errors
.N MPI_SUCCESS
.N MPI_ERR_COMM
.N MPI_ERR_COUNT
.N MPI_ERR_TYPE
.N MPI_ERR_BUFFER
.N MPI_ERR_OP
.N MPI_ERR_BUFFER_ALIAS
@*/
int MPI_Reduce_scatter(const void *sendbuf, void *recvbuf, const int recvcounts[],
                       MPI_Datatype datatype, MPI_Op op, MPI_Comm comm)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Comm *comm_ptr = NULL;
    MPIR_Errflag_t errflag = MPIR_ERR_NONE;
    MPIR_FUNC_TERSE_STATE_DECL(MPID_STATE_MPI_REDUCE_SCATTER);

    MPIR_ERRTEST_INITIALIZED_ORDIE();

    MPID_THREAD_CS_ENTER(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    MPIR_FUNC_TERSE_COLL_ENTER(MPID_STATE_MPI_REDUCE_SCATTER);

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
            MPIR_Datatype *datatype_ptr = NULL;
            MPIR_Op *op_ptr = NULL;
            int i, size, sum;

            MPIR_Comm_valid_ptr(comm_ptr, mpi_errno, FALSE);
            if (mpi_errno != MPI_SUCCESS)
                goto fn_fail;

            size = comm_ptr->local_size;
            /* even in intercomm. case, recvcounts is of size local_size */

            sum = 0;
            for (i = 0; i < size; i++) {
                MPIR_ERRTEST_COUNT(recvcounts[i], mpi_errno);
                sum += recvcounts[i];
            }

            MPIR_ERRTEST_DATATYPE(datatype, "datatype", mpi_errno);
            if (HANDLE_GET_KIND(datatype) != HANDLE_KIND_BUILTIN) {
                MPIR_Datatype_get_ptr(datatype, datatype_ptr);
                MPIR_Datatype_valid_ptr(datatype_ptr, mpi_errno);
                if (mpi_errno != MPI_SUCCESS)
                    goto fn_fail;
                MPIR_Datatype_committed_ptr(datatype_ptr, mpi_errno);
                if (mpi_errno != MPI_SUCCESS)
                    goto fn_fail;
            }

            MPIR_ERRTEST_RECVBUF_INPLACE(recvbuf, recvcounts[comm_ptr->rank], mpi_errno);
            if (comm_ptr->comm_kind == MPIR_COMM_KIND__INTERCOMM) {
                MPIR_ERRTEST_SENDBUF_INPLACE(sendbuf, sum, mpi_errno);
            } else if (sendbuf != MPI_IN_PLACE && sum != 0)
                MPIR_ERRTEST_ALIAS_COLL(sendbuf, recvbuf, mpi_errno);

            MPIR_ERRTEST_USERBUFFER(recvbuf, recvcounts[comm_ptr->rank], datatype, mpi_errno);
            MPIR_ERRTEST_USERBUFFER(sendbuf, sum, datatype, mpi_errno);
            MPIR_ERRTEST_OP(op, mpi_errno);

            if (HANDLE_GET_KIND(op) != HANDLE_KIND_BUILTIN) {
                MPIR_Op_get_ptr(op, op_ptr);
                MPIR_Op_valid_ptr(op_ptr, mpi_errno);
            }
            if (HANDLE_GET_KIND(op) == HANDLE_KIND_BUILTIN) {
                mpi_errno = (*MPIR_OP_HDL_TO_DTYPE_FN(op)) (datatype);
            }
            if (mpi_errno != MPI_SUCCESS)
                goto fn_fail;
        }
        MPID_END_ERROR_CHECKS;
    }
#endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */

    mpi_errno = MPIR_Reduce_scatter(sendbuf, recvbuf, recvcounts, datatype, op, comm_ptr, &errflag);
    if (mpi_errno)
        goto fn_fail;

    /* ... end of body of routine ... */

  fn_exit:
    MPIR_FUNC_TERSE_COLL_EXIT(MPID_STATE_MPI_REDUCE_SCATTER);
    MPID_THREAD_CS_EXIT(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
#ifdef HAVE_ERROR_CHECKING
    {
        mpi_errno =
            MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER,
                                 "**mpi_reduce_scatter", "**mpi_reduce_scatter %p %p %p %D %O %C",
                                 sendbuf, recvbuf, recvcounts, datatype, op, comm);
    }
#endif
    mpi_errno = MPIR_Err_return_comm(comm_ptr, FCNAME, mpi_errno);
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}
