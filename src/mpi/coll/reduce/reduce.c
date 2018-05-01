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
    - name        : MPIR_CVAR_REDUCE_SHORT_MSG_SIZE
      category    : COLLECTIVE
      type        : int
      default     : 2048
      class       : device
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        the short message algorithm will be used if the send buffer size is <=
        this value (in bytes)

    - name        : MPIR_CVAR_ENABLE_SMP_REDUCE
      category    : COLLECTIVE
      type        : boolean
      default     : true
      class       : device
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        Enable SMP aware reduce.

    - name        : MPIR_CVAR_MAX_SMP_REDUCE_MSG_SIZE
      category    : COLLECTIVE
      type        : int
      default     : 0
      class       : device
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        Maximum message size for which SMP-aware reduce is used.  A
        value of '0' uses SMP-aware reduce for all message sizes.

    - name        : MPIR_CVAR_REDUCE_INTRA_ALGORITHM
      category    : COLLECTIVE
      type        : string
      default     : auto
      class       : device
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : |-
        Variable to select reduce algorithm
        auto                  - Internal algorithm selection
        binomial              - Force binomial algorithm
        nb                    - Force nonblocking algorithm
        reduce_scatter_gather - Force reduce scatter gather algorithm

    - name        : MPIR_CVAR_REDUCE_INTER_ALGORITHM
      category    : COLLECTIVE
      type        : string
      default     : auto
      class       : device
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : |-
        Variable to select reduce algorithm
        auto                     - Internal algorithm selection
        local_reduce_remote_send - Force local-reduce-remote-send algorithm
        nb                       - Force nonblocking algorithm

    - name        : MPIR_CVAR_REDUCE_DEVICE_COLLECTIVE
      category    : COLLECTIVE
      type        : boolean
      default     : true
      class       : device
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        If set to true, MPI_Reduce will allow the device to override the
        MPIR-level collective algorithms. The device still has the
        option to call the MPIR-level algorithms manually.
        If set to false, the device-level reduce function will not be
        called.

=== END_MPI_T_CVAR_INFO_BLOCK ===
*/

/* -- Begin Profiling Symbol Block for routine MPI_Reduce */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Reduce = PMPI_Reduce
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Reduce  MPI_Reduce
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Reduce as PMPI_Reduce
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPI_Reduce(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype,
               MPI_Op op, int root, MPI_Comm comm)
    __attribute__ ((weak, alias("PMPI_Reduce")));
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Reduce
#define MPI_Reduce PMPI_Reduce


/* This is the machine-independent implementation of reduce. The algorithm is:

   Algorithm: MPI_Reduce

   For long messages and for builtin ops and if count >= pof2 (where
   pof2 is the nearest power-of-two less than or equal to the number
   of processes), we use Rabenseifner's algorithm (see
   http://www.hlrs.de/organization/par/services/models/mpi/myreduce.html).
   This algorithm implements the reduce in two steps: first a
   reduce-scatter, followed by a gather to the root. A
   recursive-halving algorithm (beginning with processes that are
   distance 1 apart) is used for the reduce-scatter, and a binomial tree
   algorithm is used for the gather. The non-power-of-two case is
   handled by dropping to the nearest lower power-of-two: the first
   few odd-numbered processes send their data to their left neighbors
   (rank-1), and the reduce-scatter happens among the remaining
   power-of-two processes. If the root is one of the excluded
   processes, then after the reduce-scatter, rank 0 sends its result to
   the root and exits; the root now acts as rank 0 in the binomial tree
   algorithm for gather.

   For the power-of-two case, the cost for the reduce-scatter is
   lgp.alpha + n.((p-1)/p).beta + n.((p-1)/p).gamma. The cost for the
   gather to root is lgp.alpha + n.((p-1)/p).beta. Therefore, the
   total cost is:
   Cost = 2.lgp.alpha + 2.n.((p-1)/p).beta + n.((p-1)/p).gamma

   For the non-power-of-two case, assuming the root is not one of the
   odd-numbered processes that get excluded in the reduce-scatter,
   Cost = (2.floor(lgp)+1).alpha + (2.((p-1)/p) + 1).n.beta +
           n.(1+(p-1)/p).gamma


   For short messages, user-defined ops, and count < pof2, we use a
   binomial tree algorithm for both short and long messages.

   Cost = lgp.alpha + n.lgp.beta + n.lgp.gamma


   We use the binomial tree algorithm in the case of user-defined ops
   because in this case derived datatypes are allowed, and the user
   could pass basic datatypes on one process and derived on another as
   long as the type maps are the same. Breaking up derived datatypes
   to do the reduce-scatter is tricky.

   FIXME: Per the MPI-2.1 standard this case is not possible.  We
   should be able to use the reduce-scatter/gather approach as long as
   count >= pof2.  [goodell@ 2009-01-21]

   Possible improvements:

   End Algorithm: MPI_Reduce
*/


#undef FUNCNAME
#define FUNCNAME MPIR_Reduce_intra_auto
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Reduce_intra_auto(const void *sendbuf,
                           void *recvbuf,
                           int count,
                           MPI_Datatype datatype,
                           MPI_Op op, int root, MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag)
{
    int mpi_errno = MPI_SUCCESS;
    int mpi_errno_ret = MPI_SUCCESS;
    int is_commutative, type_size, pof2;
    int nbytes = 0;

    if (count == 0)
        return MPI_SUCCESS;

    /* is the op commutative? We do SMP optimizations only if it is. */
    is_commutative = MPIR_Op_is_commutative(op);

    MPIR_Datatype_get_size_macro(datatype, type_size);
    nbytes = MPIR_CVAR_MAX_SMP_REDUCE_MSG_SIZE ? type_size * count : 0;

    if (MPIR_CVAR_ENABLE_SMP_COLLECTIVES &&
        MPIR_CVAR_ENABLE_SMP_REDUCE &&
        MPIR_Comm_is_node_aware(comm_ptr) &&
        is_commutative && nbytes <= MPIR_CVAR_MAX_SMP_REDUCE_MSG_SIZE) {
        mpi_errno = MPIR_Reduce_intra_smp(sendbuf, recvbuf, count, datatype,
                                          op, root, comm_ptr, errflag);

        if (mpi_errno) {
            /* for communication errors, just record the error but continue */
            *errflag =
                MPIX_ERR_PROC_FAILED ==
                MPIR_ERR_GET_CLASS(mpi_errno) ? MPIR_ERR_PROC_FAILED : MPIR_ERR_OTHER;
            MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
            MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
        }

        goto fn_exit;
    }

    MPIR_Datatype_get_size_macro(datatype, type_size);

    /* get nearest power-of-two less than or equal to number of ranks in the communicator */
    pof2 = comm_ptr->pof2;

    if ((count * type_size > MPIR_CVAR_REDUCE_SHORT_MSG_SIZE) &&
        (HANDLE_GET_KIND(op) == HANDLE_KIND_BUILTIN) && (count >= pof2)) {
        /* do a reduce-scatter followed by gather to root. */
        mpi_errno =
            MPIR_Reduce_intra_reduce_scatter_gather(sendbuf, recvbuf, count, datatype, op, root,
                                                    comm_ptr, errflag);
    } else {
        /* use a binomial tree algorithm */
        mpi_errno =
            MPIR_Reduce_intra_binomial(sendbuf, recvbuf, count, datatype, op, root, comm_ptr,
                                       errflag);
    }
    if (mpi_errno) {
        /* for communication errors, just record the error but continue */
        *errflag =
            MPIX_ERR_PROC_FAILED ==
            MPIR_ERR_GET_CLASS(mpi_errno) ? MPIR_ERR_PROC_FAILED : MPIR_ERR_OTHER;
        MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
        MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
    }

  fn_exit:
    if (mpi_errno_ret)
        mpi_errno = mpi_errno_ret;
    else if (*errflag != MPIR_ERR_NONE)
        MPIR_ERR_SET(mpi_errno, *errflag, "**coll_fail");
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIR_Reduce_inter_auto
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Reduce_inter_auto(const void *sendbuf,
                           void *recvbuf,
                           int count,
                           MPI_Datatype datatype,
                           MPI_Op op, int root, MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag)
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = MPIR_Reduce_inter_local_reduce_remote_send(sendbuf, recvbuf, count, datatype,
                                                           op, root, comm_ptr, errflag);

    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIR_Reduce_impl
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Reduce_impl(const void *sendbuf, void *recvbuf, int count,
                     MPI_Datatype datatype, MPI_Op op, int root,
                     MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag)
{
    int mpi_errno = MPI_SUCCESS;

    if (comm_ptr->comm_kind == MPIR_COMM_KIND__INTRACOMM) {
        /* intracommunicator */
        switch (MPIR_Reduce_intra_algo_choice) {
            case MPIR_REDUCE_INTRA_ALGO_BINOMIAL:
                mpi_errno = MPIR_Reduce_intra_binomial(sendbuf, recvbuf,
                                                       count, datatype, op, root, comm_ptr,
                                                       errflag);
                break;
            case MPIR_REDUCE_INTRA_ALGO_REDUCE_SCATTER_GATHER:
                mpi_errno = MPIR_Reduce_intra_reduce_scatter_gather(sendbuf, recvbuf,
                                                                    count, datatype, op, root,
                                                                    comm_ptr, errflag);
                break;
            case MPIR_REDUCE_INTRA_ALGO_NB:
                mpi_errno = MPIR_Reduce_allcomm_nb(sendbuf, recvbuf,
                                                   count, datatype, op, root, comm_ptr, errflag);
                break;
            case MPIR_REDUCE_INTRA_ALGO_AUTO:
                MPL_FALLTHROUGH;
            default:
                mpi_errno = MPIR_Reduce_intra_auto(sendbuf, recvbuf,
                                                   count, datatype, op, root, comm_ptr, errflag);
                break;
        }
    } else {
        /* intercommunicator */
        switch (MPIR_Reduce_inter_algo_choice) {
            case MPIR_REDUCE_INTER_ALGO_LOCAL_REDUCE_REMOTE_SEND:
                mpi_errno =
                    MPIR_Reduce_inter_local_reduce_remote_send(sendbuf, recvbuf, count, datatype,
                                                               op, root, comm_ptr, errflag);
                break;
            case MPIR_REDUCE_INTER_ALGO_NB:
                mpi_errno = MPIR_Reduce_allcomm_nb(sendbuf, recvbuf,
                                                   count, datatype, op, root, comm_ptr, errflag);
                break;
            case MPIR_REDUCE_INTER_ALGO_AUTO:
                MPL_FALLTHROUGH;
            default:
                mpi_errno = MPIR_Reduce_inter_auto(sendbuf, recvbuf, count, datatype,
                                                   op, root, comm_ptr, errflag);
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
#define FUNCNAME MPIR_Reduce
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Reduce(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype,
                MPI_Op op, int root, MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag)
{
    int mpi_errno = MPI_SUCCESS;

    if (MPIR_CVAR_REDUCE_DEVICE_COLLECTIVE && MPIR_CVAR_DEVICE_COLLECTIVES) {
        mpi_errno = MPID_Reduce(sendbuf, recvbuf, count, datatype, op, root, comm_ptr, errflag);
    } else {
        mpi_errno = MPIR_Reduce_impl(sendbuf, recvbuf, count, datatype, op, root, comm_ptr,
                                     errflag);
    }

    return mpi_errno;
}

#endif


#undef FUNCNAME
#define FUNCNAME MPI_Reduce
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)

/*@

MPI_Reduce - Reduces values on all processes to a single value

Input Parameters:
+ sendbuf - address of send buffer (choice)
. count - number of elements in send buffer (integer)
. datatype - data type of elements of send buffer (handle)
. op - reduce operation (handle)
. root - rank of root process (integer)
- comm - communicator (handle)

Output Parameters:
. recvbuf - address of receive buffer (choice,
 significant only at 'root')

.N ThreadSafe

.N Fortran

.N collops

.N Errors
.N MPI_SUCCESS
.N MPI_ERR_COMM
.N MPI_ERR_COUNT
.N MPI_ERR_TYPE
.N MPI_ERR_BUFFER
.N MPI_ERR_BUFFER_ALIAS

@*/
int MPI_Reduce(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype,
               MPI_Op op, int root, MPI_Comm comm)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Comm *comm_ptr = NULL;
    MPIR_Errflag_t errflag = MPIR_ERR_NONE;
    MPIR_FUNC_TERSE_STATE_DECL(MPID_STATE_MPI_REDUCE);

    MPIR_ERRTEST_INITIALIZED_ORDIE();

    MPID_THREAD_CS_ENTER(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    MPIR_FUNC_TERSE_COLL_ENTER(MPID_STATE_MPI_REDUCE);

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
            int rank;

            MPIR_Comm_valid_ptr(comm_ptr, mpi_errno, FALSE);
            if (mpi_errno != MPI_SUCCESS)
                goto fn_fail;

            if (comm_ptr->comm_kind == MPIR_COMM_KIND__INTRACOMM) {
                MPIR_ERRTEST_INTRA_ROOT(comm_ptr, root, mpi_errno);

                MPIR_ERRTEST_COUNT(count, mpi_errno);
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

                if (sendbuf != MPI_IN_PLACE)
                    MPIR_ERRTEST_USERBUFFER(sendbuf, count, datatype, mpi_errno);

                rank = comm_ptr->rank;
                if (rank == root) {
                    MPIR_ERRTEST_RECVBUF_INPLACE(recvbuf, count, mpi_errno);
                    MPIR_ERRTEST_USERBUFFER(recvbuf, count, datatype, mpi_errno);
                    if (count != 0 && sendbuf != MPI_IN_PLACE) {
                        MPIR_ERRTEST_ALIAS_COLL(sendbuf, recvbuf, mpi_errno);
                    }
                } else
                    MPIR_ERRTEST_SENDBUF_INPLACE(sendbuf, count, mpi_errno);
            }

            if (comm_ptr->comm_kind == MPIR_COMM_KIND__INTERCOMM) {
                MPIR_ERRTEST_INTER_ROOT(comm_ptr, root, mpi_errno);

                if (root == MPI_ROOT) {
                    MPIR_ERRTEST_COUNT(count, mpi_errno);
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
                    MPIR_ERRTEST_RECVBUF_INPLACE(recvbuf, count, mpi_errno);
                    MPIR_ERRTEST_USERBUFFER(recvbuf, count, datatype, mpi_errno);
                }

                else if (root != MPI_PROC_NULL) {
                    MPIR_ERRTEST_COUNT(count, mpi_errno);
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
                    MPIR_ERRTEST_SENDBUF_INPLACE(sendbuf, count, mpi_errno);
                    MPIR_ERRTEST_USERBUFFER(sendbuf, count, datatype, mpi_errno);
                }
            }

            MPIR_ERRTEST_OP(op, mpi_errno);

            if (mpi_errno != MPI_SUCCESS)
                goto fn_fail;
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

    mpi_errno = MPIR_Reduce(sendbuf, recvbuf, count, datatype, op, root, comm_ptr, &errflag);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

    /* ... end of body of routine ... */

  fn_exit:
    MPIR_FUNC_TERSE_COLL_EXIT(MPID_STATE_MPI_REDUCE);
    MPID_THREAD_CS_EXIT(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
#ifdef HAVE_ERROR_CHECKING
    {
        mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE,
                                         FCNAME, __LINE__, MPI_ERR_OTHER,
                                         "**mpi_reduce", "**mpi_reduce %p %p %d %D %O %d %C",
                                         sendbuf, recvbuf, count, datatype, op, root, comm);
    }
#endif
    mpi_errno = MPIR_Err_return_comm(comm_ptr, FCNAME, mpi_errno);
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}
