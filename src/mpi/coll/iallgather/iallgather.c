/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"
/* for MPIR_TSP_sched_t */
#include "tsp_gentran.h"
#include "gentran_utils.h"
#include "../iallgather/iallgather_tsp_brucks_algos_prototypes.h"
#include "../iallgather/iallgather_tsp_recexch_algos_prototypes.h"
#include "../iallgather/iallgather_tsp_ring_algos_prototypes.h"

/*
*/

/* This is the machine-independent implementation of allgather. The algorithm is:

   Algorithm: MPI_Allgather

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

   It is interesting to note that either of the above algorithms for
   MPI_Allgather has the same cost as the tree algorithm for MPI_Gather!

   For long messages or medium-size messages and non-power-of-two
   no. of processes, we use a ring algorithm. In the first step, each
   process i sends its contribution to process i+1 and receives
   the contribution from process i-1 (with wrap-around). From the
   second step onwards, each process i forwards to process i+1 the
   data it received from process i-1 in the previous step. This takes
   a total of p-1 steps.

   Cost = (p-1).alpha + n.((p-1)/p).beta

   We use this algorithm instead of recursive doubling for long
   messages because we find that this communication pattern (nearest
   neighbor) performs twice as fast as recursive doubling for long
   messages (on Myrinet and IBM SP).

   Possible improvements:

   End Algorithm: MPI_Allgather
*/

int MPIR_Iallgather_allcomm_sched_auto(const void *sendbuf, MPI_Aint sendcount,
                                       MPI_Datatype sendtype, void *recvbuf, MPI_Aint recvcount,
                                       MPI_Datatype recvtype, MPIR_Comm * comm_ptr,
                                       bool is_persistent, void **sched_p,
                                       enum MPIR_sched_type *sched_type_p)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_Csel_coll_sig_s coll_sig = {
        .coll_type = MPIR_CSEL_COLL_TYPE__IALLGATHER,
        .comm_ptr = comm_ptr,

        .u.iallgather.sendbuf = sendbuf,
        .u.iallgather.sendcount = sendcount,
        .u.iallgather.sendtype = sendtype,
        .u.iallgather.recvbuf = recvbuf,
        .u.iallgather.recvcount = recvcount,
        .u.iallgather.recvtype = recvtype,
    };

    MPII_Csel_container_s *cnt = MPIR_Csel_search(comm_ptr->csel_comm, coll_sig);
    MPIR_Assert(cnt);

    switch (cnt->id) {
        /* *INDENT-OFF* */
        case MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Iallgather_intra_gentran_brucks:
            MPII_GENTRAN_CREATE_SCHED_P();
            mpi_errno =
                MPIR_TSP_Iallgather_sched_intra_brucks(sendbuf, sendcount, sendtype, recvbuf,
                                                       recvcount, recvtype, comm_ptr,
                                                       cnt->u.iallgather.intra_gentran_brucks.k,
                                                       *sched_p);
            break;

        case MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Iallgather_intra_sched_brucks:
            MPII_SCHED_CREATE_SCHED_P();
            mpi_errno = MPIR_Iallgather_intra_sched_brucks(sendbuf, sendcount, sendtype, recvbuf,
                                                           recvcount, recvtype, comm_ptr, *sched_p);
            break;

        case MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Iallgather_intra_sched_recursive_doubling:
            MPII_SCHED_CREATE_SCHED_P();
            mpi_errno = MPIR_Iallgather_intra_sched_recursive_doubling(sendbuf, sendcount, sendtype,
                                                                       recvbuf, recvcount, recvtype,
                                                                       comm_ptr, *sched_p);
            break;

        case MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Iallgather_intra_sched_ring:
            MPII_SCHED_CREATE_SCHED_P();
            mpi_errno = MPIR_Iallgather_intra_sched_ring(sendbuf, sendcount, sendtype, recvbuf,
                                                         recvcount, recvtype, comm_ptr, *sched_p);
            break;

        case MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Iallgather_intra_gentran_recexch_doubling:
            MPII_GENTRAN_CREATE_SCHED_P();
            mpi_errno =
                MPIR_TSP_Iallgather_sched_intra_recexch(sendbuf, sendcount, sendtype, recvbuf,
                                                        recvcount, recvtype, comm_ptr,
                                                        MPIR_IALLGATHER_RECEXCH_TYPE_DISTANCE_DOUBLING,
                                                        cnt->u.
                                                        iallgather.intra_gentran_recexch_doubling.k,
                                                        *sched_p);
            break;

        case MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Iallgather_intra_gentran_recexch_halving:
            MPII_GENTRAN_CREATE_SCHED_P();
            mpi_errno =
                MPIR_TSP_Iallgather_sched_intra_recexch(sendbuf, sendcount, sendtype, recvbuf,
                                                        recvcount, recvtype, comm_ptr,
                                                        MPIR_IALLGATHER_RECEXCH_TYPE_DISTANCE_HALVING,
                                                        cnt->u.
                                                        iallgather.intra_gentran_recexch_halving.k,
                                                        *sched_p);
            break;

        case MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Iallgather_intra_gentran_ring:
            MPII_GENTRAN_CREATE_SCHED_P();
            mpi_errno =
                MPIR_TSP_Iallgather_sched_intra_ring(sendbuf, sendcount, sendtype, recvbuf,
                                                     recvcount, recvtype, comm_ptr, *sched_p);
            break;

        case MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Iallgather_inter_sched_local_gather_remote_bcast:
            MPII_SCHED_CREATE_SCHED_P();
            mpi_errno =
                MPIR_Iallgather_inter_sched_local_gather_remote_bcast(sendbuf, sendcount, sendtype,
                                                                      recvbuf, recvcount, recvtype,
                                                                      comm_ptr, *sched_p);
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

int MPIR_Iallgather_sched_impl(const void *sendbuf, MPI_Aint sendcount,
                               MPI_Datatype sendtype, void *recvbuf, MPI_Aint recvcount,
                               MPI_Datatype recvtype, MPIR_Comm * comm_ptr, bool is_persistent,
                               void **sched_p, enum MPIR_sched_type *sched_type_p)
{
    int mpi_errno = MPI_SUCCESS;

    /* If the user picks one of the transport-enabled algorithms, branch there
     * before going down to the MPIR_Sched-based algorithms. */
    /* TODO - Eventually the intention is to replace all of the
     * MPIR_Sched-based algorithms with transport-enabled algorithms, but that
     * will require sufficient performance testing and replacement algorithms. */
    if (comm_ptr->comm_kind == MPIR_COMM_KIND__INTRACOMM) {
        /* intracommunicator */
        switch (MPIR_CVAR_IALLGATHER_INTRA_ALGORITHM) {
            /* *INDENT-OFF* */
            case MPIR_CVAR_IALLGATHER_INTRA_ALGORITHM_gentran_recexch_doubling:
                MPII_GENTRAN_CREATE_SCHED_P();
                mpi_errno =
                    MPIR_TSP_Iallgather_sched_intra_recexch(sendbuf, sendcount, sendtype, recvbuf,
                                                            recvcount, recvtype, comm_ptr,
                                                            MPIR_IALLGATHER_RECEXCH_TYPE_DISTANCE_DOUBLING,
                                                            MPIR_CVAR_IALLGATHER_RECEXCH_KVAL,
                                                            *sched_p);
                break;

            case MPIR_CVAR_IALLGATHER_INTRA_ALGORITHM_gentran_recexch_halving:
                MPII_GENTRAN_CREATE_SCHED_P();
                mpi_errno =
                    MPIR_TSP_Iallgather_sched_intra_recexch(sendbuf, sendcount, sendtype, recvbuf,
                                                            recvcount, recvtype, comm_ptr,
                                                            MPIR_IALLGATHER_RECEXCH_TYPE_DISTANCE_HALVING,
                                                            MPIR_CVAR_IALLGATHER_RECEXCH_KVAL,
                                                            *sched_p);
                break;

            case MPIR_CVAR_IALLGATHER_INTRA_ALGORITHM_gentran_brucks:
                MPII_GENTRAN_CREATE_SCHED_P();
                mpi_errno =
                    MPIR_TSP_Iallgather_sched_intra_brucks(sendbuf, sendcount, sendtype, recvbuf,
                                                           recvcount, recvtype, comm_ptr,
                                                           MPIR_CVAR_IALLGATHER_BRUCKS_KVAL,
                                                           *sched_p);
                break;

            case MPIR_CVAR_IALLGATHER_INTRA_ALGORITHM_gentran_ring:
                MPII_GENTRAN_CREATE_SCHED_P();
                mpi_errno =
                    MPIR_TSP_Iallgather_sched_intra_ring(sendbuf, sendcount, sendtype, recvbuf,
                                                         recvcount, recvtype, comm_ptr, *sched_p);
                break;

            case MPIR_CVAR_IALLGATHER_INTRA_ALGORITHM_sched_brucks:
                MPII_SCHED_CREATE_SCHED_P();
                mpi_errno = MPIR_Iallgather_intra_sched_brucks(sendbuf, sendcount, sendtype,
                                                               recvbuf, recvcount, recvtype,
                                                               comm_ptr, *sched_p);
                break;

            case MPIR_CVAR_IALLGATHER_INTRA_ALGORITHM_sched_recursive_doubling:
                MPII_SCHED_CREATE_SCHED_P();
                mpi_errno =
                    MPIR_Iallgather_intra_sched_recursive_doubling(sendbuf, sendcount, sendtype,
                                                                   recvbuf, recvcount, recvtype,
                                                                   comm_ptr, *sched_p);
                break;

            case MPIR_CVAR_IALLGATHER_INTRA_ALGORITHM_sched_ring:
                MPII_SCHED_CREATE_SCHED_P();
                mpi_errno = MPIR_Iallgather_intra_sched_ring(sendbuf, sendcount, sendtype, recvbuf,
                                                             recvcount, recvtype, comm_ptr,
                                                             *sched_p);
                break;

            case MPIR_CVAR_IALLGATHER_INTRA_ALGORITHM_auto:
                mpi_errno =
                    MPIR_Iallgather_allcomm_sched_auto(sendbuf, sendcount, sendtype, recvbuf,
                                                       recvcount, recvtype, comm_ptr, is_persistent,
                                                       sched_p, sched_type_p);
                break;

            default:
                MPIR_Assert(0);
            /* *INDENT-ON* */
        }
    } else {
        switch (MPIR_CVAR_IALLGATHER_INTER_ALGORITHM) {
            /* *INDENT-OFF* */
            case MPIR_CVAR_IALLGATHER_INTER_ALGORITHM_sched_local_gather_remote_bcast:
                MPII_SCHED_CREATE_SCHED_P();
                mpi_errno =
                    MPIR_Iallgather_inter_sched_local_gather_remote_bcast(sendbuf, sendcount,
                                                                          sendtype, recvbuf,
                                                                          recvcount, recvtype,
                                                                          comm_ptr, *sched_p);
                break;

            case MPIR_CVAR_IALLGATHER_INTER_ALGORITHM_auto:
                mpi_errno =
                    MPIR_Iallgather_allcomm_sched_auto(sendbuf, sendcount, sendtype, recvbuf,
                                                       recvcount, recvtype, comm_ptr, is_persistent,
                                                       sched_p, sched_type_p);
                break;

            default:
                MPIR_Assert(0);
            /* *INDENT-ON* */
        }
    }

    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIR_Iallgather_impl(const void *sendbuf, MPI_Aint sendcount, MPI_Datatype sendtype,
                         void *recvbuf, MPI_Aint recvcount, MPI_Datatype recvtype,
                         MPIR_Comm * comm_ptr, MPIR_Request ** request)
{
    int mpi_errno = MPI_SUCCESS;

    *request = NULL;

    enum MPIR_sched_type sched_type;
    void *sched;
    mpi_errno = MPIR_Iallgather_sched_impl(sendbuf, sendcount, sendtype, recvbuf, recvcount,
                                           recvtype, comm_ptr, false, &sched, &sched_type);
    MPIR_ERR_CHECK(mpi_errno);

    MPII_SCHED_START(sched_type, sched, comm_ptr, request);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIR_Iallgather(const void *sendbuf, MPI_Aint sendcount, MPI_Datatype sendtype,
                    void *recvbuf, MPI_Aint recvcount, MPI_Datatype recvtype,
                    MPIR_Comm * comm_ptr, MPIR_Request ** request)
{
    int mpi_errno = MPI_SUCCESS;

    if ((MPIR_CVAR_DEVICE_COLLECTIVES == MPIR_CVAR_DEVICE_COLLECTIVES_all) ||
        ((MPIR_CVAR_DEVICE_COLLECTIVES == MPIR_CVAR_DEVICE_COLLECTIVES_percoll) &&
         MPIR_CVAR_IALLGATHER_DEVICE_COLLECTIVE)) {
        mpi_errno =
            MPID_Iallgather(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, comm_ptr,
                            request);
    } else {
        mpi_errno = MPIR_Iallgather_impl(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype,
                                         comm_ptr, request);
    }

    return mpi_errno;
}
