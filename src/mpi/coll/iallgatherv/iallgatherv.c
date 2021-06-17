/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"
#include "iallgatherv.h"
/* for MPIR_TSP_sched_t */
#include "tsp_gentran.h"
#include "gentran_utils.h"
#include "../iallgatherv/iallgatherv_tsp_brucks_algos_prototypes.h"
#include "../iallgatherv/iallgatherv_tsp_recexch_algos_prototypes.h"
#include "../iallgatherv/iallgatherv_tsp_ring_algos_prototypes.h"

/*
*/

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

int MPIR_Iallgatherv_allcomm_sched_auto(const void *sendbuf, MPI_Aint sendcount,
                                        MPI_Datatype sendtype, void *recvbuf,
                                        const MPI_Aint * recvcounts, const MPI_Aint * displs,
                                        MPI_Datatype recvtype, MPIR_Comm * comm_ptr,
                                        bool is_persistent, void **sched_p,
                                        enum MPIR_sched_type *sched_type_p)
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
        /* *INDENT-OFF* */
        case MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Iallgatherv_intra_gentran_brucks:
            MPII_GENTRAN_CREATE_SCHED_P();
            mpi_errno =
                MPIR_TSP_Iallgatherv_sched_intra_brucks(sendbuf, sendcount, sendtype, recvbuf,
                                                        recvcounts, displs, recvtype, comm_ptr,
                                                        cnt->u.iallgatherv.intra_gentran_brucks.k,
                                                        *sched_p);
            break;

        case MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Iallgatherv_intra_sched_auto:
            MPII_SCHED_CREATE_SCHED_P();
            mpi_errno = MPIR_Iallgatherv_intra_sched_auto(sendbuf, sendcount, sendtype, recvbuf,
                                                          recvcounts, displs, recvtype, comm_ptr,
                                                          *sched_p);
            break;

        case MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Iallgatherv_intra_sched_brucks:
            MPII_SCHED_CREATE_SCHED_P();
            mpi_errno = MPIR_Iallgatherv_intra_sched_brucks(sendbuf, sendcount, sendtype, recvbuf,
                                                            recvcounts, displs, recvtype, comm_ptr,
                                                            *sched_p);
            break;

        case MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Iallgatherv_intra_sched_recursive_doubling:
            MPII_SCHED_CREATE_SCHED_P();
            mpi_errno =
                MPIR_Iallgatherv_intra_sched_recursive_doubling(sendbuf, sendcount, sendtype,
                                                                recvbuf, recvcounts, displs,
                                                                recvtype, comm_ptr, *sched_p);
            break;

        case MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Iallgatherv_intra_sched_ring:
            MPII_SCHED_CREATE_SCHED_P();
            mpi_errno = MPIR_Iallgatherv_intra_sched_ring(sendbuf, sendcount, sendtype, recvbuf,
                                                          recvcounts, displs, recvtype, comm_ptr,
                                                          *sched_p);
            break;

        case MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Iallgatherv_intra_gentran_recexch_doubling:
            MPII_GENTRAN_CREATE_SCHED_P();
            mpi_errno =
                MPIR_TSP_Iallgatherv_sched_intra_recexch(sendbuf, sendcount, sendtype, recvbuf,
                                                         recvcounts, displs, recvtype, comm_ptr,
                                                         MPIR_IALLGATHERV_RECEXCH_TYPE_DISTANCE_DOUBLING,
                                                         cnt->u.
                                                         iallgatherv.intra_gentran_recexch_doubling.
                                                         k, *sched_p);
            break;

        case MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Iallgatherv_intra_gentran_recexch_halving:
            MPII_GENTRAN_CREATE_SCHED_P();
            mpi_errno =
                MPIR_TSP_Iallgatherv_sched_intra_recexch(sendbuf, sendcount, sendtype, recvbuf,
                                                         recvcounts, displs, recvtype, comm_ptr,
                                                         MPIR_IALLGATHERV_RECEXCH_TYPE_DISTANCE_HALVING,
                                                         cnt->u.
                                                         iallgatherv.intra_gentran_recexch_doubling.
                                                         k, *sched_p);
            break;

        case MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Iallgatherv_intra_gentran_ring:
            MPII_GENTRAN_CREATE_SCHED_P();
            mpi_errno =
                MPIR_TSP_Iallgatherv_sched_intra_ring(sendbuf, sendcount, sendtype, recvbuf,
                                                      recvcounts, displs, recvtype, comm_ptr,
                                                      *sched_p);
            break;

        case MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Iallgatherv_inter_sched_auto:
            MPII_SCHED_CREATE_SCHED_P();
            mpi_errno = MPIR_Iallgatherv_inter_sched_auto(sendbuf, sendcount, sendtype, recvbuf,
                                                          recvcounts, displs, recvtype, comm_ptr,
                                                          *sched_p);
            break;

        case MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Iallgatherv_inter_sched_remote_gather_local_bcast:
            MPII_SCHED_CREATE_SCHED_P();
            mpi_errno =
                MPIR_Iallgatherv_inter_sched_remote_gather_local_bcast(sendbuf, sendcount, sendtype,
                                                                       recvbuf, recvcounts, displs,
                                                                       recvtype, comm_ptr,
                                                                       *sched_p);
            break;

        default:
            MPIR_Assert(0);
        /* *INDENT-ON* */
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIR_Iallgatherv_intra_sched_auto(const void *sendbuf, MPI_Aint sendcount,
                                      MPI_Datatype sendtype, void *recvbuf,
                                      const MPI_Aint recvcounts[], const MPI_Aint displs[],
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

int MPIR_Iallgatherv_inter_sched_auto(const void *sendbuf, MPI_Aint sendcount,
                                      MPI_Datatype sendtype, void *recvbuf,
                                      const MPI_Aint recvcounts[], const MPI_Aint displs[],
                                      MPI_Datatype recvtype, MPIR_Comm * comm_ptr, MPIR_Sched_t s)
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = MPIR_Iallgatherv_inter_sched_remote_gather_local_bcast(sendbuf, sendcount,
                                                                       sendtype, recvbuf,
                                                                       recvcounts, displs, recvtype,
                                                                       comm_ptr, s);

    return mpi_errno;
}

int MPIR_Iallgatherv_sched_auto(const void *sendbuf, MPI_Aint sendcount, MPI_Datatype sendtype,
                                void *recvbuf, const MPI_Aint recvcounts[], const MPI_Aint displs[],
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

int MPIR_Iallgatherv_sched_impl(const void *sendbuf, MPI_Aint sendcount, MPI_Datatype sendtype,
                                void *recvbuf, const MPI_Aint recvcounts[], const MPI_Aint displs[],
                                MPI_Datatype recvtype, MPIR_Comm * comm_ptr, bool is_persistent,
                                void **sched_p, enum MPIR_sched_type *sched_type_p)
{
    int mpi_errno = MPI_SUCCESS;

    int comm_size = comm_ptr->local_size;
    /* If the user picks one of the transport-enabled algorithms, branch there
     * before going down to the MPIR_Sched-based algorithms. */
    /* TODO - Eventually the intention is to replace all of the
     * MPIR_Sched-based algorithms with transport-enabled algorithms, but that
     * will require sufficient performance testing and replacement algorithms. */
    if (comm_ptr->comm_kind == MPIR_COMM_KIND__INTRACOMM) {
        /* intracommunicator */
        switch (MPIR_CVAR_IALLGATHERV_INTRA_ALGORITHM) {
            /* *INDENT-OFF* */
            case MPIR_CVAR_IALLGATHERV_INTRA_ALGORITHM_gentran_recexch_doubling:
                /* This algo cannot handle unordered data */
                MPII_COLLECTIVE_FALLBACK_CHECK(comm_ptr->rank,
                                               MPII_Iallgatherv_is_displs_ordered
                                               (comm_size, recvcounts, displs), mpi_errno,
                                               "Iallgatherv gentran_recexch_doubling cannot be applied.\n");
                MPII_GENTRAN_CREATE_SCHED_P();
                mpi_errno =
                    MPIR_TSP_Iallgatherv_sched_intra_recexch(sendbuf, sendcount, sendtype, recvbuf,
                                                             recvcounts, displs, recvtype, comm_ptr,
                                                             MPIR_IALLGATHERV_RECEXCH_TYPE_DISTANCE_DOUBLING,
                                                             MPIR_CVAR_IALLGATHERV_RECEXCH_KVAL,
                                                             *sched_p);
                break;

            case MPIR_CVAR_IALLGATHERV_INTRA_ALGORITHM_gentran_recexch_halving:
                /* This algo cannot handle unordered data */
                MPII_COLLECTIVE_FALLBACK_CHECK(comm_ptr->rank,
                                               MPII_Iallgatherv_is_displs_ordered
                                               (comm_size, recvcounts, displs), mpi_errno,
                                               "Iallgatherv gentran_recexch_halving cannot be applied.\n");
                MPII_GENTRAN_CREATE_SCHED_P();
                mpi_errno =
                    MPIR_TSP_Iallgatherv_sched_intra_recexch(sendbuf, sendcount, sendtype, recvbuf,
                                                             recvcounts, displs, recvtype, comm_ptr,
                                                             MPIR_IALLGATHERV_RECEXCH_TYPE_DISTANCE_HALVING,
                                                             MPIR_CVAR_IALLGATHERV_RECEXCH_KVAL,
                                                             *sched_p);
                break;

            case MPIR_CVAR_IALLGATHERV_INTRA_ALGORITHM_gentran_ring:
                MPII_GENTRAN_CREATE_SCHED_P();
                mpi_errno =
                    MPIR_TSP_Iallgatherv_sched_intra_ring(sendbuf, sendcount, sendtype, recvbuf,
                                                          recvcounts, displs, recvtype, comm_ptr,
                                                          *sched_p);
                break;

            case MPIR_CVAR_IALLGATHERV_INTRA_ALGORITHM_gentran_brucks:
                MPII_GENTRAN_CREATE_SCHED_P();
                mpi_errno =
                    MPIR_TSP_Iallgatherv_sched_intra_brucks(sendbuf, sendcount, sendtype, recvbuf,
                                                            recvcounts, displs, recvtype, comm_ptr,
                                                            MPIR_CVAR_IALLGATHERV_BRUCKS_KVAL,
                                                            *sched_p);
                break;

            case MPIR_CVAR_IALLGATHERV_INTRA_ALGORITHM_sched_brucks:
                MPII_SCHED_CREATE_SCHED_P();
                mpi_errno = MPIR_Iallgatherv_intra_sched_brucks(sendbuf, sendcount, sendtype,
                                                                recvbuf, recvcounts, displs,
                                                                recvtype, comm_ptr, *sched_p);
                break;

            case MPIR_CVAR_IALLGATHERV_INTRA_ALGORITHM_sched_recursive_doubling:
                MPII_SCHED_CREATE_SCHED_P();
                mpi_errno = MPIR_Iallgatherv_intra_sched_recursive_doubling(sendbuf, sendcount,
                                                                            sendtype, recvbuf,
                                                                            recvcounts, displs,
                                                                            recvtype, comm_ptr,
                                                                            *sched_p);
                break;

            case MPIR_CVAR_IALLGATHERV_INTRA_ALGORITHM_sched_ring:
                MPII_SCHED_CREATE_SCHED_P();
                mpi_errno = MPIR_Iallgatherv_intra_sched_ring(sendbuf, sendcount, sendtype, recvbuf,
                                                              recvcounts, displs, recvtype,
                                                              comm_ptr, *sched_p);
                break;

            case MPIR_CVAR_IALLGATHERV_INTRA_ALGORITHM_sched_auto:
                MPII_SCHED_CREATE_SCHED_P();
                mpi_errno = MPIR_Iallgatherv_intra_sched_auto(sendbuf, sendcount, sendtype, recvbuf,
                                                              recvcounts, displs, recvtype,
                                                              comm_ptr, *sched_p);
                break;

            case MPIR_CVAR_IALLGATHERV_INTRA_ALGORITHM_auto:
                mpi_errno =
                    MPIR_Iallgatherv_allcomm_sched_auto(sendbuf, sendcount, sendtype, recvbuf,
                                                        recvcounts, displs, recvtype, comm_ptr,
                                                        is_persistent, sched_p, sched_type_p);
                break;

            default:
                MPIR_Assert(0);
            /* *INDENT-ON* */
        }
    } else {
        switch (MPIR_CVAR_IALLGATHERV_INTER_ALGORITHM) {
            /* *INDENT-OFF* */
            case MPIR_CVAR_IALLGATHERV_INTER_ALGORITHM_sched_remote_gather_local_bcast:
                MPII_SCHED_CREATE_SCHED_P();
                mpi_errno =
                    MPIR_Iallgatherv_inter_sched_remote_gather_local_bcast(sendbuf, sendcount,
                                                                           sendtype, recvbuf,
                                                                           recvcounts, displs,
                                                                           recvtype, comm_ptr,
                                                                           *sched_p);
                break;

            case MPIR_CVAR_IALLGATHERV_INTER_ALGORITHM_sched_auto:
                MPII_SCHED_CREATE_SCHED_P();
                mpi_errno = MPIR_Iallgatherv_inter_sched_auto(sendbuf, sendcount, sendtype, recvbuf,
                                                              recvcounts, displs, recvtype,
                                                              comm_ptr, *sched_p);
                break;

            case MPIR_CVAR_IALLGATHERV_INTER_ALGORITHM_auto:
                mpi_errno =
                    MPIR_Iallgatherv_allcomm_sched_auto(sendbuf, sendcount, sendtype, recvbuf,
                                                        recvcounts, displs, recvtype, comm_ptr,
                                                        is_persistent, sched_p, sched_type_p);
                break;

            default:
                MPIR_Assert(0);
            /* *INDENT-ON* */
        }
    }

    MPIR_ERR_CHECK(mpi_errno);
    goto fn_exit;

  fallback:
    if (comm_ptr->comm_kind == MPIR_COMM_KIND__INTRACOMM) {
        MPII_SCHED_CREATE_SCHED_P();
        mpi_errno = MPIR_Iallgatherv_intra_sched_auto(sendbuf, sendcount, sendtype, recvbuf,
                                                      recvcounts, displs, recvtype, comm_ptr,
                                                      *sched_p);
    } else {
        MPII_SCHED_CREATE_SCHED_P();
        mpi_errno = MPIR_Iallgatherv_inter_sched_auto(sendbuf, sendcount, sendtype, recvbuf,
                                                      recvcounts, displs, recvtype, comm_ptr,
                                                      *sched_p);
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIR_Iallgatherv_impl(const void *sendbuf, MPI_Aint sendcount, MPI_Datatype sendtype,
                          void *recvbuf, const MPI_Aint recvcounts[], const MPI_Aint displs[],
                          MPI_Datatype recvtype, MPIR_Comm * comm_ptr, MPIR_Request ** request)
{
    int mpi_errno = MPI_SUCCESS;

    *request = NULL;

    enum MPIR_sched_type sched_type;
    void *sched;
    mpi_errno = MPIR_Iallgatherv_sched_impl(sendbuf, sendcount, sendtype, recvbuf, recvcounts,
                                            displs, recvtype, comm_ptr, false, &sched, &sched_type);
    MPIR_ERR_CHECK(mpi_errno);

    MPII_SCHED_START(sched_type, sched, comm_ptr, request);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIR_Iallgatherv(const void *sendbuf, MPI_Aint sendcount, MPI_Datatype sendtype,
                     void *recvbuf, const MPI_Aint recvcounts[], const MPI_Aint displs[],
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
