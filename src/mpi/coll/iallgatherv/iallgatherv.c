/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"
#include "iallgatherv.h"

/*
=== BEGIN_MPI_T_CVAR_INFO_BLOCK ===

cvars:
    - name        : MPIR_CVAR_IALLGATHERV_RECEXCH_KVAL
      category    : COLLECTIVE
      type        : int
      default     : 2
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        k value for recursive exchange based iallgatherv

    - name        : MPIR_CVAR_IALLGATHERV_BRUCKS_KVAL
      category    : COLLECTIVE
      type        : int
      default     : 2
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        k value for radix in brucks based iallgatherv

    - name        : MPIR_CVAR_IALLGATHERV_INTRA_ALGORITHM
      category    : COLLECTIVE
      type        : enum
      default     : auto
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : |-
        Variable to select iallgatherv algorithm
        auto - Internal algorithm selection (can be overridden with MPIR_CVAR_COLL_SELECTION_TUNING_JSON_FILE)
        sched_auto - Internal algorithm selection for sched-based algorithms
        sched_brucks             - Force brucks algorithm
        sched_recursive_doubling - Force recursive doubling algorithm
        sched_ring               - Force ring algorithm
        gentran_recexch_doubling - Force generic transport recursive exchange with neighbours doubling in distance in each phase
        gentran_recexch_halving  - Force generic transport recursive exchange with neighbours halving in distance in each phase
        gentran_ring             - Force generic transport ring algorithm
        gentran_brucks           - Force generic transport based brucks algorithm

    - name        : MPIR_CVAR_IALLGATHERV_INTER_ALGORITHM
      category    : COLLECTIVE
      type        : enum
      default     : auto
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : |-
        Variable to select iallgatherv algorithm
        auto - Internal algorithm selection (can be overridden with MPIR_CVAR_COLL_SELECTION_TUNING_JSON_FILE)
        sched_auto - Internal algorithm selection for sched-based algorithms
        sched_remote_gather_local_bcast - Force remote-gather-local-bcast algorithm

    - name        : MPIR_CVAR_IALLGATHERV_DEVICE_COLLECTIVE
      category    : COLLECTIVE
      type        : boolean
      default     : true
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        This CVAR is only used when MPIR_CVAR_DEVICE_COLLECTIVES
        is set to "percoll".  If set to true, MPI_Iallgatherv will
        allow the device to override the MPIR-level collective
        algorithms.  The device might still call the MPIR-level
        algorithms manually.  If set to false, the device-override
        will be disabled.

=== END_MPI_T_CVAR_INFO_BLOCK ===
*/

/* -- Begin Profiling Symbol Block for routine MPI_Iallgatherv */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Iallgatherv = PMPI_Iallgatherv
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Iallgatherv  MPI_Iallgatherv
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Iallgatherv as PMPI_Iallgatherv
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPI_Iallgatherv(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf,
                    const int recvcounts[], const int displs[], MPI_Datatype recvtype,
                    MPI_Comm comm, MPI_Request * request)
    __attribute__ ((weak, alias("PMPI_Iallgatherv")));
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Iallgatherv
#define MPI_Iallgatherv PMPI_Iallgatherv

/* This is the machine-independent implementation of allgatherv. The algorithm is:

   Algorithm: MPI_Allgatherv

   For short messages and non-power-of-two no. of processes, we use
   the algorithm from the Jehoshua Bruck et al IEEE TPDS Nov 97
   paper. It is a variant of the disemmination algorithm for
   barrier. It takes ceiling(lg p) steps.

   Cost = lgp.alpha + n.((p-1)/p).beta
   where n is total size of data gathered on each process.

   For short or medium-size messages and power-of-two no. of
   processes, we use the recursive doubling algorithm.

   Cost = lgp.alpha + n.((p-1)/p).beta

   TODO: On TCP, we may want to use recursive doubling instead of the Bruck
   algorithm in all cases because of the pairwise-exchange property of
   recursive doubling (see Benson et al paper in Euro PVM/MPI
   2003).

   For long messages or medium-size messages and non-power-of-two
   no. of processes, we use a ring algorithm. In the first step, each
   process i sends its contribution to process i+1 and receives
   the contribution from process i-1 (with wrap-around). From the
   second step onwards, each process i forwards to process i+1 the
   data it received from process i-1 in the previous step. This takes
   a total of p-1 steps.

   Cost = (p-1).alpha + n.((p-1)/p).beta

   Possible improvements:

   End Algorithm: MPI_Allgatherv
*/

int MPIR_Iallgatherv_allcomm_auto(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                                  void *recvbuf, const int *recvcounts, const int *displs,
                                  MPI_Datatype recvtype, MPIR_Comm * comm_ptr,
                                  MPIR_Request ** request)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_Csel_coll_sig_s coll_sig = {
        .coll_type = MPIR_CSEL_COLL_TYPE__IALLGATHERV,
        .comm_ptr = comm_ptr,

        .u.iallgatherv.sendbuf = sendbuf,
        .u.iallgatherv.sendcount = sendcount,
        .u.iallgatherv.sendtype = sendtype,
        .u.iallgatherv.recvbuf = recvbuf,
        .u.iallgatherv.recvcounts = recvcounts,
        .u.iallgatherv.displs = displs,
        .u.iallgatherv.recvtype = recvtype,
    };

    MPII_Csel_container_s *cnt = MPIR_Csel_search(comm_ptr->csel_comm, coll_sig);
    MPIR_Assert(cnt);

    switch (cnt->id) {
        case MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Iallgatherv_intra_gentran_brucks:
            mpi_errno =
                MPIR_Iallgatherv_intra_gentran_brucks(sendbuf, sendcount, sendtype, recvbuf,
                                                      recvcounts, displs, recvtype, comm_ptr,
                                                      cnt->u.iallgatherv.intra_gentran_brucks.k,
                                                      request);
            break;

        case MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Iallgatherv_intra_sched_auto:
            MPII_SCHED_WRAPPER(MPIR_Iallgatherv_intra_sched_auto, comm_ptr, request, sendbuf,
                               sendcount, sendtype, recvbuf, recvcounts, displs, recvtype);
            break;

        case MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Iallgatherv_intra_sched_brucks:
            MPII_SCHED_WRAPPER(MPIR_Iallgatherv_intra_sched_brucks, comm_ptr, request, sendbuf,
                               sendcount, sendtype, recvbuf, recvcounts, displs, recvtype);
            break;

        case MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Iallgatherv_intra_sched_recursive_doubling:
            MPII_SCHED_WRAPPER(MPIR_Iallgatherv_intra_sched_recursive_doubling, comm_ptr, request,
                               sendbuf, sendcount, sendtype, recvbuf, recvcounts, displs, recvtype);
            break;

        case MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Iallgatherv_intra_sched_ring:
            MPII_SCHED_WRAPPER(MPIR_Iallgatherv_intra_sched_ring, comm_ptr, request, sendbuf,
                               sendcount, sendtype, recvbuf, recvcounts, displs, recvtype);
            break;

        case MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Iallgatherv_intra_gentran_recexch_doubling:
            mpi_errno =
                MPIR_Iallgatherv_intra_gentran_recexch_doubling(sendbuf, sendcount, sendtype,
                                                                recvbuf, recvcounts, displs,
                                                                recvtype, comm_ptr,
                                                                cnt->u.
                                                                iallgatherv.intra_gentran_recexch_doubling.
                                                                k, request);
            break;

        case MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Iallgatherv_intra_gentran_recexch_halving:
            mpi_errno =
                MPIR_Iallgatherv_intra_gentran_recexch_halving(sendbuf, sendcount, sendtype,
                                                               recvbuf, recvcounts, displs,
                                                               recvtype, comm_ptr,
                                                               cnt->u.
                                                               iallgatherv.intra_gentran_recexch_halving.
                                                               k, request);
            break;

        case MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Iallgatherv_intra_gentran_ring:
            mpi_errno =
                MPIR_Iallgatherv_intra_gentran_ring(sendbuf, sendcount, sendtype, recvbuf,
                                                    recvcounts, displs, recvtype, comm_ptr,
                                                    request);
            break;

        case MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Iallgatherv_inter_sched_auto:
            MPII_SCHED_WRAPPER(MPIR_Iallgatherv_inter_sched_auto, comm_ptr, request, sendbuf,
                               sendcount, sendtype, recvbuf, recvcounts, displs, recvtype);
            break;

        case MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Iallgatherv_inter_sched_remote_gather_local_bcast:
            MPII_SCHED_WRAPPER(MPIR_Iallgatherv_inter_sched_remote_gather_local_bcast, comm_ptr,
                               request, sendbuf, sendcount, sendtype, recvbuf, recvcounts, displs,
                               recvtype);
            break;

        default:
            MPIR_Assert(0);
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIR_Iallgatherv_intra_sched_auto(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                                      void *recvbuf, const int recvcounts[], const int displs[],
                                      MPI_Datatype recvtype, MPIR_Comm * comm_ptr, MPIR_Sched_t s)
{
    int mpi_errno = MPI_SUCCESS;
    int i, comm_size, total_count, recvtype_size;

    comm_size = comm_ptr->local_size;
    MPIR_Datatype_get_size_macro(recvtype, recvtype_size);

    total_count = 0;
    for (i = 0; i < comm_size; i++)
        total_count += recvcounts[i];

    if (total_count == 0)
        goto fn_exit;

    if ((total_count * recvtype_size < MPIR_CVAR_ALLGATHER_LONG_MSG_SIZE) &&
        !(comm_size & (comm_size - 1))) {
        /* Short or medium size message and power-of-two no. of processes. Use
         * recursive doubling algorithm */
        mpi_errno =
            MPIR_Iallgatherv_intra_sched_recursive_doubling(sendbuf, sendcount, sendtype, recvbuf,
                                                            recvcounts, displs, recvtype, comm_ptr,
                                                            s);
        MPIR_ERR_CHECK(mpi_errno);
    } else if (total_count * recvtype_size < MPIR_CVAR_ALLGATHER_SHORT_MSG_SIZE) {
        /* Short message and non-power-of-two no. of processes. Use
         * Bruck algorithm (see description above). */
        mpi_errno =
            MPIR_Iallgatherv_intra_sched_brucks(sendbuf, sendcount, sendtype, recvbuf, recvcounts,
                                                displs, recvtype, comm_ptr, s);
        MPIR_ERR_CHECK(mpi_errno);
    } else {
        /* long message or medium-size message and non-power-of-two
         * no. of processes. Use ring algorithm. */
        mpi_errno =
            MPIR_Iallgatherv_intra_sched_ring(sendbuf, sendcount, sendtype, recvbuf, recvcounts,
                                              displs, recvtype, comm_ptr, s);
        MPIR_ERR_CHECK(mpi_errno);
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIR_Iallgatherv_inter_sched_auto(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                                      void *recvbuf, const int recvcounts[], const int displs[],
                                      MPI_Datatype recvtype, MPIR_Comm * comm_ptr, MPIR_Sched_t s)
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = MPIR_Iallgatherv_inter_sched_remote_gather_local_bcast(sendbuf, sendcount,
                                                                       sendtype, recvbuf,
                                                                       recvcounts, displs, recvtype,
                                                                       comm_ptr, s);

    return mpi_errno;
}

int MPIR_Iallgatherv_sched_auto(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                                void *recvbuf, const int recvcounts[], const int displs[],
                                MPI_Datatype recvtype, MPIR_Comm * comm_ptr, MPIR_Sched_t s)
{
    int mpi_errno = MPI_SUCCESS;

    if (comm_ptr->comm_kind == MPIR_COMM_KIND__INTRACOMM) {
        mpi_errno = MPIR_Iallgatherv_intra_sched_auto(sendbuf, sendcount, sendtype,
                                                      recvbuf, recvcounts, displs, recvtype,
                                                      comm_ptr, s);
    } else {
        mpi_errno = MPIR_Iallgatherv_inter_sched_auto(sendbuf, sendcount, sendtype,
                                                      recvbuf, recvcounts, displs, recvtype,
                                                      comm_ptr, s);
    }

    return mpi_errno;
}

int MPIR_Iallgatherv_impl(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                          void *recvbuf, const int recvcounts[], const int displs[],
                          MPI_Datatype recvtype, MPIR_Comm * comm_ptr, MPIR_Request ** request)
{
    int mpi_errno = MPI_SUCCESS;

    *request = NULL;
    int comm_size = comm_ptr->local_size;
    /* If the user picks one of the transport-enabled algorithms, branch there
     * before going down to the MPIR_Sched-based algorithms. */
    /* TODO - Eventually the intention is to replace all of the
     * MPIR_Sched-based algorithms with transport-enabled algorithms, but that
     * will require sufficient performance testing and replacement algorithms. */
    if (comm_ptr->comm_kind == MPIR_COMM_KIND__INTRACOMM) {
        /* intracommunicator */
        switch (MPIR_CVAR_IALLGATHERV_INTRA_ALGORITHM) {
            case MPIR_CVAR_IALLGATHERV_INTRA_ALGORITHM_gentran_recexch_doubling:
                /* This algo cannot handle unordered data */
                MPII_COLLECTIVE_FALLBACK_CHECK(comm_ptr->rank,
                                               MPII_Iallgatherv_is_displs_ordered
                                               (comm_size, recvcounts, displs), mpi_errno,
                                               "Iallgatherv gentran_recexch_doubling cannot be applied.\n");
                mpi_errno =
                    MPIR_Iallgatherv_intra_gentran_recexch_doubling(sendbuf, sendcount, sendtype,
                                                                    recvbuf, recvcounts, displs,
                                                                    recvtype, comm_ptr,
                                                                    MPIR_CVAR_IALLGATHERV_RECEXCH_KVAL,
                                                                    request);
                break;

            case MPIR_CVAR_IALLGATHERV_INTRA_ALGORITHM_gentran_recexch_halving:
                /* This algo cannot handle unordered data */
                MPII_COLLECTIVE_FALLBACK_CHECK(comm_ptr->rank,
                                               MPII_Iallgatherv_is_displs_ordered
                                               (comm_size, recvcounts, displs), mpi_errno,
                                               "Iallgatherv gentran_recexch_halving cannot be applied.\n");
                mpi_errno =
                    MPIR_Iallgatherv_intra_gentran_recexch_halving(sendbuf, sendcount, sendtype,
                                                                   recvbuf, recvcounts, displs,
                                                                   recvtype, comm_ptr,
                                                                   MPIR_CVAR_IALLGATHERV_RECEXCH_KVAL,
                                                                   request);
                break;

            case MPIR_CVAR_IALLGATHERV_INTRA_ALGORITHM_gentran_ring:
                mpi_errno =
                    MPIR_Iallgatherv_intra_gentran_ring(sendbuf, sendcount, sendtype,
                                                        recvbuf, recvcounts, displs,
                                                        recvtype, comm_ptr, request);
                break;

            case MPIR_CVAR_IALLGATHERV_INTRA_ALGORITHM_gentran_brucks:
                mpi_errno =
                    MPIR_Iallgatherv_intra_gentran_brucks(sendbuf, sendcount, sendtype,
                                                          recvbuf, recvcounts, displs,
                                                          recvtype, comm_ptr,
                                                          MPIR_CVAR_IALLGATHERV_BRUCKS_KVAL,
                                                          request);
                break;

            case MPIR_CVAR_IALLGATHERV_INTRA_ALGORITHM_sched_brucks:
                MPII_SCHED_WRAPPER(MPIR_Iallgatherv_intra_sched_brucks, comm_ptr, request, sendbuf,
                                   sendcount, sendtype, recvbuf, recvcounts, displs, recvtype);
                break;

            case MPIR_CVAR_IALLGATHERV_INTRA_ALGORITHM_sched_recursive_doubling:
                MPII_SCHED_WRAPPER(MPIR_Iallgatherv_intra_sched_recursive_doubling, comm_ptr,
                                   request, sendbuf, sendcount, sendtype, recvbuf, recvcounts,
                                   displs, recvtype);
                break;

            case MPIR_CVAR_IALLGATHERV_INTRA_ALGORITHM_sched_ring:
                MPII_SCHED_WRAPPER(MPIR_Iallgatherv_intra_sched_ring, comm_ptr, request, sendbuf,
                                   sendcount, sendtype, recvbuf, recvcounts, displs, recvtype);
                break;

            case MPIR_CVAR_IALLGATHERV_INTRA_ALGORITHM_sched_auto:
                MPII_SCHED_WRAPPER(MPIR_Iallgatherv_intra_sched_auto, comm_ptr, request, sendbuf,
                                   sendcount, sendtype, recvbuf, recvcounts, displs, recvtype);
                break;

            case MPIR_CVAR_IALLGATHERV_INTRA_ALGORITHM_auto:
                mpi_errno =
                    MPIR_Iallgatherv_allcomm_auto(sendbuf, sendcount, sendtype, recvbuf, recvcounts,
                                                  displs, recvtype, comm_ptr, request);
                break;

            default:
                MPIR_Assert(0);
        }
    } else {
        switch (MPIR_CVAR_IALLGATHERV_INTER_ALGORITHM) {
            case MPIR_CVAR_IALLGATHERV_INTER_ALGORITHM_sched_remote_gather_local_bcast:
                MPII_SCHED_WRAPPER(MPIR_Iallgatherv_inter_sched_remote_gather_local_bcast, comm_ptr,
                                   request, sendbuf, sendcount, sendtype, recvbuf, recvcounts,
                                   displs, recvtype);
                break;

            case MPIR_CVAR_IALLGATHERV_INTER_ALGORITHM_sched_auto:
                MPII_SCHED_WRAPPER(MPIR_Iallgatherv_inter_sched_auto, comm_ptr, request, sendbuf,
                                   sendcount, sendtype, recvbuf, recvcounts, displs, recvtype);
                break;

            case MPIR_CVAR_IALLGATHERV_INTER_ALGORITHM_auto:
                mpi_errno =
                    MPIR_Iallgatherv_allcomm_auto(sendbuf, sendcount, sendtype, recvbuf, recvcounts,
                                                  displs, recvtype, comm_ptr, request);
                break;

            default:
                MPIR_Assert(0);
        }
    }

    MPIR_ERR_CHECK(mpi_errno);
    goto fn_exit;

  fallback:
    if (comm_ptr->comm_kind == MPIR_COMM_KIND__INTRACOMM) {
        MPII_SCHED_WRAPPER(MPIR_Iallgatherv_intra_sched_auto, comm_ptr, request, sendbuf,
                           sendcount, sendtype, recvbuf, recvcounts, displs, recvtype);
    } else {
        MPII_SCHED_WRAPPER(MPIR_Iallgatherv_inter_sched_auto, comm_ptr, request, sendbuf,
                           sendcount, sendtype, recvbuf, recvcounts, displs, recvtype);
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIR_Iallgatherv(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                     void *recvbuf, const int recvcounts[], const int displs[],
                     MPI_Datatype recvtype, MPIR_Comm * comm_ptr, MPIR_Request ** request)
{
    int mpi_errno = MPI_SUCCESS;

    if ((MPIR_CVAR_DEVICE_COLLECTIVES == MPIR_CVAR_DEVICE_COLLECTIVES_all) ||
        ((MPIR_CVAR_DEVICE_COLLECTIVES == MPIR_CVAR_DEVICE_COLLECTIVES_percoll) &&
         MPIR_CVAR_IALLGATHERV_DEVICE_COLLECTIVE)) {
        mpi_errno =
            MPID_Iallgatherv(sendbuf, sendcount, sendtype, recvbuf, recvcounts, displs, recvtype,
                             comm_ptr, request);
    } else {
        mpi_errno = MPIR_Iallgatherv_impl(sendbuf, sendcount, sendtype, recvbuf, recvcounts,
                                          displs, recvtype, comm_ptr, request);
    }

    return mpi_errno;
}

#endif /* MPICH_MPI_FROM_PMPI */

/*@
MPI_Iallgatherv - Gathers data from all tasks and deliver the combined data
                  to all tasks in a nonblocking way

Input Parameters:
+ sendbuf - starting address of the send buffer (choice)
. sendcount - number of elements in send buffer (non-negative integer)
. sendtype - data type of send buffer elements (handle)
. recvcounts - non-negative integer array (of length group size) containing the number of elements that are received from each process
. displs - integer array (of length group size). Entry i specifies the displacement relative to recvbuf at which to place the incoming data from process i
. recvtype - data type of receive buffer elements (handle)
- comm - communicator (handle)

Output Parameters:
+ recvbuf - starting address of the receive buffer (choice)
- request - communication request (handle)

.N ThreadSafe

.N Fortran

.N Errors
@*/
int MPI_Iallgatherv(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf,
                    const int recvcounts[], const int displs[], MPI_Datatype recvtype,
                    MPI_Comm comm, MPI_Request * request)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Comm *comm_ptr = NULL;
    MPIR_Request *request_ptr = NULL;
    MPIR_FUNC_TERSE_STATE_DECL(MPID_STATE_MPI_IALLGATHERV);

    MPID_THREAD_CS_ENTER(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    MPIR_FUNC_TERSE_ENTER(MPID_STATE_MPI_IALLGATHERV);

    /* Validate parameters, especially handles needing to be converted */
#ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            if (sendbuf != MPI_IN_PLACE) {
                MPIR_ERRTEST_DATATYPE(sendtype, "sendtype", mpi_errno);
                MPIR_ERRTEST_COUNT(sendcount, mpi_errno);
            }
            MPIR_ERRTEST_DATATYPE(recvtype, "recvtype", mpi_errno);
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
            MPIR_Comm_valid_ptr(comm_ptr, mpi_errno, FALSE);
            if (mpi_errno != MPI_SUCCESS)
                goto fn_fail;

            if (sendbuf != MPI_IN_PLACE) {
                if (!HANDLE_IS_BUILTIN(sendtype)) {
                    MPIR_Datatype *sendtype_ptr = NULL;
                    MPIR_Datatype_get_ptr(sendtype, sendtype_ptr);
                    MPIR_Datatype_valid_ptr(sendtype_ptr, mpi_errno);
                    if (mpi_errno != MPI_SUCCESS)
                        goto fn_fail;
                    MPIR_Datatype_committed_ptr(sendtype_ptr, mpi_errno);
                    if (mpi_errno != MPI_SUCCESS)
                        goto fn_fail;
                }

                /* catch common aliasing cases */
                if (comm_ptr->comm_kind == MPIR_COMM_KIND__INTRACOMM &&
                    sendtype == recvtype && recvcounts[comm_ptr->rank] != 0 && sendcount != 0) {
                    int recvtype_size;
                    MPIR_Datatype_get_size_macro(recvtype, recvtype_size);
                    MPIR_ERRTEST_ALIAS_COLL(sendbuf,
                                            (char *) recvbuf +
                                            displs[comm_ptr->rank] * recvtype_size, mpi_errno);
                }
            }

            MPIR_ERRTEST_ARGNULL(recvcounts, "recvcounts", mpi_errno);
            MPIR_ERRTEST_ARGNULL(displs, "displs", mpi_errno);
            if (!HANDLE_IS_BUILTIN(recvtype)) {
                MPIR_Datatype *recvtype_ptr = NULL;
                MPIR_Datatype_get_ptr(recvtype, recvtype_ptr);
                MPIR_Datatype_valid_ptr(recvtype_ptr, mpi_errno);
                if (mpi_errno != MPI_SUCCESS)
                    goto fn_fail;
                MPIR_Datatype_committed_ptr(recvtype_ptr, mpi_errno);
                if (mpi_errno != MPI_SUCCESS)
                    goto fn_fail;
            }

            MPIR_ERRTEST_ARGNULL(request, "request", mpi_errno);
            /* TODO more checks may be appropriate (counts, in_place, buffer aliasing, etc) */
        }
        MPID_END_ERROR_CHECKS;
    }
#endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */

    mpi_errno = MPIR_Iallgatherv(sendbuf, sendcount, sendtype, recvbuf, recvcounts,
                                 displs, recvtype, comm_ptr, &request_ptr);
    MPIR_ERR_CHECK(mpi_errno);

    /* create a complete request, if needed */
    if (!request_ptr)
        request_ptr = MPIR_Request_create_complete(MPIR_REQUEST_KIND__COLL);
    /* return the handle of the request to the user */
    *request = request_ptr->handle;

    /* ... end of body of routine ... */

  fn_exit:
    MPIR_FUNC_TERSE_EXIT(MPID_STATE_MPI_IALLGATHERV);
    MPID_THREAD_CS_EXIT(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
#ifdef HAVE_ERROR_CHECKING
    {
        mpi_errno =
            MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, __func__, __LINE__, MPI_ERR_OTHER,
                                 "**mpi_iallgatherv",
                                 "**mpi_iallgatherv %p %d %D %p %p %p %D %C %p", sendbuf, sendcount,
                                 sendtype, recvbuf, recvcounts, displs, recvtype, comm, request);
    }
#endif
    mpi_errno = MPIR_Err_return_comm(comm_ptr, __func__, mpi_errno);
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}
