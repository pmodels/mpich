/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2010 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/*
=== BEGIN_MPI_T_CVAR_INFO_BLOCK ===

cvars:
    - name        : MPIR_CVAR_IALLREDUCE_TREE_KVAL
      category    : COLLECTIVE
      type        : int
      default     : 2
      class       : device
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        k value for tree based iallreduce (for tree_kary and tree_knomial)

    - name        : MPIR_CVAR_IALLREDUCE_TREE_PIPELINE_CHUNK_SIZE
      category    : COLLECTIVE
      type        : int
      default     : -1
      class       : device
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        Maximum chunk size (in bytes) for pipelining in tree based
        iallreduce (tree_kary and tree_knomial). Default value is 0, that is,
        no pipelining by default

    - name        : MPIR_CVAR_IALLREDUCE_TREE_BUFFER_PER_CHILD
      category    : COLLECTIVE
      type        : boolean
      default     : 0
      class       : device
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        If set to true, a rank in tree_kary and tree_knomial algorithms will allocate
        a dedicated buffer for every child it receives data from. This would mean more
        memory consumption but it would allow preposting of the receives and hence reduce
        the number of unexpected messages. If set to false, there is only one buffer that is
        used to receive the data from all the children. The receives are therefore serialized,
        that is, only one receive can be posted at a time.

    - name        : MPIR_CVAR_IALLREDUCE_RECEXCH_KVAL
      category    : COLLECTIVE
      type        : int
      default     : 2
      class       : device
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        k value for recursive exchange based iallreduce

    - name        : MPIR_CVAR_IALLREDUCE_INTRA_ALGORITHM
      category    : COLLECTIVE
      type        : string
      default     : auto
      class       : device
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : |-
        Variable to select iallreduce algorithm
        auto                     - Internal algorithm selection
        naive                    - Force naive algorithm
        recursive_doubling       - Force recursive doubling algorithm
        reduce_scatter_allgather - Force reduce scatter allgather algorithm
        recexch_single_buffer    - Force generic transport recursive exchange with single buffer for receives
        recexch_multiple_buffer  - Force generic transport recursive exchange with multiple buffers for receives

    - name        : MPIR_CVAR_IALLREDUCE_INTER_ALGORITHM
      category    : COLLECTIVE
      type        : string
      default     : auto
      class       : device
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : |-
        Variable to select iallreduce algorithm
        auto                      - Internal algorithm selection
        remote_reduce_local_bcast - Force remote-reduce-local-bcast algorithm

    - name        : MPIR_CVAR_IALLREDUCE_DEVICE_COLLECTIVE
      category    : COLLECTIVE
      type        : boolean
      default     : true
      class       : device
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        If set to true, MPI_Iallreduce will allow the device to override the
        MPIR-level collective algorithms. The device still has the
        option to call the MPIR-level algorithms manually.
        If set to false, the device-level iallreduce function will not be
        called.

=== END_MPI_T_CVAR_INFO_BLOCK ===
*/

/* -- Begin Profiling Symbol Block for routine MPI_Iallreduce */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Iallreduce = PMPI_Iallreduce
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Iallreduce  MPI_Iallreduce
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Iallreduce as PMPI_Iallreduce
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPI_Iallreduce(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype,
                   MPI_Op op, MPI_Comm comm, MPI_Request * request)
    __attribute__ ((weak, alias("PMPI_Iallreduce")));
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Iallreduce
#define MPI_Iallreduce PMPI_Iallreduce

#undef FUNCNAME
#define FUNCNAME MPIR_Iallreduce_sched_intra_auto
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Iallreduce_sched_intra_auto(const void *sendbuf, void *recvbuf, int count,
                                     MPI_Datatype datatype, MPI_Op op, MPIR_Comm * comm_ptr,
                                     MPIR_Sched_t s)
{
    int mpi_errno = MPI_SUCCESS;
    int pof2, type_size;

    MPIR_Assert(comm_ptr->comm_kind == MPIR_COMM_KIND__INTRACOMM);

    MPIR_Datatype_get_size_macro(datatype, type_size);

    /* get nearest power-of-two less than or equal to number of ranks in the communicator */
    pof2 = comm_ptr->pof2;

    /* If op is user-defined or count is less than pof2, use
     * recursive doubling algorithm. Otherwise do a reduce-scatter
     * followed by allgather. (If op is user-defined,
     * derived datatypes are allowed and the user could pass basic
     * datatypes on one process and derived on another as long as
     * the type maps are the same. Breaking up derived
     * datatypes to do the reduce-scatter is tricky, therefore
     * using recursive doubling in that case.) */

    if ((count * type_size <= MPIR_CVAR_ALLREDUCE_SHORT_MSG_SIZE) ||
        (HANDLE_GET_KIND(op) != HANDLE_KIND_BUILTIN) || (count < pof2)) {
        /* use recursive doubling */
        mpi_errno =
            MPIR_Iallreduce_sched_intra_recursive_doubling(sendbuf, recvbuf, count, datatype, op,
                                                           comm_ptr, s);
        if (mpi_errno)
            MPIR_ERR_POP(mpi_errno);
    } else {
        /* do a reduce-scatter followed by allgather */
        mpi_errno =
            MPIR_Iallreduce_sched_intra_reduce_scatter_allgather(sendbuf, recvbuf, count, datatype,
                                                                 op, comm_ptr, s);
        if (mpi_errno)
            MPIR_ERR_POP(mpi_errno);
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIR_Iallreduce_sched_inter_auto
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Iallreduce_sched_inter_auto(const void *sendbuf, void *recvbuf, int count,
                                     MPI_Datatype datatype, MPI_Op op, MPIR_Comm * comm_ptr,
                                     MPIR_Sched_t s)
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = MPIR_Iallreduce_sched_inter_remote_reduce_local_bcast(sendbuf, recvbuf, count,
                                                                      datatype, op, comm_ptr, s);

    return mpi_errno;
}


#undef FUNCNAME
#define FUNCNAME MPIR_Iallreduce_sched_impl
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Iallreduce_sched_impl(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype,
                               MPI_Op op, MPIR_Comm * comm_ptr, MPIR_Sched_t s)
{
    int mpi_errno = MPI_SUCCESS;

    if (comm_ptr->comm_kind == MPIR_COMM_KIND__INTRACOMM) {
        if (comm_ptr->hierarchy_kind == MPIR_COMM_HIERARCHY_KIND__PARENT &&
            MPIR_CVAR_ENABLE_SMP_COLLECTIVES && MPIR_CVAR_ENABLE_SMP_ALLREDUCE) {
            mpi_errno =
                MPIR_Iallreduce_sched_intra_smp(sendbuf, recvbuf, count, datatype, op, comm_ptr, s);
        } else {
            /* intracommunicator */
            switch (MPIR_Iallreduce_intra_algo_choice) {
                case MPIR_IALLREDUCE_INTRA_ALGO_NAIVE:
                    mpi_errno = MPIR_Iallreduce_sched_intra_naive(sendbuf, recvbuf, count,
                                                                  datatype, op, comm_ptr, s);
                    break;
                case MPIR_IALLREDUCE_INTRA_ALGO_RECURSIVE_DOUBLING:
                    mpi_errno =
                        MPIR_Iallreduce_sched_intra_recursive_doubling(sendbuf, recvbuf, count,
                                                                       datatype, op, comm_ptr, s);
                    break;
                case MPIR_IALLREDUCE_INTRA_ALGO_REDUCE_SCATTER_ALLGATHER:
                    mpi_errno =
                        MPIR_Iallreduce_sched_intra_reduce_scatter_allgather(sendbuf, recvbuf,
                                                                             count, datatype, op,
                                                                             comm_ptr, s);
                    break;
                case MPIR_IALLREDUCE_INTRA_ALGO_AUTO:
                    MPL_FALLTHROUGH;
                default:
                    mpi_errno = MPIR_Iallreduce_sched_intra_auto(sendbuf, recvbuf, count,
                                                                 datatype, op, comm_ptr, s);
                    break;
            }
        }
    } else {
        /* intercommunicator */
        switch (MPIR_Iallreduce_inter_algo_choice) {
            case MPIR_IALLREDUCE_INTER_ALGO_REMOTE_REDUCE_LOCAL_BCAST:
                mpi_errno =
                    MPIR_Iallreduce_sched_inter_remote_reduce_local_bcast(sendbuf, recvbuf, count,
                                                                          datatype, op, comm_ptr,
                                                                          s);
                break;
            case MPIR_IALLREDUCE_INTER_ALGO_AUTO:
                MPL_FALLTHROUGH;
            default:
                mpi_errno = MPIR_Iallreduce_sched_inter_auto(sendbuf, recvbuf, count,
                                                             datatype, op, comm_ptr, s);
                break;
        }

    }
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIR_Iallreduce_sched
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Iallreduce_sched(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype,
                          MPI_Op op, MPIR_Comm * comm_ptr, MPIR_Sched_t s)
{
    int mpi_errno = MPI_SUCCESS;

    if (MPIR_CVAR_IALLREDUCE_DEVICE_COLLECTIVE && MPIR_CVAR_DEVICE_COLLECTIVES) {
        mpi_errno = MPID_Iallreduce_sched(sendbuf, recvbuf, count, datatype, op, comm_ptr, s);
    } else {
        mpi_errno = MPIR_Iallreduce_sched_impl(sendbuf, recvbuf, count, datatype, op, comm_ptr, s);
    }

    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIR_Iallreduce_impl
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Iallreduce_impl(const void *sendbuf, void *recvbuf, int count,
                         MPI_Datatype datatype, MPI_Op op, MPIR_Comm * comm_ptr,
                         MPIR_Request ** request)
{
    int mpi_errno = MPI_SUCCESS;
    int tag = -1;
    MPIR_Sched_t s = MPIR_SCHED_NULL;

    *request = NULL;
    /* If the user picks one of the transport-enabled algorithms, branch there
     * before going down to the MPIR_Sched-based algorithms. */
    /* TODO - Eventually the intention is to replace all of the
     * MPIR_Sched-based algorithms with transport-enabled algorithms, but that
     * will require sufficient performance testing and replacement algorithms. */
    if (comm_ptr->comm_kind == MPIR_COMM_KIND__INTRACOMM) {
        /* intracommunicator */
        switch (MPIR_Iallreduce_intra_algo_choice) {
            case MPIR_IALLREDUCE_INTRA_ALGO_GENTRAN_RECEXCH_SINGLE_BUFFER:
                mpi_errno =
                    MPIR_Iallreduce_intra_recexch_single_buffer(sendbuf, recvbuf, count, datatype,
                                                                op, comm_ptr, request);
                if (mpi_errno)
                    MPIR_ERR_POP(mpi_errno);
                goto fn_exit;
                break;
            case MPIR_IALLREDUCE_INTRA_ALGO_GENTRAN_RECEXCH_MULTIPLE_BUFFER:
                mpi_errno =
                    MPIR_Iallreduce_intra_recexch_multiple_buffer(sendbuf, recvbuf, count, datatype,
                                                                  op, comm_ptr, request);
                if (mpi_errno)
                    MPIR_ERR_POP(mpi_errno);
                goto fn_exit;
                break;
            case MPIR_IALLREDUCE_INTRA_ALGO_GENTRAN_TREE_KARY:
                mpi_errno =
                    MPIR_Iallreduce_intra_tree_kary(sendbuf, recvbuf, count, datatype,
                                                    op, comm_ptr, request);
                if (mpi_errno)
                    MPIR_ERR_POP(mpi_errno);
                goto fn_exit;
                break;
            case MPIR_IALLREDUCE_INTRA_ALGO_GENTRAN_TREE_KNOMIAL:
                mpi_errno =
                    MPIR_Iallreduce_intra_tree_knomial(sendbuf, recvbuf, count, datatype,
                                                       op, comm_ptr, request);
                if (mpi_errno)
                    MPIR_ERR_POP(mpi_errno);
                goto fn_exit;
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

    mpi_errno = MPIR_Iallreduce_sched(sendbuf, recvbuf, count, datatype, op, comm_ptr, s);
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
#define FUNCNAME MPIR_Iallreduce
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Iallreduce(const void *sendbuf, void *recvbuf, int count,
                    MPI_Datatype datatype, MPI_Op op, MPIR_Comm * comm_ptr, MPIR_Request ** request)
{
    int mpi_errno = MPI_SUCCESS;

    if (MPIR_CVAR_IALLREDUCE_DEVICE_COLLECTIVE && MPIR_CVAR_DEVICE_COLLECTIVES) {
        mpi_errno = MPID_Iallreduce(sendbuf, recvbuf, count, datatype, op, comm_ptr, request);
    } else {
        mpi_errno = MPIR_Iallreduce_impl(sendbuf, recvbuf, count, datatype, op, comm_ptr, request);
    }

    return mpi_errno;
}

#endif /* MPICH_MPI_FROM_PMPI */

#undef FUNCNAME
#define FUNCNAME MPI_Iallreduce
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
/*@
MPI_Iallreduce - Combines values from all processes and distributes the result
                 back to all processes in a nonblocking way

Input Parameters:
+ sendbuf - starting address of the send buffer (choice)
. count - number of elements in send buffer (non-negative integer)
. datatype - data type of elements of send buffer (handle)
. op - operation (handle)
- comm - communicator (handle)

Output Parameters:
+ recvbuf - starting address of the receive buffer (choice)
- request - communication request (handle)

.N ThreadSafe

.N Fortran

.N Errors
@*/
int MPI_Iallreduce(const void *sendbuf, void *recvbuf, int count,
                   MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, MPI_Request * request)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Comm *comm_ptr = NULL;
    MPIR_Request *request_ptr = NULL;
    MPIR_FUNC_TERSE_STATE_DECL(MPID_STATE_MPI_IALLREDUCE);

    MPID_THREAD_CS_ENTER(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    MPIR_FUNC_TERSE_ENTER(MPID_STATE_MPI_IALLREDUCE);

    /* Validate parameters, especially handles needing to be converted */
#ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            MPIR_ERRTEST_DATATYPE(datatype, "datatype", mpi_errno);
            MPIR_ERRTEST_COUNT(count, mpi_errno);
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

            if (comm_ptr->comm_kind == MPIR_COMM_KIND__INTERCOMM)
                MPIR_ERRTEST_SENDBUF_INPLACE(sendbuf, count, mpi_errno);

            if (sendbuf != MPI_IN_PLACE)
                MPIR_ERRTEST_USERBUFFER(sendbuf, count, datatype, mpi_errno);

            MPIR_ERRTEST_ARGNULL(request, "request", mpi_errno);

            if (comm_ptr->comm_kind == MPIR_COMM_KIND__INTRACOMM && count != 0 &&
                sendbuf != MPI_IN_PLACE)
                MPIR_ERRTEST_ALIAS_COLL(sendbuf, recvbuf, mpi_errno);

            /* TODO more checks may be appropriate (counts, in_place, buffer aliasing, etc) */
        }
        MPID_END_ERROR_CHECKS;
    }
#endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */

    mpi_errno = MPIR_Iallreduce(sendbuf, recvbuf, count, datatype, op, comm_ptr, &request_ptr);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

    /* create a complete request, if needed */
    if (!request_ptr)
        request_ptr = MPIR_Request_create_complete(MPIR_REQUEST_KIND__COLL);
    /* return the handle of the request to the user */
    *request = request_ptr->handle;

    /* ... end of body of routine ... */

  fn_exit:
    MPIR_FUNC_TERSE_EXIT(MPID_STATE_MPI_IALLREDUCE);
    MPID_THREAD_CS_EXIT(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
#ifdef HAVE_ERROR_CHECKING
    {
        mpi_errno =
            MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER,
                                 "**mpi_iallreduce", "**mpi_iallreduce %p %p %d %D %O %C %p",
                                 sendbuf, recvbuf, count, datatype, op, comm, request);
    }
#endif
    mpi_errno = MPIR_Err_return_comm(comm_ptr, FCNAME, mpi_errno);
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}
