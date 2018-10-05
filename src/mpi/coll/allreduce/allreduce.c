/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/*
=== BEGIN_MPI_T_CVAR_INFO_BLOCK ===

cvars:
    - name        : MPIR_CVAR_ALLREDUCE_SHORT_MSG_SIZE
      category    : COLLECTIVE
      type        : int
      default     : 2048
      class       : device
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        the short message algorithm will be used if the send buffer size is <=
        this value (in bytes)

    - name        : MPIR_CVAR_ENABLE_SMP_COLLECTIVES
      category    : COLLECTIVE
      type        : boolean
      default     : true
      class       : device
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        Enable SMP aware collective communication.

    - name        : MPIR_CVAR_ENABLE_SMP_ALLREDUCE
      category    : COLLECTIVE
      type        : boolean
      default     : true
      class       : device
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        Enable SMP aware allreduce.

    - name        : MPIR_CVAR_MAX_SMP_ALLREDUCE_MSG_SIZE
      category    : COLLECTIVE
      type        : int
      default     : 0
      class       : device
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        Maximum message size for which SMP-aware allreduce is used.  A
        value of '0' uses SMP-aware allreduce for all message sizes.

    - name        : MPIR_CVAR_ALLREDUCE_INTRA_ALGORITHM
      category    : COLLECTIVE
      type        : string
      default     : auto
      class       : device
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : |-
        Variable to select allreduce algorithm
        auto                     - Internal algorithm selection
        nb                       - Force nonblocking algorithm
        recursive_doubling       - Force recursive doubling algorithm
        reduce_scatter_allgather - Force reduce scatter allgather algorithm

    - name        : MPIR_CVAR_ALLREDUCE_INTER_ALGORITHM
      category    : COLLECTIVE
      type        : string
      default     : auto
      class       : device
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : |-
        Variable to select allreduce algorithm
        auto                  - Internal algorithm selection
        nb                    - Force nonblocking algorithm
        reduce_exchange_bcast - Force reduce-exchange-bcast algorithm

    - name        : MPIR_CVAR_ALLREDUCE_DEVICE_COLLECTIVE
      category    : COLLECTIVE
      type        : boolean
      default     : true
      class       : device
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        If set to true, MPI_Allreduce will allow the device to override the
        MPIR-level collective algorithms. The device still has the
        option to call the MPIR-level algorithms manually.
        If set to false, the device-level allreduce function will not be
        called.

=== END_MPI_T_CVAR_INFO_BLOCK ===
*/

/* -- Begin Profiling Symbol Block for routine MPI_Allreduce */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Allreduce = PMPI_Allreduce
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Allreduce  MPI_Allreduce
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Allreduce as PMPI_Allreduce
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPI_Allreduce(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype,
                  MPI_Op op, MPI_Comm comm)
    __attribute__ ((weak, alias("PMPI_Allreduce")));
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Allreduce
#define MPI_Allreduce PMPI_Allreduce

/* The order of entries in this table must match the definitions in
   mpi.h.in */
MPI_User_function *MPIR_Op_table[] = {
    NULL, MPIR_MAXF,
    MPIR_MINF, MPIR_SUM,
    MPIR_PROD, MPIR_LAND,
    MPIR_BAND, MPIR_LOR, MPIR_BOR,
    MPIR_LXOR, MPIR_BXOR,
    MPIR_MINLOC, MPIR_MAXLOC,
    MPIR_REPLACE, MPIR_NO_OP
};

MPIR_Op_check_dtype_fn *MPIR_Op_check_dtype_table[] = {
    NULL, MPIR_MAXF_check_dtype,
    MPIR_MINF_check_dtype, MPIR_SUM_check_dtype,
    MPIR_PROD_check_dtype, MPIR_LAND_check_dtype,
    MPIR_BAND_check_dtype, MPIR_LOR_check_dtype, MPIR_BOR_check_dtype,
    MPIR_LXOR_check_dtype, MPIR_BXOR_check_dtype,
    MPIR_MINLOC_check_dtype, MPIR_MAXLOC_check_dtype,
    MPIR_REPLACE_check_dtype, MPIR_NO_OP_check_dtype
};

#undef FUNCNAME
#define FUNCNAME MPIR_Allreduce_intra_auto
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Allreduce_intra_auto(const void *sendbuf,
                              void *recvbuf,
                              int count,
                              MPI_Datatype datatype,
                              MPI_Op op, MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag)
{
    int mpi_errno = MPI_SUCCESS;
    int mpi_errno_ret = MPI_SUCCESS;
    int pof2;
    int is_commutative;
    int nbytes = 0;
    MPI_Aint type_size;

    MPIR_Datatype_get_size_macro(datatype, type_size);
    nbytes = MPIR_CVAR_MAX_SMP_ALLREDUCE_MSG_SIZE ? type_size * count : 0;
    is_commutative = MPIR_Op_is_commutative(op);

    if (count == 0)
        goto fn_exit;

    /* is the op commutative? We do SMP optimizations only if it is. */
    if (MPIR_CVAR_ENABLE_SMP_COLLECTIVES &&
        MPIR_CVAR_ENABLE_SMP_ALLREDUCE &&
        MPIR_Comm_is_node_aware(comm_ptr) &&
        is_commutative && nbytes <= MPIR_CVAR_MAX_SMP_ALLREDUCE_MSG_SIZE) {
        mpi_errno =
            MPIR_Allreduce_intra_smp(sendbuf, recvbuf, count, datatype, op, comm_ptr, errflag);

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

    pof2 = comm_ptr->pof2;
    if ((nbytes <= MPIR_CVAR_ALLREDUCE_SHORT_MSG_SIZE) ||
        (HANDLE_GET_KIND(op) != HANDLE_KIND_BUILTIN) || (count < pof2)) {
        mpi_errno =
            MPIR_Allreduce_intra_recursive_doubling(sendbuf, recvbuf, count, datatype, op,
                                                    comm_ptr, errflag);
        if (mpi_errno) {
            /* for communication errors, just record the error but continue */
            *errflag =
                MPIX_ERR_PROC_FAILED ==
                MPIR_ERR_GET_CLASS(mpi_errno) ? MPIR_ERR_PROC_FAILED : MPIR_ERR_OTHER;
            MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
            MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
        }
    } else {
        mpi_errno =
            MPIR_Allreduce_intra_reduce_scatter_allgather(sendbuf, recvbuf, count, datatype, op,
                                                          comm_ptr, errflag);
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
    if (mpi_errno_ret)
        mpi_errno = mpi_errno_ret;
    return (mpi_errno);
}


#undef FUNCNAME
#define FUNCNAME MPIR_Allreduce_inter_auto
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Allreduce_inter_auto(const void *sendbuf,
                              void *recvbuf,
                              int count,
                              MPI_Datatype datatype,
                              MPI_Op op, MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag)
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno =
        MPIR_Allreduce_inter_reduce_exchange_bcast(sendbuf, recvbuf, count, datatype, op, comm_ptr,
                                                   errflag);

    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIR_Allreduce_impl
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Allreduce_impl(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype,
                        MPI_Op op, MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag)
{
    int mpi_errno = MPI_SUCCESS;

    if (comm_ptr->comm_kind == MPIR_COMM_KIND__INTRACOMM) {
        /* intracommunicator */
        switch (MPIR_Allreduce_intra_algo_choice) {
            case MPIR_ALLREDUCE_INTRA_ALGO_RECURSIVE_DOUBLING:
                mpi_errno = MPIR_Allreduce_intra_recursive_doubling(sendbuf, recvbuf, count,
                                                                    datatype, op, comm_ptr,
                                                                    errflag);
                break;
            case MPIR_ALLREDUCE_INTRA_ALGO_REDUCE_SCATTER_ALLGATHER:
                mpi_errno = MPIR_Allreduce_intra_reduce_scatter_allgather(sendbuf, recvbuf, count,
                                                                          datatype, op, comm_ptr,
                                                                          errflag);
                break;
            case MPIR_ALLREDUCE_INTRA_ALGO_NB:
                mpi_errno = MPIR_Allreduce_allcomm_nb(sendbuf, recvbuf, count,
                                                      datatype, op, comm_ptr, errflag);
                break;
            case MPIR_ALLREDUCE_INTRA_ALGO_AUTO:
                MPL_FALLTHROUGH;
            default:
                mpi_errno = MPIR_Allreduce_intra_auto(sendbuf, recvbuf, count, datatype, op,
                                                      comm_ptr, errflag);
                break;
        }
    } else {
        /* intercommunicator */
        switch (MPIR_Allreduce_inter_algo_choice) {
            case MPIR_ALLREDUCE_INTER_ALGO_REDUCE_EXCHANGE_BCAST:
                mpi_errno =
                    MPIR_Allreduce_inter_reduce_exchange_bcast(sendbuf, recvbuf, count, datatype,
                                                               op, comm_ptr, errflag);
                break;
            case MPIR_ALLREDUCE_INTER_ALGO_NB:
                mpi_errno = MPIR_Allreduce_allcomm_nb(sendbuf, recvbuf, count,
                                                      datatype, op, comm_ptr, errflag);
                break;
            case MPIR_ALLREDUCE_INTER_ALGO_AUTO:
                MPL_FALLTHROUGH;
            default:
                mpi_errno = MPIR_Allreduce_inter_auto(sendbuf, recvbuf, count, datatype, op,
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
#define FUNCNAME MPIR_Allreduce
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Allreduce(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype,
                   MPI_Op op, MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag)
{
    int mpi_errno = MPI_SUCCESS;

    if (MPIR_CVAR_ALLREDUCE_DEVICE_COLLECTIVE && MPIR_CVAR_DEVICE_COLLECTIVES) {
        mpi_errno = MPID_Allreduce(sendbuf, recvbuf, count, datatype, op, comm_ptr, errflag);
    } else {
        mpi_errno = MPIR_Allreduce_impl(sendbuf, recvbuf, count, datatype, op, comm_ptr, errflag);
    }

    return mpi_errno;
}

#endif

#undef FUNCNAME
#define FUNCNAME MPI_Allreduce
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
/*@
MPI_Allreduce - Combines values from all processes and distributes the result
                back to all processes

Input Parameters:
+ sendbuf - starting address of send buffer (choice)
. count - number of elements in send buffer (integer)
. datatype - data type of elements of send buffer (handle)
. op - operation (handle)
- comm - communicator (handle)

Output Parameters:
. recvbuf - starting address of receive buffer (choice)

.N ThreadSafe

.N Fortran

.N collops

.N Errors
.N MPI_ERR_BUFFER
.N MPI_ERR_COUNT
.N MPI_ERR_TYPE
.N MPI_ERR_OP
.N MPI_ERR_COMM
@*/
int MPI_Allreduce(const void *sendbuf, void *recvbuf, int count,
                  MPI_Datatype datatype, MPI_Op op, MPI_Comm comm)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Comm *comm_ptr = NULL;
    MPIR_Errflag_t errflag = MPIR_ERR_NONE;
    MPIR_FUNC_TERSE_STATE_DECL(MPID_STATE_MPI_ALLREDUCE);

    MPIR_ERRTEST_INITIALIZED_ORDIE();

    MPID_THREAD_CS_ENTER(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    MPIR_FUNC_TERSE_COLL_ENTER(MPID_STATE_MPI_ALLREDUCE);

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

            MPIR_Comm_valid_ptr(comm_ptr, mpi_errno, FALSE);
            if (mpi_errno != MPI_SUCCESS)
                goto fn_fail;
            MPIR_ERRTEST_COUNT(count, mpi_errno);
            MPIR_ERRTEST_DATATYPE(datatype, "datatype", mpi_errno);
            MPIR_ERRTEST_OP(op, mpi_errno);

            if (HANDLE_GET_KIND(datatype) != HANDLE_KIND_BUILTIN) {
                MPIR_Datatype_get_ptr(datatype, datatype_ptr);
                MPIR_Datatype_valid_ptr(datatype_ptr, mpi_errno);
                if (mpi_errno != MPI_SUCCESS)
                    goto fn_fail;
                MPIR_Datatype_committed_ptr(datatype_ptr, mpi_errno);
                if (mpi_errno != MPI_SUCCESS)
                    goto fn_fail;
            }

            if (comm_ptr->comm_kind == MPIR_COMM_KIND__INTERCOMM) {
                MPIR_ERRTEST_SENDBUF_INPLACE(sendbuf, count, mpi_errno);
            } else {
                if (count != 0 && sendbuf != MPI_IN_PLACE)
                    MPIR_ERRTEST_ALIAS_COLL(sendbuf, recvbuf, mpi_errno);
            }

            if (sendbuf != MPI_IN_PLACE)
                MPIR_ERRTEST_USERBUFFER(sendbuf, count, datatype, mpi_errno);

            MPIR_ERRTEST_RECVBUF_INPLACE(recvbuf, count, mpi_errno);
            MPIR_ERRTEST_USERBUFFER(recvbuf, count, datatype, mpi_errno);

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

    mpi_errno = MPIR_Allreduce(sendbuf, recvbuf, count, datatype, op, comm_ptr, &errflag);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

    /* ... end of body of routine ... */

  fn_exit:
    MPIR_FUNC_TERSE_COLL_EXIT(MPID_STATE_MPI_ALLREDUCE);
    MPID_THREAD_CS_EXIT(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
#ifdef HAVE_ERROR_CHECKING
    {
        mpi_errno =
            MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER,
                                 "**mpi_allreduce", "**mpi_allreduce %p %p %d %D %O %C", sendbuf,
                                 recvbuf, count, datatype, op, comm);
    }
#endif
    mpi_errno = MPIR_Err_return_comm(comm_ptr, FCNAME, mpi_errno);
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}
