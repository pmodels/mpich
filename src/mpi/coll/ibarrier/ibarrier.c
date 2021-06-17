/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"
/* for MPIR_TSP_sched_t */
#include "tsp_gentran.h"
#include "gentran_utils.h"
#include "../iallreduce/iallreduce_tsp_recexch_algos_prototypes.h"

/*
*/

/* any non-MPI functions go here, especially non-static ones */

int MPIR_Ibarrier_allcomm_sched_auto(MPIR_Comm * comm_ptr, bool is_persistent, void **sched_p,
                                     enum MPIR_sched_type *sched_type_p)
{
    int mpi_errno = MPI_SUCCESS;
    void *recvbuf = NULL;

    MPIR_Csel_coll_sig_s coll_sig = {
        .coll_type = MPIR_CSEL_COLL_TYPE__IBARRIER,
        .comm_ptr = comm_ptr,
    };

    MPII_Csel_container_s *cnt = MPIR_Csel_search(comm_ptr->csel_comm, coll_sig);
    MPIR_Assert(cnt);

    switch (cnt->id) {
        /* *INDENT-OFF* */
        case MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Ibarrier_intra_sched_auto:
            MPII_SCHED_CREATE_SCHED_P();
            mpi_errno = MPIR_Ibarrier_intra_sched_auto(comm_ptr, *sched_p);
            break;

        case MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Ibarrier_intra_sched_recursive_doubling:
            MPII_SCHED_CREATE_SCHED_P();
            mpi_errno = MPIR_Ibarrier_intra_sched_recursive_doubling(comm_ptr, *sched_p);
            break;

        case MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Ibarrier_intra_gentran_recexch:
            MPII_GENTRAN_CREATE_SCHED_P();
            mpi_errno =
                MPIR_TSP_Iallreduce_sched_intra_recexch(MPI_IN_PLACE, recvbuf, 0, MPI_BYTE, MPI_SUM,
                                                        comm_ptr,
                                                        MPIR_IALLREDUCE_RECEXCH_TYPE_MULTIPLE_BUFFER,
                                                        cnt->u.ibarrier.intra_gentran_recexch.k,
                                                        *sched_p);
            break;

        case MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Ibarrier_inter_sched_auto:
            MPII_SCHED_CREATE_SCHED_P();
            mpi_errno = MPIR_Ibarrier_inter_sched_auto(comm_ptr, *sched_p);
            break;

        case MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Ibarrier_inter_sched_bcast:
            MPII_SCHED_CREATE_SCHED_P();
            mpi_errno = MPIR_Ibarrier_inter_sched_bcast(comm_ptr, *sched_p);
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

int MPIR_Ibarrier_intra_sched_auto(MPIR_Comm * comm_ptr, MPIR_Sched_t s)
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = MPIR_Ibarrier_intra_sched_recursive_doubling(comm_ptr, s);

    return mpi_errno;
}

/* It will choose between several different algorithms based on the given
 * parameters. */
int MPIR_Ibarrier_inter_sched_auto(MPIR_Comm * comm_ptr, MPIR_Sched_t s)
{
    int mpi_errno;

    mpi_errno = MPIR_Ibarrier_inter_sched_bcast(comm_ptr, s);

    return mpi_errno;
}

int MPIR_Ibarrier_sched_auto(MPIR_Comm * comm_ptr, MPIR_Sched_t s)
{
    int mpi_errno = MPI_SUCCESS;

    if (comm_ptr->comm_kind == MPIR_COMM_KIND__INTRACOMM) {
        mpi_errno = MPIR_Ibarrier_intra_sched_auto(comm_ptr, s);
    } else {
        mpi_errno = MPIR_Ibarrier_inter_sched_auto(comm_ptr, s);
    }

    return mpi_errno;
}

int MPIR_Ibarrier_sched_impl(MPIR_Comm * comm_ptr, bool is_persistent, void **sched_p,
                             enum MPIR_sched_type *sched_type_p)
{
    int mpi_errno = MPI_SUCCESS;
    void *recvbuf = NULL;

    /* If the user picks one of the transport-enabled algorithms, branch there
     * before going down to the MPIR_Sched-based algorithms. */
    /* TODO - Eventually the intention is to replace all of the
     * MPIR_Sched-based algorithms with transport-enabled algorithms, but that
     * will require sufficient performance testing and replacement algorithms. */
    if (comm_ptr->comm_kind == MPIR_COMM_KIND__INTRACOMM) {
        /* intracommunicator */
        switch (MPIR_CVAR_IBARRIER_INTRA_ALGORITHM) {
            /* *INDENT-OFF* */
            case MPIR_CVAR_IBARRIER_INTRA_ALGORITHM_gentran_recexch:
                MPII_GENTRAN_CREATE_SCHED_P();
                mpi_errno =
                    MPIR_TSP_Iallreduce_sched_intra_recexch(MPI_IN_PLACE, recvbuf, 0, MPI_BYTE,
                                                            MPI_SUM, comm_ptr,
                                                            MPIR_IALLREDUCE_RECEXCH_TYPE_MULTIPLE_BUFFER,
                                                            MPIR_CVAR_IBARRIER_RECEXCH_KVAL,
                                                            *sched_p);
                break;

            case MPIR_CVAR_IBARRIER_INTRA_ALGORITHM_sched_recursive_doubling:
                MPII_SCHED_CREATE_SCHED_P();
                mpi_errno = MPIR_Ibarrier_intra_sched_recursive_doubling(comm_ptr, *sched_p);
                break;

            case MPIR_CVAR_IBARRIER_INTRA_ALGORITHM_sched_auto:
                MPII_SCHED_CREATE_SCHED_P();
                mpi_errno = MPIR_Ibarrier_intra_sched_auto(comm_ptr, *sched_p);
                break;

            case MPIR_CVAR_IBARRIER_INTRA_ALGORITHM_auto:
                mpi_errno = MPIR_Ibarrier_allcomm_sched_auto(comm_ptr, is_persistent, sched_p,
                                                             sched_type_p);
                break;

            default:
                MPIR_Assert(0);
            /* *INDENT-ON* */
        }
    } else {
        switch (MPIR_CVAR_IBARRIER_INTER_ALGORITHM) {
            /* *INDENT-OFF* */
            case MPIR_CVAR_IBARRIER_INTER_ALGORITHM_sched_bcast:
                MPII_SCHED_CREATE_SCHED_P();
                mpi_errno = MPIR_Ibarrier_inter_sched_bcast(comm_ptr, *sched_p);
                break;

            case MPIR_CVAR_IBARRIER_INTER_ALGORITHM_sched_auto:
                MPII_SCHED_CREATE_SCHED_P();
                mpi_errno = MPIR_Ibarrier_inter_sched_auto(comm_ptr, *sched_p);
                break;

            case MPIR_CVAR_IBARRIER_INTER_ALGORITHM_auto:
                mpi_errno = MPIR_Ibarrier_allcomm_sched_auto(comm_ptr, is_persistent, sched_p,
                                                             sched_type_p);
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

int MPIR_Ibarrier_impl(MPIR_Comm * comm_ptr, MPIR_Request ** request)
{
    int mpi_errno = MPI_SUCCESS;

    *request = NULL;

    enum MPIR_sched_type sched_type;
    void *sched;
    mpi_errno = MPIR_Ibarrier_sched_impl(comm_ptr, false, &sched, &sched_type);
    MPIR_ERR_CHECK(mpi_errno);

    MPII_SCHED_START(sched_type, sched, comm_ptr, request);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIR_Ibarrier(MPIR_Comm * comm_ptr, MPIR_Request ** request)
{
    int mpi_errno = MPI_SUCCESS;

    if ((MPIR_CVAR_DEVICE_COLLECTIVES == MPIR_CVAR_DEVICE_COLLECTIVES_all) ||
        ((MPIR_CVAR_DEVICE_COLLECTIVES == MPIR_CVAR_DEVICE_COLLECTIVES_percoll) &&
         MPIR_CVAR_IBARRIER_DEVICE_COLLECTIVE)) {
        mpi_errno = MPID_Ibarrier(comm_ptr, request);
    } else {
        mpi_errno = MPIR_Ibarrier_impl(comm_ptr, request);
    }

    return mpi_errno;
}
