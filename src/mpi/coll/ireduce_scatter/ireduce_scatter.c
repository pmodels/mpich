/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"
/* for MPIR_TSP_sched_t */
#include "tsp_gentran.h"
#include "gentran_utils.h"
#include "../ireduce_scatter/ireduce_scatter_tsp_recexch_algos_prototypes.h"

/*
*/

int MPIR_Ireduce_scatter_allcomm_sched_auto(const void *sendbuf, void *recvbuf,
                                            const MPI_Aint * recvcounts, MPI_Datatype datatype,
                                            MPI_Op op, MPIR_Comm * comm_ptr,
                                            bool is_persistent, void **sched_p,
                                            enum MPIR_sched_type *sched_type_p)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_Csel_coll_sig_s coll_sig = {
        .coll_type = MPIR_CSEL_COLL_TYPE__IREDUCE_SCATTER,
        .comm_ptr = comm_ptr,

        .u.ireduce_scatter.sendbuf = sendbuf,
        .u.ireduce_scatter.recvbuf = recvbuf,
        .u.ireduce_scatter.recvcounts = recvcounts,
        .u.ireduce_scatter.datatype = datatype,
        .u.ireduce_scatter.op = op,
    };

    MPII_Csel_container_s *cnt = MPIR_Csel_search(comm_ptr->csel_comm, coll_sig);
    MPIR_Assert(cnt);

    switch (cnt->id) {
        /* *INDENT-OFF* */
        case MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Ireduce_scatter_intra_sched_auto:
            MPII_SCHED_CREATE_SCHED_P();
            mpi_errno = MPIR_Ireduce_scatter_intra_sched_auto(sendbuf, recvbuf, recvcounts,
                                                              datatype, op, comm_ptr, *sched_p);
            break;

        case MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Ireduce_scatter_intra_sched_noncommutative:
            MPII_SCHED_CREATE_SCHED_P();
            mpi_errno = MPIR_Ireduce_scatter_intra_sched_noncommutative(sendbuf, recvbuf,
                                                                        recvcounts, datatype, op,
                                                                        comm_ptr, *sched_p);
            break;

        case MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Ireduce_scatter_intra_sched_pairwise:
            MPII_SCHED_CREATE_SCHED_P();
            mpi_errno = MPIR_Ireduce_scatter_intra_sched_pairwise(sendbuf, recvbuf, recvcounts,
                                                                  datatype, op, comm_ptr, *sched_p);
            break;

        case MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Ireduce_scatter_intra_sched_recursive_doubling:
            MPII_SCHED_CREATE_SCHED_P();
            mpi_errno =
                MPIR_Ireduce_scatter_intra_sched_recursive_doubling(sendbuf, recvbuf, recvcounts,
                                                                    datatype, op, comm_ptr,
                                                                    *sched_p);
            break;

        case MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Ireduce_scatter_intra_sched_recursive_halving:
            MPII_SCHED_CREATE_SCHED_P();
            mpi_errno =
                MPIR_Ireduce_scatter_intra_sched_recursive_halving(sendbuf, recvbuf, recvcounts,
                                                                   datatype, op, comm_ptr,
                                                                   *sched_p);
            break;

        case MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Ireduce_scatter_intra_gentran_recexch:
            MPII_GENTRAN_CREATE_SCHED_P();
            mpi_errno =
                MPIR_TSP_Ireduce_scatter_sched_intra_recexch(sendbuf, recvbuf, recvcounts, datatype,
                                                             op, comm_ptr,
                                                             IREDUCE_SCATTER_RECEXCH_TYPE_DISTANCE_DOUBLING,
                                                             cnt->u.ireduce_scatter.
                                                             intra_gentran_recexch.k,
                                                             *sched_p);
            break;

        case MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Ireduce_scatter_inter_sched_auto:
            MPII_SCHED_CREATE_SCHED_P();
            mpi_errno = MPIR_Ireduce_scatter_inter_sched_auto(sendbuf, recvbuf, recvcounts,
                                                              datatype, op, comm_ptr, *sched_p);
            break;

        case MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Ireduce_scatter_inter_sched_remote_reduce_local_scatterv:
            MPII_SCHED_CREATE_SCHED_P();
            mpi_errno =
                MPIR_Ireduce_scatter_inter_sched_remote_reduce_local_scatterv(sendbuf, recvbuf,
                                                                              recvcounts, datatype,
                                                                              op, comm_ptr,
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

int MPIR_Ireduce_scatter_intra_sched_auto(const void *sendbuf, void *recvbuf,
                                          const MPI_Aint recvcounts[], MPI_Datatype datatype,
                                          MPI_Op op, MPIR_Comm * comm_ptr, MPIR_Sched_t s)
{
    int mpi_errno = MPI_SUCCESS;
    int i;
    int is_commutative;
    int total_count, type_size, nbytes;
    int comm_size;

    is_commutative = MPIR_Op_is_commutative(op);

    comm_size = comm_ptr->local_size;
    total_count = 0;
    for (i = 0; i < comm_size; i++) {
        total_count += recvcounts[i];
    }
    if (total_count == 0) {
        goto fn_exit;
    }
    MPIR_Datatype_get_size_macro(datatype, type_size);
    nbytes = total_count * type_size;

    /* select an appropriate algorithm based on commutivity and message size */
    if (is_commutative && (nbytes < MPIR_CVAR_REDUCE_SCATTER_COMMUTATIVE_LONG_MSG_SIZE)) {
        mpi_errno =
            MPIR_Ireduce_scatter_intra_sched_recursive_halving(sendbuf, recvbuf, recvcounts,
                                                               datatype, op, comm_ptr, s);
        MPIR_ERR_CHECK(mpi_errno);
    } else if (is_commutative && (nbytes >= MPIR_CVAR_REDUCE_SCATTER_COMMUTATIVE_LONG_MSG_SIZE)) {
        mpi_errno =
            MPIR_Ireduce_scatter_intra_sched_pairwise(sendbuf, recvbuf, recvcounts, datatype, op,
                                                      comm_ptr, s);
        MPIR_ERR_CHECK(mpi_errno);
    } else {    /* (!is_commutative) */

        int is_block_regular = TRUE;
        for (i = 0; i < (comm_size - 1); ++i) {
            if (recvcounts[i] != recvcounts[i + 1]) {
                is_block_regular = FALSE;
                break;
            }
        }

        if (MPL_is_pof2(comm_size, NULL) && is_block_regular) {
            /* noncommutative, pof2 size, and block regular */
            mpi_errno =
                MPIR_Ireduce_scatter_intra_sched_noncommutative(sendbuf, recvbuf, recvcounts,
                                                                datatype, op, comm_ptr, s);
            MPIR_ERR_CHECK(mpi_errno);
        } else {
            /* noncommutative and (non-pof2 or block irregular), use recursive doubling. */
            mpi_errno =
                MPIR_Ireduce_scatter_intra_sched_recursive_doubling(sendbuf, recvbuf, recvcounts,
                                                                    datatype, op, comm_ptr, s);
            MPIR_ERR_CHECK(mpi_errno);
        }
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIR_Ireduce_scatter_inter_sched_auto(const void *sendbuf, void *recvbuf,
                                          const MPI_Aint recvcounts[], MPI_Datatype datatype,
                                          MPI_Op op, MPIR_Comm * comm_ptr, MPIR_Sched_t s)
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno =
        MPIR_Ireduce_scatter_inter_sched_remote_reduce_local_scatterv(sendbuf, recvbuf, recvcounts,
                                                                      datatype, op, comm_ptr, s);

    return mpi_errno;
}

int MPIR_Ireduce_scatter_sched_auto(const void *sendbuf, void *recvbuf, const MPI_Aint recvcounts[],
                                    MPI_Datatype datatype, MPI_Op op, MPIR_Comm * comm_ptr,
                                    MPIR_Sched_t s)
{
    int mpi_errno = MPI_SUCCESS;

    if (comm_ptr->comm_kind == MPIR_COMM_KIND__INTRACOMM) {
        mpi_errno = MPIR_Ireduce_scatter_intra_sched_auto(sendbuf, recvbuf,
                                                          recvcounts, datatype, op, comm_ptr, s);
    } else {
        mpi_errno = MPIR_Ireduce_scatter_inter_sched_auto(sendbuf, recvbuf, recvcounts,
                                                          datatype, op, comm_ptr, s);
    }

    return mpi_errno;
}

int MPIR_Ireduce_scatter_sched_impl(const void *sendbuf, void *recvbuf, const MPI_Aint recvcounts[],
                                    MPI_Datatype datatype, MPI_Op op, MPIR_Comm * comm_ptr,
                                    bool is_persistent, void **sched_p,
                                    enum MPIR_sched_type *sched_type_p)
{
    int mpi_errno = MPI_SUCCESS;
    int is_commutative = MPIR_Op_is_commutative(op);

    /* If the user picks one of the transport-enabled algorithms, branch there
     * before going down to the MPIR_Sched-based algorithms. */
    /* TODO - Eventually the intention is to replace all of the
     * MPIR_Sched-based algorithms with transport-enabled algorithms, but that
     * will require sufficient performance testing and replacement algorithms. */
    if (comm_ptr->comm_kind == MPIR_COMM_KIND__INTRACOMM) {
        switch (MPIR_CVAR_IREDUCE_SCATTER_INTRA_ALGORITHM) {
            /* *INDENT-OFF* */
            case MPIR_CVAR_IREDUCE_SCATTER_INTRA_ALGORITHM_gentran_recexch:
                MPII_COLLECTIVE_FALLBACK_CHECK(comm_ptr->rank, is_commutative, mpi_errno,
                                               "Ireduce_scatter gentran_recexch cannot be applied.\n");
                MPII_GENTRAN_CREATE_SCHED_P();
                mpi_errno =
                    MPIR_TSP_Ireduce_scatter_sched_intra_recexch(sendbuf, recvbuf, recvcounts,
                                                                 datatype, op, comm_ptr,
                                                                 IREDUCE_SCATTER_RECEXCH_TYPE_DISTANCE_DOUBLING,
                                                                 MPIR_CVAR_IREDUCE_SCATTER_RECEXCH_KVAL,
                                                                 *sched_p);
                break;

            case MPIR_CVAR_IREDUCE_SCATTER_INTRA_ALGORITHM_sched_noncommutative:
                MPII_SCHED_CREATE_SCHED_P();
                mpi_errno = MPIR_Ireduce_scatter_intra_sched_noncommutative(sendbuf, recvbuf,
                                                                            recvcounts, datatype,
                                                                            op, comm_ptr, *sched_p);
                break;

            case MPIR_CVAR_IREDUCE_SCATTER_INTRA_ALGORITHM_sched_pairwise:
                MPII_SCHED_CREATE_SCHED_P();
                mpi_errno = MPIR_Ireduce_scatter_intra_sched_pairwise(sendbuf, recvbuf, recvcounts,
                                                                      datatype, op, comm_ptr,
                                                                      *sched_p);
                break;

            case MPIR_CVAR_IREDUCE_SCATTER_INTRA_ALGORITHM_sched_recursive_halving:
                MPII_SCHED_CREATE_SCHED_P();
                mpi_errno = MPIR_Ireduce_scatter_intra_sched_recursive_halving(sendbuf, recvbuf,
                                                                               recvcounts, datatype,
                                                                               op, comm_ptr,
                                                                               *sched_p);
                break;

            case MPIR_CVAR_IREDUCE_SCATTER_INTRA_ALGORITHM_sched_recursive_doubling:
                MPII_SCHED_CREATE_SCHED_P();
                mpi_errno = MPIR_Ireduce_scatter_intra_sched_recursive_doubling(sendbuf, recvbuf,
                                                                                recvcounts,
                                                                                datatype, op,
                                                                                comm_ptr, *sched_p);
                break;

            case MPIR_CVAR_IREDUCE_SCATTER_INTRA_ALGORITHM_sched_auto:
                MPII_SCHED_CREATE_SCHED_P();
                mpi_errno = MPIR_Ireduce_scatter_intra_sched_auto(sendbuf, recvbuf, recvcounts,
                                                                  datatype, op, comm_ptr, *sched_p);
                break;

            case MPIR_CVAR_IREDUCE_SCATTER_INTRA_ALGORITHM_auto:
                mpi_errno =
                    MPIR_Ireduce_scatter_allcomm_sched_auto(sendbuf, recvbuf, recvcounts, datatype,
                                                            op, comm_ptr, is_persistent, sched_p,
                                                            sched_type_p);
                break;

            default:
                MPIR_Assert(0);
            /* *INDENT-ON* */
        }
    } else {
        switch (MPIR_CVAR_IREDUCE_SCATTER_INTER_ALGORITHM) {
            /* *INDENT-OFF* */
            case MPIR_CVAR_IREDUCE_SCATTER_INTER_ALGORITHM_sched_remote_reduce_local_scatterv:
                MPII_SCHED_CREATE_SCHED_P();
                mpi_errno =
                    MPIR_Ireduce_scatter_inter_sched_remote_reduce_local_scatterv(sendbuf, recvbuf,
                                                                                  recvcounts,
                                                                                  datatype, op,
                                                                                  comm_ptr,
                                                                                  *sched_p);
                break;

            case MPIR_CVAR_IREDUCE_SCATTER_INTER_ALGORITHM_sched_auto:
                MPII_SCHED_CREATE_SCHED_P();
                mpi_errno = MPIR_Ireduce_scatter_inter_sched_auto(sendbuf, recvbuf, recvcounts,
                                                                  datatype, op, comm_ptr, *sched_p);
                break;

            case MPIR_CVAR_IREDUCE_SCATTER_INTER_ALGORITHM_auto:
                mpi_errno =
                    MPIR_Ireduce_scatter_allcomm_sched_auto(sendbuf, recvbuf, recvcounts, datatype,
                                                            op, comm_ptr, is_persistent, sched_p,
                                                            sched_type_p);
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
        mpi_errno = MPIR_Ireduce_scatter_intra_sched_auto(sendbuf, recvbuf, recvcounts, datatype,
                                                          op, comm_ptr, *sched_p);
    } else {
        MPII_SCHED_CREATE_SCHED_P();
        mpi_errno = MPIR_Ireduce_scatter_inter_sched_auto(sendbuf, recvbuf, recvcounts, datatype,
                                                          op, comm_ptr, *sched_p);
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIR_Ireduce_scatter_impl(const void *sendbuf, void *recvbuf, const MPI_Aint recvcounts[],
                              MPI_Datatype datatype, MPI_Op op, MPIR_Comm * comm_ptr,
                              MPIR_Request ** request)
{
    int mpi_errno = MPI_SUCCESS;

    *request = NULL;

    enum MPIR_sched_type sched_type;
    void *sched;
    mpi_errno = MPIR_Ireduce_scatter_sched_impl(sendbuf, recvbuf, recvcounts, datatype, op,
                                                comm_ptr, false, &sched, &sched_type);
    MPIR_ERR_CHECK(mpi_errno);

    MPII_SCHED_START(sched_type, sched, comm_ptr, request);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIR_Ireduce_scatter(const void *sendbuf, void *recvbuf, const MPI_Aint recvcounts[],
                         MPI_Datatype datatype, MPI_Op op, MPIR_Comm * comm_ptr,
                         MPIR_Request ** request)
{
    int mpi_errno = MPI_SUCCESS;
    void *in_recvbuf = recvbuf;
    void *host_sendbuf;
    void *host_recvbuf;
    int count = 0;

    for (int i = 0; i < MPIR_Comm_size(comm_ptr); i++)
        count += recvcounts[i];

    MPIR_Coll_host_buffer_alloc(sendbuf, recvbuf, count, datatype, &host_sendbuf, &host_recvbuf);
    if (host_sendbuf)
        sendbuf = host_sendbuf;
    if (host_recvbuf)
        recvbuf = host_recvbuf;

    if ((MPIR_CVAR_DEVICE_COLLECTIVES == MPIR_CVAR_DEVICE_COLLECTIVES_all) ||
        ((MPIR_CVAR_DEVICE_COLLECTIVES == MPIR_CVAR_DEVICE_COLLECTIVES_percoll) &&
         MPIR_CVAR_IREDUCE_SCATTER_DEVICE_COLLECTIVE)) {
        mpi_errno =
            MPID_Ireduce_scatter(sendbuf, recvbuf, recvcounts, datatype, op, comm_ptr, request);
    } else {
        mpi_errno = MPIR_Ireduce_scatter_impl(sendbuf, recvbuf, recvcounts, datatype, op, comm_ptr,
                                              request);
    }

    MPIR_Coll_host_buffer_swap_back(host_sendbuf, host_recvbuf, in_recvbuf, count, datatype,
                                    *request);

    return mpi_errno;
}
