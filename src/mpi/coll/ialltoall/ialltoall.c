/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"
/* for MPIR_TSP_sched_t */
#include "tsp_gentran.h"
#include "gentran_utils.h"
#include "../ialltoall/ialltoall_tsp_brucks_algos_prototypes.h"
#include "../ialltoall/ialltoall_tsp_ring_algos_prototypes.h"
#include "../ialltoall/ialltoall_tsp_scattered_algos_prototypes.h"

/*
*/

/* This is the machine-independent implementation of alltoall. The algorithm is:

   Algorithm: MPI_Alltoall

   We use four algorithms for alltoall. For short messages and
   (comm_size >= 8), we use the algorithm by Jehoshua Bruck et al,
   IEEE TPDS, Nov. 1997. It is a store-and-forward algorithm that
   takes lgp steps. Because of the extra communication, the bandwidth
   requirement is (n/2).lgp.beta.

   Cost = lgp.alpha + (n/2).lgp.beta

   where n is the total amount of data a process needs to send to all
   other processes.

   For medium size messages and (short messages for comm_size < 8), we
   use an algorithm that posts all irecvs and isends and then does a
   waitall. We scatter the order of sources and destinations among the
   processes, so that all processes don't try to send/recv to/from the
   same process at the same time.

   *** Modification: We post only a small number of isends and irecvs
   at a time and wait on them as suggested by Tony Ladd. ***
   *** See comments below about an additional modification that
   we may want to consider ***

   For long messages and power-of-two number of processes, we use a
   pairwise exchange algorithm, which takes p-1 steps. We
   calculate the pairs by using an exclusive-or algorithm:
           for (i=1; i<comm_size; i++)
               dest = rank ^ i;
   This algorithm doesn't work if the number of processes is not a power of
   two. For a non-power-of-two number of processes, we use an
   algorithm in which, in step i, each process  receives from (rank-i)
   and sends to (rank+i).

   Cost = (p-1).alpha + n.beta

   where n is the total amount of data a process needs to send to all
   other processes.

   Possible improvements:

   End Algorithm: MPI_Alltoall
*/

int MPIR_Ialltoall_allcomm_sched_auto(const void *sendbuf, MPI_Aint sendcount,
                                      MPI_Datatype sendtype, void *recvbuf, MPI_Aint recvcount,
                                      MPI_Datatype recvtype, MPIR_Comm * comm_ptr,
                                      bool is_persistent, void **sched_p,
                                      enum MPIR_sched_type *sched_type_p)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_Csel_coll_sig_s coll_sig = {
        .coll_type = MPIR_CSEL_COLL_TYPE__IALLTOALL,
        .comm_ptr = comm_ptr,

        .u.ialltoall.sendbuf = sendbuf,
        .u.ialltoall.sendcount = sendcount,
        .u.ialltoall.sendtype = sendtype,
        .u.ialltoall.recvcount = recvcount,
        .u.ialltoall.recvbuf = recvbuf,
        .u.ialltoall.recvtype = recvtype,
    };

    MPII_Csel_container_s *cnt = MPIR_Csel_search(comm_ptr->csel_comm, coll_sig);
    MPIR_Assert(cnt);

    switch (cnt->id) {
        /* *INDENT-OFF* */
        case MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Ialltoall_intra_gentran_ring:
            MPII_GENTRAN_CREATE_SCHED_P();
            mpi_errno =
                MPIR_TSP_Ialltoall_sched_intra_ring(sendbuf, sendcount, sendtype, recvbuf,
                                                    recvcount, recvtype, comm_ptr, *sched_p);
            break;

        case MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Ialltoall_intra_gentran_brucks:
            MPII_GENTRAN_CREATE_SCHED_P();
            mpi_errno =
                MPIR_TSP_Ialltoall_sched_intra_brucks(sendbuf, sendcount, sendtype, recvbuf,
                                                      recvcount, recvtype, comm_ptr,
                                                      cnt->u.ialltoall.intra_gentran_brucks.k,
                                                      cnt->u.ialltoall.intra_gentran_brucks.buffer_per_phase,
                                                      *sched_p);
            break;

        case MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Ialltoall_intra_gentran_scattered:
            MPII_GENTRAN_CREATE_SCHED_P();
            mpi_errno =
                MPIR_TSP_Ialltoall_sched_intra_scattered(sendbuf, sendcount, sendtype, recvbuf,
                                                         recvcount, recvtype, comm_ptr,
                                                         cnt->u.ialltoall.
                                                         intra_gentran_scattered.batch_size,
                                                         cnt->u.ialltoall.
                                                         intra_gentran_scattered.bblock, *sched_p);
            break;

        case MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Ialltoall_intra_sched_auto:
            MPII_SCHED_CREATE_SCHED_P();
            mpi_errno = MPIR_Ialltoall_intra_sched_auto(sendbuf, sendcount, sendtype, recvbuf,
                                                        recvcount, recvtype, comm_ptr, *sched_p);
            break;

        case MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Ialltoall_intra_sched_brucks:
            MPII_SCHED_CREATE_SCHED_P();
            mpi_errno = MPIR_Ialltoall_intra_sched_brucks(sendbuf, sendcount, sendtype, recvbuf,
                                                          recvcount, recvtype, comm_ptr, *sched_p);
            break;

        case MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Ialltoall_intra_sched_inplace:
            MPII_SCHED_CREATE_SCHED_P();
            mpi_errno = MPIR_Ialltoall_intra_sched_inplace(sendbuf, sendcount, sendtype, recvbuf,
                                                           recvcount, recvtype, comm_ptr, *sched_p);
            break;

        case MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Ialltoall_intra_sched_pairwise:
            MPII_SCHED_CREATE_SCHED_P();
            mpi_errno = MPIR_Ialltoall_intra_sched_pairwise(sendbuf, sendcount, sendtype, recvbuf,
                                                            recvcount, recvtype, comm_ptr,
                                                            *sched_p);
            break;

        case MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Ialltoall_intra_sched_permuted_sendrecv:
            MPII_SCHED_CREATE_SCHED_P();
            mpi_errno = MPIR_Ialltoall_intra_sched_permuted_sendrecv(sendbuf, sendcount, sendtype,
                                                                     recvbuf, recvcount, recvtype,
                                                                     comm_ptr, *sched_p);
            break;

        case MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Ialltoall_inter_sched_auto:
            MPII_SCHED_CREATE_SCHED_P();
            mpi_errno = MPIR_Ialltoall_inter_sched_auto(sendbuf, sendcount, sendtype, recvbuf,
                                                        recvcount, recvtype, comm_ptr, *sched_p);
            break;

        case MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Ialltoall_inter_sched_pairwise_exchange:
            MPII_SCHED_CREATE_SCHED_P();
            mpi_errno = MPIR_Ialltoall_inter_sched_pairwise_exchange(sendbuf, sendcount, sendtype,
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

int MPIR_Ialltoall_intra_sched_auto(const void *sendbuf, MPI_Aint sendcount, MPI_Datatype sendtype,
                                    void *recvbuf, MPI_Aint recvcount, MPI_Datatype recvtype,
                                    MPIR_Comm * comm_ptr, MPIR_Sched_t s)
{
    int mpi_errno = MPI_SUCCESS;
    int nbytes, comm_size, sendtype_size;

    comm_size = comm_ptr->local_size;

    MPIR_Datatype_get_size_macro(sendtype, sendtype_size);
    nbytes = sendtype_size * sendcount;

    if (sendbuf == MPI_IN_PLACE) {
        mpi_errno =
            MPIR_Ialltoall_intra_sched_inplace(sendbuf, sendcount, sendtype, recvbuf, recvcount,
                                               recvtype, comm_ptr, s);
    } else if ((nbytes <= MPIR_CVAR_ALLTOALL_SHORT_MSG_SIZE) && (comm_size >= 8)) {
        mpi_errno =
            MPIR_Ialltoall_intra_sched_brucks(sendbuf, sendcount, sendtype, recvbuf, recvcount,
                                              recvtype, comm_ptr, s);
    } else if (nbytes <= MPIR_CVAR_ALLTOALL_MEDIUM_MSG_SIZE) {
        mpi_errno =
            MPIR_Ialltoall_intra_sched_permuted_sendrecv(sendbuf, sendcount, sendtype, recvbuf,
                                                         recvcount, recvtype, comm_ptr, s);
    } else {
        mpi_errno =
            MPIR_Ialltoall_intra_sched_pairwise(sendbuf, sendcount, sendtype, recvbuf, recvcount,
                                                recvtype, comm_ptr, s);
    }
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}


int MPIR_Ialltoall_inter_sched_auto(const void *sendbuf, MPI_Aint sendcount, MPI_Datatype
                                    sendtype, void *recvbuf, MPI_Aint recvcount,
                                    MPI_Datatype recvtype, MPIR_Comm * comm_ptr, MPIR_Sched_t s)
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = MPIR_Ialltoall_inter_sched_pairwise_exchange(sendbuf, sendcount,
                                                             sendtype, recvbuf, recvcount, recvtype,
                                                             comm_ptr, s);

    return mpi_errno;
}

int MPIR_Ialltoall_sched_auto(const void *sendbuf, MPI_Aint sendcount, MPI_Datatype sendtype,
                              void *recvbuf, MPI_Aint recvcount, MPI_Datatype recvtype,
                              MPIR_Comm * comm_ptr, MPIR_Sched_t s)
{
    int mpi_errno = MPI_SUCCESS;

    if (comm_ptr->comm_kind == MPIR_COMM_KIND__INTRACOMM) {
        mpi_errno = MPIR_Ialltoall_intra_sched_auto(sendbuf, sendcount, sendtype,
                                                    recvbuf, recvcount, recvtype, comm_ptr, s);
    } else {
        mpi_errno = MPIR_Ialltoall_inter_sched_auto(sendbuf, sendcount, sendtype,
                                                    recvbuf, recvcount, recvtype, comm_ptr, s);
    }

    return mpi_errno;
}

int MPIR_Ialltoall_sched_impl(const void *sendbuf, MPI_Aint sendcount, MPI_Datatype sendtype,
                              void *recvbuf, MPI_Aint recvcount, MPI_Datatype recvtype,
                              MPIR_Comm * comm_ptr, bool is_persistent, void **sched_p,
                              enum MPIR_sched_type *sched_type_p)
{
    int mpi_errno = MPI_SUCCESS;

    /* If the user picks one of the transport-enabled algorithms, branch there
     * before going down to the MPIR_Sched-based algorithms. */
    /* TODO - Eventually the intention is to replace all of the
     * MPIR_Sched-based algorithms with transport-enabled algorithms, but that
     * will require sufficient performance testing and replacement algorithms. */
    if (comm_ptr->comm_kind == MPIR_COMM_KIND__INTRACOMM) {
        /* intracommunicator */
        switch (MPIR_CVAR_IALLTOALL_INTRA_ALGORITHM) {
            /* *INDENT-OFF* */
            case MPIR_CVAR_IALLTOALL_INTRA_ALGORITHM_gentran_ring:
                MPII_GENTRAN_CREATE_SCHED_P();
                mpi_errno =
                    MPIR_TSP_Ialltoall_sched_intra_ring(sendbuf, sendcount, sendtype, recvbuf,
                                                        recvcount, recvtype, comm_ptr, *sched_p);
                break;

            case MPIR_CVAR_IALLTOALL_INTRA_ALGORITHM_gentran_brucks:
                MPII_GENTRAN_CREATE_SCHED_P();
                mpi_errno =
                    MPIR_TSP_Ialltoall_sched_intra_brucks(sendbuf, sendcount, sendtype, recvbuf,
                                                          recvcount, recvtype, comm_ptr,
                                                          MPIR_CVAR_IALLTOALL_BRUCKS_KVAL,
                                                          MPIR_CVAR_IALLTOALL_BRUCKS_BUFFER_PER_NBR,
                                                          *sched_p);
                break;

            case MPIR_CVAR_IALLTOALL_INTRA_ALGORITHM_gentran_scattered:
                MPII_GENTRAN_CREATE_SCHED_P();
                mpi_errno =
                    MPIR_TSP_Ialltoall_sched_intra_scattered(sendbuf, sendcount, sendtype, recvbuf,
                                                             recvcount, recvtype, comm_ptr,
                                                             MPIR_CVAR_IALLTOALL_SCATTERED_BATCH_SIZE,
                                                             MPIR_CVAR_IALLTOALL_SCATTERED_OUTSTANDING_TASKS,
                                                             *sched_p);
                break;

            case MPIR_CVAR_IALLTOALL_INTRA_ALGORITHM_sched_brucks:
                MPII_SCHED_CREATE_SCHED_P();
                mpi_errno = MPIR_Ialltoall_intra_sched_brucks(sendbuf, sendcount, sendtype, recvbuf,
                                                              recvcount, recvtype, comm_ptr,
                                                              *sched_p);
                break;

            case MPIR_CVAR_IALLTOALL_INTRA_ALGORITHM_sched_inplace:
                MPII_SCHED_CREATE_SCHED_P();
                mpi_errno = MPIR_Ialltoall_intra_sched_inplace(sendbuf, sendcount, sendtype,
                                                               recvbuf, recvcount, recvtype,
                                                               comm_ptr, *sched_p);
                break;

            case MPIR_CVAR_IALLTOALL_INTRA_ALGORITHM_sched_pairwise:
                MPII_SCHED_CREATE_SCHED_P();
                mpi_errno = MPIR_Ialltoall_intra_sched_pairwise(sendbuf, sendcount, sendtype,
                                                                recvbuf, recvcount, recvtype,
                                                                comm_ptr, *sched_p);
                break;

            case MPIR_CVAR_IALLTOALL_INTRA_ALGORITHM_sched_permuted_sendrecv:
                MPII_SCHED_CREATE_SCHED_P();
                mpi_errno = MPIR_Ialltoall_intra_sched_permuted_sendrecv(sendbuf, sendcount,
                                                                         sendtype, recvbuf,
                                                                         recvcount, recvtype,
                                                                         comm_ptr, *sched_p);
                break;

            case MPIR_CVAR_IALLTOALL_INTRA_ALGORITHM_sched_auto:
                MPII_SCHED_CREATE_SCHED_P();
                mpi_errno = MPIR_Ialltoall_intra_sched_auto(sendbuf, sendcount, sendtype, recvbuf,
                                                            recvcount, recvtype, comm_ptr,
                                                            *sched_p);
                break;

            case MPIR_CVAR_IALLTOALL_INTRA_ALGORITHM_auto:
                mpi_errno =
                    MPIR_Ialltoall_allcomm_sched_auto(sendbuf, sendcount, sendtype, recvbuf,
                                                      recvcount, recvtype, comm_ptr, is_persistent,
                                                      sched_p, sched_type_p);
                break;

            default:
                MPIR_Assert(0);
            /* *INDENT-ON* */
        }
    } else {
        switch (MPIR_CVAR_IALLTOALL_INTER_ALGORITHM) {
            /* *INDENT-OFF* */
            case MPIR_CVAR_IALLTOALL_INTER_ALGORITHM_sched_pairwise_exchange:
                MPII_SCHED_CREATE_SCHED_P();
                mpi_errno = MPIR_Ialltoall_inter_sched_pairwise_exchange(sendbuf, sendcount,
                                                                         sendtype, recvbuf,
                                                                         recvcount, recvtype,
                                                                         comm_ptr, *sched_p);
                break;

            case MPIR_CVAR_IALLTOALL_INTER_ALGORITHM_sched_auto:
                MPII_SCHED_CREATE_SCHED_P();
                mpi_errno = MPIR_Ialltoall_inter_sched_auto(sendbuf, sendcount, sendtype, recvbuf,
                                                            recvcount, recvtype, comm_ptr,
                                                            *sched_p);
                break;

            case MPIR_CVAR_IALLTOALL_INTER_ALGORITHM_auto:
                mpi_errno =
                    MPIR_Ialltoall_allcomm_sched_auto(sendbuf, sendcount, sendtype, recvbuf,
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

int MPIR_Ialltoall_impl(const void *sendbuf, MPI_Aint sendcount,
                        MPI_Datatype sendtype, void *recvbuf, MPI_Aint recvcount,
                        MPI_Datatype recvtype, MPIR_Comm * comm_ptr, MPIR_Request ** request)
{
    int mpi_errno = MPI_SUCCESS;

    *request = NULL;

    enum MPIR_sched_type sched_type;
    void *sched;
    mpi_errno = MPIR_Ialltoall_sched_impl(sendbuf, sendcount, sendtype, recvbuf, recvcount,
                                          recvtype, comm_ptr, false, &sched, &sched_type);
    MPIR_ERR_CHECK(mpi_errno);

    MPII_SCHED_START(sched_type, sched, comm_ptr, request);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIR_Ialltoall(const void *sendbuf, MPI_Aint sendcount, MPI_Datatype sendtype,
                   void *recvbuf, MPI_Aint recvcount, MPI_Datatype recvtype,
                   MPIR_Comm * comm_ptr, MPIR_Request ** request)
{
    int mpi_errno = MPI_SUCCESS;

    if ((MPIR_CVAR_DEVICE_COLLECTIVES == MPIR_CVAR_DEVICE_COLLECTIVES_all) ||
        ((MPIR_CVAR_DEVICE_COLLECTIVES == MPIR_CVAR_DEVICE_COLLECTIVES_percoll) &&
         MPIR_CVAR_IALLTOALL_DEVICE_COLLECTIVE)) {
        mpi_errno =
            MPID_Ialltoall(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, comm_ptr,
                           request);
    } else {
        mpi_errno = MPIR_Ialltoall_impl(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype,
                                        comm_ptr, request);
    }

    return mpi_errno;
}
