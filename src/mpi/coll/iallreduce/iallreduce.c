/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"

/*
=== BEGIN_MPI_T_CVAR_INFO_BLOCK ===

cvars:
    - name        : MPIR_CVAR_IALLREDUCE_TREE_KVAL
      category    : COLLECTIVE
      type        : int
      default     : 2
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        k value for tree based iallreduce (for tree_kary and tree_knomial)

    - name        : MPIR_CVAR_IALLREDUCE_TREE_TYPE
      category    : COLLECTIVE
      type        : string
      default     : kary
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        Tree type for tree based ibcast
        kary      - kary tree type
        knomial_1 - knomial_1 tree type
        knomial_2 - knomial_2 tree type

    - name        : MPIR_CVAR_IALLREDUCE_TREE_PIPELINE_CHUNK_SIZE
      category    : COLLECTIVE
      type        : int
      default     : 0
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        Maximum chunk size (in bytes) for pipelining in tree based
        iallreduce. Default value is 0, that is, no pipelining by default

    - name        : MPIR_CVAR_IALLREDUCE_TREE_BUFFER_PER_CHILD
      category    : COLLECTIVE
      type        : boolean
      default     : 0
      class       : none
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
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        k value for recursive exchange based iallreduce

    - name        : MPIR_CVAR_IALLREDUCE_INTRA_ALGORITHM
      category    : COLLECTIVE
      type        : enum
      default     : auto
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : |-
        Variable to select iallreduce algorithm
        auto - Internal algorithm selection (can be overridden with MPIR_CVAR_COLL_SELECTION_TUNING_JSON_FILE)
        sched_auto - Internal algorithm selection for sched-based algorithms
        sched_naive                      - Force naive algorithm
        sched_smp                        - Force smp algorithm
        sched_recursive_doubling         - Force recursive doubling algorithm
        sched_reduce_scatter_allgather   - Force reduce scatter allgather algorithm
        gentran_recexch_single_buffer    - Force generic transport recursive exchange with single buffer for receives
        gentran_recexch_multiple_buffer  - Force generic transport recursive exchange with multiple buffers for receives
        gentran_tree                     - Force generic transport tree algorithm
        gentran_ring                     - Force generic transport ring algorithm
        gentran_recexch_reduce_scatter_recexch_allgatherv  - Force generic transport recursive exchange with reduce scatter and allgatherv

    - name        : MPIR_CVAR_IALLREDUCE_INTER_ALGORITHM
      category    : COLLECTIVE
      type        : enum
      default     : auto
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : |-
        Variable to select iallreduce algorithm
        auto - Internal algorithm selection (can be overridden with MPIR_CVAR_COLL_SELECTION_TUNING_JSON_FILE)
        sched_auto - Internal algorithm selection for sched-based algorithms
        sched_remote_reduce_local_bcast - Force remote-reduce-local-bcast algorithm

    - name        : MPIR_CVAR_IALLREDUCE_DEVICE_COLLECTIVE
      category    : COLLECTIVE
      type        : boolean
      default     : true
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        This CVAR is only used when MPIR_CVAR_DEVICE_COLLECTIVES
        is set to "percoll".  If set to true, MPI_Iallreduce will
        allow the device to override the MPIR-level collective
        algorithms.  The device might still call the MPIR-level
        algorithms manually.  If set to false, the device-override
        will be disabled.

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


int MPIR_Iallreduce_allcomm_auto(const void *sendbuf, void *recvbuf, int count,
                                 MPI_Datatype datatype, MPI_Op op, MPIR_Comm * comm_ptr,
                                 MPIR_Request ** request)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_Csel_coll_sig_s coll_sig = {
        .coll_type = MPIR_CSEL_COLL_TYPE__IALLREDUCE,
        .comm_ptr = comm_ptr,

        .u.iallreduce.sendbuf = sendbuf,
        .u.iallreduce.recvbuf = recvbuf,
        .u.iallreduce.count = count,
        .u.iallreduce.datatype = datatype,
        .u.iallreduce.op = op,
    };

    MPII_Csel_container_s *cnt = MPIR_Csel_search(comm_ptr->csel_comm, coll_sig);
    MPIR_Assert(cnt);

    switch (cnt->id) {
        case MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Iallreduce_intra_sched_auto:
            MPII_SCHED_WRAPPER(MPIR_Iallreduce_intra_sched_auto, comm_ptr, request, sendbuf,
                               recvbuf, count, datatype, op);
            break;

        case MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Iallreduce_intra_sched_naive:
            MPII_SCHED_WRAPPER(MPIR_Iallreduce_intra_sched_naive, comm_ptr, request, sendbuf,
                               recvbuf, count, datatype, op);
            break;

        case MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Iallreduce_intra_sched_recursive_doubling:
            MPII_SCHED_WRAPPER(MPIR_Iallreduce_intra_sched_recursive_doubling, comm_ptr, request,
                               sendbuf, recvbuf, count, datatype, op);
            break;

        case MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Iallreduce_intra_sched_reduce_scatter_allgather:
            MPII_SCHED_WRAPPER(MPIR_Iallreduce_intra_sched_reduce_scatter_allgather, comm_ptr,
                               request, sendbuf, recvbuf, count, datatype, op);
            break;

        case MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Iallreduce_intra_gentran_recexch_single_buffer:
            mpi_errno =
                MPIR_Iallreduce_intra_gentran_recexch_single_buffer(sendbuf, recvbuf, count,
                                                                    datatype, op, comm_ptr,
                                                                    cnt->u.
                                                                    iallreduce.intra_gentran_recexch_single_buffer.k,
                                                                    request);
            break;

        case MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Iallreduce_intra_gentran_recexch_multiple_buffer:
            mpi_errno =
                MPIR_Iallreduce_intra_gentran_recexch_multiple_buffer(sendbuf, recvbuf, count,
                                                                      datatype, op, comm_ptr,
                                                                      cnt->u.
                                                                      iallreduce.intra_gentran_recexch_multiple_buffer.k,
                                                                      request);
            break;

        case MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Iallreduce_intra_gentran_tree:
            mpi_errno =
                MPIR_Iallreduce_intra_gentran_tree(sendbuf, recvbuf, count, datatype, op, comm_ptr,
                                                   cnt->u.iallreduce.intra_gentran_tree.tree_type,
                                                   cnt->u.iallreduce.intra_gentran_tree.k,
                                                   cnt->u.iallreduce.intra_gentran_tree.chunk_size,
                                                   cnt->u.iallreduce.
                                                   intra_gentran_tree.buffer_per_child, request);
            break;

        case MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Iallreduce_intra_gentran_ring:
            mpi_errno =
                MPIR_Iallreduce_intra_gentran_ring(sendbuf, recvbuf, count, datatype, op, comm_ptr,
                                                   request);
            break;

        case MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Iallreduce_intra_gentran_recexch_reduce_scatter_recexch_allgatherv:
            mpi_errno =
                MPIR_Iallreduce_intra_gentran_recexch_reduce_scatter_recexch_allgatherv(sendbuf,
                                                                                        recvbuf,
                                                                                        count,
                                                                                        datatype,
                                                                                        op,
                                                                                        comm_ptr,
                                                                                        cnt->
                                                                                        u.iallreduce.intra_gentran_recexch_reduce_scatter_recexch_allgatherv.k,
                                                                                        request);
            break;

        case MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Iallreduce_intra_sched_smp:
            MPII_SCHED_WRAPPER(MPIR_Iallreduce_intra_sched_smp, comm_ptr, request, sendbuf, recvbuf,
                               count, datatype, op);
            break;

        case MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Iallreduce_inter_sched_auto:
            MPII_SCHED_WRAPPER(MPIR_Iallreduce_inter_sched_auto, comm_ptr, request, sendbuf,
                               recvbuf, count, datatype, op);
            break;

        case MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Iallreduce_inter_sched_remote_reduce_local_bcast:
            MPII_SCHED_WRAPPER(MPIR_Iallreduce_inter_sched_remote_reduce_local_bcast, comm_ptr,
                               request, sendbuf, recvbuf, count, datatype, op);
            break;

        default:
            MPIR_Assert(0);
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIR_Iallreduce_intra_sched_auto(const void *sendbuf, void *recvbuf, int count,
                                     MPI_Datatype datatype, MPI_Op op, MPIR_Comm * comm_ptr,
                                     MPIR_Sched_t s)
{
    int mpi_errno = MPI_SUCCESS;
    int pof2, type_size;

    MPIR_Assert(comm_ptr->comm_kind == MPIR_COMM_KIND__INTRACOMM);

    if (comm_ptr->hierarchy_kind == MPIR_COMM_HIERARCHY_KIND__PARENT && MPIR_Op_is_commutative(op)) {
        mpi_errno =
            MPIR_Iallreduce_intra_sched_smp(sendbuf, recvbuf, count, datatype, op, comm_ptr, s);
        if (mpi_errno)
            MPIR_ERR_POP(mpi_errno);

        goto fn_exit;
    }

    MPIR_Datatype_get_size_macro(datatype, type_size);

    /* get nearest power-of-two less than or equal to number of ranks in the communicator */
    pof2 = comm_ptr->coll.pof2;

    /* If op is user-defined or count is less than pof2, use
     * recursive doubling algorithm. Otherwise do a reduce-scatter
     * followed by allgather. (If op is user-defined,
     * derived datatypes are allowed and the user could pass basic
     * datatypes on one process and derived on another as long as
     * the type maps are the same. Breaking up derived
     * datatypes to do the reduce-scatter is tricky, therefore
     * using recursive doubling in that case.) */

    if ((count * type_size <= MPIR_CVAR_ALLREDUCE_SHORT_MSG_SIZE) ||
        (!HANDLE_IS_BUILTIN(op)) || (count < pof2)) {
        /* use recursive doubling */
        mpi_errno =
            MPIR_Iallreduce_intra_sched_recursive_doubling(sendbuf, recvbuf, count, datatype, op,
                                                           comm_ptr, s);
        MPIR_ERR_CHECK(mpi_errno);
    } else {
        /* do a reduce-scatter followed by allgather */
        mpi_errno =
            MPIR_Iallreduce_intra_sched_reduce_scatter_allgather(sendbuf, recvbuf, count, datatype,
                                                                 op, comm_ptr, s);
        MPIR_ERR_CHECK(mpi_errno);
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIR_Iallreduce_inter_sched_auto(const void *sendbuf, void *recvbuf, int count,
                                     MPI_Datatype datatype, MPI_Op op, MPIR_Comm * comm_ptr,
                                     MPIR_Sched_t s)
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = MPIR_Iallreduce_inter_sched_remote_reduce_local_bcast(sendbuf, recvbuf, count,
                                                                      datatype, op, comm_ptr, s);

    return mpi_errno;
}


int MPIR_Iallreduce_sched_auto(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype,
                               MPI_Op op, MPIR_Comm * comm_ptr, MPIR_Sched_t s)
{
    int mpi_errno = MPI_SUCCESS;

    if (comm_ptr->comm_kind == MPIR_COMM_KIND__INTRACOMM) {
        mpi_errno =
            MPIR_Iallreduce_intra_sched_auto(sendbuf, recvbuf, count, datatype, op, comm_ptr, s);
    } else {
        mpi_errno =
            MPIR_Iallreduce_inter_sched_auto(sendbuf, recvbuf, count, datatype, op, comm_ptr, s);
    }

    return mpi_errno;
}

int MPIR_Iallreduce_impl(const void *sendbuf, void *recvbuf, int count,
                         MPI_Datatype datatype, MPI_Op op, MPIR_Comm * comm_ptr,
                         MPIR_Request ** request)
{
    int mpi_errno = MPI_SUCCESS;
    int is_commutative = MPIR_Op_is_commutative(op);
    int nranks = comm_ptr->local_size;

    *request = NULL;
    /* If the user picks one of the transport-enabled algorithms, branch there
     * before going down to the MPIR_Sched-based algorithms. */
    /* TODO - Eventually the intention is to replace all of the
     * MPIR_Sched-based algorithms with transport-enabled algorithms, but that
     * will require sufficient performance testing and replacement algorithms. */
    if (comm_ptr->comm_kind == MPIR_COMM_KIND__INTRACOMM) {
        /* intracommunicator */
        switch (MPIR_CVAR_IALLREDUCE_INTRA_ALGORITHM) {
            case MPIR_CVAR_IALLREDUCE_INTRA_ALGORITHM_gentran_recexch_single_buffer:
                mpi_errno =
                    MPIR_Iallreduce_intra_gentran_recexch_single_buffer(sendbuf, recvbuf, count,
                                                                        datatype, op, comm_ptr,
                                                                        MPIR_CVAR_IALLREDUCE_RECEXCH_KVAL,
                                                                        request);
                break;

            case MPIR_CVAR_IALLREDUCE_INTRA_ALGORITHM_gentran_recexch_multiple_buffer:
                mpi_errno =
                    MPIR_Iallreduce_intra_gentran_recexch_multiple_buffer(sendbuf, recvbuf, count,
                                                                          datatype, op, comm_ptr,
                                                                          MPIR_CVAR_IALLREDUCE_RECEXCH_KVAL,
                                                                          request);
                break;

            case MPIR_CVAR_IALLREDUCE_INTRA_ALGORITHM_gentran_tree:
                mpi_errno =
                    MPIR_Iallreduce_intra_gentran_tree(sendbuf, recvbuf, count, datatype,
                                                       op, comm_ptr, MPIR_Iallreduce_tree_type,
                                                       MPIR_CVAR_IALLREDUCE_TREE_KVAL,
                                                       MPIR_CVAR_IALLREDUCE_TREE_PIPELINE_CHUNK_SIZE,
                                                       MPIR_CVAR_IALLREDUCE_TREE_BUFFER_PER_CHILD,
                                                       request);
                break;

            case MPIR_CVAR_IALLREDUCE_INTRA_ALGORITHM_gentran_ring:
                MPII_COLLECTIVE_FALLBACK_CHECK(comm_ptr->rank, is_commutative, mpi_errno,
                                               "Iallreduce gentran_ring cannot be applied.\n");
                mpi_errno =
                    MPIR_Iallreduce_intra_gentran_ring(sendbuf, recvbuf, count, datatype,
                                                       op, comm_ptr, request);
                break;

            case MPIR_CVAR_IALLREDUCE_INTRA_ALGORITHM_gentran_recexch_reduce_scatter_recexch_allgatherv:
                MPII_COLLECTIVE_FALLBACK_CHECK(comm_ptr->rank, is_commutative &&
                                               count >= nranks, mpi_errno,
                                               "Iallreduce gentran_recexch_reduce_scatter_recexch_allgatherv cannot be applied.\n");
                /* This algorithm will work for commutative
                 * operations and if the count is bigger than total
                 * number of ranks. If it not commutative or if the
                 * count < nranks, MPIR_Iallreduce_sched algorithm
                 * will be run */
                mpi_errno =
                    MPIR_Iallreduce_intra_gentran_recexch_reduce_scatter_recexch_allgatherv
                    (sendbuf, recvbuf, count, datatype, op, comm_ptr,
                     MPIR_CVAR_IALLREDUCE_RECEXCH_KVAL, request);
                break;

            case MPIR_CVAR_IALLREDUCE_INTRA_ALGORITHM_sched_naive:
                MPII_SCHED_WRAPPER(MPIR_Iallreduce_intra_sched_naive, comm_ptr, request, sendbuf,
                                   recvbuf, count, datatype, op);
                break;

            case MPIR_CVAR_IALLREDUCE_INTRA_ALGORITHM_sched_recursive_doubling:
                MPII_SCHED_WRAPPER(MPIR_Iallreduce_intra_sched_recursive_doubling, comm_ptr,
                                   request, sendbuf, recvbuf, count, datatype, op);
                break;

            case MPIR_CVAR_IALLREDUCE_INTRA_ALGORITHM_sched_reduce_scatter_allgather:
                MPII_SCHED_WRAPPER(MPIR_Iallreduce_intra_sched_reduce_scatter_allgather, comm_ptr,
                                   request, sendbuf, recvbuf, count, datatype, op);
                break;

            case MPIR_CVAR_IALLREDUCE_INTRA_ALGORITHM_sched_smp:
                MPII_SCHED_WRAPPER(MPIR_Iallreduce_intra_sched_smp, comm_ptr,
                                   request, sendbuf, recvbuf, count, datatype, op);
                break;

            case MPIR_CVAR_IALLREDUCE_INTRA_ALGORITHM_sched_auto:
                MPII_SCHED_WRAPPER(MPIR_Iallreduce_intra_sched_auto, comm_ptr, request, sendbuf,
                                   recvbuf, count, datatype, op);
                break;

            case MPIR_CVAR_IALLREDUCE_INTRA_ALGORITHM_auto:
                mpi_errno =
                    MPIR_Iallreduce_allcomm_auto(sendbuf, recvbuf, count, datatype, op, comm_ptr,
                                                 request);
                break;

            default:
                MPIR_Assert(0);
        }
    } else {
        switch (MPIR_CVAR_IALLREDUCE_INTER_ALGORITHM) {
            case MPIR_CVAR_IALLREDUCE_INTER_ALGORITHM_sched_remote_reduce_local_bcast:
                MPII_SCHED_WRAPPER(MPIR_Iallreduce_inter_sched_remote_reduce_local_bcast, comm_ptr,
                                   request, sendbuf, recvbuf, count, datatype, op);
                break;

            case MPIR_CVAR_IALLREDUCE_INTER_ALGORITHM_sched_auto:
                MPII_SCHED_WRAPPER(MPIR_Iallreduce_inter_sched_auto, comm_ptr, request, sendbuf,
                                   recvbuf, count, datatype, op);
                break;

            case MPIR_CVAR_IALLREDUCE_INTER_ALGORITHM_auto:
                mpi_errno =
                    MPIR_Iallreduce_allcomm_auto(sendbuf, recvbuf, count, datatype, op, comm_ptr,
                                                 request);
                break;

            default:
                MPIR_Assert(0);
        }
    }

    MPIR_ERR_CHECK(mpi_errno);
    goto fn_exit;

  fallback:
    if (comm_ptr->comm_kind == MPIR_COMM_KIND__INTRACOMM) {
        MPII_SCHED_WRAPPER(MPIR_Iallreduce_intra_sched_auto, comm_ptr, request, sendbuf,
                           recvbuf, count, datatype, op);
    } else {
        MPII_SCHED_WRAPPER(MPIR_Iallreduce_inter_sched_auto, comm_ptr, request, sendbuf,
                           recvbuf, count, datatype, op);
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIR_Iallreduce(const void *sendbuf, void *recvbuf, int count,
                    MPI_Datatype datatype, MPI_Op op, MPIR_Comm * comm_ptr, MPIR_Request ** request)
{
    int mpi_errno = MPI_SUCCESS;
    void *in_recvbuf = recvbuf;
    void *host_sendbuf;
    void *host_recvbuf;

    MPIR_Coll_host_buffer_alloc(sendbuf, recvbuf, count, datatype, &host_sendbuf, &host_recvbuf);
    if (host_sendbuf)
        sendbuf = host_sendbuf;
    if (host_recvbuf)
        recvbuf = host_recvbuf;

    if ((MPIR_CVAR_DEVICE_COLLECTIVES == MPIR_CVAR_DEVICE_COLLECTIVES_all) ||
        ((MPIR_CVAR_DEVICE_COLLECTIVES == MPIR_CVAR_DEVICE_COLLECTIVES_percoll) &&
         MPIR_CVAR_IALLREDUCE_DEVICE_COLLECTIVE)) {
        mpi_errno = MPID_Iallreduce(sendbuf, recvbuf, count, datatype, op, comm_ptr, request);
    } else {
        mpi_errno = MPIR_Iallreduce_impl(sendbuf, recvbuf, count, datatype, op, comm_ptr, request);
    }

    MPIR_Coll_host_buffer_swap_back(host_sendbuf, host_recvbuf, in_recvbuf, count, datatype,
                                    *request);

    return mpi_errno;
}

#endif /* MPICH_MPI_FROM_PMPI */

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
    MPIR_ERR_CHECK(mpi_errno);

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
            MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, __func__, __LINE__, MPI_ERR_OTHER,
                                 "**mpi_iallreduce", "**mpi_iallreduce %p %p %d %D %O %C %p",
                                 sendbuf, recvbuf, count, datatype, op, comm, request);
    }
#endif
    mpi_errno = MPIR_Err_return_comm(comm_ptr, __func__, mpi_errno);
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}
