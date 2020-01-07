/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2010 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/*
=== BEGIN_MPI_T_CVAR_INFO_BLOCK ===

cvars:
    - name        : MPIR_CVAR_IREDUCE_TREE_KVAL
      category    : COLLECTIVE
      type        : int
      default     : 2
      class       : device
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        k value for tree (kary, knomial, etc.) based ireduce

    - name        : MPIR_CVAR_IREDUCE_TREE_TYPE
      category    : COLLECTIVE
      type        : string
      default     : kary
      class       : device
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        Tree type for tree based ireduce
        kary      - kary tree
        knomial_1 - knomial_1 tree
        knomial_2 - knomial_2 tree

    - name        : MPIR_CVAR_IREDUCE_TREE_PIPELINE_CHUNK_SIZE
      category    : COLLECTIVE
      type        : int
      default     : -1
      class       : device
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        Maximum chunk size (in bytes) for pipelining in tree based
        ireduce. Default value is 0, that is, no pipelining by default

    - name        : MPIR_CVAR_IREDUCE_RING_CHUNK_SIZE
      category    : COLLECTIVE
      type        : int
      default     : 0
      class       : device
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        Maximum chunk size (in bytes) for pipelining in ireduce ring algorithm.
        Default value is 0, that is, no pipelining by default

    - name        : MPIR_CVAR_IREDUCE_TREE_BUFFER_PER_CHILD
      category    : COLLECTIVE
      type        : boolean
      default     : 0
      class       : device
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        If set to true, a rank in tree algorithms will allocate
        a dedicated buffer for every child it receives data from. This would mean more
        memory consumption but it would allow preposting of the receives and hence reduce
        the number of unexpected messages. If set to false, there is only one buffer that is
        used to receive the data from all the children. The receives are therefore serialized,
        that is, only one receive can be posted at a time.

    - name        : MPIR_CVAR_IREDUCE_INTRA_ALGORITHM
      category    : COLLECTIVE
      type        : enum
      default     : auto
      class       : device
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : |-
        Variable to select ireduce algorithm
        auto                  - Internal algorithm selection
        binomial              - Force binomial algorithm
        reduce_scatter_gather - Force reduce scatter gather algorithm
        gentran_tree          - Force Generic Transport Tree
        gentran_ring          - Force Generic Transport Ring

    - name        : MPIR_CVAR_IREDUCE_INTER_ALGORITHM
      category    : COLLECTIVE
      type        : enum
      default     : auto
      class       : device
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : |-
        Variable to select ireduce algorithm
        auto                     - Internal algorithm selection
        local_reduce_remote_send - Force local-reduce-remote-send algorithm

    - name        : MPIR_CVAR_IREDUCE_DEVICE_COLLECTIVE
      category    : COLLECTIVE
      type        : boolean
      default     : true
      class       : device
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        If set to true, MPI_Ireduce will allow the device to override the
        MPIR-level collective algorithms. The device still has the
        option to call the MPIR-level algorithms manually.
        If set to false, the device-level ireduce function will not be
        called.

=== END_MPI_T_CVAR_INFO_BLOCK ===
*/

/* -- Begin Profiling Symbol Block for routine MPI_Ireduce */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Ireduce = PMPI_Ireduce
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Ireduce  MPI_Ireduce
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Ireduce as PMPI_Ireduce
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPI_Ireduce(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype,
                MPI_Op op, int root, MPI_Comm comm, MPI_Request * request)
    __attribute__ ((weak, alias("PMPI_Ireduce")));
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Ireduce
#define MPI_Ireduce PMPI_Ireduce

int MPIR_Ireduce_sched_intra_auto(const void *sendbuf, void *recvbuf, int count,
                                  MPI_Datatype datatype, MPI_Op op, int root, MPIR_Comm * comm_ptr,
                                  MPIR_Sched_t s)
{
    int mpi_errno = MPI_SUCCESS;
    int pof2, type_size;

    MPIR_Assert(comm_ptr->comm_kind == MPIR_COMM_KIND__INTRACOMM);

    MPIR_Datatype_get_size_macro(datatype, type_size);

    /* get nearest power-of-two less than or equal to number of ranks in the communicator */
    pof2 = comm_ptr->coll.pof2;

    if ((count * type_size > MPIR_CVAR_REDUCE_SHORT_MSG_SIZE) &&
        (HANDLE_IS_BUILTIN(op)) && (count >= pof2)) {
        /* do a reduce-scatter followed by gather to root. */
        mpi_errno =
            MPIR_Ireduce_sched_intra_reduce_scatter_gather(sendbuf, recvbuf, count, datatype, op,
                                                           root, comm_ptr, s);
        MPIR_ERR_CHECK(mpi_errno);
    } else {
        /* use a binomial tree algorithm */
        mpi_errno =
            MPIR_Ireduce_sched_intra_binomial(sendbuf, recvbuf, count, datatype, op, root, comm_ptr,
                                              s);
        MPIR_ERR_CHECK(mpi_errno);
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIR_Ireduce_sched_inter_auto(const void *sendbuf, void *recvbuf,
                                  int count, MPI_Datatype datatype, MPI_Op op, int root,
                                  MPIR_Comm * comm_ptr, MPIR_Sched_t s)
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = MPIR_Ireduce_sched_inter_local_reduce_remote_send(sendbuf, recvbuf, count,
                                                                  datatype, op, root, comm_ptr, s);

    return mpi_errno;
}

int MPIR_Ireduce_sched_impl(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype,
                            MPI_Op op, int root, MPIR_Comm * comm_ptr, MPIR_Sched_t s)
{
    int mpi_errno = MPI_SUCCESS;

    if (comm_ptr->comm_kind == MPIR_COMM_KIND__INTRACOMM) {
        if (comm_ptr->hierarchy_kind == MPIR_COMM_HIERARCHY_KIND__PARENT &&
            MPIR_CVAR_ENABLE_SMP_COLLECTIVES && MPIR_CVAR_ENABLE_SMP_REDUCE) {
            mpi_errno = MPIR_Ireduce_sched_intra_smp(sendbuf, recvbuf, count,
                                                     datatype, op, root, comm_ptr, s);
        } else {
            /* intracommunicator */
            switch (MPIR_CVAR_IREDUCE_INTRA_ALGORITHM) {
                case MPIR_CVAR_IREDUCE_INTRA_ALGORITHM_binomial:
                    mpi_errno = MPIR_Ireduce_sched_intra_binomial(sendbuf, recvbuf, count,
                                                                  datatype, op, root, comm_ptr, s);
                    break;
                case MPIR_CVAR_IREDUCE_INTRA_ALGORITHM_reduce_scatter_gather:
                    mpi_errno =
                        MPIR_Ireduce_sched_intra_reduce_scatter_gather(sendbuf, recvbuf, count,
                                                                       datatype, op, root, comm_ptr,
                                                                       s);
                    break;
                case MPIR_CVAR_IREDUCE_INTRA_ALGORITHM_auto:
                    MPL_FALLTHROUGH;
                default:
                    mpi_errno = MPIR_Ireduce_sched_intra_auto(sendbuf, recvbuf, count,
                                                              datatype, op, root, comm_ptr, s);
                    break;
            }
        }
    } else {
        /* intercommunicator */
        switch (MPIR_CVAR_IREDUCE_INTER_ALGORITHM) {
            case MPIR_CVAR_IREDUCE_INTER_ALGORITHM_local_reduce_remote_send:
                mpi_errno =
                    MPIR_Ireduce_sched_inter_local_reduce_remote_send(sendbuf, recvbuf, count,
                                                                      datatype, op, root, comm_ptr,
                                                                      s);
                break;
            case MPIR_CVAR_IREDUCE_INTER_ALGORITHM_auto:
                MPL_FALLTHROUGH;
            default:
                mpi_errno = MPIR_Ireduce_sched_inter_auto(sendbuf, recvbuf, count,
                                                          datatype, op, root, comm_ptr, s);
                break;
        }
    }

    return mpi_errno;
}

int MPIR_Ireduce_sched(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype,
                       MPI_Op op, int root, MPIR_Comm * comm_ptr, MPIR_Sched_t s)
{
    int mpi_errno = MPI_SUCCESS;

    if (MPIR_CVAR_IREDUCE_DEVICE_COLLECTIVE && MPIR_CVAR_DEVICE_COLLECTIVES) {
        mpi_errno = MPID_Ireduce_sched(sendbuf, recvbuf, count, datatype, op, root, comm_ptr, s);
    } else {
        mpi_errno = MPIR_Ireduce_sched_impl(sendbuf, recvbuf, count, datatype, op, root, comm_ptr,
                                            s);
    }

    return mpi_errno;
}

int MPIR_Ireduce_impl(const void *sendbuf, void *recvbuf, int count,
                      MPI_Datatype datatype, MPI_Op op, int root,
                      MPIR_Comm * comm_ptr, MPIR_Request ** request)
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
        switch (MPIR_CVAR_IREDUCE_INTRA_ALGORITHM) {
            case MPIR_CVAR_IREDUCE_INTRA_ALGORITHM_gentran_tree:
                mpi_errno =
                    MPIR_Ireduce_intra_gentran_tree(sendbuf, recvbuf, count, datatype, op, root,
                                                    comm_ptr, request);
                MPIR_ERR_CHECK(mpi_errno);
                goto fn_exit;
                break;
            case MPIR_CVAR_IREDUCE_INTRA_ALGORITHM_gentran_ring:
                mpi_errno =
                    MPIR_Ireduce_intra_gentran_ring(sendbuf, recvbuf, count, datatype, op, root,
                                                    comm_ptr, request);
                MPIR_ERR_CHECK(mpi_errno);
                goto fn_exit;
                break;
            default:
                /* go down to the MPIR_Sched-based algorithms */
                break;
        }
    }

    /* If the user doesn't pick a transport-enabled algorithm, go to the old
     * sched function. */
    mpi_errno = MPIR_Sched_next_tag(comm_ptr, &tag);
    MPIR_ERR_CHECK(mpi_errno);
    mpi_errno = MPIR_Sched_create(&s);
    MPIR_ERR_CHECK(mpi_errno);

    mpi_errno = MPIR_Ireduce_sched(sendbuf, recvbuf, count, datatype, op, root, comm_ptr, s);
    MPIR_ERR_CHECK(mpi_errno);

    mpi_errno = MPIR_Sched_start(&s, comm_ptr, tag, request);
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIR_Ireduce(const void *sendbuf, void *recvbuf, int count,
                 MPI_Datatype datatype, MPI_Op op, int root,
                 MPIR_Comm * comm_ptr, MPIR_Request ** request)
{
    int mpi_errno = MPI_SUCCESS;

    if (MPIR_CVAR_IREDUCE_DEVICE_COLLECTIVE && MPIR_CVAR_DEVICE_COLLECTIVES) {
        mpi_errno = MPID_Ireduce(sendbuf, recvbuf, count, datatype, op, root, comm_ptr, request);
    } else {
        mpi_errno = MPIR_Ireduce_impl(sendbuf, recvbuf, count, datatype, op, root, comm_ptr,
                                      request);
    }

    return mpi_errno;
}

#endif /* MPICH_MPI_FROM_PMPI */

/*@
MPI_Ireduce - Reduces values on all processes to a single value
              in a nonblocking way

Input Parameters:
+ sendbuf - address of the send buffer (choice)
. count - number of elements in send buffer (non-negative integer)
. datatype - data type of elements of send buffer (handle)
. op - reduce operation (handle)
. root - rank of root process (integer)
- comm - communicator (handle)

Output Parameters:
+ recvbuf - address of the receive buffer (significant only at root) (choice)
- request - communication request (handle)

.N ThreadSafe

.N Fortran

.N Errors
@*/
int MPI_Ireduce(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype,
                MPI_Op op, int root, MPI_Comm comm, MPI_Request * request)
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
            MPIR_ERRTEST_COUNT(count, mpi_errno);
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
            int rank;

            MPIR_Comm_valid_ptr(comm_ptr, mpi_errno, FALSE);
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

            if (comm_ptr->comm_kind == MPIR_COMM_KIND__INTRACOMM) {
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

            /* TODO more checks may be appropriate (counts, in_place, etc) */
        }
        MPID_END_ERROR_CHECKS;
    }
#endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */

    mpi_errno = MPIR_Ireduce(sendbuf, recvbuf, count, datatype, op, root, comm_ptr, &request_ptr);
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
                                 "**mpi_ireduce", "**mpi_ireduce %p %p %d %D %O %d %C %p", sendbuf,
                                 recvbuf, count, datatype, op, root, comm, request);
    }
#endif
    mpi_errno = MPIR_Err_return_comm(comm_ptr, __func__, mpi_errno);
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}
